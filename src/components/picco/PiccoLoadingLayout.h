#pragma once

#include "PiccoLayout.h"
#include "PiccoTokens.h"

// Book loading screen (board L_B, 480 x 800): framed cover, title and author, and while
// indexing a progress line with its status text. No header; positions run from the screen top.
namespace PiccoLoadingLayout {

using PiccoLayout::Box;

constexpr int kCoverTop = 120;
constexpr int kCoverW = 200;
constexpr int kCoverH = 300;
constexpr int kTitleGap = 36;  // cover -> title line
constexpr int kTitleLine = 36;
constexpr int kAuthorLine = 26;
constexpr int kProgressTop = 620;
constexpr int kProgressInset = 88;
constexpr int kStatusGap = 12;
constexpr int kStatusLine = 24;  // 17 px text

struct Layout {
  Box cover;
  Box title;
  Box author;
  Box bar;           // PiccoProgress box, kProgressFill tall
  Box status;        // "Preparing pages · 45 %"
  Box progressArea;  // bar and status: the region a progress update repaints
};

constexpr Layout layout(const Box& screen) {
  using namespace PiccoTokens;
  Layout l{};
  l.cover = Box{screen.x + (screen.width - kCoverW) / 2, screen.y + kCoverTop, kCoverW, kCoverH};
  const int textW = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  l.title = Box{screen.x + kGutter, l.cover.bottom() + kTitleGap, textW, kTitleLine};
  l.author = Box{l.title.x, l.title.bottom(), textW, kAuthorLine};
  l.bar = Box{screen.x + kProgressInset, screen.y + kProgressTop,
              PiccoLayout::maxOf(0, screen.width - 2 * kProgressInset), kProgressFill};
  l.status = Box{l.title.x, l.bar.bottom() + kStatusGap, textW, kStatusLine};
  l.progressArea = Box{l.status.x, l.bar.y, textW, l.status.bottom() - l.bar.y};
  return l;
}

}  // namespace PiccoLoadingLayout
