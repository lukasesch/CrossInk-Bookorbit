// Picco sign-in form (T_Login): server, user name and password with the stored values,
// one Sign in button. A field opens the keyboard over the same form; its Next key walks
// to the next field and the last one signs in. Wi-Fi and authentication are the
// existing flow in BookOrbitAuthActivity.cpp.
#include "BookOrbitAuthActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>
#include <WiFi.h>

#include <algorithm>
#include <utility>

#include "BookOrbitCredentialStore.h"
#include "MappedInputManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/icons/piccoIcons.h"
#include "components/picco/PiccoListUi.h"
#include "components/picco/PiccoPillButton.h"
#include "components/picco/PiccoSystemLayout.h"
#include "components/picco/PiccoToast.h"
#include "fontIds.h"

namespace {
constexpr int kLabelFont = SMALL_FONT_ID;
constexpr int kValueFont = UI_10_FONT_ID;
constexpr int kLeadMaxLines = 3;
constexpr int kServerField = 0;
constexpr int kUserField = 1;
constexpr int kPasswordField = 2;

const char* fieldLabel(const int field) {
  switch (field) {
    case kServerField:
      return tr(STR_PICCO_SERVER_ADDRESS);
    case kUserField:
      return tr(STR_USERNAME);
    default:
      return tr(STR_PASSWORD);
  }
}

std::string fieldValue(const int field) {
  switch (field) {
    case kServerField:
      return BOOKORBIT_STORE.getServerUrl();
    case kUserField:
      return BOOKORBIT_STORE.getUsername();
    default:
      return BOOKORBIT_STORE.getPassword();
  }
}

// Same persistence as the BookOrbit settings rows.
void saveField(const int field, const std::string& value) {
  switch (field) {
    case kServerField:
      BOOKORBIT_STORE.setServerUrl((value == "https://" || value == "http://") ? std::string{} : value);
      break;
    case kUserField:
      BOOKORBIT_STORE.setCredentials(value, BOOKORBIT_STORE.getPassword());
      break;
    default:
      BOOKORBIT_STORE.setCredentials(BOOKORBIT_STORE.getUsername(), value);
      break;
  }
  BOOKORBIT_STORE.saveToFile();
}

bool hit(const Rect& r, const int x, const int y) {
  return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height;
}
}  // namespace

void BookOrbitAuthActivity::openField(const int field) {
  KeyboardForm form;
  form.lead = tr(STR_PICCO_SIGN_IN_LEAD);
  for (int i = 0; i < kPiccoFields; ++i) {
    form.fields.push_back(KeyboardFormField{fieldLabel(i), fieldValue(i), i == kPasswordField});
  }
  form.editing = field;
  form.okLabel = field + 1 < kPiccoFields ? tr(STR_NEXT_FIELD) : tr(STR_PICCO_SIGN_IN);

  std::string initial = fieldValue(field);
  if (field == kServerField && initial.empty()) initial = "https://";
  const InputType type = field == kServerField     ? InputType::Url
                         : field == kPasswordField ? InputType::Password
                                                   : InputType::Text;
  const size_t maxLength = field == kServerField ? 128 : 64;
  auto keyboard = std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_PICCO_CONNECT_BOOKORBIT),
                                                          std::move(initial), maxLength, type);
  keyboard->setForm(std::move(form));
  startActivityForResult(std::move(keyboard), [this, field](const ActivityResult& result) {
    if (result.isCancelled) {
      requestUpdate();
      return;
    }
    saveField(field, std::get<KeyboardResult>(result.data).text);
    if (field + 1 < kPiccoFields) {
      openField(field + 1);
    } else {
      startSignIn();
    }
  });
}

void BookOrbitAuthActivity::startSignIn() {
  if (!BOOKORBIT_STORE.hasCredentials() || BOOKORBIT_STORE.getServerUrl().empty()) {
    {
      RenderLock lock(*this);
      state = FAILED;
      errorMessage = tr(STR_PICCO_SIGN_IN_MISSING);
    }
    requestUpdate();
    return;
  }
  {
    RenderLock lock(*this);
    state = CONNECTING;
    errorMessage.clear();
  }
  if (WiFi.status() == WL_CONNECTED) {
    onWifiSelectionComplete(true);
    return;
  }
  startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput),
                         [this](const ActivityResult& result) { onWifiSelectionComplete(!result.isCancelled); });
}

