////////////////////////////////////
// Watch Face Variants
#define TimeRenderOh  true
#define DateFormatUS  true

// General config
#define DEBUG       false
#define BUFFER_SIZE 44

// Complete strftime format string (day-name prefix + date)
#if DateFormatUS
#define DateFormat "%a %m.%d"
#else
#define DateFormat "%a %d.%m"
#endif

// Per-platform layout configuration, populated by layout_config_init()
#include <stdint.h>
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
  int      bt_replaces_batt;   // 1 on small screens: BT icon replaces battery layer
} LayoutConfig;
