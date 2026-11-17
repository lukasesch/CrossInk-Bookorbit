#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "components/themes/BaseTheme.h"

// Read-only progress: 7 px black fill (r4) on a 1 px track, vertically centred in rect.
namespace PiccoProgress {
void draw(const GfxRenderer& renderer, const Rect& rect, int permille);
}

#endif
