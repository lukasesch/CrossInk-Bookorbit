#include "PiccoHomeUi.h"

#if CROSSINK_APP_PICCO_UI

#include <Bitmap.h>
#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <LibraryBuilder.h>
#include <LibraryIndexFile.h>
#include <Memory.h>
#include <TrustedTime.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "BookOrbitCredentialStore.h"
#include "BookOrbitHomeStore.h"
#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "SilentRestart.h"
#include "activities/reader/GlobalReadingStats.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/piccoIcons.h"
#include "components/picco/PiccoCard.h"
#include "components/picco/PiccoPillButton.h"
#include "components/picco/PiccoProgress.h"
#include "components/picco/PiccoTheme.h"
#include "components/themes/bookorbit/BookOrbitHomeText.h"
#include "components/themes/bookorbit/BookOrbitHomeUi.h"
#include "fontIds.h"

namespace {
using PiccoLayout::Box;
using namespace PiccoHomeLayout;

enum class Align : uint8_t { Left, Center, Right };

const char* displayTitle(const RecentBook& book) { return book.title.empty() ? book.path.c_str() : book.title.c_str(); }

// Text with its line box at box.y, vertically centred in box.
void drawIn(const GfxRenderer& renderer, const int font, const Box& box, const char* text,
            const EpdFontFamily::Style style = EpdFontFamily::REGULAR, const Align align = Align::Left,
            const bool black = true) {
  if (text == nullptr || text[0] == '\0' || box.width <= 0) return;
  const std::string visible = renderer.truncatedText(font, text, box.width, style);
  const int w = renderer.getTextWidth(font, visible.c_str(), style);
  int x = box.x;
  if (align == Align::Center) x += (box.width - w) / 2;
  if (align == Align::Right) x = box.right() - w;
  const int y = box.y + (box.height - renderer.getLineHeight(font)) / 2;
  renderer.drawText(font, x, y, visible.c_str(), black, style);
}

// ASCII upper case for section labels (the board's text-transform), like PiccoTheme's.
void upperCase(const char* text, char* out, const size_t outSize) {
  size_t i = 0;
  for (; text != nullptr && text[i] != '\0' && i + 1 < outSize; ++i) {
    out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
  }
  out[i] = '\0';
}

void drawSectionLabel(const GfxRenderer& renderer, const Box& box, const char* text) {
  char caps[64];
  upperCase(text, caps, sizeof(caps));
  drawIn(renderer, SMALL_FONT_ID, box, caps, EpdFontFamily::BOLD);
}

Rect toRect(const Box& b) { return Rect{b.x, b.y, b.width, b.height}; }
}  // namespace

PiccoHomeUi::PiccoHomeUi(GfxRenderer& renderer, const MappedInputManager& mappedInput)
    : renderer(renderer), mappedInput(mappedInput) {}

std::string PiccoHomeUi::coverPath(const RecentBook& book) {
  return BookOrbitHomeUi::coverPath(book, kCoverWidth, kCoverHeight);
}

void PiccoHomeUi::begin(const BookOrbitHomeSource& source) {
  data = source;
  layout = homeLayout(Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()},
                      UITheme::getHeaderRect(renderer, PiccoTokens::kHeaderHeight).y);
  account = BOOKORBIT_STORE.hasCredentials();
  hasToday = getCurrentLocalReadingStatsDateTimeAnyClock(today);
  offline = homeSyncWentOffline();

  // The snapshot (about 900 bytes) is only needed while the numbers are worked out.
  hasSnapshot = false;
  syncedAt = 0;
  booksThisYearKnown = false;
  {
    std::unique_ptr<BookOrbitHomeSnapshot> snapshot;
    if (account) {
      snapshot = makeUniqueNoThrow<BookOrbitHomeSnapshot>();
      if (!snapshot) LOG_ERR("HOME", "No heap for the BookOrbit snapshot");
      if (snapshot && !BookOrbitHomeStore::load(*snapshot)) snapshot.reset();
    }
    reading.compute(data, snapshot.get(), SETTINGS.shouldTrackReadingStats(), hasToday, today);
    if (snapshot) {
      hasSnapshot = true;
      syncedAt = snapshot->generatedAt;
      if (hasToday && snapshot->goalYear == static_cast<int16_t>(today.date.year)) {
        booksThisYearKnown = true;
        booksThisYear = reading.goalCompleted;
      }
    }
  }

  // Only the index header: the count as of the Library's last scan, no card walk.
  libraryBooks = -1;
  if (Storage.exists(library::libraryIndexPath())) {
    library::LibraryIndexFile index;
    if (index.open(library::libraryIndexPath())) libraryBooks = index.bookCount();
  }
  repaintRequested = true;
}

