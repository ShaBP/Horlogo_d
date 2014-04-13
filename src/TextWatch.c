#include "pebble.h"
#include <ctype.h>
#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

Window *window;

typedef struct {
  TextLayer *currentLayer;
  TextLayer *nextLayer;
  PropertyAnimation *currentAnimation;
  PropertyAnimation *nextAnimation;
} Line;

Line line1;
Line line2;
Line line3;
TextLayer *date;
TextLayer *day;
TextLayer *battery;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];

static void destroy_property_animation(PropertyAnimation **prop_animation) {
  if (*prop_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *prop_animation)) {
    animation_unschedule((Animation*) *prop_animation);
  }

  property_animation_destroy(*prop_animation);
  *prop_animation = NULL;
}

//Handle Date
void setDate(struct tm *tm)
{
  static char dateString[] = "september 99, 9999";
  static char dayString[] = "wednesday";
//  switch(tm->tm_mday)
//  {
//    case 1 :
//    case 21 :
//    case 31 :
//      strftime(dateString, sizeof(dateString), "%B %est, %Y", tm);
//      break;
//    case 2 :
//    case 22 :
//      strftime(dateString, sizeof(dateString), "%B %end, %Y", tm);
//      break;
//    case 3 :
//    case 23 :
//      strftime(dateString, sizeof(dateString), "%B %erd, %Y", tm);
//      break;
//    default :
//      strftime(dateString, sizeof(dateString), "%B %eth, %Y", tm);
//      break;
//  }
  strftime(dateString, sizeof(dateString), "s%V, %d/%m", tm);
  strftime(dayString, sizeof(dayString), "%A", tm);
  dateString[0] = tolower((int)dateString[0]);
  dayString[0] = tolower((int)dayString[0]);
  if (strcmp(dayString,"saturday")==0) {
    strcpy(dayString, "sabato");
    }
  else if (strcmp(dayString,"sunday")==0) {
    strcpy(dayString, "dimanxo"); // font tricks to display the special Esperanto character
    }  
  else if (strcmp(dayString,"monday")==0) {
    strcpy(dayString, "lundo");
    }
  else if (strcmp(dayString,"tuesday")==0) {
    strcpy(dayString, "mardo");
    }
  else if (strcmp(dayString,"wednesday")==0) {
    strcpy(dayString, "merkredo");
    }
  else if (strcmp(dayString,"thursday")==0) {
    strcpy(dayString, "waydo"); // font tricks to display the special Esperanto character
    }
  else if (strcmp(dayString,"friday")==0) {
    strcpy(dayString, "vendredo");
    }
  text_layer_set_text(date, dateString);
  text_layer_set_text(day, dayString);
}

// handling battery level display
void setBatteryLevel()
{
  static char battery_text[] = "100%";

  BatteryChargeState charge_state = battery_state_service_peek();
  if (charge_state.is_charging) {
      snprintf(battery_text, sizeof(battery_text), "chrg");
  } else {
      snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery, battery_text);
}

static void handle_battery(BatteryChargeState charge_state) 
{
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
        snprintf(battery_text, sizeof(battery_text), "chrg");
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
    }
  text_layer_set_text(battery, battery_text);
}

// Vibe generator 
void generate_vibe(uint32_t vibe_pattern_number) {
  vibes_cancel();
  switch ( vibe_pattern_number ) {
  case 0: // No Vibration
    return;
  case 1: // Single short
    vibes_short_pulse();
    break;
  case 2: // Double short
    vibes_double_pulse();
    break;
  case 3: // Triple
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {200, 100, 200, 100, 200},
      .num_segments = 5
    } );
  case 4: // Long
    vibes_long_pulse();
    break;
  case 5: // Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {50, 200, 50, 200, 50, 200, 50},
      .num_segments = 7
    } );
    break;
  case 6: // Less Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {100, 200, 100, 200, 100, 200, 100},
      .num_segments = 7
    } );
    break;
  case 7: // Not Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {500, 250, 500, 250, 500, 250, 500},
      .num_segments = 7
    } );
    break;
  case 8: // Three times double pulses
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {100, 100, 100, 500, 100, 100, 100, 500, 100, 100, 100},
      .num_segments = 11 
    } );
    break;
  default: // No Vibration
    return;
  }
}

