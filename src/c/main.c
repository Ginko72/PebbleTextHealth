#include <pebble.h>
#include <ctype.h>

#include "num2words-en.h"
#include "config.h"
#include "geometry.h"
#include "settings.h"

extern uint32_t MESSAGE_KEY_TICK_PERSISTENCE;
extern uint32_t MESSAGE_KEY_STEP_TARGET;

static Settings s_settings;

static Window    *s_main_window;

// Time display layers
static TextLayer *s_time_layer;
static TextLayer *s_time2_layer;
static TextLayer *s_time3_layer;
static TextLayer *s_date_layer;

// Custom fonts
static GFont s_time_font;
static GFont s_time2_font;
static GFont s_date_font;
static GFont s_batt_font;

// Battery
static TextLayer *s_batt_layer;

// Tick marks layer and layout
static Layer       *s_tick_layer;
static LayoutConfig s_layout;

// Bluetooth
static BitmapLayer *s_bt_icon_layer;
static GBitmap     *s_bt_icon_bitmap;

// Steps and step ring (health-capable platforms only)
#if defined(PBL_HEALTH)
static Layer *s_step_layer;
static Layer *s_step_mask_layer;
static int    s_step_level;      // 0: <target, 1: 2×target, 2: 3×target, 3: ≥3×target
static int32_t s_step_angle;     // Angle for the current lap progress
static int    s_steps;
static int    s_step_target;     // configured daily goal (steps)
#endif

// Seconds ring — hidden unless backlight is active
static Layer  *s_seconds_ring_layer;
static GPoint  s_tick_starts[60];
static GPoint  s_tick_ends[60];

// Uncomment to test time (makes time static)
// #define TESTING_TIME   // 12:17 for width and 8:28 for height
// #define TESTING_H 12
// #define TESTING_M 17

// Uncomment to test step count (makes static)
// #define TESTING_STEPS 11250

// Uncomment for seconds testing
// #undef DEBUG_SECONDS_ALWAYS_ON
// #define DEBUG_SECONDS_ALWAYS_ON true

// Initialize layout configuration based on screen size and shape
static void layout_config_init(LayoutConfig *cfg, GRect bounds) {
  if (bounds.size.w >= 200) {
    cfg->time_font_id  = RESOURCE_ID_FONT_AMIKO_BOLD_46;
    cfg->time_height   = 54;
    cfg->time_layer_h  = 60;
    cfg->time2_font_id = RESOURCE_ID_FONT_AMIKO_REGULAR_32;
    cfg->time2_height  = 42;
    cfg->time2_layer_h = 50;
    cfg->date_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_22;
    cfg->date_height   = 22;
    cfg->batt_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_22;
    cfg->batt_height   = 22;
    cfg->block_y       = bounds.size.h / 2 - 10;
  } else if (bounds.size.w >= 180) {
    cfg->time_font_id  = RESOURCE_ID_FONT_AMIKO_BOLD_38;
    cfg->time_height   = 46;
    cfg->time_layer_h  = 54;
    cfg->time2_font_id = RESOURCE_ID_FONT_AMIKO_REGULAR_24;
    cfg->time2_height  = 26;
    cfg->time2_layer_h = 34;
    cfg->date_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_16;
    cfg->date_height   = 22;
    cfg->batt_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_16;
    cfg->batt_height   = 22;
    cfg->block_y       = bounds.size.h / 2 - 8;
  } else {
    cfg->time_font_id  = RESOURCE_ID_FONT_AMIKO_BOLD_38;
    cfg->time_height   = 46;
    cfg->time_layer_h  = 54;
    cfg->time2_font_id = RESOURCE_ID_FONT_AMIKO_REGULAR_24;
    cfg->time2_height  = 26;
    cfg->time2_layer_h = 34;
    cfg->date_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_16;
    cfg->date_height   = 16;
    cfg->batt_font_id  = RESOURCE_ID_FONT_AMIKO_REGULAR_16;
    cfg->batt_height   = 16;
    cfg->block_y       = bounds.size.h / 2 - 6;
  }
  cfg->block_height      = cfg->time_height + 2 * cfg->time2_height;
  cfg->corner_inset      = PBL_IF_ROUND_ELSE(bounds.size.w/10, 0);
  cfg->step_width        = 4;
  cfg->step_outer_rad    = 30;
  cfg->tick_width        = 4;
  cfg->step_layer_offset = 0;
  cfg->tick_outer_rad    = cfg->step_outer_rad - cfg->step_width;
  cfg->tick_layer_offset = cfg->step_layer_offset + cfg->step_width;
  cfg->tick_inner_rad    = cfg->tick_outer_rad - cfg->tick_width;
  cfg->batt_y             = 12 + cfg->corner_inset/6;
  cfg->bt_gap             = 3;
  cfg->date_bottom_offset = 14 + cfg->date_height + cfg->corner_inset/2;
  cfg->date_y             = bounds.size.h - cfg->date_bottom_offset;

  // Seconds ring geometry
  cfg->seconds_tick_length = (uint16_t)(4 + bounds.size.w / 72);  // 4px (144px) → 7px (260px)
  #if defined(PBL_ROUND)
  cfg->seconds_ring_radius = (uint16_t)(bounds.size.w / 2 - cfg->step_layer_offset - cfg->step_width - 2);
  cfg->seconds_ring_rect   = GRectZero;
  #else
  // Seconds ring sits just inside the step ring's inner edge
  int sri = cfg->step_layer_offset + cfg->step_width + 2;
  cfg->seconds_ring_rect = GRect(sri, sri,
                                 bounds.size.w - 2 * sri,
                                 bounds.size.h - 2 * sri);
  cfg->seconds_ring_radius = 0;
  #endif
}


