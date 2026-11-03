#include "debug/debug_menu.h"

#include "core.h"
#include "frame/fight_warp.h"
#include "frame/frame_driver.h"
#include "game.h"

#include <lucent/log.h>

#include <vector>

namespace tekken3 {

DebugMenu::DebugMenu() {
  using frame::FightWarp;
  warp({
      .levels = {"p1", "p2"},
      .places =
          [](Core &) {
            std::vector<psx::game::WarpPlace> fighters;
            for (std::uint32_t p1 = 0; p1 < FightWarp::kCharacterCount; ++p1) {
              psx::game::WarpPlace place{.name = lucent::format("character {}", p1)};
              for (std::uint32_t p2 = 0; p2 < FightWarp::kCharacterCount; ++p2) {
                place.places.push_back({
                    .name = lucent::format("vs character {}", p2),
                    .arrive =
                        [p1, p2](Core &core) {
                          return core.game->frameDriverAs<frame::FrameDriver>().armWarp(p1, p2);
                        },
                });
              }
              fighters.push_back(std::move(place));
            }
            return fighters;
          },
      .refusal =
          [](Core &core) {
            return core.game->frameDriverIf<frame::FrameDriver>() != nullptr ? std::string()
                                                                             : "the game loop is not running";
          },
  });
}

} // namespace tekken3
