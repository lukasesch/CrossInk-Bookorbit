#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "activities/reader/ReaderSettingsController.h"
#include "activities/util/FrontlightPanelModel.h"

class Epub;

// What an open reader hands the Picco sheet when it opens: the book state the main pane
// shows, what the More / Advanced panes offer, and the settings callbacks its
// ReaderSettingsController commits through. Filled by Activity::fillReaderSheetDetails.
struct ReaderSheetDetails {
  enum class Format : uint8_t { Epub, Xtc, Txt };

  Format format = Format::Txt;
  bool hasContents = false;
  bool hasMenu = false;  // XTC: the More tile opens today's menu list
  // Sheet pane / tab / focus to open on (the reader keeps it across a reopen).
  FrontlightDrawerState initialState{};

  // EPUB only below.
  std::shared_ptr<Epub> epub;
  ReaderSettingsController::Callbacks callbacks{};
  std::string dictionaryFontFamily;
  uint8_t dictionaryFontPointSize = 0;
  bool hasDictionaryFontOverride = false;

  uint32_t stablePage = 0;
  uint32_t stablePageCount = 0;
  uint16_t footnoteCount = 0;
  uint16_t bookmarkCount = 0;
  uint16_t autoPageTurnSeconds = 30;
  bool autoPageTurnActive = false;
  bool hasFootnotes = false;
  bool hasDictionary = false;
  bool hasClippings = false;
  bool pageBookmarked = false;
  bool completed = false;
  bool showReadingPaceReset = false;
  bool globalStats = false;
  bool bookStats = false;
};