PiccoHomeStops::HeroMode PiccoHomeUi::heroMode() const {
  return PiccoHomeStops::heroMode(data.books != nullptr ? static_cast<int>(data.books->size()) : 0);
}

void PiccoHomeUi::moveFocus(const int delta) {
  if (!showRing) {
    showRing = true;
    // The first press shows where focus is rather than moving it past Resume.
    if (delta < 0) focus = PiccoHomeStops::moveFocus(focus, delta);
    return;
  }
  focus = PiccoHomeStops::moveFocus(focus, delta);
}

PiccoHomeUi::Action PiccoHomeUi::focusedAction() const {
  return PiccoHomeStops::actionFor(PiccoHomeStops::kFocusOrder[static_cast<size_t>(focus)], heroMode(), account);
}

PiccoHomeUi::Action PiccoHomeUi::actionForTap(const int id) const {
  if (id < 0 || id >= static_cast<int>(Target::Count)) return Action::None;
  return PiccoHomeStops::actionFor(static_cast<Target>(id), heroMode(), account);
}

void PiccoHomeUi::showSyncing() {
  syncing = true;
  repaintRequested = true;
}

uint32_t PiccoHomeUi::framebufferHash() const {
  // Same FNV-1a check as BookOrbitHomeUi: lets render() trust the framebuffer without
  // tracking popups, menus or panels that drew over it.
  const uint8_t* buffer = renderer.getFrameBuffer();
  if (buffer == nullptr) return 0;
  const size_t size = renderer.getBufferSize();
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i + sizeof(uint32_t) <= size; i += sizeof(uint32_t)) {
    uint32_t word;
    memcpy(&word, buffer + i, sizeof(word));
    hash = (hash ^ word) * 16777619u;
  }
  return hash;
}

void PiccoHomeUi::render(const HalDisplay::RefreshMode refreshMode) {
  const int ringFocus = showRing ? focus : -1;
  const bool trusted =
      painted && !repaintRequested && refreshMode == HalDisplay::FAST_REFRESH && framebufferHash() == paintedHash;
  if (trusted) {
    registerTouchTargets();
    if (ringFocus == paintedFocus) return;
    drawRing(paintedFocus, false);
    drawRing(ringFocus, true);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  } else {
    paint();
    drawRing(ringFocus, true);
    registerTouchTargets();
    renderer.displayBuffer(refreshMode);
    // Literata and the key figures are big glyph sets Home has no use for until the next paint.
    if (auto* fontCache = renderer.getFontCacheManager()) fontCache->clearCache();
  }
  painted = true;
  repaintRequested = false;
  paintedFocus = ringFocus;
  paintedHash = framebufferHash();
}

void PiccoHomeUi::registerTouchTargets() const {
  for (int i = 0; i < static_cast<int>(Target::Count); ++i) {
    const Box box = PiccoHomeStops::targetRect(layout, static_cast<Target>(i), syncRect);
    if (!box.empty()) TouchRegistry::getInstance().add(toRect(box), i, TouchRegistry::Item);
  }
}

void PiccoHomeUi::drawRing(const int focusIndex, const bool state) const {
  if (focusIndex < 0) return;
  const auto stop = PiccoHomeStops::focusStop(layout, focusIndex);
  const auto ring = focusRing(stop.rect, stop.radius);
  renderer.drawRoundedRect(ring.box.x, ring.box.y, ring.box.width, ring.box.height, PiccoTokens::kSelectionStroke,
                           ring.radius, state);
}

void PiccoHomeUi::paint() {
  renderer.clearScreen();
  drawHeader();
  drawHero();
  drawStats();
  drawTiles();
}

