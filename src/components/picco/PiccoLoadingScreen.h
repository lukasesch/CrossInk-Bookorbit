#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include <string>

class GfxRenderer;
struct RecentBook;

// The Picco screen while a book opens (board L_B). Title, author and cover come from the
// book's recent-books entry; a book without one shows its file name and the text cover.
namespace PiccoLoadingScreen {
// Picco theme in portrait; elsewhere the loading popup stays.
bool applies(const GfxRenderer& renderer);
// The cover thumbnail at exactly the screen's cover size (Home generates it).
std::string coverPath(const RecentBook& book);
// Clears the screen, draws it and refreshes the panel. indexing adds the status line.
void draw(const GfxRenderer& renderer, const char* bookPath, bool indexing);
// Repaints only the progress bar and status line, then fast-refreshes.
void drawProgress(const GfxRenderer& renderer, int percent);
}  // namespace PiccoLoadingScreen

#endif
