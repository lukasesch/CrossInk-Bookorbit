#pragma once

#include <cstdint>

#include "components/picco/PiccoLibraryLayout.h"
#include "components/picco/PiccoTokens.h"

// Pure geometry for the reader's list screens (T_Contents, T_LookupHistory): the
// Contents tab bar shared by chapters, bookmarks and clippings, and the list band under it.
namespace PiccoReaderLists {

enum class ContentsTab : uint8_t { Chapters, Bookmarks, Clippings };
constexpr int kContentsTabCount = 3;

// 3-cell tab bar, kBandGap under the header hairline.
constexpr PiccoLayout::Box contentsTabs(const PiccoLayout::Box& screen, const int headerBottom) {
  return PiccoLibraryLayout::bandBelow(screen, headerBottom, PiccoTokens::kTabBarHeight);
}

// Top of the list band: under the tabs, or under the header when there are none.
constexpr int listTop(const PiccoLayout::Box& screen, const int headerBottom, const bool tabs) {
  return (tabs ? contentsTabs(screen, headerBottom).bottom() : headerBottom) + PiccoLibraryLayout::kBandGap;
}

// Tab under (px, py), or -1.
constexpr int contentsTabAt(const PiccoLayout::Box& screen, const int headerBottom, const int px, const int py) {
  return PiccoLibraryLayout::segmentAt(contentsTabs(screen, headerBottom), kContentsTabCount, px, py);
}

}  // namespace PiccoReaderLists