void PiccoHomeUi::drawHeader() {
  HeaderChrome chrome;
  chrome.title = tr(STR_HOME);
  chrome.leading = HeaderLeading::House;
  char age[24] = {};
  if (account) {
    if (syncing) {
      chrome.sync = HeaderSyncState::Syncing;
    } else if (offline) {
      chrome.sync = HeaderSyncState::Offline;
    } else {
      chrome.sync = HeaderSyncState::Idle;
      const time_t now = time(nullptr);
      PiccoHomeStops::formatSyncLabel(trustedtime::isLive(), now > 0 ? static_cast<uint32_t>(now) : 0,
                                      APP_STATE.bookOrbitSyncedAt, syncedAt, tr(STR_PICCO_NOW), tr(STR_PICCO_SYNC), age,
                                      sizeof(age));
      chrome.syncLabel = age;
    }
  }
  const Rect header = toRect(layout.header);
  GUI.drawHeaderChrome(renderer, header, chrome);
  syncRect = account ? PiccoTheme::headerLayout(renderer, header, chrome).sync : Box{};
}

void PiccoHomeUi::drawCover(const RecentBook* book) const {
  const Box& cover = layout.hero.cover;
  if (book != nullptr) {
    const std::string path = coverPath(*book);
    HalFile file;
    if (!path.empty() && Storage.exists(path.c_str()) && Storage.openFileForRead("HOME", path.c_str(), file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const Rect fitted = fittedBitmapRect(bitmap, toRect(cover));
        renderer.drawBitmap(bitmap, fitted.x, fitted.y, fitted.width, fitted.height);
        file.close();
        renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
        return;
      }
      file.close();
    }
  }

  // Text cover: double rule, title in Literata, author at the foot.
  renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
  const TextCover text = textCoverLayout(cover);
  renderer.fillRect(text.rule.x, text.rule.y, text.rule.width, PiccoTokens::kHairline, true);
  renderer.fillRect(text.rule.x, text.rule.bottom() - 1, text.rule.width, PiccoTokens::kHairline, true);
  if (book == nullptr) return;
  const auto lines = renderer.wrappedText(LITERATA_10_FONT_ID, displayTitle(*book), text.title.width, 3);
  constexpr int kLine = 24;  // 22 px type at line-height 1.1
  const int count = static_cast<int>(lines.size());
  int y = text.title.y + (text.title.height - count * kLine) / 2;
  for (const auto& line : lines) {
    drawIn(renderer, LITERATA_10_FONT_ID, Box{text.title.x, y, text.title.width, kLine}, line.c_str(),
           EpdFontFamily::REGULAR, Align::Center);
    y += kLine;
  }
  drawIn(renderer, SMALL_FONT_ID, text.author, book->author.c_str(), EpdFontFamily::REGULAR, Align::Center);
}

void PiccoHomeUi::drawHero() const {
  const Hero& hero = layout.hero;
  const bool hasBook = heroMode() == PiccoHomeStops::HeroMode::Book;
  const RecentBook* book = hasBook ? &data.books->front() : nullptr;
  drawCover(book);
  drawSectionLabel(renderer, hero.label, tr(STR_CONTINUE_READING));

  const char* title = book != nullptr ? displayTitle(*book) : tr(STR_NO_OPEN_BOOK);
  const bool hasAuthor = book != nullptr && !book->author.empty();
  drawIn(renderer, LITERATA_15_FONT_ID, titleLine(hero), title);
  if (hasAuthor) drawIn(renderer, UI_10_FONT_ID, authorLine(hero), book->author.c_str());

  if (book != nullptr) {
    char percent[8] = {};
    int percentW = 0;
    if (data.currentBookProgress >= 0.0f) {
      std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(data.currentBookProgress + 0.5f));
      percentW = renderer.getTextWidth(SMALL_FONT_ID, percent);
      drawIn(renderer, SMALL_FONT_ID, hero.meta, percent, EpdFontFamily::REGULAR, Align::Right);
      PiccoProgress::draw(renderer, toRect(hero.progress), static_cast<int>(data.currentBookProgress * 10.0f + 0.5f));
    }
    if (data.chapterTitle != nullptr && !data.chapterTitle->empty()) {
      const int gap = percentW > 0 ? PiccoTokens::kGroupGap : 0;
      drawIn(renderer, SMALL_FONT_ID, Box{hero.meta.x, hero.meta.y, hero.meta.width - percentW - gap, hero.meta.height},
             data.chapterTitle->c_str());
    }
  }

  PiccoPillButton::draw(renderer, toRect(hero.pill), hasBook ? tr(STR_HOME_RESUME) : tr(STR_LIBRARY),
                        &icon_picco_arrow_right_22);
}

