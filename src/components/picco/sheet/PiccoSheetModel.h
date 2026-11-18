#pragma once

#include <array>
#include <cstdint>

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoSegmented.h"
#include "components/picco/PiccoTokens.h"

// Pure model of the Picco reader sheet (boards T_ReaderSheetDivider, T_ReaderSheetText*,
// T_ReaderSheetMore*, T_GoTo, T_AutoPageTurn, T_BookAdvanced*): panes, their rows, the
// geometry both drawing and hit tests use, focus order, the Go to keypad and the Picco
// gesture table. No renderer, no settings, so the native tests cover it directly.
namespace PiccoSheet {

using PiccoLayout::Box;

// ---- Variants and panes ------------------------------------------------------------------

// Outside: header + light band. Reader: TXT / XTC (title, progress, the tiles they have).
// Epub: the full sheet.
enum class Variant : uint8_t { Outside, Reader, Epub };

enum class Pane : uint8_t { Main, Text, TextMore, More, GoTo, AutoPageTurn, Advanced, FontRendering, Qr, Count };

// Where Back (header chevron, Back button) leads.
constexpr Pane parentPane(const Pane pane) {
  switch (pane) {
    case Pane::Text:
    case Pane::More:
    case Pane::GoTo:
      return Pane::Main;
    case Pane::TextMore:
      return Pane::Text;
    case Pane::AutoPageTurn:
    case Pane::Advanced:
    case Pane::Qr:
      return Pane::More;
    case Pane::FontRendering:
      return Pane::Advanced;
    case Pane::Main:
    case Pane::Count:
      break;
  }
  return Pane::Main;
}

// Panes with a 2-cell tab under the header. Go to has Page only when the book has stable pages.
constexpr uint8_t tabCount(const Pane pane, const bool hasStablePages) {
  switch (pane) {
    case Pane::Text:
    case Pane::More:
    case Pane::Advanced:
      return 2;
    case Pane::GoTo:
      return hasStablePages ? 2 : 1;
    default:
      return 1;
  }
}

// ---- Tiles ---------------------------------------------------------------------------------

enum class Tile : uint8_t { Text, Bookmark, Contents, More };

struct Tiles {
  std::array<Tile, 4> items{};
  uint8_t count = 0;
  constexpr void add(const Tile tile) { items[count++] = tile; }
};

// hasContents: the reader has a table of contents; hasMenu: it has a reader menu (XTC).
constexpr Tiles tilesFor(const Variant variant, const bool hasContents, const bool hasMenu) {
  Tiles tiles;
  switch (variant) {
    case Variant::Epub:
      tiles.add(Tile::Text);
      tiles.add(Tile::Bookmark);
      tiles.add(Tile::Contents);
      tiles.add(Tile::More);
      break;
    case Variant::Reader:
      if (hasContents) tiles.add(Tile::Contents);
      if (hasMenu) tiles.add(Tile::More);
      break;
    case Variant::Outside:
      break;
  }
  return tiles;
}

// ---- Rows ----------------------------------------------------------------------------------

enum class Row : uint8_t {
  // Text: font & layout
  FontCurrent,
  FontOther,
  MoreFonts,
  FontSize,
  LineSpacing,
  SideMargins,
  Alignment,
  Orientation,
  // Text: paragraphs & style
  Hyphenation,
  ForceIndents,
  ExtraSpacing,
  Focus,
  GuideDots,
  DarkMode,
  TextAa,
  // More text options
  WordSpacing,
  CharacterSpacing,
  TopBottomMargins,
  EmbeddedStyle,
  PublisherFonts,
  PublisherPages,
  Images,
  // More: navigate & share
  Footnotes,
  AutoPageTurn,
  Bookmarks,
  Clippings,
  Lookup,
  LookupHistory,
  SaveClipping,
  BookDictionary,
  NearbySync,
  SendBook,
  Qr,
  KOReaderSync,
  // More: this book
  MarkFinished,
  BookStats,
  StatusBar,
  RenderMode,
  Controls,
  Screenshot,
  Advanced,
  // Advanced
  IndexingMethod,
  TrackStats,
  DictionaryFont,
  DictionaryFontSize,
  FontRendering,
  ResetPace,
  DeleteStats,
  DeleteBookmarks,
  DeleteCache,
  ResetBookSettings,
  // Font rendering (scalable fonts)
  TtfHinting,
  TtfRaster,
  TtfInterpreter,
  TtfWeight,
  TtfSlant,
  TtfStemDarkening,
  TtfReset,
};

enum class Section : uint8_t {
  None,
  Font,
  Paragraphs,
  Style,
  Spacing,
  BookStyles,
  Navigate,
  Words,
  SyncShare,
  ThisBook,
  Book,
  Reset,
};

// Heading: section label. Chip: font chip (3 per line). Stepper: - value +. Segmented: icon
// cells. Switch / Value: list row with a switch or a value that opens a picker. Cell: grid
// button (2 per line). Button: list row with a framed action button.
enum class Kind : uint8_t { Heading, Chip, Stepper, Segmented, Switch, Value, Cell, Button };

constexpr Kind kindOf(const Row row) {
  switch (row) {
    case Row::FontCurrent:
    case Row::FontOther:
    case Row::MoreFonts:
      return Kind::Chip;
    case Row::FontSize:
    case Row::LineSpacing:
    case Row::SideMargins:
    case Row::WordSpacing:
    case Row::CharacterSpacing:
    case Row::TopBottomMargins:
      return Kind::Stepper;
    case Row::Alignment:
    case Row::Orientation:
      return Kind::Segmented;
    case Row::Hyphenation:
    case Row::ForceIndents:
    case Row::ExtraSpacing:
    case Row::Focus:
    case Row::GuideDots:
    case Row::DarkMode:
    case Row::TextAa:
    case Row::EmbeddedStyle:
    case Row::PublisherPages:
    case Row::TrackStats:
    case Row::TtfStemDarkening:
      return Kind::Switch;
    case Row::PublisherFonts:
    case Row::Images:
    case Row::IndexingMethod:
    case Row::DictionaryFont:
    case Row::DictionaryFontSize:
    case Row::FontRendering:
    case Row::TtfHinting:
    case Row::TtfRaster:
    case Row::TtfInterpreter:
    case Row::TtfWeight:
    case Row::TtfSlant:
      return Kind::Value;
    case Row::ResetPace:
    case Row::DeleteStats:
    case Row::DeleteBookmarks:
    case Row::DeleteCache:
    case Row::ResetBookSettings:
    case Row::TtfReset:
      return Kind::Button;
    default:
      return Kind::Cell;
  }
}

// Rows with a second, smaller line under the label (80 tall instead of 64).
constexpr bool hasHint(const Row row) {
  switch (row) {
    case Row::Focus:
    case Row::DarkMode:
    case Row::EmbeddedStyle:
    case Row::ResetPace:
    case Row::DeleteStats:
    case Row::DeleteBookmarks:
    case Row::DeleteCache:
    case Row::ResetBookSettings:
      return true;
    default:
      return false;
  }
}

// What the book and build offer; rows for missing features are left out, like the drawer's catalog.
struct Availability {
  bool hasFootnotes = false;
  bool hasDictionary = false;
  bool hasBookmarks = false;
  bool hasClippings = false;
  bool showReadingPaceReset = false;
  bool globalStats = true;
  bool bookStats = true;
  bool koreaderSync = false;
  bool publisherFonts = false;  // CROSSINK_SCALABLE_FONTS
  bool fontRendering = false;   // the current family is scalable
  bool ttfInterpreter = false;  // hinting is on, so the interpreter can be chosen
};

struct Block {
  Kind kind = Kind::Heading;
  Section section = Section::None;
  Row row = Row::FontCurrent;
};

constexpr size_t kMaxBlocks = 24;

struct Blocks {
  std::array<Block, kMaxBlocks> items{};
  uint8_t count = 0;
  constexpr void heading(const Section section) { items[count++] = Block{Kind::Heading, section, Row::FontCurrent}; }
  constexpr void row(const Row row) { items[count++] = Block{kindOf(row), Section::None, row}; }
};

constexpr Blocks paneBlocks(const Pane pane, const uint8_t tab, const Availability& a) {
  Blocks b;
  switch (pane) {
    case Pane::Text:
      if (tab == 0) {
        b.heading(Section::Font);
        b.row(Row::FontCurrent);
        b.row(Row::FontOther);
        b.row(Row::MoreFonts);
        b.row(Row::FontSize);
        b.row(Row::LineSpacing);
        b.row(Row::SideMargins);
        b.row(Row::Alignment);
        b.row(Row::Orientation);
      } else {
        b.heading(Section::Paragraphs);
        b.row(Row::Hyphenation);
        b.row(Row::ForceIndents);
        b.row(Row::ExtraSpacing);
        b.heading(Section::Style);
        b.row(Row::Focus);
        b.row(Row::GuideDots);
        b.row(Row::DarkMode);
        b.row(Row::TextAa);
      }
      break;
    case Pane::TextMore:
      b.heading(Section::Spacing);
      b.row(Row::WordSpacing);
      b.row(Row::CharacterSpacing);
      b.row(Row::TopBottomMargins);
      b.heading(Section::BookStyles);
      b.row(Row::EmbeddedStyle);
      if (a.publisherFonts) b.row(Row::PublisherFonts);
      b.row(Row::PublisherPages);
      b.row(Row::Images);
      break;
    case Pane::More:
      if (tab == 0) {
        b.heading(Section::Navigate);
        if (a.hasFootnotes) b.row(Row::Footnotes);
        b.row(Row::AutoPageTurn);
        if (a.hasBookmarks) b.row(Row::Bookmarks);
        if (a.hasClippings) b.row(Row::Clippings);
        b.heading(Section::Words);
        if (a.hasDictionary) {
          b.row(Row::Lookup);
          b.row(Row::LookupHistory);
        }
        b.row(Row::SaveClipping);
        b.row(Row::BookDictionary);
        b.heading(Section::SyncShare);
        b.row(Row::NearbySync);
        b.row(Row::SendBook);
        b.row(Row::Qr);
        if (a.koreaderSync) b.row(Row::KOReaderSync);
      } else {
        b.heading(Section::ThisBook);
        b.row(Row::MarkFinished);
        if (a.globalStats && a.bookStats) b.row(Row::BookStats);
        b.row(Row::StatusBar);
        b.row(Row::RenderMode);
        b.row(Row::Controls);
        b.row(Row::Screenshot);
        b.row(Row::Advanced);
      }
      break;
    case Pane::Advanced:
      if (tab == 0) {
        b.heading(Section::Book);
        b.row(Row::IndexingMethod);
        if (a.globalStats) b.row(Row::TrackStats);
        b.row(Row::DictionaryFont);
        b.row(Row::DictionaryFontSize);
        if (a.fontRendering) b.row(Row::FontRendering);
      } else {
        b.heading(Section::Reset);
        if (a.showReadingPaceReset) b.row(Row::ResetPace);
        if (a.globalStats && a.bookStats) b.row(Row::DeleteStats);
        if (a.hasBookmarks) b.row(Row::DeleteBookmarks);
        b.row(Row::DeleteCache);
        b.row(Row::ResetBookSettings);
      }
      break;
    case Pane::FontRendering:
      b.row(Row::TtfHinting);
      b.row(Row::TtfRaster);
      if (a.ttfInterpreter) b.row(Row::TtfInterpreter);
      b.row(Row::TtfWeight);
      b.row(Row::TtfSlant);
      b.row(Row::TtfStemDarkening);
      b.row(Row::TtfReset);
      break;
    case Pane::Main:
    case Pane::GoTo:
    case Pane::AutoPageTurn:
    case Pane::Qr:
    case Pane::Count:
      break;
  }
  return b;
}

// ---- Geometry ------------------------------------------------------------------------------

constexpr int kSmallLine = 21;  // 17 px type
constexpr int kBodyLine = 25;   // 21 px type
constexpr int kTitleLine = 35;  // 32 px Literata
constexpr int kSheetFoot = 28;  // last content -> sheet bottom (handle band)
constexpr int kMainFoot = 48;   // the main sheet keeps more room under its tiles
constexpr int kLightRow = 62;
constexpr int kLightBandPad = 6;
constexpr int kTileHeight = 84;
constexpr int kTileGap = 8;
constexpr int kCellHeight = 60;
constexpr int kCellGap = 8;
constexpr int kChipHeight = 60;
constexpr int kChipsPerLine = 3;
constexpr int kCellsPerLine = 2;
constexpr int kTabTop = 12;
constexpr int kStepperValueW = 64;
constexpr int kOrientationCellW = 60;
constexpr int kAlignmentCellW = 52;
constexpr int kButtonW = 104;
constexpr int kLightIcon = 28;
constexpr int kLightGap = 16;
constexpr int kLightValueW = 46;
constexpr int kGoToKeyRows = 3;
constexpr int kGoToKeyCols = 4;

constexpr int contentWidth(const int screenWidth) {
  return PiccoLayout::maxOf(0, screenWidth - 2 * PiccoTokens::kGutter);
}

constexpr Box headerBox(const int screenWidth) { return Box{0, 0, screenWidth, PiccoTokens::kHeaderHeight}; }

// The 2-cell tab under a pane header.
struct TabBar {
  Box bar;
  std::array<Box, 2> cells;
};

constexpr TabBar tabBar(const int screenWidth, const int top) {
  using namespace PiccoTokens;
  TabBar t{};
  t.bar = Box{kGutter, top, contentWidth(screenWidth), kTabBarHeight};
  for (int i = 0; i < 2; ++i) t.cells[static_cast<size_t>(i)] = PiccoSegmented::cell(t.bar, 2, i);
  return t;
}

// Light band: hairline, 6 px, one 62 px row per light control, 6 px, hairline.
struct LightRow {
  Box row;
  Box icon;
  Box track;  // slider span; the knob centre runs along it
  Box value;
};

constexpr LightRow lightRow(const int screenWidth, const int top) {
  using namespace PiccoTokens;
  LightRow r{};
  r.row = Box{kGutter, top, contentWidth(screenWidth), kLightRow};
  const int cy = top + kLightRow / 2;
  r.icon = Box{kGutter, cy - kLightIcon / 2, kLightIcon, kLightIcon};
  r.value = Box{r.row.right() - kLightValueW, cy - kSliderKnob / 2, kLightValueW, kSliderKnob};
  const int trackX = r.icon.right() + kLightGap;
  r.track = Box{trackX, cy - kSliderKnob / 2, PiccoLayout::maxOf(0, r.value.x - kLightGap - trackX), kSliderKnob};
  return r;
}

constexpr int lightBandHeight(const bool warmth) {
  return 2 * PiccoTokens::kHairline + 2 * kLightBandPad + kLightRow * (warmth ? 2 : 1);
}

// Slider value for a touch at x along a track (permille, clamped).
constexpr int sliderPermilleAt(const Box& track, const int x) {
  if (track.width <= 0) return 0;
  const int offset = x - track.x;
  if (offset <= 0) return 0;
  if (offset >= track.width) return 1000;
  return static_cast<int>((static_cast<int64_t>(offset) * 1000 + track.width / 2) / track.width);
}

constexpr int sliderKnobCenterX(const Box& track, const int permille) {
  return track.x + PiccoLayout::progressFillWidth(track.width, permille);
}

struct MainLayout {
  Box header;
  Box label;  // "NOW READING"
  Box title;
  Box author;
  Box goTo;      // the whole progress block (tap -> Go to)
  Box goToLine;  // chapter / page left, percent right
  Box progress;  // 7 px bar
  Box goToFoot;  // "Go to ›" right
  Box band;      // hairline to hairline
  std::array<LightRow, 2> light;
  uint8_t lightRows = 0;
  std::array<Box, 4> tiles;
  uint8_t tileCount = 0;
  int bottom = 0;  // sheet bottom edge
};

// hasGoTo: the reader offers Go to (EPUB), so the progress block ends in a "Go to ›" line.
constexpr MainLayout mainLayout(const int screenWidth, const bool book, const bool hasAuthor, const bool warmth,
                                const int tileCount, const bool hasGoTo = true) {
  using namespace PiccoTokens;
  MainLayout m{};
  const int w = contentWidth(screenWidth);
  m.header = headerBox(screenWidth);
  int y = kHeaderHeight;
  if (book) {
    y += 16;
    m.label = Box{kGutter, y, w, kSmallLine};
    y = m.label.bottom() + 4;
    m.title = Box{kGutter, y, w, kTitleLine};
    y = m.title.bottom() + 2;
    if (hasAuthor) {
      m.author = Box{kGutter, y, w, kBodyLine};
      y = m.author.bottom();
    }
    y += 16;
    const int goToTop = y;
    m.goToLine = Box{kGutter, y, w, kSmallLine};
    y = m.goToLine.bottom() + 10;
    m.progress = Box{kGutter, y, w, kProgressFill};
    y = m.progress.bottom();
    if (hasGoTo) {
      y += 10;
      m.goToFoot = Box{kGutter, y, w, kSmallLine};
      y = m.goToFoot.bottom();
    }
    y += 18;
    m.goTo = Box{kGutter, goToTop, w, y - goToTop};
  } else {
    y += 16;
  }
  m.lightRows = warmth ? 2 : 1;
  m.band = Box{kGutter, y, w, lightBandHeight(warmth)};
  int rowTop = y + kHairline + kLightBandPad;
  for (int i = 0; i < m.lightRows; ++i) {
    m.light[static_cast<size_t>(i)] = lightRow(screenWidth, rowTop);
    rowTop += kLightRow;
  }
  y = m.band.bottom();
  const int count = tileCount < 0 ? 0 : (tileCount > 4 ? 4 : tileCount);
  m.tileCount = static_cast<uint8_t>(count);
  if (count > 0) {
    y += 16;
    const int tileW = (w - (count - 1) * kTileGap) / count;
    for (int i = 0; i < count; ++i) {
      const int x = kGutter + i * (tileW + kTileGap);
      const int width = i == count - 1 ? kGutter + w - x : tileW;
      m.tiles[static_cast<size_t>(i)] = Box{x, y, width, kTileHeight};
    }
    y += kTileHeight;
  }
  m.bottom = y + kMainFoot;
  return m;
}

// Height of a block's own box (headings include the space above and below their label).
constexpr int blockHeight(const Block& block, const bool firstHeading) {
  using namespace PiccoTokens;
  switch (block.kind) {
    case Kind::Heading:
      return (firstHeading ? kSectionFirstAbove : kSectionAbove) + kSmallLine + kSectionBelow;
    case Kind::Chip:
      return kChipHeight;
    case Kind::Cell:
      return kCellHeight;
    case Kind::Stepper:
    case Kind::Segmented:
    case Kind::Button:
      return kRowWithControl;
    case Kind::Switch:
    case Kind::Value:
      return hasHint(block.row) ? kRowWithHint : kRowOneLine;
  }
  return kRowOneLine;
}

struct BlockLayout {
  std::array<Box, kMaxBlocks> boxes{};
  // A hairline sits on the top edge of a list row that follows another list row.
  std::array<bool, kMaxBlocks> hairlineAbove{};
  int bottom = 0;  // last content edge
};

constexpr bool isListRow(const Kind kind) {
  return kind == Kind::Stepper || kind == Kind::Switch || kind == Kind::Value || kind == Kind::Button ||
         kind == Kind::Segmented;
}

// Lays blocks out from top; chips go three and cells two to a line. rowHairlines is false
// for the Text pane's control rows, which the board draws without separators.
constexpr BlockLayout layoutBlocks(const Blocks& blocks, const int screenWidth, const int top,
                                   const bool rowHairlines) {
  using namespace PiccoTokens;
  BlockLayout out{};
  const int w = contentWidth(screenWidth);
  int y = top;
  bool firstHeading = true;
  int lineIndex = 0;  // position within a run of chips / cells
  Kind runKind = Kind::Heading;
  int runTop = 0;
  for (int i = 0; i < blocks.count; ++i) {
    const Block& block = blocks.items[static_cast<size_t>(i)];
    const bool grid = block.kind == Kind::Chip || block.kind == Kind::Cell;
    if (!grid || runKind != block.kind) lineIndex = 0;
    if (grid) {
      const int perLine = block.kind == Kind::Chip ? kChipsPerLine : kCellsPerLine;
      const int gap = block.kind == Kind::Chip ? kNeighbourGap : kCellGap;
      const int col = lineIndex % perLine;
      if (col == 0) {
        runTop = lineIndex == 0 ? y : y + gap;
        y = runTop + blockHeight(block, false);
      }
      const int cellW = (w - (perLine - 1) * gap) / perLine;
      const int x = kGutter + col * (cellW + gap);
      const int width = col == perLine - 1 ? kGutter + w - x : cellW;
      out.boxes[static_cast<size_t>(i)] = Box{x, runTop, width, blockHeight(block, false)};
      ++lineIndex;
      runKind = block.kind;
      continue;
    }
    runKind = block.kind;
    const int h = blockHeight(block, firstHeading);
    if (block.kind == Kind::Heading) {
      const int above = firstHeading ? kSectionFirstAbove : kSectionAbove;
      out.boxes[static_cast<size_t>(i)] = Box{kGutter, y + above, w, kSmallLine};
      firstHeading = false;
    } else {
      out.boxes[static_cast<size_t>(i)] = Box{kGutter, y, w, h};
      const Kind previous = i > 0 ? blocks.items[static_cast<size_t>(i - 1)].kind : Kind::Heading;
      out.hairlineAbove[static_cast<size_t>(i)] = rowHairlines && isListRow(previous) && isListRow(block.kind);
    }
    y += h;
  }
  out.bottom = y;
  return out;
}

// Top of a pane's content: the header, then the tab bar when the pane has one.
constexpr int paneContentTop(const int tabs) {
  return PiccoTokens::kHeaderHeight + (tabs > 1 ? kTabTop + PiccoTokens::kTabBarHeight : 0);
}

// Sheet bottom of a block pane: tabbed panes keep the taller tab's height so switching
// tabs never uncovers the page.
constexpr int blockPaneBottom(const Pane pane, const Availability& a, const int screenWidth) {
  const int tabs = tabCount(pane, false);
  const int top = paneContentTop(tabs);
  int bottom = 0;
  for (int tab = 0; tab < tabs; ++tab) {
    const BlockLayout layout =
        layoutBlocks(paneBlocks(pane, static_cast<uint8_t>(tab), a), screenWidth, top, pane != Pane::Text || tab != 0);
    bottom = PiccoLayout::maxOf(bottom, layout.bottom);
  }
  return bottom + kSheetFoot;
}

// Stepper: [-] value [+], right-aligned in a control row.
struct Stepper {
  Box minus;
  Box value;
  Box plus;
};

constexpr Stepper stepper(const Box& row) {
  using namespace PiccoTokens;
  Stepper s{};
  const int cy = row.y + row.height / 2;
  s.plus = Box{row.right() - kStepperButtonW, cy - kStepperButtonH / 2, kStepperButtonW, kStepperButtonH};
  s.value = Box{s.plus.x - kNeighbourGap - kStepperValueW, s.plus.y, kStepperValueW, kStepperButtonH};
  s.minus = Box{s.value.x - kNeighbourGap - kStepperButtonW, s.plus.y, kStepperButtonW, kStepperButtonH};
  return s;
}

// Segmented control: outer frame and n cells, right-aligned in a control row.
struct Segmented {
  Box frame;
  std::array<Box, 5> cells;
  uint8_t count = 0;
};

constexpr Segmented segmented(const Box& row, const int count, const int cellWidth) {
  Segmented s{};
  const int n = count < 0 ? 0 : (count > 5 ? 5 : count);
  s.count = static_cast<uint8_t>(n);
  s.frame = PiccoSegmented::rightAligned(row, n, cellWidth);
  for (int i = 0; i < n; ++i) s.cells[static_cast<size_t>(i)] = PiccoSegmented::cell(s.frame, n, i);
  return s;
}

// Framed action button (Reset / Delete) or switch, right-aligned and centred in a row.
constexpr Box rowButton(const Box& row) {
  return Box{row.right() - kButtonW, row.y + (row.height - PiccoTokens::kActionHeight) / 2, kButtonW,
             PiccoTokens::kActionHeight};
}

constexpr Box rowSwitch(const Box& row) {
  using namespace PiccoTokens;
  return Box{row.right() - kSwitchW, row.y + (row.height - kSwitchH) / 2, kSwitchW, kSwitchH};
}

// Go to (board T_GoTo).
struct GoToLayout {
  TabBar tabs;
  bool hasTabs = false;
  Box number;
  Box detail;
  Box slider;
  std::array<Box, 4> steps;  // -10, -1, +1, +10
  int rule = 0;              // hairline above the keypad
  std::array<Box, kGoToKeyRows * kGoToKeyCols> keys;
  Box pill;
  int bottom = 0;
};

// Keypad cells, row-major: 1 2 3 ⌫ / 4 5 6 Clear / 7 8 9 0.
enum class Key : uint8_t { Digit, Backspace, Clear };
constexpr Key keyAt(const int index) { return index == 3 ? Key::Backspace : index == 7 ? Key::Clear : Key::Digit; }
constexpr uint8_t keyDigit(const int index) {
  constexpr uint8_t digits[kGoToKeyRows * kGoToKeyCols] = {1, 2, 3, 0, 4, 5, 6, 0, 7, 8, 9, 0};
  return index >= 0 && index < kGoToKeyRows * kGoToKeyCols ? digits[index] : 0;
}

constexpr GoToLayout goToLayout(const int screenWidth, const bool hasTabs) {
  using namespace PiccoTokens;
  GoToLayout g{};
  const int w = contentWidth(screenWidth);
  int y = kHeaderHeight + 16;
  g.hasTabs = hasTabs;
  if (hasTabs) {
    g.tabs = tabBar(screenWidth, y);
    y += kTabBarHeight + 16;
  }
  g.number = Box{kGutter, y, w, 56};
  y = g.number.bottom() + 6;
  g.detail = Box{kGutter, y, w, kSmallLine};
  y = g.detail.bottom() + 16;
  g.slider = Box{kGutter, y, w, kSliderKnob};
  y = g.slider.bottom() + 16;
  const int stepW = (w - 3 * kNeighbourGap) / 4;
  for (int i = 0; i < 4; ++i) {
    const int x = kGutter + i * (stepW + kNeighbourGap);
    g.steps[static_cast<size_t>(i)] = Box{x, y, i == 3 ? kGutter + w - x : stepW, kActionHeight};
  }
  y += kActionHeight + 16;
  g.rule = y;
  y += kHairline + 16;
  for (int r = 0; r < kGoToKeyRows; ++r) {
    for (int c = 0; c < kGoToKeyCols; ++c) {
      const int x = kGutter + c * (stepW + kNeighbourGap);
      g.keys[static_cast<size_t>(r * kGoToKeyCols + c)] =
          Box{x, y + r * (kActionHeight + kNeighbourGap), c == 3 ? kGutter + w - x : stepW, kActionHeight};
    }
  }
  y += kGoToKeyRows * kActionHeight + (kGoToKeyRows - 1) * kNeighbourGap + 16;
  g.pill = Box{kGutter, y, w, kPillHeight};
  g.bottom = g.pill.bottom() + 32;
  return g;
}

// Auto page turn (board T_AutoPageTurn).
struct AutoTurnLayout {
  Box row;
  Box hint;
  Box pill;
  int bottom = 0;
};

constexpr AutoTurnLayout autoTurnLayout(const int screenWidth) {
  using namespace PiccoTokens;
  AutoTurnLayout a{};
  const int w = contentWidth(screenWidth);
  a.row = Box{kGutter, kHeaderHeight + 16, w, kRowWithControl};
  a.hint = Box{kGutter, a.row.bottom() + 16, w, 24};
  a.pill = Box{kGutter, a.hint.bottom() + 18, w, kPillHeight};
  a.bottom = a.pill.bottom() + kSheetFoot;
  return a;
}

// Page as QR (board T_QrPage): framed code under the header, two caption lines.
constexpr int kQrFrame = 332;
constexpr int kQrPad = 20;
constexpr int kQrTop = 24;
constexpr int kQrCaptionGap = 16;
constexpr int kQrCaptionLine = 26;
constexpr int kQrDetailGap = 4;
constexpr int kQrDetailLine = 21;
constexpr int kQrFoot = 57;

struct QrLayout {
  Box frame;
  Box code;
  Box caption;
  Box detail;
  int bottom = 0;
};

constexpr QrLayout qrLayout(const int screenWidth) {
  using namespace PiccoTokens;
  QrLayout q{};
  const int w = contentWidth(screenWidth);
  const int side = PiccoLayout::minOf(kQrFrame, w);
  q.frame = Box{(screenWidth - side) / 2, kHeaderHeight + kQrTop, side, side};
  q.code = Box{q.frame.x + kQrPad, q.frame.y + kQrPad, side - 2 * kQrPad, side - 2 * kQrPad};
  q.caption = Box{kGutter, q.frame.bottom() + kQrCaptionGap, w, kQrCaptionLine};
  q.detail = Box{kGutter, q.caption.bottom() + kQrDetailGap, w, kQrDetailLine};
  q.bottom = q.detail.bottom() + kQrFoot;
  return q;
}

// ---- Go to entry ---------------------------------------------------------------------------

// The number Go to jumps to: typed on the keypad, stepped, or dragged. Typing starts over
// on the first digit after a step or a drag.
struct GoToEntry {
  uint32_t value = 0;
  uint32_t minimum = 0;
  uint32_t maximum = 100;
  bool typing = false;
};

constexpr uint32_t clampEntry(const GoToEntry& e, const int64_t value) {
  if (value < static_cast<int64_t>(e.minimum)) return e.minimum;
  if (value > static_cast<int64_t>(e.maximum)) return e.maximum;
  return static_cast<uint32_t>(value);
}

constexpr void goToDigit(GoToEntry& e, const uint8_t digit) {
  const uint64_t next = e.typing ? static_cast<uint64_t>(e.value) * 10 + digit : digit;
  e.typing = true;
  // Ignore a digit that would leave the range rather than silently clamping it.
  if (next > e.maximum) return;
  e.value = static_cast<uint32_t>(next);
}

constexpr void goToBackspace(GoToEntry& e) {
  e.typing = true;
  e.value /= 10;
}

constexpr void goToClear(GoToEntry& e) {
  e.typing = true;
  e.value = 0;
}

constexpr void goToStep(GoToEntry& e, const int delta) {
  e.typing = false;
  e.value = clampEntry(e, static_cast<int64_t>(e.value) + delta);
}

constexpr void goToSetPermille(GoToEntry& e, const int permille) {
  e.typing = false;
  const int p = permille < 0 ? 0 : (permille > 1000 ? 1000 : permille);
  const int64_t span = static_cast<int64_t>(e.maximum) - e.minimum;
  e.value = clampEntry(e, e.minimum + (span * p + 500) / 1000);
}

constexpr int goToPermille(const GoToEntry& e) {
  if (e.maximum <= e.minimum) return 0;
  const int64_t offset = static_cast<int64_t>(clampEntry(e, e.value)) - e.minimum;
  return static_cast<int>(offset * 1000 / (static_cast<int64_t>(e.maximum) - e.minimum));
}

// The value a confirmed jump uses (a cleared entry still lands inside the book).
constexpr uint32_t goToTarget(const GoToEntry& e) { return clampEntry(e, e.value); }

// ---- Focus ---------------------------------------------------------------------------------

// Rocker focus over a pane's stops, wrapping. -1 (no ring yet) goes to the first stop.
constexpr int moveFocus(const int focus, const int delta, const int count) {
  if (count <= 0) return -1;
  if (focus < 0 || focus >= count) return delta < 0 ? count - 1 : 0;
  return ((focus + delta) % count + count) % count;
}

// ---- Gestures ------------------------------------------------------------------------------

enum class Place : uint8_t { ReaderEpub, ReaderTxtXtc, Elsewhere };

enum class Input : uint8_t {
  SwipeDown,
  SwipeUp,
  TopEdgeDown,
  BottomEdgeUp,
  LongPress,
  Up,
  Down,
  Power,
  UpDown,
  Back,     // button-only devices
  Confirm,  // button-only devices
};

enum class Effect : uint8_t {
  OpenSheet,
  GoHome,
  TurnPage,
  ScreenOwn,     // the screen's own use: word selection, list paging, scrolling
  MoveFocus,     // rocker focus on screens with lists
  PowerSetting,  // the configured power-button action (sleep, page turn, ...)
  EscapeHatch,   // Up + Down: touch back on, else its mapped action (reader menu = sheet)
  Back,
  Activate,
};

// The Picco gesture table (docs/picco.md). Readers have no reader-menu swipe any more:
// the sheet opens downwards, Home is upwards.
constexpr Effect gestureEffect(const Place place, const Input input) {
  const bool reader = place != Place::Elsewhere;
  switch (input) {
    case Input::SwipeDown:
      return reader ? Effect::OpenSheet : Effect::ScreenOwn;
    case Input::TopEdgeDown:
      return Effect::OpenSheet;
    case Input::SwipeUp:
      return reader ? Effect::GoHome : Effect::ScreenOwn;
    case Input::BottomEdgeUp:
      return Effect::GoHome;
    case Input::LongPress:
      return Effect::ScreenOwn;
    case Input::Up:
    case Input::Down:
      return reader ? Effect::TurnPage : Effect::MoveFocus;
    case Input::Power:
      return Effect::PowerSetting;
    case Input::UpDown:
      return Effect::EscapeHatch;
    case Input::Back:
      return Effect::Back;
    case Input::Confirm:
      return reader ? Effect::OpenSheet : Effect::Activate;
  }
  return Effect::ScreenOwn;
}

}  // namespace PiccoSheet
