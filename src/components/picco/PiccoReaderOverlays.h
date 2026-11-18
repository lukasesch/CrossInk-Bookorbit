#pragma once

#include <array>
#include <cstdint>

#include "PiccoLayout.h"

// Pure geometry for the Picco overlays over a book page (boards T_WordLookup, T_Footnote,
// T_ClippingSelect) and the Picco sleep screen (T_Sleep). No renderer, so the native
// tests check it; the activities draw and hit-test from the same boxes.
namespace PiccoOverlay {

using PiccoLayout::Box;

constexpr int kPopoverGap = 8;     // anchor -> popover edge
constexpr int kButtonRowGap = 16;  // popover text -> its button row
constexpr int kMaxButtons = 3;

// Equal framed buttons (60 tall, gap 8) filling width from x, top at y.
struct ButtonRow {
  std::array<Box, kMaxButtons> cells{};
  uint8_t count = 0;
};

constexpr ButtonRow buttonRow(const int x, const int y, const int width, const int count) {
  using namespace PiccoTokens;
  ButtonRow row{};
  const int n = count < 0 ? 0 : (count > kMaxButtons ? kMaxButtons : count);
  row.count = static_cast<uint8_t>(n);
  if (n == 0) return row;
  const int cell = (width - (n - 1) * kNeighbourGap) / n;
  for (int i = 0; i < n; ++i) {
    // The last cell takes the rounding remainder so the row ends exactly at x + width.
    const int left = x + i * (cell + kNeighbourGap);
    const int right = i == n - 1 ? x + width : left + cell;
    row.cells[static_cast<size_t>(i)] = Box{left, y, right - left, kActionHeight};
  }
  return row;
}

// Popover frame (r22, 1 px, padding 24) spanning the screen between the gutters: below the
// anchor when it fits, else above it, else pinned to the bottom gutter.
constexpr Box popoverFrame(const Box& screen, const Box& anchor, const int contentHeight) {
  using namespace PiccoTokens;
  const int height = contentHeight + 2 * kDialogPadding;
  const int x = screen.x + kGutter;
  const int width = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  const int below = anchor.bottom() + kPopoverGap;
  if (below + height <= screen.bottom() - kGutter) return Box{x, below, width, height};
  const int above = anchor.y - kPopoverGap - height;
  if (above >= screen.y + kGutter) return Box{x, above, width, height};
  return Box{x, PiccoLayout::maxOf(screen.y + kGutter, screen.bottom() - kGutter - height), width, height};
}

constexpr Box popoverInner(const Box& frame) {
  using namespace PiccoTokens;
  return Box{frame.x + kDialogPadding, frame.y + kDialogPadding, PiccoLayout::maxOf(0, frame.width - 2 * kDialogPadding),
             PiccoLayout::maxOf(0, frame.height - 2 * kDialogPadding)};
}

// Footnote popover (T_Footnote): "NOTE n" label, then Go to note | Close.
constexpr int kNoteLabelHeight = 22;
constexpr int kNoteButtonsGap = 18;

struct NotePopover {
  Box frame;
  Box label;
  ButtonRow buttons;
};

constexpr NotePopover notePopover(const Box& screen, const Box& anchor) {
  NotePopover p{};
  p.frame = popoverFrame(screen, anchor, kNoteLabelHeight + kNoteButtonsGap + PiccoTokens::kActionHeight);
  const Box inner = popoverInner(p.frame);
  p.label = Box{inner.x, inner.y, inner.width, kNoteLabelHeight};
  p.buttons = buttonRow(inner.x, p.label.bottom() + kNoteButtonsGap, inner.width, 2);
  return p;
}

// Selection toolbar (T_ClippingSelect): hairline, "n words" left, Cancel (110) and the
// Save pill (saveWidth, measured by PiccoPillButton) right, 20 above the screen bottom.
constexpr int kToolbarBottom = 20;
constexpr int kToolbarPadTop = 12;
constexpr int kToolbarCancelW = 110;

struct ClipToolbar {
  Box band;  // white band under the controls; its top is the hairline
  Box count;
  Box cancel;
  Box save;
};

constexpr ClipToolbar clipToolbar(const Box& screen, const int saveWidth) {
  using namespace PiccoTokens;
  ClipToolbar t{};
  const int x = screen.x + kGutter;
  const int right = screen.right() - kGutter;
  const int top = screen.bottom() - kToolbarBottom - kActionHeight - kToolbarPadTop;
  t.band = Box{x, top, right - x, kToolbarPadTop + kActionHeight + kToolbarBottom};
  const int controlsY = top + kToolbarPadTop;
  t.save = Box{right - saveWidth, controlsY, saveWidth, kPillHeight};
  t.cancel = Box{t.save.x - kNeighbourGap - kToolbarCancelW, controlsY, kToolbarCancelW, kActionHeight};
  t.count = Box{x, controlsY, PiccoLayout::maxOf(0, t.cancel.x - kGroupGap - x), kActionHeight};
  return t;
}

// Sleep screen (T_Sleep): framed 2:3 cover, chapter + percent, progress, wake hint.
constexpr int kSleepCoverTop = 96;
constexpr int kSleepCoverW = 300;
constexpr int kSleepCoverH = 450;
constexpr int kSleepMetaGap = 26;
constexpr int kSleepLine = 22;
constexpr int kSleepProgressGap = 10;
constexpr int kSleepHintBottom = 40;

struct SleepLayout {
  Box cover;
  Box meta;
  Box progress;
  Box hint;
};

constexpr SleepLayout sleepLayout(const Box& screen) {
  using namespace PiccoTokens;
  SleepLayout s{};
  s.hint = Box{screen.x + kGutter, screen.bottom() - kSleepHintBottom - kSleepLine,
               PiccoLayout::maxOf(0, screen.width - 2 * kGutter), kSleepLine};
  // Shrink the cover on short screens so the meta block keeps clear of the hint.
  const int reserve = kSleepMetaGap + kSleepLine + kSleepProgressGap + kProgressFill + kSleepMetaGap;
  const int room = s.hint.y - (screen.y + kSleepCoverTop) - reserve;
  const int coverH = PiccoLayout::maxOf(0, PiccoLayout::minOf(kSleepCoverH, room));
  const int coverW = PiccoLayout::minOf(coverH * kSleepCoverW / kSleepCoverH, screen.width - 2 * kGutter);
  s.cover = Box{screen.x + (screen.width - coverW) / 2, screen.y + kSleepCoverTop, coverW, coverH};
  s.meta = Box{s.cover.x, s.cover.bottom() + kSleepMetaGap, coverW, kSleepLine};
  s.progress = Box{s.cover.x, s.meta.bottom() + kSleepProgressGap, coverW, kProgressFill};
  return s;
}

}  // namespace PiccoOverlay
