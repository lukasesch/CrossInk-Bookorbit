#pragma once

#include <string>

class GfxRenderer;

// Passed by value through the loader/reader handoff. Check only between loading
// stages, while the framebuffer is available and before painting page contents.
class BookLoadingFeedback {
  bool pending = false;
  // The Picco loading screen (not the popup) is up and takes progress.
  bool progressScreen = false;
  int shownProgress = -1;
  std::string bookPath;

 public:
  static constexpr int PROGRESS_STEP = 10;
  // Each progress repaint costs a panel refresh: only every PROGRESS_STEP points, and never
  // 100, since the first page replaces the screen then anyway.
  static bool progressDue(int shown, int next) { return next < 100 && (shown < 0 || next >= shown + PROGRESS_STEP); }

  void setBook(std::string path) { bookPath = std::move(path); }
  void start() { pending = true; }
  void finish() { pending = false; }
  bool showIfPending(GfxRenderer& renderer);
  void showIndexing(GfxRenderer& renderer);
  // Indexing progress, 0-100. Only the Picco loading screen shows it; call it only while
  // the indexing notice from showIndexing() is still on screen.
  void showProgress(GfxRenderer& renderer, int percent);
};
