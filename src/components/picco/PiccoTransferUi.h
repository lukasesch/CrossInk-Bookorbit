#pragma once

#include "AppCapabilities.h"

class GfxRenderer;

// Picco chrome for the transfer screens (USB drive, Wi-Fi file transfer): the shared
// header and the T_States status band / error frame. Every call returns false on the
// other themes, so callers keep their own drawing behind a single check.
namespace PiccoTransferUi {

#if CROSSINK_APP_PICCO_UI
// Header with the back chevron when the screen can be left, else the title alone.
bool drawHeader(const GfxRenderer& renderer, const char* title, bool canGoBack);
// Status band (hairlines above and below, centred title and detail) or, for an error,
// the 2 px error frame under the header.
bool drawStatus(const GfxRenderer& renderer, const char* message, const char* detail, bool error);
// Top of the content under the header.
int contentTop(const GfxRenderer& renderer);
#else
inline bool drawHeader(const GfxRenderer&, const char*, bool) { return false; }
inline bool drawStatus(const GfxRenderer&, const char*, const char*, bool) { return false; }
inline int contentTop(const GfxRenderer&) { return 0; }
#endif

}  // namespace PiccoTransferUi
