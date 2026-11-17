// Picco look for the Library (T_Library, T_LibrarySort boards). Behaviour (index, sort,
// search, filters, book actions) stays in LibraryActivity.cpp; this file only lays out
// and draws the Picco screens and maps their controls onto the same actions.
#include "LibraryActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <Bitmap.h>
#include <FsHelpers.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <cstdio>

#include "BookmarkStore.h"
#include "ClippingStore.h"
#include "activities/ActivityManager.h"
#include "activities/home/RecentBookProgress.h"
#include "activities/home/SavedItemsHomeActivity.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/libraryIcons.h"
#include "components/icons/listIcons.h"
#include "components/picco/PiccoLibraryLayout.h"
#include "components/picco/PiccoListUi.h"
#include "components/picco/PiccoPageIndicator.h"
#include "components/picco/PiccoProgress.h"
#include "fontIds.h"

extern ActivityManager activityManager;

namespace fui = freeink::ui;
using PiccoLayout::Box;
using PiccoListUi::toBox;
using PiccoListUi::toRect;

namespace {
constexpr fui::ActionId ACTION_ROW = 1;
constexpr fui::ActionId ACTION_CONTROL = 2;
constexpr fui::ActionId ACTION_PICCO_SORT = 4;
constexpr int16_t SORT_ORDER_BASE = 10;
constexpr int16_t SORT_MENU = 20;
constexpr int CONTROL_VIEW = 0;
constexpr int CONTROL_SEARCH = 1;
constexpr int CONTROL_SAVED = 2;
constexpr int CONTROL_SORT = 3;
constexpr int CONTROL_ORDER = 4;
constexpr int SORT_OPTION_COUNT = 7;
constexpr int SORT_FOCUS_COUNT = SORT_OPTION_COUNT + 2;
constexpr int kSmallFont = SMALL_FONT_ID;
constexpr int kBodyFont = UI_10_FONT_ID;
constexpr int kBookFont = LITERATA_10_FONT_ID;  // grid cover text and titles (board: Literata)

Box screenBox(const GfxRenderer& renderer) { return Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()}; }

// The shared header source: panel insets and calibration move its top.
Rect headerRect(const GfxRenderer& renderer) {
  return UITheme::getHeaderRect(renderer, UITheme::getInstance().getMetrics().headerHeight);
}

PiccoLibraryLayout::LibraryBands bands(const GfxRenderer& renderer) {
  const Rect header = headerRect(renderer);
  return PiccoLibraryLayout::libraryBands(screenBox(renderer), header.y + header.height);
}
Box tabsBox(const GfxRenderer& renderer) { return bands(renderer).tabs; }
Box sortBand(const GfxRenderer& renderer) { return bands(renderer).sort; }
Box bodyBox(const GfxRenderer& renderer) { return bands(renderer).body; }
Box viewToggleBox(const GfxRenderer& renderer) {
  const Box band = sortBand(renderer);
  const int width = PiccoLibraryLayout::segmentedWidth(2, PiccoLibraryLayout::kViewToggleCell);
  return Box{band.right() - width, band.y, width, band.height};
}

// Board order (T_LibrarySort) -> the Library's sort values.
constexpr uint8_t kSortForOption[SORT_OPTION_COUNT] = {
    4 /*RecentlyRead*/, 0 /*DateAdded*/, 1 /*Title*/, 2 /*AuthorLast*/, 3 /*AuthorFirst*/, 5 /*Series*/, 6 /*Genre*/};
constexpr StrId kSortLabels[SORT_OPTION_COUNT] = {StrId::STR_LIBRARY_RECENTLY_OPENED,
                                                  StrId::STR_LIBRARY_DATE_ADDED,
                                                  StrId::STR_TITLE,
                                                  StrId::STR_LIBRARY_AUTHOR_LAST_NAME,
                                                  StrId::STR_LIBRARY_AUTHOR_FIRST_NAME,
                                                  StrId::STR_LIBRARY_SERIES,
                                                  StrId::STR_LIBRARY_GENRE};

