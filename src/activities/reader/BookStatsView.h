#pragma once

#include <string>

#include "AppCapabilities.h"
#include "BookReadingStats.h"
#include "DailyReadingStats.h"
#include "GlobalReadingStats.h"

class GfxRenderer;
class MappedInputManager;

namespace BookStatsTouchTarget {
constexpr int StartedDaysStat = 1;
constexpr int DateFieldBase = 10;
constexpr int DateFieldCount = 6;
constexpr int DateAdjustUp = 20;
constexpr int DateAdjustDown = 21;
constexpr int DateSave = 22;
constexpr int DateCancel = 23;
constexpr int SyncAll = 24;

constexpr int dateField(const int index) { return DateFieldBase + index; }
}  // namespace BookStatsTouchTarget

void renderPerBookStatsPage(GfxRenderer& renderer, const MappedInputManager* mappedInput, const std::string& bookTitle,
                            const BookReadingStats& stats, float progressPercent, bool hasEstimatedTimeLeft,
                            uint32_t estimatedTimeLeftSeconds, bool showButtonHints, bool showEditButton,
                            bool showMoreButton);

// showSyncAction adds "Sync Reading Stats": Confirm on button devices, a header icon on touch.
void renderGlobalStatsPage(GfxRenderer& renderer, const MappedInputManager* mappedInput, const char* screenTitle,
                           const GlobalReadingStats& stats, bool showButtonHints, bool showMoreButton,
                           const DailyReadingStats::Summary* daily = nullptr, bool showSyncAction = false);

void renderNoRtcCombinedStatsPage(GfxRenderer& renderer, const MappedInputManager* mappedInput,
                                  const std::string& bookTitle, const BookReadingStats& bookStats,
                                  float progressPercent, bool hasEstimatedTimeLeft, uint32_t estimatedTimeLeftSeconds,
                                  const GlobalReadingStats& deviceStats, const GlobalReadingStats* allDevicesStats,
                                  bool showButtonHints, bool showSyncAction = false);

void renderEditBookDatesPage(GfxRenderer& renderer, const MappedInputManager* mappedInput, const std::string& bookTitle,
                             const BookReadingStats& stats, int selectedField, bool showButtonHints);

#if CROSSINK_APP_PICCO_UI
#include "components/picco/PiccoLayout.h"

// Picco per-book stats (T_BookStats): title, stat strip, progress, day-of-week chart, time of
// day. Tapping the dates row registers BookStatsTouchTarget::StartedDaysStat (date editor).
void renderPiccoPerBookStatsPage(GfxRenderer& renderer, const std::string& bookTitle, const BookReadingStats& stats,
                                 float progressPercent, bool hasEstimatedTimeLeft, uint32_t estimatedTimeLeftSeconds,
                                 bool showEditTarget);

// Three figures between two hairlines with dividers (T_BookStats, T_EndOfBook).
void drawPiccoStatsStrip(const GfxRenderer& renderer, const PiccoLayout::Box& strip, const char* const* values,
                         const char* const* labels);
#endif
