#include "runtime/session.h"

#include "core.h"
#include "game.h"

#include <lucent/log.h>

#include <cstdlib>

namespace tekken3 {

Session &session(Core &core) {
  auto *const owned = core.game == nullptr ? nullptr : dynamic_cast<Session *>(core.game->sessionState.get());
  if (owned == nullptr) {
    lucent::error("tekken3-core", "a Tekken 3 owner reached a Core whose Game has no Tekken 3 session");
    std::abort();
  }
  return *owned;
}

} // namespace tekken3
