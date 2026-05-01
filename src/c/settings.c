#include <pebble.h>
#include "settings.h"

#define KEY_TICK_PERSISTENCE 0
#define KEY_STEP_TARGET      1
#define KEY_DATE_FORMAT_US   2

void settings_load(Settings *s) {
  s->tick_persistence_ms = persist_exists(KEY_TICK_PERSISTENCE)
      ? (uint16_t)(persist_read_int(KEY_TICK_PERSISTENCE) * 1000)
      : 3000;
  s->step_target = persist_exists(KEY_STEP_TARGET)
      ? (uint16_t)persist_read_int(KEY_STEP_TARGET)
      : 10000;
  s->date_format_is_us = persist_exists(KEY_DATE_FORMAT_US)
      ? persist_read_bool(KEY_DATE_FORMAT_US)
      : true;
}

void settings_save(const Settings *s) {
  persist_write_int(KEY_TICK_PERSISTENCE, s->tick_persistence_ms / 1000);
  persist_write_int(KEY_STEP_TARGET, s->step_target);
  persist_write_bool(KEY_DATE_FORMAT_US, s->date_format_is_us);
}
