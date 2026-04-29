#pragma once
#include <stdint.h>

typedef struct {
  uint16_t tick_persistence_ms;  // 3000–60000 ms, step 1000
} Settings;

void settings_load(Settings *s);
void settings_save(const Settings *s);
