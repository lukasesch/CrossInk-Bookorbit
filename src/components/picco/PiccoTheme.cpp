#include "PiccoTheme.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

#include "CrossPointSettings.h"
#include "PiccoProgress.h"
#include "PiccoToast.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/piccoIcons.h"
#include "fontIds.h"

namespace {
using PiccoLayout::Box;

constexpr int kTitleFont = UI_12_FONT_ID;
constexpr int kStatusFont = SMALL_FONT_ID;
constexpr int kContextualPadX = 12;
// Fixed samples keep the cluster (and every hit rect left of it) still as values change.
constexpr char kClockSample[] = "88:88";
constexpr char kBatterySample[] = "100%";

// Last measured status cluster, for hit tests that run without a renderer. Written only
// on the render task; read on the input path as plain ints (a stale value only shifts a
// target by a few pixels until the next render).
struct ClusterCache {
  int clockWidth = 0;
  int batteryTextWidth = 44;
};
ClusterCache g_cluster;

Box toBox(const Rect& r) { return Box{r.x, r.y, r.width, r.height}; }

PiccoLayout::Leading toLeading(const HeaderLeading leading) {
  switch (leading) {
    case HeaderLeading::Back:
      return PiccoLayout::Leading::Back;
    case HeaderLeading::House:
      return PiccoLayout::Leading::House;
    case HeaderLeading::None:
      break;
  }
  return PiccoLayout::Leading::None;
}

bool clockShown(const HeaderChrome& chrome) {
  if (!chrome.showStatus || !halClock.canDisplayTime()) return false;
  switch (SETTINGS.hideClock) {
    case CrossPointSettings::HIDE_CLOCK_ALWAYS:
      return false;
    case CrossPointSettings::HIDE_CLOCK_IN_READER:
      return !chrome.readerContext;
    default:
      return true;
  }
}

const char* syncText(const HeaderChrome& chrome) {
  switch (chrome.sync) {
    case HeaderSyncState::Idle:
      return chrome.syncLabel != nullptr ? chrome.syncLabel : "";
    case HeaderSyncState::Syncing:
      return tr(STR_PICCO_SYNCING);
    case HeaderSyncState::Offline:
      return tr(STR_PICCO_OFFLINE);
    case HeaderSyncState::Hidden:
      break;
  }
  return nullptr;
}

int titleFontFor(const GfxRenderer& renderer, const HeaderChrome& chrome) {
  return chrome.filenameTitle ? renderer.filenameFontId(kTitleFont) : kTitleFont;
}

int textTop(const GfxRenderer& renderer, const int fontId, const int centerY) {
  return centerY - renderer.getLineHeight(fontId) / 2;
}

void drawIconCentered(const GfxRenderer& renderer, const freeink::Icon& icon, const Box& box) {
  drawLucideIcon(renderer, icon, box.x + (box.width - icon.w) / 2, box.y + (box.height - icon.h) / 2);
}

// Battery glyph from the board's Lucide battery at 28 x 22: 17 x 10 body, 2 px stroke, nub.
void drawBattery(const GfxRenderer& renderer, const Box& box, const uint16_t percentage) {
  constexpr int kBodyW = 17;
  constexpr int kBodyH = 10;
  const int bx = box.x + 5;
  const int by = box.y + (box.height - kBodyH) / 2;
  renderer.drawRoundedRect(bx, by, kBodyW, kBodyH, 2, 2, true);
  renderer.fillRect(bx + kBodyW, by + 3, 2, kBodyH - 6, true);
  constexpr int kInnerW = kBodyW - 6;
  const int fill = gpio.isUsbConnected() ? kInnerW : std::clamp<int>(percentage, 0, 100) * kInnerW / 100;
  if (fill > 0) renderer.fillRect(bx + 3, by + 3, fill, kBodyH - 6, true);

  char text[8];
  std::snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(percentage));
  const int textX = box.x + PiccoTokens::kHeaderBatteryIconW + PiccoTokens::kHeaderBatteryGap;
  renderer.drawText(kStatusFont, textX, textTop(renderer, kStatusFont, box.y + box.height / 2), text);
}
}  // namespace

