#include "entry/tekken3_catalog.h"

#include "executable_identity.h"
#include "program/title_runtime.h"
#include "single_title_catalog.h"

namespace tekken3 {
namespace {

constexpr psx::host::TitleIdentity kTitleIdentity{
    .displayName = "Tekken 3",
    .serial = "SLUS_004.02",
    .slug = "tekken3",
    .fileSize = identity::kExecutableBytes,
    .sha256 = identity::kExecutableSha256,
    .entry = identity::kEntry,
    .globalPointer = identity::kGlobalPointer,
    .textAddress = identity::kTextAddress,
    .textSize = identity::kTextBytes,
    .stackAddress = identity::kStackAddress,
    .stackOffset = identity::kStackOffset,
    .discEnvVar = "PSXPORT_TEKKEN3_DISC",
    .discExecutablePath = identity::kDiscExecutablePath,
};

// The resident program is the header's text extent, in physical addresses.
constexpr std::uint32_t kResidentPhysicalLo = identity::kTextAddress & 0x1FFFFFFFu;
constexpr ResidentProgramRange kResidentProgram{kResidentPhysicalLo, kResidentPhysicalLo + identity::kTextBytes};

} // namespace

std::unique_ptr<psx::host::TitleCatalog> createCatalog() {
  return std::make_unique<psx::host::SingleTitleCatalog>(
      kTitleIdentity, "scratch/bin", std::make_unique<TitleRuntime>(kResidentProgram, identity::kEntry));
}

} // namespace tekken3