int optionForSort(const uint8_t sort) {
  for (int i = 0; i < SORT_OPTION_COUNT; ++i) {
    if (kSortForOption[i] == sort) return i;
  }
  return 0;
}

const RecentBook* recentForPath(const std::string& path) {
  const auto& books = RECENT_BOOKS.getBooks();
  const auto it = std::find_if(books.begin(), books.end(), [&path](const RecentBook& b) { return b.path == path; });
  return it == books.end() ? nullptr : &*it;
}

template <typename Screen>
void hit(Screen& screen, const Box& box, const fui::ActionId action, const int value,
         const uint16_t mask = fui::InputTouch) {
  screen.frame().hit(fui::Rect{static_cast<int16_t>(box.x), static_cast<int16_t>(box.y),
                               static_cast<int16_t>(box.width), static_cast<int16_t>(box.height)},
                     action, static_cast<int16_t>(value), mask);
}
}  // namespace

void LibraryActivity::enterPicco() {
  sortScreenOpen = false;
  sortFocus = -1;
  gridPercentPage = -1;
  app.on(ACTION_PICCO_SORT, &LibraryActivity::onPiccoSortEvent, this);

  // Tab counts: the whole library (index header only) and every saved bookmark and clipping.
  libraryBookCount = -1;
  if (auto counter = makeUniqueNoThrow<library::LibraryIndexFile>(false)) {
    if (counter->open(library::libraryIndexPath())) libraryBookCount = counter->bookCount();
    counter->close();
  }
  savedCount = 0;
  std::vector<BookmarkedBookEntry> bookmarked;
  if (BookmarkStore::getAllBookmarkedBooks(bookmarked)) {
    for (const auto& entry : bookmarked) savedCount += entry.count;
  }
  std::vector<ClippedBookEntry> clipped;
  if (ClippingStore::getAllClippedBooks(clipped)) {
    for (const auto& entry : clipped) savedCount += entry.count;
  }
}

int LibraryActivity::piccoGridPageSize() const {
  return std::min(PICCO_GRID_MAX, PiccoLibraryLayout::gridPageSize(bodyBox(renderer).height));
}

const char* LibraryActivity::orderLabel(const bool forDescending) const {
  const bool byDate = sort == Sort::DateAdded || sort == Sort::RecentlyRead;
  if (byDate) return forDescending ? tr(STR_PICCO_NEWEST_FIRST) : tr(STR_PICCO_OLDEST_FIRST);
  return forDescending ? tr(STR_PICCO_Z_TO_A) : tr(STR_PICCO_A_TO_Z);
}

bool LibraryActivity::activatePiccoControl(const int control) {
  if (!UITheme::isPicco()) return false;
  switch (control) {
    case CONTROL_VIEW:
      SETTINGS.recentBooksView = SETTINGS.recentBooksView == CrossPointSettings::RECENT_BOOKS_GRID
                                     ? CrossPointSettings::RECENT_BOOKS_LIST
                                     : CrossPointSettings::RECENT_BOOKS_GRID;
      if (!SETTINGS.saveToFile()) LOG_ERR("LIB", "Cannot save Library view");
      resetViewport();
      if (mappedInput.hasTouchHardware()) selection = CONTROL_VIEW;
      requestUpdate();
      return true;
    case CONTROL_SAVED:
      openSavedTab();
      return true;
    case CONTROL_SORT:
      sortScreenOpen = true;
      sortFocus = -1;
      uiReady = false;
      requestUpdate();
      return true;
    default:
      return false;  // search and order keep their shared actions
  }
}

void LibraryActivity::openSavedTab() {
  // The count the Books tab shows, so the Saved tab can label it the same way.
  const int books = libraryBookCount >= 0 ? libraryBookCount : rowCount();
  auto saved = makeUniqueNoThrow<SavedItemsHomeActivity>(renderer, mappedInput, books);
  if (!saved) {
    LOG_ERR("LIB", "Cannot allocate Saved tab");
    return;
  }
  index.close();
  activityManager.replaceActivity(std::move(saved));
}

