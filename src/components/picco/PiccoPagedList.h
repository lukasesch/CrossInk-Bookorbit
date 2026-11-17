#pragma once

#include <cstdint>

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// Pure paging for Picco lists (T_DesignLanguage: swipe flips one page, the rocker moves row
// by row and flips at the page edge, `n / m` at the bottom). No renderer: native-tested.
namespace PiccoPagedList {

// ---- Uniform lists (one row height, no sections) ------------------------------------------

struct Cursor {
  int selected = 0;
  int top = 0;
};

// First row of the page holding `index`, clamped the way a list clamps its last viewport.
constexpr int pageTopFor(const int index, const int rowsPerPage, const int count) {
  if (rowsPerPage <= 0 || index <= 0) return 0;
  const int maxTop = PiccoLayout::maxOf(0, count - rowsPerPage);
  return PiccoLayout::minOf((index / rowsPerPage) * rowsPerPage, maxTop);
}

// One rocker step (direction +1 down, -1 up), wrapping at both ends. The viewport stays put
// while the selection is on it and flips a whole page when the selection leaves it.
constexpr Cursor rockerStep(const Cursor cursor, const int direction, const int rowsPerPage, const int count) {
  if (count <= 0) return Cursor{};
  int selected = cursor.selected + (direction > 0 ? 1 : -1);
  if (selected >= count) selected = 0;
  if (selected < 0) selected = count - 1;
  const int rows = PiccoLayout::maxOf(1, rowsPerPage);
  int top = cursor.top;
  if (selected < top || selected >= top + rows) top = pageTopFor(selected, rows, count);
  return Cursor{selected, top};
}

// ---- Sectioned lists (settings pages) -----------------------------------------------------

struct Entry {
  bool section = false;
  int16_t height = PiccoTokens::kRowOneLine;  // rows only; a section's height comes from its text
};

struct Slot {
  int16_t y = 0;       // rows: band top (the hairline row); sections: text top
  int16_t height = 0;  // rows: band height; sections: text line height
  bool hairline = false;
};

// Section block: kSectionFirstAbove (first on a page) or kSectionAbove, the text, kSectionBelow.
constexpr int sectionAbove(const bool firstOnPage) {
  return firstOnPage ? PiccoTokens::kSectionFirstAbove : PiccoTokens::kSectionAbove;
}

// Lays out entries from `start` between top and bottom. Fills out[start..] and returns the
// first index that did not fit (count when everything fit). A section never ends a page
// without its first row; a page always takes at least one entry so paging terminates.
// A page that opens on a row (a section continued from the page before) starts kGroupGap down,
// or, with continuedLabel, below a repeated "<section> (continued)" label (T_Controls2).
// No hairline above a page's or a section's first row (addendum Q3).
inline int layoutPage(const Entry* entries, const int count, const int start, const int top, const int bottom,
                      const int sectionTextHeight, Slot* out, const bool continuedLabel = false) {
  using namespace PiccoTokens;
  int y = top;
  bool previousWasRow = false;
  for (int i = start; i < count; ++i) {
    const bool first = i == start;
    if (entries[i].section) {
      const int above = sectionAbove(first);
      const int block = above + sectionTextHeight + kSectionBelow;
      const int nextRow = i + 1 < count && !entries[i + 1].section ? entries[i + 1].height : 0;
      if (!first && y + block + nextRow > bottom) return i;
      out[i] = Slot{static_cast<int16_t>(y + above), static_cast<int16_t>(sectionTextHeight), false};
      y += block;
      previousWasRow = false;
    } else {
      if (first) y += continuedLabel ? sectionAbove(true) + sectionTextHeight + kSectionBelow : kGroupGap;
      if (!first && y + entries[i].height > bottom) return i;
      out[i] = Slot{static_cast<int16_t>(y), entries[i].height, previousWasRow};
      y += entries[i].height;
      previousWasRow = true;
    }
  }
  return count;
}

// Section heading the entry at `index` belongs to; -1 when none precedes it.
constexpr int owningSection(const Entry* entries, const int index) {
  for (int i = index; i >= 0; --i) {
    if (entries[i].section) return i;
  }
  return -1;
}

// Whether the page starting at `start` repeats its section's label as "(continued)".
constexpr bool opensContinued(const Entry* entries, const int count, const int start) {
  return start > 0 && start < count && !entries[start].section && owningSection(entries, start) >= 0;
}

// Text top of a repeated "(continued)" label on a page whose list starts at `top`.
constexpr int continuedLabelY(const int top) { return top + PiccoTokens::kSectionFirstAbove; }

// Page starts for the whole list; returns the page count (>= 1), at most maxPages.
// continuedLabels: pages that open mid-section repeat the section label (opensContinued).
inline int paginate(const Entry* entries, const int count, const int top, const int bottom, const int sectionTextHeight,
                    Slot* out, int16_t* pageStarts, const int maxPages, const bool continuedLabels = false) {
  if (maxPages <= 0) return 0;
  pageStarts[0] = 0;
  int pages = 1;
  int start = 0;
  while (start < count) {
    const bool continued = continuedLabels && opensContinued(entries, count, start);
    const int next = layoutPage(entries, count, start, top, bottom, sectionTextHeight, out, continued);
    if (next >= count) break;
    if (pages == maxPages) break;
    pageStarts[pages++] = static_cast<int16_t>(next);
    start = next;
  }
  return pages;
}

// 0-based page holding entry `index`.
constexpr int pageOf(const int16_t* pageStarts, const int pages, const int index) {
  int page = 0;
  for (int p = 1; p < pages; ++p) {
    if (index >= pageStarts[p]) page = p;
  }
  return page;
}

constexpr int pageEnd(const int16_t* pageStarts, const int pages, const int page, const int count) {
  return page + 1 < pages ? pageStarts[page + 1] : count;
}

// Next selectable (non-section) entry after `index` in `direction`, wrapping; -1 when none.
inline int nextSelectable(const Entry* entries, const int count, const int index, const int direction) {
  if (count <= 0) return -1;
  int i = index;
  for (int step = 0; step < count; ++step) {
    i += direction > 0 ? 1 : -1;
    if (i >= count) i = 0;
    if (i < 0) i = count - 1;
    if (!entries[i].section) return i;
  }
  return -1;
}

// First selectable entry on a page; -1 when the page holds only sections.
inline int firstSelectableOnPage(const Entry* entries, const int count, const int16_t* pageStarts, const int pages,
                                 const int page) {
  const int end = pageEnd(pageStarts, pages, page, count);
  for (int i = pageStarts[page]; i < end; ++i) {
    if (!entries[i].section) return i;
  }
  return -1;
}

}  // namespace PiccoPagedList
