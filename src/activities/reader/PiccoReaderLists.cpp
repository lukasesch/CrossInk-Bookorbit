#include "PiccoReaderLists.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>
#include <string>

#include "BookmarkStore.h"
#include "ClippingStore.h"
#include "MappedInputManager.h"
#include "components/UIScale.h"
#include "components/picco/PiccoListUi.h"
#include "fontIds.h"

namespace PiccoReaderLists {
namespace {
// T_Contents: tab labels are 17 px (the selected one 600), so "Bookmarks · 24" fits a third.
constexpr int kTabFont = SMALL_FONT_ID;
}  // namespace

PiccoLayout::Box screenBox(const GfxRenderer& renderer) {
  return PiccoLayout::Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
}

int headerBottom(const Rect& header) { return header.y + header.height; }

void drawContentsTabs(const GfxRenderer& renderer, const Rect& header, const ContentsTab selected) {
  const auto tabs = contentsTabs(screenBox(renderer), headerBottom(header));
  char bookmarks[32];
  char clippings[32];
  std::snprintf(bookmarks, sizeof(bookmarks), tr(STR_PICCO_BOOKMARKS_TAB_FORMAT),
                static_cast<unsigned>(BOOKMARKS.getBookmarks().size()));
  std::snprintf(clippings, sizeof(clippings), tr(STR_PICCO_CLIPPINGS_TAB_FORMAT),
                static_cast<unsigned>(CLIPPINGS.clippingCount()));
  const char* labels[kContentsTabCount] = {tr(STR_PICCO_CHAPTERS_TAB), bookmarks, clippings};
  const int selectedIndex = static_cast<int>(selected);
  PiccoListUi::drawSegmentedFrame(renderer, PiccoListUi::toRect(tabs), kContentsTabCount, selectedIndex);
  for (int i = 0; i < kContentsTabCount; ++i) {
    const auto cell = PiccoLibraryLayout::segmentCell(tabs, kContentsTabCount, i);
    const auto style = i == selectedIndex ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    const std::string visible = renderer.truncatedText(kTabFont, labels[i], cell.width - 8, style);
    const int w = renderer.getTextWidth(kTabFont, visible.c_str(), style);
    renderer.drawText(kTabFont, cell.x + (cell.width - w) / 2,
                      PiccoListUi::textTop(renderer, kTabFont, cell.y + cell.height / 2), visible.c_str(), true, style);
  }
}

bool contentsTabTapped(const MappedInputManager& input, const GfxRenderer& renderer, const Rect& header,
                       ContentsTab& tab) {
  const auto tabs = contentsTabs(screenBox(renderer), headerBottom(header));
  if (!input.wasTapInRect(tabs.x, tabs.y, tabs.width, tabs.height)) return false;
  for (int i = 0; i < kContentsTabCount; ++i) {
    const auto cell = PiccoLibraryLayout::segmentCell(tabs, kContentsTabCount, i);
    // Pads between cells belong to the nearer cell (segmentAt), so widen each by half a pad.
    const int left = i == 0 ? tabs.x : cell.x - PiccoTokens::kTabPad / 2;
    const int right = i == kContentsTabCount - 1 ? tabs.right() : cell.right() + PiccoTokens::kTabPad / 2;
    if (input.wasTapInRect(left, tabs.y, right - left, tabs.height)) {
      tab = static_cast<ContentsTab>(i);
      return true;
    }
  }
  return false;
}

freeink::ui::Insets listInsets(const GfxRenderer& renderer, const Rect& header, const bool tabs) {
  const auto screen = screenBox(renderer);
  const int top = listTop(screen, headerBottom(header), tabs);
  return freeink::ui::Insets{static_cast<int16_t>(top), 0,
                             static_cast<int16_t>(screen.height - PiccoLayout::listBottomLimit(screen)), 0};
}

BookFontScope::BookFontScope(freeink::ui::GfxRendererTarget& target, const bool active)
    : target_(target), active_(active) {
  if (active_) target_.setFont(kSlot, LITERATA_10_FONT_ID);
}

BookFontScope::~BookFontScope() {
  if (active_) target_.setFont(kSlot, uiScaleSpec().titleFontId);
}

}  // namespace PiccoReaderLists

#endif