// Update Time
static void update_time() {
  static char s_textLine1[BUFFER_SIZE];
  static char s_textLine2[BUFFER_SIZE];
  static char s_textLine3[BUFFER_SIZE];

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  #ifdef TESTING_TIME
  tick_time->tm_hour = TESTING_H;
  tick_time->tm_min = TESTING_M;
  #endif

  time_to_3words(tick_time->tm_hour, tick_time->tm_min, s_textLine1, s_textLine2, s_textLine3, BUFFER_SIZE);
  #if DEBUG
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line1: %s", s_textLine1);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line2: %s", s_textLine2);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line3: %s", s_textLine3);
  #endif

  text_layer_set_text(s_time_layer, s_textLine1);
  text_layer_set_text(s_time2_layer, s_textLine2);
  text_layer_set_text(s_time3_layer, s_textLine3);

  // Date String
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), DateFormat, tick_time);
  // Lowercase 1st char of day
  s_date_buffer[0] = tolower((unsigned char)s_date_buffer[0]);
  // Remove leading zeros. Process index 7 first to preserve index 4.
  if (s_date_buffer[7] == '0') memmove(&s_date_buffer[7], &s_date_buffer[8], sizeof(s_date_buffer) - 8);
  if (s_date_buffer[4] == '0') memmove(&s_date_buffer[4], &s_date_buffer[5], sizeof(s_date_buffer) - 5);
  text_layer_set_text(s_date_layer, s_date_buffer);
}


#if defined(PBL_HEALTH)
// Update Steps
static void update_steps() {
  // Get raw steps
  #ifdef TESTING_STEPS
  s_steps = TESTING_STEPS;
  #else
  s_steps = (int)health_service_sum_today(HealthMetricStepCount);
  #endif

  s_step_level = s_steps / s_step_target;
  int remainder = s_steps % s_step_target;
  
  if (s_step_level >= 3) {
    s_step_level = 3;
    s_step_angle = TRIG_MAX_ANGLE;
  } else {
    #if defined(PBL_ROUND)
    s_step_angle = (int32_t)remainder * TRIG_MAX_ANGLE / s_step_target;
    #else
    GRect bounds = layer_get_bounds(window_get_root_layer(s_main_window));
    GRect step_rect = GRect(s_layout.step_layer_offset, s_layout.step_layer_offset,
                            bounds.size.w - 2 * s_layout.step_layer_offset,
                            bounds.size.h - 2 * s_layout.step_layer_offset);
    uint16_t perim = rounded_rect_perimeter(step_rect, (uint16_t)s_layout.tick_outer_rad);
    uint16_t dist = (uint16_t)((uint32_t)remainder * perim / s_step_target);
    GPoint pt = rounded_rect_perimeter_point(step_rect, (uint16_t)s_layout.tick_outer_rad, perim, dist);
    s_step_angle = trig_angle_from_center(pt, GPoint(bounds.size.w / 2, bounds.size.h / 2));
    #endif
  }

  layer_mark_dirty(s_step_layer);
}


