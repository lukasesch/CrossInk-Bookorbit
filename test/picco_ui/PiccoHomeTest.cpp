#include <gtest/gtest.h>

#include <string>

#include "components/picco/home/PiccoHomeLayout.h"
#include "components/picco/home/PiccoHomeStops.h"

using PiccoLayout::Box;
using namespace PiccoHomeLayout;
using PiccoHomeStops::Action;
using PiccoHomeStops::HeroMode;
using PiccoHomeStops::Target;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
constexpr Home kHome = homeLayout(kScreen);

void expectBox(const Box& b, const int x, const int y, const int w, const int h) {
  EXPECT_EQ(b.x, x);
  EXPECT_EQ(b.y, y);
  EXPECT_EQ(b.width, w);
  EXPECT_EQ(b.height, h);
}

std::string syncAge(const uint32_t seconds) {
  char out[16];
  PiccoHomeStops::formatSyncAge(seconds, "now", out, sizeof(out));
  return out;
}

std::string minutes(const uint32_t m) {
  char out[24];
  PiccoHomeStops::formatMinutes(m, out, sizeof(out));
  return out;
}
}  // namespace

// Board T_Combined at 480 x 800.
TEST(PiccoHome, HeroMatchesTheBoard) {
  const Hero& h = kHome.hero;
  expectBox(h.area, 24, 96, 432, 192);
  expectBox(h.cover, 24, 96, 128, 192);
  expectBox(h.text, 172, 96, 284, 192);
  expectBox(h.label, 172, 96, 284, 20);
  EXPECT_EQ(h.title.y, 118);
  expectBox(h.pill, 172, 228, 284, 60);
  EXPECT_EQ(h.pill.bottom(), h.cover.bottom());
  expectBox(h.progress, 172, 209, 284, 7);
  expectBox(h.meta, 172, 183, 284, 20);
}

TEST(PiccoHome, TitleAndAuthorClearTheMetaRow) {
  const Hero& h = kHome.hero;
  EXPECT_EQ(titleLine(h).height, 36);
  EXPECT_EQ(authorLine(h).y, titleLine(h).bottom());
  EXPECT_LE(authorLine(h).bottom(), h.meta.y);
}

TEST(PiccoHome, StatsCardMatchesTheBoard) {
  const Stats& s = kHome.stats;
  expectBox(s.card, 24, 304, 432, 224);
  expectBox(s.inner, 40, 316, 400, 200);
  expectBox(s.head, 40, 316, 400, 21);
  expectBox(s.streak, 40, 345, 150, 104);
  expectBox(s.chart, 208, 345, 232, 104);
  expectBox(s.columns[0], 208, 345, 28, 104);
  EXPECT_EQ(s.columns[6].right(), 440);
  EXPECT_EQ(s.barBase, 424);
  EXPECT_EQ(s.barMaxH, 79);
  EXPECT_EQ(s.totalsRule.y, 461);
  EXPECT_EQ(s.totals[0].y, 470);
  EXPECT_EQ(s.totals[0].bottom(), s.inner.bottom());
  EXPECT_EQ(s.totalsDividers[0].x, 40 + 133);
  EXPECT_EQ(s.totals[1].x, 40 + 133 + 10);
}

TEST(PiccoHome, TilesMatchTheBoard) {
  expectBox(kHome.tiles[0].rect, 24, 544, 208, 108);
  expectBox(kHome.tiles[1].rect, 248, 544, 208, 108);
  expectBox(kHome.tiles[2].rect, 24, 668, 208, 108);
  expectBox(kHome.tiles[3].rect, 248, 668, 208, 108);
  EXPECT_EQ(kHome.tiles[3].rect.bottom(), 776);
  expectBox(kHome.tiles[0].icon, 40, 556, 32, 32);
  EXPECT_EQ(kHome.tiles[0].subtitle.bottom(), 544 + 108 - 12);
}

