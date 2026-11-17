#include "PiccoStatsPage.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>

#include "MappedInputManager.h"
#include "PiccoLibraryLayout.h"
#include "PiccoListUi.h"
#include "PiccoSystemLayout.h"
#include "activities/reader/BookReadingStats.h"
#include "activities/reader/BookStatsView.h"
#include "activities/reader/ReadingDailyMinutes.h"
#include "components/TouchHeaderBackButton.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/icons/syncIcons.h"
#include "fontIds.h"

namespace PiccoStatsPage {
namespace {
using PiccoLayout::Box;
using namespace PiccoStatsLayout;

constexpr int kSmall = SMALL_FONT_ID;
constexpr int kBody = UI_10_FONT_ID;
constexpr int kFigure = FIGURES_27_FONT_ID;
constexpr int kFigureUnitGap = 8;
constexpr int kBarRadius = 3;

constexpr std::array<StrId, READING_DAY_OF_WEEK_COUNT> kDayLabels = {
    StrId::STR_STATS_MON, StrId::STR_STATS_TUE, StrId::STR_STATS_WED, StrId::STR_STATS_THU,
    StrId::STR_STATS_FRI, StrId::STR_STATS_SAT, StrId::STR_STATS_SUN};
constexpr std::array<StrId, READING_TIME_BUCKET_COUNT> kDayPartLabels = {
    StrId::STR_STATS_MORNING, StrId::STR_STATS_AFTERNOON, StrId::STR_STATS_EVENING, StrId::STR_STATS_NIGHT};

enum class Align : uint8_t { Left, Center, Right };

void drawIn(const GfxRenderer& renderer, const int font, const Box& box, const char* text,
            const EpdFontFamily::Style style = EpdFontFamily::REGULAR, const Align align = Align::Left) {
  if (text == nullptr || text[0] == '\0' || box.width <= 0) return;
  const std::string visible = renderer.truncatedText(font, text, box.width, style);
  const int w = renderer.getTextWidth(font, visible.c_str(), style);
  int x = box.x;
  if (align == Align::Center) x += (box.width - w) / 2;
  if (align == Align::Right) x = box.right() - w;
  renderer.drawText(font, x, box.y + (box.height - renderer.getLineHeight(font)) / 2, visible.c_str(), true, style);
}

void drawSectionLabel(const GfxRenderer& renderer, const Box& box, const char* text) {
  char caps[64];
  size_t i = 0;
  for (; text != nullptr && text[i] != '\0' && i + 1 < sizeof(caps); ++i) {
    caps[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
  }
  caps[i] = '\0';
  drawIn(renderer, kSmall, box, caps, EpdFontFamily::BOLD);
}

// Minutes as the chart labels them: "47" up to 99, whole hours above ("12h").
void formatChartMinutes(const uint32_t minutes, char* buf, const size_t len) {
  if (minutes < 100) {
    snprintf(buf, len, "%lu", static_cast<unsigned long>(minutes));
  } else {
    snprintf(buf, len, "%luh", static_cast<unsigned long>((minutes + 30) / 60));
  }
}

void formatDailyDuration(const uint32_t seconds, char* buf, const size_t len) {
  if (seconds == 0) {
    snprintf(buf, len, "0 min");
    return;
  }
  BookReadingStats::formatDuration(seconds, buf, len);
}

void drawHeader(GfxRenderer& renderer, const Content& content) {
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  TouchHeaderBackButton::draw(renderer, header, tr(STR_READING_STATS), false,
                              content.showSync ? TouchHeaderBackButton::TRAILING_ACTION_RESERVE : 0);
  if (content.showSync) {
    TouchHeaderBackButton::drawTrailingIcon(renderer, header, icon_cloud_upload_32);
    TouchRegistry::getInstance().add(TouchHeaderBackButton::trailingTouchRect(header), BookStatsTouchTarget::SyncAll,
                                     TouchRegistry::Item);
  }
}

void drawStreak(const GfxRenderer& renderer, const Box& area, const Content& content) {
  const GlobalReadingStats& stats = *content.stats;
  ReadingStatsDateTime today;
  const bool hasToday = getCurrentLocalReadingStatsDateTime(today);
  const uint16_t streak = hasToday ? stats.currentReadingStreak(&today.date) : 0;

  char figure[8];
  snprintf(figure, sizeof(figure), "%u", static_cast<unsigned>(streak));
  const int figureH = renderer.getLineHeight(kFigure);
  const int smallH = renderer.getLineHeight(kSmall);
  const int blockTop = area.y + (area.height - figureH - 4 - smallH) / 2;
  renderer.drawText(kFigure, area.x, blockTop, figure);
  const int baseline = blockTop + renderer.getFontAscenderSize(kFigure);
  const int unitX = area.x + renderer.getTextWidth(kFigure, figure) + kFigureUnitGap;
  renderer.drawText(kBody, unitX, baseline - renderer.getFontAscenderSize(kBody),
                    streak == 1 ? tr(STR_HOME_DAY) : tr(STR_HOME_DAYS), true, EpdFontFamily::BOLD);
  renderer.drawText(kSmall, area.x, blockTop + figureH + 4, tr(STR_PICCO_READING_STREAK));

  // Today and the 7-day average: this device's daily counters only.
  const DailyReadingStats::Summary* daily = content.allDevices ? nullptr : content.daily;
  if (daily == nullptr) return;
  char todayText[24];
  char week[24];
  snprintf(todayText, sizeof(todayText), "-");
  snprintf(week, sizeof(week), "-");
  if (daily->hasToday) formatDailyDuration(daily->todaySeconds, todayText, sizeof(todayText));
  if (daily->hasSevenDayAverage) formatDailyDuration(daily->sevenDayAverageSeconds, week, sizeof(week));
  const char* labels[2] = {tr(STR_STATS_TODAY_LBL), tr(STR_STATS_SEVEN_DAY_AVG_LBL)};
  const char* values[2] = {todayText, week};
  const int labelW = std::max(renderer.getTextWidth(kSmall, labels[0]), renderer.getTextWidth(kSmall, labels[1]));
  const int rowH = renderer.getLineHeight(kBody) + 6;
  const int top = area.y + (area.height - 2 * rowH) / 2;
  const int labelX = area.right() - labelW;
  for (int i = 0; i < 2; ++i) {
    const Box row{area.x + area.width / 2, top + i * rowH, labelX - 12 - (area.x + area.width / 2), rowH};
    drawIn(renderer, kBody, row, values[i], EpdFontFamily::BOLD, Align::Right);
    drawIn(renderer, kSmall, Box{labelX, row.y, labelW, rowH}, labels[i]);
  }
}

// Bars over 7 columns; values above, labels under. `outlined` marks today's bar.
void drawBars(const GfxRenderer& renderer, const Box& chart, const std::array<uint32_t, 7>& minutes,
              const std::array<const char*, 7>& labels, const int outlined) {
  uint32_t busiest = 0;
  for (const uint32_t m : minutes) busiest = std::max(busiest, m);
  const int lineH = kLabelLine;
  const int base = chart.bottom() - lineH - kChartTextGap;
  for (int i = 0; i < kWeekColumns; ++i) {
    const Box col = weekColumn(chart, i);
    const bool isOutlined = i == outlined;
    const int h = barHeight(minutes[static_cast<size_t>(i)], busiest, kWeekBarMax);
    if (h > 0) {
      const int top = base - h;
      if (!isOutlined && h > 2) renderer.fillRectDither(col.x + 1, top + 1, col.width - 2, h - 1, Color::DarkGray);
      renderer.drawRoundedRect(col.x, top, col.width, h, PiccoTokens::kHairline, std::min(kBarRadius, h / 2), true,
                               true, false, false, true);
    } else {
      renderer.fillRect(col.x, base - 1, col.width, 1, true);
    }
    char value[12];
    formatChartMinutes(minutes[static_cast<size_t>(i)], value, sizeof(value));
    drawIn(renderer, kSmall, Box{col.x, base - h - kChartTextGap - lineH, col.width, lineH}, value,
           EpdFontFamily::REGULAR, Align::Center);
    drawIn(renderer, kSmall, Box{col.x, base + kChartTextGap, col.width, lineH}, labels[static_cast<size_t>(i)],
           isOutlined ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, Align::Center);
  }
}

void drawWeek(const GfxRenderer& renderer, const Layout& l, const Content& content) {
  std::array<uint32_t, 7> minutes{};
  std::array<const char*, 7> labels{};
  ReadingStatsDateTime today;
  // This week from this device's minutes per day (what Home's card shows); without a
  // date, or for all devices, the all-time spread over the weekdays.
  if (!content.allDevices && getCurrentLocalReadingStatsDateTimeAnyClock(today)) {
    drawSectionLabel(renderer, l.weekLabel, tr(STR_HOME_THIS_WEEK));
    const uint32_t todayIndex = readingStatsDayIndex(today.date);
    const int todayOfWeek = readingStatsDayOfWeekIndex(today.date);
    ReadingDailyMinutes::Reader daily;
    for (int i = 0; i < kWeekColumns; ++i) {
      minutes[static_cast<size_t>(i)] = daily.minutes(todayIndex - static_cast<uint32_t>(kWeekColumns - 1 - i));
      labels[static_cast<size_t>(i)] = I18N.get(kDayLabels[static_cast<size_t>((todayOfWeek + 1 + i) % 7)]);
    }
    drawBars(renderer, l.weekChart, minutes, labels, kWeekColumns - 1);
    return;
  }
  drawSectionLabel(renderer, l.weekLabel, tr(STR_STATS_DAY_OF_WEEK));
  for (int i = 0; i < kWeekColumns; ++i) {
    minutes[static_cast<size_t>(i)] = content.stats->dayOfWeekSeconds[static_cast<size_t>(i)] / 60;
    labels[static_cast<size_t>(i)] = I18N.get(kDayLabels[static_cast<size_t>(i)]);
  }
  drawBars(renderer, l.weekChart, minutes, labels, -1);
}

void drawTotals(const GfxRenderer& renderer, const Layout& l, const GlobalReadingStats& stats) {
  drawSectionLabel(renderer, l.totalsLabel, tr(STR_PICCO_ALL_TIME));
  const Box& grid = l.totalsGrid;
  renderer.fillRect(grid.x, grid.y, grid.width, PiccoTokens::kHairline, true);
  for (int row = 1; row <= kTotalsRows; ++row) {
    renderer.fillRect(grid.x, grid.y + row * kTotalsCell - 1, grid.width, PiccoTokens::kHairline, true);
  }
  char values[6][24];
  BookReadingStats::formatDuration(stats.totalReadingSeconds, values[0], sizeof(values[0]));
  if (stats.completedBooks > 0) {
    snprintf(values[1], sizeof(values[1]), "%lu", static_cast<unsigned long>(stats.completedBooks));
  } else {
    snprintf(values[1], sizeof(values[1]), "-");
  }
  snprintf(values[2], sizeof(values[2]), "%lu", static_cast<unsigned long>(stats.totalSessions));
  BookReadingStats::formatDuration(stats.totalSessions > 0 ? stats.totalReadingSeconds / stats.totalSessions : 0,
                                   values[3], sizeof(values[3]));
  const float pagesPerMinute = stats.totalReadingSeconds > 0
                                   ? static_cast<float>(stats.totalPagesTurned) * 60.0f / stats.totalReadingSeconds
                                   : 0.0f;
  snprintf(values[4], sizeof(values[4]), "%.1f", pagesPerMinute);
  unsigned daysRead = 0;
  for (const uint8_t bits : stats.readingHistoryBits) daysRead += static_cast<unsigned>(__builtin_popcount(bits));
  snprintf(values[5], sizeof(values[5]), "%u", daysRead);
  const char* labels[6] = {tr(STR_STATS_TIME_LBL),        tr(STR_STATS_COMPLETED_LBL), tr(STR_STATS_SESSIONS_LBL),
                           tr(STR_STATS_AVG_SESSION_LBL), tr(STR_STATS_PAGES_PER_MIN), tr(STR_HOME_DAYS_READ)};
  const int valueH = renderer.getLineHeight(kBody);
  const int labelH = renderer.getLineHeight(kSmall);
  for (int i = 0; i < kTotalsColumns * kTotalsRows; ++i) {
    const Box cell = totalsCell(grid, i);
    const int top = cell.y + (cell.height - valueH - labelH) / 2;
    const Box text{cell.x, top, cell.width - PiccoTokens::kNeighbourGap, valueH};
    drawIn(renderer, kBody, text, values[i], EpdFontFamily::BOLD);
    drawIn(renderer, kSmall, Box{text.x, top + valueH, text.width, labelH}, labels[i]);
  }
}

void drawDayParts(const GfxRenderer& renderer, const Layout& l, const GlobalReadingStats& stats) {
  drawSectionLabel(renderer, l.dayLabel, tr(STR_STATS_TIME_OF_DAY));
  uint32_t total = 0;
  for (const uint32_t s : stats.timeOfDaySeconds) total += s;
  const int day = busiestIndex(stats.dayOfWeekSeconds);
  const int part = busiestIndex(stats.timeOfDaySeconds);
  if (day >= 0 && part >= 0) {
    char most[64];
    char detail[48];
    snprintf(detail, sizeof(detail), "%s · %s", I18N.get(kDayLabels[static_cast<size_t>(day)]),
             I18N.get(kDayPartLabels[static_cast<size_t>(part)]));
    snprintf(most, sizeof(most), tr(STR_PICCO_MOST_ON_FORMAT), detail);
    const int labelW = renderer.getTextWidth(kSmall, tr(STR_STATS_TIME_OF_DAY), EpdFontFamily::BOLD);
    const Box right{l.dayLabel.x + labelW + PiccoTokens::kGroupGap, l.dayLabel.y,
                    l.dayLabel.width - labelW - PiccoTokens::kGroupGap, l.dayLabel.height};
    drawIn(renderer, kSmall, right, most, EpdFontFamily::REGULAR, Align::Right);
  }
  for (int i = 0; i < kDayParts; ++i) {
    const Box col = dayPart(l.dayParts, i);
    drawIn(renderer, kSmall, Box{col.x, col.y, col.width, kLabelLine},
           I18N.get(kDayPartLabels[static_cast<size_t>(i)]));
    const int barY = col.y + kLabelLine + kChartTextGap;
    renderer.fillRect(col.x, barY + kDayPartBar / 2 - 1, col.width, PiccoTokens::kHairline, true);
    const int pct = sharePercent(stats.timeOfDaySeconds[static_cast<size_t>(i)], total);
    const int w = pct > 0 ? std::max(2, col.width * pct / 100) : 0;
    if (w > 0) {
      renderer.fillRect(col.x, barY, w, kDayPartBar, false);
      if (w > 2) renderer.fillRectDither(col.x + 1, barY + 1, w - 2, kDayPartBar - 2, Color::DarkGray);
      renderer.drawRoundedRect(col.x, barY, w, kDayPartBar, PiccoTokens::kHairline, std::min(kBarRadius, w / 2), false,
                               true, false, true, true);
    }
    char text[8];
    snprintf(text, sizeof(text), "%d%%", pct);
    drawIn(renderer, kSmall, Box{col.x, barY + kDayPartBar + kChartTextGap, col.width, kLabelLine}, text);
  }
}
}  // namespace

void render(GfxRenderer& renderer, const MappedInputManager& mappedInput, const Content& content) {
  (void)mappedInput;
  renderer.clearScreen();
  if (content.stats == nullptr) return;
  drawHeader(renderer, content);
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  const Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const Layout l = layout(screen, header.y + header.height, content.showTabs);
  if (content.showTabs) {
    const char* labels[2] = {tr(STR_STATS_THIS_DEVICE_SCREEN), tr(STR_STATS_ALL_DEVICES_SCREEN)};
    const Rect tabs = PiccoListUi::toRect(l.tabs);
    PiccoListUi::drawSegmented(renderer, tabs, labels, 2, content.allDevices ? 1 : 0);
    for (int i = 0; i < 2; ++i) {
      const Box cell = PiccoLibraryLayout::segmentCell(l.tabs, 2, i);
      TouchRegistry::getInstance().add(PiccoListUi::toRect(cell), i == 0 ? kTabThisDevice : kTabAllDevices,
                                       TouchRegistry::Tab);
    }
  }
  drawStreak(renderer, l.streak, content);
  drawWeek(renderer, l, content);
  drawTotals(renderer, l, *content.stats);
  drawDayParts(renderer, l, *content.stats);
}

}  // namespace PiccoStatsPage

#endif
