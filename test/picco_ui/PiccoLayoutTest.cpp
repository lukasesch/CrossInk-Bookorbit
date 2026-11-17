#include <gtest/gtest.h>

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoTokens.h"

using namespace PiccoTokens;
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
constexpr Box kHeader{0, 0, 480, kHeaderHeight};

constexpr bool clears(const int rowHeight, const int controlHeight) {
  return (rowHeight - controlHeight) / 2 >= kControlClearance;
}
}  // namespace

TEST(PiccoTokens, RowHeights) {
  EXPECT_EQ(kRowOneLine, 64);
  EXPECT_EQ(kRowWithHint, 80);
  EXPECT_EQ(kRowWithControl, 80);
  EXPECT_EQ(rowHeightForControl(60), 80);
}

TEST(PiccoTokens, FramedControlsKeepTheirClearance) {
  EXPECT_TRUE(clears(kRowOneLine, kSwitchH));
  EXPECT_TRUE(clears(kRowWithControl, kCompactControl));
  EXPECT_TRUE(clears(kRowWithControl, kStepperButtonH));
  // Why the control row exists: a 60 px control does not fit a one-line row.
  EXPECT_FALSE(clears(kRowOneLine, kCompactControl));
}

TEST(PiccoTokens, InteractiveSizesMeetTheTouchMinimum) {
  for (const int size : {kCompactControl, kStepperButtonW, kStepperButtonH, kActionHeight, kPillHeight, kHeaderControl,
                         kDialogButtonHeight, kTextFieldHeight}) {
    EXPECT_GE(size, kTouchMin) << size;
  }
  EXPECT_LE(kTabCell + 2 * (kTabPad + kHairline), kTabBarHeight);
  // Keyboard keys are the one documented exception.
  EXPECT_LT(kKeyH, kTouchMin);
}

TEST(PiccoHeaderLayout, BackHeaderMatchesTheBoard) {
  PiccoLayout::HeaderInputs in;
  in.leading = PiccoLayout::Leading::Back;
  in.titleWidth = 100;
  in.titleHeight = 30;
  in.clockWidth = 50;
  in.batteryTextWidth = 40;
  const auto h = PiccoLayout::headerLayout(kHeader, in);

  EXPECT_EQ(h.hairline.y, kHeaderHeight - 1);
  EXPECT_EQ(h.hairline.x, kGutter);
  EXPECT_EQ(h.hairline.right(), 480 - kGutter);
  EXPECT_EQ(h.battery.right(), 480 - kGutter);
  EXPECT_EQ(h.leadingIcon.x, kGutter - 2);
  EXPECT_EQ(h.leadingIcon.width, kIconChevron);
  EXPECT_EQ(h.title.x, h.leadingIcon.right() + kHeaderLeadingGap);
  EXPECT_EQ(h.clock.right() + kHeaderClusterGap, h.battery.x);
  EXPECT_TRUE(h.divider.empty());
  EXPECT_TRUE(h.contextual.empty());
}

TEST(PiccoHeaderLayout, BackTargetIsTallAndInsideTheBand) {
  PiccoLayout::HeaderInputs in;
  in.leading = PiccoLayout::Leading::Back;
  in.titleWidth = 80;
  const auto h = PiccoLayout::headerLayout(kHeader, in);
  EXPECT_GE(h.leadingTouch.height, kTouchMin);
  EXPECT_GE(h.leadingTouch.width, kTouchMin);
  EXPECT_GE(h.leadingTouch.y, kHeader.y);
  EXPECT_LE(h.leadingTouch.bottom(), h.hairline.y);
  EXPECT_TRUE(h.leadingTouch.contains(h.leadingIcon.x + 1, h.centerY));
  EXPECT_TRUE(h.leadingTouch.contains(h.title.right() - 1, h.centerY));
}

TEST(PiccoHeaderLayout, DividerOnlyWithAnActionOrSync) {
  PiccoLayout::HeaderInputs in;
  in.batteryTextWidth = 40;
  EXPECT_TRUE(PiccoLayout::headerLayout(kHeader, in).divider.empty());
  in.contextualWidth = kHeaderControl;
  const auto withAction = PiccoLayout::headerLayout(kHeader, in);
  EXPECT_FALSE(withAction.divider.empty());
  EXPECT_EQ(withAction.divider.height, kHeaderDividerH);
  in.contextualWidth = 0;
  in.syncWidth = 70;
  EXPECT_FALSE(PiccoLayout::headerLayout(kHeader, in).divider.empty());
}

TEST(PiccoHeaderLayout, ContextualActionStaysLeftOfTheStatus) {
  PiccoLayout::HeaderInputs in;
  in.leading = PiccoLayout::Leading::Back;
  in.titleWidth = 400;
  in.contextualWidth = kHeaderControl;
  in.syncWidth = 80;
  in.clockWidth = 50;
  in.batteryTextWidth = 40;
  const auto h = PiccoLayout::headerLayout(kHeader, in);
  EXPECT_FALSE(h.contextual.overlaps(h.clock));
  EXPECT_FALSE(h.contextual.overlaps(h.sync));
  EXPECT_FALSE(h.sync.overlaps(h.divider));
  EXPECT_LE(h.contextual.right(), h.sync.x);
  EXPECT_LE(h.divider.right(), h.clock.x);
  EXPECT_EQ(h.contextual.height, kHeaderControl);
  // The title gives way to the cluster.
  EXPECT_LE(h.title.right(), h.clusterLeft);
  EXPECT_EQ(h.clusterLeft, 480 - kGutter - PiccoLayout::statusClusterWidth(80, 50, 40, kHeaderControl));
}

