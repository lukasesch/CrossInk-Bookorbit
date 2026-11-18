#include "PiccoLoadingScreen.h"

#if CROSSINK_APP_PICCO_UI

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "PiccoLoadingLayout.h"
#include "PiccoProgress.h"
#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/themes/BaseTheme.h"
#include "components/themes/bookorbit/BookOrbitHomeUi.h"
#include "fontIds.h"
#include "home/PiccoHomeLayout.h"
#include "sheet/PiccoSheetDraw.h"

namespace PiccoLoadingScreen {

namespace {
using PiccoLayout::Box;
namespace D = PiccoSheetDraw;

constexpr int kCoverTitleLine = 24;  // Literata 10 (22 px) at line-height 1.1, as on Home
constexpr int kCoverTitleLines = 4;

PiccoLoadingLayout::Layout screenLayout(const GfxRenderer& renderer) {
  return PiccoLoadingLayout::layout(Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()});
}

const RecentBook* findBook(const char* bookPath) {
  if (bookPath == nullptr || bookPath[0] == '\0') return nullptr;
  for (const RecentBook& book : RECENT_BOOKS.getBooks()) {
    if (book.path == bookPath) return &book;
  }
  return nullptr;
}

const char* fileName(const char* path) {
  if (path == nullptr) return "";
  const char* slash = std::strrchr(path, '/');
  return slash != nullptr ? slash + 1 : path;
}

bool drawCoverImage(const GfxRenderer& renderer, const Box& cover, const RecentBook& book) {
  if (book.coverState == RecentBook::CoverState::Missing) return false;
  const std::string path = coverPath(book);
  if (path.empty() || !Storage.exists(path.c_str())) return false;
  HalFile file;
  if (!Storage.openFileForRead("LOAD", path.c_str(), file)) return false;
  Bitmap bitmap(file);
  const bool ok = bitmap.parseHeaders() == BmpReaderError::Ok;
  if (ok) {
    const Rect fitted = fittedBitmapRect(bitmap, Rect{cover.x, cover.y, cover.width, cover.height});
    renderer.drawBitmap(bitmap, fitted.x, fitted.y, fitted.width, fitted.height);
  }
  file.close();
  return ok;
}

// Home's text cover (double rule, Literata title, small author) in the larger frame.
void drawTextCover(const GfxRenderer& renderer, const Box& cover, const char* title, const char* author) {
  const PiccoHomeLayout::TextCover text = PiccoHomeLayout::textCoverLayout(cover);
  renderer.fillRect(text.rule.x, text.rule.y, text.rule.width, PiccoTokens::kHairline, true);
  renderer.fillRect(text.rule.x, text.rule.bottom() - 1, text.rule.width, PiccoTokens::kHairline, true);
  const auto lines = renderer.wrappedText(LITERATA_10_FONT_ID, title, text.title.width, kCoverTitleLines);
  int y = text.title.y + (text.title.height - static_cast<int>(lines.size()) * kCoverTitleLine) / 2;
  for (const auto& line : lines) {
    D::text(renderer, LITERATA_10_FONT_ID, Box{text.title.x, y, text.title.width, kCoverTitleLine}, line.c_str(),
            EpdFontFamily::REGULAR, D::Align::Center);
    y += kCoverTitleLine;
  }
  if (author[0] != '\0') {
    D::text(renderer, SMALL_FONT_ID, text.author, author, EpdFontFamily::REGULAR, D::Align::Center);
  }
}
}  // namespace

bool applies(const GfxRenderer& renderer) {
  return UITheme::isPicco() && renderer.getScreenWidth() < renderer.getScreenHeight();
}

std::string coverPath(const RecentBook& book) {
  return BookOrbitHomeUi::coverPath(book, PiccoLoadingLayout::kCoverW, PiccoLoadingLayout::kCoverH);
}

void draw(const GfxRenderer& renderer, const char* bookPath, const bool indexing) {
  const PiccoLoadingLayout::Layout l = screenLayout(renderer);
  const RecentBook* book = findBook(bookPath);
  const char* title = book != nullptr && !book->title.empty() ? book->title.c_str() : fileName(bookPath);
  const char* author = book != nullptr ? book->author.c_str() : "";

  renderer.clearScreen();
  if (book == nullptr || !drawCoverImage(renderer, l.cover, *book)) drawTextCover(renderer, l.cover, title, author);
  renderer.drawRect(l.cover.x, l.cover.y, l.cover.width, l.cover.height, true);
  D::text(renderer, LITERATA_15_FONT_ID, l.title, title, EpdFontFamily::REGULAR, D::Align::Center);
  if (author[0] != '\0') D::text(renderer, UI_10_FONT_ID, l.author, author, EpdFontFamily::REGULAR, D::Align::Center);
  // Progress arrives later, if at all; until then the status line names the wait.
  if (indexing) {
    D::text(renderer, SMALL_FONT_ID, l.status, tr(STR_PICCO_PREPARING_PAGES), EpdFontFamily::REGULAR, D::Align::Center);
  }
  renderer.displayBuffer();
}

void drawProgress(const GfxRenderer& renderer, const int percent) {
  const PiccoLoadingLayout::Layout l = screenLayout(renderer);
  const int value = std::clamp(percent, 0, 100);
  const Box& area = l.progressArea;
  renderer.fillRect(area.x, area.y, area.width, area.height, false);
  PiccoProgress::draw(renderer, Rect{l.bar.x, l.bar.y, l.bar.width, l.bar.height}, value * 10);
  char status[64];
  std::snprintf(status, sizeof(status), tr(STR_PICCO_PREPARING_PAGES_FORMAT), value);
  D::text(renderer, SMALL_FONT_ID, l.status, status, EpdFontFamily::REGULAR, D::Align::Center);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

}  // namespace PiccoLoadingScreen

#endif
