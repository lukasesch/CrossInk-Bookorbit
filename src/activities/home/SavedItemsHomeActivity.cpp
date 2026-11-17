#include "SavedItemsHomeActivity.h"

#include <Epub.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>

#include "../reader/EpubReaderBookmarkListActivity.h"
#include "../reader/EpubReaderClippingListActivity.h"
#include "BookActions.h"
#include "BookmarkStore.h"
#include "ClippingStore.h"
#include "CrossPointState.h"
#include "FileBrowserActionActivity.h"
#include "FilenameFontSystem.h"
#include "MappedInputManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/CompactHeader.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"

#if CROSSINK_APP_PICCO_UI
#include <LibraryBuilder.h>
#include <LibraryIndexFile.h>

#include <cstdio>

#include "activities/ActivityManager.h"
#include "components/picco/PiccoListUi.h"
#include "components/picco/PiccoPageIndicator.h"

extern ActivityManager activityManager;
#endif

namespace fui = freeink::ui;

namespace {
constexpr unsigned long SAVED_ITEM_DELETE_HOLD_MS = 1000;
constexpr fui::ActionId ACTION_ROW = 1;
constexpr fui::ActionId ACTION_BOOKS_TAB = 2;

void mergeBookmarkEntry(std::vector<SavedBookEntry>& out, const BookmarkedBookEntry& entry) {
  auto it = std::find_if(out.begin(), out.end(), [&](const SavedBookEntry& existing) {
    return existing.bookPath == entry.bookPath && existing.bookType == entry.bookType;
  });
  if (it != out.end()) {
    it->bookmarkCount = entry.count;
    if (it->bookTitle.empty()) it->bookTitle = entry.bookTitle;
    if (it->bookAuthor.empty()) it->bookAuthor = entry.bookAuthor;
    return;
  }
  out.push_back(
      {entry.bookTitle, entry.bookAuthor, entry.bookPath, entry.bookType, entry.count, static_cast<uint16_t>(0)});
}

void mergeClippingEntry(std::vector<SavedBookEntry>& out, const ClippedBookEntry& entry) {
  auto it = std::find_if(out.begin(), out.end(), [&](const SavedBookEntry& existing) {
    return existing.bookPath == entry.bookPath && existing.bookType == entry.bookType;
  });
  if (it != out.end()) {
    it->clippingCount = entry.count;
    if (it->bookTitle.empty()) it->bookTitle = entry.bookTitle;
    if (it->bookAuthor.empty()) it->bookAuthor = entry.bookAuthor;
    return;
  }
  out.push_back(
      {entry.bookTitle, entry.bookAuthor, entry.bookPath, entry.bookType, static_cast<uint16_t>(0), entry.count});
}
}  // namespace

SavedItemsHomeActivity::SavedItemsHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                               const int libraryBookCount)
    : Activity("SavedItemsHome", renderer, mappedInput),
      libraryBookCount(libraryBookCount),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void SavedItemsHomeActivity::reloadSavedBooks() {
  books.clear();

  std::vector<BookmarkedBookEntry> bookmarkedBooks;
  std::vector<ClippedBookEntry> clippedBooks;
  BookmarkStore::getAllBookmarkedBooks(bookmarkedBooks);
  ClippingStore::getAllClippedBooks(clippedBooks);

  books.reserve(bookmarkedBooks.size() + clippedBooks.size());
  for (const auto& entry : bookmarkedBooks) {
    mergeBookmarkEntry(books, entry);
  }
  for (const auto& entry : clippedBooks) {
    mergeClippingEntry(books, entry);
  }

  for (auto& entry : books) {
    if (!entry.bookTitle.empty()) continue;
    if (entry.bookType == "epub") {
      // Older completion moves could erase the saved title and author. Recover
      // display metadata without loading CSS, page layouts or location indexes.
      // Keep the EPUB parser off the activity task's small stack.
      auto epub = makeUniqueNoThrow<Epub>(entry.bookPath, "/.crosspoint");
      std::string title;
      std::string author;
      if (epub && epub->loadMetadata(title, author)) {
        entry.bookTitle = std::move(title);
        if (entry.bookAuthor.empty()) entry.bookAuthor = std::move(author);
      } else {
        LOG_ERR("SVA", "Could not recover saved book metadata: %s", entry.bookPath.c_str());
      }
    }
    if (entry.bookTitle.empty()) entry.bookTitle = entry.bookPath.substr(entry.bookPath.find_last_of('/') + 1);
  }

  if (books.empty()) {
    selectedIndex = 0;
  } else if (selectedIndex >= static_cast<int>(books.size())) {
    selectedIndex = static_cast<int>(books.size()) - 1;
  }
}

