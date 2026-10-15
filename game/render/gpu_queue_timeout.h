// Tekken's linked-libgpu queue timeout and reset contract; the clock is the guest field counter.
#pragma once

#include <cstdint>

class Core;

namespace tekken3::render {

// One call of the linked critical-section setter (a0 enabled) from a site returning to returnPc.
struct CriticalSectionCall {
  std::uint32_t enabled = 0;
  std::uint32_t returnPc = 0;
};

// The libgpu state the timeout reports before the reset.
struct TimeoutReport {
  std::uint32_t queueDepth = 0;
  std::uint32_t gpuData = 0;
  std::uint32_t gpuControl = 0;
  std::uint32_t dmaControl = 0;
};

// Boundary for guest access, guest calls and the GPU timeout report.
class QueueMachine {
public:
  virtual ~QueueMachine() = default;
  virtual std::uint32_t fieldCounter() const = 0;
  virtual std::uint32_t read32(std::uint32_t address) const = 0;
  virtual void write32(std::uint32_t address, std::uint32_t value) = 0;
  // Returns the previous setting.
  virtual std::uint32_t setCriticalSection(CriticalSectionCall call) = 0;
  virtual void reportTimeout(const TimeoutReport &report) = 0;
};

// pollQueue returns -1 once the deadline passed and the queue was reset.
void armDeadline(QueueMachine &machine);
[[nodiscard]] std::int32_t pollQueue(QueueMachine &machine);

// Replaces the two linked GPU timeout-clock functions that query VSync.
void installOverrides(Core &core);

} // namespace tekken3::render
