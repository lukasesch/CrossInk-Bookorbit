#include <gtest/gtest.h>

#include "components/picco/sheet/PiccoSheetModel.h"

using namespace PiccoSheet;

namespace {
constexpr int kWidth = 480;

Availability everything() {
  Availability a;
  a.hasFootnotes = true;
  a.hasDictionary = true;
  a.hasBookmarks = true;
  a.hasClippings = true;
  a.showReadingPaceReset = true;
  a.globalStats = true;
  a.bookStats = true;
  a.publisherFonts = true;
  a.fontRendering = true;
  a.ttfInterpreter = true;
  return a;
}

int countRows(const Blocks& blocks) {
  int rows = 0;
  for (int i = 0; i < blocks.count; ++i) {
    if (blocks.items[static_cast<size_t>(i)].kind != Kind::Heading) ++rows;
  }
  return rows;
}

bool hasRow(const Blocks& blocks, const Row row) {
  for (int i = 0; i < blocks.count; ++i) {
    const Block& b = blocks.items[static_cast<size_t>(i)];
    if (b.kind != Kind::Heading && b.row == row) return true;
  }
  return false;
}
}  // namespace

TEST(PiccoSheetPanes, BackLeadsToTheOpeningPane) {
  EXPECT_EQ(parentPane(Pane::Text), Pane::Main);
  EXPECT_EQ(parentPane(Pane::TextMore), Pane::Text);
  EXPECT_EQ(parentPane(Pane::More), Pane::Main);
  EXPECT_EQ(parentPane(Pane::GoTo), Pane::Main);
  EXPECT_EQ(parentPane(Pane::AutoPageTurn), Pane::More);
  EXPECT_EQ(parentPane(Pane::Advanced), Pane::More);
  EXPECT_EQ(parentPane(Pane::FontRendering), Pane::Advanced);
  EXPECT_EQ(parentPane(Pane::Main), Pane::Main);
}

TEST(PiccoSheetPanes, TwoCellTabs) {
  EXPECT_EQ(tabCount(Pane::Text, false), 2);
  EXPECT_EQ(tabCount(Pane::More, false), 2);
  EXPECT_EQ(tabCount(Pane::Advanced, false), 2);
  EXPECT_EQ(tabCount(Pane::TextMore, true), 1);
  // Go to offers Page only when the book has stable page numbers.
  EXPECT_EQ(tabCount(Pane::GoTo, true), 2);
  EXPECT_EQ(tabCount(Pane::GoTo, false), 1);
}

TEST(PiccoSheetTiles, PerVariant) {
  const Tiles epub = tilesFor(Variant::Epub, true, false);
  ASSERT_EQ(epub.count, 4);
  EXPECT_EQ(epub.items[0], Tile::Text);
  EXPECT_EQ(epub.items[1], Tile::Bookmark);
  EXPECT_EQ(epub.items[2], Tile::Contents);
  EXPECT_EQ(epub.items[3], Tile::More);
  const Tiles xtc = tilesFor(Variant::Reader, true, true);
  ASSERT_EQ(xtc.count, 2);
  EXPECT_EQ(xtc.items[0], Tile::Contents);
  EXPECT_EQ(xtc.items[1], Tile::More);
  EXPECT_EQ(tilesFor(Variant::Reader, false, false).count, 0);  // TXT
  EXPECT_EQ(tilesFor(Variant::Outside, true, true).count, 0);
}

TEST(PiccoSheetRows, EveryDrawerOptionHasAPlace) {
  const Availability a = everything();
  const Row expected[] = {
      Row::FontSize,
      Row::LineSpacing,
      Row::SideMargins,
      Row::Alignment,
      Row::Orientation,
      Row::Hyphenation,
      Row::ForceIndents,
      Row::ExtraSpacing,
      Row::Focus,
      Row::GuideDots,
      Row::DarkMode,
      Row::TextAa,
      Row::WordSpacing,
      Row::CharacterSpacing,
      Row::TopBottomMargins,
      Row::EmbeddedStyle,
      Row::PublisherFonts,
      Row::PublisherPages,
      Row::Images,
      Row::Footnotes,
      Row::AutoPageTurn,
      Row::Bookmarks,
      Row::Clippings,
      Row::Lookup,
      Row::LookupHistory,
      Row::SaveClipping,
      Row::BookDictionary,
      Row::NearbySync,
      Row::SendBook,
      Row::Qr,
      Row::MarkFinished,
      Row::BookStats,
      Row::StatusBar,
      Row::RenderMode,
      Row::Controls,
      Row::Screenshot,
      Row::IndexingMethod,
      Row::TrackStats,
      Row::DictionaryFont,
      Row::DictionaryFontSize,
      Row::FontRendering,
      Row::ResetPace,
      Row::DeleteStats,
      Row::DeleteBookmarks,
      Row::DeleteCache,
      Row::ResetBookSettings,
      Row::TtfHinting,
      Row::TtfInterpreter,
      Row::TtfStemDarkening,
      Row::TtfReset,
  };
  const Pane panes[] = {Pane::Text, Pane::TextMore, Pane::More, Pane::Advanced, Pane::FontRendering};
  for (const Row row : expected) {
    bool found = false;
    for (const Pane pane : panes) {
      for (uint8_t tab = 0; tab < tabCount(pane, false); ++tab) found = found || hasRow(paneBlocks(pane, tab, a), row);
    }
    EXPECT_TRUE(found) << "row " << static_cast<int>(row);
  }
}

