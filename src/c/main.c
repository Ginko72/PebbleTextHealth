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

// Steps and Second Tick Marks
static Layer *s_tick_layer;
static Layer *s_tick_mask_layer;
static Layer *s_step_layer;
static Layer *s_step_mask_layer;
static int    s_bezel_width;
static int    s_step_width;
static int    s_tick_width;
static int    s_step_layer_offset;
static int    s_tick_layer_offset;
static int    s_step_outer_rad;
static int    s_tick_outer_rad;
static int    s_tick_inner_rad;
static int    s_steps;

// Bluetooth
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

// #define TESTING

// Update Time
static void update_time() {
	// The current time text will be stored in the following 3 strings
	static char s_textLine1[BUFFER_SIZE];
	static char s_textLine2[BUFFER_SIZE];
	static char s_textLine3[BUFFER_SIZE];
  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  #ifdef TESTING
  tick_time->tm_hour = 8;
  tick_time->tm_min = 17;
  #endif

  time_to_3words(tick_time->tm_hour, tick_time->tm_min, s_textLine1, s_textLine2, s_textLine3, BUFFER_SIZE);
  #if DEBUG
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
    // Update the meter
    layer_mark_dirty(s_step_layer);
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
  static char s_textLine[BUFFER_SIZE];
  
  // Record the new battery level
  s_batt_level = state.charge_percent;

  // Update the meter
  snprintf(s_textLine, BUFFER_SIZE, "%d%%", s_batt_level);
  text_layer_set_text(s_batt_layer, s_textLine);
}


// Steps Mask Layer update proc
static void step_mask_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // This is a mask, black the outer edge of the field...
  int stroke_w = s_step_outer_rad;
  int offset   = s_step_layer_offset - stroke_w/2;
  int radius   = s_step_outer_rad + stroke_w/2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, stroke_w);
  graphics_draw_round_rect(ctx, GRect(offset, offset, 
                                      bounds.size.w - 2*offset, 
                                      bounds.size.h - 2*offset), radius);
  
  // Black the inner rounded rectangle...
  int step_inner_offset = s_step_layer_offset + s_step_width;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(step_inner_offset, step_inner_offset,
                                bounds.size.w - 2*step_inner_offset,
                                bounds.size.h - 2*step_inner_offset), s_tick_outer_rad, GCornersAll);
  

}


// Steps Layer update proc
static void step_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  GPoint cPoint = grect_center_point(&bounds);
  int cx = cPoint.x;
  int cy = cPoint.y;
  
  int white_angl  = 0;
  int green_angl  = 0;
  int purple_angl = 0;
  
  #ifdef TESTING
  s_steps = 11000;
  #endif
  
  // Compute angles
  if (s_steps <= 10000) {
    white_angl = trunc(s_steps * 360.0/10000.0);
  } else if (s_steps <= 20000) {
    white_angl = 360;
    green_angl = trunc((s_steps-10000) * 360.0/10000.0);
  } else if (s_steps <= 30000) {
    green_angl = 360;
    purple_angl = trunc((s_steps-20000) * 360.0/10000.0);
  }
  
  // White ring (0-10k)
  if (white_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, GRect(cx - 1, 
                                    cy - ((bounds.size.w + bounds.size.h)/2),
                                    3, bounds.size.h + bounds.size.w), 
                         GOvalScaleModeFillCircle, 150, 
                         0, DEG_TO_TRIGANGLE(white_angl));
  }
  
  // Green ring (10-20k)
  if (green_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_radial(ctx, GRect(cx - 1, 
                                    cy - ((bounds.size.w + bounds.size.h)/2),
                                    3, bounds.size.h + bounds.size.w), 
                         GOvalScaleModeFillCircle, 150, 
                         0, DEG_TO_TRIGANGLE(green_angl));
  }
  
  // Purple ring (10-20k)
  if (purple_angl > 0) {
    graphics_context_set_fill_color(ctx, GColorVividViolet);
    graphics_fill_radial(ctx, GRect(cx - 1, 
                                    cy - ((bounds.size.w + bounds.size.h)/2),
                                    3, bounds.size.h + bounds.size.w), 
                         GOvalScaleModeFillCircle, 150, 
                         0, DEG_TO_TRIGANGLE(purple_angl));
  }
}