void PiccoHomeUi::drawStats() const {
  const Stats& s = layout.stats;
  PiccoCard::draw(renderer, toRect(s.card));
  drawSectionLabel(renderer, s.head, tr(STR_READING_STATS));
  drawIn(renderer, SMALL_FONT_ID, s.head, tr(STR_HOME_THIS_WEEK), EpdFontFamily::REGULAR, Align::Right);

  // Streak: key figure and unit on one baseline.
  char figure[8];
  std::snprintf(figure, sizeof(figure), "%u", static_cast<unsigned>(reading.streak));
  const int figureW = renderer.getTextWidth(FIGURES_27_FONT_ID, figure);
  const int figureTop = s.figure.y + (s.figure.height - renderer.getLineHeight(FIGURES_27_FONT_ID)) / 2;
  renderer.drawText(FIGURES_27_FONT_ID, s.figure.x, figureTop, figure);
  const int baseline = figureTop + renderer.getFontAscenderSize(FIGURES_27_FONT_ID);
  const char* unit = reading.streak == 1 ? tr(STR_HOME_DAY) : tr(STR_HOME_DAYS);
  const int unitX = s.figure.x + figureW + kFigureUnitGap;
  renderer.drawText(UI_10_FONT_ID, unitX, baseline - renderer.getFontAscenderSize(UI_10_FONT_ID),
                    renderer.truncatedText(UI_10_FONT_ID, unit, s.figure.right() - unitX, EpdFontFamily::BOLD).c_str(),
                    true, EpdFontFamily::BOLD);
  drawIn(renderer, SMALL_FONT_ID, s.streakLabel, tr(STR_PICCO_READING_STREAK));
  char duration[24];
  PiccoHomeStops::formatMinutes(reading.weekMinutes[kBarCount - 1], duration, sizeof(duration));
  char todayText[48];
  std::snprintf(todayText, sizeof(todayText), tr(STR_PICCO_TODAY_FORMAT), duration);
  drawIn(renderer, SMALL_FONT_ID, s.today, todayText, EpdFontFamily::BOLD);

  // The last seven days, today last; today's bar is an outline, the rest 50 % dither.
  uint32_t busiest = 0;
  for (const uint32_t m : reading.weekMinutes) busiest = std::max(busiest, m);
  const char* initials = tr(STR_HOME_WEEKDAY_INITIALS);
  const int todayOfWeek = hasToday ? readingStatsDayOfWeekIndex(today.date) : 6;
  for (int i = 0; i < kBarCount; ++i) {
    const Box& col = s.columns[static_cast<size_t>(i)];
    const bool isToday = i == kBarCount - 1;
    const int h = barHeight(reading.weekMinutes[static_cast<size_t>(i)], busiest, s.barMaxH);
    if (h > 0) {
      const int top = s.barBase - h;
      if (!isToday && h > 2) renderer.fillRectDither(col.x + 1, top + 1, col.width - 2, h - 2, Color::DarkGray);
      renderer.drawRoundedRect(col.x, top, col.width, h, PiccoTokens::kHairline, std::min(kBarRadius, h / 2), true,
                               true, false, false, true);
    }
    char initial[5];
    BookOrbitHomeText::copyInitial(initials, (todayOfWeek + 1 + i) % 7, initial, sizeof(initial));
    drawIn(renderer, SMALL_FONT_ID, Box{col.x, s.barBase + kBarLabelGap, col.width, kSmallLine}, initial,
           isToday ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, Align::Center);
  }

  // Totals: week time, books this year, time this month.
  renderer.fillRect(s.totalsRule.x, s.totalsRule.y, s.totalsRule.width, s.totalsRule.height, true);
  for (const Box& d : s.totalsDividers) renderer.fillRect(d.x, d.y, d.width, d.height, true);
  char values[kTotalsCount][24];
  char labels[kTotalsCount][32];
  PiccoHomeStops::formatMinutes(reading.weekTotal, values[0], sizeof(values[0]));
  std::snprintf(labels[0], sizeof(labels[0]), "%s", tr(STR_HOME_THIS_WEEK));
  if (booksThisYearKnown) {
    std::snprintf(values[1], sizeof(values[1]), "%ld %s", static_cast<long>(std::max<int32_t>(0, booksThisYear)),
                  tr(STR_HOME_BOOKS));
    std::snprintf(labels[1], sizeof(labels[1]), tr(STR_PICCO_IN_YEAR_FORMAT), static_cast<unsigned>(today.date.year));
  } else {
    std::snprintf(values[1], sizeof(values[1]), "%lu %s",
                  static_cast<unsigned long>(data.stats != nullptr ? data.stats->completedBooks : 0),
                  tr(STR_HOME_BOOKS));
    std::snprintf(labels[1], sizeof(labels[1]), "%s", tr(STR_HOME_ON_THIS_DEVICE));
  }
  PiccoHomeStops::formatMinutes(reading.currentMonthMinutes, values[2], sizeof(values[2]));
  std::snprintf(labels[2], sizeof(labels[2]), "%s", tr(STR_PICCO_THIS_MONTH));
  for (int i = 0; i < kTotalsCount; ++i) {
    const Box& t = s.totals[static_cast<size_t>(i)];
    drawIn(renderer, UI_10_FONT_ID, Box{t.x, t.y, t.width, kTotalsValueLine}, values[i], EpdFontFamily::BOLD);
    drawIn(renderer, SMALL_FONT_ID, Box{t.x, t.y + kTotalsValueLine, t.width, kSmallLine}, labels[i]);
  }
}

