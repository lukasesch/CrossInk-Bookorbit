#pragma once

#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include <atomic>
#include <vector>

#include "FrontlightPanelModel.h"
#include "activities/Activity.h"
#include "components/OptionPopup.h"
#include "util/ButtonNavigator.h"
#if CROSSINK_APP_PICCO_UI
#include <Memory.h>

#include <array>
#include <memory>

#include "activities/reader/EpubReaderMenuModel.h"
#include "activities/reader/ReaderSettingsController.h"
#include "components/picco/sheet/PiccoSheetModel.h"
#endif

struct Rect;

// Frontlight quick panel (boards with a frontlight + home key, e.g. X4 Pro).
// Opened globally by the top-edge down-swipe: a top-anchored drop-down (only the
// upper third of the screen) with brightness and warmth sliders driving the
// light live, plus an on/off toggle with a frontlight icon. It renders as an overlay —
// the content underneath stays on screen below the panel, and a tap there
// dismisses it. State is persisted once on exit (SPIFFS write throttling).
class FrontlightPanelActivity final : public Activity {
  // The sun toggle, two sliders, and four -/+ targets need seven interaction
  // slots. Keep small headroom for the panel's fixed controls.
  using UiApp = freeink::ui::FreeInkApp<20, 7>;

  ButtonNavigator buttonNavigator;

  freeink::ui::GfxRendererTarget uiTarget;  // must precede `app`: the app holds a reference to it
  UiApp app;
  std::atomic<bool> uiReady{false};

  uint8_t brightness = 60;
  uint8_t warmth = 50;
  bool observedLightOn = false;  // redraw tracking only; the HAL owns live on/off state
  FrontlightPanelContext context;
  FrontlightDrawerState drawerState{};
  OptionPopup optionPopup;
  bool initialInversion = false;
  bool initialTouchscreenDisabled = false;
  bool previousTouchscreenRecoveryOverride = false;
  bool ttfRenderingChanged = false;
  // Swallow the swipe/tap fallout of a slider drag so its release can't
  // trigger the back gesture and close the panel mid-adjustment.
  bool draggingSlider = false;
  // Bottom edge of the drop-down (px). Content lays out above it and the
  // handle occupies its final band. Set by render() before the app lays out.
  int panelBottom = 0;
  int16_t panelRowHeight = 0;
  int16_t panelSpaceSm = 0;
  int16_t panelSpaceLg = 0;
  freeink::ui::Rect drawerHandleRect{};
  std::vector<std::string> readerTitleLines;
  // Cleared by computePanelBottom() when the chapter line cannot fit beside
  // the controls (e.g. landscape at Large scale).
  bool showChapterLine = false;

  static void panelScreen(UiApp::ScreenType& screen, void* user);
  static void onBrightnessEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onWarmthEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onToggleEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onBrightnessStepEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onWarmthStepEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onQuickActionEvent(const freeink::ui::ActionEvent& event, void* user);
  static void onDismissEvent(const freeink::ui::ActionEvent& event, void* user);
  void buildPanelScreen(UiApp::ScreenType& screen);
  void addStepSlider(UiApp::ScreenType& screen, const freeink::ui::Rect& row, uint8_t value,
                     freeink::ui::ActionId sliderAction, freeink::ui::ActionId stepAction);
  Rect homeButtonRect() const;
  // Height of the drop-down, derived from the content it holds (header +
  // sliders + toggle). Same layout math as buildPanelScreen so the frame,
  // content margin, and dismiss threshold all agree.
  int computePanelBottom();
  void prepareReaderDetailsLayout();
  void drawReaderDetails(freeink::ui::Screen<20>& screen);
  void adjustBrightness(int delta);
  void adjustWarmth(int delta);
  void toggleLight();
  void toggleReaderTouchscreen();
  void close();
  void activateQuickAction(int index);
  void openSyncDialog();
  void closeSyncDialog();
  void openReadingStats();
  void openGlobalSettings();
  void drawHeader();
  bool showsBookProgress() const;
  const char* bookSummaryChapter() const;
  int bookSummaryHeight() const;
  void drawBookProgress();

#if CROSSINK_APP_PICCO_UI
  // ---- Picco reader sheet (FrontlightPanelSheet.cpp) ----
  // Portrait PICCO theme only; landscape keeps the panel above.
  enum class SheetHit : uint8_t {
    HeaderLeading,
    HeaderSync,
    HeaderContextual,
    Tab,
    GoToBlock,
    LightToggle,
    LightSlider,
    Tile,
    Chip,
    Minus,
    Plus,
    Segment,
    Switch,
    Value,
    Cell,
    Button,
    GoToSlider,
    GoToStep,
    GoToKey,
    Pill,
  };
  struct SheetTarget {
    PiccoLayout::Box box;
    SheetHit hit = SheetHit::Cell;
    uint8_t id = 0;     // row, tile, tab or light row
    int16_t value = 0;  // step delta, segment value, key index
    int8_t stop = -1;   // rocker focus stop
  };
  struct SheetStop {
    PiccoLayout::Box box;
    uint8_t radius = 14;
    uint8_t target = 0;       // first target of the stop
    bool adjustable = false;  // Confirm enters editing, Up/Down change the value
  };
  static constexpr size_t kMaxSheetTargets = 48;
  static constexpr size_t kMaxSheetStops = 32;