TEST(PiccoHeaderLayout, TitleWidthNeverGoesNegative) {
  PiccoLayout::HeaderInputs in;
  in.leading = PiccoLayout::Leading::Back;
  in.titleWidth = 200;
  in.contextualWidth = 300;
  in.batteryTextWidth = 40;
  EXPECT_EQ(PiccoLayout::headerLayout(kHeader, in).title.width, 0);
}

TEST(PiccoHeaderLayout, TitleHonoursTheCallerReserve) {
  PiccoLayout::HeaderInputs in;
  in.titleWidth = 300;
  in.batteryTextWidth = 40;
  in.titleRightReserve = 250;
  const auto h = PiccoLayout::headerLayout(kHeader, in);
  EXPECT_LE(h.title.right(), 480 - 250);
}

TEST(PiccoHeaderLayout, HouseLeadsWithTheLargerIcon) {
  PiccoLayout::HeaderInputs in;
  in.leading = PiccoLayout::Leading::House;
  in.titleWidth = 60;
  const auto h = PiccoLayout::headerLayout(kHeader, in);
  EXPECT_EQ(h.leadingIcon.width, kIconHouse);
  EXPECT_EQ(h.title.x, h.leadingIcon.right() + kHeaderLeadingGap);
}

TEST(PiccoPaging, IndicatorBandAndListLimit) {
  const Box band = PiccoLayout::pageIndicatorBand(kScreen);
  EXPECT_EQ(band.y, 748);
  EXPECT_EQ(band.bottom(), 780);
  EXPECT_EQ(band.x, kGutter);
  EXPECT_EQ(band.width, 480 - 2 * kGutter);
  EXPECT_EQ(PiccoLayout::listBottomLimit(kScreen), 736);
  EXPECT_LE(PiccoLayout::listBottomLimit(kScreen), band.y);
}

TEST(PiccoPaging, PageCount) {
  EXPECT_EQ(PiccoLayout::pageCount(0, 7), 1);
  EXPECT_EQ(PiccoLayout::pageCount(7, 7), 1);
  EXPECT_EQ(PiccoLayout::pageCount(8, 7), 2);
  EXPECT_EQ(PiccoLayout::pageCount(33, 7), 5);
  EXPECT_EQ(PiccoLayout::pageCount(5, 0), 1);
}

TEST(PiccoPaging, PageForTopFollowsTheClampedLastViewport) {
  EXPECT_EQ(PiccoLayout::pageForTop(0, 7, 10), 1);
  EXPECT_EQ(PiccoLayout::pageForTop(3, 7, 10), 2);  // last viewport is clamped to count - rows
  EXPECT_EQ(PiccoLayout::pageForTop(7, 7, 20), 2);
  EXPECT_EQ(PiccoLayout::pageForTop(13, 7, 20), 3);
  EXPECT_EQ(PiccoLayout::pageForTop(0, 7, 5), 1);
}

TEST(PiccoPaging, OneSwipeFlipsOnePage) {
  EXPECT_EQ(PiccoLayout::topAfterSwipe(0, +1, 7, 20), 7);
  EXPECT_EQ(PiccoLayout::topAfterSwipe(7, +1, 7, 20), 13);
  EXPECT_EQ(PiccoLayout::topAfterSwipe(13, +1, 7, 20), 13);
  EXPECT_EQ(PiccoLayout::topAfterSwipe(13, -1, 7, 20), 6);
  EXPECT_EQ(PiccoLayout::topAfterSwipe(3, -1, 7, 20), 0);
  // Every swipe from the first page reaches the last one in pageCount - 1 steps.
  int top = 0;
  for (int i = 1; i < PiccoLayout::pageCount(20, 7); ++i) top = PiccoLayout::topAfterSwipe(top, +1, 7, 20);
  EXPECT_EQ(PiccoLayout::pageForTop(top, 7, 20), PiccoLayout::pageCount(20, 7));
}

