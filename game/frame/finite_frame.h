// The guest's main-loop bodies the title replaces: boot prefix, frame barrier, display init and the field step.
#pragma once

#include "guest_call_site.h"

#include <array>
#include <cstdint>

namespace tekken3::frame {

// Machine boundary shared by the Core adapter and the sequence tests.
class Machine {
public:
  virtual ~Machine() = default;

  virtual void call(psx::cpu::GuestCallSite site) = 0;
  virtual void call1(psx::cpu::GuestCallSite site, std::uint32_t a0) = 0;
  virtual void call3(psx::cpu::GuestCallSite site, const std::array<std::uint32_t, 3> &arguments) = 0;
  // A guest call that may outlive the field it starts on; both return true once it has returned.
  virtual bool beginSpanningCall(psx::cpu::GuestCallSite site) = 0;
  virtual bool resumeSpanningCall() = 0;
  virtual bool spanningCallPending() const = 0;
  virtual void deliverEvent(std::uint32_t eventClass, std::uint32_t spec) = 0;
  virtual std::uint32_t returnValue() const = 0;
  virtual std::uint32_t readRegister(std::uint32_t index) const = 0;
  virtual void writeRegister(std::uint32_t index, std::uint32_t value) = 0;
  virtual void tick(std::uint32_t guestInstructions) = 0;

  virtual std::uint8_t read8(std::uint32_t address) const = 0;
  virtual std::uint16_t read16(std::uint32_t address) const = 0;
  virtual std::uint32_t read32(std::uint32_t address) const = 0;
  virtual void write8(std::uint32_t address, std::uint8_t value) = 0;
  virtual void write16(std::uint32_t address, std::uint16_t value) = 0;
  virtual void write32(std::uint32_t address, std::uint32_t value) = 0;

  virtual void commitPresentation() = 0;
  virtual void serviceAudioSink() = 0;
  virtual void servicePad() = 0;
};

// The two retail initializers run as spanning calls so the guest's own interrupt service and libcd
// wait loops get host turns; every later field is a Running field.
enum class Stage : std::uint8_t { FirstInitializer, SecondInitializer, Running };

// What a field must remember: the stage, and a spanning call (initializer or mode body) still running.
struct StepState {
  Stage stage = Stage::FirstInitializer;
  bool callPending = false;
  std::uint32_t buffer = 0;
};

// The three overridden entries and the field step, reproducing the guest bodies they replace.
class FiniteFrame {
public:
  static constexpr std::uint32_t kMain = 0x80028BA0u;
  static constexpr std::uint32_t kFrameBarrier = 0x800296C4u;
  static constexpr std::uint32_t kDisplayInit = 0x800B0954u;
  // The RCntCNT2 event 0x80029894 registers as EvMdINTR.
  static constexpr std::uint32_t kFrameTimerEventClass = 0xF2000002u;
  static constexpr std::uint32_t kFrameTimerEventSpec = 0x00000002u;

  // The non-returning 0x80028BA0 prologue; the initializers it calls run from `step`.
  static void runBootPrefix(Machine &machine);
  [[nodiscard]] static bool runBarrier(Machine &machine);
  static void runDisplayInit(Machine &machine);
  static void step(Machine &machine, StepState &state);

private:
  static void stepBoot(Machine &machine, StepState &state);
};

} // namespace tekken3::frame
