#ifdef SIMULATOR
#include "PiccoScreens4R1.h"

#include <Epub.h>
#include <HalStorage.h>
#include <Logging.h>

#include <memory>

#include "AppCapabilities.h"
#include "BookmarkStore.h"
#include "ClippingStore.h"
#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "activities/reader/BookStatsActivity.h"
#include "activities/reader/EndOfBookOptions.h"
#include "activities/reader/EpubReaderChapterSelectionActivity.h"
#include "activities/reader/LookedUpWordsActivity.h"
#include "util/LookupHistory.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

namespace {
constexpr int SETTLE_TICKS = 6;
// Contents tab centres: the tab band sits 16 px under the header (headerTop ~9 here).
constexpr int kTabY = 137;
constexpr int kTabX[3] = {96, 240, 384};

std::shared_ptr<Epub> g_epub;

void captureIf(const char* activity, const std::string& name,
               const std::function<void(const std::string&)>& renderAndCapture) {
  if (activityManager.isCurrentActivityNamed(activity)) {
    renderAndCapture(name);
    return;
  }
  LOG_ERR("SMOKE", "Picco screens 4R1: %s skipped, %s is not up", name.c_str(), activity);
}

void addTap(std::vector<std::function<int()>>& steps, const int x, const int y) {
  steps.push_back([x, y] {
    mappedInputManager.simulatorInjectTouchDown(x, y);
    return 0;
  });
  steps.push_back([x, y] {
    mappedInputManager.simulatorInjectTouchRelease(x, y);
    return SETTLE_TICKS;
  });
}

BookReadingStats bookStatsFixture() {
  BookReadingStats stats;
  stats.sessionCount = 31;
  stats.totalReadingSeconds = 9u * 3600u + 20u * 60u;
  stats.totalPagesTurned = 616;
  stats.startDate = ReadingStatsDate{2026, 10, 2};
  stats.timeOfDaySeconds = {2u * 3600u, 3u * 3600u, 4u * 3600u, 20u * 60u};
  stats.dayOfWeekSeconds = {24u * 60u, 41u * 60u, 12u * 60u, 55u * 60u, 38u * 60u, 29u * 60u, 47u * 60u};
  return stats;
}
}  // namespace

