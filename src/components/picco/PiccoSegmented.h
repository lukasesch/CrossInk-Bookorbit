#pragma once

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// The one Picco segmented control (tab bars, view toggle, sort order, sheet tabs and icon
// rows): 1 px frame r16, 4 px pad, equal cells r12; the selected cell is a 2 px stroke.
// Pure geometry shared by every draw and hit test; PiccoListUi draws it with a renderer,
// drawUiSegmentedTabBar with FreeInkUI.
namespace PiccoSegmented {
using PiccoLayout::Box;

constexpr int kInset = PiccoTokens::kHairline + PiccoTokens::kTabPad;

// Frame width for `count` cells of `cellWidth`.
constexpr int frameWidth(const int count, const int cellWidth) {
  return count <= 0 ? 2 * kInset : 2 * kInset + count * cellWidth + (count - 1) * PiccoTokens::kTabPad;
}

// Cell `index` of `count` equal cells; the last takes the remainder so the right pad stays 4 px.
constexpr Box cell(const Box& frame, const int count, const int index) {
  using PiccoTokens::kTabPad;
  if (count <= 0 || index < 0 || index >= count) return Box{};
  const int inner = frame.width - 2 * kInset - (count - 1) * kTabPad;
  const int width = PiccoLayout::maxOf(0, inner / count);
  const int x = frame.x + kInset + index * (width + kTabPad);
  const int w = index == count - 1 ? frame.right() - kInset - x : width;
  return Box{x, frame.y + kInset, PiccoLayout::maxOf(0, w), PiccoLayout::maxOf(0, frame.height - 2 * kInset)};
}

// Cell under (px, py), or -1 outside the frame. Pads belong to the nearer cell so the whole
// frame is a target.
constexpr int at(const Box& frame, const int count, const int px, const int py) {
  if (!frame.contains(px, py)) return -1;
  for (int i = 0; i < count; ++i) {
    const Box c = cell(frame, count, i);
    const int left = i == 0 ? frame.x : c.x - PiccoTokens::kTabPad / 2;
    const int right = i == count - 1 ? frame.right() : c.right() + PiccoTokens::kTabPad / 2;
    if (px >= left && px < right) return i;
  }
  return -1;
}

// Tab-bar-high frame of `count` fixed cells, right-aligned and vertically centred in a row.
constexpr Box rightAligned(const Box& row, const int count, const int cellWidth) {
  const int width = frameWidth(count, cellWidth);
  return Box{row.right() - width, row.y + (row.height - PiccoTokens::kTabBarHeight) / 2, width,
             PiccoTokens::kTabBarHeight};
}

}  // namespace PiccoSegmented