TEST(PiccoSwitch, KnobSidesAndShapes) {
  const auto on = PiccoLayout::switchGeometry(0, 0, true);
  const auto off = PiccoLayout::switchGeometry(0, 0, false);
  EXPECT_EQ(on.knob.right(), kSwitchW - kSwitchKnobInset);
  EXPECT_EQ(off.knob.x, kSwitchKnobInset);
  EXPECT_EQ(on.knob.y, kSwitchKnobInset);
  EXPECT_EQ(on.knob.bottom() + kSwitchKnobInset, kSwitchH);

  const int onCx = on.knob.x + kSwitchKnob / 2;
  const int offCx = off.knob.x + kSwitchKnob / 2;
  const int cy = kSwitchH / 2;
  // On: solid knob; off: hollow knob with an outline.
  EXPECT_TRUE(PiccoLayout::switchInk(onCx, cy, true));
  EXPECT_FALSE(PiccoLayout::switchInk(offCx, cy, false));
  EXPECT_TRUE(PiccoLayout::switchInk(off.knob.x, cy, false));
  // Track: 2 px stroke, white inside, no ink at the corners.
  EXPECT_TRUE(PiccoLayout::switchInk(kSwitchW / 2, 0, false));
  EXPECT_TRUE(PiccoLayout::switchInk(kSwitchW / 2, 1, false));
  EXPECT_FALSE(PiccoLayout::switchInk(kSwitchW / 2, 3, false));
  EXPECT_FALSE(PiccoLayout::switchInk(0, 0, true));
  EXPECT_FALSE(PiccoLayout::switchInk(kSwitchW - 1, kSwitchH - 1, true));
  // The off knob sits left: nothing where the on knob would be.
  EXPECT_FALSE(PiccoLayout::switchInk(onCx, cy, false));
}

TEST(PiccoSwitch, BitmapMatchesTheInkFunction) {
  for (const bool state : {false, true}) {
    const auto bits = PiccoLayout::switchBits(state);
    for (int y = 0; y < kSwitchH; ++y) {
      for (int x = 0; x < kSwitchW; ++x) {
        const bool transparent = (bits[y * PiccoLayout::kSwitchRowBytes + x / 8] >> (7 - x % 8)) & 1;
        ASSERT_EQ(!transparent, PiccoLayout::switchInk(x, y, state)) << x << "," << y << " on=" << state;
      }
    }
  }
}

TEST(PiccoProgress, FillWidthClampsAndRounds) {
  EXPECT_EQ(PiccoLayout::progressFillWidth(400, 500), 200);
  EXPECT_EQ(PiccoLayout::progressFillWidth(400, -5), 0);
  EXPECT_EQ(PiccoLayout::progressFillWidth(400, 2000), 400);
  EXPECT_EQ(PiccoLayout::progressFillWidth(3, 500), 2);
  EXPECT_EQ(PiccoLayout::progressFillWidth(0, 500), 0);
}

TEST(PiccoProgress, TrackIsCentredOnTheFill) {
  const auto g = PiccoLayout::progressGeometry(Box{10, 20, 400, 7}, 620);
  EXPECT_EQ(g.fill.height, kProgressFill);
  EXPECT_EQ(g.track.height, kProgressTrack);
  EXPECT_EQ(g.track.y - g.fill.y, (kProgressFill - kProgressTrack) / 2);
  EXPECT_EQ(g.fill.width, 248);
  EXPECT_EQ(g.track.width, 400);
}

TEST(PiccoToast, SitsUnderTheHeaderInsideTheGutter) {
  const Box info = PiccoLayout::toastRect(kScreen, 80, PiccoLayout::ToastKind::Info);
  EXPECT_EQ(info.y, 80 + kToastGapBelowHeader);
  EXPECT_EQ(info.x, kGutter);
  EXPECT_EQ(info.right(), 480 - kGutter);
  EXPECT_EQ(info.height, 76);
  EXPECT_EQ(PiccoLayout::toastRect(kScreen, 80, PiccoLayout::ToastKind::Error).height, 84);
  EXPECT_EQ(PiccoLayout::toastRect(kScreen, 80, PiccoLayout::ToastKind::ErrorTwoLine).height, 104);
}

TEST(PiccoPill, WidthAddsPaddingAndIcon) {
  EXPECT_EQ(PiccoLayout::pillWidth(100, 0), 100 + 2 * kPillPadX);
  EXPECT_EQ(PiccoLayout::pillWidth(100, 22), 100 + 2 * kPillPadX + 22 + kHeaderLeadingGap);
}

TEST(PiccoListRow, FocusFrameOutsetsIntoTheGutterAndClearsTheHairlines) {
  // Band between the hairlines at y 100 and 164 (one 64 px step).
  const Box frame = PiccoLayout::rowFocusFrame(480, 100, kRowOneLine);
  EXPECT_EQ(frame.x, 12);
  EXPECT_EQ(frame.right(), 468);
  EXPECT_EQ(frame.y, 104);
  EXPECT_EQ(frame.bottom(), 100 + kRowOneLine - 4);

  const auto g = PiccoLayout::listRowGeometry(kRowOneLine);
  // Labels stay on the gutter and clear the 2 px frame by ~10 px.
  EXPECT_EQ(g.rowInset + g.sidePadding, kGutter);
  EXPECT_GE(g.sidePadding - kSelectionStroke, 10);
  // Row box + gap keep the 64 / 80 step; the hairline sits mid-gap, 4 px from each frame.
  EXPECT_EQ(g.rowHeight + g.rowGap, kRowOneLine);
  EXPECT_EQ(g.rowGap / 2, kRowFrameInsetY);
  EXPECT_EQ(PiccoLayout::listRowGeometry(kRowWithHint).rowHeight + g.rowGap, kRowWithHint);
  EXPECT_EQ(kRowFrameRadius, kSmallRadius);
}
