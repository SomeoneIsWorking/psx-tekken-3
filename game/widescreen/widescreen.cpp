#include "widescreen/widescreen.h"

#include "core.h"
#include "game.h"
#include "guest_widescreen_projection.h"
#include "mods.h"
#include "native_dispatch.h"
#include "runtime/session.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <lucent/log.h>

namespace tekken3::widescreen {
namespace {

constexpr std::uint32_t kViewDimensions = 0x80080A40u;
constexpr std::uint32_t kStageClip = 0x8006CC28u;
constexpr std::uint32_t kEffectClip = 0x8006E44Cu;
constexpr std::uint32_t kScratch = 0x1F800000u;
constexpr std::uint32_t kBootViewWidth = 384u;
constexpr std::uint32_t kBootViewHeight = 480u;
constexpr std::uint32_t kBootDrawWidth = 368u;
constexpr std::uint32_t kAlternateViewWidth = 320u;
constexpr std::uint32_t kAlternateViewHeight = 240u;
constexpr std::uint32_t kAlternateDrawWidth = 320u;

void originalViewDimensions(Core *core) {
  psx::cpu::callOriginalToReturn(
      *core, kViewDimensions, psx::cpu::ExecutionBudget::currentTurn(*core), "Tekken3::viewDimensions original");
}

void originalStageClip(Core *core) {
  psx::cpu::callOriginalToReturn(
      *core, kStageClip, psx::cpu::ExecutionBudget::currentTurn(*core), "Tekken3::stageClip original");
}

void originalEffectClip(Core *core) {
  psx::cpu::callOriginalToReturn(
      *core, kEffectClip, psx::cpu::ExecutionBudget::currentTurn(*core), "Tekken3::effectClip original");
}

std::int16_t x(std::uint32_t packed) {
  return static_cast<std::int16_t>(packed & 0xFFFFu);
}

std::int16_t y(std::uint32_t packed) {
  return static_cast<std::int16_t>(packed >> 16u);
}

std::uint32_t scratchVertex(Core &core, std::uint32_t indices, std::uint32_t index) {
  const std::uint32_t vertexIndex = (indices >> (index * 8u)) & 0xFFu;
  return core.mem_r32(kScratch + vertexIndex * 4u);
}

// The guest columns the presented canvas shows; on Record the canvas adds margins beside the retail draw width.
struct CanvasColumns {
  int left = 0;
  int right = 0;
};

CanvasColumns canvasColumns(const psx::present::GuestProjectionPlan &plan) {
  const int margin = (plan.presentationExtent.width - plan.guestDrawWidth) / 2;
  return {-margin, plan.guestDrawWidth + margin};
}

template <std::size_t Count>
bool visible(const std::array<std::uint32_t, Count> &vertices, const CanvasColumns &columns) {
  bool everyAbove = true;
  bool everyLeft = true;
  bool anyInsideRight = false;
  for (const std::uint32_t vertex : vertices) {
    everyAbove = everyAbove && y(vertex) < 0;
    everyLeft = everyLeft && x(vertex) < columns.left;
    anyInsideRight = anyInsideRight || x(vertex) < columns.right;
  }
  return !everyAbove && !everyLeft && anyInsideRight;
}

void writePacketWord(Core &core, std::uint32_t packet, std::uint32_t index, std::uint32_t value) {
  core.mem_w32(packet + index * 4u, value);
}

void linkPacket(Core &core, std::uint32_t packet, std::uint32_t orderingTable, std::uint32_t words) {
  writePacketWord(core, packet, 0, (core.mem_r32(orderingTable) & 0x00FFFFFFu) | (words << 24u));
  core.mem_w32(orderingTable, packet & 0x00FFFFFFu);
}

} // namespace

const WidescreenProjection &policyFrom(Core *core, const char *owner) {
  const auto *const policy = dynamic_cast<const WidescreenProjection *>(core->runtime.guestWidescreenProjection());
  if (policy == nullptr) {
    lucent::error("wide", "Tekken 3 {} override reached another title's policy", owner);
    std::abort();
  }
  return *policy;
}

WidescreenProjection::WidescreenProjection()
    : latch_(psx::present::latchGuestProjection), retailDimensions_(originalViewDimensions),
      retailStageClip_(originalStageClip), retailEffectClip_(originalEffectClip) {}

WidescreenProjection::WidescreenProjection(ProjectionLatch latch, GuestBody retailDimensions)
    : latch_(latch), retailDimensions_(retailDimensions), retailStageClip_(originalStageClip),
      retailEffectClip_(originalEffectClip) {
  if (latch_ == nullptr || retailDimensions_ == nullptr) {
    lucent::error("wide", "Tekken 3 projection owner requires the plan latch and the retail dimension body");
    std::abort();
  }
}

psx::present::GuestProjectionGeometry WidescreenProjection::measuredGeometry(std::uint32_t viewWidth,
                                                                             std::uint32_t viewHeight) {
  if (viewWidth == kBootViewWidth && viewHeight == kBootViewHeight) {
    return {{static_cast<int>(viewWidth), static_cast<int>(viewHeight)}, static_cast<int>(kBootDrawWidth)};
  }
  if (viewWidth == kAlternateViewWidth && viewHeight == kAlternateViewHeight) {
    return {{static_cast<int>(viewWidth), static_cast<int>(viewHeight)}, static_cast<int>(kAlternateDrawWidth)};
  }
  if (viewWidth == 0 || viewHeight == 0 || viewWidth > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
      viewHeight > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
    lucent::error("wide", "Tekken 3 received invalid view dimensions {}x{}", viewWidth, viewHeight);
    std::abort();
  }

  // FUN_800809D8 can publish non-preset render targets; keep their authored width.
  return {{static_cast<int>(viewWidth), static_cast<int>(viewHeight)}, static_cast<int>(viewWidth)};
}

void WidescreenProjection::install(Core &core) const {
  psx::cpu::installNativeOverride(core, kViewDimensions, "Tekken3::viewDimensions", publishDimensionsOverride);
  psx::cpu::installNativeOverride(core, kStageClip, "Tekken3::stageClip", stageClipOverride);
  psx::cpu::installNativeOverride(core, kEffectClip, "Tekken3::effectClip", effectClipOverride);
  stageWedge_.install(core);
}

const StageTileWedge &WidescreenProjection::stageWedge() const {
  return stageWedge_;
}

void WidescreenProjection::publishDimensions(Core &core, psx::present::GuestProjectionPlan &plan) const {
  const psx::present::GuestProjectionGeometry native = measuredGeometry(core.r[4], core.r[5]);
  plan = latch_(core, native);
  if (plan.projectionExtent.width == 0 || plan.projectionExtent.width > std::numeric_limits<std::uint16_t>::max()) {
    lucent::error("wide", "Tekken 3 received invalid projected width {}", plan.projectionExtent.width);
    std::abort();
  }

  lucent::info("wide",
               "Tekken 3 view {}x{} / draw {} -> view {}x{} / draw {}",
               native.extent.width,
               native.extent.height,
               native.drawWidth,
               plan.projectionExtent.width,
               plan.projectionExtent.height,
               plan.guestDrawWidth);
  core.r[4] = static_cast<std::uint32_t>(plan.projectionExtent.width);
  core.r[5] = static_cast<std::uint32_t>(plan.projectionExtent.height);
  retailDimensions_(&core);
}

void WidescreenProjection::clipStagePrimitives(Core &core, const psx::present::GuestProjectionPlan &plan) const {
  if (!plan.widescreen()) {
    retailStageClip_(&core);
    return;
  }

  std::uint32_t command = core.r[4];
  std::uint32_t packet = core.r[5];
  const std::uint32_t orderingTable = core.r[6];
  const std::uint32_t state = core.r[7];
  std::int32_t inputRemaining = static_cast<std::int32_t>(core.mem_r32(state));
  std::int32_t capacityRemaining = static_cast<std::int32_t>(core.mem_r32(state + 4u));
  std::uint32_t producedBytes = 0;
  const CanvasColumns columns = canvasColumns(plan);

  while (inputRemaining > 0 && capacityRemaining > 0) {
    const std::uint32_t opcode = core.mem_r32(command) & 0xFF000000u;
    std::uint32_t commandWords = 0;
    std::uint32_t packetWords = 0;
    std::array<std::uint32_t, 4> vertices{};

    if (opcode == 0x2C000000u) {
      const std::uint32_t indices = core.mem_r32(command + 16u);
      for (std::uint32_t index = 0; index < 4; ++index) {
        vertices[index] = scratchVertex(core, indices, index);
      }
      commandWords = 5;
      if (visible(vertices, columns)) {
        packetWords = 10;
        linkPacket(core, packet, orderingTable, 9);
        writePacketWord(core, packet, 1, core.mem_r32(command));
        writePacketWord(core, packet, 2, vertices[0]);
        writePacketWord(core, packet, 3, core.mem_r32(command + 4u));
        writePacketWord(core, packet, 4, vertices[1]);
        writePacketWord(core, packet, 5, core.mem_r32(command + 8u));
        writePacketWord(core, packet, 6, vertices[2]);
        writePacketWord(core, packet, 7, core.mem_r16(command + 12u));
        writePacketWord(core, packet, 8, vertices[3]);
        writePacketWord(core, packet, 9, core.mem_r16(command + 14u));
      }
    } else if (opcode == 0x24000000u) {
      const std::uint32_t indices = core.mem_r32(command + 16u);
      for (std::uint32_t index = 0; index < 3; ++index) {
        vertices[index] = scratchVertex(core, indices, index);
      }
      commandWords = 5;
      const std::array<std::uint32_t, 3> triangle{vertices[0], vertices[1], vertices[2]};
      if (visible(triangle, columns)) {
        packetWords = 8;
        linkPacket(core, packet, orderingTable, 7);
        writePacketWord(core, packet, 1, core.mem_r32(command));
        writePacketWord(core, packet, 2, vertices[0]);
        writePacketWord(core, packet, 3, core.mem_r32(command + 4u));
        writePacketWord(core, packet, 4, vertices[1]);
        writePacketWord(core, packet, 5, core.mem_r32(command + 8u));
        writePacketWord(core, packet, 6, vertices[2]);
        writePacketWord(core, packet, 7, core.mem_r16(command + 12u));
      }
    } else if (opcode == 0x28000000u) {
      const std::uint32_t indices = core.mem_r32(command + 4u);
      for (std::uint32_t index = 0; index < 4; ++index) {
        vertices[index] = scratchVertex(core, indices, index);
      }
      commandWords = 2;
      if (visible(vertices, columns)) {
        packetWords = 6;
        linkPacket(core, packet, orderingTable, 5);
        writePacketWord(core, packet, 1, core.mem_r32(command));
        for (std::uint32_t index = 0; index < 4; ++index) {
          writePacketWord(core, packet, index + 2u, vertices[index]);
        }
      }
    }

    if (commandWords != 0) {
      command += commandWords * 4u;
    }
    if (packetWords != 0) {
      packet += packetWords * 4u;
      producedBytes += packetWords * 4u;
      --capacityRemaining;
    }
    --inputRemaining;
  }

  core.mem_w32(state, producedBytes);
  core.mem_w32(state + 4u, static_cast<std::uint32_t>(capacityRemaining));
  core.r[2] = static_cast<std::uint32_t>(capacityRemaining);
}

void WidescreenProjection::clipEffectPrimitive(Core &core, const psx::present::GuestProjectionPlan &plan) const {
  if (!plan.widescreen()) {
    retailEffectClip_(&core);
    return;
  }

  const std::uint32_t command = core.r[4];
  const std::uint32_t packet = core.r[5];
  const std::uint32_t orderingTables = core.r[6];
  const std::uint32_t context = core.r[7];
  const std::array<std::uint32_t, 4> vertices{
      core.mem_r32(kScratch),
      core.mem_r32(kScratch + 4u),
      core.mem_r32(kScratch + 8u),
      core.mem_r32(kScratch + 12u),
  };
  std::int32_t orderingTableIndex = static_cast<std::int32_t>((core.mem_r32(command) >> 21u) & 0x3FFu);
  if ((core.mem_r32(context + 12u) & 0x2000u) != 0) {
    orderingTableIndex -= 5;
  }
  const CanvasColumns columns = canvasColumns(plan);
  if (x(vertices[0]) > columns.right || x(vertices[1]) < columns.left || y(vertices[0]) > 468 || y(vertices[2]) < 20 ||
      orderingTableIndex < 0) {
    return;
  }

  const std::uint32_t orderingTable = orderingTables + static_cast<std::uint32_t>(orderingTableIndex) * 4u;
  linkPacket(core, packet, orderingTable, 9);
  writePacketWord(core, packet, 1, core.mem_r32(command));
  for (std::uint32_t index = 0; index < 4; ++index) {
    writePacketWord(core, packet, index * 2u + 2u, vertices[index]);
  }
}

void WidescreenProjection::publishDimensionsOverride(Core *core) {
  policyFrom(core, "projection").publishDimensions(*core, session(*core).projectionPlan);
}

void WidescreenProjection::stageClipOverride(Core *core) {
  policyFrom(core, "stage clip").clipStagePrimitives(*core, session(*core).projectionPlan);
}

void WidescreenProjection::effectClipOverride(Core *core) {
  policyFrom(core, "effect clip").clipEffectPrimitive(*core, session(*core).projectionPlan);
}

} // namespace tekken3::widescreen
