#pragma once

#include <array>
#include <cstdint>

#include "PiccoTokens.h"

// Pure Picco geometry: every component draws and hit-tests from these functions, and the
// native tests check them. No renderer, no settings; callers pass measured text widths.
namespace PiccoLayout {

struct Box {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  constexpr int right() const { return x + width; }
  constexpr int bottom() const { return y + height; }
  constexpr bool empty() const { return width <= 0 || height <= 0; }
  constexpr bool contains(const int px, const int py) const {
    return px >= x && px < right() && py >= y && py < bottom();
  }
  constexpr bool overlaps(const Box& o) const {
    return !empty() && !o.empty() && x < o.right() && o.x < right() && y < o.bottom() && o.y < bottom();
  }
};

constexpr int maxOf(const int a, const int b) { return a > b ? a : b; }
constexpr int minOf(const int a, const int b) { return a < b ? a : b; }

// ---- Header ------------------------------------------------------------------------------

enum class Leading : uint8_t { None, Back, House };

struct HeaderInputs {
  Leading leading = Leading::None;
  int titleWidth = 0;
  int leadingLabelWidth = 0;  // "Home" after the house; 0 otherwise
  int contextualWidth = 0;    // 0 = no contextual action; else the action's width (>= kHeaderControl)
  int syncWidth = 0;          // icon + gap + label; 0 = hidden
  int clockWidth = 0;         // 0 = hidden
  int batteryTextWidth = 0;   // "82%"
  int titleHeight = 0;        // title font line height, for the touch rect only
  int titleRightReserve = 0;  // caller-owned controls at the header's right edge
};

struct HeaderLayout {
  Box band;          // inside the header rect, above the hairline
  Box hairline;      // 1 px, spans the gutter
  Box leadingIcon;   // chevron or house
  Box leadingTouch;  // back / home target, >= kTouchMin tall
  Box title;
  Box contextual;  // kHeaderControl square, left of the divider
  Box divider;
  Box sync;
  Box clock;
  Box battery;  // icon + gap + percent
  int clusterLeft = 0;
  int centerY = 0;
};

constexpr int statusClusterWidth(const int syncWidth, const int clockWidth, const int batteryTextWidth,
                                 const int contextualWidth) {
  const int battery = PiccoTokens::kHeaderBatteryIconW + PiccoTokens::kHeaderBatteryGap + batteryTextWidth;
  int width = battery;
  if (clockWidth > 0) width += clockWidth + PiccoTokens::kHeaderClusterGap;
  const bool divider = contextualWidth > 0 || syncWidth > 0;
  if (divider) width += PiccoTokens::kHairline + PiccoTokens::kHeaderClusterGap;
  if (syncWidth > 0) width += syncWidth + PiccoTokens::kHeaderClusterGap;
  if (contextualWidth > 0) width += contextualWidth + PiccoTokens::kHeaderClusterGap;
  return width;
}

constexpr Box centeredBox(const int x, const int centerY, const int width, const int height) {
  return Box{x, centerY - height / 2, width, height};
}

constexpr HeaderLayout headerLayout(const Box& header, const HeaderInputs& in) {
  using namespace PiccoTokens;
  HeaderLayout out{};
  const int bandHeight = maxOf(0, header.height - kHairline);
  out.band = Box{header.x, header.y, header.width, bandHeight};
  const int left = header.x + kGutter;
  const int right = header.right() - kGutter;
  out.hairline = Box{left, header.y + bandHeight, maxOf(0, right - left), kHairline};
  out.centerY = header.y + bandHeight / 2;

  // Right to left: battery, clock, divider, sync, contextual.
  int x = right;
  const int batteryW = kHeaderBatteryIconW + kHeaderBatteryGap + in.batteryTextWidth;
  x -= batteryW;
  out.battery = centeredBox(x, out.centerY, batteryW, kHeaderBatteryIconH);
  if (in.clockWidth > 0) {
    x -= kHeaderClusterGap + in.clockWidth;
    out.clock = centeredBox(x, out.centerY, in.clockWidth, kHeaderBatteryIconH);
  }
  if (in.contextualWidth > 0 || in.syncWidth > 0) {
    x -= kHeaderClusterGap + kHairline;
    out.divider = centeredBox(x, out.centerY, kHairline, kHeaderDividerH);
  }
  if (in.syncWidth > 0) {
    x -= kHeaderClusterGap + in.syncWidth;
    out.sync = centeredBox(x, out.centerY, in.syncWidth, kHeaderControl);
  }
  if (in.contextualWidth > 0) {
    x -= kHeaderClusterGap + in.contextualWidth;
    out.contextual = centeredBox(x, out.centerY, in.contextualWidth, kHeaderControl);
  }
  out.clusterLeft = x;

  // Left: optional leading icon, then the title.
  int titleX = left;
  if (in.leading != Leading::None) {
    const int iconSize = in.leading == Leading::House ? kIconHouse : kIconChevron;
    // The board pulls the chevron 2 px into the gutter so its stroke lines up with the rows.
    const int iconX = left - 2;
    out.leadingIcon = centeredBox(iconX, out.centerY, iconSize, iconSize);
    titleX = iconX + iconSize + kHeaderLeadingGap;
    if (in.leading == Leading::House && in.leadingLabelWidth > 0) {
      titleX += in.leadingLabelWidth + kHeaderLeadingGap;
    }
  }
  int titleRight = out.clusterLeft - kGroupGap;
  if (in.titleRightReserve > 0) titleRight = minOf(titleRight, header.right() - in.titleRightReserve);
  const int titleWidth = maxOf(0, minOf(in.titleWidth, titleRight - titleX));
  out.title = centeredBox(titleX, out.centerY, titleWidth, in.titleHeight);

  if (in.leading != Leading::None) {
    // The whole band above the hairline, from the screen edge through the title.
    const int touchRight = maxOf(left + kTouchMin, titleX + titleWidth + kGroupGap);
    out.leadingTouch = Box{header.x, header.y, maxOf(0, minOf(touchRight, titleRight) - header.x), bandHeight};
  }
  return out;
}

// ---- List rows ---------------------------------------------------------------------------

// FreeInkUI list props that put a row's focus frame kRowFrameOutsetX into the gutter and
// kRowFrameInsetY off the hairlines while labels stay on the gutter: the row box is the
// frame, the hairline sits in the middle of the gap between two row boxes.
struct ListRowGeometry {
  int rowInset;     // band edge -> row box
  int sidePadding;  // row box -> label
  int rowGap;       // between row boxes; the hairline is drawn at its centre
  int rowHeight;    // row box height; rowHeight + rowGap = the row step
};

constexpr ListRowGeometry listRowGeometry(const int rowStep) {
  using namespace PiccoTokens;
  return ListRowGeometry{kGutter - kRowFrameOutsetX, kRowFrameOutsetX, 2 * kRowFrameInsetY,
                         rowStep - 2 * kRowFrameInsetY};
}

// Focus frame of a row whose hairline-to-hairline band starts at bandTop, in a list
// spanning the screen width.
constexpr Box rowFocusFrame(const int screenWidth, const int bandTop, const int rowStep) {
  const ListRowGeometry g = listRowGeometry(rowStep);
  return Box{g.rowInset, bandTop + PiccoTokens::kRowFrameInsetY, screenWidth - 2 * g.rowInset, g.rowHeight};
}

// ---- Page indicator and swipe paging -----------------------------------------------------

constexpr Box pageIndicatorBand(const Box& screen) {
  return Box{screen.x + PiccoTokens::kGutter, screen.bottom() - PiccoTokens::kPageIndicatorFromBottom,
             maxOf(0, screen.width - 2 * PiccoTokens::kGutter), PiccoTokens::kPageIndicatorHeight};
}

// Lowest y a paged list may reach.
constexpr int listBottomLimit(const Box& screen) { return screen.bottom() - PiccoTokens::kListBottomReserve; }

constexpr int pageCount(const int count, const int rowsPerPage) {
  if (count <= 0 || rowsPerPage <= 0) return 1;
  return (count + rowsPerPage - 1) / rowsPerPage;
}

// 1-based page shown for a viewport starting at top. A list clamps its last viewport to
// count - rows, so a top at or past that clamp is the last page.
constexpr int pageForTop(const int top, const int rowsPerPage, const int count) {
  const int pages = pageCount(count, rowsPerPage);
  if (rowsPerPage <= 0 || top <= 0) return 1;
  if (top + rowsPerPage >= count) return pages;
  return minOf(pages, top / rowsPerPage + 1);
}

// Viewport top after one swipe (direction +1 = next page, -1 = previous).
constexpr int topAfterSwipe(const int top, const int direction, const int rowsPerPage, const int count) {
  if (rowsPerPage <= 0) return 0;
  const int maxTop = maxOf(0, count - rowsPerPage);
  const int next = top + (direction > 0 ? rowsPerPage : -rowsPerPage);
  return next < 0 ? 0 : minOf(next, maxTop);
}

// ---- Switch ------------------------------------------------------------------------------

struct SwitchGeometry {
  Box track;
  Box knob;
};

constexpr SwitchGeometry switchGeometry(const int x, const int y, const bool on) {
  using namespace PiccoTokens;
  SwitchGeometry g{};
  g.track = Box{x, y, kSwitchW, kSwitchH};
  const int knobX = on ? x + kSwitchW - kSwitchKnobInset - kSwitchKnob : x + kSwitchKnobInset;
  g.knob = Box{knobX, y + kSwitchKnobInset, kSwitchKnob, kSwitchKnob};
  return g;
}

namespace detail {
// Point (2px+1, 2py+1) in doubled coordinates inside a rounded rect of radius r.
constexpr bool insideRounded(const int px, const int py, const int x, const int y, const int w, const int h,
                             const int r) {
  const int cx2 = 2 * px + 1;
  const int cy2 = 2 * py + 1;
  if (cx2 < 2 * x || cx2 >= 2 * (x + w) || cy2 < 2 * y || cy2 >= 2 * (y + h)) return false;
  const int rr = r < 0 ? 0 : (2 * r > w ? w / 2 : (2 * r > h ? h / 2 : r));
  const int left = 2 * (x + rr);
  const int right = 2 * (x + w - rr);
  const int top = 2 * (y + rr);
  const int bottom = 2 * (y + h - rr);
  const int dx = cx2 < left ? left - cx2 : (cx2 > right ? cx2 - right : 0);
  const int dy = cy2 < top ? top - cy2 : (cy2 > bottom ? cy2 - bottom : 0);
  return dx * dx + dy * dy <= 4 * rr * rr;
}

constexpr bool ringPixel(const int px, const int py, const int x, const int y, const int w, const int h, const int r,
                         const int stroke) {
  return insideRounded(px, py, x, y, w, h, r) &&
         !insideRounded(px, py, x + stroke, y + stroke, w - 2 * stroke, h - 2 * stroke, r - stroke);
}
}  // namespace detail

// Whether the switch draws ink at (px, py), relative to its top-left corner.
constexpr bool switchInk(const int px, const int py, const bool on) {
  using namespace PiccoTokens;
  const SwitchGeometry g = switchGeometry(0, 0, on);
  if (detail::ringPixel(px, py, 0, 0, kSwitchW, kSwitchH, kSwitchH / 2, kSwitchStroke)) return true;
  const Box& k = g.knob;
  if (on) return detail::insideRounded(px, py, k.x, k.y, k.width, k.height, kSwitchKnob / 2);
  return detail::ringPixel(px, py, k.x, k.y, k.width, k.height, kSwitchKnob / 2, kSwitchStroke);
}

constexpr int kSwitchRowBytes = (PiccoTokens::kSwitchW + 7) / 8;
using SwitchBits = std::array<uint8_t, kSwitchRowBytes * PiccoTokens::kSwitchH>;

// 1-bpp mask in the freeink::Icon convention (MSB first, bit 0 = ink), built at compile time.
constexpr SwitchBits switchBits(const bool on) {
  SwitchBits bits{};
  for (int y = 0; y < PiccoTokens::kSwitchH; ++y) {
    for (int x = 0; x < kSwitchRowBytes * 8; ++x) {
      const bool ink = x < PiccoTokens::kSwitchW && switchInk(x, y, on);
      if (!ink) bits[y * kSwitchRowBytes + x / 8] |= static_cast<uint8_t>(0x80u >> (x % 8));
    }
  }
  return bits;
}

// ---- Progress, pill, card, toast ---------------------------------------------------------

// Fill width for permille in [0, 1000]; out-of-range values clamp.
constexpr int progressFillWidth(const int width, const int permille) {
  const int clamped = permille < 0 ? 0 : (permille > 1000 ? 1000 : permille);
  return width <= 0 ? 0 : static_cast<int>((static_cast<int64_t>(width) * clamped + 500) / 1000);
}

struct ProgressGeometry {
  Box track;  // 1 px, vertically centred on the fill
  Box fill;
};

constexpr ProgressGeometry progressGeometry(const Box& rect, const int permille) {
  using namespace PiccoTokens;
  ProgressGeometry g{};
  const int top = rect.y + (rect.height - kProgressFill) / 2;
  g.track = Box{rect.x, top + (kProgressFill - kProgressTrack) / 2, rect.width, kProgressTrack};
  g.fill = Box{rect.x, top, progressFillWidth(rect.width, permille), kProgressFill};
  return g;
}

constexpr int pillWidth(const int labelWidth, const int iconWidth) {
  return labelWidth + 2 * PiccoTokens::kPillPadX + (iconWidth > 0 ? iconWidth + PiccoTokens::kHeaderLeadingGap : 0);
}

enum class ToastKind : uint8_t { Info, Progress, Error, ErrorTwoLine };

constexpr int toastHeight(const ToastKind kind) {
  switch (kind) {
    case ToastKind::Info:
    case ToastKind::Progress:
      return PiccoTokens::kToastHeight;
    case ToastKind::Error:
      return PiccoTokens::kErrorToastHeight;
    case ToastKind::ErrorTwoLine:
      return PiccoTokens::kErrorToastTwoLineHeight;
  }
  return PiccoTokens::kToastHeight;
}

// Toast band: gutter to gutter, kToastGapBelowHeader under the header's bottom edge.
constexpr Box toastRect(const Box& screen, const int headerBottom, const ToastKind kind) {
  return Box{screen.x + PiccoTokens::kGutter, headerBottom + PiccoTokens::kToastGapBelowHeader,
             maxOf(0, screen.width - 2 * PiccoTokens::kGutter), toastHeight(kind)};
}

}  // namespace PiccoLayout
