#pragma once

#include <cstdint>

// Which overlay a reader's openReaderMenu() shows. Drawer is the menu of the other themes
// (the EPUB drawer, the XTC menu list); Sheet is the Picco reader sheet.
enum class ReaderMenuKind : uint8_t { Drawer, Sheet };

constexpr ReaderMenuKind readerMenuKindFor(const bool piccoTheme) {
  return piccoTheme ? ReaderMenuKind::Sheet : ReaderMenuKind::Drawer;
}

// CROSSINK_APP_PICCO_UI comes from the build flags (AppCapabilities.h defaults it to 0).
#if defined(CROSSINK_APP_PICCO_UI) && CROSSINK_APP_PICCO_UI
#include "components/UITheme.h"

inline ReaderMenuKind readerMenuKind() { return readerMenuKindFor(UITheme::isPicco()); }
#else
constexpr ReaderMenuKind readerMenuKind() { return ReaderMenuKind::Drawer; }
#endif