TEST(PiccoHome, FocusRingsStayInTheGutters) {
  for (int i = 0; i < PiccoHomeStops::kFocusCount; ++i) {
    const auto stop = PiccoHomeStops::focusStop(kHome, i);
    const auto ring = focusRing(stop.rect, stop.radius);
    EXPECT_GE(ring.box.x, 0) << i;
    EXPECT_LE(ring.box.right(), 480) << i;
    for (int j = 0; j < PiccoHomeStops::kFocusCount; ++j) {
      if (j == i) continue;
      EXPECT_FALSE(ring.box.overlaps(PiccoHomeStops::focusStop(kHome, j).rect)) << i << " over " << j;
    }
  }
  const auto resume = focusRing(kHome.hero.pill, 30);
  EXPECT_EQ(resume.radius, 34);
  EXPECT_FALSE(resume.box.overlaps(kHome.hero.progress));
  EXPECT_FALSE(resume.box.overlaps(kHome.hero.cover));
}

TEST(PiccoHome, FocusOrderWraps) {
  EXPECT_EQ(PiccoHomeStops::kFocusOrder[0], Target::Resume);
  EXPECT_EQ(PiccoHomeStops::kFocusOrder[1], Target::Stats);
  EXPECT_EQ(PiccoHomeStops::kFocusOrder[2], Target::BookOrbit);
  EXPECT_EQ(PiccoHomeStops::kFocusOrder[5], Target::Settings);
  EXPECT_EQ(PiccoHomeStops::moveFocus(5, 1), 0);
  EXPECT_EQ(PiccoHomeStops::moveFocus(0, -1), 5);
  EXPECT_EQ(PiccoHomeStops::moveFocus(1, 1), 2);
}

TEST(PiccoHome, BarHeights) {
  EXPECT_EQ(barHeight(0, 60, 79), 0);
  EXPECT_EQ(barHeight(60, 60, 79), 79);
  EXPECT_EQ(barHeight(30, 60, 79), 40);
  EXPECT_EQ(barHeight(1, 600, 79), 2);  // any reading shows
  EXPECT_EQ(barHeight(10, 0, 79), 0);
  EXPECT_EQ(barHeight(90, 60, 79), 79);
}

TEST(PiccoHome, SyncAgeText) {
  EXPECT_EQ(syncAge(0), "now");
  EXPECT_EQ(syncAge(59), "now");
  EXPECT_EQ(syncAge(120), "2 min");
  EXPECT_EQ(syncAge(59 * 60 + 59), "59 min");
  EXPECT_EQ(syncAge(3 * 3600 + 1200), "3 h");
  EXPECT_EQ(syncAge(2 * 86400 + 5), "2 d");
}

TEST(PiccoHome, ReadingTimeText) {
  EXPECT_EQ(minutes(47), "47 min");
  EXPECT_EQ(minutes(312), "5 h 12 m");
  EXPECT_EQ(minutes(21 * 60), "21 h");
}

TEST(PiccoHome, EmptyStateSwapsResumeForLibrary) {
  EXPECT_EQ(PiccoHomeStops::heroMode(0), HeroMode::Empty);
  EXPECT_EQ(PiccoHomeStops::heroMode(1), HeroMode::Book);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Resume, HeroMode::Book, false), Action::OpenBook);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Cover, HeroMode::Book, false), Action::OpenBook);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Resume, HeroMode::Empty, false), Action::OpenLibrary);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Cover, HeroMode::Empty, true), Action::OpenLibrary);
}

TEST(PiccoHome, SyncNeedsAnAccount) {
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Sync, HeroMode::Book, true), Action::Refresh);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Sync, HeroMode::Book, false), Action::None);
  EXPECT_TRUE(PiccoHomeStops::targetRect(kHome, Target::Sync, Box{}).empty());
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Stats, HeroMode::Empty, false), Action::OpenStats);
  EXPECT_EQ(PiccoHomeStops::actionFor(Target::Files, HeroMode::Book, false), Action::OpenFiles);
}
