#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include "PiccoTokens.h"
#include "components/themes/BaseTheme.h"

// The single framed summary (Home stats): 1 px frame, r16; content is the caller's.
namespace PiccoCard {
inline void draw(const GfxRenderer& renderer, const Rect& rect, const bool selected = false) {
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height,
                           selected ? PiccoTokens::kSelectionStroke : PiccoTokens::kHairline, PiccoTokens::kCardRadius,
                           true);
}
}  // namespace PiccoCard

#endif
