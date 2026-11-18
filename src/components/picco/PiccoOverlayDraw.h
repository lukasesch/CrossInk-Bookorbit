#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "PiccoReaderOverlays.h"

class GfxRenderer;

// Drawing for the Picco overlays over a book page; geometry from PiccoReaderOverlays.h.
namespace PiccoOverlayDraw {

using PiccoLayout::Box;

// White r22 card with a 1 px edge, opaque over the page.
void popover(const GfxRenderer& renderer, const Box& frame);
// Framed r14 button with a centred 17 px label; focused adds the 2 px selection stroke.
void button(const GfxRenderer& renderer, const Box& box, const char* label, bool focused = false);
// 2 px r5 frame around a word or note mark on the page.
void markFrame(const GfxRenderer& renderer, const Box& mark);

}  // namespace PiccoOverlayDraw

#endif
