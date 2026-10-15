// FrameDriver::stepFrame on a recording machine: boot gate, field index, and where an armed warp may act.

#include "core.h"
#include "frame/fight_warp.h"
#include "frame/finite_frame.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "psx_exe_image.h"
#include "recording_machine.h"
#include "runtime/runtime.h"

#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace {

using tekken3::frame::FightWarp;
using tekken3::frame::FiniteFrame;
using tekken3::frame::FrameDriver;
using tekken3::test::Operation;
using tekken3::test::RecordingMachine;

int failures = 0;

void check(bool condition, const char *detail) {
  if (!condition) {
    std::printf("FAIL: %s\n", detail);
    ++failures;
  }
}

// The three overrides attach only to code an admitted image owns.
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

std::ptrdiff_t indexOf(const std::vector<Operation> &operations, const char *kind, std::uint32_t address) {
  const auto found = std::find_if(operations.begin(), operations.end(), [&](const Operation &operation) {
    return operation.kind == kind && operation.address == address;
  });
  return found == operations.end() ? -1 : found - operations.begin();
}

bool warpActed(const RecordingMachine &machine) {
  return indexOf(machine.operations, "call3", FightWarp::kGameStart) >= 0;
}

void showMenu(RecordingMachine &machine) {
  machine.memory[FightWarp::kMode] = FightWarp::kMenuMode;
  machine.memory[FightWarp::kPhase] = FightWarp::kMenuChoosing;
}

struct Fixture {
  tekken3::Runtime runtime;
  std::unique_ptr<Game> game = std::make_unique<Game>(runtime, psx::config::Config{});
  RecordingMachine machine;
  FrameDriver *driver = nullptr;

  bool boot() {
    Core &core = game->core;
    const auto loaded = psx::cpu::loadPsxExeImage(core, residentText(), "blank resident text");
    if (!loaded) {
      std::printf("FAIL: blank resident text rejected: %s\n", loaded.detail.c_str());
      return false;
    }
    auto owned = std::make_unique<FrameDriver>(*game, machine);
    driver = owned.get();
    game->frameDriver = std::move(owned);
    driver->installOverrides();
    core.pc = FiniteFrame::kMain;
    core.r[29] = 0x801FFF00u;
    driver->runBootPrefix(core);
    return true;
  }
};

// A field before the boot prefix completes is a sequencing defect, not a field to run.
void fieldBeforeBootAborts() {
  const pid_t child = fork();
  if (child == 0) {
    Fixture fixture;
    auto owned = std::make_unique<FrameDriver>(*fixture.game, fixture.machine);
    owned->stepFrame(fixture.game->core, 1);
    std::_Exit(0);
  }
  int status = 0;
  waitpid(child, &status, 0);
  check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "a field ran before the finite boot prefix");
}

void warpWaitsForRunningIdleField() {
  Fixture fixture;
  if (!fixture.boot()) {
    ++failures;
    return;
  }
  RecordingMachine &machine = fixture.machine;
  Core &core = fixture.game->core;
  check(machine.operations.size() == 4u, "the boot prefix did not run on the driver's machine");
  check(fixture.driver->armWarp("warp 5 9").starts_with("ok:"), "warp 5 9 did not arm");
  showMenu(machine);

  // Both initializers span a field each; the menu is on screen but the frame is still booting.
  machine.boundedStartReturns = false;
  machine.boundedResumeReturns = false;
  fixture.driver->stepFrame(core, 10);
  fixture.driver->stepFrame(core, 11);
  machine.boundedResumeReturns = true;
  fixture.driver->stepFrame(core, 12);
  machine.boundedStartReturns = true;
  fixture.driver->stepFrame(core, 13);
  check(!warpActed(machine), "the warp acted during a boot initializer");
  check(fixture.game->timing.logicFrame == 10u, "only the field that began the boot call publishes its index");

  // A Running field whose mode body outlives it: the next field resumes that call, so no warp may enter.
  machine.operations.clear();
  machine.memory[FightWarp::kMode] = 2u;
  machine.boundedStartReturns = false;
  machine.boundedResumeReturns = false;
  fixture.driver->stepFrame(core, 14);
  check(machine.spanningCallPending(), "the mode body did not stay pending");
  showMenu(machine);
  fixture.driver->stepFrame(core, 15);
  check(!warpActed(machine), "the warp acted while a mode body was still in flight");
  check(fixture.game->timing.logicFrame == 14u, "a resumed field published a new logic frame");

  machine.boundedResumeReturns = true;
  fixture.driver->stepFrame(core, 16);
  check(!machine.spanningCallPending(), "the mode body did not return");

  // The first idle Running field: the menu's game start, then that field's barrier and mode body.
  machine.operations.clear();
  machine.boundedStartReturns = true;
  fixture.driver->stepFrame(core, 17);
  const std::ptrdiff_t start = indexOf(machine.operations, "call3", FightWarp::kGameStart);
  const std::ptrdiff_t leave = indexOf(machine.operations, "call1", FightWarp::kMenuLeave);
  const std::ptrdiff_t barrier = indexOf(machine.operations, "call", FiniteFrame::kFrameBarrier);
  check(start == 0 && leave == 1, "the warp's game start was not the field's first guest work");
  check(barrier > leave, "the field step ran before the warp");
}

} // namespace

int main() {
  fieldBeforeBootAborts();
  warpWaitsForRunningIdleField();
  std::printf("frame_driver_contract: %s\n", failures == 0 ? "PASS" : "FAIL");
  return failures == 0 ? 0 : 1;
}
