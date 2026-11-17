#pragma once

#include <array>
#include <cstdint>
#include <cstdio>

#include "PiccoHomeLayout.h"

// What the Picco Home's targets do, the rocker's focus order and the small formatting the
// header and stats need. Pure value code: the native tests cover all of it.
namespace PiccoHomeStops {

using PiccoLayout::Box;

enum class Target : uint8_t { Resume, Stats, BookOrbit, Library, Files, Settings, Cover, Sync, Count };

enum class Action : uint8_t { None, OpenBook, OpenLibrary, OpenStats, OpenCatalog, OpenFiles, OpenSettings, Refresh };

// The hero's variant: no recent book swaps Resume for a Library pill.
enum class HeroMode : uint8_t { Book, Empty };

constexpr HeroMode heroMode(const int recentBookCount) {
  return recentBookCount > 0 ? HeroMode::Book : HeroMode::Empty;
}

constexpr Action actionFor(const Target target, const HeroMode mode, const bool hasAccount) {
  switch (target) {
    case Target::Resume:
    case Target::Cover:
      return mode == HeroMode::Book ? Action::OpenBook : Action::OpenLibrary;
    case Target::Stats:
      return Action::OpenStats;
    case Target::BookOrbit:
      return Action::OpenCatalog;
    case Target::Library:
      return Action::OpenLibrary;
    case Target::Files:
      return Action::OpenFiles;
    case Target::Settings:
      return Action::OpenSettings;
    case Target::Sync:
      return hasAccount ? Action::Refresh : Action::None;
    case Target::Count:
      break;
  }
  return Action::None;
}

// Rocker order: Resume, stats card, then the tiles row by row. Cover and Sync are touch only.
constexpr std::array<Target, 6> kFocusOrder{Target::Resume,  Target::Stats, Target::BookOrbit,
                                            Target::Library, Target::Files, Target::Settings};
constexpr int kFocusCount = static_cast<int>(kFocusOrder.size());

constexpr int moveFocus(const int index, const int delta) {
  const int n = kFocusCount;
  return ((index + delta) % n + n) % n;
}

struct Stop {
  Box rect;
  int radius = 0;
};

// The element a focus index stands for, with its corner radius.
constexpr Stop focusStop(const PiccoHomeLayout::Home& home, const int focusIndex) {
  switch (kFocusOrder[static_cast<size_t>(moveFocus(focusIndex, 0))]) {
    case Target::Resume:
      return Stop{home.hero.pill, PiccoTokens::kPillRadius};
    case Target::Stats:
      return Stop{home.stats.card, PiccoTokens::kCardRadius};
    case Target::BookOrbit:
      return Stop{home.tiles[0].rect, PiccoTokens::kLargeTileRadius};
    case Target::Library:
      return Stop{home.tiles[1].rect, PiccoTokens::kLargeTileRadius};
    case Target::Files:
      return Stop{home.tiles[2].rect, PiccoTokens::kLargeTileRadius};
    case Target::Settings:
      return Stop{home.tiles[3].rect, PiccoTokens::kLargeTileRadius};
    default:
      break;
  }
  return Stop{};
}

// Touch rect of a target; the sync button's comes from the header layout (empty if hidden).
constexpr Box targetRect(const PiccoHomeLayout::Home& home, const Target target, const Box& syncRect) {
  switch (target) {
    case Target::Resume:
      return home.hero.pill;
    case Target::Cover:
      return home.hero.cover;
    case Target::Stats:
      return home.stats.card;
    case Target::BookOrbit:
      return home.tiles[0].rect;
    case Target::Library:
      return home.tiles[1].rect;
    case Target::Files:
      return home.tiles[2].rect;
    case Target::Settings:
      return home.tiles[3].rect;
    case Target::Sync:
      return syncRect;
    case Target::Count:
      break;
  }
  return Box{};
}

// ---- Text --------------------------------------------------------------------------------

// Sync age for the header: nowLabel under a minute, then "N min", "N h", "N d".
inline void formatSyncAge(const uint32_t ageSeconds, const char* nowLabel, char* out, const size_t outSize) {
  const uint32_t minutes = ageSeconds / 60u;
  if (minutes < 1) {
    std::snprintf(out, outSize, "%s", nowLabel != nullptr ? nowLabel : "");
  } else if (minutes < 60) {
    std::snprintf(out, outSize, "%lu min", static_cast<unsigned long>(minutes));
  } else if (minutes < 24u * 60u) {
    std::snprintf(out, outSize, "%lu h", static_cast<unsigned long>(minutes / 60u));
  } else {
    std::snprintf(out, outSize, "%lu d", static_cast<unsigned long>(minutes / (24u * 60u)));
  }
}

// Reading time: "47 min", "5 h 12 m", "21 h".
inline void formatMinutes(const uint32_t minutes, char* out, const size_t outSize) {
  if (minutes < 60) {
    std::snprintf(out, outSize, "%lu min", static_cast<unsigned long>(minutes));
  } else if (minutes % 60u == 0) {
    std::snprintf(out, outSize, "%lu h", static_cast<unsigned long>(minutes / 60u));
  } else {
    std::snprintf(out, outSize, "%lu h %lu m", static_cast<unsigned long>(minutes / 60u),
                  static_cast<unsigned long>(minutes % 60u));
  }
}

}  // namespace PiccoHomeStops