// handling bluetooth disconnection event
void bluetooth_connection_callback(bool connected) {
  if (!connected){
     generate_vibe(7); 
  }
}

// Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
  Layer *current = (Layer *)context;
  GRect rect = layer_get_frame(current);
  rect.origin.x = 144;
  layer_set_frame(current, rect);
}

// Animate line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
  GRect rect = layer_get_frame((Layer*)next);
  rect.origin.x -= 144;

  destroy_property_animation(&(line->nextAnimation));

  line->nextAnimation = property_animation_create_layer_frame((Layer*)next, NULL, &rect);
  animation_set_duration((Animation*)line->nextAnimation, 400);
  animation_set_curve((Animation*)line->nextAnimation, AnimationCurveEaseOut);

  animation_schedule((Animation*)line->nextAnimation);

  GRect rect2 = layer_get_frame((Layer*)current);
  rect2.origin.x -= 144;

  destroy_property_animation(&(line->currentAnimation));

  line->currentAnimation = property_animation_create_layer_frame((Layer*)current, NULL, &rect2);
  animation_set_duration((Animation*)line->currentAnimation, 400);
  animation_set_curve((Animation*)line->currentAnimation, AnimationCurveEaseOut);

  animation_set_handlers((Animation*)line->currentAnimation, (AnimationHandlers) {
      .stopped = (AnimationStoppedHandler)animationStoppedHandler
      }, current);

  animation_schedule((Animation*)line->currentAnimation);
}

// Update line
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
  TextLayer *next, *current;

  GRect rect = layer_get_frame((Layer*)line->currentLayer);
  current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
  next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;

  // Update correct text only
  if (current == line->currentLayer) {
    memset(lineStr[1], 0, BUFFER_SIZE);
    memcpy(lineStr[1], value, strlen(value));
    text_layer_set_text(next, lineStr[1]);
  } else {
    memset(lineStr[0], 0, BUFFER_SIZE);
    memcpy(lineStr[0], value, strlen(value));
    text_layer_set_text(next, lineStr[0]);
  }

  makeAnimationsForLayers(line, current, next);
}

// Check to see if the current line needs to be updated
bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue)
{
  char *currentStr;
  GRect rect = layer_get_frame((Layer*)line->currentLayer);
  currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

  if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
      (strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
    return true;
  }
  return false;
}

// Update screen based on new time
void display_time(struct tm *t)
{
  // The current time text will be stored in the following 3 strings
  char textLine1[BUFFER_SIZE];
  char textLine2[BUFFER_SIZE];
  char textLine3[BUFFER_SIZE];

  time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);

  if (needToUpdateLine(&line1, line1Str, textLine1)) {
    updateLineTo(&line1, line1Str, textLine1);
  }
  if (needToUpdateLine(&line2, line2Str, textLine2)) {
    updateLineTo(&line2, line2Str, textLine2);
  }
  if (needToUpdateLine(&line3, line3Str, textLine3)) {
    updateLineTo(&line3, line3Str, textLine3);
  }
}

// Update screen without animation first time we start the watchface
void display_initial_time(struct tm *t)
{
  time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);

  text_layer_set_text(line1.currentLayer, line1Str[0]);
  text_layer_set_text(line2.currentLayer, line2Str[0]);
  text_layer_set_text(line3.currentLayer, line3Str[0]);
  setDate(t);
  setBatteryLevel();
}


// Configure the first line of text
void configureBoldLayer(TextLayer *textlayer)
{
  text_layer_set_font(textlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORION_ESP_DIKA_38)));
//  text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_color(textlayer, GColorWhite);
  text_layer_set_background_color(textlayer, GColorClear);
  text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for the 2nd and 3rd lines
