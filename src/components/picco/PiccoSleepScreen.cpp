#include "PiccoSleepScreen.h"

#if CROSSINK_APP_PICCO_UI

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "PiccoProgress.h"
#include "PiccoReaderOverlays.h"
#include "RecentBooksStore.h"
#include "components/themes/BaseTheme.h"
#include "fontIds.h"
#include "home/PiccoHomeLayout.h"
#include "sheet/PiccoSheetDraw.h"

namespace PiccoSleepScreen {

namespace {
using PiccoLayout::Box;
namespace D = PiccoSheetDraw;

constexpr int kTitleLine = 38;  // Literata 15 at line-height ~1.1

bool drawCoverImage(const GfxRenderer& renderer, const Box& cover, const RecentBook& book) {
  if (book.coverBmpPath.empty() || !Storage.exists(book.coverBmpPath.c_str())) return false;
  HalFile file;
  if (!Storage.openFileForRead("SLP", book.coverBmpPath.c_str(), file)) return false;
  Bitmap bitmap(file);
  const bool ok = bitmap.parseHeaders() == BmpReaderError::Ok;
  if (ok) {
    const Rect fitted = fittedBitmapRect(bitmap, Rect{cover.x, cover.y, cover.width, cover.height});
    renderer.drawBitmap(bitmap, fitted.x, fitted.y, fitted.width, fitted.height);
  }
  file.close();
  return ok;
}

void drawTextCover(const GfxRenderer& renderer, const Box& cover, const RecentBook& book) {
  const PiccoHomeLayout::TextCover text = PiccoHomeLayout::textCoverLayout(cover);
  renderer.fillRect(text.rule.x, text.rule.y, text.rule.width, PiccoTokens::kHairline, true);
  renderer.fillRect(text.rule.x, text.rule.bottom() - 1, text.rule.width, PiccoTokens::kHairline, true);
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  const auto lines = renderer.wrappedText(LITERATA_15_FONT_ID, title, text.title.width, 4);
  int y = text.title.y + (text.title.height - static_cast<int>(lines.size()) * kTitleLine) / 2;
  for (const auto& line : lines) {
    D::text(renderer, LITERATA_15_FONT_ID, Box{text.title.x, y, text.title.width, kTitleLine}, line.c_str(),
            EpdFontFamily::REGULAR, D::Align::Center);
    y += kTitleLine;
  }
  D::text(renderer, SMALL_FONT_ID, text.author, book.author.c_str(), EpdFontFamily::REGULAR, D::Align::Center);
}
}  // namespace

void draw(const GfxRenderer& renderer, const RecentBook& book, const char* chapter, const float progressPercent) {
  renderer.clearScreen();
  const PiccoOverlay::SleepLayout s =
      PiccoOverlay::sleepLayout(Box{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()});
  if (!drawCoverImage(renderer, s.cover, book)) drawTextCover(renderer, s.cover, book);
  renderer.drawRect(s.cover.x, s.cover.y, s.cover.width, s.cover.height, true);

  int percentWidth = 0;
  if (progressPercent >= 0.0f) {
    const int percent = std::clamp(static_cast<int>(progressPercent + 0.5f), 0, 100);
    char text[8];
    std::snprintf(text, sizeof(text), "%d%%", percent);
    percentWidth = renderer.getTextWidth(SMALL_FONT_ID, text) + PiccoTokens::kGroupGap;
    D::text(renderer, SMALL_FONT_ID, s.meta, text, EpdFontFamily::REGULAR, D::Align::Right);
    PiccoProgress::draw(renderer, Rect{s.progress.x, s.progress.y, s.progress.width, s.progress.height},
                        static_cast<int>(progressPercent * 10.0f + 0.5f));
  }
  if (chapter != nullptr && chapter[0] != '\0') {
    D::text(renderer, SMALL_FONT_ID, Box{s.meta.x, s.meta.y, s.meta.width - percentWidth, s.meta.height}, chapter);
  }
  D::text(renderer, SMALL_FONT_ID, s.hint, tr(STR_PICCO_SLEEP_HINT), EpdFontFamily::REGULAR, D::Align::Center);
}

}  // namespace PiccoSleepScreen

#endif
