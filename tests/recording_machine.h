// A Machine that records every operation and answers reads from a sparse word map.
#pragma once

#include "frame/finite_frame.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tekken3::test {

struct Operation {
  std::string kind;
  std::uint32_t address = 0;
  std::uint32_t returnPc = 0;
  std::uint32_t a0 = 0;
  std::uint32_t a1 = 0;
  std::uint32_t a2 = 0;
};

class RecordingMachine final : public tekken3::frame::Machine {
public:
  void call(psx::cpu::GuestCallSite site) override {
    operations.push_back({"call", site.entry, site.returnPc});
    result = callResults[site.entry];
  }

  void call1(psx::cpu::GuestCallSite site, std::uint32_t a0) override {
    operations.push_back({"call1", site.entry, site.returnPc, a0});
    result = callResults[site.entry];
  }

  void call3(psx::cpu::GuestCallSite site, const std::array<std::uint32_t, 3> &arguments) override {
    operations.push_back({"call3", site.entry, site.returnPc, arguments[0], arguments[1], arguments[2]});
    result = callResults[site.entry];
  }

  bool beginSpanningCall(psx::cpu::GuestCallSite site) override {
    ++boundedStarts;
    call(site);
    pending = !boundedStartReturns;
    return boundedStartReturns;
  }

  bool resumeSpanningCall() override {
    ++boundedResumes;
    operations.push_back({"resume"});
    pending = !boundedResumeReturns;
    return boundedResumeReturns;
  }

  bool spanningCallPending() const override {
    return pending;
  }

  void deliverEvent(std::uint32_t eventClass, std::uint32_t spec) override {
    operations.push_back({"event", eventClass, spec});
    if (releaseFrameEvent) {
      memory[0x8009BC5Cu] = 1;
    }
  }

  std::uint32_t returnValue() const override {
    return result;
  }

  std::uint32_t readRegister(std::uint32_t index) const override {
    return registers[index];
  }

  void writeRegister(std::uint32_t index, std::uint32_t value) override {
    registers[index] = value;
  }

  void tick(std::uint32_t guestInstructions) override {
    guestTicks += guestInstructions;
  }

  std::uint8_t read8(std::uint32_t address) const override {
    return static_cast<std::uint8_t>(read32(address));
  }

  std::uint16_t read16(std::uint32_t address) const override {
    return static_cast<std::uint16_t>(read32(address));
  }

  std::uint32_t read32(std::uint32_t address) const override {
    const auto found = memory.find(address);
    return found == memory.end() ? 0 : found->second;
  }

  void write8(std::uint32_t address, std::uint8_t value) override {
    memory[address] = value;
    operations.push_back({"write8", address, 0, value});
  }

  void write16(std::uint32_t address, std::uint16_t value) override {
    memory[address] = value;
    operations.push_back({"write16", address, 0, value});
  }

  void write32(std::uint32_t address, std::uint32_t value) override {
    memory[address] = value;
    operations.push_back({"write32", address, 0, value});
  }

  void commitPresentation() override {
    operations.push_back({"present"});
  }

  void serviceAudioSink() override {
    operations.push_back({"audio"});
  }

  void servicePad() override {
    operations.push_back({"pad"});
  }

  std::vector<Operation> operations;
  std::unordered_map<std::uint32_t, std::uint32_t> memory;
  std::unordered_map<std::uint32_t, std::uint32_t> callResults;
  std::array<std::uint32_t, 32> registers{};
  std::uint32_t guestTicks = 0;
  std::uint32_t result = 0;
  bool releaseFrameEvent = true;
  bool boundedStartReturns = true;
  bool boundedResumeReturns = true;
  bool pending = false;
  unsigned boundedStarts = 0;
  unsigned boundedResumes = 0;
};

} // namespace tekken3::test
