#pragma once

#include "AppCapabilities.h"

#if CROSSINK_APP_PICCO_UI

#include "PiccoLibraryLayout.h"
#include "components/themes/BaseTheme.h"

namespace freeink {
struct Icon;
}

// Controls shared by the Library, Files and BookOrbit screens: segmented control (tabs,
// view toggle, order), outlined dropdown button, check rows. Geometry: PiccoLibraryLayout.
namespace PiccoListUi {

inline Rect toRect(const PiccoLayout::Box& b) { return Rect{b.x, b.y, b.width, b.height}; }
inline PiccoLayout::Box toBox(const Rect& r) { return PiccoLayout::Box{r.x, r.y, r.width, r.height}; }

// The segmented control (geometry: PiccoSegmented). Frame (1 px, r16) and the selected
// cell's 2 px stroke (r12); -1 selects nothing.
void drawSegmentedFrame(const GfxRenderer& renderer, const Rect& outer, int count, int selected);
// Frame plus centred labels (the selected one bold), 8 px clear of each cell side.
void drawSegmented(const GfxRenderer& renderer, const Rect& outer, const char* const* labels, int count, int selected,
                   int fontId);
void drawSegmented(const GfxRenderer& renderer, const Rect& outer, const char* const* labels, int count, int selected);
// Frame plus a centred icon per cell (null leaves a cell blank).
void drawSegmentedIcons(const GfxRenderer& renderer, const Rect& outer, const freeink::Icon* const* icons, int count,
                        int selected);

// Outlined button (1 px, r14, or 2 px when focused) with a label and an optional trailing icon.
Rect outlineButtonRect(const GfxRenderer& renderer, int x, int centerY, const char* label, const freeink::Icon* icon);
void drawOutlineButton(const GfxRenderer& renderer, const Rect& rect, const char* label, const freeink::Icon* icon,
                       bool focused = false, bool centered = false);

// Row label with a check at the right when selected (bold), hairline above (T_LibrarySort).
void drawCheckRow(const GfxRenderer& renderer, const Rect& row, const char* label, bool checked, bool focused,
                  bool hairlineAbove);

// Four-square grid glyph (no Lucide grid icon in the firmware's icon set).
void drawGridGlyph(const GfxRenderer& renderer, int x, int y, int size);

// 2 px focus frame around a box, outset into the gutter like a list row's.
void drawFocusFrame(const GfxRenderer& renderer, const Rect& rect, int radius);

// Text top for a font centred on centerY.
int textTop(const GfxRenderer& renderer, int fontId, int centerY);

}  // namespace PiccoListUi

#endif