void LibraryActivity::selectSortOption(const int option) {
  if (option < 0 || option >= SORT_OPTION_COUNT) return;
  const auto value = static_cast<Sort>(kSortForOption[option]);
  // Series and genre need index format 4, as in the sort popup.
  if ((value == Sort::Series || value == Sort::Genre) && index.isOpen() && index.header().formatVersion < 4) return;
  sort = value;
  descending = sort == Sort::DateAdded || sort == Sort::RecentlyRead;
  SETTINGS.librarySortMethod = static_cast<uint8_t>(sort);
  SETTINGS.librarySortDescending = descending;
  if (!SETTINGS.saveToFile()) LOG_ERR("LIB", "Cannot save Library sort");
  refreshIndexIfNeeded(true);
  resetViewport();
  requestUpdate();
}

void LibraryActivity::selectOrder(const int cell) {
  // Date sorts list newest first in the left cell, text sorts A-Z.
  const bool byDate = sort == Sort::DateAdded || sort == Sort::RecentlyRead;
  const bool wantDescending = byDate ? cell == 0 : cell == 1;
  if (wantDescending != descending) activateControl(CONTROL_ORDER);
  requestUpdate();
}

void LibraryActivity::onPiccoSortEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<LibraryActivity*>(user);
  self->app.clearTapFlash();
  if (event.value == SORT_MENU) {
    self->openMenu();
  } else if (event.value >= SORT_ORDER_BASE) {
    self->selectOrder(event.value - SORT_ORDER_BASE);
  } else {
    self->selectSortOption(event.value);
  }
}

bool LibraryActivity::handlePiccoSortInput(const LibraryInputBuffer::Event& input) {
  using Type = LibraryInputBuffer::Type;
  const auto close = [this] {
    pendingInput.clear();
    sortScreenOpen = false;
    uiReady = false;
    requestUpdate();
  };
  if (input.type == Type::BackRelease) {
    close();
    return true;
  }
  if (input.type == Type::TouchRelease && input.x >= 0 && input.y >= 0) {
    const auto header = headerRect(renderer);
    const auto back = TouchHeaderBackButton::layout(header).touchRect;
    if (input.y < header.y + header.height && input.x >= back.x && input.x < back.x + back.width) {
      close();
      return true;
    }
  }
  const auto move = [this](const int next) {
    sortFocus = next;
    requestUpdate();
  };
  switch (input.type) {
    case Type::Next:
      move(sortFocus < 0 ? 0 : (sortFocus + 1) % SORT_FOCUS_COUNT);
      return true;
    case Type::Previous:
      move(sortFocus <= 0 ? SORT_FOCUS_COUNT - 1 : sortFocus - 1);
      return true;
    case Type::ConfirmRelease:
      pendingInput.clear();
      if (sortFocus >= SORT_OPTION_COUNT) {
        selectOrder(sortFocus - SORT_OPTION_COUNT);
      } else if (sortFocus >= 0) {
        selectSortOption(sortFocus);
      }
      return true;
    case Type::SwipeUp:
    case Type::SwipeDown:
    case Type::SwipeLeft:
    case Type::SwipeRight:
    case Type::NextPage:
    case Type::PreviousPage:
      return true;
    default:
      return false;  // touches route through the app's hits
  }
}

void LibraryActivity::loadPiccoGridPercents() {
  if (!gridEnabled() || gridPercentPage == gridPageStart) return;
  const int pageSize = piccoGridPageSize();
  RecentBook book;
  for (int slot = 0; slot < pageSize; ++slot) {
    const int row = gridPageStart + slot;
    gridPagePercent[slot] = -1.0f;
    if (row >= rowCount() || !readBook(row, book)) continue;
    gridPagePercent[slot] = FsHelpers::hasEpubExtension(book.path) ? RecentBookProgress::loadCachedEpubPercent(book)
                                                                   : RecentBookProgress::loadPercent(book);
  }
  gridPercentPage = gridPageStart;
  requestUpdate();
}

