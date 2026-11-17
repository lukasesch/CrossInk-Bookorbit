#include <gtest/gtest.h>

#include <vector>

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoPagedList.h"
#include "components/picco/PiccoSystemLists.h"

using namespace PiccoTokens;
using PiccoSystemLists::kSection;

// T_Wifi: "Available" + networks, then "More" + the hidden-network row (T_Wifi2).
TEST(Picco4B3Wifi, RowsMapToNetworks) {
  constexpr int scanned = 3;
  EXPECT_EQ(PiccoSystemLists::wifiRowCount(scanned, true), 6);
  const int expected[] = {kSection, 0, 1, 2, kSection, 3};
  for (int row = 0; row < 6; ++row) EXPECT_EQ(PiccoSystemLists::wifiNetworkAt(row, scanned, true), expected[row]);
  for (int network = 0; network <= scanned; ++network) {
    EXPECT_EQ(PiccoSystemLists::wifiNetworkAt(PiccoSystemLists::wifiRowOf(network, scanned), scanned, true), network);
  }
  EXPECT_EQ(PiccoSystemLists::wifiNetworkAt(6, scanned, true), kSection);
}

// A failed scan leaves only the hidden-network row under "More".
TEST(Picco4B3Wifi, EmptyScanKeepsHiddenRow) {
  EXPECT_EQ(PiccoSystemLists::wifiRowCount(0, true), 2);
  EXPECT_EQ(PiccoSystemLists::wifiNetworkAt(0, 0, true), kSection);
  EXPECT_EQ(PiccoSystemLists::wifiNetworkAt(1, 0, true), 0);
  EXPECT_EQ(PiccoSystemLists::wifiRowOf(0, 0), 1);
}

TEST(Picco4B3Wifi, SignalLevels) {
  EXPECT_EQ(PiccoSystemLists::wifiSignalLevel(-40), 3);
  EXPECT_EQ(PiccoSystemLists::wifiSignalLevel(-60), 3);
  EXPECT_EQ(PiccoSystemLists::wifiSignalLevel(-61), 2);
  EXPECT_EQ(PiccoSystemLists::wifiSignalLevel(-70), 2);
  EXPECT_EQ(PiccoSystemLists::wifiSignalLevel(-71), 1);
}

// T_HyphenationPacks: Installed first, then Available; a group without items has no label.
TEST(Picco4B3Split, InstalledThenAvailable) {
  const bool installed[] = {true, false, true, false, false};
  int16_t order[7];
  const int rows = PiccoSystemLists::splitInstalled(installed, 5, order);
  const int16_t expected[] = {kSection, 0, 2, kSection, 1, 3, 4};
  ASSERT_EQ(rows, 7);
  for (int i = 0; i < rows; ++i) EXPECT_EQ(order[i], expected[i]);
  EXPECT_TRUE(PiccoSystemLists::sectionIsInstalled(order, rows, 0, installed));
  EXPECT_FALSE(PiccoSystemLists::sectionIsInstalled(order, rows, 3, installed));

  const bool none[] = {false, false};
  EXPECT_EQ(PiccoSystemLists::splitInstalled(none, 2, order), 3);
  EXPECT_EQ(order[0], kSection);
  EXPECT_FALSE(PiccoSystemLists::sectionIsInstalled(order, 3, 0, none));
}

// "Get" / "Remove": 60 px framed control, 10 px clear of the 80 px row's hairlines.
TEST(Picco4B3RowButton, GeometryKeepsClearance) {
  EXPECT_EQ(PiccoLayout::rowButtonWidth(30, 22), 92);  // Get + icon: the minimum
  EXPECT_EQ(PiccoLayout::rowButtonWidth(72, 0), 104);  // Remove
  const auto button = PiccoLayout::rowButton(456, 100 + kRowWithControl / 2, 92);
  EXPECT_EQ(button.right(), 456);
  EXPECT_EQ(button.height, kActionHeight);
  EXPECT_EQ(button.y - 100, kControlClearance);
  EXPECT_EQ(100 + kRowWithControl - button.bottom(), kControlClearance);
}

// Hyphenation note before a section: rows of 80 page at the indicator band.
TEST(Picco4B3Split, PacksPageLikeTheBoard) {
  // Installed: 2 rows, Available: 5 rows (T_HyphenationPacks shows 4 of them on page 1).
  std::vector<PiccoPagedList::Entry> e{{true, 0}, {false, kRowWithHint}, {false, kRowWithControl}, {true, 0}};
  for (int i = 0; i < 5; ++i) e.push_back({false, kRowWithControl});
  std::vector<PiccoPagedList::Slot> slots(e.size());
  int16_t starts[4];
  // Header 80, note 20 + 21, then the section's 30 (drawn as the list top here).
  const int top = 80 + 20 + 21 + kSectionAbove - kSectionFirstAbove;
  const int pages = PiccoPagedList::paginate(e.data(), static_cast<int>(e.size()), top, 800 - kListBottomReserve, 21,
                                             slots.data(), starts, 4, true);
  EXPECT_EQ(pages, 2);
  EXPECT_EQ(starts[1], 8);
}
