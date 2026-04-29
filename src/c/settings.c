#include <pebble.h>
#include "settings.h"

#define KEY_BACKLIGHT_TIMEOUT 0

void settings_load(Settings *s) {
  s->backlight_timeout_ms = persist_exists(KEY_BACKLIGHT_TIMEOUT)
      ? (uint16_t)(persist_read_int(KEY_BACKLIGHT_TIMEOUT) * 1000)
      : 3000;
}

void settings_save(const Settings *s) {
  persist_write_int(KEY_BACKLIGHT_TIMEOUT, s->backlight_timeout_ms / 1000);
}
