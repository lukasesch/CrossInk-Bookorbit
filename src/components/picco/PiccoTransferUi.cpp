#include "PiccoTransferUi.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>

#include "PiccoSystemLayout.h"
#include "PiccoToast.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/icons/piccoIcons.h"
#include "fontIds.h"

namespace PiccoTransferUi {
namespace {
constexpr int kTitleFont = UI_12_FONT_ID;
constexpr int kDetailFont = SMALL_FONT_ID;
constexpr int kTextGap = 10;
}  // namespace

int contentTop(const GfxRenderer& renderer) {
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  return header.y + header.height;
}

bool drawHeader(const GfxRenderer& renderer, const char* title, const bool canGoBack) {
  if (!UITheme::isPicco()) return false;
  const Rect header = TouchHeaderBackButton::compactHeaderRect(renderer);
  HeaderChrome chrome;
  chrome.title = title;
  chrome.leading = canGoBack ? HeaderLeading::Back : HeaderLeading::None;
  GUI.drawHeaderChrome(renderer, header, chrome);
  return true;
}

bool drawStatus(const GfxRenderer& renderer, const char* message, const char* detail, const bool error) {
  if (!UITheme::isPicco()) return false;
  const int top = contentTop(renderer);
  if (error) {
    PiccoToast::Content toast;
    toast.kind = PiccoLayout::ToastKind::ErrorTwoLine;
    toast.title = message;
    toast.detail = detail;
    toast.icon = &icon_picco_alert_22;
    PiccoToast::draw(renderer, PiccoToast::rect(renderer, top, toast.kind), toast);
    return true;
  }
  const PiccoLayout::Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const auto band = PiccoTransferLayout::statusBand(screen, top);
  renderer.fillRect(band.x, band.y, band.width, PiccoTokens::kHairline, true);
  renderer.fillRect(band.x, band.bottom() - 1, band.width, PiccoTokens::kHairline, true);
  const int textW = band.width - 2 * PiccoTransferLayout::kStatusPadX;
  const auto titleLines = renderer.wrappedText(kTitleFont, message, textW, 2, EpdFontFamily::BOLD);
  const auto detailLines =
      detail != nullptr ? renderer.wrappedText(kDetailFont, detail, textW, 3) : std::vector<std::string>{};
  const int titleLh = renderer.getLineHeight(kTitleFont);
  const int detailLh = renderer.getLineHeight(kDetailFont);
  const int blockH = static_cast<int>(titleLines.size()) * titleLh +
                     (detailLines.empty() ? 0 : kTextGap + static_cast<int>(detailLines.size()) * detailLh);
  int y = band.y + (band.height - blockH) / 2;
  for (const auto& line : titleLines) {
    const int w = renderer.getTextWidth(kTitleFont, line.c_str(), EpdFontFamily::BOLD);
    renderer.drawText(kTitleFont, band.x + (band.width - w) / 2, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += titleLh;
  }
  if (!detailLines.empty()) y += kTextGap;
  for (const auto& line : detailLines) {
    const int w = renderer.getTextWidth(kDetailFont, line.c_str());
    renderer.drawText(kDetailFont, band.x + (band.width - w) / 2, y, line.c_str());
    y += detailLh;
  }
  return true;
}

}  // namespace PiccoTransferUi

#endif
