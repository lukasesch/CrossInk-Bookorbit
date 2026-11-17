#include "PiccoSheetDraw.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include <cctype>
#include <string>

#include "components/UiAppHelpers.h"
#include "components/icons/piccoIcons.h"
#include "fontIds.h"

namespace PiccoSheetDraw {

namespace {
using namespace PiccoTokens;

constexpr int kSmallFont = SMALL_FONT_ID;
constexpr int kBodyFont = UI_10_FONT_ID;
constexpr int kLargeFont = UI_12_FONT_ID;
constexpr int kHintGap = 2;
constexpr int kTileIconGap = 6;
constexpr int kChipGap = 10;
constexpr int kCellPadX = 14;
}  // namespace

void text(const GfxRenderer& renderer, const int fontId, const Box& box, const char* value,
          const EpdFontFamily::Style style, const Align align) {
  if (value == nullptr || value[0] == '\0' || box.width <= 0) return;
  const std::string visible = renderer.truncatedText(fontId, value, box.width, style);
  const int w = renderer.getTextWidth(fontId, visible.c_str(), style);
  int x = box.x;
  if (align == Align::Center) x += (box.width - w) / 2;
  if (align == Align::Right) x = box.right() - w;
  renderer.drawText(fontId, x, box.y + (box.height - renderer.getLineHeight(fontId)) / 2, visible.c_str(), true, style);
}

void sectionLabel(const GfxRenderer& renderer, const Box& box, const char* value) {
  char caps[64];
  size_t i = 0;
  for (; value != nullptr && value[i] != '\0' && i + 1 < sizeof(caps); ++i) {
    caps[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(value[i])));
  }
  caps[i] = '\0';
  text(renderer, kSmallFont, box, caps, EpdFontFamily::BOLD);
}

void icon(const GfxRenderer& renderer, const freeink::Icon& glyph, const Box& box) {
  drawLucideIcon(renderer, glyph, box.x + (box.width - glyph.w) / 2, box.y + (box.height - glyph.h) / 2);
}

void sheet(const GfxRenderer& renderer, const int screenWidth, const int bottom) {
  renderer.fillRect(0, 0, screenWidth, bottom - kSheetRadius, false);
  renderer.fillRoundedRect(0, bottom - 2 * kSheetRadius, screenWidth, 2 * kSheetRadius, kSheetRadius, false, false,
                           true, true, Color::White);
  // Only the lower edge shows: the sides and top of the outline sit just off screen.
  renderer.drawRoundedRect(-1, -2 * kSheetRadius, screenWidth + 2, bottom + 2 * kSheetRadius, kHairline, kSheetRadius,
                           false, false, true, true, true);
  const int handleY = bottom - kSheetHandleOffset - kSheetHandleH;
  renderer.fillRoundedRect((screenWidth - kSheetHandleW) / 2, handleY, kSheetHandleW, kSheetHandleH, 3, Color::Black);
  renderer.fillRectDither(kGutter, bottom, screenWidth - 2 * kGutter, kSheetDitherStrip, Color::DarkGray);
}

void hairline(const GfxRenderer& renderer, const int x, const int y, const int width) {
  renderer.fillRect(x, y, width, kHairline, true);
}

void focusRing(const GfxRenderer& renderer, const Box& element, const int radius) {
  constexpr int kOutset = 4;
  renderer.drawRoundedRect(element.x - kOutset, element.y - kOutset, element.width + 2 * kOutset,
                           element.height + 2 * kOutset, kSelectionStroke, radius + kOutset, true);
}

void slider(const GfxRenderer& renderer, const Box& track, const int permille) {
  const int cy = track.y + track.height / 2;
  renderer.fillRect(track.x, cy, track.width, kHairline, true);
  const int knobX = PiccoSheet::sliderKnobCenterX(track, permille);
  renderer.fillRect(track.x, cy - kSliderFill / 2 + 1, knobX - track.x, kSliderFill, true);
  const int half = kSliderKnob / 2;
  renderer.fillRoundedRect(knobX - half, cy - half, kSliderKnob, kSliderKnob, half, Color::White);
  renderer.drawRoundedRect(knobX - half, cy - half, kSliderKnob, kSliderKnob, kSliderKnobStroke, half, true);
}

void tabBar(const GfxRenderer& renderer, const PiccoSheet::TabBar& bar, const char* first, const char* second,
            const int selected) {
  renderer.drawRoundedRect(bar.bar.x, bar.bar.y, bar.bar.width, bar.bar.height, kHairline, kTabRadius, true);
  const char* labels[2] = {first, second};
  for (int i = 0; i < 2; ++i) {
    const Box& c = bar.cells[static_cast<size_t>(i)];
    if (i == selected) renderer.drawRoundedRect(c.x, c.y, c.width, c.height, kSelectionStroke, kTabCellRadius, true);
    text(renderer, kSmallFont, Box{c.x + 8, c.y, c.width - 16, c.height}, labels[i],
         i == selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, Align::Center);
  }
}