void SavedItemsHomeActivity::onEnter() {
  Activity::onEnter();
  {
    RenderLock lock(*this);
    filenameFontSystem.ensureLoaded(renderer);
  }
  reloadSavedBooks();
  selectedIndex = 0;
  topIndex = 0;
  visibleRows = 1;
  uiReady = false;
  applySharedUiTheme(app, uiTarget);
  app.on(ACTION_ROW, &SavedItemsHomeActivity::onRowEvent, this);
#if CROSSINK_APP_PICCO_UI
  app.on(ACTION_BOOKS_TAB, &SavedItemsHomeActivity::onBooksTabEvent, this);
  if (UITheme::isPicco()) enterPicco();
#endif
  app.setScreen(&SavedItemsHomeActivity::listScreen, this);
  requestUpdate();
}

void SavedItemsHomeActivity::onExit() {
  books.clear();
  Activity::onExit();
}

void SavedItemsHomeActivity::loop() {
  if (TouchHeaderBackButton::wasTapped(mappedInput, TouchHeaderBackButton::compactHeaderRect(renderer))) {
    onGoHome();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  if (!books.empty() && !piccoTabFocused && !longPressOpenHandled &&
      mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() >= SAVED_ITEM_DELETE_HOLD_MS) {
    longPressOpenHandled = true;
    showSavedBookActionMenu(selectedIndex, true);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (longPressOpenHandled) {
      longPressOpenHandled = false;
      return;
    }
#if CROSSINK_APP_PICCO_UI
    if (piccoTabFocused) {
      activityManager.goToLibrary();
      return;
    }
#endif
    if (!books.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(books.size())) {
      openSavedItems(selectedIndex);
    }
    return;
  }

  const int listSize = static_cast<int>(books.size());
#if CROSSINK_APP_PICCO_UI
  // The Books tab stays reachable when there is nothing saved.
  if (listSize == 0 && !UITheme::isPicco()) return;
#else
  if (listSize == 0) return;
#endif

  if (uiReady) {
    const fui::InputSnapshot snap = touchSnapshotFrom(mappedInput);
    if (snap.touchPressed || snap.touchReleased) {
      const auto event = app.route(snap);
      if (app.invalidated()) requestUpdate();
      if (event) return;
    }
  }
#if CROSSINK_APP_PICCO_UI
  if (UITheme::isPicco()) {
    handlePiccoRocker(listSize);
    if (listSize == 0) return;
  }
#endif
  if (listSize == 0) return;

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    const int next = scrollListBy(topIndex, swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows,
                                  visibleRows, listSize);
    if (next != topIndex) {
      topIndex = next;
      requestUpdate();
    }
    return;
  }

  const auto moveSelection = [this, listSize](const int next) {
#if CROSSINK_APP_PICCO_UI
    piccoTabFocused = false;
#endif
    selectedIndex = next;
    topIndex = followListSelection(selectedIndex, topIndex, visibleRows, listSize);
    requestUpdate();
  };
#if CROSSINK_APP_PICCO_UI
  if (!UITheme::isPicco())
#endif
  {
    buttonNavigator.onNextRelease(
        [this, listSize, &moveSelection] { moveSelection(ButtonNavigator::nextIndex(selectedIndex, listSize)); });
    buttonNavigator.onPreviousRelease(
        [this, listSize, &moveSelection] { moveSelection(ButtonNavigator::previousIndex(selectedIndex, listSize)); });
  }
  buttonNavigator.onNextContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::nextPageIndex(selectedIndex, listSize, visibleRows));
  });
  buttonNavigator.onPreviousContinuous([this, listSize, &moveSelection] {
    moveSelection(ButtonNavigator::previousPageIndex(selectedIndex, listSize, visibleRows));
  });
}

void SavedItemsHomeActivity::onRowEvent(const fui::ActionEvent& event, void* user) {
  auto* self = static_cast<SavedItemsHomeActivity*>(user);
  if (event.value < 0 || event.value >= static_cast<int16_t>(self->books.size())) return;
  self->selectedIndex = event.value;
  if (event.longPress) {
    self->app.clearTapFlash();
    self->showSavedBookActionMenu(self->selectedIndex, false);
    return;
  }
  self->app.clearTapFlash();
  self->openSavedItems(self->selectedIndex);
}

void SavedItemsHomeActivity::listScreen(UiApp::ScreenType& screen, void* user) {
  static_cast<SavedItemsHomeActivity*>(user)->buildListScreen(screen);
}

