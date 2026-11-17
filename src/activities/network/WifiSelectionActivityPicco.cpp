// Picco Wi-Fi list (T_Wifi, T_Wifi2): Available networks with signal and lock, then More
// with the hidden network. The radio is off while the list shows (releaseWifiForNetworkList),
// so the board's "Connected" section has no source here.
#include "WifiSelectionActivity.h"

#if CROSSINK_APP_PICCO_UI

#include <GfxRenderer.h>
#include <I18n.h>

#include <vector>

#include "MappedInputManager.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/icons/piccoIcons.h"
#include "components/icons/piccoSystemIcons.h"
#include "components/picco/PiccoSystemLists.h"

namespace {
using PiccoSettingsRows::Row;
using PiccoSettingsRows::Trailing;

const freeink::Icon& signalIcon(const int32_t rssi) {
  switch (PiccoSystemLists::wifiSignalLevel(rssi)) {
    case 3:
      return icon_picco_wifi_3_26;
    case 2:
      return icon_picco_wifi_2_26;
    default:
      return icon_picco_wifi_1_26;
  }
}
}  // namespace

bool WifiSelectionActivity::usesPiccoList() const { return UITheme::isPicco(); }

void WifiSelectionActivity::piccoListLoop() {
  const int scanned = static_cast<int>(realNetworkCount);
  const bool hidden = networks.size() > realNetworkCount;
  const int count = PiccoSystemLists::wifiRowCount(scanned, hidden);
  const auto networkAt = [scanned, hidden](const int row) {
    return PiccoSystemLists::wifiNetworkAt(row, scanned, hidden);
  };
  const auto forget = [this](const int network) {
    if (network < 0 || network >= static_cast<int>(networks.size()) || !networks[network].hasSavedPassword) return;
    selectedNetworkIndex = static_cast<size_t>(network);
    selectedSSID = networks[network].ssid;
    state = WifiSelectionState::FORGET_PROMPT;
    forgetPromptSelection = 0;  // Default to "Cancel"
    requestUpdate();
  };

  // Long-press a saved network to forget it (the Left button does the same on the rocker).
  int x = 0;
  int y = 0;
  int id = -1;
  if (mappedInput.wasScreenLongPress(x, y) && TouchRegistry::getInstance().hitTest(x, y, TouchRegistry::Item, id) &&
      id >= 0 && id < count) {
    forget(networkAt(id));
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    forget(networkAt(piccoSelected));
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    startWifiScan();
    return;
  }

  const auto in = piccoList.input(
      mappedInput, renderer, buttonNavigator, tr(STR_PICCO_WIFI), count, piccoSelected,
      [&networkAt](const int row) { return networkAt(row) >= 0; }, [this] { requestUpdate(); });
  switch (in.event) {
    case PiccoListView::Event::Back:
      mappedInput.suppressNextBackRelease();
      onComplete(false);
      return;
    case PiccoListView::Event::Action:
      startWifiScan();
      return;
    case PiccoListView::Event::Activate:
    case PiccoListView::Event::Button: {
      const int network = networkAt(in.index);
      if (network < 0) return;
      selectedNetworkIndex = static_cast<size_t>(network);
      selectNetwork(network);
      return;
    }
    case PiccoListView::Event::Step:
    case PiccoListView::Event::Dismiss:
    case PiccoListView::Event::None:
      return;
  }
}

void WifiSelectionActivity::renderPiccoList() {
  const int scanned = static_cast<int>(realNetworkCount);
  const bool hidden = networks.size() > realNetworkCount;
  const int count = PiccoSystemLists::wifiRowCount(scanned, hidden);
  // Cold path, one per redraw; labels borrow from `networks`.
  std::vector<Row> rows(count);
  for (int row = 0; row < count; ++row) {
    Row& r = rows[row];
    const int network = PiccoSystemLists::wifiNetworkAt(row, scanned, hidden);
    if (network < 0) {
      r.section = true;
      r.label = row == 0 && scanned > 0 ? tr(STR_PICCO_SECTION_AVAILABLE) : tr(STR_PICCO_SECTION_MORE);
      continue;
    }
    const auto& info = networks[network];
    r.trailing = Trailing::Chevron;
    if (info.isHiddenPlaceholder) {
      r.label = tr(STR_PICCO_WIFI_JOIN_HIDDEN);
      r.hint = tr(STR_PICCO_WIFI_JOIN_HIDDEN_HINT);
      continue;
    }
    r.label = info.ssid.c_str();
    r.leading = &signalIcon(info.rssi);
    r.marker = info.isEncrypted ? &icon_picco_lock_22 : nullptr;
    r.hint = info.hasSavedPassword ? tr(STR_PICCO_WIFI_SAVED) : (info.isEncrypted ? nullptr : tr(STR_PICCO_WIFI_OPEN));
  }
  if (piccoSelected < 0 || piccoSelected >= count ||
      PiccoSystemLists::wifiNetworkAt(piccoSelected, scanned, hidden) < 0) {
    piccoSelected = PiccoSystemLists::wifiRowOf(0, scanned);
  }
  PiccoListView::Frame frame;
  frame.title = tr(STR_PICCO_WIFI);
  frame.action = &icon_picco_sync_26;
  frame.note = scanned == 0 ? tr(STR_NO_NETWORKS) : nullptr;
  piccoList.draw(renderer, mappedInput, frame, rows.data(), count, piccoSelected);
}

#endif