TEST(PiccoSheetRows, MissingFeaturesAreLeftOut) {
  Availability a;  // nothing optional
  const Blocks navigate = paneBlocks(Pane::More, 0, a);
  EXPECT_FALSE(hasRow(navigate, Row::Footnotes));
  EXPECT_FALSE(hasRow(navigate, Row::Lookup));
  EXPECT_FALSE(hasRow(navigate, Row::KOReaderSync));
  EXPECT_TRUE(hasRow(navigate, Row::AutoPageTurn));
  a.globalStats = false;
  EXPECT_FALSE(hasRow(paneBlocks(Pane::Advanced, 0, a), Row::TrackStats));
  EXPECT_FALSE(hasRow(paneBlocks(Pane::Advanced, 1, a), Row::DeleteStats));
  EXPECT_FALSE(hasRow(paneBlocks(Pane::More, 1, a), Row::BookStats));
  EXPECT_EQ(countRows(paneBlocks(Pane::FontRendering, 0, a)), 6);  // no interpreter while hinting is off
}

TEST(PiccoSheetRows, KindsAndHints) {
  EXPECT_EQ(kindOf(Row::FontSize), Kind::Stepper);
  EXPECT_EQ(kindOf(Row::Alignment), Kind::Segmented);
  EXPECT_EQ(kindOf(Row::Hyphenation), Kind::Switch);
  EXPECT_EQ(kindOf(Row::Images), Kind::Value);
  EXPECT_EQ(kindOf(Row::Qr), Kind::Cell);
  EXPECT_EQ(kindOf(Row::DeleteCache), Kind::Button);
  EXPECT_TRUE(hasHint(Row::Focus));
  EXPECT_FALSE(hasHint(Row::GuideDots));
}

TEST(PiccoSheetLayout, MainMatchesTheBoard) {
  const MainLayout m = mainLayout(kWidth, true, true, true, 4);
  EXPECT_EQ(m.label.y, 96);
  EXPECT_EQ(m.title.y, 121);
  EXPECT_EQ(m.goTo.y, 199);
  EXPECT_EQ(m.band.y, 286);
  EXPECT_EQ(m.band.bottom(), 424);
  EXPECT_EQ(m.tiles[0].y, 440);
  EXPECT_EQ(m.tiles[0].height, kTileHeight);
  EXPECT_EQ(m.tiles[3].right(), kWidth - 24);
  EXPECT_EQ(m.bottom, 572);
  // Light rows: icon, slider and value side by side, the slider between them.
  const LightRow& row = m.light[0];
  EXPECT_LT(row.icon.right(), row.track.x);
  EXPECT_LT(row.track.right(), row.value.x);
  EXPECT_EQ(row.value.right(), kWidth - 24);
}

TEST(PiccoSheetLayout, OutsideReadersShowTheLightBandOnly) {
  const MainLayout m = mainLayout(kWidth, false, false, true, 0);
  EXPECT_EQ(m.band.y, 96);
  EXPECT_EQ(m.tileCount, 0);
  EXPECT_EQ(m.bottom, m.band.bottom() + kMainFoot);
  EXPECT_LT(mainLayout(kWidth, false, false, false, 0).bottom, m.bottom);
}

TEST(PiccoSheetLayout, PaneHeights) {
  const Availability a = everything();
  EXPECT_EQ(goToLayout(kWidth, true).bottom, 716);
  EXPECT_EQ(autoTurnLayout(kWidth).bottom, 322);
  // Text keeps its taller tab's height so switching tabs never uncovers the page.
  EXPECT_EQ(blockPaneBottom(Pane::Text, a, kWidth), 780);
  EXPECT_EQ(blockPaneBottom(Pane::TextMore, a, kWidth), 736);
  EXPECT_EQ(blockPaneBottom(Pane::More, a, kWidth), 747);
  EXPECT_EQ(blockPaneBottom(Pane::Advanced, a, kWidth), 637);
  for (const Pane pane : {Pane::Text, Pane::TextMore, Pane::More, Pane::Advanced, Pane::FontRendering}) {
    EXPECT_LE(blockPaneBottom(pane, a, kWidth), 800 - PiccoTokens::kSheetDitherStrip) << static_cast<int>(pane);
  }
}