void SavedItemsHomeActivity::buildListScreen(UiApp::ScreenType& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
#if CROSSINK_APP_PICCO_UI
  if (UITheme::isPicco()) {
    buildPiccoTabs(screen);
  } else
#endif
  {
    setUiContentMargin(screen, renderer,
                       fui::Insets{static_cast<int16_t>(CompactHeader::contentTop(renderer)), 0,
                                   static_cast<int16_t>(UITheme::getButtonHintsReserve(renderer)), 0});
    screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  }
  if (books.empty()) {
    screen.centeredText(tr(STR_NO_BOOKMARKS), screen.theme().bodyText);
    return;
  }
  std::vector<fui::ListItem> items;
  items.reserve(books.size());
  for (size_t i = 0; i < books.size(); ++i) {
    fui::ListItem item;
    item.label = books[i].bookTitle.c_str();
    if (!books[i].bookAuthor.empty()) item.subtitle = books[i].bookAuthor.c_str();
    item.actionValue = static_cast<int16_t>(i);
    items.push_back(item);
  }
  FilenameUiFontScope fonts(uiTarget, renderer);
  fui::ListProps props;
  props.items = items.data();
  props.count = static_cast<uint16_t>(items.size());
  props.selectedIndex = static_cast<int16_t>(selectedIndex);
#if CROSSINK_APP_PICCO_UI
  if (piccoTabFocused) props.selectedIndex = -1;
#endif
  props.action = ACTION_ROW;
  props.inputMask = static_cast<uint16_t>(fui::InputTouch | fui::InputLongPress);
  props.labelText = screen.theme().bodyText;
  props.labelText.bold = true;
  const fui::Rect bounds = screen.body();
  const auto rows = configureUiList(props, screen.theme(), bounds, UiListRowType::WithSubtitle);
  visibleRows = rows > 0 ? rows : 1;
  topIndex = scrollListBy(topIndex, 0, visibleRows, static_cast<int>(books.size()));
  props.topIndex = static_cast<uint16_t>(topIndex);
  screen.list(props);
}

void SavedItemsHomeActivity::render(RenderLock&&) {
#if CROSSINK_APP_PICCO_UI
  if (UITheme::isPicco()) {
    renderPicco();
    return;
  }
#endif
  renderer.clearScreen();

  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::drawCompact(renderer, tr(STR_BOOKMARKS_AND_CLIPPINGS));
  } else {
    CompactHeader::drawTitle(renderer, tr(STR_BOOKMARKS_AND_CLIPPINGS));
  }
  uiReady = false;
  renderUiApp(app, uiTarget);
  uiReady = true;

  const auto labels =
      mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_HOME)), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}

void SavedItemsHomeActivity::openSavedItems(const int bookIndex) {
  if (bookIndex < 0 || bookIndex >= static_cast<int>(books.size())) return;
  const SavedBookEntry entry = books[bookIndex];
  const bool hasBookmarks = entry.bookmarkCount > 0;
  const bool hasClippings = entry.clippingCount > 0;

  if (hasBookmarks && hasClippings) {
    showSavedKindMenu(bookIndex);
  } else if (hasBookmarks) {
    openBookmarkList(entry);
  } else if (hasClippings) {
    openClippingList(entry);
  }
}

void SavedItemsHomeActivity::showSavedKindMenu(const int bookIndex) {
  if (bookIndex < 0 || bookIndex >= static_cast<int>(books.size())) return;
  const SavedBookEntry entry = books[bookIndex];

  std::vector<FileBrowserActionActivity::MenuItem> items;
  items.reserve(2);
  items.push_back({FileBrowserAction::ViewBookmarks, StrId::STR_BOOKMARKS});
  items.push_back({FileBrowserAction::ViewClippings, StrId::STR_CLIPPINGS});

  startActivityForResult(
      std::make_unique<FileBrowserActionActivity>(renderer, mappedInput, entry.bookTitle, std::move(items)),
      [this, entry](const ActivityResult& result) {
        const auto* actionResult = std::get_if<FileBrowserActionResult>(&result.data);
        if (result.isCancelled || !actionResult) {
          requestUpdate();
          return;
        }

        switch (static_cast<FileBrowserAction>(actionResult->action)) {
          case FileBrowserAction::ViewBookmarks:
            openBookmarkList(entry);
            break;
          case FileBrowserAction::ViewClippings:
            openClippingList(entry);
            break;
          default:
            requestUpdate();
            break;
        }
      });
}

