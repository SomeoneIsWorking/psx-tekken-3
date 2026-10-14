#include "entry/product_launch.h"

#include "c_subsys.h"
#include "core.h"
#include "game.h"
#include "machine.h"
#include "program/title_runtime.h"
#include "psx_exe_image.h"
#include "run_budget.h"
#include "title_catalog.h"

#include <cstdint>
#include <memory>
#include <string>

namespace {

constexpr psx::host::TitleIdentity kTitleIdentity{
    .displayName = "Tekken 3",
    .serial = "SLUS_004.02",
    .slug = "tekken3",
    .discEnvVar = "PSXPORT_TEKKEN3_DISC",
    .discExecutablePath = "TEKKEN3/SLUS_004.02",
};
constexpr const char *kProvisioningRoot = "scratch/bin";

} // namespace

namespace tekken3 {

int launchProduct(TitleRuntime &runtime, int argc, char **argv) {
  const std::string executable =
      argc > 1 ? std::string(argv[1]) : kTitleIdentity.defaultExecutable(kProvisioningRoot).string();

  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  Core *const core = &game->core;
  psx::Machine machine{*game};
  machine.insertDisc(kTitleIdentity);

  watchdog_init();
  load_exe(executable.c_str(), core);

  machine.bindDevices();
  machine.prepare();
  runtime.bootInit(*core);

  machine.attachControlChannel();
  machine.run(static_cast<std::uint32_t>(psx::host::RunBudget::fromConfig().cap()));
  return 0;
}

} // namespace tekken3