void configureLightLayer(TextLayer *textlayer)
{
  text_layer_set_font(textlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORION_ESP_NORMALA_38)));
//  text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_color(textlayer, GColorWhite);
  text_layer_set_background_color(textlayer, GColorClear);
  text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed);

void init() {

  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  // 1st line layers
  line1.currentLayer = text_layer_create(GRect(0, 10, 144, 50));
  line1.nextLayer = text_layer_create(GRect(144, 10, 144, 50));
  configureBoldLayer(line1.currentLayer);
  configureBoldLayer(line1.nextLayer);

  // 2nd layers
  line2.currentLayer = text_layer_create(GRect(0, 47, 144, 50));
  line2.nextLayer = text_layer_create(GRect(144, 47, 144, 50));
  configureLightLayer(line2.currentLayer);
  configureLightLayer(line2.nextLayer);

  // 3rd layers
  line3.currentLayer = text_layer_create(GRect(0, 84, 144, 50));
  line3.nextLayer = text_layer_create(GRect(144, 84, 144, 50));
  configureLightLayer(line3.currentLayer);
  configureLightLayer(line3.nextLayer);

  //date & day layers
  date = text_layer_create(GRect(50, 138, 94, 168-138));
  text_layer_set_text_color(date, GColorWhite);
  text_layer_set_background_color(date, GColorClear);
  text_layer_set_font(date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORION_ESP_NORMALA_20)));
  //  text_layer_set_font(date, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(date, GTextAlignmentRight);
  day = text_layer_create(GRect(0, 120, 144, 168-120));
  text_layer_set_text_color(day, GColorWhite);
  text_layer_set_background_color(day, GColorClear);
  text_layer_set_font(day, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORION_ESP_DIKA_20)));
  //text_layer_set_font(day, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(day, GTextAlignmentRight);

  //battery status layer
  battery = text_layer_create(GRect(0,138,80,168-138));
  text_layer_set_text_color(battery, GColorWhite);
  text_layer_set_background_color(battery, GColorClear);
  text_layer_set_font(battery, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORION_ESP_NORMALA_20)));
  //text_layer_set_font(battery, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(battery, GTextAlignmentLeft);
  
  // Configure time on init
  time_t now = time(NULL);
  display_initial_time(localtime(&now));

  // Load layers
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, (Layer*)line1.currentLayer);
  layer_add_child(window_layer, (Layer*)line1.nextLayer);
  layer_add_child(window_layer, (Layer*)line2.currentLayer);
  layer_add_child(window_layer, (Layer*)line2.nextLayer);
  layer_add_child(window_layer, (Layer*)line3.currentLayer);
  layer_add_child(window_layer, (Layer*)line3.nextLayer);
  layer_add_child(window_layer, (Layer*)date);
  layer_add_child(window_layer, (Layer*)day);
  layer_add_child(window_layer, (Layer*)battery);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
}

void deinit() {

  tick_timer_service_unsubscribe();

  layer_destroy((Layer*)line1.currentLayer);
  layer_destroy((Layer*)line1.nextLayer);
  layer_destroy((Layer*)line2.currentLayer);
  layer_destroy((Layer*)line2.nextLayer);
  layer_destroy((Layer*)line3.currentLayer);
  layer_destroy((Layer*)line3.nextLayer);

  destroy_property_animation(&line1.currentAnimation);
  destroy_property_animation(&line1.nextAnimation);
  destroy_property_animation(&line2.currentAnimation);
  destroy_property_animation(&line2.nextAnimation);
  destroy_property_animation(&line3.currentAnimation);
  destroy_property_animation(&line3.nextAnimation);

  layer_destroy((Layer*)date);
  layer_destroy((Layer*)day);
  layer_destroy((Layer*)battery);

  window_destroy(window);
}

// Time handler called every minute by the system
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_time(tick_time);
  if (units_changed & DAY_UNIT) {
    setDate(tick_time);
  }
}

int main(void) {

  init();
  app_event_loop();
  deinit();

}