  bool piccoSheet = false;
  PiccoSheet::Variant sheetVariant = PiccoSheet::Variant::Outside;
  PiccoSheet::Pane sheetPane = PiccoSheet::Pane::Main;
  uint8_t sheetTab = 0;
  std::unique_ptr<ReaderSettingsController> sheetSettings;
  // Written by render (under its RenderLock), read by loop under a RenderLock.
  std::array<SheetTarget, kMaxSheetTargets> sheetTargets{};
  std::array<SheetStop, kMaxSheetStops> sheetStops{};
  uint8_t sheetTargetCount = 0;
  uint8_t sheetStopCount = 0;
  int sheetBottom = 0;
  int sheetFocus = -1;
  bool sheetRing = false;
  ReaderButtonSliderState sheetSlider{};
  bool sheetDragging = false;  // a slider follows the finger until it lifts
  SheetTarget sheetDragTarget{};
  PiccoSheet::GoToEntry goToEntry{};
  bool goToPercent = true;
  uint16_t autoTurnSeconds = 30;
  bool dictionariesReady = false;
  bool sheetHasBookOrbit = false;
  bool sheetOffline = false;
  uint32_t sheetSyncedAt = 0;
  // The page under the sheet, so a shorter pane uncovers it in the same refresh.
  // One framebuffer copy (48 KB, PSRAM on the Picco); without it the sheet never shrinks.
  HeapByteBuffer sheetBackdrop;
  bool sheetBackdropSaved = false;
  int sheetPaintedBottom = 0;

  void sheetEnter();
  void sheetExit();
  void sheetLoop();
  void sheetRender();
  void sheetDraw();
  void sheetDrawHeader();
  void sheetDrawMain();
  void sheetDrawBlocks();
  void sheetDrawGoTo();
  void sheetDrawAutoTurn();
  void sheetDrawLight(const PiccoSheet::LightRow& row, int index);
  void sheetSetPane(PiccoSheet::Pane pane);
  void sheetActivate(const SheetTarget& target);
  void sheetAdjust(int stop, int delta);
  void sheetSlide(const SheetTarget& target, int x);
  void sheetActivateRow(PiccoSheet::Row row);
  void sheetShowRowOptions(PiccoSheet::Row row);
  void sheetShowFonts();
  void sheetShowDictionaries();
  void sheetRequest(int16_t action, int32_t value = -1, uint32_t page = 0, bool reopen = false);
  void sheetFinish(const ReaderSheetRequest& request, FrontlightPanelAction action = FrontlightPanelAction::None);
  void sheetBack();
  void sheetOpenGoTo(bool percent);
  PiccoSheet::Availability sheetAvailability() const;
  PiccoSheet::Tiles sheetTiles() const;
  int sheetHeight() const;
  int sheetAddTarget(const PiccoLayout::Box& box, SheetHit hit, uint8_t id = 0, int16_t value = 0, int stop = -1);
  int sheetAddStop(const PiccoLayout::Box& box, uint8_t radius, bool adjustable);
  const char* sheetRowLabel(PiccoSheet::Row row) const;
  const char* sheetRowHint(PiccoSheet::Row row) const;
  const char* sheetRowValue(PiccoSheet::Row row, char* buffer, size_t size) const;
  bool sheetRowOn(PiccoSheet::Row row) const;
  bool sheetRowEnabled(PiccoSheet::Row row) const;
#endif

 public:
  explicit FrontlightPanelActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                   FrontlightPanelContext context = {});
  void onEnter() override;
  void onExit() override;
  // Keep the visible sliders and exit-time save in sync with a global edge action.
  void onExternalFrontlightChange() override;
  void loop() override;
  void render(RenderLock&&) override;
  // Let loop() dismiss handle swipes even when the drawer reaches the bottom
  // edge. Keep the physical Home control separate from swipe dismissal.
  bool allowGlobalHomeSwipeGesture() const override { return false; }
  // From an active reader, the Home control returns to the library; elsewhere
  // it dismisses the overlay back to the current screen.
  bool handleHomeGesture() override;
  bool requiresFreshBackdrop() const override { return true; }
#if defined(SIMULATOR) && CROSSINK_APP_PICCO_UI
  bool simulatorIsSheet() const { return piccoSheet; }
  void simulatorShowSheetPane(const PiccoSheet::Pane pane, const uint8_t tab) {
    sheetSetPane(pane);
    sheetTab = tab;
  }
#endif
#ifdef SIMULATOR
  freeink::ui::Rect simulatorQuickActionRect(int index) const;
  freeink::ui::Rect simulatorHandleRect() const { return drawerHandleRect; }
  bool simulatorShowsChapterLine() const { return showChapterLine; }
  bool simulatorShowsReaderDetails() const { return context.showReaderDetails; }
  Rect simulatorHomeButtonRect() const { return homeButtonRect(); }
  Rect simulatorSyncOptionRect(const int index) const { return optionPopup.simulatorOptionRect(index); }
  bool simulatorSyncOptionDisabled(const int index) const { return optionPopup.simulatorOptionDisabled(index); }
  int simulatorContentBottom = 0;
  int simulatorActionBarTop = 0;
  void simulatorActivateQuickAction(int index) { activateQuickAction(index); }
#endif
};