void SavedItemsHomeActivity::showSavedBookActionMenu(const int bookIndex, const bool ignoreInitialConfirmRelease) {
  if (bookIndex < 0 || bookIndex >= static_cast<int>(books.size())) return;
  const SavedBookEntry entry = books[bookIndex];

  std::vector<FileBrowserActionActivity::MenuItem> items;
  items.reserve(2);
  if (entry.bookmarkCount > 0) {
    items.push_back({FileBrowserAction::DeleteBookmarks, StrId::STR_DELETE_BOOKMARKS});
  }
  if (entry.clippingCount > 0) {
    items.push_back({FileBrowserAction::DeleteClippings, StrId::STR_DELETE_CLIPPINGS});
  }

  startActivityForResult(
      std::make_unique<FileBrowserActionActivity>(renderer, mappedInput, entry.bookTitle, std::move(items),
                                                  ignoreInitialConfirmRelease),
      [this, entry](const ActivityResult& result) {
        longPressOpenHandled = false;
        const auto* actionResult = std::get_if<FileBrowserActionResult>(&result.data);
        if (!result.isCancelled && actionResult) {
          switch (static_cast<FileBrowserAction>(actionResult->action)) {
            case FileBrowserAction::DeleteBookmarks: {
              auto confirmation = makeUniqueNoThrow<ConfirmationActivity>(
                  renderer, mappedInput, BookActions::confirmationHeading(StrId::STR_DELETE_BOOKMARKS),
                  entry.bookTitle);
              if (!confirmation) {
                LOG_ERR("SVA", "OOM: bookmark clear ConfirmationActivity");
                reloadSavedBooks();
                requestUpdate();
                return;
              }
              startActivityForResult(std::move(confirmation), [this, entry](const ActivityResult& confirmation) {
                if (!confirmation.isCancelled) {
                  BOOKMARKS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType);
                  BOOKMARKS.clearAll();
                }
                reloadSavedBooks();
                requestUpdate();
              });
              return;
            }
            case FileBrowserAction::DeleteClippings:
              CLIPPINGS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType);
              CLIPPINGS.clearAll();
              break;
            default:
              break;
          }
        }
        reloadSavedBooks();
        requestUpdate();
      });
}

void SavedItemsHomeActivity::openBookmarkList(const SavedBookEntry& entry) {
  if (!BOOKMARKS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType)) {
    LOG_ERR("SVA", "Failed to load bookmarks");
    requestUpdate();
    return;
  }
  auto bookmarkList = makeUniqueNoThrow<EpubReaderBookmarkListActivity>(renderer, mappedInput);
  if (!bookmarkList) {
    LOG_ERR("SVA", "OOM: bookmark list activity (%u bytes)",
            static_cast<unsigned>(sizeof(EpubReaderBookmarkListActivity)));
    requestUpdate();
    return;
  }

  startActivityForResult(std::move(bookmarkList), [this, entry](const ActivityResult& result) {
    if (!result.isCancelled) {
      const auto* bm = std::get_if<BookmarkResult>(&result.data);
      if (bm) {
        APP_STATE.pendingBookmarkSpine = bm->spineIndex;
        APP_STATE.pendingBookmarkProgress = bm->progress;
        APP_STATE.pendingBookmarkParagraphIndex = bm->paragraphIndex;
        APP_STATE.pendingClippingIndex = UINT16_MAX;
        APP_STATE.saveToFile();
        onSelectBook(entry.bookPath);
      } else {
        LOG_ERR("SVA", "openBookmarkList: unexpected result variant");
        requestUpdate();
      }
    } else {
      reloadSavedBooks();
      requestUpdate();
    }
  });
}

void SavedItemsHomeActivity::openClippingList(const SavedBookEntry& entry) {
  CLIPPINGS.loadForBook(entry.bookPath, entry.bookTitle, entry.bookAuthor, entry.bookType);

  startActivityForResult(std::make_unique<EpubReaderClippingListActivity>(renderer, mappedInput),
                         [this, entry](const ActivityResult& result) {
                           if (!result.isCancelled) {
                             const auto* clipping = std::get_if<ClippingJumpResult>(&result.data);
                             if (clipping) {
                               APP_STATE.pendingBookmarkSpine = clipping->spineIndex;
                               APP_STATE.pendingBookmarkProgress =
                                   clipping->pageCount > 0 ? static_cast<float>(clipping->page) / clipping->pageCount
                                                           : 0.0f;
                               APP_STATE.pendingBookmarkParagraphIndex = clipping->paragraphIndex;
                               APP_STATE.pendingClippingIndex = clipping->clippingIndex;
                               APP_STATE.saveToFile();
                               onSelectBook(entry.bookPath);
                             } else {
                               LOG_ERR("SVA", "openClippingList: unexpected result variant");
                               requestUpdate();
                             }
                           } else {
                             reloadSavedBooks();
                             requestUpdate();
                           }
                         });
}

