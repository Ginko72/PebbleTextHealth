#include <pebble.h>
#include <ctype.h>
#include <math.h>

#include "num2words-en.h"
#include "config.h"

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
static int        s_batt_level;

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
static int    s_steps;
#endif


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
  // If needed, replace leading '0' in month and day with ''
  if (s_date_buffer[7] == '0') memmove(&s_date_buffer[6], &s_date_buffer[7], sizeof(s_date_buffer) - 7);
  if (s_date_buffer[4] == '0') memmove(&s_date_buffer[4], &s_date_buffer[5], sizeof(s_date_buffer) - 5);
  text_layer_set_text(s_date_layer, s_date_buffer);
}


#if defined(PBL_HEALTH)

// Update Steps
static void update_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_steps = (int)health_service_sum_today(metric);
    APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", s_steps);
    layer_mark_dirty(s_step_layer);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Data unavailable!");
  }
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
}


// Battery change callback
static void battery_callback(BatteryChargeState state) {
  static char s_textLine[BUFFER_SIZE];

  s_batt_level = state.charge_percent;
  snprintf(s_textLine, BUFFER_SIZE, "%d%%", s_batt_level);
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

  #ifdef TESTING_STEPS
  s_steps = TESTING_STEPS;
  #endif

  // White ring: proportional for 0-10k, full for >10k — all platforms
  int white_angl = (s_steps <= 10000) ? trunc(s_steps * 360.0 / 10000.0) : 360;
  if (white_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(cx - 1,
                                    cy - ((bounds.size.w + bounds.size.h) / 2),
                                    3, bounds.size.h + bounds.size.w),
                         GOvalScaleModeFillCircle, 150,
                         0, DEG_TO_TRIGANGLE(white_angl));
  }

  // Green ring (10-20k) and purple ring (20-30k) — color platforms only
  #if defined(PBL_COLOR)
  int green_angl  = 0;
  int purple_angl = 0;
  if (s_steps > 10000 && s_steps <= 20000) {
    green_angl = trunc((s_steps - 10000) * 360.0 / 10000.0);
  } else if (s_steps > 20000 && s_steps <= 30000) {
    green_angl = 360;
    purple_angl = trunc((s_steps - 20000) * 360.0 / 10000.0);
  }
  if (green_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_radial(ctx, GRect(cx - 1,
                                    cy - ((bounds.size.w + bounds.size.h) / 2),
                                    3, bounds.size.h + bounds.size.w),
                         GOvalScaleModeFillCircle, 150,
                         0, DEG_TO_TRIGANGLE(green_angl));
  }
  if (purple_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorVividViolet);
    graphics_fill_radial(ctx, GRect(cx - 1,
                                    cy - ((bounds.size.w + bounds.size.h) / 2),
                                    3, bounds.size.h + bounds.size.w),
                         GOvalScaleModeFillCircle, 150,
                         0, DEG_TO_TRIGANGLE(purple_angl));
  }
  #endif // PBL_COLOR
}

#endif // PBL_HEALTH


// Tick Layer update proc (future tick marks feature)
static void tick_update_proc(Layer *layer, GContext *ctx) {
  // GRect bounds = layer_get_bounds(layer);
  // GPoint cPoint = grect_center_point(&bounds);
  // int cx = cPoint.x;
  // int cy = cPoint.y;
}


static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if (!connected) {
    vibes_double_pulse();
  }
}


static void layout_config_init(LayoutConfig *cfg, GRect bounds) {
  bool wide              = bounds.size.w >= 200;
  cfg->time_font_id      = wide ? RESOURCE_ID_FONT_AMIKO_BOLD_46 : RESOURCE_ID_FONT_AMIKO_BOLD_38;
  cfg->time_height       = wide ? 55 : 44;
  cfg->time2_height      = 26;
  cfg->block_height      = cfg->time_height + 2 * cfg->time2_height;
  cfg->corner_inset      = PBL_IF_ROUND_ELSE(20, 0);
  cfg->step_width        = 4;
  cfg->step_outer_rad    = 30;
  cfg->tick_width        = 6;
  cfg->step_layer_offset = 0;
  cfg->tick_outer_rad    = cfg->step_outer_rad - cfg->step_width;
  cfg->tick_layer_offset = cfg->step_layer_offset + cfg->step_width;
  cfg->tick_inner_rad    = cfg->tick_outer_rad - cfg->tick_width;
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Main Window Bounds (w,h): (%d,%d)", bounds.size.w, bounds.size.h);

  layout_config_init(&s_layout, bounds);

  // Load custom fonts
  s_time_font  = fonts_load_custom_font(resource_get_handle(s_layout.time_font_id));
  s_time2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_24));
  s_date_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_18));
  s_batt_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_16));

  // Time block placement
  int ci      = s_layout.corner_inset;
  int tw      = bounds.size.w - 2 * ci;
  int block_y = bounds.size.h / 2;
  int time_y  = block_y - s_layout.block_height / 2;
  int time2_y = time_y  + s_layout.time_height;
  int time3_y = time2_y + s_layout.time2_height;
  int date_y  = bounds.size.h - 32;

  // Create the hour (line 1) TextLayer
  s_time_layer = text_layer_create(GRect(ci, time_y, tw, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the minute (line 2) TextLayer
  s_time2_layer = text_layer_create(GRect(ci, time2_y, tw, 40));
  text_layer_set_background_color(s_time2_layer, GColorClear);
  text_layer_set_text_color(s_time2_layer, GColorWhite);
  text_layer_set_font(s_time2_layer, s_time2_font);
  text_layer_set_text_alignment(s_time2_layer, GTextAlignmentCenter);

  // Create the minute (line 3) TextLayer
  s_time3_layer = text_layer_create(GRect(ci, time3_y, tw, 40));
  text_layer_set_background_color(s_time3_layer, GColorClear);
  text_layer_set_text_color(s_time3_layer, GColorWhite);
  text_layer_set_font(s_time3_layer, s_time2_font);
  text_layer_set_text_alignment(s_time3_layer, GTextAlignmentCenter);

  // Create the date TextLayer — just above the bottom status bar
  s_date_layer = text_layer_create(GRect(ci, date_y, tw, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Tick layer geometry (all platforms)
  int tick_x = s_layout.tick_layer_offset;
  int tick_y = s_layout.tick_layer_offset;
  int tick_w = bounds.size.w - 2 * s_layout.tick_layer_offset;
  int tick_h = bounds.size.h - 2 * s_layout.tick_layer_offset;

  // Create battery level Layer
  s_batt_layer = text_layer_create(GRect(ci, 10, tw, 40));
  text_layer_set_background_color(s_batt_layer, GColorClear);
  text_layer_set_text_color(s_batt_layer, GColorWhite);
  text_layer_set_font(s_batt_layer, s_batt_font);
  text_layer_set_text_alignment(s_batt_layer, GTextAlignmentCenter);

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // Create the BitmapLayer to display the GBitmap — centered at the top
  int bt_y = PBL_IF_ROUND_ELSE(bounds.size.h / 8, bounds.size.h / 32) + 32;
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

  // Add layers to the Window (order matters for z-ordering)
  #if defined(PBL_HEALTH)
  layer_add_child(window_layer, s_step_layer);
  layer_add_child(window_layer, s_step_mask_layer);
  #endif
  layer_add_child(window_layer, s_tick_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time2_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time3_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_batt_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}


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

  #if defined(PBL_HEALTH)
  layer_destroy(s_step_layer);
  layer_destroy(s_step_mask_layer);
  #endif

  // Destroy Bluetooth elements
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}


static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

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
}


static void deinit() {
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();

  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