// Health Handler
static void health_handler(HealthEventType event, void *context) {
  switch (event) {
    case HealthEventSignificantUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, "New HealthService HealthEventSignificantUpdate event");
      update_steps();
      break;
    case HealthEventMovementUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, "New HealthService HealthEventMovementUpdate event");
      update_steps();
      break;
    case HealthEventSleepUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, "New HealthService HealthEventSleepUpdate event");
      break;
    case HealthEventHeartRateUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, "New HealthService HealthEventHeartRateUpdate event");
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_INFO, "New HealthService default event");
      break;
  }
}
#endif // PBL_HEALTH


// Tick Handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  #if defined(PBL_HEALTH)
  update_steps();
  #endif
}


// Battery change callback
static void battery_callback(BatteryChargeState state) {
  static char s_textLine[BUFFER_SIZE];

  snprintf(s_textLine, BUFFER_SIZE, "%d%%", state.charge_percent);
  text_layer_set_text(s_batt_layer, s_textLine);
}


#if defined(PBL_HEALTH)
// Steps Mask Layer update proc
static void step_mask_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  #if defined(PBL_ROUND)
  GPoint center = grect_center_point(&bounds);
  // Fill the center (inside the step ring)
  int inner_radius = bounds.size.w / 2 - s_layout.step_layer_offset - s_layout.step_width;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, inner_radius);
  // Mask the outer edge of the ring
  int outer_radius = bounds.size.w / 2 - s_layout.step_layer_offset + s_layout.step_outer_rad / 2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, s_layout.step_outer_rad);
  graphics_draw_circle(ctx, center, outer_radius);
  #else
  // Black the outer edge of the field
  int stroke_w = s_layout.step_outer_rad;
  int offset   = s_layout.step_layer_offset - stroke_w / 2;
  int radius   = s_layout.step_outer_rad + stroke_w / 2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, stroke_w);
  graphics_draw_round_rect(ctx, GRect(offset, offset,
                                      bounds.size.w - 2 * offset,
                                      bounds.size.h - 2 * offset), radius);
  // Black the inner rounded rectangle
  int step_inner_offset = s_layout.step_layer_offset + s_layout.step_width;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(step_inner_offset, step_inner_offset,
                                bounds.size.w - 2 * step_inner_offset,
                                bounds.size.h - 2 * step_inner_offset),
                     s_layout.tick_outer_rad, GCornersAll);
  #endif
}


// Steps Layer update proc
static void step_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint cPoint = grect_center_point(&bounds);
  int cx = cPoint.x;
  int cy = cPoint.y;
  GRect radial_rect = GRect(cx - 1, cy - ((bounds.size.w + bounds.size.h) / 2),
                            3, bounds.size.h + bounds.size.w);

  // Stage 1: Draw background ring for completed laps
  #if defined(PBL_COLOR)
  if (s_step_level == 1) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, radial_rect, GOvalScaleModeFillCircle, 150, 0, TRIG_MAX_ANGLE);
  } else if (s_step_level >= 2) {
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_radial(ctx, radial_rect, GOvalScaleModeFillCircle, 150, 0, TRIG_MAX_ANGLE);
  }
  #endif

  // Stage 2: Draw current lap progress
  GColor foreground_color = GColorWhite;
  #if defined(PBL_COLOR)
  if (s_step_level == 1) foreground_color = GColorGreen;
  else if (s_step_level >= 2) foreground_color = GColorVividViolet;
  #endif

  if (s_step_angle > 0) {
    graphics_context_set_fill_color(ctx, foreground_color);
    graphics_fill_radial(ctx, radial_rect, GOvalScaleModeFillCircle, 150, 0, s_step_angle);
  }
}
#endif // PBL_HEALTH


