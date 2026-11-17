#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "components/themes/BaseTheme.h"

// Centred "n / m" in the band above the screen's bottom edge (PiccoLayout::pageIndicatorBand).
// Lists page by swipe; a single page shows nothing.
namespace PiccoPageIndicator {
// Screen-relative band the indicator owns; lists end above listBottom().
Rect band(const GfxRenderer& renderer);
int listBottom(const GfxRenderer& renderer);
// bandOverride moves the indicator when a screen footer owns the standard band.
void draw(const GfxRenderer& renderer, int page, int pageCount, const Rect* bandOverride = nullptr);
// Convenience for a viewport list: page derived from top / rowsPerPage / count.
void drawForList(const GfxRenderer& renderer, int top, int rowsPerPage, int count, const Rect* bandOverride = nullptr);
}  // namespace PiccoPageIndicator

#endif
