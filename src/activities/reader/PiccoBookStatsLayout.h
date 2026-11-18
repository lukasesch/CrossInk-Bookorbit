#pragma once

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoTokens.h"

// Pure geometry for the Picco per-book stats page (T_BookStats) and the end-of-book
// screen (T_EndOfBook). Screen-relative; headerBottom comes from the drawn header.
namespace PiccoBookStatsLayout {
using PiccoLayout::Box;

constexpr int kLabelLine = 21;
constexpr int kTitleTop = 19;   // hairline to the title (board: 81 -> 100)
constexpr int kStripTop = 101;  // hairline to the stat strip (title and author above it)
constexpr int kStripHeight = 80;
constexpr int kStripColumns = 3;
constexpr int kStripTextInset = 14;  // text right of a column's divider
constexpr int kProgressGap = 20;
constexpr int kProgressHeight = 74;  // figures, bar, dates
constexpr int kChartHeight = 170;
constexpr int kChartBarMax = 110;
constexpr int kChartColumns = 7;
constexpr int kChartColumnGap = 12;
constexpr int kChartTextGap = 6;
constexpr int kNoteGap = 17;
constexpr int kDayParts = 4;
constexpr int kDayPartGap = 12;
constexpr int kDayPartBar = 14;
constexpr int kDayPartTextGap = 4;

struct Layout {
  Box title;
  Box strip;  // hairline on top and bottom, dividers between columns
  Box progress;
  Box chartLabel;
  Box chart;
  Box note;
  Box dayLabel;
  Box dayParts;
};

constexpr Layout layout(const Box& screen, const int headerBottom) {
  using namespace PiccoTokens;
  Layout out{};
  const int x = screen.x + kGutter;
  const int w = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  out.title = Box{x, headerBottom + kTitleTop, w, kStripTop - kTitleTop};
  out.strip = Box{x, headerBottom + kStripTop, w, kStripHeight};
  out.progress = Box{x, out.strip.bottom() + kProgressGap, w, kProgressHeight};
  const int chartSection = out.progress.bottom();
  out.chartLabel = Box{x, chartSection + kSectionAbove, w, kLabelLine};
  out.chart = Box{x, out.chartLabel.bottom() + kSectionBelow, w, kChartHeight};
  out.note = Box{x, out.chart.bottom() + kNoteGap, w, kLabelLine};
  out.dayLabel = Box{x, out.note.bottom() + kGroupGap, w, kLabelLine};
  out.dayParts = Box{x, out.dayLabel.bottom() + kSectionBelow, w,
                     kLabelLine + kDayPartTextGap + kDayPartBar + kDayPartTextGap + kLabelLine};
  return out;
}

constexpr Box column(const Box& area, const int count, const int gap, const int index) {
  if (count <= 0 || index < 0 || index >= count) return Box{};
  const int width = (area.width - (count - 1) * gap) / count;
  return Box{area.x + index * (width + gap), area.y, width, area.height};
}

constexpr Box stripCell(const Box& strip, const int index) { return column(strip, kStripColumns, 0, index); }
// Text box of a strip cell: columns after the first start right of their divider.
constexpr Box stripText(const Box& strip, const int index) {
  const Box cell = stripCell(strip, index);
  const int inset = index > 0 ? kStripTextInset : 0;
  return Box{cell.x + inset, cell.y, PiccoLayout::maxOf(0, cell.width - inset - PiccoTokens::kNeighbourGap),
             cell.height};
}
constexpr Box chartColumn(const Box& chart, const int index) {
  return column(chart, kChartColumns, kChartColumnGap, index);
}
constexpr Box dayPart(const Box& area, const int index) { return column(area, kDayParts, kDayPartGap, index); }

// Width of the progress fill for a percentage (0..100, clamped).
constexpr int progressFill(const int width, const int percent) {
  const int p = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
  return width * p / 100;
}

// ---- End of book (T_EndOfBook) ------------------------------------------------------------

constexpr int kEobLabelTop = 19;  // hairline to "FINISHED"
constexpr int kEobTitleGap = 6;   // label to title
constexpr int kEobStripGap = 18;  // author to the strip
constexpr int kEobRow = 100;      // a "Continue with" book row
constexpr int kEobCoverW = 56;
constexpr int kEobCoverH = 84;
constexpr int kEobCoverGap = 16;
constexpr int kEobButtonsBottom = 20;  // screen bottom to the button pair

// Button pair at the bottom: two equal outlined buttons, kGroupGap apart.
constexpr Box eobButton(const Box& screen, const int index) {
  using namespace PiccoTokens;
  const Box band{screen.x + kGutter, screen.bottom() - kEobButtonsBottom - kActionHeight,
                 PiccoLayout::maxOf(0, screen.width - 2 * kGutter), kActionHeight};
  return column(band, 2, kGroupGap, index);
}

// A "Continue with" row (hairline on top) and its cover placeholder.
constexpr Box eobRow(const Box& list, const int index) {
  return Box{list.x, list.y + index * kEobRow, list.width, kEobRow};
}
constexpr Box eobCover(const Box& row) {
  return Box{row.x, row.y + (row.height - kEobCoverH) / 2, kEobCoverW, kEobCoverH};
}

}  // namespace PiccoBookStatsLayout
