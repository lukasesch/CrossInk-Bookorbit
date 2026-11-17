// Picco look for text entry (T_Login): the field (or the whole sign-in form) under the
// header, the keyboard as a band at the bottom with 43 x 56 keys. Editing, cursor and
// key handling stay in KeyboardEntryActivity.cpp; this file only lays out and draws.
#include "KeyboardEntryActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <BidiUtils.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstring>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/picco/PiccoSystemLayout.h"
#include "fontIds.h"

namespace fui = freeink::ui;
using PiccoLayout::Box;

namespace {
constexpr fui::ActionId ACTION_KEY = 1;  // mirrors KeyboardEntryActivity.cpp
constexpr int kLabelFont = SMALL_FONT_ID;
constexpr int kToggleGap = 12;
constexpr int kLeadMaxLines = 3;

fui::Rect toFui(const Box& b) {
  return fui::Rect{static_cast<int16_t>(b.x), static_cast<int16_t>(b.y), static_cast<int16_t>(b.width),
                   static_cast<int16_t>(b.height)};
}

Box screenBox(const GfxRenderer& renderer) {
  const auto safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  return Box{0, 0, renderer.getScreenWidth(), safe.y + safe.height};
}

int headerBottom(const GfxRenderer& renderer, const MappedInputManager& input) {
  const Rect header = TouchHeaderBackButton::headerRect(renderer, input);
  return header.y + header.height;
}
}  // namespace

fui::Rect KeyboardEntryActivity::piccoKeyboardRect() const {
  return toFui(PiccoKeyboardLayout::band(screenBox(renderer), currentLayout().rowCount));
}

KeyboardEntryActivity::PiccoField KeyboardEntryActivity::piccoField(const std::string& displayText) const {
  using namespace PiccoTokens;
  const Box screen = screenBox(renderer);
  const int fieldWidth = screen.width - 2 * kGutter;
  const bool isPassword = inputType == InputType::Password;
  const int toggleW = isPassword ? std::max(renderer.getTextWidth(kLabelFont, tr(STR_PICCO_SHOW)),
                                            renderer.getTextWidth(kLabelFont, tr(STR_HIDE)))
                                 : 0;
  const int toggleReserve = isPassword ? toggleW + kToggleGap : 0;
  const int maxLineWidth = std::max(1, fieldWidth - 2 * kTextFieldPadX - toggleReserve);

  // Wrapped lines grow the frame; measureRange needs a mutable copy.
  std::string s = displayText;
  int lines = 1;
  for (int start = 0; start < static_cast<int>(s.length());) {
    const int end = lineBreakEnd(s, start, maxLineWidth);
    if (end >= static_cast<int>(s.length())) break;
    start = end;
    ++lines;
  }

  PiccoKeyboardLayout::FormInputs in;
  in.headerBottom = headerBottom(renderer, mappedInput);
  const bool hasForm = !form.fields.empty();
  if (hasForm && !form.lead.empty()) {
    in.leadLines =
        static_cast<int>(renderer.wrappedText(kLabelFont, form.lead.c_str(), fieldWidth, kLeadMaxLines).size());
  }
  in.labelHeight = hasForm ? PiccoStatsLayout::kLabelLine : 0;
  in.fieldCount = hasForm ? static_cast<int>(form.fields.size()) : 1;
  in.editing = hasForm ? form.editing : 0;
  in.editingExtra = PiccoKeyboardLayout::extraHeightForLines(lines, inputLineHeight);
  in.keyboardTop = keyboardRect().y;
  const auto layout = PiccoKeyboardLayout::formLayout(screen, in);
  const Box frame = layout.fields[std::max(0, std::min(in.editing, layout.fieldCount - 1))];
  const Box text = PiccoKeyboardLayout::fieldText(frame, inputLineHeight, toggleReserve);

  PiccoField out;
  out.frame = toFui(frame);
  out.text = fui::Rect{static_cast<int16_t>(text.x), static_cast<int16_t>(text.y), static_cast<int16_t>(maxLineWidth),
                       static_cast<int16_t>(inputLineHeight)};
  if (isPassword) {
    const int toggleRight = frame.right() - kTextFieldPadX;
    out.toggle =
        fui::Rect{static_cast<int16_t>(toggleRight - toggleW - kToggleGap / 2), static_cast<int16_t>(frame.y),
                  static_cast<int16_t>(toggleW + kToggleGap / 2 + kTextFieldPadX), static_cast<int16_t>(frame.height)};
  }
  out.maxLineWidth = maxLineWidth;
  return out;
}

