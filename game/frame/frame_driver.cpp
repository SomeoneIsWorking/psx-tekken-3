#include "frame/frame_driver.h"

#include "core.h"
#include "execution/finite_guest_call.h"
#include "execution_control.h"
#include "execution_services.h"
#include "game.h"
#include "native_dispatch.h"

#include <lucent/log.h>

#include <cstdlib>

namespace tekken3::frame {
namespace {

constexpr auto kFieldTurn = psx::cpu::ExecutionBudget::fromCycles(FiniteFrame::kFieldTurnCycles);

} // namespace

CoreMachine::CoreMachine(Game &game, Core &core) : game_(game), core_(core) {}

void CoreMachine::call(psx::cpu::GuestCallSite site) {
  core_.r[31] = site.returnPc;
  execution::FiniteGuestCall::callToReturn(core_, site.entry, "Tekken3 frame guest call");
}

void CoreMachine::call1(psx::cpu::GuestCallSite site, std::uint32_t a0) {
  core_.r[4] = a0;
  call(site);
}

void CoreMachine::call3(psx::cpu::GuestCallSite site, const std::array<std::uint32_t, 3> &arguments) {
  core_.r[4] = arguments[0];
  core_.r[5] = arguments[1];
  core_.r[6] = arguments[2];
  call(site);
}

bool CoreMachine::beginSpanningCall(psx::cpu::GuestCallSite site) {
  spanningCall_.begin(core_, "Tekken3 spanning guest call", site, psx::cpu::kUnboundedCallTurns);
  return settled(advance());
}

bool CoreMachine::resumeSpanningCall() {
  return settled(advance());
}

bool CoreMachine::spanningCallPending() const {
  return spanningCall_.pending();
}

// CdInit, mode 0 (resource decompress) and mode 2 (sector wait) span fields; the frame state bounds the wait.
bool CoreMachine::settled(const psx::cpu::CallStep &step) {
  if (step.outcome == psx::cpu::CallOutcome::Refused) {
    lucent::error("tekken3-guest",
                  "Tekken3 spanning guest call refused after {} host turn(s) and {} cycles at 0x{:08X}: {}",
                  step.turns,
                  step.cycles,
                  step.guestPc,
                  step.detail);
    std::abort();
  }
  return step.outcome == psx::cpu::CallOutcome::Returned;
}

psx::cpu::CallStep CoreMachine::advance() {
  return spanningCall_.advance({}, kFieldTurn);
}

void CoreMachine::deliverEvent(std::uint32_t eventClass, std::uint32_t spec) {
  game_.hle.deliverEvent(eventClass, spec);
}

std::uint32_t CoreMachine::returnValue() const {
  return core_.r[2];
}

std::uint32_t CoreMachine::readRegister(std::uint32_t index) const {
  return core_.r[index];
}

void CoreMachine::writeRegister(std::uint32_t index, std::uint32_t value) {
  core_.r[index] = value;
}

void CoreMachine::tick(std::uint32_t guestInstructions) {
  psx::cpu::accountGuestInstructions(core_, guestInstructions);
}

std::uint8_t CoreMachine::read8(std::uint32_t address) const {
  return core_.mem_r8(address);
}

std::uint16_t CoreMachine::read16(std::uint32_t address) const {
  return core_.mem_r16(address);
}

std::uint32_t CoreMachine::read32(std::uint32_t address) const {
  return core_.mem_r32(address);
}

void CoreMachine::write8(std::uint32_t address, std::uint8_t value) {
  core_.mem_w8(address, value);
}

void CoreMachine::write16(std::uint32_t address, std::uint16_t value) {
  core_.mem_w16(address, value);
}

void CoreMachine::write32(std::uint32_t address, std::uint32_t value) {
  core_.mem_w32(address, value);
}

void CoreMachine::commitPresentation() {
  game_.presentation.commit(&core_, 1);
}

void CoreMachine::serviceAudioSink() {
  game_.spu_audio.frame();
}

void CoreMachine::servicePad() {
  game_.pad.serviceFrame();
}

FrameDriver::FrameDriver(Game &game)
    : game_(game), ownedMachine_(std::make_unique<CoreMachine>(game, game.core)), machine_(*ownedMachine_) {}

FrameDriver::FrameDriver(Game &game, Machine &machine) : game_(game), machine_(machine) {}

FrameDriver::~FrameDriver() = default;

void FrameDriver::mainOverride(Core *core) {
  FrameDriver &driver = core->game->frameDriverAs<FrameDriver>();
  if (!driver.bootStarted_ || driver.bootComplete_) {
    lucent::error("boot", "Tekken 3 finite main reached outside its one boot dispatch");
    std::abort();
  }
  FiniteFrame::runBootPrefix(driver.machine_);
  driver.bootComplete_ = true;
  psx::cpu::requestExecutionExit(*core, psx::cpu::ExecutionExitReason::HostService);
}

void FrameDriver::barrierOverride(Core *core) {
  FrameDriver &driver = core->game->frameDriverAs<FrameDriver>();
  if (!FiniteFrame::runBarrier(driver.machine_)) {
    lucent::error("frame",
                  "Tekken 3 RCntCNT2 event class 0x{:08X} spec 0x{:08X} did not release the frame barrier",
                  FiniteFrame::kFrameTimerEventClass,
                  FiniteFrame::kFrameTimerEventSpec);
    std::abort();
  }
}

void FrameDriver::displayInitOverride(Core *core) {
  FiniteFrame::runDisplayInit(core->game->frameDriverAs<FrameDriver>().machine_);
}

void FrameDriver::installOverrides() {
  psx::cpu::installNativeOverride(game_.core, FiniteFrame::kMain, "Tekken3::finiteMain", mainOverride);
  psx::cpu::installNativeOverride(game_.core, FiniteFrame::kFrameBarrier, "Tekken3::frameBarrier", barrierOverride);
  psx::cpu::installNativeOverride(game_.core, FiniteFrame::kDisplayInit, "Tekken3::displayInit", displayInitOverride);
}

void FrameDriver::runBootPrefix(Core &core) {
  if (bootStarted_) {
    lucent::error("boot", "Tekken 3 finite boot prefix was requested more than once");
    std::abort();
  }
  bootStarted_ = true;
  const std::uint32_t entry = core.pc;
  const auto result =
      psx::cpu::dispatchGuest(core, entry, psx::cpu::ExecutionBudget::currentTurn(core), "tekken3 boot prefix");
  if (result.reason != psx::cpu::ExecutionExitReason::HostService) {
    lucent::error("boot",
                  "Tekken 3 boot exited as {} at 0x{:08X}, expected finite-main host transfer",
                  psx::cpu::executionExitName(result.reason),
                  result.guestPc);
    std::abort();
  }
  if (!bootComplete_) {
    lucent::error("boot",
                  "Tekken 3 retail entry 0x{:08X} returned without reaching finite main 0x{:08X}",
                  entry,
                  FiniteFrame::kMain);
    std::abort();
  }
  lucent::info("boot", "Tekken 3 finite main prefix complete; native driver owns loop 0x80028BCC");
}

void FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  if (!bootComplete_) {
    lucent::error("frame", "Tekken 3 frame {} ran before its finite boot prefix", frame);
    std::abort();
  }
  if (stepState_.callPending != machine_.spanningCallPending()) {
    lucent::error("frame", "Tekken 3 spanning call and frame continuation disagree at field {}", frame);
    std::abort();
  }
  if (!stepState_.callPending) {
    game_.timing.logicFrame = frame;
    core.rsub.otAttr.beginLogicFrame(frame);
    if (stepState_.stage == Stage::Running) {
      warp_.apply(machine_);
    }
  }
  FiniteFrame::step(machine_, stepState_);
}

std::string FrameDriver::armWarp(const psx::debug::CommandArgs &args) {
  return warp_.arm(args);
}

} // namespace tekken3::frame