TEST(PiccoSheetLayout, GridsAndRows) {
  const Availability a = everything();
  const Blocks blocks = paneBlocks(Pane::Text, 0, a);
  const BlockLayout layout = layoutBlocks(blocks, kWidth, paneContentTop(2), false);
  // Heading, then three chips on one line.
  EXPECT_EQ(layout.boxes[1].y, layout.boxes[2].y);
  EXPECT_EQ(layout.boxes[2].y, layout.boxes[3].y);
  EXPECT_EQ(layout.boxes[3].right(), kWidth - 24);
  EXPECT_FALSE(layout.hairlineAbove[5]);  // the Text pane draws no separators

  const BlockLayout more = layoutBlocks(paneBlocks(Pane::More, 0, a), kWidth, paneContentTop(2), true);
  // Footnotes and Auto page turn share a line, Bookmarks starts the next one.
  EXPECT_EQ(more.boxes[1].y, more.boxes[2].y);
  EXPECT_EQ(more.boxes[3].y, more.boxes[1].bottom() + kCellGap);

  const BlockLayout style = layoutBlocks(paneBlocks(Pane::Text, 1, a), kWidth, paneContentTop(2), true);
  EXPECT_FALSE(style.hairlineAbove[1]);  // no hairline on a section's first row
  EXPECT_TRUE(style.hairlineAbove[2]);
}

TEST(PiccoSheetLayout, Controls) {
  const Box row{24, 200, 432, 80};
  const Stepper s = stepper(row);
  EXPECT_EQ(s.plus.right(), row.right());
  EXPECT_EQ(s.minus.width, PiccoTokens::kStepperButtonW);
  EXPECT_EQ(s.plus.y, 210);  // 10 px clearance in an 80 px row
  const Segmented seg = segmented(row, 4, kOrientationCellW);
  EXPECT_EQ(seg.frame.right(), row.right());
  EXPECT_EQ(seg.frame.height, PiccoTokens::kTabBarHeight);
  EXPECT_EQ(seg.cells[3].right() + PiccoTokens::kTabPad + PiccoTokens::kHairline, seg.frame.right());
  EXPECT_EQ(sliderPermilleAt(Box{100, 0, 200, 28}, 50), 0);
  EXPECT_EQ(sliderPermilleAt(Box{100, 0, 200, 28}, 200), 500);
  EXPECT_EQ(sliderPermilleAt(Box{100, 0, 200, 28}, 400), 1000);
}

TEST(PiccoSheetGoTo, Keypad) {
  GoToEntry e{546, 1, 880, false};
  goToDigit(e, 1);  // the first digit replaces the shown page
  EXPECT_EQ(e.value, 1u);
  goToDigit(e, 2);
  goToDigit(e, 0);
  EXPECT_EQ(e.value, 120u);
  goToDigit(e, 9);  // 1209 > 880: ignored, not clamped
  EXPECT_EQ(e.value, 120u);
  goToBackspace(e);
  EXPECT_EQ(e.value, 12u);
  goToClear(e);
  EXPECT_EQ(e.value, 0u);
  EXPECT_EQ(goToTarget(e), 1u);  // a cleared entry still lands in the book
  goToStep(e, 10);
  EXPECT_EQ(e.value, 10u);
  goToStep(e, -100);
  EXPECT_EQ(e.value, 1u);
  goToSetPermille(e, 1000);
  EXPECT_EQ(e.value, 880u);
  EXPECT_EQ(goToPermille(e), 1000);
  EXPECT_EQ(keyAt(3), Key::Backspace);
  EXPECT_EQ(keyAt(7), Key::Clear);
  EXPECT_EQ(keyDigit(11), 0);
  EXPECT_EQ(keyDigit(8), 7);
}

TEST(PiccoSheetFocus, WrapsAndStartsOnTheFirstStop) {
  EXPECT_EQ(moveFocus(-1, 1, 5), 0);
  EXPECT_EQ(moveFocus(-1, -1, 5), 4);
  EXPECT_EQ(moveFocus(4, 1, 5), 0);
  EXPECT_EQ(moveFocus(0, -1, 5), 4);
  EXPECT_EQ(moveFocus(2, 1, 0), -1);
}

TEST(PiccoSheetGestures, Table) {
  EXPECT_EQ(gestureEffect(Place::ReaderEpub, Input::SwipeDown), Effect::OpenSheet);
  EXPECT_EQ(gestureEffect(Place::ReaderEpub, Input::SwipeUp), Effect::GoHome);
  EXPECT_EQ(gestureEffect(Place::ReaderTxtXtc, Input::SwipeUp), Effect::GoHome);
  EXPECT_EQ(gestureEffect(Place::ReaderTxtXtc, Input::TopEdgeDown), Effect::OpenSheet);
  EXPECT_EQ(gestureEffect(Place::Elsewhere, Input::TopEdgeDown), Effect::OpenSheet);
  EXPECT_EQ(gestureEffect(Place::Elsewhere, Input::SwipeUp), Effect::ScreenOwn);
  EXPECT_EQ(gestureEffect(Place::Elsewhere, Input::BottomEdgeUp), Effect::GoHome);
  EXPECT_EQ(gestureEffect(Place::ReaderEpub, Input::UpDown), Effect::EscapeHatch);
  EXPECT_EQ(gestureEffect(Place::ReaderEpub, Input::Up), Effect::TurnPage);
  EXPECT_EQ(gestureEffect(Place::ReaderEpub, Input::Confirm), Effect::OpenSheet);
}
