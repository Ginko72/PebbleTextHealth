#include <pebble.h>
#include "settings.h"

#define KEY_TICK_PERSISTENCE 0

void settings_load(Settings *s) {
  s->tick_persistence_ms = persist_exists(KEY_TICK_PERSISTENCE)
      ? (uint16_t)(persist_read_int(KEY_TICK_PERSISTENCE) * 1000)
      : 3000;
}

void settings_save(const Settings *s) {
  persist_write_int(KEY_TICK_PERSISTENCE, s->tick_persistence_ms / 1000);
}
