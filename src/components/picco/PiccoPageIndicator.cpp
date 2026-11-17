#include "PiccoPageIndicator.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>

#include "PiccoLayout.h"
#include "fontIds.h"

namespace PiccoPageIndicator {
namespace {
PiccoLayout::Box screenBox(const GfxRenderer& renderer) {
  return PiccoLayout::Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
}
}  // namespace

Rect band(const GfxRenderer& renderer) {
  const auto b = PiccoLayout::pageIndicatorBand(screenBox(renderer));
  return Rect{b.x, b.y, b.width, b.height};
}

int listBottom(const GfxRenderer& renderer) { return PiccoLayout::listBottomLimit(screenBox(renderer)); }

void draw(const GfxRenderer& renderer, const int page, const int pageCount, const Rect* bandOverride) {
  if (pageCount <= 1) return;
  char text[16];
  std::snprintf(text, sizeof(text), tr(STR_PICCO_PAGE_INDICATOR_FORMAT), static_cast<unsigned>(page),
                static_cast<unsigned>(pageCount));
  const Rect area = bandOverride != nullptr ? *bandOverride : band(renderer);
  const int width = renderer.getTextWidth(SMALL_FONT_ID, text);
  const int y = area.y + (area.height - renderer.getLineHeight(SMALL_FONT_ID)) / 2;
  renderer.drawText(SMALL_FONT_ID, area.x + (area.width - width) / 2, y, text);
}

void drawForList(const GfxRenderer& renderer, const int top, const int rowsPerPage, const int count,
                 const Rect* bandOverride) {
  draw(renderer, PiccoLayout::pageForTop(top, rowsPerPage, count), PiccoLayout::pageCount(count, rowsPerPage),
       bandOverride);
}
}  // namespace PiccoPageIndicator

#endif
