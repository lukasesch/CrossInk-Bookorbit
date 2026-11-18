#include <gtest/gtest.h>

#include "components/picco/PiccoReaderOverlays.h"
#include "components/picco/sheet/PiccoSheetModel.h"

using namespace PiccoTokens;
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
}

TEST(Picco4R2Buttons, EqualCellsFillTheWidth) {
  const auto row = PiccoOverlay::buttonRow(48, 300, 384, 3);
  ASSERT_EQ(row.count, 3);
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(row.cells[i].height, kActionHeight);
    EXPECT_EQ(row.cells[i].y, 300);
  }
  EXPECT_EQ(row.cells[0].x, 48);
  EXPECT_EQ(row.cells[1].x - row.cells[0].right(), kNeighbourGap);
  EXPECT_EQ(row.cells[2].x - row.cells[1].right(), kNeighbourGap);
  EXPECT_EQ(row.cells[2].right(), 48 + 384);
  EXPECT_LE(row.cells[2].width - row.cells[0].width, 2);
}

TEST(Picco4R2Buttons, CountIsClamped) {
  EXPECT_EQ(PiccoOverlay::buttonRow(0, 0, 300, 0).count, 0);
  EXPECT_EQ(PiccoOverlay::buttonRow(0, 0, 300, 7).count, PiccoOverlay::kMaxButtons);
  EXPECT_EQ(PiccoOverlay::buttonRow(0, 0, 300, 1).cells[0].width, 300);
}

// T_Footnote: the popover hangs 8 px under the note mark, between the 24 px gutters.
TEST(Picco4R2Popover, BelowTheAnchorWhenItFits) {
  const auto p = PiccoOverlay::notePopover(kScreen, Box{300, 260, 20, 24});
  EXPECT_EQ(p.frame.x, kGutter);
  EXPECT_EQ(p.frame.right(), 480 - kGutter);
  EXPECT_EQ(p.frame.y, 260 + 24 + PiccoOverlay::kPopoverGap);
  EXPECT_EQ(p.label.x, kGutter + kDialogPadding);
  EXPECT_EQ(p.buttons.count, 2);
  EXPECT_EQ(p.buttons.cells[1].right(), 480 - kGutter - kDialogPadding);
  EXPECT_EQ(p.buttons.cells[0].bottom(), p.frame.bottom() - kDialogPadding);
}

TEST(Picco4R2Popover, AboveTheAnchorNearTheBottom) {
  const Box anchor{100, 700, 20, 24};
  const auto p = PiccoOverlay::notePopover(kScreen, anchor);
  EXPECT_EQ(p.frame.bottom(), anchor.y - PiccoOverlay::kPopoverGap);
  EXPECT_FALSE(p.frame.overlaps(anchor));
}

TEST(Picco4R2Popover, PinnedWhenNeitherSideFits) {
  const Box frame = PiccoOverlay::popoverFrame(kScreen, Box{0, 380, 10, 20}, 500);
  EXPECT_GE(frame.y, kGutter);
  EXPECT_LE(frame.bottom(), 800 - kGutter);
}

// T_ClippingSelect: hairline at 708, Cancel 110 wide left of the Save pill, 20 above the edge.
TEST(Picco4R2Clip, ToolbarMatchesTheBoard) {
  const auto t = PiccoOverlay::clipToolbar(kScreen, 200);
  EXPECT_EQ(t.band.y, 800 - 20 - kActionHeight - 12);
  EXPECT_EQ(t.band.bottom(), 800);
  EXPECT_EQ(t.save.right(), 480 - kGutter);
  EXPECT_EQ(t.save.width, 200);
  EXPECT_EQ(t.save.bottom(), 800 - 20);
  EXPECT_EQ(t.cancel.width, 110);
  EXPECT_EQ(t.save.x - t.cancel.right(), kNeighbourGap);
  EXPECT_EQ(t.count.x, kGutter);
  EXPECT_EQ(t.cancel.x - t.count.right(), kGroupGap);
  EXPECT_TRUE(t.band.contains(t.save.x, t.save.y));
}

// T_Sleep: 300 x 450 cover at (90, 96), meta at 572, wake hint 40 above the bottom.
TEST(Picco4R2Sleep, LayoutMatchesTheBoard) {
  const auto s = PiccoOverlay::sleepLayout(kScreen);
  EXPECT_EQ(s.cover.x, 90);
  EXPECT_EQ(s.cover.y, 96);
  EXPECT_EQ(s.cover.width, 300);
  EXPECT_EQ(s.cover.height, 450);
  EXPECT_EQ(s.meta.y, 572);
  EXPECT_EQ(s.progress.y, s.meta.bottom() + PiccoOverlay::kSleepProgressGap);
  EXPECT_EQ(s.hint.bottom(), 800 - PiccoOverlay::kSleepHintBottom);
  EXPECT_LT(s.progress.bottom(), s.hint.y);
}

TEST(Picco4R2Sleep, ShortScreenShrinksTheCover) {
  const auto s = PiccoOverlay::sleepLayout(Box{0, 0, 480, 600});
  EXPECT_LT(s.cover.height, PiccoOverlay::kSleepCoverH);
  EXPECT_NEAR(s.cover.width * 3, s.cover.height * 2, 3);
  EXPECT_LT(s.progress.bottom(), s.hint.y);
}

// T_QrPage: 332 px frame under the header, sheet ends at 560; Back leads to More.
TEST(Picco4R2Qr, PaneLayout) {
  const auto q = PiccoSheet::qrLayout(480);
  EXPECT_EQ(q.frame.x, 74);
  EXPECT_EQ(q.frame.y, kHeaderHeight + 24);
  EXPECT_EQ(q.frame.width, 332);
  EXPECT_EQ(q.code.width, 292);
  EXPECT_EQ(q.caption.y, q.frame.bottom() + 16);
  EXPECT_EQ(q.bottom, 560);
}

TEST(Picco4R2Qr, PaneNavigation) {
  using PiccoSheet::Pane;
  EXPECT_EQ(PiccoSheet::parentPane(Pane::Qr), Pane::More);
  EXPECT_EQ(PiccoSheet::tabCount(Pane::Qr, true), 1);
  EXPECT_EQ(PiccoSheet::paneBlocks(Pane::Qr, 0, PiccoSheet::Availability{}).count, 0);
}
