#pragma once

#include "game_runtime.h"
#include "stock_pad_buffers.h"
#include "stock_vsync.h"
#include "widescreen/widescreen.h"

#include <cstdio>
#include <memory>

class Core;
class Game;

namespace tekken3 {

// Tekken 3 (SLUS_004.02) on Record at its native 60 fps: the linked libetc VSync and libpad buffers bound at
// this executable's addresses, the finite frame driver, the widened projection and the `warp` command.
class Runtime final : public psx::game::GameRuntime {
public:
  Runtime();

  // Dispatches the retail entry until the frame driver's native main takes the loop.
  void startProgram(Core &core) override;
  void registerOverrides(Game &game) override;
  std::unique_ptr<psx::game::TitleSessionState> createSession(Game &game) override;
  std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game) override;
  void deliverPad(Core &core, const psx::input::PadDelivery &delivery) override;

  const GuestWidescreenProjection *guestWidescreenProjection() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &game) const override;

  // `warp <p1> <p2>`.
  bool titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) override;

private:
  psx::hle::StockVSync vsync_;
  psx::input::StockPadBuffers pad_;
  widescreen::WidescreenProjection projection_;
};

} // namespace tekken3
