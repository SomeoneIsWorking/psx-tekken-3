#include "frame/fight_warp.h"

#include <lucent/log.h>

namespace tekken3::frame {

std::string FightWarp::arm(const psx::debug::CommandArgs &args) {
  if (args.size() != 3) {
    return lucent::format(
        "usage: warp <p1 character 0..{}> <p2 character 0..{}>", kCharacterCount - 1, kCharacterCount - 1);
  }
  const auto first = args.number(1);
  const auto second = args.number(2);
  if (!first || !second || *first >= kCharacterCount || *second >= kCharacterCount) {
    return lucent::format("refused: characters are 0..{}", kCharacterCount - 1);
  }
  request_ = Request{*first, *second};
  started_ = false;
  return lucent::format("ok: VS warp armed, characters {} and {}", *first, *second);
}

void FightWarp::apply(Machine &machine) {
  if (!request_) {
    return;
  }
  const std::uint16_t mode = machine.read16(kMode);
  if (!started_ && mode == kMenuMode && machine.read16(kPhase) == kMenuChoosing) {
    machine.call3({kGameStart, kGameStartReturn}, {kVersusType, 0u, 0u});
    machine.call1({kMenuLeave, kMenuLeaveReturn}, 1u);
    started_ = true;
    lucent::info("tekken3-warp", "VS started from the main menu");
    return;
  }
  if (started_ && mode == kSelectMode && machine.read16(kPhase) == kSelectWaiting) {
    for (std::uint32_t player = 0; player < 2u; ++player) {
      const std::uint32_t chosen = player == 0u ? request_->first : request_->second;
      machine.write32(kSelection + player * kSelectionStride + kSelectionChoice, chosen << 2u);
    }
    machine.write16(kPhase, kSelectExit);
    lucent::info("tekken3-warp", "character select taken: {} vs {}", request_->first, request_->second);
    request_.reset();
    started_ = false;
  }
}

} // namespace tekken3::frame
