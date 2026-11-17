#pragma once

#include <FreeInkUI.h>

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// FreeInkUI tokens for the Picco theme, applied over the shared defaults by uiThemeTokens().
// Spacing tokens (spaceXs..spaceLg) stay at SDK defaults; design spacing lives in PiccoTokens.
inline void applyPiccoTokens(freeink::ui::ThemeTokens& tokens) {
  namespace fui = freeink::ui;
  using namespace PiccoTokens;

  const auto stroke = [](const uint8_t width, const uint8_t radius) {
    fui::BoxStyle box;
    box.border = fui::Paint::solid(fui::Color::Black);
    box.borderWidth = width;
    box.radius = radius;
    return box;
  };
  const auto strokeSet = [&](const uint8_t radius) {
    fui::StyleSet set;
    set.explicitlySet = true;
    set.normal = stroke(kHairline, radius);
    set.selected = stroke(kSelectionStroke, radius);
    set.focused = set.selected;
    set.active = set.selected;
    set.disabled = set.normal;
    return set;
  };

  tokens.rowHeight = kRowOneLine;
  tokens.headerHeight = kHeaderHeight;
  tokens.minTouchSize = kTouchMin;
  tokens.progressHeight = kProgressFill;
  constexpr auto rows = PiccoLayout::listRowGeometry(kRowOneLine);
  tokens.listMinRowHeight = static_cast<int16_t>(rows.rowHeight);
  tokens.listTouchMinRowHeight = static_cast<int16_t>(rows.rowHeight);
  tokens.listTouchRowGap = 0;
  tokens.listRowGap = static_cast<int16_t>(rows.rowGap);
  tokens.listRowRadius = 0;
  tokens.listSeparator = fui::Paint::solid(fui::Color::Black);
  tokens.listSidePadding = static_cast<int16_t>(rows.sidePadding);
  tokens.listInset = static_cast<int16_t>(rows.rowInset);
  tokens.listScrollWidth = 0;
  tokens.headerSidePadding = kGutter;
  tokens.headerUnderline = kHairline;
  tokens.headerTitleAlign = fui::TextAlign::Left;
  tokens.controlRadius = kSmallRadius;
  tokens.sheetRadius = kSheetRadius;
  tokens.titleText.bold = true;
  tokens.bodyText.bold = false;

  // Rows are frameless; selection is a 2 px stroke, never a fill.
  fui::StyleSet row;
  row.explicitlySet = true;
  row.normal.foreground = fui::Paint::solid(fui::Color::Black);
  row.selected = stroke(kSelectionStroke, kRowFrameRadius);
  row.focused = row.selected;
  row.active = row.selected;
  row.disabled = row.normal;
  tokens.listRow = row;
  // InvertFill leaves explicit row styles as they are (the other styles rewrite `selected`).
  tokens.listSelectionStyle = fui::SelectionStyle::InvertFill;

  tokens.button = strokeSet(kSmallRadius);
  tokens.key = strokeSet(kKeyRadius);
  tokens.textField = strokeSet(kSmallRadius);
  // tokens.popup stays default: fui::header() frames its band with it (Phase 4 dialogs).
}
