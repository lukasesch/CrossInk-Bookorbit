#include <gtest/gtest.h>

#include "components/picco/PiccoSystemLayout.h"

// Lane 4A3: sign-in form and keyboard band (T_Login), reading stats (T_Stats), transfer status.
using PiccoLayout::Box;

namespace {
constexpr Box kScreen{0, 0, 480, 800};
}  // namespace

TEST(PiccoSystem4A3, KeyboardBandSitsOnTheBottom) {
  using namespace PiccoKeyboardLayout;
  EXPECT_EQ(bandHeight(4), 264);  // the board's four rows
  EXPECT_EQ(bandHeight(5), 326);  // with the number row
  const Box b = band(kScreen, 4);
  EXPECT_EQ(b.y, 536);
  EXPECT_EQ(b.bottom(), 800);
  const Box keys = keysRect(b);
  EXPECT_EQ(keys.x, 6);
  EXPECT_EQ(keys.y, 546);
  EXPECT_EQ(keys.height, 4 * 56 + 3 * 6);
}

TEST(PiccoSystem4A3, SignInFormFollowsTheHeader) {
  using namespace PiccoKeyboardLayout;
  for (const int headerTop : {0, 9}) {
    FormInputs in;
    in.headerBottom = headerTop + PiccoTokens::kHeaderHeight;
    in.leadLines = 2;
    in.labelHeight = 21;
    in.fieldCount = 3;
    in.editing = -1;
    in.trailingHeight = PiccoTokens::kPillHeight;
    const FormLayout f = formLayout(kScreen, in);
    EXPECT_TRUE(f.leadShown);
    EXPECT_EQ(f.lead.y, headerTop + 96);
    EXPECT_EQ(f.labels[0].y, headerTop + 154);
    EXPECT_EQ(f.fields[0].y, headerTop + 179);
    EXPECT_EQ(f.fields[0].height, 60);
    EXPECT_EQ(f.fields[1].y, headerTop + 274);
    EXPECT_EQ(f.fields[2].bottom(), headerTop + 429);
    EXPECT_EQ(f.trailing.y, headerTop + 445);
    EXPECT_EQ(f.fields[2].x, 24);
    EXPECT_EQ(f.fields[2].width, 432);
  }
}

TEST(PiccoSystem4A3, LeadGivesWayToAGrowingField) {
  using namespace PiccoKeyboardLayout;
  FormInputs in;
  in.headerBottom = 89;
  in.leadLines = 2;
  in.labelHeight = 21;
  in.fieldCount = 3;
  in.editing = 0;
  in.keyboardTop = band(kScreen, 5).y;  // 474
  const FormLayout fits = formLayout(kScreen, in);
  EXPECT_TRUE(fits.leadShown);
  EXPECT_TRUE(fits.fits);
  in.editingExtra = extraHeightForLines(3, 25);
  const FormLayout grown = formLayout(kScreen, in);
  EXPECT_FALSE(grown.leadShown);
  EXPECT_TRUE(grown.fits);
  EXPECT_EQ(grown.fields[0].height, 110);
}

TEST(PiccoSystem4A3, FieldTextIsPaddedAndCentred) {
  const Box text = PiccoKeyboardLayout::fieldText(Box{24, 200, 432, 60}, 24, 50);
  EXPECT_EQ(text.x, 40);
  EXPECT_EQ(text.y, 218);
  EXPECT_EQ(text.width, 432 - 32 - 50);
}

TEST(PiccoSystem4A3, StatsSectionsMatchTheBoard) {
  using namespace PiccoStatsLayout;
  for (const int headerTop : {0, 9}) {
    const Layout l = layout(kScreen, headerTop + 80, true);
    EXPECT_EQ(l.tabs.y, headerTop + 96);
    EXPECT_EQ(l.streak.y, headerTop + 176);
    EXPECT_EQ(l.weekLabel.y, headerTop + 290);
    EXPECT_EQ(l.weekChart.y, headerTop + 323);
    EXPECT_EQ(l.totalsLabel.y, headerTop + 483);
    EXPECT_EQ(l.totalsGrid.y, headerTop + 516);
    EXPECT_EQ(l.dayLabel.y, headerTop + 674);
    EXPECT_EQ(l.dayParts.y, headerTop + 707);
    EXPECT_LE(l.dayParts.bottom(), 800);
  }
  const Layout single = layout(kScreen, 80, false);
  EXPECT_TRUE(single.tabs.empty());
  EXPECT_EQ(single.streak.y, 96);
}

TEST(PiccoSystem4A3, StatsCellsSplitTheWidth) {
  using namespace PiccoStatsLayout;
  const Box chart{24, 323, 432, 130};
  EXPECT_EQ(weekColumn(chart, 0).width, 51);
  EXPECT_EQ(weekColumn(chart, 6).x, 24 + 6 * 63);
  const Box grid{24, 516, 432, 128};
  EXPECT_EQ(totalsCell(grid, 4).x, 24 + 144);
  EXPECT_EQ(totalsCell(grid, 4).y, 580);
  EXPECT_EQ(dayPart(Box{24, 707, 432, 64}, 3).x, 24 + 3 * 111);
}

TEST(PiccoSystem4A3, BarsScaleToTheBusiestDay) {
  using PiccoStatsLayout::barHeight;
  EXPECT_EQ(barHeight(66, 66, 76), 76);
  EXPECT_EQ(barHeight(31, 66, 76), 36);
  EXPECT_EQ(barHeight(1, 1000, 76), 2);  // a short day still shows
  EXPECT_EQ(barHeight(0, 66, 76), 0);
  EXPECT_EQ(barHeight(5, 0, 76), 0);
}

TEST(PiccoSystem4A3, SharesAndBusiest) {
  using namespace PiccoStatsLayout;
  EXPECT_EQ(sharePercent(48, 100), 48);
  EXPECT_EQ(sharePercent(1, 3), 33);
  EXPECT_EQ(sharePercent(2, 3), 67);
  EXPECT_EQ(sharePercent(5, 0), 0);
  EXPECT_EQ(busiestIndex(std::array<uint32_t, 4>{14, 21, 48, 17}), 2);
  EXPECT_EQ(busiestIndex(std::array<uint32_t, 4>{0, 0, 0, 0}), -1);
  EXPECT_EQ(busiestIndex(std::array<uint32_t, 3>{5, 5, 1}), 0);
}

TEST(PiccoSystem4A3, StatusBandCentresUnderTheHeader) {
  const Box band = PiccoTransferLayout::statusBand(kScreen, 89);
  EXPECT_EQ(band.height, 236);
  EXPECT_EQ(band.y, 89 + (711 - 236) / 2);
  EXPECT_EQ(band.x, 24);
}
