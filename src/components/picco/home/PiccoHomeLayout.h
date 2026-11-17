#pragma once

#include <array>
#include <cstdint>

#include "components/picco/PiccoLayout.h"
#include "components/picco/PiccoTokens.h"

// Pure geometry of the Picco Home (board T_Combined, 480 x 800, 1 board px = 1 device px).
// Everything is derived from the screen box; the renderer only fills these boxes.
namespace PiccoHomeLayout {

using PiccoLayout::Box;

// Hero: cover left, text column right.
constexpr int kHeroTop = 96;
constexpr int kCoverW = 128;
constexpr int kCoverH = 192;
constexpr int kHeroGap = 20;
constexpr int kSectionLabelLine = 20;
constexpr int kTitleTopGap = 2;
constexpr int kTitleLine = 36;
constexpr int kTitleMaxLines = 2;
constexpr int kAuthorLine = 24;
constexpr int kMetaLine = 20;
constexpr int kMetaGap = 6;   // meta row -> progress
constexpr int kPillGap = 12;  // progress -> pill

// Text cover: padding 14 / 10, double rule 5 tall.
constexpr int kCoverPadY = 14;
constexpr int kCoverPadX = 10;
constexpr int kCoverRuleH = 5;

// Stats card.
constexpr int kSectionGap = 16;
constexpr int kStatsHeight = 224;
constexpr int kCardPadX = 16;
constexpr int kCardPadY = 12;
constexpr int kStatsHeadLine = 21;
constexpr int kStatsBodyGap = 8;
constexpr int kStatsBodyH = 104;
constexpr int kStreakColW = 150;
constexpr int kStreakColGap = 18;
constexpr int kFigureLine = 56;
constexpr int kFigureUnitGap = 6;
constexpr int kStreakLabelGap = 4;
constexpr int kSmallLine = 21;
constexpr int kBarCount = 7;
constexpr int kBarGap = 6;
constexpr int kBarLabelGap = 4;
constexpr int kBarRadius = 3;
constexpr int kTotalsPadTop = 8;
constexpr int kTotalsPadX = 10;
constexpr int kTotalsValueLine = 25;
constexpr int kTotalsCount = 3;

// Tiles.
constexpr int kTileCount = 4;
constexpr int kTileGap = 16;
constexpr int kTileLabelLine = 25;
constexpr int kTileSubtitleGap = 2;

// Focus ring: kSelectionStroke wide, its outer edge kFocusOutset outside the element.
constexpr int kFocusOutset = 4;

struct Hero {
  Box area;
  Box cover;
  Box text;   // the column right of the cover
  Box label;  // "CONTINUE READING"
  Box title;  // first title line; further lines step kTitleLine
  Box meta;   // chapter left, percent right
  Box progress;
  Box pill;
};

struct TextCover {
  Box rule;    // double rule: top and bottom 1 px lines
  Box title;   // centred between the rule and the author
  Box author;  // one line at the bottom
};

struct Stats {
  Box card;
  Box inner;
  Box head;    // label left, "This week" right
  Box streak;  // left column of the body
  Box figure;  // key figure line
  Box streakLabel;
  Box today;
  Box chart;  // bars plus their day labels
  std::array<Box, kBarCount> columns;
  int barBase = 0;                       // y below the bars
  int barMaxH = 0;                       // tallest bar
  Box totalsRule;                        // hairline over the totals
  std::array<Box, kTotalsCount> totals;  // value line on top, label line below
  std::array<Box, kTotalsCount - 1> totalsDividers;
};

struct Tile {
  Box rect;
  Box icon;
  Box label;
  Box subtitle;
};

struct Home {
  Box screen;
  Box header;
  Hero hero;
  Stats stats;
  std::array<Tile, kTileCount> tiles;
};

constexpr Box inset(const Box& b, const int dx, const int dy) {
  return Box{b.x + dx, b.y + dy, PiccoLayout::maxOf(0, b.width - 2 * dx), PiccoLayout::maxOf(0, b.height - 2 * dy)};
}

constexpr Hero heroLayout(const Box& screen) {
  using namespace PiccoTokens;
  Hero h{};
  h.area = Box{screen.x + kGutter, screen.y + kHeroTop, PiccoLayout::maxOf(0, screen.width - 2 * kGutter), kCoverH};
  h.cover = Box{h.area.x, h.area.y, kCoverW, kCoverH};
  const int textX = h.cover.right() + kHeroGap;
  h.text = Box{textX, h.area.y, PiccoLayout::maxOf(0, h.area.right() - textX), h.area.height};
  h.label = Box{textX, h.area.y, h.text.width, kSectionLabelLine};
  h.title = Box{textX, h.label.bottom() + kTitleTopGap, h.text.width, kTitleLine};
  // Bottom-aligned block: the pill's bottom edge is the cover's.
  h.pill = Box{textX, h.area.bottom() - kPillHeight, h.text.width, kPillHeight};
  h.progress = Box{textX, h.pill.y - kPillGap - kProgressFill, h.text.width, kProgressFill};
  h.meta = Box{textX, h.progress.y - kMetaGap - kMetaLine, h.text.width, kMetaLine};
  return h;
}

// Title lines that fit above the meta row. One line keeps the board's 36 px line and the
// author under it; a second line drops the author and tightens to 32 px (line-height 1)
// so both lines still clear the meta row.
constexpr int kTitleLineTight = 32;

constexpr int titleStep(const int lines) { return lines > 1 ? kTitleLineTight : kTitleLine; }

constexpr int titleLinesAvailable(const Hero& h, const bool withAuthor) {
  const int room = h.meta.y - h.title.y;
  if (withAuthor) return 1;
  return kTitleMaxLines * kTitleLineTight <= room ? kTitleMaxLines : 1;
}

constexpr Box titleLine(const Hero& h, const int line, const int lines) {
  const int step = titleStep(lines);
  return Box{h.title.x, h.title.y + line * step, h.title.width, step};
}

constexpr Box authorLine(const Hero& h) { return Box{h.title.x, h.title.y + kTitleLine, h.title.width, kAuthorLine}; }

constexpr TextCover textCoverLayout(const Box& cover) {
  TextCover c{};
  const Box inner = inset(cover, kCoverPadX, kCoverPadY);
  c.rule = Box{inner.x, inner.y, inner.width, kCoverRuleH};
  c.author = Box{inner.x, inner.bottom() - kSmallLine, inner.width, kSmallLine};
  c.title = Box{inner.x, c.rule.bottom(), inner.width, PiccoLayout::maxOf(0, c.author.y - c.rule.bottom())};
  return c;
}

constexpr Stats statsLayout(const Box& screen, const int top) {
  using namespace PiccoTokens;
  Stats s{};
  s.card = Box{screen.x + kGutter, top, PiccoLayout::maxOf(0, screen.width - 2 * kGutter), kStatsHeight};
  s.inner = inset(s.card, kCardPadX, kCardPadY);
  s.head = Box{s.inner.x, s.inner.y, s.inner.width, kStatsHeadLine};
  const int bodyTop = s.head.bottom() + kStatsBodyGap;
  s.streak = Box{s.inner.x, bodyTop, kStreakColW, kStatsBodyH};
  // The streak block (figure, gap, two small lines) is centred in the body.
  const int blockH = kFigureLine + kStreakLabelGap + 2 * kSmallLine;
  const int blockTop = bodyTop + (kStatsBodyH - blockH) / 2;
  s.figure = Box{s.inner.x, blockTop, kStreakColW, kFigureLine};
  s.streakLabel = Box{s.inner.x, s.figure.bottom() + kStreakLabelGap, kStreakColW, kSmallLine};
  s.today = Box{s.inner.x, s.streakLabel.bottom(), kStreakColW, kSmallLine};

  const int chartX = s.streak.right() + kStreakColGap;
  s.chart = Box{chartX, bodyTop, PiccoLayout::maxOf(0, s.inner.right() - chartX), kStatsBodyH};
  const int columnW = PiccoLayout::maxOf(0, (s.chart.width - (kBarCount - 1) * kBarGap) / kBarCount);
  for (int i = 0; i < kBarCount; ++i) {
    s.columns[i] = Box{chartX + i * (columnW + kBarGap), bodyTop, columnW, kStatsBodyH};
  }
  s.barBase = s.chart.bottom() - kSmallLine - kBarLabelGap;
  s.barMaxH = s.barBase - s.chart.y;

  // Totals sit on the card's inner bottom; the hairline spans the inner width.
  const int totalsH = kTotalsValueLine + kSmallLine;
  const int totalsTop = s.inner.bottom() - totalsH;
  s.totalsRule = Box{s.inner.x, totalsTop - kTotalsPadTop - kHairline, s.inner.width, kHairline};
  const int colW = s.inner.width / kTotalsCount;
  for (int i = 0; i < kTotalsCount; ++i) {
    const int colX = s.inner.x + i * colW;
    const int padL = i == 0 ? 0 : kTotalsPadX;
    const int padR = i == 0 ? kTotalsPadX : 0;
    s.totals[i] = Box{colX + padL, totalsTop, colW - padL - padR, totalsH};
    if (i > 0) s.totalsDividers[i - 1] = Box{colX, totalsTop, kHairline, totalsH};
  }
  return s;
}

constexpr std::array<Tile, kTileCount> tileLayout(const Box& screen, const int top) {
  using namespace PiccoTokens;
  std::array<Tile, kTileCount> tiles{};
  const int areaW = PiccoLayout::maxOf(0, screen.width - 2 * kGutter);
  const int areaH = PiccoLayout::maxOf(0, screen.bottom() - kGutter - top);
  const int w = (areaW - kTileGap) / 2;
  const int h = (areaH - kTileGap) / 2;
  for (int i = 0; i < kTileCount; ++i) {
    Tile& t = tiles[i];
    t.rect = Box{screen.x + kGutter + (i % 2) * (w + kTileGap), top + (i / 2) * (h + kTileGap), w, h};
    const Box inner = inset(t.rect, kCardPadX, kCardPadY);
    t.icon = Box{inner.x, inner.y, kIconTile, kIconTile};
    t.subtitle = Box{inner.x, inner.bottom() - kSmallLine, inner.width, kSmallLine};
    t.label = Box{inner.x, t.subtitle.y - kTileSubtitleGap - kTileLabelLine, inner.width, kTileLabelLine};
  }
  return tiles;
}

constexpr Home homeLayout(const Box& screen) {
  Home home{};
  home.screen = screen;
  home.header = Box{screen.x, screen.y, screen.width, PiccoTokens::kHeaderHeight};
  home.hero = heroLayout(screen);
  home.stats = statsLayout(screen, home.hero.area.bottom() + kSectionGap);
  home.tiles = tileLayout(screen, home.stats.card.bottom() + kSectionGap);
  return home;
}

// Bar height for one day: the busiest day of the week fills maxHeight; any reading at
// all shows at least 2 px so it never reads as a day off.
constexpr int barHeight(const uint32_t minutes, const uint32_t busiest, const int maxHeight) {
  if (minutes == 0 || busiest == 0 || maxHeight <= 0) return 0;
  const uint32_t clamped = minutes > busiest ? busiest : minutes;
  const int h = static_cast<int>((static_cast<uint64_t>(clamped) * maxHeight + busiest / 2) / busiest);
  return h < 2 ? 2 : h;
}

// Focus ring around an element of radius r: outer edge kFocusOutset outside it.
struct Ring {
  Box box;
  int radius;
};

constexpr Ring focusRing(const Box& element, const int radius) {
  return Ring{Box{element.x - kFocusOutset, element.y - kFocusOutset, element.width + 2 * kFocusOutset,
                  element.height + 2 * kFocusOutset},
              radius + kFocusOutset};
}

}  // namespace PiccoHomeLayout
