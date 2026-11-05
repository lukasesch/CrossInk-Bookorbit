#pragma once

#include <cstdint>
#include <memory>
#include <string>

class Activity;

enum class FrontlightPanelAction : uint8_t {
  None = 0,
  SyncProgress,
  NearbyPositionSync,
  SendNearbyBook,
  // This fork's own provider, appended after upstream's actions.
  BookOrbitSync,
};

enum class FrontlightBookSource : uint8_t { CurrentBook = 0, LastBook, DeviceOnly };

constexpr FrontlightBookSource chooseFrontlightBookSource(const bool activeEpub, const bool currentBookValid,
                                                          const bool lastBookValid) {
  if (activeEpub && currentBookValid) return FrontlightBookSource::CurrentBook;
  if (lastBookValid) return FrontlightBookSource::LastBook;
  return FrontlightBookSource::DeviceOnly;
}

constexpr bool hasFrontlightActiveReaderBook(const bool isReaderActivity, const bool currentBookValid) {
  return isReaderActivity && currentBookValid;
}

constexpr bool shouldShowStickyReaderDetails(const bool hasStickyReaderDetailsPanel, const bool hasFrontlight,
                                             const bool activeReaderBook) {
  return hasStickyReaderDetailsPanel && !hasFrontlight && activeReaderBook;
}

constexpr bool supportsFrontlightDrawer(const bool hasTouchHardware, const bool hasFrontlight,
                                        const bool hasReaderDetailsPanel = false) {
  return hasTouchHardware && (hasFrontlight || hasReaderDetailsPanel);
}

// Touch boards without a home key open the reader menu with a down-swipe. Where a
// frontlight is fitted (Picco), the down-swipe that starts at the top edge opens the
// light panel instead, as the X4 Pro's top edge does. The Sticky keeps its own scheme.
constexpr bool readerTopEdgeOpensLightPanel(const bool hasTouchHardware, const bool hasHomeKey,
                                            const bool hasFrontlight, const bool stickyScheme) {
  return hasTouchHardware && !hasHomeKey && hasFrontlight && !stickyScheme;
}

struct FrontlightPanelBookDetails {
  std::string title;
  std::string author;
  std::string chapter;
  uint32_t chapterPage = 0;
  uint32_t chapterPageCount = 0;
  bool chapterPageCountEstimated = false;
  int progressPercent = 0;
};

struct FrontlightDrawerState {
  int8_t selectedAction = 0;
  bool syncDialogOpen = false;
};

struct FrontlightPanelResult {
  FrontlightPanelAction action = FrontlightPanelAction::None;
  FrontlightDrawerState state{};
  bool activeEpub = false;
  bool ttfRenderingChanged = false;
  std::string bookPath;
};

struct FrontlightPanelContext {
  Activity* sourceActivity = nullptr;
  // An open reader of any supported format. This controls reader header chrome
  // and Home navigation; activeEpub identifies the reader save/handoff path.
  bool activeReaderBook = false;
  bool activeEpub = false;
  bool showReaderDetails = false;
  bool showReadingStatsAction = true;
  std::string bookTitle;
  std::string bookPath;
  FrontlightPanelBookDetails bookDetails;
  std::unique_ptr<Activity> readingStatsActivity;
  FrontlightDrawerState drawerState{};
};
