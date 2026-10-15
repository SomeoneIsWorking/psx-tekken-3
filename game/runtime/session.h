#pragma once

#include "game_runtime.h"
#include "guest_widescreen_projection.h"

class Core;

namespace tekken3 {

// What one Core of Tekken 3 owns outside guest RAM, created with its Game.
class Session final : public psx::game::TitleSessionState {
public:
  Session() = default;

  // The plan the guest's last view-dimension call latched; the clippers and the tile wedge read it.
  GuestProjectionPlan projectionPlan;
};

// The session of the Game `core` belongs to; aborts when it has none.
Session &session(Core &core);

} // namespace tekken3
