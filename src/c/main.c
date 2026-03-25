#include <pebble.h>
#include <ctype.h>

#include "num2words-en.h"
#include "config.h"

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_time2_layer;
static TextLayer *s_time3_layer;
static TextLayer *s_date_layer;

// Custom fonts
static GFont s_time_font;
static GFont s_time2_font;
static GFont s_date_font;

// Battery
static Layer *s_battery_layer;
static int s_battery_level;

// Steps
static Layer *s_steps_layer;
static int s_steps;


// Bluetooth
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

// #define TESTING

// Time update
static void update_time() {
	// The current time text will be stored in the following 3 strings
	static char s_textLine1[BUFFER_SIZE];
	static char s_textLine2[BUFFER_SIZE];
	static char s_textLine3[BUFFER_SIZE];
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  #ifdef TESTING
  tick_time->tm_hour = 12;
  tick_time->tm_min = 33;
  #endif

  time_to_3words(tick_time->tm_hour, tick_time->tm_min, s_textLine1, s_textLine2, s_textLine3, BUFFER_SIZE);
  # ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line1: %s", s_textLine1);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line2: %s", s_textLine2);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time Line3: %s", s_textLine3);
  #endif
  
  // Time String
  text_layer_set_text(s_time_layer, s_textLine1);
  text_layer_set_text(s_time2_layer, s_textLine2);
  text_layer_set_text(s_time3_layer, s_textLine3);

  // Date String
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %m.%d", tick_time);
  // Lowercase 1st char of day
  s_date_buffer[0] += 32;
  // If needed, replace leading '0' in month and day with ''
  if (s_date_buffer[7] == '0') memmove(&s_date_buffer[6], &s_date_buffer[7], sizeof(s_date_buffer) - 7);
  if (s_date_buffer[4] == '0') memmove(&s_date_buffer[4], &s_date_buffer[5], sizeof(s_date_buffer) - 5);
  text_layer_set_text(s_date_layer, s_date_buffer);
}


// Update Steps
static void update_steps() {
   HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  // Check the metric has data available for today
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric,
    start, end);
  
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    // Data is available!
    s_steps = (int)health_service_sum_today(metric);
    APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", s_steps);
  } else {
    // No data recorded yet today
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }  
}


// Tick Handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}


// Health Handler
static void health_handler(HealthEventType event, void *context) {
   // Which type of event occurred?
  switch(event) {
    case HealthEventSignificantUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO,
              "New HealthService HealthEventSignificantUpdate event");
      update_steps(); 
      // Update the meter
      // layer_mark_dirty(s_steps_layer);
      break;
    case HealthEventMovementUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO,
              "New HealthService HealthEventMovementUpdate event");
      update_steps();
      break;
    case HealthEventSleepUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO,
              "New HealthService HealthEventSleepUpdate event");
      break;
    case HealthEventHeartRateUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO,
              "New HealthService HealthEventHeartRateUpdate event");
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_INFO,
              "New HealthService default event");
      break;
  }
}


// Battery change callback
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;

  // Update the meter
  layer_mark_dirty(s_battery_layer);
}


// Steps layer update procedure
static void steps_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (inside the border)
  int bar_width = ((s_battery_level * (bounds.size.w - 4)) / 100);

  // Draw the border
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_round_rect(ctx, bounds, 2);

  // Choose color based on battery level
  GColor bar_color;
  if (s_battery_level <= 20) {
    bar_color = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
  } else if (s_battery_level <= 40) {
    bar_color = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
  } else {
    bar_color = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
  }

  // Draw the filled bar inside the border
  graphics_context_set_fill_color(ctx, bar_color);
  graphics_fill_rect(ctx, GRect(2, 2, bar_width, bounds.size.h - 4), 1, GCornerNone);
}


// Battery layer update procedure
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (inside the border)
  int bar_width = ((s_battery_level * (bounds.size.w - 4)) / 100);

  // Draw the border
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_round_rect(ctx, bounds, 2);

  // Choose color based on battery level
  GColor bar_color;
  if (s_battery_level <= 20) {
    bar_color = PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
  } else if (s_battery_level <= 40) {
    bar_color = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite);
  } else {
    bar_color = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
  }

  // Draw the filled bar inside the border
  graphics_context_set_fill_color(ctx, bar_color);
  graphics_fill_rect(ctx, GRect(2, 2, bar_width, bounds.size.h - 4), 1, GCornerNone);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if (!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Load custom fonts
  s_time_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FIGTREE_BOLD_48));
  s_time2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FIGTREE_MEDIUM_32));
  s_date_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FIGTREE_LIGHT_18));

  // Place the time + date block vertically
  int bar_height   = 6;
  int time_height  = 52;
  int time2_height = 30;
  int date_height  = 30;
  int block_height = 120;
  int time_y  = (bounds.size.h / 2) - (block_height / 2) - 10;
  int time2_y = (bounds.size.h / 2) - (block_height / 2) + time_height - 10;
  int time3_y = (bounds.size.h / 2) - (block_height / 2) + time_height + time2_height - 10;
  int date_y  = bounds.size.h - PBL_IF_ROUND_ELSE(bounds.size.h / 8, bounds.size.h / 32) - 12 - date_height;

  // Create the hour (line1) TextLayer
  s_time_layer = text_layer_create(
      GRect(0, time_y, bounds.size.w, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the miunte (line2) TextLayer
  s_time2_layer = text_layer_create(
      GRect(0, time2_y, bounds.size.w, 40));
  text_layer_set_background_color(s_time2_layer, GColorClear);
  text_layer_set_text_color(s_time2_layer, GColorWhite);
  text_layer_set_font(s_time2_layer, s_time2_font);
  text_layer_set_text_alignment(s_time2_layer, GTextAlignmentCenter);
  
  // Create the minute (line3) TestLayer
  s_time3_layer = text_layer_create(
      GRect(0, time3_y, bounds.size.w, 40));
  text_layer_set_background_color(s_time3_layer, GColorClear);
  text_layer_set_text_color(s_time3_layer, GColorWhite);
  text_layer_set_font(s_time3_layer, s_time2_font);
  text_layer_set_text_alignment(s_time3_layer, GTextAlignmentCenter);
  
  // Create the date TextLayer — just above the bottom status bar
  s_date_layer = text_layer_create(
      GRect(0, date_y, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Create battery meter Layer — visible bar near the top
  int bar_width = bounds.size.w * 2 / 3;
  int bar_x = (bounds.size.w - bar_width) / 2;
  int bar_y = bounds.size.h - PBL_IF_ROUND_ELSE(bounds.size.h / 8, bounds.size.h / 32) - 12;
  s_battery_layer = layer_create(GRect(bar_x, bar_y, bar_width, bar_height));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // Create the BitmapLayer to display the GBitmap — below the battery bar, centered
  int bt_y = PBL_IF_ROUND_ELSE(bounds.size.h / 8, bounds.size.h / 32) + bar_height + 10;
  s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w - 30) / 2, bt_y, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);

  // Add layers to the Window (order matters for z-ordering)
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time2_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time3_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);

  // Unload custom fonts
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);

  // Destroy battery layer
  layer_destroy(s_battery_layer);

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
  // Attempt to subscribe
  if(!health_service_events_subscribe(health_handler, NULL)) {
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
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
