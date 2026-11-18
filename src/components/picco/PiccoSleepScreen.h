#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

class GfxRenderer;
struct RecentBook;

// The Picco sleep screen (board T_Sleep): the book's cover in a 1 px frame, chapter and
// percent, the progress line and the wake hint. A book without a cover image gets the
// typographic cover Home uses. Draws into the frame buffer; the caller refreshes.
namespace PiccoSleepScreen {
void draw(const GfxRenderer& renderer, const RecentBook& book, const char* chapter, float progressPercent);
}

#endif
