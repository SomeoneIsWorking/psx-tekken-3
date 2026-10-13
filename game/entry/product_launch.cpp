#include "entry/product_launch.h"

#include "c_subsys.h"
#include "core.h"
#include "game.h"
#include "machine.h"
#include "program/title_runtime.h"
#include "psx_exe_image.h"
#include "run_budget.h"

#include <cstdint>
#include <memory>

namespace {

constexpr const char *kDefaultExecutable = "scratch/bin/tekken3/SLUS_004.02";

} // namespace

namespace tekken3 {

int launchProduct(TitleRuntime &runtime, int argc, char **argv) {
  const char *const executable = argc > 1 ? argv[1] : kDefaultExecutable;

  psxport_install_game(runtime);
  auto game = std::make_unique<Game>();
  // No legacy GameConfig here, so bind the disc key on the disc subsystem directly.
  game->disc.env_key = "PSXPORT_TEKKEN3_DISC";
  Core *const core = &game->core;

  watchdog_init();
  load_exe(executable, core);

  psx::Machine machine{*game};
  machine.bindDevices();
  machine.prepare();
  runtime.bootInit(*core);

  machine.attachControlChannel();
  machine.run(static_cast<std::uint32_t>(psx::host::RunBudget::fromConfig().cap()));
  return 0;
}

} // namespace tekken3
