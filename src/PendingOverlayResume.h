#pragma once

#include <cstdint>
#include <string>
#include <utility>

enum class PendingOverlayOrigin : uint8_t { None = 0, Reader, Home, FileBrowser, Library };
// Raw values are persisted: append only.
enum class PendingOverlayType : uint8_t { None = 0, ReaderDrawer, FrontlightDrawer, ReaderSheet };

// Overlays a reader restores through its reader menu. A ReaderSheet resume opens
// the Picco sheet; under another theme it opens the drawer like ReaderDrawer.
constexpr bool pendingOverlayOpensReaderMenu(const PendingOverlayType overlay) {
  switch (overlay) {
    case PendingOverlayType::ReaderDrawer:
    case PendingOverlayType::ReaderSheet:
      return true;
    case PendingOverlayType::None:
    case PendingOverlayType::FrontlightDrawer:
      return false;
  }
  return false;
}

// Top sheets (light panel, Picco reader sheet) keep the reader's orientation across a sync.
constexpr bool pendingOverlayIsTopSheet(const PendingOverlayType overlay) {
  return overlay == PendingOverlayType::FrontlightDrawer || overlay == PendingOverlayType::ReaderSheet;
}

struct PendingOverlayResume {
  PendingOverlayOrigin origin = PendingOverlayOrigin::None;
  PendingOverlayType overlay = PendingOverlayType::None;
  // Library returns use tab for sort method and pane for descending order.
  uint8_t tab = 0;
  uint8_t pane = 0;
  // File Browser can navigate beyond the UI list's 16-bit row window.
  int32_t selectedIndex = 0;
  int32_t scrollPosition = 0;
  std::string bookPath;
  std::string fileBrowserPath;
  std::string libraryQuery;
  bool returnHomeAfterReaderFlow = false;
  // A book can override global orientation. Preserve that layout while a
  // reader-originated sync restarts through the network boot flow.
  uint8_t readerOrientation = 0;
  bool preserveReaderOrientation = false;

  bool valid() const {
    if (origin == PendingOverlayOrigin::Library) return true;
    if (origin == PendingOverlayOrigin::FileBrowser) return !fileBrowserPath.empty();
    return origin != PendingOverlayOrigin::None && overlay != PendingOverlayType::None;
  }
  bool returnsToBookList() const {
    return valid() && (origin == PendingOverlayOrigin::FileBrowser || origin == PendingOverlayOrigin::Library);
  }
  void clear() { *this = PendingOverlayResume{}; }
};

inline bool consumePendingOverlayResumeOnce(PendingOverlayResume& stored, PendingOverlayResume& value) {
  if (!stored.valid()) return false;
  value = std::move(stored);
  stored.clear();
  return true;
}