void LibraryActivity::buildPiccoScreen(UiApp::ScreenType& screen) {
  if (sortScreenOpen) {
    buildPiccoSortScreen(screen);
    return;
  }
  const bool focusShown = showSelection;
  const auto header = headerRect(renderer);

  // Header contextual action: search.
  const Box search = toBox(TouchHeaderBackButton::trailingTouchRect(header));
  if (mappedInput.hasTouchHardware()) hit(screen, search, ACTION_CONTROL, CONTROL_SEARCH);
  if (focusShown && selection == CONTROL_SEARCH) PiccoListUi::drawFocusFrame(renderer, toRect(search), 12);

  // Tabs: Books · n | Saved · m.
  const Box tabs = tabsBox(renderer);
  char booksLabel[32];
  char savedLabel[32];
  const unsigned books = libraryBookCount >= 0 ? static_cast<unsigned>(libraryBookCount) : rowCount();
  std::snprintf(booksLabel, sizeof(booksLabel), tr(STR_PICCO_BOOKS_TAB_FORMAT), books);
  std::snprintf(savedLabel, sizeof(savedLabel), tr(STR_PICCO_SAVED_TAB_FORMAT), static_cast<unsigned>(savedCount));
  const char* tabLabels[] = {booksLabel, savedLabel};
  PiccoListUi::drawSegmented(renderer, toRect(tabs), tabLabels, 2, 0);
  const Box savedCell = PiccoLibraryLayout::segmentCell(tabs, 2, 1);
  if (mappedInput.hasTouchHardware()) hit(screen, savedCell, ACTION_CONTROL, CONTROL_SAVED);
  if (focusShown && selection == CONTROL_SAVED) PiccoListUi::drawFocusFrame(renderer, toRect(savedCell), 12);

  // Sort row: dropdown, order (or the search query / a warning), view toggle.
  const Box band = sortBand(renderer);
  const int centerY = band.y + band.height / 2;
  const Box viewToggle = viewToggleBox(renderer);
  const char* middle = orderLabel(descending);
  bool middleIsOrder = true;
  std::string quoted;
  if (filterFailed || scanFailed || index.ranksDegraded()) {
    middle = filterFailed ? tr(STR_LIBRARY_SEARCH_FAILED)
             : scanFailed ? I18n::getInstance().get(scanFailureText)
                          : tr(STR_LIBRARY_UNSORTED);
    middleIsOrder = false;
  } else if (!query.empty()) {
    quoted.assign("\"").append(query).append("\"");
    middle = quoted.c_str();
    middleIsOrder = false;
  }
  // The order text keeps its width (up to a third of the row); the dropdown takes the rest.
  const int middleNeed = std::min(renderer.getTextWidth(kSmallFont, middle), (viewToggle.x - band.x) / 3);
  const int maxSortWidth = viewToggle.x - band.x - middleNeed - 2 * PiccoTokens::kControlClearance;
  Rect sortButton = PiccoListUi::outlineButtonRect(renderer, band.x, centerY, sortLabel(), &icon_chevron_down_24);
  sortButton.width = std::min(sortButton.width, maxSortWidth);
  PiccoListUi::drawOutlineButton(renderer, sortButton, sortLabel(), &icon_chevron_down_24,
                                 focusShown && selection == CONTROL_SORT);
  if (mappedInput.hasTouchHardware()) hit(screen, toBox(sortButton), ACTION_CONTROL, CONTROL_SORT);
  const int middleLeft = sortButton.x + sortButton.width + PiccoTokens::kControlClearance;
  const int middleRight = viewToggle.x - PiccoTokens::kControlClearance;
  const std::string visible = renderer.truncatedText(kSmallFont, middle, std::max(0, middleRight - middleLeft));
  const int middleW = renderer.getTextWidth(kSmallFont, visible.c_str());
  const int middleX = middleLeft + std::max(0, (middleRight - middleLeft - middleW) / 2);
  renderer.drawText(kSmallFont, middleX, PiccoListUi::textTop(renderer, kSmallFont, centerY), visible.c_str());
  if (middleIsOrder) {
    const Box orderBox{middleLeft, band.y, std::max(0, middleRight - middleLeft), band.height};
    if (mappedInput.hasTouchHardware()) hit(screen, orderBox, ACTION_CONTROL, CONTROL_ORDER);
    if (focusShown && selection == CONTROL_ORDER) {
      PiccoListUi::drawFocusFrame(
          renderer,
          Rect{middleX - 8, centerY - PiccoTokens::kActionHeight / 2, middleW + 16, PiccoTokens::kActionHeight},
          PiccoTokens::kSmallRadius);
    }
  }

  const bool grid = gridEnabled();
  PiccoListUi::drawSegmentedFrame(renderer, toRect(viewToggle), 2, grid ? 0 : 1);
  const Box gridCell = PiccoLibraryLayout::segmentCell(viewToggle, 2, 0);
  const Box listCell = PiccoLibraryLayout::segmentCell(viewToggle, 2, 1);
  constexpr int kGlyph = 18;
  PiccoListUi::drawGridGlyph(renderer, gridCell.x + (gridCell.width - kGlyph) / 2,
                             gridCell.y + (gridCell.height - kGlyph) / 2, kGlyph);
  drawLucideIcon(renderer, icon_menu_24, listCell.x + (listCell.width - icon_menu_24.w) / 2,
                 listCell.y + (listCell.height - icon_menu_24.h) / 2);
  if (mappedInput.hasTouchHardware()) hit(screen, viewToggle, ACTION_CONTROL, CONTROL_VIEW);
  if (focusShown && selection == CONTROL_VIEW) PiccoListUi::drawFocusFrame(renderer, toRect(viewToggle), 16);

  const Box body = bodyBox(renderer);
  if (rowCount() == 0) {
    if (scanFailed || filterFailed) return;
    const char* message = tr(STR_LIBRARY_EMPTY);
    if (hasActiveFilter()) {
      message = tr(STR_LIBRARY_NO_RESULTS);
    } else if (sort == Sort::RecentlyRead) {
      message = tr(STR_NO_RECENT_BOOKS);
    }
    const int w = renderer.getTextWidth(kBodyFont, message);
    renderer.drawText(kBodyFont, body.x + std::max(0, (body.width - w) / 2),
                      PiccoListUi::textTop(renderer, kBodyFont, body.y + body.height / 2), message);
    return;
  }
  if (grid) {
    buildPiccoGrid(fui::Rect{static_cast<int16_t>(body.x), static_cast<int16_t>(body.y),
                             static_cast<int16_t>(body.width), static_cast<int16_t>(body.height)},
                   screen);
    return;
  }
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(body.y), 0,
      static_cast<int16_t>(renderer.getScreenHeight() - PiccoLayout::listBottomLimit(screenBox(renderer))), 0});
  buildBookList(screen);
}

