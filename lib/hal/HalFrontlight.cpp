#include "HalFrontlight.h"

#include <Logging.h>

HalFrontlight HalFrontlight::instance;

void HalFrontlight::begin(const uint8_t brightness, const uint8_t warmth, const bool on) {
  if (!manager.present()) {
    return;
  }
  manager.begin();
  lastBrightness = brightness > 100 ? 100 : brightness;
  lit = on;
#if FREEINK_DEVICE_PICCO
  // Warmth applied at brightness 0 is an off write, which would put the LED driver
  // in standby right before the on write.
  manager.setBrightness(lit ? lastBrightness : 0);
  manager.setColorTemperature(warmth > 100 ? 100 : warmth);
#else
  manager.setColorTemperature(warmth > 100 ? 100 : warmth);
  manager.setBrightness(lit ? lastBrightness : 0);
#endif
  LOG_INF("LIGHT", "Frontlight up: %u%% warm=%u%% %s", lastBrightness, manager.colorTemperature(), lit ? "on" : "off");
}

void HalFrontlight::setBrightness(const uint8_t percent) {
  lastBrightness = percent > 100 ? 100 : percent;
  if (lit) {
    manager.setBrightness(lastBrightness);
  }
}

void HalFrontlight::setWarmth(const uint8_t warmPercent) {
  manager.setColorTemperature(warmPercent > 100 ? 100 : warmPercent);
}

void HalFrontlight::setOn(const bool on) {
  if (on == lit) {
    return;
  }
  lit = on;
  manager.setBrightness(lit ? lastBrightness : 0);
}
