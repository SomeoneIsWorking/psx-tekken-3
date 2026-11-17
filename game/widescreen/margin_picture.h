// Whether a frame holds picture beside the display buffer: only the stage clipper draws into the margins.
#pragma once

#include "horizontal_margins.h"

namespace tekken3::widescreen {

// A frame whose mode body ran the stage clipper (FUN_8006CC28) holds the widened world in its margins; every other
// frame is a 4:3 2D layout (cards, title, menus, character select, the VS screen) and pillarboxes.
class MarginPicture {
public:
  void stageDrawn() {
    drawing_ = true;
  }

  // The mode body that drew the next frame to be sealed has returned.
  void bodyComplete() {
    sealed_ = drawing_;
    drawing_ = false;
  }

  [[nodiscard]] psx::present::PictureBeyondBuffer sealed() const {
    if (sealed_) {
      return {};
    }
    return {.picture = {0, 0}};
  }

private:
  bool drawing_ = false;
  bool sealed_ = false;
};

} // namespace tekken3::widescreen
