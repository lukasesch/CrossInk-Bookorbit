#ifdef SIMULATOR
#include "PiccoScreens4R2.h"

#include "AppCapabilities.h"

#if CROSSINK_APP_CAP_TOUCH && CROSSINK_APP_PICCO_UI
#include <HalStorage.h>
#include <Logging.h>

#include <cstdlib>
#include <cstring>
#include <memory>

#include "CrossPointSettings.h"
#include "activities/ActivityManager.h"
#include "activities/RenderLock.h"
#include "activities/boot_sleep/SleepActivity.h"
#include "activities/reader/DictionaryDefinitionActivity.h"
#include "activities/reader/EpubReaderFootnoteSelectActivity.h"
#include "activities/util/FrontlightPanelActivity.h"
#include "fontIds.h"

extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

namespace {
using Steps = std::vector<std::function<int()>>;
constexpr int kSettle = 6;
constexpr int kBookOpen = 40;
constexpr const char* kDictPath = "/.crosspoint/picco-4r2-dict";
constexpr char kDefinition[] =
    "A line of shrubs or low trees planted close together, kept trimmed to mark the edge of a garden, field or "
    "path.";

[[noreturn]] void fail(const char* message) {
  LOG_ERR("SMOKE", "Picco screens 4R2: %s", message);
  std::_Exit(2);
}

void expect(const char* activity, const char* what) {
  if (activityManager.isCurrentActivityNamed(activity)) return;
  LOG_ERR("SMOKE", "Picco screens 4R2: %s expected %s", what, activity);
  std::_Exit(2);
}

FrontlightPanelActivity& sheet() {
  auto* panel = dynamic_cast<FrontlightPanelActivity*>(activityManager.simulatorCurrentActivity());
  if (!panel || !panel->simulatorIsSheet()) fail("the reader sheet is not open");
  return *panel;
}

void press(Steps& steps, const MappedInputManager::Button button) {
  steps.push_back([button] {
    mappedInputManager.simulatorInjectPress(button);
    return 0;
  });
  steps.push_back([button] {
    mappedInputManager.simulatorInjectRelease(button);
    return kSettle;
  });
}

void openSheet(Steps& steps) {
  const int x = renderer.getScreenWidth() / 2;
  const int fromY = renderer.getScreenHeight() / 3;
  const int toY = renderer.getScreenHeight() * 2 / 3;
  steps.push_back([x, fromY] {
    mappedInputManager.simulatorInjectTouchDown(x, fromY);
    return 0;
  });
  steps.push_back([x, toY] {
    mappedInputManager.simulatorInjectTouchMove(x, toY);
    return 0;
  });
  steps.push_back([x, toY] {
    mappedInputManager.simulatorInjectTouchRelease(x, toY);
    return kSettle;
  });
  steps.push_back([] {
    expect("FrontlightPanel", "reader swipe down opens the sheet");
    return 0;
  });
}

// The reader page drawn by the definition's background callback: lines of book text.
void drawBookBackground(void*) {
  renderer.clearScreen();
  const int line = renderer.getLineHeight(UI_10_FONT_ID) + 8;
  for (int y = 28; y + line < renderer.getScreenHeight(); y += line) {
    renderer.drawText(UI_10_FONT_ID, 28, y, "She had walked the length of the garden twice");
  }
}
}  // namespace

