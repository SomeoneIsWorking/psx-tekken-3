#include "product/catalog.h"

#include "executable_identity.h"
#include "runtime/runtime.h"
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

} // namespace

std::unique_ptr<psx::host::TitleCatalog> createCatalog() {
  return std::make_unique<psx::host::SingleTitleCatalog>(kTitleIdentity, "scratch/bin", std::make_unique<Runtime>());
}

} // namespace tekken3