// Seconds ring update proc — draw-calls only, no computation
static void seconds_ring_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int current_second = t->tm_sec;

  for (int i = 0; i < 60; i++) {
    graphics_context_set_stroke_color(ctx,
        (i < current_second) ? GColorWhite : GColorDarkGray);
    graphics_draw_line(ctx, s_tick_starts[i], s_tick_ends[i]);
  }
}


// Tick Layer update proc (future tick marks feature)
static void tick_update_proc(Layer *layer, GContext *ctx) {
  // GRect bounds = layer_get_bounds(layer);
  // GPoint cPoint = grect_center_point(&bounds);
  // int cx = cPoint.x;
  // int cy = cPoint.y;
}


// ---------------------------------------------------------------------------
// Backlight / seconds ring activation
// ---------------------------------------------------------------------------

static bool       s_tick_persistence_active = false;
static AppTimer  *s_tick_persistence_timer  = NULL;
static uint16_t   s_tick_persistence_ms = 3000;  // overwritten by settings

static void seconds_ring_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    tick_handler(tick_time, units_changed);
  }
  layer_mark_dirty(s_seconds_ring_layer);
}

static void tick_persistence_callback(void *context) {
  if (DEBUG_SECONDS_ALWAYS_ON) { s_tick_persistence_timer = NULL; return; }
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  layer_set_hidden(s_seconds_ring_layer, true);
  s_tick_persistence_active = false;
  s_tick_persistence_timer  = NULL;
}

