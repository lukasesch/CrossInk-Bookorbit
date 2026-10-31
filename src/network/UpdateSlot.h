#pragma once

#include <esp_ota_ops.h>

// The slot an update may write, or nullptr when there is none. With one OTA slot
// esp_ota_get_next_update_partition() answers with the running slot, and the raw
// flasher would erase the code it is executing.
inline const esp_partition_t* updateSlot() {
  const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
#ifndef SIMULATOR
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (next && (!running || next->address == running->address)) return nullptr;
#endif
  return next;
}
