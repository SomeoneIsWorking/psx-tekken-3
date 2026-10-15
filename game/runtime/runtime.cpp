#include "runtime/runtime.h"

#include "cd/sector_ready_order.h"
#include "core.h"
#include "fieldclock/field_clock.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "render/gpu_queue_timeout.h"
#include "runtime/session.h"

#include <cstring>

namespace tekken3 {
namespace {

// FUN_800B0B9C passes each 42-byte pad record's +2 receive buffer to InitPAD (FUN_800946A8).
constexpr psx::input::StockPadBuffers::Layout kPadBuffers{
    .slot0Buffer = 0x800A9132u,
    .slot1Buffer = 0x800A915Cu,
};

} // namespace

// Every VSync outside the overridden display init passes a negative mode, a field-count query.
Runtime::Runtime() : vsync_(field::kEntry, field::kCounter), pad_(kPadBuffers) {}

void Runtime::startProgram(Core &core) {
  frame::FrameDriver::from(core).runBootPrefix(core);
}

void Runtime::registerOverrides(Game &game) {
  vsync_.bind(game);
  frame::FrameDriver::from(game.core).installOverrides();
  cd::installOverrides(game.core);
  render::installOverrides(game.core);
  projection_.install(game.core);
}

std::unique_ptr<psx::game::TitleSessionState> Runtime::createSession(Game &) {
  return std::make_unique<Session>();
}

std::unique_ptr<psx::game::FrameDriver> Runtime::createFrameDriver(Game &game) {
  return std::make_unique<frame::FrameDriver>(game);
}

void Runtime::deliverPad(Core &core, const psx::input::PadDelivery &delivery) {
  pad_.deliver(core, delivery);
}

const GuestWidescreenProjection *Runtime::guestWidescreenProjection() const {
  return &projection_;
}

RenderCapabilities Runtime::renderCapabilities() const {
  // The picture is the guest's GP0 output replayed from the frame record; native 60 fps, so no in-betweens.
  return RenderCapabilities{
      .defaultPath = RenderPath::Record,
      .nativeRenderPath = false,
  };
}

bool Runtime::guestVramIsPicture(const Game &) const {
  return false;
}

bool Runtime::titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) {
  if (std::strcmp(input.command, "warp") != 0) {
    return false;
  }
  std::fprintf(reply, "%s\n", frame::FrameDriver::from(core).armWarp(input.line).c_str());
  return true;
}

} // namespace tekken3