void LibraryActivity::buildPiccoGrid(const fui::Rect& areaRect, UiApp::ScreenType& screen) {
  const Box area{areaRect.x, areaRect.y, areaRect.width, areaRect.height};
  const int pageSize = piccoGridPageSize();
  const Box firstCover = PiccoLibraryLayout::gridCover(PiccoLibraryLayout::gridCell(area, 0));
  if (firstCover.width != gridCoverWidth || firstCover.height != gridCoverHeight) {
    gridCoverWidth = static_cast<int16_t>(firstCover.width);
    gridCoverHeight = static_cast<int16_t>(firstCover.height);
    loadedGridPageStart = -1;
    nextGridCoverRow = -1;
  }
  const int selectedRow = selection - CONTROL_COUNT;
  const int visible = std::min(pageSize, rowCount() - gridPageStart);
  for (int slot = 0; slot < visible; ++slot) {
    const int row = gridPageStart + slot;
    const Box cell = PiccoLibraryLayout::gridCell(area, slot);
    const Box cover = PiccoLibraryLayout::gridCover(cell);
    if (!readBook(row, rowScratch)) continue;

    bool drawn = false;
    if (const RecentBook* recent = recentForPath(rowScratch.path); recent && !recent->coverBmpPath.empty()) {
      const std::string path = UITheme::getCoverThumbPath(recent->coverBmpPath, cover.width, cover.height);
      FsFile file;
      if (!path.empty() && Storage.exists(path.c_str()) && Storage.openFileForRead("LIB", path, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.getWidth() > 0 && bitmap.getHeight() > 0)
          drawn = renderer.drawBitmap(bitmap, cover.x, cover.y, cover.width, cover.height);
        file.close();
      }
    }
    if (!drawn) {
      // Text cover: double rule, title, author (the board's placeholder).
      constexpr int kPadX = 10;
      constexpr int kPadY = 12;
      constexpr int kRuleGap = 5;
      const int innerX = cover.x + kPadX;
      const int innerW = cover.width - 2 * kPadX;
      renderer.fillRect(innerX, cover.y + kPadY, innerW, 1, true);
      renderer.fillRect(innerX, cover.y + kPadY + kRuleGap, innerW, 1, true);
      const auto lines = renderer.wrappedText(kBookFont, rowScratch.title.c_str(), innerW, 3);
      const int lineH = renderer.getLineHeight(kBookFont);
      int y = cover.y + (cover.height - static_cast<int>(lines.size()) * lineH) / 2;
      for (const auto& line : lines) {
        const int w = renderer.getTextWidth(kBookFont, line.c_str());
        renderer.drawText(kBookFont, innerX + (innerW - w) / 2, y, line.c_str());
        y += lineH;
      }
      if (!rowScratch.author.empty()) {
        const std::string author = renderer.truncatedText(kSmallFont, rowScratch.author.c_str(), innerW);
        const int w = renderer.getTextWidth(kSmallFont, author.c_str());
        renderer.drawText(kSmallFont, innerX + (innerW - w) / 2,
                          cover.bottom() - kPadY - renderer.getLineHeight(kSmallFont), author.c_str());
      }
    }
    renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
    if (showSelection && row == selectedRow) PiccoListUi::drawFocusFrame(renderer, toRect(cover), 2);

    // Title and progress under the cover.
    const int lineH = renderer.getLineHeight(kSmallFont);
    const int titleY = cover.bottom() + PiccoLibraryLayout::kGridTitleGap;
    const std::string title = renderer.truncatedText(kBookFont, rowScratch.title.c_str(), cell.width);
    renderer.drawText(kBookFont, cell.x, titleY, title.c_str());
    const float percent = gridPercentPage == gridPageStart ? gridPagePercent[slot] : -1.0f;
    const bool finished = isFinishedRow(row);
    std::string state;
    if (finished) {
      state = tr(STR_DONE);
    } else if (RecentBookProgress::hasPercent(percent) && percent > 0.0f) {
      state = RecentBookProgress::formatPercent(percent);
    } else {
      state = tr(STR_PICCO_BOOK_NEW);
    }
    const int stateW = renderer.getTextWidth(kSmallFont, state.c_str());
    const int progressY = titleY + renderer.getLineHeight(kBookFont) + 2;
    renderer.drawText(kSmallFont, cell.right() - stateW, progressY, state.c_str());
    const int permille = finished ? 1000 : RecentBookProgress::hasPercent(percent) ? static_cast<int>(percent * 10) : 0;
    PiccoProgress::draw(renderer, Rect{cell.x, progressY, std::max(0, cell.width - stateW - 8), lineH}, permille);

    hit(screen, cell, ACTION_ROW, row, fui::InputTouch | fui::InputLongPress);
  }
}

