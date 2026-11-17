#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <cstdint>

#include "PiccoPagedList.h"
#include "components/themes/BaseTheme.h"

namespace freeink {
struct Icon;
}

// Settings-style rows (T_Settings, T_SettingsDisplay): caps section labels, 64 px rows (80
// with a hint), a trailing value, chevron or switch, 2 px focus frame. Pages come from
// PiccoPagedList; this draws one page and reports its row rects for touch.
namespace PiccoSettingsRows {

enum class Trailing : uint8_t { None, Value, Chevron, ValueChevron, Switch, Check, Stepper, Button };

struct Row {
  const char* label = nullptr;
  const char* hint = nullptr;
  const char* value = nullptr;
  Trailing trailing = Trailing::None;
  bool on = false;
  bool section = false;
  bool bold = false;
  const freeink::Icon* leading = nullptr;     // left of the label (T_Wifi signal)
  const freeink::Icon* marker = nullptr;      // left of the trailing control (T_Wifi lock)
  const freeink::Icon* buttonIcon = nullptr;  // Trailing::Button: icon before `value`
};

// Section label line height (small bold font).
int sectionTextHeight(const GfxRenderer& renderer);
// Row height for a row: 80 with a hint, a stepper or a button, else 64.
int16_t rowHeight(const Row& row);
// A stepper row's minus / plus buttons (touch targets).
PiccoLayout::Stepper stepperRects(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot);
// A button row's framed control (Trailing::Button; its touch target).
Rect buttonRect(const GfxRenderer& renderer, const Row& row, const PiccoPagedList::Slot& slot);
// "<label> (continued)" at the top of a page that opens mid-section.
void drawContinued(const GfxRenderer& renderer, const char* sectionLabel, const char* continuedFormat, int y);

// Draws entries [first, end) placed by slots; focus < 0 draws no frame.
void drawPage(const GfxRenderer& renderer, const Row* rows, const PiccoPagedList::Slot* slots, int first, int end,
              int focus);
// Focus frame of a placed row (also its touch rect).
Rect rowRect(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot);
void drawFocus(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot, bool state);

}  // namespace PiccoSettingsRows

#endif
