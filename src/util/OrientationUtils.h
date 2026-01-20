#pragma once

#include <GfxRenderer.h>

#include "CrossPointSettings.h"

inline GfxRenderer::Orientation getUiOrientation() {
  const auto orientation =
      static_cast<CrossPointSettings::ORIENTATION>(SETTINGS.orientation);
  if (orientation == CrossPointSettings::INVERTED) {
    return GfxRenderer::Orientation::PortraitInverted;
  }
  return GfxRenderer::Orientation::Portrait;
}

inline void applyUiOrientation(GfxRenderer &renderer) {
  renderer.setOrientation(getUiOrientation());
}

inline int getUiTopInset(const GfxRenderer &renderer) {
  return renderer.getOrientation() == GfxRenderer::Orientation::PortraitInverted
             ? GfxRenderer::BUTTON_HINT_HEIGHT
             : 0;
}

inline int getUiLeftInset(const GfxRenderer &renderer) {
  return renderer.getOrientation() ==
                 GfxRenderer::Orientation::LandscapeClockwise
             ? GfxRenderer::BUTTON_HINT_WIDTH
             : 0;
}

inline int getUiRightInset(const GfxRenderer &renderer) {
  return renderer.getOrientation() ==
                 GfxRenderer::Orientation::LandscapeCounterClockwise
             ? GfxRenderer::BUTTON_HINT_WIDTH
             : 0;
}
