// Widescreen projection policy: the widened view plus the two horizontal clippers.
#pragma once

#include "guest_widescreen_projection.h"
#include "widescreen/stage_tile_wedge.h"

#include <cstdint>

class Core;

namespace tekken3::widescreen {

// The runtime's projection policy; aborts if Core's runtime has none.
const WidescreenProjection &policyFrom(Core *core, const char *owner);

// The policy is immutable; the plan each call latches is the caller's (the Core's session).
class WidescreenProjection final : public psx::present::GuestWidescreenProjection {
public:
  using GuestBody = void (*)(Core *);
  using ProjectionLatch = psx::present::GuestProjectionPlan (*)(Core &, psx::present::GuestProjectionGeometry);

  WidescreenProjection();
  WidescreenProjection(ProjectionLatch latch, GuestBody retailDimensions);

  void install(Core &core) const;
  void publishDimensions(Core &core, psx::present::GuestProjectionPlan &plan) const;
  void clipStagePrimitives(Core &core, const psx::present::GuestProjectionPlan &plan) const;
  void clipEffectPrimitive(Core &core, const psx::present::GuestProjectionPlan &plan) const;

  [[nodiscard]] const StageTileWedge &stageWedge() const;

  static psx::present::GuestProjectionGeometry measuredGeometry(std::uint32_t viewWidth, std::uint32_t viewHeight);

private:
  static void publishDimensionsOverride(Core *core);
  static void stageClipOverride(Core *core);
  static void effectClipOverride(Core *core);

  ProjectionLatch latch_;
  GuestBody retailDimensions_;
  GuestBody retailStageClip_;
  GuestBody retailEffectClip_;
  StageTileWedge stageWedge_;
};

} // namespace tekken3::widescreen
