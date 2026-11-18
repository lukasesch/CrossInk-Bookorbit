#include "QrDisplayActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/QrUtils.h"

#if CROSSINK_APP_PICCO_UI
#include "components/picco/PiccoTokens.h"
#endif

void QrDisplayActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void QrDisplayActivity::onExit() { Activity::onExit(); }

void QrDisplayActivity::loop() {
  int x = 0;
  int y = 0;
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm) || mappedInput.wasScreenTapped(x, y)) {
    finish();
    return;
  }
}

void QrDisplayActivity::render(RenderLock&&) {
  renderer.clearScreen();
#if CROSSINK_APP_PICCO_UI
  // Picco header and the framed code of T_QrPage; any tap or button closes, as before.
  if (UITheme::isPicco()) {
    const Rect header = UITheme::getHeaderRect(renderer, PiccoTokens::kHeaderHeight);
    HeaderChrome chrome;
    chrome.title = tr(STR_PICCO_QR_TITLE);
    chrome.leading = HeaderLeading::Back;
    GUI.drawHeaderChrome(renderer, header, chrome);
    const int side = std::min(renderer.getScreenWidth() - 2 * PiccoTokens::kGutter,
                              renderer.getScreenHeight() - (header.y + header.height) - 2 * PiccoTokens::kGutter);
    const int x = (renderer.getScreenWidth() - side) / 2;
    const int y = header.y + header.height + PiccoTokens::kGutter;
    renderer.drawRoundedRect(x, y, side, side, PiccoTokens::kHairline, PiccoTokens::kSmallRadius, true);
    constexpr int kPad = 20;
    QrUtils::drawQrCode(renderer, Rect(x + kPad, y + kPad, side - 2 * kPad, side - 2 * kPad), textPayload);
    renderer.displayBuffer();
    return;
  }
#endif
  auto metrics = UITheme::getInstance().getMetrics();
  const auto safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);

  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware()) {
    TouchHeaderBackButton::draw(renderer, header, tr(STR_DISPLAY_QR), false);
  } else {
    GUI.drawHeader(renderer, header, tr(STR_DISPLAY_QR), nullptr);
  }

  const int availableWidth = safe.width - 40;
  const int startY = TouchHeaderBackButton::contentTop(renderer, mappedInput) + metrics.verticalSpacing;
  const int availableHeight = safe.y + safe.height - startY - metrics.verticalSpacing - 40;

  const Rect qrBounds(safe.x + 20, startY, availableWidth, availableHeight);
  QrUtils::drawQrCode(renderer, qrBounds, textPayload);

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4, true);

  renderer.displayBuffer();
}
