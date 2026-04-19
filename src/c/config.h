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
  int      time_height;        // Line 1 text layer height (px)
  int      time2_height;       // Lines 2 & 3 text layer height (px)
  int      block_height;       // Total 3-line time block height (px)
  int      corner_inset;       // Horizontal inset for round bezel; 0 on rect
  int      step_width;         // Step ring stroke width (px)
  int      step_outer_rad;     // Step/mask ring outer corner radius (px)
  int      tick_width;         // Tick mark ring stroke width (px)
  int      step_layer_offset;  // Step layer inset from screen edge (px)
  int      tick_outer_rad;     // Tick ring outer corner radius (px)
  int      tick_layer_offset;  // Tick layer inset from screen edge (px)
  int      tick_inner_rad;     // Tick ring inner corner radius (px)
} LayoutConfig;
