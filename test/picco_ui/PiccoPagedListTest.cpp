#include <gtest/gtest.h>

#include <cstdlib>
#include <vector>

#include "components/picco/PiccoDialogLayout.h"
#include "components/picco/PiccoPagedList.h"

using namespace PiccoPagedList;
using namespace PiccoTokens;

namespace {
constexpr int kTop = kHeaderHeight;
constexpr int kBottom = 800 - kListBottomReserve;
constexpr int kSectionText = 21;

Entry section() { return Entry{true, 0}; }
Entry row(const int16_t height = kRowOneLine) { return Entry{false, height}; }

// The T_Settings root: Reading (3 rows), Device (4), Connections (4).
std::vector<Entry> settingsRoot() {
  std::vector<Entry> e{section(), row(), row(), row(), section(), row(), row(), row(), row()};
  e.push_back(section());
  for (int i = 0; i < 4; ++i) e.push_back(row());
  return e;
}
}  // namespace

TEST(PiccoPagedList, RockerFlipsAtThePageEdge) {
  Cursor c{4, 0};
  c = rockerStep(c, 1, 5, 12);
  EXPECT_EQ(c.selected, 5);
  EXPECT_EQ(c.top, 5);  // left the page: whole-page flip
  c = rockerStep(c, -1, 5, 12);
  EXPECT_EQ(c.selected, 4);
  EXPECT_EQ(c.top, 0);
}

TEST(PiccoPagedList, RockerKeepsThePageWhileOnIt) {
  const Cursor c = rockerStep(Cursor{1, 0}, 1, 5, 12);
  EXPECT_EQ(c.selected, 2);
  EXPECT_EQ(c.top, 0);
}

TEST(PiccoPagedList, RockerWrapsToTheClampedLastPage) {
  const Cursor c = rockerStep(Cursor{0, 0}, -1, 5, 12);
  EXPECT_EQ(c.selected, 11);
  EXPECT_EQ(c.top, 7);  // the list clamps its last viewport to count - rows
  const Cursor back = rockerStep(c, 1, 5, 12);
  EXPECT_EQ(back.selected, 0);
  EXPECT_EQ(back.top, 0);
}

TEST(PiccoPagedList, SettingsRootMatchesTheBoards) {
  const auto e = settingsRoot();
  std::vector<Slot> slots(e.size());
  int16_t starts[8] = {};
  const int pages =
      paginate(e.data(), static_cast<int>(e.size()), kTop, kBottom, kSectionText, slots.data(), starts, 8);
  // T_Settings: Reading + Device on page 1, Connections on page 2.
  ASSERT_EQ(pages, 2);
  EXPECT_EQ(starts[1], 9);
  // Reading label 20 below the header, first row 12 below the label, no hairline on it.
  EXPECT_EQ(slots[0].y, kTop + kSectionFirstAbove);
  EXPECT_EQ(slots[1].y, kTop + kSectionFirstAbove + kSectionText + kSectionBelow);
  EXPECT_FALSE(slots[1].hairline);
  EXPECT_TRUE(slots[2].hairline);
  // Device label 30 below the last Reading row.
  EXPECT_EQ(slots[4].y, slots[3].y + kRowOneLine + kSectionAbove);
  EXPECT_FALSE(slots[5].hairline);
  EXPECT_LE(slots[8].y + slots[8].height, kBottom);
  // Page 2 opens with its section 20 below the header.
  EXPECT_EQ(slots[9].y, kTop + kSectionFirstAbove);
}

TEST(PiccoPagedList, SectionNeverEndsAPage) {
  // Rows fill the page so the last section's label would fit, but not its first row.
  std::vector<Entry> e{section()};
  for (int i = 0; i < 8; ++i) e.push_back(row());
  e.push_back(section());
  e.push_back(row());
  std::vector<Slot> slots(e.size());
  int16_t starts[4] = {};
  const int pages =
      paginate(e.data(), static_cast<int>(e.size()), kTop, kBottom, kSectionText, slots.data(), starts, 4);
  ASSERT_EQ(pages, 2);
  EXPECT_EQ(starts[1], 9);  // the section moved over with its row
  for (int i = 0; i < starts[1]; ++i) EXPECT_LE(slots[i].y + slots[i].height, kBottom) << i;
}

TEST(PiccoPagedList, ContinuedPageStartsWithAGapAndNoHairline) {
  std::vector<Entry> e;
  for (int i = 0; i < 12; ++i) e.push_back(row());
  std::vector<Slot> slots(e.size());
  int16_t starts[4] = {};
  const int pages =
      paginate(e.data(), static_cast<int>(e.size()), kTop, kBottom, kSectionText, slots.data(), starts, 4);
  ASSERT_EQ(pages, 2);
  const int second = starts[1];
  EXPECT_EQ(slots[second].y, kTop + kGroupGap);
  EXPECT_FALSE(slots[second].hairline);
  EXPECT_EQ(pageOf(starts, pages, second - 1), 0);
  EXPECT_EQ(pageOf(starts, pages, second), 1);
}

TEST(PiccoPagedList, SelectionSkipsSections) {
  const auto e = settingsRoot();
  const int count = static_cast<int>(e.size());
  EXPECT_EQ(nextSelectable(e.data(), count, 3, 1), 5);
  EXPECT_EQ(nextSelectable(e.data(), count, 1, -1), count - 1);
  int16_t starts[2] = {0, 9};
  EXPECT_EQ(firstSelectableOnPage(e.data(), count, starts, 2, 1), 10);
}

TEST(PiccoDialogLayout, EqualButtonPairInsideTheCard) {
  const auto g = PiccoDialogLayout::geometry(PiccoLayout::Box{0, 0, 480, 800}, 200);
  EXPECT_EQ(g.card.x, kDialogInset);
  EXPECT_EQ(g.card.right(), 480 - kDialogInset);
  EXPECT_EQ(g.cancel.width, g.confirm.width);
  EXPECT_EQ(g.confirm.x - g.cancel.right(), kDialogButtonGap);
  EXPECT_EQ(g.cancel.x, g.card.x + kDialogPadding);
  EXPECT_EQ(g.confirm.right(), g.card.right() - kDialogPadding);
  EXPECT_EQ(g.cancel.height, kDialogButtonHeight);
  EXPECT_EQ(g.card.bottom() - g.cancel.bottom(), kDialogPadding);
  EXPECT_EQ(g.cancel.y - (g.textTop + 200), kDialogPadding);
  // Centred on the screen.
  EXPECT_LE(std::abs((g.card.y + g.card.bottom()) - 800), 1);
}
