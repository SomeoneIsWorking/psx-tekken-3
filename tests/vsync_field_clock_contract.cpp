// Core-bound VSync field-clock contract: the production reader must return the measured word.
#include "core.h"
#include "fieldclock/field_clock.h"
#include "stub_game_runtime.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

constexpr std::uint32_t kMeasured = tekken3::field::kCounter;
// Neighbours and the saved-count word FUN_800859A8 subtracts from.
constexpr std::uint32_t kDecoyBefore = tekken3::field::kCounter - 4u;
constexpr std::uint32_t kDecoyAfter = tekken3::field::kCounter + 4u;
constexpr std::uint32_t kDecoySaved = 0x80099B38u;

constexpr std::uint32_t kMeasuredValue = 0x01234567u;
constexpr std::uint32_t kDecoyValue = 0x89ABCDEFu;

} // namespace

int main() {
  // Core is heap-resident (2 MiB RAM), as in production.
  psx::test::StubGameRuntime runtime;
  auto core = std::make_unique<Core>(runtime);
  core->mem_w32(kMeasured, kMeasuredValue);
  core->mem_w32(kDecoyBefore, kDecoyValue);
  core->mem_w32(kDecoyAfter, kDecoyValue);
  core->mem_w32(kDecoySaved, kDecoyValue);

  const std::uint32_t read = tekken3::field::readCounter(*core);
  if (read != kMeasuredValue) {
    std::fprintf(stderr,
                 "vsync_field_clock_contract: FAIL — the production reader returned 0x%08X for the "
                 "measured word 0x%08X, whose value is 0x%08X. A neighbouring word reads 0x%08X, so "
                 "this is a wrong address, not an empty one\n",
                 read,
                 kMeasured,
                 kMeasuredValue,
                 kDecoyValue);
    return 1;
  }

  // The reader must not write guest state.
  if (core->mem_r32(kMeasured) != kMeasuredValue) {
    std::fprintf(stderr,
                 "vsync_field_clock_contract: FAIL — the field-clock read modified 0x%08X; a "
                 "producer that writes guest state is a defect\n",
                 kMeasured);
    return 1;
  }

  // Zero is a valid field count (library init zeroes it).
  core->mem_w32(kMeasured, 0u);
  if (tekken3::field::readCounter(*core) != 0u) {
    std::fprintf(stderr, "vsync_field_clock_contract: FAIL — a field count of zero was not returned as zero\n");
    return 1;
  }

  std::printf("vsync_field_clock_contract: PASS — the production reader returns the measured word "
              "0x%08X and not any of 3 planted decoy neighbours, a zero count is returned as zero, and "
              "the read leaves guest RAM unchanged\n",
              tekken3::field::kCounter);
  std::printf("vsync_field_clock_contract: NOT covered — whether the guest's own vblank callback "
              "advances this word in a real run; that is a product measurement, not a unit fact\n");
  return 0;
}
