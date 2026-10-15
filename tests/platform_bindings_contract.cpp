// What Runtime::registerOverrides binds at SLUS_004.02's addresses and what the runtime declares to the framework.

#include "core.h"
#include "fieldclock/field_clock.h"
#include "game.h"
#include "platform_hle.h"
#include "psx_exe_image.h"
#include "runtime/runtime.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char *detail) {
  if (!condition) {
    std::printf("FAIL: %s\n", detail);
    ++failures;
  }
}

constexpr std::uint32_t kPadSlot0 = 0x800A9132u;
constexpr std::uint32_t kPadSlot1 = 0x800A915Cu;

// Overrides attach only to code an admitted image owns: a blank text over the resident range they name.
constexpr std::uint32_t kTextBase = 0x80010000u;
constexpr std::uint32_t kTextBytes = 0x000B0000u;

void word(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
  for (unsigned index = 0; index < 4; ++index) {
    bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8u));
  }
}

std::vector<std::uint8_t> residentText() {
  std::vector<std::uint8_t> bytes(psx::cpu::kPsxExeHeaderBytes + kTextBytes);
  std::memcpy(bytes.data(), "PS-X EXE", 8);
  word(bytes, 0x10, kTextBase);
  word(bytes, 0x18, kTextBase);
  word(bytes, 0x1c, kTextBytes);
  return bytes;
}

} // namespace

int main() {
  tekken3::Runtime runtime;
  auto game = std::make_unique<Game>(runtime, psx::config::Config{});
  Core &core = game->core;
  const auto loaded = psx::cpu::loadPsxExeImage(core, residentText(), "blank resident text");
  if (!loaded) {
    std::printf("FAIL: blank resident text rejected: %s\n", loaded.detail.c_str());
    return 1;
  }
  runtime.registerOverrides(*game);

  check(game->platform_hle.vsyncAddress() == tekken3::field::kEntry, "VSync is not bound at the libetc entry");
  if (const OverrideFn vsync = game->platform_hle.lookup(tekken3::field::kEntry)) {
    core.mem_w32(tekken3::field::kCounter, 73u);
    core.r[4] = static_cast<std::uint32_t>(-1);
    core.r[2] = 0xDEADBEEFu;
    vsync(&core);
    check(core.r[2] == 73u, "VSync(-1) did not return libetc's field count");
  } else {
    check(false, "the VSync entry has no override");
  }

  psx::input::PadDelivery delivery;
  delivery.packet = {0x00u, 0x41u, 0xF7u, 0xFFu};
  runtime.deliverPad(core, delivery);
  check(core.mem_r8(kPadSlot0) == 0x00u && core.mem_r8(kPadSlot0 + 1u) == 0x41u &&
            core.mem_r8(kPadSlot0 + 2u) == 0xF7u && core.mem_r8(kPadSlot0 + 3u) == 0xFFu,
        "the pad packet did not reach libpad's slot 0 receive buffer");
  check(core.mem_r8(kPadSlot1) == 0xFFu, "a disconnected slot 1 was not marked empty");

  check(core.rsub.mode.path() == RenderPath::Record, "Tekken does not present on the record path");
  check(!runtime.interpolates(), "Tekken runs at 60 fps and must not interpolate");
  check(runtime.guestWidescreenProjection() != nullptr, "the guest projection owner is missing");

  std::printf("tekken3 platform bindings: %s\n", failures == 0 ? "PASS" : "FAIL");
  return failures == 0 ? 0 : 1;
}
