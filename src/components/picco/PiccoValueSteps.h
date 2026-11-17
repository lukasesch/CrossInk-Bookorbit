#pragma once

#include <cstdint>

// Pure value ladders for Picco steppers and value pickers (T_SettingsDisplay "Sleep after").
// No renderer, no settings: native-tested.
namespace PiccoValueSteps {

// Sleep-after ladder in minutes; the last entry is the "never" sentinel
// (CrossPointSettings::SLEEP_TIMEOUT_NEVER_MINUTES).
constexpr uint16_t kSleepMinutes[] = {1, 2, 3, 5, 10, 15, 20, 30, 31};
constexpr int kSleepMinutesCount = sizeof(kSleepMinutes) / sizeof(kSleepMinutes[0]);

// Next ladder value from `current` in `direction` (+1 up, -1 down), clamped at both ends.
// A value between two entries steps to the neighbour on that side.
constexpr uint16_t step(const uint16_t* ladder, const int count, const uint16_t current, const int direction) {
  if (count <= 0) return current;
  if (direction > 0) {
    for (int i = 0; i < count; ++i) {
      if (ladder[i] > current) return ladder[i];
    }
    return ladder[count - 1];
  }
  for (int i = count - 1; i >= 0; --i) {
    if (ladder[i] < current) return ladder[i];
  }
  return ladder[0];
}

// Index of the ladder entry closest at or above `current` (the picker's checked row).
constexpr int indexFor(const uint16_t* ladder, const int count, const uint16_t current) {
  for (int i = 0; i < count; ++i) {
    if (ladder[i] >= current) return i;
  }
  return count > 0 ? count - 1 : 0;
}

// Enum stepper: display index moved by one, clamped (no wrap: "-" at the first value stays).
constexpr int stepIndex(const int index, const int count, const int direction) {
  if (count <= 0) return 0;
  const int next = index + (direction > 0 ? 1 : -1);
  return next < 0 ? 0 : (next >= count ? count - 1 : next);
}

}  // namespace PiccoValueSteps