void BookOrbitAuthActivity::loopPicco() {
  if (state == CONNECTING || state == AUTHENTICATING) return;
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    // Like the other themes: this screen replaced the settings stack, so Back goes home.
    onGoHome();
    return;
  }
  auto activate = [this](const int stop) {
    if (stop < kPiccoFields) {
      openField(stop);
    } else if (state == SUCCESS) {
      onGoHome();
    } else {
      startSignIn();
    }
  };
  int x = 0;
  int y = 0;
  if (mappedInput.wasScreenTapped(x, y)) {
    for (int i = 0; i < kPiccoFields; ++i) {
      if (hit(piccoFieldRects[i], x, y)) {
        activate(i);
        return;
      }
    }
    if (hit(piccoButtonRect, x, y)) activate(kPiccoFields);
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activate(piccoFocus < 0 ? kPiccoFields : piccoFocus);
    return;
  }
  const bool down = mappedInput.wasPressed(MappedInputManager::Button::Down) ||
                    mappedInput.wasPressed(MappedInputManager::Button::Right);
  const bool up = mappedInput.wasPressed(MappedInputManager::Button::Up) ||
                  mappedInput.wasPressed(MappedInputManager::Button::Left);
  if (down || up) {
    {
      RenderLock lock(*this);
      if (piccoFocus < 0) {
        piccoFocus = down ? 0 : kPiccoStops - 1;
      } else {
        piccoFocus = (piccoFocus + (down ? 1 : kPiccoStops - 1)) % kPiccoStops;
      }
    }
    requestUpdate();
  }
}

void BookOrbitAuthActivity::renderPicco() {
  using namespace PiccoTokens;
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  TouchHeaderBackButton::draw(renderer, header, tr(STR_PICCO_CONNECT_BOOKORBIT), false);
  const int headerBottom = header.y + header.height;
  const PiccoLayout::Box screen{0, 0, renderer.getScreenWidth(), renderer.getScreenHeight()};
  const int width = screen.width - 2 * kGutter;

  const auto lead = renderer.wrappedText(kLabelFont, tr(STR_PICCO_SIGN_IN_LEAD), width, kLeadMaxLines);
  PiccoKeyboardLayout::FormInputs in;
  in.headerBottom = headerBottom;
  in.leadLines = static_cast<int>(lead.size());
  in.labelHeight = PiccoStatsLayout::kLabelLine;
  in.fieldCount = kPiccoFields;
  in.editing = -1;
  in.trailingHeight = kPillHeight;
  const auto layout = PiccoKeyboardLayout::formLayout(screen, in);

  const int labelH = renderer.getLineHeight(kLabelFont);
  int y = layout.lead.y;
  for (const auto& line : lead) {
    renderer.drawText(kLabelFont, layout.lead.x, y + (PiccoKeyboardLayout::kLeadLine - labelH) / 2, line.c_str());
    y += PiccoKeyboardLayout::kLeadLine;
  }
  const int valueH = renderer.getLineHeight(kValueFont);
  for (int i = 0; i < kPiccoFields; ++i) {
    const auto& label = layout.labels[i];
    renderer.drawText(kLabelFont, label.x, label.y + (label.height - labelH) / 2, fieldLabel(i));
    const auto& f = layout.fields[i];
    piccoFieldRects[i] = PiccoListUi::toRect(f);
    const bool focused = piccoFocus == i;
    renderer.drawRoundedRect(f.x, f.y, f.width, f.height, focused ? kSelectionStroke : kHairline, kSmallRadius, true);
    std::string value = fieldValue(i);
    if (i == kPasswordField) value.assign(value.size(), '*');
    value = renderer.truncatedText(kValueFont, value.c_str(), f.width - 2 * kTextFieldPadX);
    renderer.drawText(kValueFont, f.x + kTextFieldPadX, f.y + (f.height - valueH) / 2, value.c_str());
  }

  // The screen's one black element; once signed in it leads home.
  piccoButtonRect = PiccoListUi::toRect(layout.trailing);
  PiccoPillButton::draw(renderer, piccoButtonRect, state == SUCCESS ? tr(STR_DONE) : tr(STR_PICCO_SIGN_IN));
  if (piccoFocus == kPiccoFields) {
    PiccoListUi::drawFocusFrame(renderer, piccoButtonRect, kPillRadius);
  }

  // Status over the content, under the header (T_States).
  PiccoToast::Content toast;
  if (state == CONNECTING || state == AUTHENTICATING) {
    toast.kind = PiccoLayout::ToastKind::Info;
    toast.title = state == AUTHENTICATING ? statusMessage.c_str() : tr(STR_CONNECTING);
    toast.icon = &icon_picco_sync_22;
  } else if (state == SUCCESS) {
    toast.kind = PiccoLayout::ToastKind::Info;
    toast.title = tr(STR_AUTH_SUCCESS);
    toast.icon = &icon_picco_check_22;
  } else if (state == FAILED) {
    toast.kind = errorMessage.empty() ? PiccoLayout::ToastKind::Error : PiccoLayout::ToastKind::ErrorTwoLine;
    toast.title = tr(STR_AUTH_FAILED);
    toast.detail = errorMessage.empty() ? nullptr : errorMessage.c_str();
    toast.icon = &icon_picco_alert_22;
  }
  if (toast.title != nullptr) {
    const Rect rect = PiccoToast::rect(renderer, headerBottom, toast.kind);
    renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);
    PiccoToast::draw(renderer, rect, toast);
  }

  renderer.displayBuffer();
}

#endif
