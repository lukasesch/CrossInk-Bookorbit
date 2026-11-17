#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "activities/reader/DailyReadingStats.h"
#include "activities/reader/GlobalReadingStats.h"

class GfxRenderer;
class MappedInputManager;

// The global reading stats screen (T_Stats): device tabs, streak, this week, all-time
// totals, time of day. BookStatsActivity keeps paging, sync and the per-book pages.
namespace PiccoStatsPage {

// TouchRegistry Tab ids of the device tabs.
constexpr int kTabThisDevice = 0;
constexpr int kTabAllDevices = 1;

struct Content {
  const GlobalReadingStats* stats = nullptr;
  const DailyReadingStats::Summary* daily = nullptr;  // This device only
  bool allDevices = false;                            // the selected tab
  bool showTabs = false;                              // synced stats exist
  bool showSync = false;                              // header upload action (BookStatsTouchTarget::SyncAll)
};

void render(GfxRenderer& renderer, const MappedInputManager& mappedInput, const Content& content);

}  // namespace PiccoStatsPage

#endif
