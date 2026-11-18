#pragma once

#include "AppCapabilities.h"
#include "PiccoReaderListsLayout.h"

#if CROSSINK_APP_PICCO_UI

#include <FreeInkUIGfxRenderer.h>

#include "components/themes/BaseTheme.h"

class GfxRenderer;
class MappedInputManager;

namespace PiccoReaderLists {

PiccoLayout::Box screenBox(const GfxRenderer& renderer);
// Bottom of the header band the activity draws (its hairline).
int headerBottom(const Rect& header);

// Contents tabs with live counts ("Bookmarks · 24"); the selected cell gets the 2 px stroke.
void drawContentsTabs(const GfxRenderer& renderer, const Rect& header, ContentsTab selected);
// True when a tap landed on a tab cell; `tab` is that cell.
bool contentsTabTapped(const MappedInputManager& input, const GfxRenderer& renderer, const Rect& header,
                       ContentsTab& tab);

// List band insets for a FreeInkUI screen: top under header/tabs, bottom above `n / m`.
freeink::ui::Insets listInsets(const GfxRenderer& renderer, const Rect& header, bool tabs);

// Book text (Literata) in the FreeInkUI title slot while a list lays out, for row labels
// that name chapters or words (boards: Literata 21).
class BookFontScope {
 public:
  BookFontScope(freeink::ui::GfxRendererTarget& target, bool active);
  ~BookFontScope();
  BookFontScope(const BookFontScope&) = delete;
  BookFontScope& operator=(const BookFontScope&) = delete;
  static constexpr freeink::ui::FontId kSlot = freeink::ui::GfxRendererTarget::FONT_TITLE;

 private:
  freeink::ui::GfxRendererTarget& target_;
  bool active_;
};

}  // namespace PiccoReaderLists

#endif