void LibraryActivity::buildPiccoSortScreen(UiApp::ScreenType& screen) {
  const Box screenArea = screenBox(renderer);
  const auto header = headerRect(renderer);
  if (mappedInput.hasTouchHardware())
    hit(screen, toBox(TouchHeaderBackButton::trailingTouchRect(header)), ACTION_PICCO_SORT, SORT_MENU);

  const int left = PiccoTokens::kGutter;
  const int width = screenArea.width - 2 * PiccoTokens::kGutter;
  const int labelH = renderer.getLineHeight(kSmallFont);
  int y = header.y + header.height + PiccoTokens::kSectionFirstAbove;
  GUI.drawSubHeader(renderer, Rect{0, y, screenArea.width, labelH}, tr(STR_PICCO_SORT_SECTION));
  y += labelH + PiccoTokens::kSectionBelow;

  const int current = optionForSort(static_cast<uint8_t>(sort));
  for (int i = 0; i < SORT_OPTION_COUNT; ++i) {
    const Rect row{left, y, width, PiccoTokens::kRowOneLine};
    PiccoListUi::drawCheckRow(renderer, row, I18N.get(kSortLabels[i]), i == current, sortFocus == i, true);
    hit(screen, toBox(row), ACTION_PICCO_SORT, i);
    y += PiccoTokens::kRowOneLine;
  }
  renderer.fillRect(left, y, width, PiccoTokens::kHairline, true);

  y += PiccoTokens::kSectionAbove;
  GUI.drawSubHeader(renderer, Rect{0, y, screenArea.width, labelH}, tr(STR_PICCO_ORDER_SECTION));
  y += labelH + PiccoTokens::kSectionBelow;
  const Box order{left, y, width, PiccoTokens::kTabBarHeight};
  const bool byDate = sort == Sort::DateAdded || sort == Sort::RecentlyRead;
  const char* labels[] = {orderLabel(byDate), orderLabel(!byDate)};
  const int selectedCell = byDate ? (descending ? 0 : 1) : (descending ? 1 : 0);
  PiccoListUi::drawSegmented(renderer, toRect(order), labels, 2, selectedCell);
  for (int cell = 0; cell < 2; ++cell) {
    const Box box = PiccoLibraryLayout::segmentCell(order, 2, cell);
    hit(screen, box, ACTION_PICCO_SORT, SORT_ORDER_BASE + cell);
    if (sortFocus == SORT_OPTION_COUNT + cell) PiccoListUi::drawFocusFrame(renderer, toRect(box), 12);
  }
}

