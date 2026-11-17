#pragma once

#include <array>
#include <cstdint>

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// Pure geometry for the sign-in form and keyboard (T_Login), the reading stats screen
// (T_Stats) and the transfer status screens. Every top is relative to the header's
// bottom edge, so the screens follow UITheme::getHeaderRect wherever the panel puts it.
namespace PiccoKeyboardLayout {

using PiccoLayout::Box;

constexpr int kPadTop = 10;
constexpr int kPadSide = 6;
constexpr int kPadBottom = 12;
constexpr int kRowGap = 6;
constexpr int kFormTop = 16;   // header bottom -> lead paragraph
constexpr int kLeadLine = 24;  // 17 px text on a 24 px line
constexpr int kFieldGap = 10;  // above every field label
constexpr int kLabelGap = 4;   // label -> field frame
constexpr int kMaxFields = 4;

// Keyboard band from its hairline to the screen bottom: padding, keys 56 tall, 6 between rows.
constexpr int bandHeight(const int rows) {
  return rows <= 0 ? 0 : kPadTop + rows * PiccoTokens::kKeyH + (rows - 1) * kRowGap + kPadBottom;
}

constexpr Box band(const Box& screen, const int rows) {
  const int h = bandHeight(rows);
  return Box{screen.x, screen.bottom() - h, screen.width, h};
}

// The keys' rect inside the band (the keyboard component insets it by its own padding).
constexpr Box keysRect(const Box& band) {
  return Box{band.x + kPadSide, band.y + kPadTop, PiccoLayout::maxOf(0, band.width - 2 * kPadSide),
             PiccoLayout::maxOf(0, band.height - kPadTop - kPadBottom)};
}

struct FormInputs {
  int headerBottom = 0;
  int leadLines = 0;    // 0 = no lead paragraph
  int labelHeight = 0;  // 0 = fields without labels
  int fieldCount = 1;
  int editing = 0;         // the field that grows with wrapped lines
  int editingExtra = 0;    // extra height of the editing field (wrapped lines)
  int keyboardTop = 0;     // 0 = no keyboard below the form
  int trailingHeight = 0;  // a control under the last field (Sign in), 0 = none
  int trailingGap = 16;
};

struct FormLayout {
  Box lead;
  std::array<Box, kMaxFields> labels{};
  std::array<Box, kMaxFields> fields{};
  Box trailing;
  int fieldCount = 0;
  bool leadShown = false;
  bool fits = true;  // last element clears the keyboard by kControlClearance
};

constexpr FormLayout formLayoutWith(const Box& screen, const FormInputs& in, const bool withLead) {
  using namespace PiccoTokens;
  FormLayout out{};
  const int x = screen.x + kGutter;
  const int w = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  int y = in.headerBottom + kFormTop;
  if (withLead && in.leadLines > 0) {
    out.lead = Box{x, y, w, in.leadLines * kLeadLine};
    out.leadShown = true;
    y = out.lead.bottom();
  } else {
    // Without a paragraph the first field sits where a list starts under the header.
    y = in.headerBottom + kSectionFirstAbove - kFieldGap;
  }
  const int count = PiccoLayout::minOf(PiccoLayout::maxOf(in.fieldCount, 0), kMaxFields);
  out.fieldCount = count;
  for (int i = 0; i < count; ++i) {
    y += kFieldGap;
    if (in.labelHeight > 0) {
      out.labels[i] = Box{x, y, w, in.labelHeight};
      y += in.labelHeight + kLabelGap;
    }
    const int h = kTextFieldHeight + (i == in.editing ? PiccoLayout::maxOf(0, in.editingExtra) : 0);
    out.fields[i] = Box{x, y, w, h};
    y += h;
  }
  if (in.trailingHeight > 0) {
    out.trailing = Box{x, y + in.trailingGap, w, in.trailingHeight};
    y = out.trailing.bottom();
  }
  out.fits = in.keyboardTop <= 0 || y + kControlClearance <= in.keyboardTop;
  return out;
}

// The lead paragraph gives way first when the editing field grows into the keyboard.
constexpr FormLayout formLayout(const Box& screen, const FormInputs& in) {
  const FormLayout full = formLayoutWith(screen, in, true);
  if (full.fits || in.leadLines <= 0) return full;
  return formLayoutWith(screen, in, false);
}

// Text inside a field: padded 16 px, single line centred in the 60 px frame, wrapped
// lines below it (the frame grows by one line height per extra line).
constexpr Box fieldText(const Box& field, const int lineHeight, const int trailingReserve) {
  using namespace PiccoTokens;
  const int top = field.y + (kTextFieldHeight - lineHeight) / 2;
  return Box{field.x + kTextFieldPadX, top, PiccoLayout::maxOf(0, field.width - 2 * kTextFieldPadX - trailingReserve),
             field.height - (top - field.y) * 2};
}

constexpr int extraHeightForLines(const int lines, const int lineHeight) {
  return lines > 1 ? (lines - 1) * lineHeight : 0;
}

}  // namespace PiccoKeyboardLayout

