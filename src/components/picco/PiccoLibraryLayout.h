#pragma once

#include "PiccoLayout.h"
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

// Outer box of a segmented control (1 px frame, 4 px pad) and its cells.
constexpr int segmentedWidth(const int cellCount, const int cellWidth) {
  using namespace PiccoTokens;
  return 2 * (kHairline + kTabPad) + cellCount * cellWidth + (cellCount - 1) * kTabPad;
}

constexpr Box segmentCell(const Box& outer, const int count, const int index) {
  using namespace PiccoTokens;
  if (count <= 0 || index < 0 || index >= count) return Box{};
  const int inset = kHairline + kTabPad;
  const int inner = outer.width - 2 * inset - (count - 1) * kTabPad;
  const int cell = PiccoLayout::maxOf(0, inner / count);
  // The last cell takes the remainder so the right pad stays 4 px.
  const int x = outer.x + inset + index * (cell + kTabPad);
  const int width = index == count - 1 ? outer.right() - inset - x : cell;
  return Box{x, outer.y + inset, PiccoLayout::maxOf(0, width), PiccoLayout::maxOf(0, outer.height - 2 * inset)};
}

// Index of the cell under (px, py), or -1 (the pads between cells belong to no cell).
constexpr int segmentAt(const Box& outer, const int count, const int px, const int py) {
  if (!outer.contains(px, py)) return -1;
  for (int i = 0; i < count; ++i) {
    const Box cell = segmentCell(outer, count, i);
    // Pads belong to the nearer cell so the whole frame is a target.
    const int left = i == 0 ? outer.x : cell.x - PiccoTokens::kTabPad / 2;
    const int right = i == count - 1 ? outer.right() : cell.right() + PiccoTokens::kTabPad / 2;
    if (px >= left && px < right) return i;
  }
  return -1;
}

// Gutter-to-gutter band of `height` starting kBandGap below `top`.
constexpr Box bandBelow(const Box& screen, const int top, const int height) {
  return Box{screen.x + PiccoTokens::kGutter, top + kBandGap, PiccoLayout::maxOf(0, screen.width - 2 * PiccoTokens::kGutter),
             height};
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

}  // namespace PiccoLibraryLayout
