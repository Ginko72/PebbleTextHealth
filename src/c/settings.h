#pragma once
#include <stdint.h>

typedef struct {
  uint16_t backlight_timeout_ms;  // 3000–10000 ms, step 1000
} Settings;

void settings_load(Settings *s);
void settings_save(const Settings *s);
