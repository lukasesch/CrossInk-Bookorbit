#include <gtest/gtest.h>

#include "components/picco/PiccoLibraryLayout.h"

// Lane 4A2: header-relative Library bands and the rocker over controls around a list.
using namespace PiccoLibraryLayout;
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
}  // namespace

TEST(PiccoLibrary4A2, BandsFollowTheHeaderTop) {
  for (const int headerTop : {0, 9}) {
    const LibraryBands b = libraryBands(kScreen, headerTop + PiccoTokens::kHeaderHeight);
    EXPECT_EQ(b.tabs.y, headerTop + 96);
    EXPECT_EQ(b.sort.y, headerTop + 176);
    EXPECT_EQ(b.body.y, headerTop + 256);
    EXPECT_EQ(b.body.bottom(), 800 - PiccoTokens::kListBottomReserve);
    EXPECT_EQ(b.tabs.x, 24);
    EXPECT_EQ(b.body.width, 432);
  }
}

TEST(PiccoLibrary4A2, RockerReachesALeadingTab) {
  // Saved tab: focus 0 = Books tab, then 5 rows, 3 per page.
  FocusCursor c{1, 0};  // first row
  c = rockerStepAround(c, -1, 1, 5, 0, 3);
  EXPECT_EQ(c.focus, 0);
  EXPECT_EQ(c.top, 0);
  c = rockerStepAround(c, -1, 1, 5, 0, 3);  // wraps to the last row, flipping the page
  EXPECT_EQ(c.focus, 5);
  EXPECT_EQ(c.top, 2);
  c = rockerStepAround(c, 1, 1, 5, 0, 3);  // past the last row: back to the tab, page kept
  EXPECT_EQ(c.focus, 0);
  EXPECT_EQ(c.top, 2);
  c = rockerStepAround(c, 1, 1, 5, 0, 3);
  EXPECT_EQ(c.focus, 1);
  EXPECT_EQ(c.top, 0);
}

TEST(PiccoLibrary4A2, RockerReachesTrailingTransferPair) {
  // Files: 4 rows then USB and Wi-Fi.
  FocusCursor c{3, 0};
  c = rockerStepAround(c, 1, 0, 4, 2, 4);
  EXPECT_EQ(c.focus, 4);
  c = rockerStepAround(c, 1, 0, 4, 2, 4);
  EXPECT_EQ(c.focus, 5);
  c = rockerStepAround(c, 1, 0, 4, 2, 4);
  EXPECT_EQ(c.focus, 0);
  c = rockerStepAround(c, -1, 0, 4, 2, 4);
  EXPECT_EQ(c.focus, 5);
}

TEST(PiccoLibrary4A2, RockerWithNoRowsCyclesTheControls) {
  FocusCursor c{0, 0};
  c = rockerStepAround(c, 1, 0, 0, 2, 4);
  EXPECT_EQ(c.focus, 1);
  c = rockerStepAround(c, 1, 0, 0, 2, 4);
  EXPECT_EQ(c.focus, 0);
  EXPECT_EQ(rockerStepAround(FocusCursor{}, 1, 0, 0, 0, 4).focus, 0);
}
