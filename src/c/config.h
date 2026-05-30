////////////////////////////////////
// Watch Face Variants
#define TimeRenderOh  true

// General config
#define DEBUG       false
#ifndef DEBUG_SECONDS_ALWAYS_ON
#define DEBUG_SECONDS_ALWAYS_ON false
#else
#undef DEBUG_SECONDS_ALWAYS_ON
#define DEBUG_SECONDS_ALWAYS_ON true
#endif

#define BUFFER_SIZE 44
// Layout configuration: ring geometry and anchored positions.
// Text heights are measured at runtime and kept as locals in main_window_load.
#include <pebble.h>
typedef struct {
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
  int      bt_gap;             // Gap between battery bottom and BT icon top (px)
  int      date_bottom_offset; // Distance from screen bottom to date layer bottom (px)
  int      date_y;             // Date text layer top y (px)

  // Seconds ring geometry
  uint16_t seconds_tick_length;  // radial length of each tick mark (px)
  uint16_t seconds_ring_radius;  // round platforms: outer radius of tick ring
  GRect    seconds_ring_rect;    // rect platforms: tight bounds of tick annular band
} LayoutConfig;
