#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "PiccoLayout.h"
#include "PiccoTokens.h"
#include "components/themes/lyra/LyraTheme.h"

namespace PiccoMetrics {
constexpr ThemeMetrics makeValues() {
  using namespace PiccoTokens;
  ThemeMetrics v = LyraMetrics::values;
  v.headerHeight = kHeaderHeight;
  v.contentSidePadding = kGutter;
  v.headerSidePadding = kGutter;
  v.headerUnderlineSize = kHairline;
  v.headerTitleAlign = 0;
  v.headerBatteryDetached = false;
  v.listRowHeight = kRowOneLine;
  v.listWithSubtitleRowHeight = kRowWithHint;
  constexpr auto rows = PiccoLayout::listRowGeometry(kRowOneLine);
  v.listRowGap = rows.rowGap;
  v.listRowRadius = 0;
  v.listInset = rows.rowInset;
  v.listSidePadding = rows.sidePadding;
  v.listSelectionStyle = 0;
  v.listScrollWidth = 0;
  v.listTitleBold = false;
  v.menuRowHeight = kRowOneLine;
  v.tabSpacing = kTabPad;
  v.tabBarHeight = kTabBarHeight;
  v.tabBarAppearance = ThemeTabBarAppearance::Segmented;
  v.popupFrameThickness = kHairline;
  v.popupCornerRadius = kDialogRadius;
  v.popupMarginX = kToastPadLeft;
  v.popupProgressBarHeight = kProgressFill;
  v.optionPopupInnerPadding = kDialogPadding;
  v.optionPopupSelectionRadius = kSmallRadius;
  v.optionPopupSelectionLight = false;
  v.optionPopupDialogSideMargin = kDialogInset;
  v.optionPopupOptionFontBold = false;
  v.textFieldNormalThickness = kHairline;
  v.textFieldCursorThickness = kSelectionStroke;
  v.keyboardKeyHeight = kKeyH;
  v.keyboardKeySpacing = kKeyGap;
  return v;
}

constexpr ThemeMetrics values = makeValues();
}  // namespace PiccoMetrics

// Picco look (plans/picco-ui-phase1-plan.md §3). Home, covers and list row steps stay
// Lyra's until Phase 2; this class restyles the shared chrome hooks only.
class PiccoTheme : public LyraTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle = nullptr,
                  bool readerContext = false, bool showStatus = true, bool filenameTitle = false) const override;
  void drawHeaderChrome(const GfxRenderer& renderer, Rect rect, const HeaderChrome& chrome) const override;
  void drawSubHeader(const GfxRenderer& renderer, Rect rect, const char* label,
                     const char* rightLabel = nullptr) const override;
  Rect drawPopup(const GfxRenderer& renderer, const char* message, bool preserveBackdrop = false) const override;
  void fillPopupProgress(const GfxRenderer& renderer, const Rect& layout, int progress) const override;
  void drawTextField(const GfxRenderer& renderer, Rect rect, int textWidth, bool cursorMode = false,
                     int contentStartX = 0, int contentWidth = 0) const override;
  OptionPopupRowPaint paintOptionPopupRow(const GfxRenderer& renderer, Rect row, bool selected,
                                          bool disabled) const override;

  // Header geometry for chrome, shared by drawing and hit testing. Text is measured with
  // the header fonts; the clock and percent use fixed samples so targets never move.
  static PiccoLayout::HeaderLayout headerLayout(const GfxRenderer& renderer, const Rect& header,
                                                const HeaderChrome& chrome);
  // Same layout without a renderer, from the last measured status cluster (hit tests that
  // run between renders). Title-dependent rects are empty.
  static PiccoLayout::HeaderLayout cachedHeaderLayout(const Rect& header, HeaderLeading leading, bool hasContextual);
  // Width from the right gutter to the left edge of the status cluster (incl. divider).
  static int statusClusterReserve(bool hasContextual);
};

#endif