void LibraryActivity::renderPicco() {
  const auto header = headerRect(renderer);
  for (int pass = 0; pass < 8; ++pass) {
    renderer.clearScreen();
    HeaderChrome chrome;
    chrome.leading = HeaderLeading::Back;
    chrome.title = sortScreenOpen ? tr(STR_PICCO_SORT_BY) : tr(STR_LIBRARY);
    chrome.contextualIcon = sortScreenOpen ? &icon_ellipsis_vertical_32 : &icon_search_24;
    GUI.drawHeaderChrome(renderer, header, chrome);
    renderUiApp(app, uiTarget);
    topIndex = listNav.top;
    if (!listNav.consumeRebuildNeeded()) break;
  }
  uiReady = true;
  if (!sortScreenOpen && rowCount() > 0) {
    if (gridEnabled()) {
      const int pageSize = piccoGridPageSize();
      PiccoPageIndicator::draw(renderer, gridPageStart / pageSize + 1, PiccoLayout::pageCount(rowCount(), pageSize));
    } else {
      PiccoPageIndicator::drawForList(renderer, topIndex, listNav.pageRowsFor(rowCount()), rowCount());
    }
  }
  if (actionPopup.processRender(renderer, mappedInput)) return;
  if (pendingCacheDeletedFeedback) {
    GUI.drawPopup(renderer, tr(STR_BOOK_CACHE_DELETED));
  } else {
    renderer.displayBuffer();
  }
}

#endif
