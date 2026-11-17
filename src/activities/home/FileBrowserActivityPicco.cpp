// Picco Files screen (T_Files board): breadcrumb band under the header, the list, and the
// USB / Wi-Fi transfer pair above the page indicator. Rows and actions stay in
// FileBrowserActivity.cpp.
#include "FileBrowserActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <string>
#include <vector>

#include "AppCapabilities.h"
#include "activities/ActivityManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/listIcons.h"
#include "components/icons/piccoIcons.h"
#include "components/picco/PiccoLibraryLayout.h"
#include "components/picco/PiccoListUi.h"
#include "fontIds.h"

extern ActivityManager activityManager;

namespace fui = freeink::ui;
using PiccoLayout::Box;

namespace {
constexpr fui::ActionId ACTION_TRANSFER = 3;  // ACTION_SELECT_DIRECTORY's handler
constexpr int TRANSFER_USB = 1;
constexpr int TRANSFER_WIFI = 2;
constexpr int kPathFont = SMALL_FONT_ID;
}  // namespace

void FileBrowserActivity::buildPiccoChrome(UiApp::ScreenType& screen) {
  const Box screenBox{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  // Same header rect the shared back button draws and hit-tests.
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  const int headerBottom = header.y + header.height;
  const Box pathBand = PiccoLibraryLayout::bandBelow(screenBox, headerBottom, PiccoTokens::kActionHeight);

  // Breadcrumb: SD card > dir > ... > current (bold); leading segments drop behind an
  // ellipsis until the rest fits.
  std::vector<std::string> segments;
  segments.emplace_back(tr(STR_SD_CARD));
  for (size_t start = 1; start < basepath.size();) {
    const size_t end = basepath.find('/', start);
    segments.push_back(basepath.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (end == std::string::npos) break;
    start = end + 1;
  }
  const auto& chevron = icon_picco_chevron_right_22;
  constexpr int kGap = 4;
  const int pathFont = renderer.filenameFontId(kPathFont);
  const auto segmentWidth = [&](const size_t i) {
    const auto style = i + 1 == segments.size() ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    return renderer.getTextWidth(pathFont, segments[i].c_str(), style);
  };
  size_t first = 0;
  const char ellipsis[] = "\xe2\x80\xa6";
  const auto totalWidth = [&](const size_t from) {
    int width = from > 0 ? renderer.getTextWidth(pathFont, ellipsis) + 2 * kGap + chevron.w : 0;
    for (size_t i = from; i < segments.size(); ++i) {
      width += segmentWidth(i);
      if (i + 1 < segments.size()) width += 2 * kGap + chevron.w;
    }
    return width;
  };
  while (first + 1 < segments.size() && totalWidth(first) > pathBand.width) ++first;
  const int centerY = pathBand.y + pathBand.height / 2;
  const int textY = PiccoListUi::textTop(renderer, pathFont, centerY);
  int x = pathBand.x;
  const auto drawChevron = [&] {
    x += kGap;
    drawLucideIcon(renderer, chevron, x, centerY - chevron.h / 2);
    x += chevron.w + kGap;
  };
  if (first > 0) {
    renderer.drawText(pathFont, x, textY, ellipsis);
    x += renderer.getTextWidth(pathFont, ellipsis);
    drawChevron();
  }
  for (size_t i = first; i < segments.size(); ++i) {
    const bool last = i + 1 == segments.size();
    const auto style = last ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
    const std::string visible =
        renderer.truncatedText(pathFont, segments[i].c_str(), std::max(0, pathBand.right() - x), style);
    renderer.drawText(pathFont, x, textY, visible.c_str(), true, style);
    x += renderer.getTextWidth(pathFont, visible.c_str(), style);
    if (!last) drawChevron();
  }

  // Transfer pair: USB Drive (when the board has it) and Wi-Fi file transfer.
  Box wifi = PiccoLibraryLayout::transferButton(screenBox, 1);
#if CROSSINK_APP_CAP_USB_DRIVE
  const Box usb = PiccoLibraryLayout::transferButton(screenBox, 0);
  PiccoListUi::drawOutlineButton(renderer, PiccoListUi::toRect(usb), tr(STR_PICCO_USB_TRANSFER), &icon_usb_24, false,
                                 true);
  screen.frame().hit(fui::Rect{static_cast<int16_t>(usb.x), static_cast<int16_t>(usb.y),
                               static_cast<int16_t>(usb.width), static_cast<int16_t>(usb.height)},
                     ACTION_TRANSFER, TRANSFER_USB, fui::InputTouch);
#else
  wifi.width = wifi.right() - PiccoLibraryLayout::transferButton(screenBox, 0).x;
  wifi.x = PiccoLibraryLayout::transferButton(screenBox, 0).x;
#endif
  PiccoListUi::drawOutlineButton(renderer, PiccoListUi::toRect(wifi), tr(STR_PICCO_WIFI_TRANSFER), &icon_wifi_24,
                                 false, true);
  screen.frame().hit(fui::Rect{static_cast<int16_t>(wifi.x), static_cast<int16_t>(wifi.y),
                               static_cast<int16_t>(wifi.width), static_cast<int16_t>(wifi.height)},
                     ACTION_TRANSFER, TRANSFER_WIFI, fui::InputTouch);

  const int listTop = pathBand.bottom() + PiccoLibraryLayout::kBandGap;
  const int listBottom = wifi.y - PiccoLibraryLayout::kBandGap;
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(listTop), 0,
                                                static_cast<int16_t>(screenBox.height - listBottom), 0});
}

void FileBrowserActivity::openPiccoTransfer(const int target) {
  if (target == TRANSFER_USB) {
#if CROSSINK_APP_CAP_USB_DRIVE
    activityManager.goToUsbDrive();
#endif
    return;
  }
  if (target == TRANSFER_WIFI) activityManager.goToFileTransfer();
}

#endif
