#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "components/themes/BaseTheme.h"

namespace freeink {
struct Icon;
}

// The one solid black button per screen: 60 tall, r30, body bold white label, optional
// trailing icon. rect() and draw() share the geometry so hit tests match the ink.
namespace PiccoPillButton {
Rect rect(const GfxRenderer& renderer, int x, int centerY, const char* label, const freeink::Icon* icon = nullptr);
void draw(const GfxRenderer& renderer, const Rect& rect, const char* label, const freeink::Icon* icon = nullptr);
}  // namespace PiccoPillButton

#endif