void addPiccoScreens4R1(std::vector<std::function<int()>>& steps, const std::string& bookPath,
                        const std::function<void(const std::string&)>& capture,
                        const std::function<void(const std::string&)>& renderAndCapture) {
  // Contents (T_Contents): chapters, then the bookmark and clipping tabs by tap.
  steps.push_back([bookPath] {
    RenderLock lock;
    g_epub = std::make_shared<Epub>(bookPath, "/.crosspoint");
    if (!g_epub->load()) {
      LOG_ERR("SMOKE", "Picco screens 4R1: book did not load");
      g_epub.reset();
      return 0;
    }
    BOOKMARKS.loadForBook(bookPath, g_epub->getTitle(), g_epub->getAuthor(), "epub");
    BOOKMARKS.clearAll();
    BOOKMARKS.addBookmark(1, 0.12f, 20, "Chapter 1", UINT16_MAX, "It was a bright cold day in April");
    BOOKMARKS.addBookmark(2, 0.35f, 20, "Chapter 2", UINT16_MAX, "The hallway smelt of boiled cabbage");
    CLIPPINGS.loadForBook(bookPath, g_epub->getTitle(), g_epub->getAuthor(), "epub");
    if (CLIPPINGS.clippingCount() == 0) {
      CLIPPINGS.addClipping(1, 0, 0, 1, 0, 6, 7, "Chapter 1", 0, "Who controls the past controls the future.", 0, 0);
    }
    const int toc = g_epub->getTocItemsCount() > 2 ? 2 : 0;
    activityManager.replaceActivity(std::make_unique<EpubReaderChapterSelectionActivity>(
        renderer, mappedInputManager, g_epub, g_epub->getPath(), toc));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("EpubReaderChapterSelection", "4r1-contents-chapters", renderAndCapture);
    return 0;
  });
  addTap(steps, kTabX[1], kTabY);
  steps.push_back([renderAndCapture] {
    captureIf("EpubReaderBookmarkList", "4r1-contents-bookmarks", renderAndCapture);
    return 0;
  });
  addTap(steps, kTabX[2], kTabY);
  steps.push_back([renderAndCapture] {
    captureIf("EpubClippingList", "4r1-contents-clippings", renderAndCapture);
    return 0;
  });
  addTap(steps, kTabX[0], kTabY);
  steps.push_back([] {
    if (!activityManager.isCurrentActivityNamed("EpubReaderChapterSelection")) {
      LOG_ERR("SMOKE", "Picco screens 4R1: Chapters tab did not return to the chapter list");
    }
    return 0;
  });

  // Book stats (T_BookStats).
  steps.push_back([] {
    if (!g_epub) return 0;
    activityManager.replaceActivity(std::make_unique<BookStatsActivity>(
        renderer, mappedInputManager, g_epub->getTitle(), g_epub->getCachePath(), bookStatsFixture(), 62.0f, true,
        3u * 3600u + 10u * 60u, GlobalReadingStats{}));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("BookStats", "4r1-book-stats", renderAndCapture);
    return 0;
  });

  // Lookup history (T_LookupHistory).
  steps.push_back([] {
    if (!g_epub) return 0;
    const std::string cache = g_epub->getCachePath();
    LookupHistory::clear(cache);
    const struct {
      const char* word;
      LookupHistory::Status status;
    } words[] = {{"benefice", LookupHistory::Status::Direct},    {"importunate", LookupHistory::Status::Direct},
                 {"sedulous", LookupHistory::Status::Stem},      {"provincial", LookupHistory::Status::Direct},
                 {"rectitude", LookupHistory::Status::NotFound}, {"ardent", LookupHistory::Status::Suggestion},
                 {"hedge", LookupHistory::Status::Direct}};
    for (const auto& w : words) LookupHistory::addWord(cache, w.word, w.status);
    activityManager.replaceActivity(std::make_unique<LookedUpWordsActivity>(renderer, mappedInputManager, cache));
    return SETTLE_TICKS;
  });
  steps.push_back([renderAndCapture] {
    captureIf("LookedUpWords", "4r1-lookup-history", renderAndCapture);
    return 0;
  });

  // End of book (T_EndOfBook): drawn the way the reader draws it, then captured as is.
  steps.push_back([bookPath, capture] {
    if (!g_epub) return 0;
    RenderLock lock;
    BookReadingStats stats = bookStatsFixture();
    stats.isCompleted = true;
    stats.finishedDate = ReadingStatsDate{2026, 10, 25};
    // Two placeholder siblings so "Continue with" has rows; removed after the capture.
    const std::string folder = bookPath.substr(0, bookPath.rfind('/') + 1);
    const std::string next[2] = {folder + "zz-claude-test-next-a.epub", folder + "zz-claude-test-next-b.epub"};
    for (const auto& path : next) {
      HalFile file;
      if (Storage.openFileForWrite("SMOKE", path, file)) file.close();
    }
    EndOfBookOptions endOfBook(renderer);
    endOfBook.loadOnce(bookPath);
    endOfBook.setFinishedBook(g_epub->getTitle(), g_epub->getAuthor(), stats);
    renderer.clearScreen();
    endOfBook.render(renderer, mappedInputManager);
    capture("4r1-end-of-book");
    for (const auto& path : next) Storage.remove(path.c_str());
    g_epub.reset();
    return 0;
  });
}

#else
void addPiccoScreens4R1(std::vector<std::function<int()>>&, const std::string&,
                        const std::function<void(const std::string&)>&,
                        const std::function<void(const std::string&)>&) {}
#endif
#endif