// Tick Layer update proc
static void tick_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  GPoint cPoint = grect_center_point(&bounds);
  // int cx = cPoint.x;
  // int cy = cPoint.y;

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
  APP_LOG(APP_LOG_LEVEL_INFO, "Main Window Bounds (w,h): (%d,%d)", bounds.size.w, bounds.size.h);

  // Load custom fonts
  int time_height  = 55;
  if (bounds.size.w >= 200) {
    s_time_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_BOLD_46));
  } else {
    s_time_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_BOLD_38));
    time_height  = 44;
  }
  s_time2_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_24));
  s_date_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_18));
  s_batt_font  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AMIKO_REGULAR_16));

  // Compute the time block + date placement in y-axis
  int time2_height = 26;
  int block_height = 110;
  int block_y = (bounds.size.h / 2);
  int time_y  = block_y - (block_height / 2);
  int time2_y = time_y  + time_height;
  int time3_y = time2_y + time2_height;
  int date_y  = bounds.size.h - 32; // block_y + (block_height / 2) + 6; 

  // Create the hour (line1) TextLayer
  s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the miunte (line2) TextLayer
  s_time2_layer = text_layer_create(GRect(0, time2_y, bounds.size.w, 40));
  text_layer_set_background_color(s_time2_layer, GColorClear);
  text_layer_set_text_color(s_time2_layer, GColorWhite);
  text_layer_set_font(s_time2_layer, s_time2_font);
  text_layer_set_text_alignment(s_time2_layer, GTextAlignmentCenter);
  
  // Create the minute (line3) TestLayer
  s_time3_layer = text_layer_create(GRect(0, time3_y, bounds.size.w, 40));
  text_layer_set_background_color(s_time3_layer, GColorClear);
  text_layer_set_text_color(s_time3_layer, GColorWhite);
  text_layer_set_font(s_time3_layer, s_time2_font);
  text_layer_set_text_alignment(s_time3_layer, GTextAlignmentCenter);
  
  // Create the date TextLayer — just above the bottom status bar
  s_date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Compute the tick and step sizes
  s_bezel_width       = 0;
  s_step_width        = 4;
  s_tick_width        = 6;
  s_step_layer_offset = s_bezel_width;
  s_tick_layer_offset = s_step_layer_offset + s_tick_width;
  s_step_outer_rad    = 30;
  s_tick_outer_rad    = s_step_outer_rad - s_step_width;
  s_tick_inner_rad    = s_tick_outer_rad - s_tick_width;
  int step_x = s_bezel_width;
  int step_y = s_bezel_width;
  int step_w = bounds.size.w - 2 * s_bezel_width;
  int step_h = bounds.size.h - 2 * s_bezel_width;
  int tick_x = s_bezel_width + s_step_width;
  int tick_y = s_bezel_width + s_step_width;
  int tick_w = step_w - 2 * s_step_width;
  int tick_h = step_h - 2 * s_step_width;
  
  // Create the step layers
  s_step_mask_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_step_mask_layer, step_mask_update_proc);
  s_step_layer      = layer_create(GRect(step_x, step_y, step_w, step_h));
  layer_set_update_proc(s_step_layer, step_update_proc);
  
  // Create the tick layer
  s_tick_layer = layer_create(GRect(tick_x, tick_y, tick_w, tick_h));
  layer_set_update_proc(s_tick_layer, tick_update_proc);
  
  // Create battery level Layer
  int batt_height = 40;
  int batt_y = 10;
  s_batt_layer = text_layer_create(GRect(0, batt_y, bounds.size.w, batt_height));
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

  // Add layers to the Window (order matters for z-ordering)
  layer_add_child(window_layer, s_step_layer);
  layer_add_child(window_layer, s_step_mask_layer);
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

  // Destroy step layer
  layer_destroy(s_step_layer);
  layer_destroy(s_step_mask_layer);

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
