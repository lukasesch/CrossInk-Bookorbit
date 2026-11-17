#include "PiccoListUi.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <Icon.h>

#include <string>

#include "components/UiAppHelpers.h"
#include "components/icons/piccoIcons.h"
#include "fontIds.h"

namespace PiccoListUi {
namespace {
constexpr int kLabelFont = UI_10_FONT_ID;
constexpr int kRowFont = UI_10_FONT_ID;
}  // namespace

int textTop(const GfxRenderer& renderer, const int fontId, const int centerY) {
  return centerY - renderer.getLineHeight(fontId) / 2;
}

void drawSegmentedFrame(const GfxRenderer& renderer, const Rect& outer, const int count, const int selected) {
  renderer.drawRoundedRect(outer.x, outer.y, outer.width, outer.height, PiccoTokens::kHairline,
                           PiccoTokens::kTabRadius, true);
  if (selected < 0 || selected >= count) return;
  const auto cell = PiccoLibraryLayout::segmentCell(toBox(outer), count, selected);
  renderer.drawRoundedRect(cell.x, cell.y, cell.width, cell.height, PiccoTokens::kSelectionStroke,
                           PiccoTokens::kTabCellRadius, true);
}

void drawSegmented(const GfxRenderer& renderer, const Rect& outer, const char* const* labels, const int count,
                   const int selected) {
  drawSegmentedFrame(renderer, outer, count, selected);
  for (int i = 0; i < count; ++i) {
    const auto cell = PiccoLibraryLayout::segmentCell(toBox(outer), count, i);
    const auto style = i == selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    const std::string visible =
        renderer.truncatedText(kLabelFont, labels[i] != nullptr ? labels[i] : "", cell.width - 16, style);
    const int w = renderer.getTextWidth(kLabelFont, visible.c_str(), style);
    renderer.drawText(kLabelFont, cell.x + (cell.width - w) / 2, textTop(renderer, kLabelFont, cell.y + cell.height / 2),
                      visible.c_str(), true, style);
  }
}

Rect outlineButtonRect(const GfxRenderer& renderer, const int x, const int centerY, const char* label,
                       const freeink::Icon* icon) {
  const int labelW = renderer.getTextWidth(kLabelFont, label != nullptr ? label : "");
  return toRect(PiccoLibraryLayout::outlineButton(x, centerY, labelW, icon != nullptr ? icon->w : 0));
}

void drawOutlineButton(const GfxRenderer& renderer, const Rect& rect, const char* label, const freeink::Icon* icon,
                       const bool focused, const bool centered) {
  renderer.drawRoundedRect(rect.x, rect.y, rect.width, rect.height,
                           focused ? PiccoTokens::kSelectionStroke : PiccoTokens::kHairline, PiccoTokens::kSmallRadius,
                           true);
  const char* text = label != nullptr ? label : "";
  const int iconW = icon != nullptr ? icon->w + PiccoLibraryLayout::kOutlineButtonIconGap : 0;
  const int maxLabel = rect.width - PiccoLibraryLayout::kOutlineButtonPadLeft -
                       PiccoLibraryLayout::kOutlineButtonPadRight - iconW;
  const std::string visible = renderer.truncatedText(kLabelFont, text, maxLabel);
  const int labelW = renderer.getTextWidth(kLabelFont, visible.c_str());
  const int contentW = labelW + iconW;
  int x = centered ? rect.x + (rect.width - contentW) / 2 : rect.x + PiccoLibraryLayout::kOutlineButtonPadLeft;
  if (centered && icon != nullptr) {
    // Centered buttons lead with the icon (T_Files transfer pair).
    drawLucideIcon(renderer, *icon, x, rect.y + (rect.height - icon->h) / 2);
    x += iconW;
    renderer.drawText(kLabelFont, x, textTop(renderer, kLabelFont, rect.y + rect.height / 2), visible.c_str());
    return;
  }
  renderer.drawText(kLabelFont, x, textTop(renderer, kLabelFont, rect.y + rect.height / 2), visible.c_str());
  if (icon != nullptr) {
    drawLucideIcon(renderer, *icon, x + labelW + PiccoLibraryLayout::kOutlineButtonIconGap,
                   rect.y + (rect.height - icon->h) / 2);
  }
}

void drawCheckRow(const GfxRenderer& renderer, const Rect& row, const char* label, const bool checked,
                  const bool focused, const bool hairlineAbove) {
  if (hairlineAbove) renderer.fillRect(row.x, row.y, row.width, PiccoTokens::kHairline, true);
  const auto style = checked ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const auto& check = icon_picco_check_22;
  const int labelMax = row.width - check.w - PiccoTokens::kGroupGap;
  const std::string visible = renderer.truncatedText(kRowFont, label != nullptr ? label : "", labelMax, style);
  renderer.drawText(kRowFont, row.x, textTop(renderer, kRowFont, row.y + row.height / 2), visible.c_str(), true,
                    style);
  if (checked) drawLucideIcon(renderer, check, row.x + row.width - check.w, row.y + (row.height - check.h) / 2);
  if (focused) {
    const auto frame = PiccoLayout::rowFocusFrame(renderer.getScreenWidth(), row.y, row.height);
    renderer.drawRoundedRect(frame.x, frame.y, frame.width, frame.height, PiccoTokens::kSelectionStroke,
                             PiccoTokens::kRowFrameRadius, true);
  }
}

void drawGridGlyph(const GfxRenderer& renderer, const int x, const int y, const int size) {
  // Lucide-like: two 7 px squares per side with a 3 px gap, 2 px strokes.
  const int square = (size - 3) / 2;
  for (int r = 0; r < 2; ++r) {
    for (int c = 0; c < 2; ++c) {
      renderer.drawRect(x + c * (square + 3), y + r * (square + 3), square, square, 2, true);
    }
  }
}

void drawFocusFrame(const GfxRenderer& renderer, const Rect& rect, const int radius) {
  constexpr int kOutset = 4;
  renderer.drawRoundedRect(rect.x - kOutset, rect.y - kOutset, rect.width + 2 * kOutset, rect.height + 2 * kOutset,
                           PiccoTokens::kSelectionStroke, radius + kOutset, true);
}

}  // namespace PiccoListUi

#endif