#if CROSSINK_APP_PICCO_UI
void SavedItemsHomeActivity::onBooksTabEvent(const fui::ActionEvent&, void* user) {
  auto* self = static_cast<SavedItemsHomeActivity*>(user);
  self->app.clearTapFlash();
  activityManager.goToLibrary();
}

Rect SavedItemsHomeActivity::piccoHeaderRect() const {
  return UITheme::getHeaderRect(renderer, UITheme::getInstance().getMetrics().headerHeight);
}

void SavedItemsHomeActivity::enterPicco() {
  piccoTabFocused = false;
  if (libraryBookCount >= 0) return;
  // Opened from Home rather than the Books tab: count the library for the tab label.
  if (auto counter = makeUniqueNoThrow<library::LibraryIndexFile>(false)) {
    if (counter->open(library::libraryIndexPath())) libraryBookCount = counter->bookCount();
    counter->close();
  }
}

void SavedItemsHomeActivity::handlePiccoRocker(const int listSize) {
  // Focus 0 is the Books tab, then the saved books.
  const auto step = [this, listSize](const int direction) {
    const PiccoLibraryLayout::FocusCursor cursor{piccoTabFocused ? 0 : selectedIndex + 1, topIndex};
    const auto next = PiccoLibraryLayout::rockerStepAround(cursor, direction, 1, listSize, 0, visibleRows);
    piccoTabFocused = next.focus == 0;
    if (!piccoTabFocused) selectedIndex = next.focus - 1;
    topIndex = next.top;
    requestUpdate();
  };
  buttonNavigator.onNextRelease([&step] { step(1); });
  buttonNavigator.onPreviousRelease([&step] { step(-1); });
}

void SavedItemsHomeActivity::buildPiccoTabs(UiApp::ScreenType& screen) {
  const PiccoLayout::Box screenBox{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const Rect header = piccoHeaderRect();
  const auto tabs = PiccoLibraryLayout::libraryBands(screenBox, header.y + header.height).tabs;
  char booksLabel[32];
  if (libraryBookCount >= 0) {
    std::snprintf(booksLabel, sizeof(booksLabel), tr(STR_PICCO_BOOKS_TAB_FORMAT),
                  static_cast<unsigned>(libraryBookCount));
  } else {
    std::snprintf(booksLabel, sizeof(booksLabel), "%s", tr(STR_PICCO_BOOKS_TAB));
  }
  unsigned saved = 0;
  for (const auto& entry : books) saved += entry.bookmarkCount + entry.clippingCount;
  char savedLabel[32];
  std::snprintf(savedLabel, sizeof(savedLabel), tr(STR_PICCO_SAVED_TAB_FORMAT), saved);
  const char* labels[] = {booksLabel, savedLabel};
  PiccoListUi::drawSegmented(renderer, PiccoListUi::toRect(tabs), labels, 2, 1);
  const auto booksCell = PiccoLibraryLayout::segmentCell(tabs, 2, 0);
  screen.frame().hit(fui::Rect{static_cast<int16_t>(booksCell.x), static_cast<int16_t>(booksCell.y),
                               static_cast<int16_t>(booksCell.width), static_cast<int16_t>(booksCell.height)},
                     ACTION_BOOKS_TAB, 0, fui::InputTouch);
  if (piccoTabFocused)
    PiccoListUi::drawFocusFrame(renderer, PiccoListUi::toRect(booksCell), PiccoTokens::kTabCellRadius);
  const int top = tabs.bottom() + PiccoLibraryLayout::kBandGap;
  screen.setContentMarginFromScreen(
      fui::Insets{static_cast<int16_t>(top), 0,
                  static_cast<int16_t>(renderer.getScreenHeight() - PiccoLayout::listBottomLimit(screenBox)), 0});
}

void SavedItemsHomeActivity::renderPicco() {
  renderer.clearScreen();
  HeaderChrome chrome;
  chrome.leading = HeaderLeading::Back;
  chrome.title = tr(STR_LIBRARY);
  GUI.drawHeaderChrome(renderer, piccoHeaderRect(), chrome);
  uiReady = false;
  renderUiApp(app, uiTarget);
  uiReady = true;
  PiccoPageIndicator::drawForList(renderer, topIndex, visibleRows, static_cast<int>(books.size()));
  renderer.displayBuffer();
}
#endif
