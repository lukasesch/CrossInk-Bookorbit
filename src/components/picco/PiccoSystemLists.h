#pragma once

#include <cstdint>

// Pure row models of the Picco system lists (T_Wifi, T_HyphenationPacks, T_Fonts). No
// renderer: native-tested.
namespace PiccoSystemLists {

constexpr int kSection = -1;

// ---- Wi-Fi (T_Wifi, T_Wifi2) ----------------------------------------------------------------

// Arcs on the signal icon: 3 strong, 2 fair, 1 weak (the legacy bars without the 4th).
constexpr int wifiSignalLevel(const int32_t rssi) { return rssi >= -60 ? 3 : (rssi >= -70 ? 2 : 1); }

// Rows: "Available" + the scanned networks (when any), then "More" + the hidden-network row
// (when the list carries one). Networks [0, scanned) are scanned; network `scanned` is hidden.
constexpr int wifiRowCount(const int scanned, const bool hidden) {
  return (scanned > 0 ? 1 + scanned : 0) + (hidden ? 2 : 0);
}

// Network index shown on `row`; kSection for a section label.
constexpr int wifiNetworkAt(const int row, const int scanned, const bool hidden) {
  const int available = scanned > 0 ? 1 + scanned : 0;
  if (row < 0 || row >= wifiRowCount(scanned, hidden)) return kSection;
  if (row < available) return row == 0 ? kSection : row - 1;
  return row == available ? kSection : scanned;
}

constexpr int wifiRowOf(const int network, const int scanned) {
  if (network < scanned) return 1 + network;
  return (scanned > 0 ? 1 + scanned : 0) + 1;
}

// ---- Installed / available split (T_HyphenationPacks, T_Fonts) -------------------------------

// Writes item indices to `order` as: kSection, installed items, kSection, the rest (a group
// without items drops its label). Returns the row count (at most count + 2).
inline int splitInstalled(const bool* installed, const int count, int16_t* order) {
  int rows = 0;
  for (int pass = 0; pass < 2; ++pass) {
    const bool wantInstalled = pass == 0;
    bool labelled = false;
    for (int i = 0; i < count; ++i) {
      if (installed[i] != wantInstalled) continue;
      if (!labelled) {
        order[rows++] = kSection;
        labelled = true;
      }
      order[rows++] = static_cast<int16_t>(i);
    }
  }
  return rows;
}

// Which group a section row opens: true for the installed group.
inline bool sectionIsInstalled(const int16_t* order, const int rows, const int row, const bool* installed) {
  return row + 1 < rows && order[row + 1] >= 0 && installed[order[row + 1]];
}

}  // namespace PiccoSystemLists
