#include "debug/debug_menu.h"

#include "core.h"
#include "frame/fight_warp.h"
#include "frame/frame_driver.h"
#include "game.h"

namespace tekken3 {

DebugMenu::DebugMenu() {
  using frame::FightWarp;
  using psx::game::DebugCall;
  using psx::game::DebugParam;
  section("Fight Warp");
  verb({
      .name = "warp",
      .summary = "Start that VS fight",
      .params = {DebugParam::integer("p1", 0, FightWarp::kCharacterCount - 1),
                 DebugParam::integer("p2", 0, FightWarp::kCharacterCount - 1)},
      .run =
          [](const DebugCall &call) {
            call.reply =
                call.core.game->frameDriverAs<frame::FrameDriver>().armWarp(call.values.u32(0), call.values.u32(1));
          },
      .refusal =
          [](Core &core) {
            return core.game->frameDriverIf<frame::FrameDriver>() != nullptr ? std::string()
                                                                             : "the game loop is not running";
          },
  });
}

} // namespace tekken3