void frame(const GfxRenderer& renderer, const Box& box, const bool selected) {
  renderer.drawRoundedRect(box.x, box.y, box.width, box.height, selected ? kSelectionStroke : kHairline, kSmallRadius,
                           true);
}

void stepper(const GfxRenderer& renderer, const PiccoSheet::Stepper& s, const char* value) {
  frame(renderer, s.minus);
  frame(renderer, s.plus);
  icon(renderer, icon_picco_minus_22, s.minus);
  icon(renderer, icon_picco_plus_22, s.plus);
  text(renderer, kBodyFont, s.value, value, EpdFontFamily::BOLD, Align::Center);
}

void segmented(const GfxRenderer& renderer, const PiccoSheet::Segmented& control, const freeink::Icon* const* icons,
               const int selected) {
  const Box& f = control.frame;
  renderer.drawRoundedRect(f.x, f.y, f.width, f.height, kHairline, kTabRadius, true);
  for (int i = 0; i < control.count; ++i) {
    const Box& c = control.cells[static_cast<size_t>(i)];
    if (i == selected) renderer.drawRoundedRect(c.x, c.y, c.width, c.height, kSelectionStroke, kTabCellRadius, true);
    if (icons[i] != nullptr) icon(renderer, *icons[i], c);
  }
}

void cell(const GfxRenderer& renderer, const Box& box, const char* label, const char* value, const bool enabled) {
  frame(renderer, box);
  const Box inner{box.x + kCellPadX, box.y, box.width - 2 * kCellPadX, box.height};
  const bool twoLines = value != nullptr && value[0] != '\0';
  const int labelH = renderer.getLineHeight(kBodyFont);
  const int valueH = twoLines ? renderer.getLineHeight(kSmallFont) : 0;
  const int top = box.y + (box.height - labelH - valueH) / 2;
  text(renderer, kBodyFont, Box{inner.x, top, inner.width, labelH}, label);
  if (twoLines) text(renderer, kSmallFont, Box{inner.x, top + labelH, inner.width, valueH}, value);
  // Unavailable actions keep their place but read as disabled.
  if (!enabled) renderer.fillRectDither(box.x + 1, box.y + 1, box.width - 2, box.height - 2, Color::LightGray);
}

void tile(const GfxRenderer& renderer, const Box& box, const freeink::Icon& glyph, const char* label) {
  frame(renderer, box);
  const int labelH = renderer.getLineHeight(kSmallFont);
  const int top = box.y + (box.height - static_cast<int>(glyph.h) - kTileIconGap - labelH) / 2;
  icon(renderer, glyph, Box{box.x, top, box.width, static_cast<int>(glyph.h)});
  text(renderer, kSmallFont, Box{box.x + 4, top + static_cast<int>(glyph.h) + kTileIconGap, box.width - 8, labelH},
       label, EpdFontFamily::REGULAR, Align::Center);
}

void chip(const GfxRenderer& renderer, const Box& box, const char* sample, const char* label, const bool selected) {
  frame(renderer, box, selected);
  const auto style = selected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const int sampleW = sample != nullptr ? renderer.getTextWidth(kLargeFont, sample) : 0;
  const int maxLabel = box.width - 24 - (sampleW > 0 ? sampleW + kChipGap : 0);
  const std::string visible = renderer.truncatedText(kSmallFont, label != nullptr ? label : "", maxLabel, style);
  const int labelW = renderer.getTextWidth(kSmallFont, visible.c_str(), style);
  const int total = labelW + (sampleW > 0 ? sampleW + kChipGap : 0);
  int x = box.x + (box.width - total) / 2;
  if (sampleW > 0) {
    text(renderer, kLargeFont, Box{x, box.y, sampleW, box.height}, sample);
    x += sampleW + kChipGap;
  }
  text(renderer, kSmallFont, Box{x, box.y, labelW + 1, box.height}, visible.c_str(), style);
}

void rowLabel(const GfxRenderer& renderer, const Box& row, const char* label, const char* hint,
              const int rightReserve) {
  const Box column{row.x, row.y, row.width - rightReserve - kControlClearance, row.height};
  const bool twoLines = hint != nullptr && hint[0] != '\0';
  const int labelH = renderer.getLineHeight(kBodyFont);
  const int hintH = twoLines ? renderer.getLineHeight(kSmallFont) + kHintGap : 0;
  const int top = row.y + (row.height - labelH - hintH) / 2;
  text(renderer, kBodyFont, Box{column.x, top, column.width, labelH}, label);
  if (twoLines)
    text(renderer, kSmallFont, Box{column.x, top + labelH + kHintGap, column.width, hintH - kHintGap}, hint);
}

}  // namespace PiccoSheetDraw

#endif