static void activate_backlight(void) {
  light_enable_interaction();

  if (DEBUG_SECONDS_ALWAYS_ON) return;

  if (s_tick_persistence_active) {
    app_timer_reschedule(s_tick_persistence_timer, s_tick_persistence_ms);
    return;
  }
  s_tick_persistence_active = true;
  layer_set_hidden(s_seconds_ring_layer, false);
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(SECOND_UNIT, seconds_ring_tick_handler);
  s_tick_persistence_timer = app_timer_register(s_tick_persistence_ms,
                                          tick_persistence_callback, NULL);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  activate_backlight();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  activate_backlight();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// ---------------------------------------------------------------------------

static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  layer_set_hidden(text_layer_get_layer(s_batt_layer), !connected);

  if (!connected) {
    vibes_double_pulse();
  }
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Main Window Bounds (w,h): (%d,%d)", bounds.size.w, bounds.size.h);

  layout_config_init(&s_layout, bounds);

  // Load custom fonts
  s_time_font  = fonts_load_custom_font(resource_get_handle(s_layout.time_font_id));
  s_time2_font = fonts_load_custom_font(resource_get_handle(s_layout.time2_font_id));
  s_date_font  = fonts_load_custom_font(resource_get_handle(s_layout.date_font_id));
  s_batt_font  = fonts_load_custom_font(resource_get_handle(s_layout.batt_font_id));

  // Time block placement
  int ci      = s_layout.corner_inset;
  int tw      = bounds.size.w - 2 * ci;
  int time_y  = s_layout.block_y - s_layout.block_height / 2;
  int time2_y = time_y  + s_layout.time_height;
  int time3_y = time2_y + s_layout.time2_height;

  // Create the hour (line 1) TextLayer
  s_time_layer = text_layer_create(GRect(ci, time_y, tw, s_layout.time_layer_h));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the minute (line 2) TextLayer
  s_time2_layer = text_layer_create(GRect(ci, time2_y, tw, s_layout.time2_layer_h));
  text_layer_set_background_color(s_time2_layer, GColorClear);
  text_layer_set_text_color(s_time2_layer, GColorWhite);
  text_layer_set_font(s_time2_layer, s_time2_font);
  text_layer_set_text_alignment(s_time2_layer, GTextAlignmentCenter);

  // Create the minute (line 3) TextLayer
  s_time3_layer = text_layer_create(GRect(ci, time3_y, tw, s_layout.time2_layer_h));
  text_layer_set_background_color(s_time3_layer, GColorClear);
  text_layer_set_text_color(s_time3_layer, GColorWhite);
  text_layer_set_font(s_time3_layer, s_time2_font);
  text_layer_set_text_alignment(s_time3_layer, GTextAlignmentCenter);

  // Create the date TextLayer — just above the bottom status bar
  s_date_layer = text_layer_create(GRect(ci, s_layout.date_y, tw, s_layout.date_height));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Tick layer geometry (all platforms)
  int tick_x = s_layout.tick_layer_offset;
  int tick_y = s_layout.tick_layer_offset;
  int tick_w = bounds.size.w - 2 * s_layout.tick_layer_offset;
  int tick_h = bounds.size.h - 2 * s_layout.tick_layer_offset;

  // Create battery level Layer (hidden on small screens where BT icon takes its place)
  s_batt_layer = text_layer_create(GRect(ci, s_layout.batt_y, tw, s_layout.batt_height));
  text_layer_set_background_color(s_batt_layer, GColorClear);
  text_layer_set_text_color(s_batt_layer, GColorWhite);
  text_layer_set_font(s_batt_layer, s_batt_font);
  text_layer_set_text_alignment(s_batt_layer, GTextAlignmentCenter);
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  int bt_y = s_layout.batt_y;
  s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w - 30) / 2, bt_y, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);

  // Tick marks layer (future feature)
  s_tick_layer = layer_create(GRect(tick_x, tick_y, tick_w, tick_h));
  layer_set_update_proc(s_tick_layer, tick_update_proc);

  #if defined(PBL_HEALTH)
  int step_x = s_layout.step_layer_offset;
  int step_y = s_layout.step_layer_offset;
  int step_w = bounds.size.w - 2 * s_layout.step_layer_offset;
  int step_h = bounds.size.h - 2 * s_layout.step_layer_offset;
  s_step_layer = layer_create(GRect(step_x, step_y, step_w, step_h));
  layer_set_update_proc(s_step_layer, step_update_proc);
  s_step_mask_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_step_mask_layer, step_mask_update_proc);
  #endif

  // Precompute seconds ring tick start/end points — one-time, no FPU
  GRect seconds_ring_bounds = GRect(0, 0, bounds.size.w, bounds.size.h);
  #if defined(PBL_ROUND)
  {
    GPoint scr_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    int outer_r = (int)s_layout.seconds_ring_radius;
    int inner_r = outer_r - (int)s_layout.seconds_tick_length;
    for (int i = 0; i < 60; i++) {
      int32_t angle = (int32_t)i * TRIG_MAX_ANGLE / 60;
      int32_t sin_a = sin_lookup(angle);
      int32_t cos_a = cos_lookup(angle);
      s_tick_starts[i] = (GPoint){
        scr_center.x + (int16_t)(sin_a * outer_r / TRIG_MAX_RATIO),
        scr_center.y - (int16_t)(cos_a * outer_r / TRIG_MAX_RATIO)
      };
      s_tick_ends[i] = (GPoint){
        scr_center.x + (int16_t)(sin_a * inner_r / TRIG_MAX_RATIO),
        scr_center.y - (int16_t)(cos_a * inner_r / TRIG_MAX_RATIO)
      };
    }
  }
  #else
  {
    GRect sr = s_layout.seconds_ring_rect;
    uint16_t sr_perim = rounded_rect_perimeter(sr, (uint16_t)s_layout.tick_inner_rad);
    GPoint sr_local_center = GPoint(sr.size.w / 2, sr.size.h / 2);
    for (int i = 0; i < 60; i++) {
      uint16_t dist = (uint16_t)((uint32_t)i * sr_perim / 60);
      GPoint outer_pt = rounded_rect_perimeter_point(
          GRect(0, 0, sr.size.w, sr.size.h),
          (uint16_t)s_layout.tick_inner_rad, sr_perim, dist);
      int dx = sr_local_center.x - outer_pt.x;
      int dy = sr_local_center.y - outer_pt.y;
      // Integer sqrt via 6 Newton-Raphson steps (converges to distance * 16)
      int mag2 = dx * dx + dy * dy;
      int mag16 = 256;
      for (int k = 0; k < 6; k++) mag16 = (mag16 + mag2 * 256 / mag16) / 2;

      if (mag16 == 0) mag16 = 1; // Prevent division by zero
      int tl = (int)s_layout.seconds_tick_length;
      s_tick_starts[i] = outer_pt;
      s_tick_ends[i] = (GPoint){
        outer_pt.x + (int16_t)((int32_t)dx * tl * 16 / mag16),
        outer_pt.y + (int16_t)((int32_t)dy * tl * 16 / mag16)
      };
    }
    seconds_ring_bounds = sr;
  }
  #endif
  s_seconds_ring_layer = layer_create(seconds_ring_bounds);
  layer_set_update_proc(s_seconds_ring_layer, seconds_ring_update_proc);
  layer_set_hidden(s_seconds_ring_layer, !DEBUG_SECONDS_ALWAYS_ON);

  // Add layers to the Window (order matters for z-ordering)
  #if defined(PBL_HEALTH)
  layer_add_child(window_layer, s_step_layer);
  layer_add_child(window_layer, s_step_mask_layer);
  #endif
  layer_add_child(window_layer, s_seconds_ring_layer);
  layer_add_child(window_layer, s_tick_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time2_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time3_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_batt_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  accel_tap_service_subscribe(accel_tap_handler);
  window_set_click_config_provider(window, click_config_provider);
}

