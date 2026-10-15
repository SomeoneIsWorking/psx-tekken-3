#pragma once

#include "frame/fight_warp.h"
#include "frame/finite_frame.h"
#include "game_runtime.h"
#include "resumable_guest_call.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

class Core;
class Game;

namespace tekken3::frame {

// The Machine the shipping driver runs on: guest calls through the product dispatcher and the Game's services.
class CoreMachine final : public Machine {
public:
  CoreMachine(Game &game, Core &core);

  void call(psx::cpu::GuestCallSite site) override;
  void call1(psx::cpu::GuestCallSite site, std::uint32_t a0) override;
  void call3(psx::cpu::GuestCallSite site, const std::array<std::uint32_t, 3> &arguments) override;
  bool beginSpanningCall(psx::cpu::GuestCallSite site) override;
  bool resumeSpanningCall() override;
  bool spanningCallPending() const override;
  void deliverEvent(std::uint32_t eventClass, std::uint32_t spec) override;
  std::uint32_t returnValue() const override;
  std::uint32_t readRegister(std::uint32_t index) const override;
  void writeRegister(std::uint32_t index, std::uint32_t value) override;
  void tick(std::uint32_t guestInstructions) override;

  std::uint8_t read8(std::uint32_t address) const override;
  std::uint16_t read16(std::uint32_t address) const override;
  std::uint32_t read32(std::uint32_t address) const override;
  void write8(std::uint32_t address, std::uint8_t value) override;
  void write16(std::uint32_t address, std::uint16_t value) override;
  void write32(std::uint32_t address, std::uint32_t value) override;

  void commitPresentation() override;
  void serviceAudioSink() override;
  void servicePad() override;

private:
  psx::cpu::CallStep advance();
  static bool settled(const psx::cpu::CallStep &step);

  Game &game_;
  Core &core_;
  psx::cpu::ResumableGuestCall spanningCall_;
};

// One display field: an armed warp at the boundary, then the native main loop body (FiniteFrame::step) on a
// Machine; also the boot prefix and the three overridden entries.
class FrameDriver final : public psx::game::FrameDriver {
public:
  // The shipping driver, on a CoreMachine over the Game's core.
  explicit FrameDriver(Game &game);
  // A driver on a test's machine.
  FrameDriver(Game &game, Machine &machine);
  ~FrameDriver() override;
  FrameDriver(const FrameDriver &) = delete;
  FrameDriver &operator=(const FrameDriver &) = delete;
  FrameDriver(FrameDriver &&) = delete;
  FrameDriver &operator=(FrameDriver &&) = delete;

  void installOverrides();
  // Dispatches the retail entry at core.pc until the native main asks for the host transfer.
  void runBootPrefix(Core &core);
  void stepFrame(Core &core, std::uint32_t frame) override;
  // Arms `warp <p1> <p2>`; later field boundaries take it through the menu and character select.
  std::string armWarp(std::string_view line);

  static FrameDriver &from(Core &core);

private:
  static void mainOverride(Core *core);
  static void barrierOverride(Core *core);
  static void displayInitOverride(Core *core);

  Game &game_;
  std::unique_ptr<CoreMachine> ownedMachine_;
  Machine &machine_;
  StepState stepState_;
  FightWarp warp_;
  bool bootStarted_ = false;
  bool bootComplete_ = false;
};

} // namespace tekken3::frame
