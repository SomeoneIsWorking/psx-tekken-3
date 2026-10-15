#include "frame/fight_warp.h"

#include <lucent/log.h>

#include <charconv>

namespace tekken3::frame {
namespace {

std::optional<std::uint32_t> character(std::string_view word) {
  std::uint32_t value = 0;
  const auto parsed = std::from_chars(word.data(), word.data() + word.size(), value);
  if (parsed.ec != std::errc{} || parsed.ptr != word.data() + word.size() || value >= FightWarp::kCharacterCount) {
    return std::nullopt;
  }
  return value;
}

std::string_view nextWord(std::string_view &rest) {
  const std::size_t start = rest.find_first_not_of(' ');
  if (start == std::string_view::npos) {
    rest = {};
    return {};
  }
  rest.remove_prefix(start);
  const std::size_t end = rest.find(' ');
  const std::string_view word = rest.substr(0, end);
  rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end);
  return word;
}

} // namespace

std::string FightWarp::arm(std::string_view line) {
  std::string_view rest = line;
  nextWord(rest);
  const std::string_view firstWord = nextWord(rest);
  const std::string_view secondWord = nextWord(rest);
  if (firstWord.empty() || secondWord.empty() || !nextWord(rest).empty()) {
    return lucent::format(
        "usage: warp <p1 character 0..{}> <p2 character 0..{}>", kCharacterCount - 1, kCharacterCount - 1);
  }
  const auto first = character(firstWord);
  const auto second = character(secondWord);
  if (!first || !second) {
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
