#include <gtest/gtest.h>

#include "activities/reader/PiccoBookStatsLayout.h"
#include "activities/reader/PiccoReaderListsLayout.h"

// Lane 4R1: Contents tabs (T_Contents), list bands, book stats (T_BookStats), end of book.
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
}  // namespace

TEST(Picco4R1, ContentsTabsSitUnderTheHeader) {
  using namespace PiccoReaderLists;
  for (const int headerTop : {0, 9}) {
    const int headerBottom = headerTop + PiccoTokens::kHeaderHeight;
    const Box tabs = contentsTabs(kScreen, headerBottom);
    EXPECT_EQ(tabs.x, 24);
    EXPECT_EQ(tabs.y, headerBottom + 16);
    EXPECT_EQ(tabs.width, 432);
    EXPECT_EQ(tabs.height, 64);
    EXPECT_EQ(listTop(kScreen, headerBottom, true), tabs.bottom() + 16);
    EXPECT_EQ(listTop(kScreen, headerBottom, false), headerBottom + 16);
  }
}

TEST(Picco4R1, ContentsTabHitTest) {
  using namespace PiccoReaderLists;
  const int headerBottom = 89;
  const int y = contentsTabs(kScreen, headerBottom).y + 32;
  EXPECT_EQ(contentsTabAt(kScreen, headerBottom, 96, y), 0);
  EXPECT_EQ(contentsTabAt(kScreen, headerBottom, 240, y), 1);
  EXPECT_EQ(contentsTabAt(kScreen, headerBottom, 384, y), 2);
  EXPECT_EQ(contentsTabAt(kScreen, headerBottom, 240, headerBottom + 4), -1);  // the gap above the tabs
  EXPECT_EQ(contentsTabAt(kScreen, headerBottom, 10, y), -1);                  // the gutter
}

TEST(Picco4R1, BookStatsFollowsTheBoard) {
  using namespace PiccoBookStatsLayout;
  // Board positions with the header hairline at 81.
  const Layout l = layout(kScreen, 81);
  EXPECT_EQ(l.title.y, 100);
  EXPECT_EQ(l.strip.y, 182);
  EXPECT_EQ(l.strip.height, 80);
  EXPECT_EQ(l.progress.y, 282);
  EXPECT_EQ(l.chartLabel.y, 386);
  EXPECT_EQ(l.chart.y, 419);
  EXPECT_EQ(l.chart.height, 170);
  EXPECT_EQ(l.note.y, 606);
  // Time of day stays on the screen with the simulator's header offset.
  EXPECT_LE(layout(kScreen, 89).dayParts.bottom(), kScreen.bottom());
}

TEST(Picco4R1, BookStatsStripAndProgress) {
  using namespace PiccoBookStatsLayout;
  const Box strip{24, 182, 432, 80};
  EXPECT_EQ(stripText(strip, 0).x, 24);
  EXPECT_EQ(stripText(strip, 1).x, 24 + 144 + 14);
  EXPECT_EQ(stripCell(strip, 2).right(), 456);
  EXPECT_EQ(stripCell(strip, 3).width, 0);
  EXPECT_EQ(progressFill(432, 62), 267);
  EXPECT_EQ(progressFill(432, -1), 0);
  EXPECT_EQ(progressFill(432, 140), 432);
  EXPECT_EQ(chartColumn(Box{24, 0, 432, 170}, 6).right(), 24 + 7 * 51 + 6 * 12);
}

TEST(Picco4R1, EndOfBookButtonsAndRows) {
  using namespace PiccoBookStatsLayout;
  const Box left = eobButton(kScreen, 0);
  const Box right = eobButton(kScreen, 1);
  EXPECT_EQ(left.y, 720);
  EXPECT_EQ(left.height, 60);
  EXPECT_EQ(left.width, 210);
  EXPECT_EQ(right.x, 24 + 210 + 12);
  const Box row = eobRow(Box{24, 400, 432, 0}, 2);
  EXPECT_EQ(row.y, 600);
  EXPECT_EQ(eobCover(row).y, 608);
  EXPECT_EQ(eobCover(row).width, 56);
}
