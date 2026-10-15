// `warp <p1> <p2>`: the menu's own VS game start, then character select's own exit with both choices written.

#include "frame/fight_warp.h"
#include "recording_machine.h"

#include <cstdint>
#include <cstdio>
#include <string>

namespace {

using tekken3::frame::FightWarp;
using tekken3::test::Operation;
using tekken3::test::RecordingMachine;

int failures = 0;

void check(bool condition, const char *detail) {
  if (!condition) {
    std::printf("FAIL: %s\n", detail);
    ++failures;
  }
}

bool startsWith(const std::string &text, const char *prefix) {
  return text.rfind(prefix, 0) == 0;
}

struct Screen {
  std::uint16_t mode = 0;
  std::uint16_t phase = 0;
};

void setScreen(RecordingMachine &machine, Screen screen) {
  machine.memory[FightWarp::kMode] = screen.mode;
  machine.memory[FightWarp::kPhase] = screen.phase;
}

void armingParsesTwoCharacters() {
  FightWarp warp;
  check(startsWith(warp.arm("warp"), "usage:") && !warp.armed(), "a bare warp armed");
  check(startsWith(warp.arm("warp 3"), "usage:") && !warp.armed(), "one character armed");
  check(startsWith(warp.arm("warp 1 2 3"), "usage:") && !warp.armed(), "three characters armed");
  check(startsWith(warp.arm("warp 22 0"), "refused:") && !warp.armed(), "character 22 armed");
  check(startsWith(warp.arm("warp x 1"), "refused:") && !warp.armed(), "a non-number armed");
  check(startsWith(warp.arm("warp 21 0"), "ok:") && warp.armed(), "warp 21 0 did not arm");
}

void unarmedWarpDoesNothing() {
  FightWarp warp;
  RecordingMachine machine;
  setScreen(machine, {FightWarp::kMenuMode, FightWarp::kMenuChoosing});
  warp.apply(machine);
  check(machine.operations.empty(), "an unarmed warp touched the guest");
}

void menuStartsVersusOnce() {
  FightWarp warp;
  warp.arm("warp 5 9");
  RecordingMachine machine;
  setScreen(machine, {8u, 8u});
  warp.apply(machine);
  check(machine.operations.empty(), "the warp acted during a fight");
  setScreen(machine, {3u, 0});
  warp.apply(machine);
  check(machine.operations.empty(), "the warp replayed the menu's call from the attract title");
  setScreen(machine, {FightWarp::kMenuMode, 0});
  warp.apply(machine);
  check(machine.operations.empty(), "the warp started a game before the menu built its list");

  setScreen(machine, {FightWarp::kMenuMode, FightWarp::kMenuChoosing});
  warp.apply(machine);
  warp.apply(machine);
  check(machine.operations.size() == 2u, "the menu game start did not run exactly once");
  if (machine.operations.size() == 2u) {
    const Operation &start = machine.operations[0];
    check(start.kind == "call3" && start.address == 0x800DA910u && start.returnPc == 0x800DBA90u && start.a0 == 1u &&
              start.a1 == 0u,
          "the game start is not the title overlay's VS start from pad side 0");
    const Operation &leave = machine.operations[1];
    check(leave.kind == "call1" && leave.address == 0x8007C30Cu && leave.returnPc == 0x800DBA98u && leave.a0 == 1u,
          "the menu did not leave through 0x8007C30C(1)");
  }
}

void selectTakesBothChoicesAndItsOwnExit() {
  FightWarp warp;
  warp.arm("warp 5 9");
  RecordingMachine machine;
  setScreen(machine, {FightWarp::kSelectMode, FightWarp::kSelectWaiting});
  warp.apply(machine);
  check(machine.operations.empty(), "character select was taken without the menu's game start");

  setScreen(machine, {FightWarp::kMenuMode, FightWarp::kMenuChoosing});
  warp.apply(machine);
  machine.operations.clear();
  setScreen(machine, {FightWarp::kSelectMode, 1u});
  warp.apply(machine);
  check(machine.operations.empty(), "character select was taken before it waited for its players");

  setScreen(machine, {FightWarp::kSelectMode, FightWarp::kSelectWaiting});
  warp.apply(machine);
  check(machine.read32(0x800B8DA8u) == 5u << 2u, "player 1's choice is not character 5, costume 0");
  check(machine.read32(0x800B8E54u) == 9u << 2u, "player 2's choice is not character 9, costume 0");
  check(machine.read16(FightWarp::kPhase) == FightWarp::kSelectExit, "select did not move to its copy phase");
  check(!machine.operations.empty() && machine.operations.back().kind == "write16",
        "the phase was not written after the choices");
  check(!warp.armed(), "the warp stayed armed after select");

  const std::size_t done = machine.operations.size();
  setScreen(machine, {FightWarp::kMenuMode, FightWarp::kMenuChoosing});
  warp.apply(machine);
  check(machine.operations.size() == done, "a spent warp started another game");
}

} // namespace

int main() {
  armingParsesTwoCharacters();
  unarmedWarpDoesNothing();
  menuStartsVersusOnce();
  selectTakesBothChoicesAndItsOwnExit();
  std::printf("fight_warp_contract: %s\n", failures == 0 ? "PASS" : "FAIL");
  return failures == 0 ? 0 : 1;
}
