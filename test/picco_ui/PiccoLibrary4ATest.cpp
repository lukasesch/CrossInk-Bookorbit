#include <gtest/gtest.h>

#include "components/picco/PiccoLibraryLayout.h"

// Lane 4A geometry against the T_Library, T_LibrarySort and T_Files boards (480 x 800).
using namespace PiccoLibraryLayout;
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
constexpr int kHeaderBottom = PiccoTokens::kHeaderHeight;
}  // namespace

TEST(PiccoLibrary4A, TabsSortRowAndBodyMatchTheBoard) {
  const Box tabs = bandBelow(kScreen, kHeaderBottom, PiccoTokens::kTabBarHeight);
  EXPECT_EQ(tabs.x, 24);
  EXPECT_EQ(tabs.y, 96);
  EXPECT_EQ(tabs.width, 432);
  const Box sort = bandBelow(kScreen, tabs.bottom(), PiccoTokens::kTabBarHeight);
  EXPECT_EQ(sort.y, 176);
  EXPECT_EQ(sort.bottom() + kBandGap, 256);  // grid top
}

TEST(PiccoLibrary4A, SegmentCellsSplitTheFrameEvenly) {
  const Box tabs{24, 96, 432, 64};
  const Box books = segmentCell(tabs, 2, 0);
  const Box saved = segmentCell(tabs, 2, 1);
  EXPECT_EQ(books.x, 29);  // 1 px frame + 4 px pad
  EXPECT_EQ(books.y, 101);
  EXPECT_EQ(books.height, 54);
  EXPECT_EQ(saved.x, books.right() + PiccoTokens::kTabPad);
  EXPECT_EQ(saved.right(), tabs.right() - 5);
  EXPECT_EQ(books.width, saved.width);
  EXPECT_TRUE(segmentCell(tabs, 2, 2).empty());
}

TEST(PiccoLibrary4A, SegmentHitCoversTheWholeFrame) {
  const Box tabs{24, 96, 432, 64};
  EXPECT_EQ(segmentAt(tabs, 2, 25, 97), 0);
  EXPECT_EQ(segmentAt(tabs, 2, 455, 159), 1);
  EXPECT_EQ(segmentAt(tabs, 2, 239, 120), 0);  // pad between the cells: nearer cell
  EXPECT_EQ(segmentAt(tabs, 2, 241, 120), 1);
  EXPECT_EQ(segmentAt(tabs, 2, 10, 120), -1);
}

TEST(PiccoLibrary4A, ViewToggleIsTwo56PxCells) {
  EXPECT_EQ(segmentedWidth(2, kViewToggleCell), 126);
  const Box toggle{456 - 126, 176, 126, 64};
  EXPECT_EQ(segmentCell(toggle, 2, 0).width, 56);
  EXPECT_EQ(segmentCell(toggle, 2, 1).width, 56);
}

TEST(PiccoLibrary4A, GridIsThreeByTwoWithBoardGaps) {
  const Box area{24, 256, 432, 800 - 64 - 256};
  EXPECT_EQ(gridRows(area.height), 2);
  EXPECT_EQ(gridPageSize(area.height), 6);
  EXPECT_EQ(gridCellWidth(area.width), 136);
  const Box first = gridCell(area, 0);
  const Box third = gridCell(area, 2);
  const Box fourth = gridCell(area, 3);
  EXPECT_EQ(first.x, 24);
  EXPECT_EQ(third.x, 24 + 2 * (136 + 12));
  EXPECT_EQ(fourth.y, 256 + 232 + 16);
  EXPECT_LE(gridCell(area, 5).bottom(), 736);
  EXPECT_EQ(gridCover(first).height, 182);
  EXPECT_EQ(gridPageStart(7, 6), 6);
  EXPECT_EQ(gridPageStart(5, 6), 0);
}

TEST(PiccoLibrary4A, FilesTransferPairEndsAtTheListLimit) {
  const Box usb = transferButton(kScreen, 0);
  const Box wifi = transferButton(kScreen, 1);
  EXPECT_EQ(usb.y, 676);
  EXPECT_EQ(usb.bottom(), 736);
  EXPECT_EQ(usb.x, 24);
  EXPECT_EQ(wifi.x, usb.right() + 12);
  EXPECT_EQ(wifi.right(), 456);
  // 7 one-line rows between the path band and the pair, as on the board.
  EXPECT_EQ(rowsBetween(172, usb.y - kBandGap, PiccoTokens::kRowOneLine), 7);
}

TEST(PiccoLibrary4A, OutlineButtonPadsLabelAndChevron) {
  const Box b = outlineButton(24, 208, 100, 24);
  EXPECT_EQ(b.width, 14 + 100 + 6 + 24 + 12);
  EXPECT_EQ(b.height, 60);
  EXPECT_EQ(b.y, 178);
}