KeyboardEntryActivity::InputFieldTouchTarget KeyboardEntryActivity::piccoInputFieldTouchTarget(const int x, const int y,
                                                                                               size_t& position) const {
  if (y >= keyboardRect().y) return InputFieldTouchTarget::None;
  std::string displayText = displayTextForCurrentState();
  const PiccoField field = piccoField(displayText);
  if (!field.toggle.empty() && field.toggle.contains(static_cast<int16_t>(x), static_cast<int16_t>(y))) {
    return InputFieldTouchTarget::PasswordToggle;
  }
  if (!field.frame.contains(static_cast<int16_t>(x), static_cast<int16_t>(y))) return InputFieldTouchTarget::None;

  const int lineHeight = std::max(1, inputLineHeight);
  const int wanted = std::max(0, (y - field.text.y) / lineHeight);
  int lineStart = 0;
  int lineEnd = 0;
  for (int line = 0;; ++line) {
    lineEnd = lineBreakEnd(displayText, lineStart, field.maxLineWidth);
    if (line >= wanted || lineEnd >= static_cast<int>(displayText.length())) break;
    lineStart = lineEnd;
  }
  const bool isRtl = rangeIsRtl(displayText, lineStart, lineEnd);
  const int lineWidth = measureRange(displayText, lineStart, lineEnd);
  const int lineX = field.text.x;
  if (x <= lineX) {
    position = static_cast<size_t>(isRtl ? lineEnd : lineStart);
    return InputFieldTouchTarget::Cursor;
  }
  if (x >= lineX + lineWidth) {
    position = static_cast<size_t>(isRtl ? lineStart : lineEnd);
    return InputFieldTouchTarget::Cursor;
  }
  int previousWidth = 0;
  for (int i = lineStart; i < lineEnd;) {
    const int next = static_cast<int>(utf8Next(displayText, static_cast<size_t>(i)));
    const int nextWidth = measureRange(displayText, lineStart, next);
    const int halfAdvance = (nextWidth - previousWidth) / 2;
    const int midpoint = isRtl ? lineX + lineWidth - previousWidth - halfAdvance : lineX + previousWidth + halfAdvance;
    if ((isRtl && x >= midpoint) || (!isRtl && x < midpoint)) {
      position = static_cast<size_t>(i);
      return InputFieldTouchTarget::Cursor;
    }
    previousWidth = nextWidth;
    i = next;
  }
  position = static_cast<size_t>(lineEnd);
  return InputFieldTouchTarget::Cursor;
}

