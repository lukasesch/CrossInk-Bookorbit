#pragma once

#include "PiccoLayout.h"
#include "PiccoPagedList.h"
#include "PiccoSegmented.h"
#include "PiccoTokens.h"

// Pure geometry for the Library, Files and BookOrbit screens (T_Library, T_LibrarySort,
// T_Files, T_BookOrbit boards). Screen-relative boxes; callers pass measured widths.
namespace PiccoLibraryLayout {
using PiccoLayout::Box;

// Gap between the header hairline (or a control band) and the next band.
constexpr int kBandGap = 16;
constexpr int kGridColumns = 3;
constexpr int kGridColumnGap = 12;
constexpr int kGridRowGap = 16;
constexpr int kGridCellHeight = 232;
constexpr int kGridCoverHeight = 182;
constexpr int kGridTitleGap = 6;
constexpr int kOutlineButtonPadLeft = 14;
constexpr int kOutlineButtonPadRight = 12;
constexpr int kOutlineButtonIconGap = 6;
constexpr int kViewToggleCell = 56;

// Segmented control geometry lives in PiccoSegmented; these names keep the screens' calls.
constexpr int segmentedWidth(const int cellCount, const int cellWidth) {
  return PiccoSegmented::frameWidth(cellCount, cellWidth);
}
constexpr Box segmentCell(const Box& outer, const int count, const int index) {
  return PiccoSegmented::cell(outer, count, index);
}
constexpr int segmentAt(const Box& outer, const int count, const int px, const int py) {
  return PiccoSegmented::at(outer, count, px, py);
}

// Gutter-to-gutter band of `height` starting kBandGap below `top`.
constexpr Box bandBelow(const Box& screen, const int top, const int height) {
  return Box{screen.x + PiccoTokens::kGutter, top + kBandGap,
             PiccoLayout::maxOf(0, screen.width - 2 * PiccoTokens::kGutter), height};
}

// Library bands under the header: tabs, sort row, body (down to the list bottom limit).
// headerBottom comes from UITheme::getHeaderRect (panel insets move it).
struct LibraryBands {
  Box tabs;
  Box sort;
  Box body;
};

constexpr LibraryBands libraryBands(const Box& screen, const int headerBottom) {
  LibraryBands out{};
  out.tabs = bandBelow(screen, headerBottom, PiccoTokens::kTabBarHeight);
  out.sort = bandBelow(screen, out.tabs.bottom(), PiccoTokens::kTabBarHeight);
  const int top = out.sort.bottom() + kBandGap;
  out.body = Box{screen.x + PiccoTokens::kGutter, top, PiccoLayout::maxOf(0, screen.width - 2 * PiccoTokens::kGutter),
                 PiccoLayout::maxOf(0, PiccoLayout::listBottomLimit(screen) - top)};
  return out;
}

// Outlined dropdown button: label, gap, trailing chevron; vertically centred in its band.
constexpr Box outlineButton(const int x, const int centerY, const int labelWidth, const int iconWidth) {
  const int width = kOutlineButtonPadLeft + labelWidth + (iconWidth > 0 ? kOutlineButtonIconGap + iconWidth : 0) +
                    kOutlineButtonPadRight;
  return PiccoLayout::centeredBox(x, centerY, width, PiccoTokens::kActionHeight);
}

// ---- Cover grid ---------------------------------------------------------------------------

constexpr int gridRows(const int areaHeight) {
  const int rows = (areaHeight + kGridRowGap) / (kGridCellHeight + kGridRowGap);
  return rows < 1 ? 1 : rows;
}

constexpr int gridPageSize(const int areaHeight) { return kGridColumns * gridRows(areaHeight); }

constexpr int gridCellWidth(const int areaWidth) {
  return PiccoLayout::maxOf(1, (areaWidth - (kGridColumns - 1) * kGridColumnGap) / kGridColumns);
}

constexpr Box gridCell(const Box& area, const int slot) {
  const int width = gridCellWidth(area.width);
  const int col = slot % kGridColumns;
  const int row = slot / kGridColumns;
  return Box{area.x + col * (width + kGridColumnGap), area.y + row * (kGridCellHeight + kGridRowGap), width,
             kGridCellHeight};
}

constexpr Box gridCover(const Box& cell) { return Box{cell.x, cell.y, cell.width, kGridCoverHeight}; }

// First row of the page holding `row`.
constexpr int gridPageStart(const int row, const int pageSize) {
  return pageSize <= 0 || row <= 0 ? 0 : (row / pageSize) * pageSize;
}

// ---- Files transfer pair ------------------------------------------------------------------

// Two equal outlined buttons ending at the list bottom limit; index 0 left, 1 right.
constexpr Box transferButton(const Box& screen, const int index) {
  using namespace PiccoTokens;
  const int bottom = PiccoLayout::listBottomLimit(screen);
  const int width = (screen.width - 2 * kGutter - kGroupGap) / 2;
  const int x = screen.x + kGutter + (index == 0 ? 0 : width + kGroupGap);
  return Box{x, bottom - kActionHeight, width, kActionHeight};
}

// Rows of `rowStep` that fit between top and bottom.
constexpr int rowsBetween(const int top, const int bottom, const int rowStep) {
  if (rowStep <= 0 || bottom <= top) return 0;
  return (bottom - top) / rowStep;
}

// ---- BookOrbit catalog (T_BookOrbit) and book detail (T_BookDetail) -----------------------

constexpr int kCatalogRow = 96;
constexpr int kCatalogCoverW = 56;
constexpr int kCatalogCoverH = 80;
constexpr int kCatalogCoverGap = 14;
constexpr int kCatalogActionW = 92;
constexpr int kCatalogActionGap = 12;

struct CatalogRow {
  Box band;    // hairline on its top edge
  Box cover;   // initial placeholder
  Box text;    // title / author / meta column
  Box action;  // Get button or state, kActionHeight tall
};

constexpr CatalogRow catalogRow(const Box& list, const int slot) {
  using namespace PiccoTokens;
  CatalogRow r{};
  r.band = Box{list.x, list.y + slot * kCatalogRow, list.width, kCatalogRow};
  const int centerY = r.band.y + kCatalogRow / 2;
  r.cover = PiccoLayout::centeredBox(list.x, centerY, kCatalogCoverW, kCatalogCoverH);
  r.action = PiccoLayout::centeredBox(list.right() - kCatalogActionW, centerY, kCatalogActionW, kActionHeight);
  const int textX = r.cover.right() + kCatalogCoverGap;
  r.text = Box{textX, r.band.y, PiccoLayout::maxOf(0, r.action.x - kCatalogActionGap - textX), kCatalogRow};
  return r;
}

constexpr int kDetailCoverW = 128;
constexpr int kDetailCoverH = 192;
constexpr int kDetailCoverGap = 20;

struct BookDetailLayout {
  Box cover;
  Box text;
  Box rule;          // hairline under the cover band
  Box primary;       // the screen's one black button
  Box secondary[2];  // outlined pair under it
};

constexpr BookDetailLayout bookDetail(const Box& screen, const int headerBottom) {
  using namespace PiccoTokens;
  BookDetailLayout d{};
  const int left = screen.x + kGutter;
  const int width = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  d.cover = Box{left, headerBottom + kBandGap, kDetailCoverW, kDetailCoverH};
  const int textX = d.cover.right() + kDetailCoverGap;
  d.text = Box{textX, d.cover.y, PiccoLayout::maxOf(0, left + width - textX), kDetailCoverH};
  d.rule = Box{left, d.cover.bottom() + kBandGap, width, kHairline};
  d.primary = Box{left, d.rule.bottom() + kBandGap, width, kActionHeight};
  const int half = (width - kGroupGap) / 2;
  d.secondary[0] = Box{left, d.primary.bottom() + kGroupGap, half, kActionHeight};
  d.secondary[1] = Box{left + width - half, d.primary.bottom() + kGroupGap, half, kActionHeight};
  return d;
}

// ---- Rocker over controls around a paged list ----------------------------------------------

// Focus index space: [0, leading) controls above the list, then the rows, then `trailing`
// controls below it. One step wraps at both ends; the viewport flips a page only when the
// focus lands on a row outside it (PiccoPagedList::rockerStep) and stays put on controls.
struct FocusCursor {
  int focus = 0;
  int top = 0;
};

constexpr FocusCursor rockerStepAround(const FocusCursor cursor, const int direction, const int leading, const int rows,
                                       const int trailing, const int rowsPerPage) {
  const int total = leading + rows + trailing;
  if (total <= 0) return FocusCursor{};
  int focus = cursor.focus + (direction > 0 ? 1 : -1);
  if (focus >= total) focus = 0;
  if (focus < 0) focus = total - 1;
  int top = cursor.top;
  const int row = focus - leading;
  const int perPage = PiccoLayout::maxOf(1, rowsPerPage);
  if (row >= 0 && row < rows && (row < top || row >= top + perPage)) {
    top = PiccoPagedList::pageTopFor(row, perPage, rows);
  }
  return FocusCursor{focus, top};
}

}  // namespace PiccoLibraryLayout
