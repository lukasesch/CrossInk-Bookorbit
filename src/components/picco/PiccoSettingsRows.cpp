#include "PiccoSettingsRows.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include <algorithm>
#include <cctype>
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

void drawSection(const GfxRenderer& renderer, const Row& row, const PiccoPagedList::Slot& slot) {
  std::string caps = row.label != nullptr ? row.label : "";
  std::transform(caps.begin(), caps.end(), caps.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
  const int width = renderer.getScreenWidth() - 2 * PiccoTokens::kGutter;
  const std::string visible = renderer.truncatedText(kSmallFont, caps.c_str(), width, EpdFontFamily::BOLD);
  renderer.drawText(kSmallFont, PiccoTokens::kGutter, slot.y, visible.c_str(), true, EpdFontFamily::BOLD);
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
  const int left = kGutter;
  const int right = renderer.getScreenWidth() - kGutter;
  if (slot.hairline) renderer.fillRect(left, slot.y, right - left, kHairline, true);
  const int centerY = slot.y + slot.height / 2;
  const int trailingLeft = drawTrailing(renderer, row, right, centerY);
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
  return row.hint != nullptr && row.hint[0] != '\0' ? PiccoTokens::kRowWithHint : PiccoTokens::kRowOneLine;
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
