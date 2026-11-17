#include <gtest/gtest.h>

#include <vector>

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoPagedList.h"
#include "components/picco/PiccoValueSteps.h"
#include "components/picco/home/PiccoHomeLayout.h"

using PiccoLayout::Box;
using namespace PiccoTokens;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
constexpr int kSectionText = 21;

PiccoPagedList::Entry section() { return PiccoPagedList::Entry{true, 0}; }
PiccoPagedList::Entry row(const int16_t height = kRowOneLine) { return PiccoPagedList::Entry{false, height}; }
}  // namespace

// The header band starts at UITheme::getHeaderRect's y; the simulator's X4 Pro profile puts it 9 px down.
TEST(Picco4B2Home, HeaderTopMovesHeaderAndHero) {
  for (const int headerTop : {0, 9}) {
    const auto home = PiccoHomeLayout::homeLayout(kScreen, headerTop);
    EXPECT_EQ(home.header.y, headerTop);
    EXPECT_EQ(home.header.height, kHeaderHeight);
    EXPECT_EQ(home.hero.area.y, headerTop + 96);
    EXPECT_EQ(home.stats.card.y, headerTop + 304);
    // Tiles keep the bottom gutter and absorb the offset (an odd remainder rounds down).
    EXPECT_LE(home.tiles[3].rect.bottom(), 776);
    EXPECT_GE(home.tiles[3].rect.bottom(), 775);
  }
  EXPECT_EQ(PiccoHomeLayout::homeLayout(kScreen, 9).tiles[0].rect.height, 103);
}

// T_Controls2: a page that opens mid-section repeats the label as "Touch (continued)".
TEST(Picco4B2PagedList, ContinuedSectionRepeatsItsLabel) {
  // Buttons (4 rows), Touch (hint row + 6 rows), Gestures (2 rows).
  std::vector<PiccoPagedList::Entry> e{section(), row(), row(), row(), row(), section(), row(kRowWithHint)};
  for (int i = 0; i < 6; ++i) e.push_back(row());
  e.push_back(section());
  e.push_back(row());
  e.push_back(row());
  const int count = static_cast<int>(e.size());
  std::vector<PiccoPagedList::Slot> slots(count);
  int16_t starts[8] = {};
  const int top = kHeaderHeight;
  const int bottom = 800 - kListBottomReserve;
  const int pages = PiccoPagedList::paginate(e.data(), count, top, bottom, kSectionText, slots.data(), starts, 8, true);
  ASSERT_GE(pages, 2);
  const int first = starts[1];
  EXPECT_FALSE(e[first].section);
  EXPECT_TRUE(PiccoPagedList::opensContinued(e.data(), count, first));
  EXPECT_EQ(PiccoPagedList::owningSection(e.data(), first), 5);
  // Label block (20 above, text, 12 below) before the first row; no hairline over it.
  EXPECT_EQ(PiccoPagedList::continuedLabelY(top), top + kSectionFirstAbove);
  EXPECT_EQ(slots[first].y, top + kSectionFirstAbove + kSectionText + kSectionBelow);
  EXPECT_FALSE(slots[first].hairline);
  // Page 1 opens on a section: nothing repeated.
  EXPECT_FALSE(PiccoPagedList::opensContinued(e.data(), count, 0));
}

TEST(Picco4B2PagedList, WithoutContinuedLabelsAPageStartsAGroupGapDown) {
  std::vector<PiccoPagedList::Entry> e{section()};
  for (int i = 0; i < 12; ++i) e.push_back(row());
  std::vector<PiccoPagedList::Slot> slots(e.size());
  int16_t starts[4] = {};
  PiccoPagedList::paginate(e.data(), static_cast<int>(e.size()), kHeaderHeight, 736, kSectionText, slots.data(), starts,
                           4);
  EXPECT_EQ(slots[starts[1]].y, kHeaderHeight + kGroupGap);
}

// T_SettingsDisplay "Sleep after": minus 64, value 84, plus 64, 8 px apart, right on the gutter.
TEST(Picco4B2Stepper, MatchesTheBoard) {
  const auto s = PiccoLayout::stepperLayout(456, 200);
  EXPECT_EQ(s.plus.right(), 456);
  EXPECT_EQ(s.plus.width, kStepperButtonW);
  EXPECT_EQ(s.plus.height, kStepperButtonH);
  EXPECT_EQ(s.value.width, 84);
  EXPECT_EQ(s.value.right() + kNeighbourGap, s.plus.x);
  EXPECT_EQ(s.minus.right() + kNeighbourGap, s.value.x);
  EXPECT_EQ(s.minus.x, 228);
  EXPECT_EQ(s.minus.y, 170);
  // Fits an 80 px row with 10 px clearance.
  EXPECT_EQ(rowHeightForControl(s.minus.height), kRowWithControl);
}

TEST(Picco4B2Steps, SleepLadder) {
  using namespace PiccoValueSteps;
  const auto* l = kSleepMinutes;
  const int n = kSleepMinutesCount;
  EXPECT_EQ(step(l, n, 10, 1), 15);
  EXPECT_EQ(step(l, n, 10, -1), 5);
  EXPECT_EQ(step(l, n, 7, 1), 10);  // off-ladder values snap to the neighbour
  EXPECT_EQ(step(l, n, 7, -1), 5);
  EXPECT_EQ(step(l, n, 1, -1), 1);   // clamped, no wrap
  EXPECT_EQ(step(l, n, 31, 1), 31);  // "never" is the top
  EXPECT_EQ(step(l, n, 30, 1), 31);
  EXPECT_EQ(indexFor(l, n, 7), 4);
  EXPECT_EQ(indexFor(l, n, 31), n - 1);
}

TEST(Picco4B2Steps, EnumIndexClamps) {
  EXPECT_EQ(PiccoValueSteps::stepIndex(0, 6, -1), 0);
  EXPECT_EQ(PiccoValueSteps::stepIndex(5, 6, 1), 5);
  EXPECT_EQ(PiccoValueSteps::stepIndex(2, 6, 1), 3);
  EXPECT_EQ(PiccoValueSteps::stepIndex(2, 6, -1), 1);
}