void addPiccoScreens4R2(Steps& steps, const std::function<void(const std::string&)>& renderAndCapture,
                        const std::function<void(const std::string&)>& capture, const std::string& bookPath) {
  // Word lookup popover with both actions (a clipping waits on the lookup).
  steps.push_back([] {
    if (!Storage.writeFile((std::string(kDictPath) + ".dict").c_str(), kDefinition) ||
        !Storage.writeFile((std::string(kDictPath) + ".ifo").c_str(),
                           "StarDict's dict ifo file\nbookname=English\nsametypesequence=m\n")) {
      fail("cannot write the dictionary");
    }
    DictLocation location;
    location.folderPath = kDictPath;
    location.size = sizeof(kDefinition) - 1;
    location.found = true;
    static DictionaryClippingRequest clipping{};
    static int backgroundContext = 0;
    activityManager.pushActivity(std::make_unique<DictionaryDefinitionActivity>(
        renderer, mappedInputManager, "hedge", location, true, "", false, "", LookupHistory::Status::NotFound,
        &backgroundContext, drawBookBackground, nullptr, 0, false, &clipping));
    return kSettle;
  });
  steps.push_back([renderAndCapture] {
    expect("DictionaryDefinition", "open word lookup");
    renderAndCapture("4r2-word-lookup");
    activityManager.goHome();
    return kSettle;
  });

  // Footnote: a note mark on a page and its popover.
  steps.push_back([] {
    auto page = std::make_unique<Page>();
    FootnoteEntry entry;
    std::strcpy(entry.number, "3");
    std::strcpy(entry.href, "notes.xhtml#n3");
    entry.linkId = 1;
    page->footnotes.push_back(entry);
    FootnoteLinkTargets targets{};
    targets[0] = FootnoteLinkTarget{300, 300, 18, 24};
    activityManager.pushActivity(std::make_unique<EpubReaderFootnoteSelectActivity>(
        renderer, mappedInputManager, std::move(page), targets, SETTINGS.getReaderFontId(), 28, 28));
    return kSettle;
  });
  steps.push_back([renderAndCapture] {
    expect("EpubReaderFootnoteSelect", "open footnote selection");
    renderAndCapture("4r2-footnote");
    activityManager.goHome();
    return kSettle;
  });

  // Clipping selection from the sheet's More pane: a start mark and a few words.
  steps.push_back([bookPath] {
    activityManager.goToReader(bookPath, true);
    return kBookOpen;
  });
  openSheet(steps);
  steps.push_back([] {
    RenderLock lock;
    sheet().simulatorActivateSheetRow(PiccoSheet::Row::SaveClipping);
    return kSettle * 2;
  });
  steps.push_back([] {
    expect("ClipSelection", "Save clipping row");
    return 0;
  });
  press(steps, MappedInputManager::Button::Confirm);
  for (int i = 0; i < 8; ++i) press(steps, MappedInputManager::Button::Right);
  steps.push_back([renderAndCapture] {
    renderAndCapture("4r2-clipping-select");
    return 0;
  });
  press(steps, MappedInputManager::Button::Back);
  press(steps, MappedInputManager::Button::Back);

  // Page as QR: the sheet's More row opens the pane.
  steps.push_back([] {
    expect("EpubReader", "clipping selection cancelled");
    return 0;
  });
  openSheet(steps);
  steps.push_back([] {
    RenderLock lock;
    sheet().simulatorShowSheetPane(PiccoSheet::Pane::More, 0);
    sheet().simulatorActivateSheetRow(PiccoSheet::Row::Qr);
    return kSettle;
  });
  steps.push_back([renderAndCapture] {
    renderAndCapture("4r2-qr-pane");
    RenderLock lock;
    sheet().simulatorShowSheetPane(PiccoSheet::Pane::Main, 0);
    return 0;
  });

  // Sleep in the Minimal mode, which Picco draws as its cover screen.
  steps.push_back([capture, bookPath] {
    activityManager.goHome();
    const uint8_t mode = SETTINGS.sleepScreen;
    SETTINGS.sleepScreen = CrossPointSettings::SLEEP_SCREEN_MODE::MINIMAL_SLEEP;
    {
      SleepActivity sleep(renderer, mappedInputManager, false, bookPath);
      sleep.onEnter();
      capture("4r2-sleep");
    }
    SETTINGS.sleepScreen = mode;
    Storage.remove((std::string(kDictPath) + ".dict").c_str());
    Storage.remove((std::string(kDictPath) + ".ifo").c_str());
    return kSettle;
  });
}

#else

void addPiccoScreens4R2(std::vector<std::function<int()>>&, const std::function<void(const std::string&)>&,
                        const std::function<void(const std::string&)>&, const std::string&) {}

#endif
#endif