PiccoLayout::HeaderLayout PiccoTheme::headerLayout(const GfxRenderer& renderer, const Rect& header,
                                                   const HeaderChrome& chrome) {
  PiccoLayout::HeaderInputs in;
  in.leading = toLeading(chrome.leading);
  const int titleFont = titleFontFor(renderer, chrome);
  in.titleWidth = chrome.title != nullptr ? renderer.getTextWidth(titleFont, chrome.title, EpdFontFamily::BOLD) : 0;
  in.titleHeight = renderer.getLineHeight(titleFont);
  in.titleRightReserve = chrome.titleRightReserve;
  if (chrome.contextualIcon != nullptr) {
    in.contextualWidth = PiccoTokens::kHeaderControl;
  } else if (chrome.contextualLabel != nullptr && chrome.contextualLabel[0] != '\0') {
    in.contextualWidth = std::max(PiccoTokens::kHeaderControl,
                                  renderer.getTextWidth(kStatusFont, chrome.contextualLabel) + 2 * kContextualPadX);
  }
  if (const char* sync = syncText(chrome)) {
    in.syncWidth = PiccoTokens::kIconSync + PiccoTokens::kHeaderIconTextGap + renderer.getTextWidth(kStatusFont, sync);
  }
  in.clockWidth = clockShown(chrome) ? renderer.getTextWidth(kStatusFont, kClockSample) : 0;
  in.batteryTextWidth = renderer.getTextWidth(kStatusFont, kBatterySample);
  g_cluster.clockWidth = in.clockWidth;
  g_cluster.batteryTextWidth = in.batteryTextWidth;
  return PiccoLayout::headerLayout(toBox(header), in);
}

PiccoLayout::HeaderLayout PiccoTheme::cachedHeaderLayout(const Rect& header, const HeaderLeading leading,
                                                         const bool hasContextual) {
  PiccoLayout::HeaderInputs in;
  in.leading = toLeading(leading);
  in.contextualWidth = hasContextual ? PiccoTokens::kHeaderControl : 0;
  in.clockWidth = g_cluster.clockWidth;
  in.batteryTextWidth = g_cluster.batteryTextWidth;
  return PiccoLayout::headerLayout(toBox(header), in);
}

int PiccoTheme::statusClusterReserve(const bool hasContextual) {
  // Gutter + battery/clock, plus the divider and its gaps when an action sits left of it.
  const int cluster = PiccoLayout::statusClusterWidth(0, g_cluster.clockWidth, g_cluster.batteryTextWidth, 0);
  return PiccoTokens::kGutter + cluster +
         (hasContextual ? PiccoTokens::kHairline + 2 * PiccoTokens::kHeaderClusterGap : 0);
}

void PiccoTheme::drawHeader(const GfxRenderer& renderer, const Rect rect, const char* title, const char* subtitle,
                            const bool readerContext, const bool showStatus, const bool filenameTitle) const {
  HeaderChrome chrome;
  chrome.title = title;
  chrome.contextualLabel = subtitle;
  chrome.readerContext = readerContext;
  chrome.showStatus = showStatus;
  chrome.filenameTitle = filenameTitle;
  drawHeaderChrome(renderer, rect, chrome);
}

