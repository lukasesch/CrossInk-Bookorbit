#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <HalDisplay.h>

#include <cstdint>
#include <string>

#include "PiccoHomeLayout.h"
#include "PiccoHomeStops.h"
#include "activities/reader/ReadingStatsUtils.h"
#include "components/themes/bookorbit/BookOrbitHomeModel.h"
#include "components/themes/bookorbit/BookOrbitHomeReadingData.h"

class GfxRenderer;
class MappedInputManager;
struct RecentBook;

// The Picco theme's Home (board T_Combined): header with the sync button, the current
// book, a reading stats card and four tiles.
//
// Like BookOrbitHomeUi, everything comes from the SD card in begin(), so opening Home
// never waits on the network, and moving the focus only redraws the 2 px rings in the
// gutters. The focus ring stays hidden until the rocker is first used, so a touch-only
// session sees the board as drawn.
class PiccoHomeUi final {
 public:
  using Action = PiccoHomeStops::Action;
  using Target = PiccoHomeStops::Target;

  static constexpr int kCoverWidth = PiccoHomeLayout::kCoverW;
  static constexpr int kCoverHeight = PiccoHomeLayout::kCoverH;

  PiccoHomeUi(GfxRenderer& renderer, const MappedInputManager& mappedInput);

  // Reads what Home shows; the pointers in source must stay valid until the next begin().
  // The next render() repaints.
  void begin(const BookOrbitHomeSource& source);
  void render(HalDisplay::RefreshMode refreshMode);

  // Rocker focus, wrapping; the first move only reveals the ring on Resume's neighbour.
  void moveFocus(int delta);
  Action focusedAction() const;
  // A TouchRegistry id this UI registered; None for anything else.
  Action actionForTap(int id) const;
  bool heroContains(int x, int y) const { return layout.hero.area.contains(x, y); }
  bool hasAccount() const { return account; }
  // Shows "Syncing…" in the header; the next render() repaints.
  void showSyncing();

  // The thumbnail the hero draws (exact size: rescaling a dithered BMP aliases).
  static std::string coverPath(const RecentBook& book);

 private:
  void paint();
  void drawHeader();
  void drawHero() const;
  void drawCover(const RecentBook* book) const;
  void drawStats() const;
  void drawTiles() const;
  void drawRing(int focusIndex, bool state) const;
  void registerTouchTargets() const;
  uint32_t framebufferHash() const;
  PiccoHomeStops::HeroMode heroMode() const;

  GfxRenderer& renderer;
  const MappedInputManager& mappedInput;
  PiccoHomeLayout::Home layout{};

  BookOrbitHomeSource data;
  BookOrbitHomeReadingData reading;
  ReadingStatsDateTime today;
  bool hasToday = false;
  bool account = false;
  bool hasSnapshot = false;
  uint32_t syncedAt = 0;  // the snapshot's Unix time, 0 when unknown
  bool booksThisYearKnown = false;
  int32_t booksThisYear = 0;
  int32_t libraryBooks = -1;  // -1 before the Library's first scan
  bool offline = false;
  bool syncing = false;

  // Written by loop(), read by the render task, like HomeActivity::selectorIndex.
  int focus = 0;
  bool showRing = false;

  // What the framebuffer holds, owned by the render task.
  PiccoLayout::Box syncRect{};
  bool painted = false;
  bool repaintRequested = true;
  int paintedFocus = -1;
  uint32_t paintedHash = 0;
};

#endif
