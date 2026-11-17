#pragma once

#include <cstdint>

// Picco design tokens in device pixels (the 480 x 800 boards map 1:1 to the panel).
// Pure constexpr so native tests include it directly; it costs nothing where unused.
// Source: plans/picco-ui-phase1-addendum.md and the T_DesignLanguage board.
namespace PiccoTokens {

constexpr int kHeaderHeight = 80;
constexpr int kHairline = 1;
constexpr int kSelectionStroke = 2;

constexpr int kGutter = 24;
constexpr int kPageTextGutter = 28;
constexpr int kDialogInset = 24;

constexpr int kTouchMin = 60;
constexpr int kCompactControl = 60;
constexpr int kStepperButtonW = 64;
constexpr int kStepperButtonH = 60;
constexpr int kActionHeight = 60;

constexpr int kTabBarHeight = 64;
constexpr int kTabCell = 54;
constexpr int kTabPad = 4;
constexpr int kTabCellRadius = 12;
constexpr int kTabRadius = 16;

constexpr int kRowOneLine = 64;
constexpr int kRowWithHint = 80;
constexpr int kRowWithControl = 80;

// Row focus frame: outset into the gutter, inset from the hairlines, small-control radius.
constexpr int kRowFrameOutsetX = 12;
constexpr int kRowFrameInsetY = 4;
constexpr int kRowFrameRadius = 14;

constexpr int kControlClearance = 10;
constexpr int kNeighbourGap = 8;
constexpr int kGroupGap = 12;

constexpr int kSectionAbove = 30;
constexpr int kSectionBelow = 12;
constexpr int kSectionFirstAbove = 20;

constexpr int kSwitchW = 64;
constexpr int kSwitchH = 36;
constexpr int kSwitchKnob = 26;
constexpr int kSwitchKnobInset = 5;
constexpr int kSwitchStroke = 2;

constexpr int kPillHeight = 60;
constexpr int kPillRadius = 30;
constexpr int kPillPadX = 30;

constexpr int kLargeTileRadius = 16;
constexpr int kCardRadius = 16;
constexpr int kSmallRadius = 14;
constexpr int kSheetRadius = 22;
constexpr int kDialogRadius = 22;

constexpr int kProgressFill = 7;
constexpr int kProgressFillRadius = 4;
constexpr int kProgressTrack = 1;
constexpr int kSliderFill = 4;
constexpr int kSliderKnob = 28;
constexpr int kSliderKnobStroke = 2;

// Page indicator band: top = screen bottom - 52, 32 tall; lists end 64 above the bottom.
constexpr int kPageIndicatorFromBottom = 52;
constexpr int kPageIndicatorHeight = 32;
constexpr int kListBottomReserve = 64;

constexpr int kToastHeight = 76;
constexpr int kErrorToastHeight = 84;
constexpr int kErrorToastTwoLineHeight = 104;
constexpr int kToastGapBelowHeader = 12;
constexpr int kToastPadLeft = 16;
constexpr int kToastRadius = 14;

constexpr int kDialogPadding = 24;
constexpr int kDialogButtonHeight = 60;
constexpr int kDialogButtonGap = 12;

constexpr int kTextFieldHeight = 60;
constexpr int kTextFieldPadX = 16;

// Keyboard keys are the documented exception to the 60 px touch minimum.
constexpr int kKeyW = 43;
constexpr int kKeyH = 56;
constexpr int kKeyGap = 4;
constexpr int kKeyRadius = 12;

constexpr int kHeaderLeadingGap = 10;
constexpr int kHeaderClusterGap = 14;
constexpr int kHeaderIconTextGap = 6;
constexpr int kHeaderBatteryGap = 5;
constexpr int kHeaderDividerH = 24;
constexpr int kHeaderControl = 60;
constexpr int kHeaderBatteryIconW = 28;
constexpr int kHeaderBatteryIconH = 22;

constexpr int kIconChevron = 26;
constexpr int kIconHouse = 28;
constexpr int kIconSync = 22;
constexpr int kIconRow = 22;
constexpr int kIconTile = 32;

constexpr int kSheetHandleW = 56;
constexpr int kSheetHandleH = 5;
constexpr int kSheetHandleOffset = 12;
constexpr int kSheetDitherStrip = 6;

// A framed control keeps kControlClearance from the hairlines above and below it.
constexpr int rowHeightForControl(const int controlHeight) { return controlHeight + 2 * kControlClearance; }

static_assert(rowHeightForControl(kCompactControl) == kRowWithControl, "control rows hold a 60 px control");
static_assert(kTabCell + 2 * (kTabPad + kHairline) <= kTabBarHeight, "tab cells fit the bar");

}  // namespace PiccoTokens
