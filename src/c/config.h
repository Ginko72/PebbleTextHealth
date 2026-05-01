////////////////////////////////////
// Watch Face Variants
#define TimeRenderOh  true
#define DateFormatUS  true

// General config
#define DEBUG       false
#ifndef DEBUG_SECONDS_ALWAYS_ON
#define DEBUG_SECONDS_ALWAYS_ON false
#else
#undef DEBUG_SECONDS_ALWAYS_ON
#define DEBUG_SECONDS_ALWAYS_ON true
#endif

#define BUFFER_SIZE 44

// Complete strftime format string (day-name prefix + date)
#if DateFormatUS
#define DateFormat "%a %m.%d"
#else
#define DateFormat "%a %d.%m"
#endif

// Per-platform layout configuration, populated by layout_config_init()
#include <pebble.h>
typedef struct {
  uint32_t time_font_id;       // Large time font resource ID
  uint32_t time2_font_id;      // Medium time font resource ID
  uint32_t date_font_id;       // Date font resource ID
  uint32_t batt_font_id;       // Battery font resource ID
  int      time_height;        // Line 1 text layer height (px)
  int      time2_height;       // Lines 2 & 3 text layer height (px)
  int      block_height;       // Total 3-line time block height (px)
  int      block_y;            // Vertical center of the time block (px)
  int      corner_inset;       // Horizontal inset for round bezel; 0 on rect
  int      step_width;         // Step ring stroke width (px)
  int      step_outer_rad;     // Step/mask ring outer corner radius (px)
  int      tick_width;         // Tick mark ring stroke width (px)
  int      step_layer_offset;  // Step layer inset from screen edge (px)
  int      tick_outer_rad;     // Tick ring outer corner radius (px)
  int      tick_layer_offset;  // Tick layer inset from screen edge (px)
  int      tick_inner_rad;     // Tick ring inner corner radius (px)
  int      batt_y;             // Battery text layer top y (px)
  int      batt_height;        // Battery text layer height (px)
  int      bt_gap;             // Gap between battery bottom and BT icon top (px)
  int      date_bottom_offset; // Distance from screen bottom to date layer top (px)
  int      date_y;             // Date text layer top y (px)
  int      date_height;        // Date text layer height (px)
  int      time_layer_h;       // Line 1 TextLayer height (px)
  int      time2_layer_h;      // Lines 2 & 3 TextLayer height (px)

  // Seconds ring geometry
  uint16_t seconds_tick_length;  // radial length of each tick mark (px)
  uint16_t seconds_ring_radius;  // round platforms: outer radius of tick ring
  GRect    seconds_ring_rect;    // rect platforms: tight bounds of tick annular band
} LayoutConfig;
