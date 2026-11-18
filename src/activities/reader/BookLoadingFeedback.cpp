#include "BookLoadingFeedback.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "components/UITheme.h"

#if CROSSINK_APP_PICCO_UI
#include "components/picco/PiccoLoadingScreen.h"
#endif

namespace {
// The Picco screen replaces the popup in portrait; returns false where the popup stays.
bool drawPiccoScreen(GfxRenderer& renderer, const std::string& bookPath, const bool indexing) {
#if CROSSINK_APP_PICCO_UI
  if (!PiccoLoadingScreen::applies(renderer)) return false;
  PiccoLoadingScreen::draw(renderer, bookPath.c_str(), indexing);
  return true;
#else
  (void)renderer;
  (void)bookPath;
  (void)indexing;
  return false;
#endif
}
}  // namespace

bool BookLoadingFeedback::showIfPending(GfxRenderer& renderer) {
  if (!renderer.hasFrameBuffer() || !pending) return false;
  finish();
  progressScreen = false;
  // A prior notice may have a different size or orientation. Replace its pixels.
  renderer.clearScreen();
  if (!drawPiccoScreen(renderer, bookPath, false)) GUI.drawPopup(renderer, tr(STR_LOADING));
  return true;
}

void BookLoadingFeedback::showIndexing(GfxRenderer& renderer) {
  finish();
  progressScreen = false;
  shownProgress = -1;
  if (!renderer.hasFrameBuffer()) return;
  renderer.clearScreen();
  progressScreen = drawPiccoScreen(renderer, bookPath, true);
  if (!progressScreen) GUI.drawPopup(renderer, tr(STR_INDEXING));
}

void BookLoadingFeedback::showProgress(GfxRenderer& renderer, const int percent) {
  if (!progressScreen || !renderer.hasFrameBuffer() || !progressDue(shownProgress, percent)) return;
  shownProgress = percent;
#if CROSSINK_APP_PICCO_UI
  PiccoLoadingScreen::drawProgress(renderer, percent);
#endif
}