// Unload main Window elements
static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_time2_layer);
  text_layer_destroy(s_time3_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_batt_layer);

  // Unload custom fonts
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_time2_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_batt_font);

  layer_destroy(s_tick_layer);
  layer_destroy(s_seconds_ring_layer);

  #if defined(PBL_HEALTH)
  layer_destroy(s_step_layer);
  layer_destroy(s_step_mask_layer);
  #endif

  accel_tap_service_unsubscribe();

  // Destroy Bluetooth elements
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}


static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_TICK_PERSISTENCE);
  if (t) {
    s_settings.tick_persistence_ms = (uint16_t)(t->value->int32 * 1000);
    settings_save(&s_settings);
    s_tick_persistence_ms = s_settings.tick_persistence_ms;
  }
  #if defined(PBL_HEALTH)
  t = dict_find(iter, MESSAGE_KEY_STEP_TARGET);
  if (t) {
    s_settings.step_target = (uint16_t)t->value->int32;
    settings_save(&s_settings);
    s_step_target = s_settings.step_target;
    update_steps();
  }
  #endif
}


// Initialize the main Window and its elements
static void init() {
  settings_load(&s_settings);
  s_tick_persistence_ms = s_settings.tick_persistence_ms;
  #if defined(PBL_HEALTH)
  s_step_target = s_settings.step_target;
  #endif

  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  update_time();

  #if defined(PBL_HEALTH)
  update_steps();
  #endif

  // Register with TickTimerService
  if (DEBUG_SECONDS_ALWAYS_ON) {
    tick_timer_service_subscribe(SECOND_UNIT, seconds_ring_tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }

  // Register for health updates
  #if defined(PBL_HEALTH)
  if (!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }
  #else
  APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  #endif

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  app_message_register_inbox_received(prv_inbox_received);
  app_message_open(64, 0);
}


// De-initialize the app, destroying the main Window and unsubscribing from services
static void deinit() {
  if (s_tick_persistence_active && s_tick_persistence_timer) {
    app_timer_cancel(s_tick_persistence_timer);
    s_tick_persistence_timer = NULL;
  }
  animation_unschedule_all();
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();

  window_destroy(s_main_window);
}


// Main app entry point
int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