namespace PiccoStatsLayout {

using PiccoLayout::Box;

constexpr int kTabsTop = 16;
constexpr int kStreakGap = 16;
constexpr int kStreakHeight = 84;
constexpr int kLabelLine = 21;
constexpr int kWeekChartHeight = 130;
constexpr int kWeekColumns = 7;
constexpr int kWeekColumnGap = 12;
constexpr int kWeekBarMax = 76;
constexpr int kChartTextGap = 4;
constexpr int kTotalsCell = 64;
constexpr int kTotalsColumns = 3;
constexpr int kTotalsRows = 2;
constexpr int kDayParts = 4;
constexpr int kDayPartGap = 12;
constexpr int kDayPartBar = 14;

struct Layout {
  Box tabs;  // empty without the device tabs
  Box streak;
  Box weekLabel;
  Box weekChart;
  Box totalsLabel;
  Box totalsGrid;  // hairline on top, a hairline under each row
  Box dayLabel;
  Box dayParts;
};

// A section: kSectionAbove, a 21 px label, kSectionBelow, then its body.
constexpr int sectionBodyTop(const int sectionTop) {
  return sectionTop + PiccoTokens::kSectionAbove + kLabelLine + PiccoTokens::kSectionBelow;
}

constexpr Layout layout(const Box& screen, const int headerBottom, const bool tabs) {
  using namespace PiccoTokens;
  Layout out{};
  const int x = screen.x + kGutter;
  const int w = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  int y = headerBottom + kTabsTop;
  if (tabs) {
    out.tabs = Box{x, y, w, kTabBarHeight};
    y = out.tabs.bottom() + kStreakGap;
  }
  out.streak = Box{x, y, w, kStreakHeight};
  y = out.streak.bottom();

  out.weekLabel = Box{x, y + kSectionAbove, w, kLabelLine};
  out.weekChart = Box{x, sectionBodyTop(y), w, kWeekChartHeight};
  y = out.weekChart.bottom();

  out.totalsLabel = Box{x, y + kSectionAbove, w, kLabelLine};
  out.totalsGrid = Box{x, sectionBodyTop(y), w, kTotalsRows * kTotalsCell};
  y = out.totalsGrid.bottom();

  out.dayLabel = Box{x, y + kSectionAbove, w, kLabelLine};
  out.dayParts = Box{x, sectionBodyTop(y), w, kLabelLine + kChartTextGap + kDayPartBar + kChartTextGap + kLabelLine};
  return out;
}

constexpr Box column(const Box& area, const int count, const int gap, const int index) {
  if (count <= 0) return Box{};
  const int width = (area.width - (count - 1) * gap) / count;
  return Box{area.x + index * (width + gap), area.y, width, area.height};
}

constexpr Box weekColumn(const Box& chart, const int index) {
  return column(chart, kWeekColumns, kWeekColumnGap, index);
}

constexpr Box totalsCell(const Box& grid, const int index) {
  const Box col = column(grid, kTotalsColumns, 0, index % kTotalsColumns);
  return Box{col.x, grid.y + (index / kTotalsColumns) * kTotalsCell, col.width, kTotalsCell};
}

constexpr Box dayPart(const Box& area, const int index) { return column(area, kDayParts, kDayPartGap, index); }

// Bar height for value against the busiest value: 0 stays empty, anything read shows at
// least a 2 px sliver so a short day is not mistaken for none.
constexpr int barHeight(const uint32_t value, const uint32_t busiest, const int maxHeight) {
  if (value == 0 || busiest == 0 || maxHeight <= 0) return 0;
  const uint64_t scaled = (static_cast<uint64_t>(value) * static_cast<uint64_t>(maxHeight) + busiest / 2) / busiest;
  const int h = static_cast<int>(scaled > static_cast<uint64_t>(maxHeight) ? maxHeight : scaled);
  return h < 2 ? 2 : h;
}

// Rounded whole percent of part in total; 0 when nothing is recorded.
constexpr int sharePercent(const uint32_t part, const uint32_t total) {
  if (total == 0) return 0;
  return static_cast<int>((static_cast<uint64_t>(part) * 100u + total / 2) / total);
}

// Index of the largest value, or -1 when every value is 0 (first wins a tie).
template <size_t N>
constexpr int busiestIndex(const std::array<uint32_t, N>& values) {
  int best = -1;
  uint32_t most = 0;
  for (size_t i = 0; i < N; ++i) {
    if (values[i] > most) {
      most = values[i];
      best = static_cast<int>(i);
    }
  }
  return best;
}

}  // namespace PiccoStatsLayout

namespace PiccoTransferLayout {

using PiccoLayout::Box;

// Blocking state (T_States "Blocking progress"): a band between two hairlines, centred
// on the space under the header.
constexpr int kStatusBandHeight = 236;
constexpr int kStatusPadX = 32;

constexpr Box statusBand(const Box& screen, const int headerBottom) {
  using namespace PiccoTokens;
  const int x = screen.x + kGutter;
  const int w = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  const int space = screen.bottom() - headerBottom;
  const int top = headerBottom + PiccoLayout::maxOf(kSectionFirstAbove, (space - kStatusBandHeight) / 2);
  return Box{x, top, w, kStatusBandHeight};
}

}  // namespace PiccoTransferLayout