void KeyboardEntryActivity::renderPicco() {
  using namespace PiccoTokens;
#if CROSSINK_APP_CAP_TOUCH
  const uint32_t feedback = pendingFeedback.load();
#endif
  renderer.clearScreen();

  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  TouchHeaderBackButton::draw(renderer, header, title.c_str(), false);

  std::string displayText = displayTextForCurrentState();
  const PiccoField field = piccoField(displayText);
  const bool hasForm = !form.fields.empty();
  const Box screen = screenBox(renderer);
  const int width = screen.width - 2 * kGutter;

  // The rest of the form around the edited field: lead, labels, the other values.
  if (hasForm) {
    PiccoKeyboardLayout::FormInputs in;
    in.headerBottom = header.y + header.height;
    const auto leadLines = form.lead.empty()
                               ? std::vector<std::string>{}
                               : renderer.wrappedText(kLabelFont, form.lead.c_str(), width, kLeadMaxLines);
    in.leadLines = static_cast<int>(leadLines.size());
    in.labelHeight = PiccoStatsLayout::kLabelLine;
    in.fieldCount = static_cast<int>(form.fields.size());
    in.editing = form.editing;
    in.editingExtra = field.frame.height - kTextFieldHeight;
    in.keyboardTop = keyboardRect().y;
    const auto layout = PiccoKeyboardLayout::formLayout(screen, in);
    if (layout.leadShown) {
      int y = layout.lead.y;
      const int lh = renderer.getLineHeight(kLabelFont);
      for (const auto& line : leadLines) {
        renderer.drawText(kLabelFont, layout.lead.x, y + (PiccoKeyboardLayout::kLeadLine - lh) / 2, line.c_str());
        y += PiccoKeyboardLayout::kLeadLine;
      }
    }
    const int labelH = renderer.getLineHeight(kLabelFont);
    const int valueH = renderer.getLineHeight(inputFontId);
    for (int i = 0; i < layout.fieldCount; ++i) {
      const Box& label = layout.labels[i];
      const std::string labelText = renderer.truncatedText(kLabelFont, form.fields[i].label.c_str(), label.width);
      renderer.drawText(kLabelFont, label.x, label.y + (label.height - labelH) / 2, labelText.c_str());
      if (i == form.editing) continue;
      const Box& f = layout.fields[i];
      renderer.drawRoundedRect(f.x, f.y, f.width, f.height, kHairline, kSmallRadius, true);
      std::string value = form.fields[i].value;
      if (form.fields[i].masked) value.assign(value.size(), '*');
      value = renderer.truncatedText(inputFontId, value.c_str(), f.width - 2 * kTextFieldPadX);
      renderer.drawText(inputFontId, f.x + kTextFieldPadX, f.y + (f.height - valueH) / 2, value.c_str());
    }
  }

  // The edited field: 2 px frame (it holds the focus), wrapped text, cursor.
  renderer.drawRoundedRect(field.frame.x, field.frame.y, field.frame.width, field.frame.height, kSelectionStroke,
                           kSmallRadius, true);
  const bool isPassword = inputType == InputType::Password;
  const int lineHeight = inputLineHeight;
  const size_t cursorCharBytes = (cursorPos < text.length()) ? utf8Next(text, cursorPos) - cursorPos : 0;
  char cursorChar[8] = {};
  if (cursorCharBytes > 0)
    memcpy(cursorChar, text.data() + cursorPos, std::min(cursorCharBytes, sizeof(cursorChar) - 1));
  int cursorX = field.text.x;
  int cursorY = field.text.y;
  int lineStart = 0;
  int lineY = field.text.y;
  while (true) {
    const int lineEnd = lineBreakEnd(displayText, lineStart, field.maxLineWidth);
    const std::string line = displayText.substr(lineStart, lineEnd - lineStart);
    renderer.drawText(inputFontId, field.text.x, lineY, line.c_str());
    const bool last = lineEnd >= static_cast<int>(displayText.length());
    if (cursorPos >= static_cast<size_t>(lineStart) &&
        (last ? cursorPos <= static_cast<size_t>(lineEnd) : cursorPos < static_cast<size_t>(lineEnd))) {
      const int before = measureRange(displayText, lineStart, static_cast<int>(cursorPos));
      cursorX = rangeIsRtl(displayText, lineStart, lineEnd)
                    ? field.text.x + measureRange(displayText, lineStart, lineEnd) - before
                    : field.text.x + before;
      cursorY = lineY;
    }
    if (last) break;
    lineStart = lineEnd;
    lineY += lineHeight;
  }
  if (cursorMode && !togglePos) {
    // Button cursor mode: a block over the character, as on the other themes.
    const int charW = cursorCharBytes > 0 ? std::max(6, renderer.getTextWidth(inputFontId, cursorChar)) : 6;
    renderer.fillRect(cursorX - 1, cursorY, charW + 2, lineHeight, true);
    if (cursorCharBytes > 0 && (!isPassword || passwordVisible)) {
      renderer.drawText(inputFontId, cursorX, cursorY, cursorChar, false);
    }
  } else {
    renderer.fillRect(cursorX, cursorY + 2, kSelectionStroke, std::max(1, lineHeight - 4), true);
  }

  if (isPassword && !field.toggle.empty()) {
    const char* label = passwordVisible ? tr(STR_HIDE) : tr(STR_PICCO_SHOW);
    const int w = renderer.getTextWidth(kLabelFont, label);
    const int x = field.frame.x + field.frame.width - kTextFieldPadX - w;
    const int y = field.frame.y + (kTextFieldHeight - renderer.getLineHeight(kLabelFont)) / 2;
    const bool selected = cursorMode && togglePos;
    if (selected) renderer.fillRect(x - 3, y - 1, w + 6, renderer.getLineHeight(kLabelFont) + 2, true);
    renderer.drawText(kLabelFont, x, y, label, !selected);
  }

  // Keyboard band: hairline on top, keys drawn by the FreeInkUI keyboard component.
  const fui::Rect kbRect = keyboardRect();
  renderer.fillRect(kbRect.x, kbRect.y, kbRect.width, kHairline, true);
  const Box keys = PiccoKeyboardLayout::keysRect(Box{kbRect.x, kbRect.y, kbRect.width, kbRect.height});

  fui::GfxRendererTarget target(renderer);
  target.setFont(fui::GfxRendererTarget::FONT_SMALL, SMALL_FONT_ID);
  target.setFont(fui::GfxRendererTarget::FONT_BODY, UI_10_FONT_ID);
  const fui::DeviceContext device = target.deviceContext();
  const fui::InputSnapshot noInput{};
#if CROSSINK_APP_CAP_TOUCH
  auto& frameInteractions = paintInteractions;
  frameInteractions.clearFlash();
  if (feedback && !cursorMode && !buttonSelectionVisible.load()) {
    frameInteractions.setFlash(ACTION_KEY, static_cast<int16_t>(feedback & 0xFFFF));
  }
#else
  auto& frameInteractions = interactions;
  frameInteractions.beginPublishCycle();
#endif
  fui::Frame<56> frame(target, device, noInput, frameInteractions);

  fui::KeyboardProps props;
  const fui::KeyboardLayout& layout = currentLayout();
  props.layout = &layout;
  props.keyAction = ACTION_KEY;
  props.okLabel = hasForm && !form.okLabel.empty() ? form.okLabel.c_str() : tr(STR_OK);
  props.modeLabel =
      (symbols || (inputType == InputType::Url && urlPanel)) ? tr(STR_KEY_MODE_ABC) : tr(STR_KEY_MODE_SYMBOLS);
  props.inputMask = static_cast<uint16_t>(fui::InputTouch | fui::InputLongPress);
  props.selectedIndex =
      (cursorMode || !buttonSelectionVisible.load()) ? -1 : static_cast<int16_t>(selectedLogicalIndex());
  props.labelText.font = layoutId == fui::KeyboardLayoutId::ArabicAr && !symbols ? fui::GfxRendererTarget::FONT_SMALL
                                                                                 : fui::GfxRendererTarget::FONT_BODY;
  props.altText.font = fui::GfxRendererTarget::FONT_SMALL;
  props.controlText.font = fui::GfxRendererTarget::FONT_SMALL;
  props.padding = fui::Insets{0, 0, 0, 0};
  props.gap = static_cast<int16_t>(kKeyGap);
  props.rowGap = static_cast<int16_t>(PiccoKeyboardLayout::kRowGap);
  props.keyRadius = static_cast<uint8_t>(kKeyRadius);
  props.background = fui::Paint::none();
  if (urlPanel) props.uniformKeyWidth = false;
  props.bottomHitOverflow = static_cast<int16_t>(std::max(0, kbRect.bottom() - keys.bottom()));
  fui::keyboard(frame, toFui(keys), props);
#if CROSSINK_APP_CAP_TOUCH
  interactions.beginPublishCycle();
  interactions.clear();
  for (size_t i = 0; i < frameInteractions.count(); ++i) {
    interactions.addInteraction(frameInteractions.data()[i]);
  }
#endif
  interactions.publish();
  interactionsReady.store(true, std::memory_order_release);

  const auto labels = mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), tr(STR_SELECT), tr(STR_DIR_LEFT),
                                            tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
#if CROSSINK_APP_CAP_TOUCH
  uint32_t expected = feedback;
  if (feedback && pendingFeedback.compare_exchange_strong(expected, 0)) {
    requestUpdate();
  }
#endif
}

#endif