void PiccoTheme::drawHeaderChrome(const GfxRenderer& renderer, const Rect rect, const HeaderChrome& chrome) const {
  const auto layout = headerLayout(renderer, rect, chrome);
  renderer.fillRect(layout.hairline.x, layout.hairline.y, layout.hairline.width, layout.hairline.height, true);

  switch (chrome.leading) {
    case HeaderLeading::Back:
      drawIconCentered(renderer, icon_picco_chevron_left_26, layout.leadingIcon);
      break;
    case HeaderLeading::House:
      drawIconCentered(renderer, icon_picco_house_28, layout.leadingIcon);
      break;
    case HeaderLeading::None:
      break;
  }

  if (chrome.title != nullptr && chrome.title[0] != '\0' && layout.title.width > 0) {
    const int font = titleFontFor(renderer, chrome);
    const std::string visible = renderer.truncatedText(font, chrome.title, layout.title.width, EpdFontFamily::BOLD);
    renderer.drawText(font, layout.title.x, textTop(renderer, font, layout.centerY), visible.c_str(), true,
                      EpdFontFamily::BOLD);
  }

  if (!layout.contextual.empty()) {
    if (chrome.contextualIcon != nullptr) {
      drawIconCentered(renderer, *chrome.contextualIcon, layout.contextual);
    } else if (chrome.contextualLabel != nullptr) {
      const int w = renderer.getTextWidth(kStatusFont, chrome.contextualLabel);
      renderer.drawText(kStatusFont, layout.contextual.x + (layout.contextual.width - w) / 2,
                        textTop(renderer, kStatusFont, layout.centerY), chrome.contextualLabel);
    }
  }
  if (!layout.divider.empty()) {
    renderer.fillRect(layout.divider.x, layout.divider.y, layout.divider.width, layout.divider.height, true);
  }
  if (const char* sync = syncText(chrome); sync != nullptr && !layout.sync.empty()) {
    const auto& icon = chrome.sync == HeaderSyncState::Offline ? icon_picco_wifi_off_22 : icon_picco_sync_22;
    drawLucideIcon(renderer, icon, layout.sync.x, layout.centerY - icon.h / 2);
    renderer.drawText(kStatusFont, layout.sync.x + icon.w + PiccoTokens::kHeaderIconTextGap,
                      textTop(renderer, kStatusFont, layout.centerY), sync);
  }

  if (!chrome.showStatus) return;
  if (!layout.clock.empty()) {
    char clock[12];
    if (halClock.formatCurrentTime(clock, sizeof(clock), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
      const int w = renderer.getTextWidth(kStatusFont, clock);
      renderer.drawText(kStatusFont, layout.clock.right() - w, textTop(renderer, kStatusFont, layout.centerY), clock);
    }
  }
  drawBattery(renderer, layout.battery, powerManager.getBatteryPercentage());
}

void PiccoTheme::drawSubHeader(const GfxRenderer& renderer, const Rect rect, const char* label,
                               const char* rightLabel) const {
  const int left = rect.x + PiccoTokens::kGutter;
  int right = rect.x + rect.width - PiccoTokens::kGutter;
  const int y = rect.y + std::max(0, (rect.height - renderer.getLineHeight(kStatusFont)) / 2);
  if (rightLabel != nullptr && rightLabel[0] != '\0') {
    const int w = renderer.getTextWidth(kStatusFont, rightLabel);
    renderer.drawText(kStatusFont, right - w, y, rightLabel);
    right -= w + PiccoTokens::kGroupGap;
  }
  std::string caps = label != nullptr ? label : "";
  std::transform(caps.begin(), caps.end(), caps.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
  const std::string visible =
      renderer.truncatedText(kStatusFont, caps.c_str(), std::max(0, right - left), EpdFontFamily::BOLD);
  renderer.drawText(kStatusFont, left, y, visible.c_str(), true, EpdFontFamily::BOLD);
}

Rect PiccoTheme::drawPopup(const GfxRenderer& renderer, const char* message, const bool preserveBackdrop) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect header = UITheme::getHeaderRect(renderer, metrics.headerHeight);
  const Rect toast = PiccoToast::rect(renderer, header.y + header.height, PiccoLayout::ToastKind::Info);

  // Only the toast rectangle is retained, briefly, as in BaseTheme::drawPopup.
  std::unique_ptr<uint8_t[]> backdrop;
  size_t bytes = 0;
  if (preserveBackdrop) {
    bytes = renderer.getRegionByteSize(toast.x, toast.y, toast.width, toast.height);
    if (bytes != 0) backdrop = makeUniqueNoThrow<uint8_t[]>(bytes);
    if (!backdrop || !renderer.copyRegionToBuffer(toast.x, toast.y, toast.width, toast.height, backdrop.get(), bytes)) {
      LOG_ERR("GUI", "Unable to preserve toast backdrop");
      return toast;
    }
  }

  PiccoToast::Content content;
  content.title = message;
  PiccoToast::draw(renderer, toast, content);
  renderer.displayBuffer();
  if (backdrop) renderer.copyBufferToRegion(toast.x, toast.y, toast.width, toast.height, backdrop.get(), bytes);
  return toast;
}

void PiccoTheme::fillPopupProgress(const GfxRenderer& renderer, const Rect& layout, const int progress) const {
  const int pad = PiccoTokens::kToastPadLeft;
  const Rect bar{layout.x + pad, layout.y + layout.height - pad / 2 - PiccoTokens::kProgressFill - 4,
                 std::max(0, layout.width - 2 * pad), PiccoTokens::kProgressFill};
  PiccoProgress::draw(renderer, bar, std::clamp(progress, 0, 100) * 10);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

void PiccoTheme::drawTextField(const GfxRenderer& renderer, const Rect rect, const int textWidth, const bool cursorMode,
                               const int contentStartX, const int contentWidth) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  constexpr int kPadY = 8;
  const int innerW = contentWidth > 0 ? contentWidth : textWidth;
  const int innerX = rect.x + (contentWidth > 0 ? contentStartX : (rect.width - textWidth) / 2);
  const int x0 = std::max(0, innerX - PiccoTokens::kTextFieldPadX);
  const int x1 = std::min(renderer.getScreenWidth(), innerX + innerW + PiccoTokens::kTextFieldPadX);
  const int y0 = rect.y - kPadY;
  // Bottom where BaseTheme draws its underline, so the box encloses every text line.
  const int y1 = rect.y + rect.height + renderer.getLineHeight(UI_12_FONT_ID) + metrics.verticalSpacing;
  const int height = std::max(PiccoTokens::kTextFieldHeight, y1 - y0);
  const int thickness = cursorMode ? metrics.textFieldCursorThickness : metrics.textFieldNormalThickness;
  renderer.drawRoundedRect(x0, y0, std::max(0, x1 - x0), height, thickness, PiccoTokens::kSmallRadius, true);
}

OptionPopupRowPaint PiccoTheme::paintOptionPopupRow(const GfxRenderer& renderer, const Rect row, const bool selected,
                                                    const bool disabled) const {
  // Selection is a check mark and weight, never a fill.
  OptionPopupRowPaint paint;
  paint.invertText = true;
  paint.bold = selected && !disabled;
  if (paint.bold) {
    const auto& icon = icon_picco_check_22;
    drawLucideIcon(renderer, icon, row.x + row.width - icon.w - PiccoTokens::kNeighbourGap,
                   row.y + (row.height - icon.h) / 2);
  }
  return paint;
}

#endif
