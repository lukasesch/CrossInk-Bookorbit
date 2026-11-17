#include "PiccoSettingsRows.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

#include "PiccoSwitch.h"
#include "PiccoTokens.h"
#include "components/UiAppHelpers.h"
#include "components/icons/piccoIcons.h"
#include "fontIds.h"

namespace PiccoSettingsRows {
namespace {
using PiccoLayout::Box;
constexpr int kLabelFont = UI_10_FONT_ID;
constexpr int kSmallFont = SMALL_FONT_ID;
constexpr int kHintGap = 2;
constexpr int kValueChevronGap = 4;

void drawSectionText(const GfxRenderer& renderer, const char* text, const int y) {
  std::string caps = text != nullptr ? text : "";
  std::transform(caps.begin(), caps.end(), caps.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
  const int width = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;
  const std::string visible = renderer.truncatedText(kSmallFont, caps.c_str(), width, EpdFontFamily::BOLD);
  renderer.drawText(kSmallFont, PiccoTokens::kGutter, y, visible.c_str(), true, EpdFontFamily::BOLD);
}

void drawSection(const GfxRenderer& renderer, const Row& row, const PiccoPagedList::Slot& slot) {
  drawSectionText(renderer, row.label, slot.y);
}

void drawStepperButton(const GfxRenderer& renderer, const Box& box, const freeink::Icon& icon) {
  renderer.drawRoundedRect(box.x, box.y, box.width, box.height, PiccoTokens::kHairline, PiccoTokens::kSmallRadius,
                           true);
  drawLucideIcon(renderer, icon, box.x + (box.width - icon.w) / 2, box.y + (box.height - icon.h) / 2);
}

// Draws the trailing control right-aligned at `right`; returns its left edge.
int drawTrailing(const GfxRenderer& renderer, const Row& row, const int right, const int centerY) {
  using namespace PiccoTokens;
  int x = right;
  switch (row.trailing) {
    case Trailing::Switch:
      x -= kSwitchW;
      PiccoSwitch::draw(renderer, x, centerY - kSwitchH / 2, row.on);
      return x;
    case Trailing::Check: {
      const auto& icon = icon_picco_check_22;
      x -= icon.w;
      if (row.on) drawLucideIcon(renderer, icon, x, centerY - icon.h / 2);
      return x;
    }
    case Trailing::Chevron:
    case Trailing::ValueChevron: {
      const auto& icon = icon_picco_chevron_right_22;
      x -= icon.w;
      drawLucideIcon(renderer, icon, x, centerY - icon.h / 2);
      if (row.trailing == Trailing::Chevron || row.value == nullptr || row.value[0] == '\0') return x;
      x -= kValueChevronGap;
      break;
    }
    case Trailing::Value:
      if (row.value == nullptr || row.value[0] == '\0') return x;
      break;
    case Trailing::Stepper: {
      const auto stepper = PiccoLayout::stepperLayout(right, centerY);
      drawStepperButton(renderer, stepper.minus, icon_picco_minus_22);
      drawStepperButton(renderer, stepper.plus, icon_picco_plus_22);
      const char* value = row.value != nullptr ? row.value : "";
      const std::string text = renderer.truncatedText(kSmallFont, value, stepper.value.width, EpdFontFamily::BOLD);
      const int w = renderer.getTextWidth(kSmallFont, text.c_str(), EpdFontFamily::BOLD);
      renderer.drawText(kSmallFont, stepper.value.x + (stepper.value.width - w) / 2,
                        centerY - renderer.getLineHeight(kSmallFont) / 2, text.c_str(), true, EpdFontFamily::BOLD);
      return stepper.minus.x;
    }
    case Trailing::Button: {
      const Rect r = buttonRect(renderer, row, PiccoPagedList::Slot{static_cast<int16_t>(centerY), 0, false});
      renderer.drawRoundedRect(r.x, r.y, r.width, r.height, kHairline, kSmallRadius, true);
      const char* label = row.value != nullptr ? row.value : "";
      const int iconW = row.buttonIcon != nullptr ? row.buttonIcon->w + kHeaderIconTextGap : 0;
      const int textW = renderer.getTextWidth(kLabelFont, label);
      int cx = r.x + (r.width - iconW - textW) / 2;
      if (row.buttonIcon != nullptr) {
        drawLucideIcon(renderer, *row.buttonIcon, cx, centerY - row.buttonIcon->h / 2);
        cx += iconW;
      }
      renderer.drawText(kLabelFont, cx, centerY - renderer.getLineHeight(kLabelFont) / 2, label);
      return r.x;
    }
    case Trailing::None:
      return x;
  }
  // Values never take more than half the row; the label keeps the rest.
  const int maxWidth = (right - kGutter) / 2;
  const std::string value = renderer.truncatedText(kSmallFont, row.value, maxWidth);
  const int w = renderer.getTextWidth(kSmallFont, value.c_str());
  x -= w;
  renderer.drawText(kSmallFont, x, centerY - renderer.getLineHeight(kSmallFont) / 2, value.c_str());
  return x;
}

void drawRow(const GfxRenderer& renderer, const Row& row, const PiccoPagedList::Slot& slot) {
  using namespace PiccoTokens;
  const int right = renderer.getScreenWidth() - kGutter;
  if (slot.hairline) renderer.fillRect(kGutter, slot.y, right - kGutter, kHairline, true);
  const int centerY = slot.y + slot.height / 2;
  int trailingLeft = drawTrailing(renderer, row, right, centerY);
  if (row.marker != nullptr) {
    trailingLeft -= (trailingLeft < right ? kGroupGap : 0) + row.marker->w;
    drawLucideIcon(renderer, *row.marker, trailingLeft, centerY - row.marker->h / 2);
  }
  int left = kGutter;
  if (row.leading != nullptr) {
    drawLucideIcon(renderer, *row.leading, left, centerY - row.leading->h / 2);
    left += row.leading->w + kGroupGap;
  }
  const int labelWidth = std::max(0, trailingLeft - kGroupGap - left);

  const int labelH = renderer.getLineHeight(kLabelFont);
  const bool hint = row.hint != nullptr && row.hint[0] != '\0';
  const int hintH = hint ? kHintGap + renderer.getLineHeight(kSmallFont) : 0;
  const int labelY = centerY - (labelH + hintH) / 2;
  const auto style = row.bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const std::string label =
      renderer.truncatedText(kLabelFont, row.label != nullptr ? row.label : "", labelWidth, style);
  renderer.drawText(kLabelFont, left, labelY, label.c_str(), true, style);
  if (hint) {
    const std::string text = renderer.truncatedText(kSmallFont, row.hint, labelWidth);
    renderer.drawText(kSmallFont, left, labelY + labelH + kHintGap, text.c_str());
  }
}
}  // namespace

int sectionTextHeight(const GfxRenderer& renderer) { return renderer.getLineHeight(kSmallFont); }

int16_t rowHeight(const Row& row) {
  if (row.trailing == Trailing::Stepper || row.trailing == Trailing::Button) return PiccoTokens::kRowWithControl;
  return row.hint != nullptr && row.hint[0] != '\0' ? PiccoTokens::kRowWithHint : PiccoTokens::kRowOneLine;
}

PiccoLayout::Stepper stepperRects(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot) {
  return PiccoLayout::stepperLayout(renderer.getScreenWidth() - PiccoTokens::kGutter, slot.y + slot.height / 2);
}

Rect buttonRect(const GfxRenderer& renderer, const Row& row, const PiccoPagedList::Slot& slot) {
  const int labelW = renderer.getTextWidth(kLabelFont, row.value != nullptr ? row.value : "");
  const int iconW = row.buttonIcon != nullptr ? row.buttonIcon->w : 0;
  const auto box = PiccoLayout::rowButton(renderer.getScreenWidth() - PiccoTokens::kGutter, slot.y + slot.height / 2,
                                          PiccoLayout::rowButtonWidth(labelW, iconW));
  return Rect{box.x, box.y, box.width, box.height};
}

void drawContinued(const GfxRenderer& renderer, const char* sectionLabel, const char* continuedFormat, const int y) {
  char text[96];
  std::snprintf(text, sizeof(text), continuedFormat, sectionLabel != nullptr ? sectionLabel : "");
  drawSectionText(renderer, text, y);
}

Rect rowRect(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot) {
  const Box frame = PiccoLayout::rowFocusFrame(renderer.getScreenWidth(), slot.y, slot.height);
  return Rect{frame.x, frame.y, frame.width, frame.height};
}

void drawFocus(const GfxRenderer& renderer, const PiccoPagedList::Slot& slot, const bool state) {
  const Rect r = rowRect(renderer, slot);
  renderer.drawRoundedRect(r.x, r.y, r.width, r.height, PiccoTokens::kSelectionStroke, PiccoTokens::kRowFrameRadius,
                           state);
}

void drawPage(const GfxRenderer& renderer, const Row* rows, const PiccoPagedList::Slot* slots, const int first,
              const int end, const int focus) {
  for (int i = first; i < end; ++i) {
    if (rows[i].section) {
      drawSection(renderer, rows[i], slots[i]);
    } else {
      drawRow(renderer, rows[i], slots[i]);
    }
  }
  if (focus >= first && focus < end && !rows[focus].section) drawFocus(renderer, slots[focus], true);
}

}  // namespace PiccoSettingsRows

#endif
