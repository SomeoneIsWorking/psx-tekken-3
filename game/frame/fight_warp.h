#pragma once

#include "frame/finite_frame.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace tekken3::frame {

// `warp <p1> <p2>`: a VS fight between two chosen characters, on the stage VS mode picks for them.
// From the main menu the warp makes the menu's own game-start call (title overlay 0x800DA910 with game type VS,
// which enters mode 7), and when character select (mode 10) waits for its players it writes both choices and
// takes the select's own exit (phase 3, FUN_80053FAC), so mode 8 builds the fight from them.
class FightWarp {
public:
  // Frame mode word and its phase (FUN_8004F8F8, FUN_80052C30).
  static constexpr std::uint32_t kMode = 0x800AE204u;
  static constexpr std::uint32_t kPhase = 0x800AE224u;
  // Title overlay main menu (0x800DB5B4); phase 1 takes the player's choice, phase 2 starts the chosen game.
  static constexpr std::uint16_t kMenuMode = 4u;
  static constexpr std::uint16_t kMenuChoosing = 1u;
  // Mode 10 (0x80055590): phase 2 waits on both players' FUN_80053000, phase 3 copies the choices.
  static constexpr std::uint16_t kSelectMode = 10u;
  static constexpr std::uint16_t kSelectWaiting = 2u;
  static constexpr std::uint16_t kSelectExit = 3u;
  // Menu phase 2: title overlay game start (a0 game type, a1 pad side) at 0x800DBA88, then 0x8007C30C(1).
  static constexpr std::uint32_t kGameStart = 0x800DA910u;
  static constexpr std::uint32_t kGameStartReturn = 0x800DBA90u;
  static constexpr std::uint32_t kMenuLeave = 0x8007C30Cu;
  static constexpr std::uint32_t kMenuLeaveReturn = 0x800DBA98u;
  // Game type 1 (0x800AFA88): both sides human (0x800DAB3C).
  static constexpr std::uint32_t kVersusType = 1u;
  // Mode 10's selection block 0x800B8D58: per player, 0xAC bytes apart, character << 2 | costume at +0x50.
  static constexpr std::uint32_t kSelection = 0x800B8D58u;
  static constexpr std::uint32_t kSelectionStride = 0xACu;
  static constexpr std::uint32_t kSelectionChoice = 0x50u;
  // FUN_8004F3B0 takes characters 0..21; 22 and above select the fallback.
  static constexpr std::uint32_t kCharacterCount = 22u;

  struct Request {
    std::uint32_t first = 0;
    std::uint32_t second = 0;
  };

  // Parses one command line; the returned text answers the client.
  std::string arm(std::string_view line);
  [[nodiscard]] bool armed() const {
    return request_.has_value();
  }
  // At a field boundary with no guest call in flight: starts VS from the menu, or completes character select.
  void apply(Machine &machine);

private:
  std::optional<Request> request_;
  bool started_ = false;
};

} // namespace tekken3::frame