void PiccoHomeUi::drawTiles() const {
  char bookOrbit[40];
  if (!account) {
    std::snprintf(bookOrbit, sizeof(bookOrbit), "%s", tr(STR_CONNECT));
  } else if (!hasSnapshot && APP_STATE.bookOrbitSyncedAt == 0) {
    std::snprintf(bookOrbit, sizeof(bookOrbit), "%s", tr(STR_HOME_NOT_SYNCED));
  } else {
    const time_t now = time(nullptr);
    uint32_t ageSeconds = 0;
    const bool known = PiccoHomeStops::syncAgeSeconds(trustedtime::isLive(), now > 0 ? static_cast<uint32_t>(now) : 0,
                                                      APP_STATE.bookOrbitSyncedAt, syncedAt, ageSeconds);
    if (!known || ageSeconds < 60) {
      std::snprintf(bookOrbit, sizeof(bookOrbit), "%s", known ? tr(STR_PICCO_NOW) : tr(STR_PICCO_SYNC));
    } else {
      char age[16];
      PiccoHomeStops::formatSyncAge(ageSeconds, tr(STR_PICCO_NOW), age, sizeof(age));
      std::snprintf(bookOrbit, sizeof(bookOrbit), tr(STR_HOME_SYNCED_AGO_FORMAT), age);
    }
  }
  char library[32];
  if (libraryBooks >= 0) {
    std::snprintf(library, sizeof(library), "%ld %s", static_cast<long>(libraryBooks), tr(STR_HOME_BOOKS));
  } else {
    std::snprintf(library, sizeof(library), "%s", tr(STR_BROWSE));
  }
  // Storage and transport names, not words: the same in every language.
  static constexpr char kFilesHint[] = "SD \xC2\xB7 USB \xC2\xB7 Wi-Fi";
  char settings[40];
  std::snprintf(settings, sizeof(settings), "%s, Wi-Fi", tr(STR_CAT_DISPLAY));

  struct TileContent {
    const freeink::Icon* icon;
    const char* label;
    const char* subtitle;
  };
  const TileContent content[kTileCount] = {
      {&icon_picco_orbit_32, tr(STR_BOOKORBIT), bookOrbit},
      {&icon_picco_books_32, tr(STR_LIBRARY), library},
      {&icon_picco_folder_32, tr(STR_BROWSE_FILES), kFilesHint},
      {&icon_picco_sliders_32, tr(STR_SETTINGS_SHORT), settings},
  };
  for (int i = 0; i < kTileCount; ++i) {
    const Tile& tile = layout.tiles[static_cast<size_t>(i)];
    PiccoCard::draw(renderer, toRect(tile.rect));
    drawLucideIcon(renderer, *content[i].icon, tile.icon.x, tile.icon.y);
    drawIn(renderer, UI_10_FONT_ID, tile.label, content[i].label, EpdFontFamily::BOLD);
    drawIn(renderer, SMALL_FONT_ID, tile.subtitle, content[i].subtitle);
  }
}

#endif
