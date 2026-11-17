#pragma once

#include "AppCapabilities.h"
#include "PiccoDialogLayout.h"

#if CROSSINK_APP_PICCO_UI

#include "components/themes/BaseTheme.h"

namespace PiccoDialog {
enum class Focus : int8_t { None = -1, Cancel = 0, Confirm = 1 };

// Darkens what is on screen with a 50 % checkerboard, keeping the content legible below.
void scrim(const GfxRenderer& renderer);
// Draws the card; returns its geometry for hit tests.
PiccoDialogLayout::Geometry draw(const GfxRenderer& renderer, const char* title, const char* body,
                                 const char* cancelLabel, const char* confirmLabel, Focus focus);
}  // namespace PiccoDialog

#endif
