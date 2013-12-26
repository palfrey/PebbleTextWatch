#include "pebble.h"
#include "pebble_app_info.h"
#include "pebble_fonts.h"
#include "resource_ids.auto.h"

#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

Window *window;
Layer *batteryLayer;
BitmapLayer * white_image_layer;

typedef struct {
	TextLayer* currentLayer;
	TextLayer* nextLayer;	
	PropertyAnimation* currentAnimation;
	PropertyAnimation* nextAnimation;
} Line;

Line line1;
Line line2;
Line line3;
Line dateLine;

struct tm t;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];
static char dateStr[2][BUFFER_SIZE];

bool last_connected = true;

void bluetooth_connection_handler(bool connected) {
	Layer *layer = (Layer*)white_image_layer;
	layer_set_hidden(layer, connected);
	if (last_connected != connected && !connected) {
		vibes_short_pulse();
	}
	last_connected = connected;
}
	
// Animation handler
void animationStoppedHandler(Animation *animation, bool finished, void *context)
{
	Layer *current = text_layer_get_layer((TextLayer *)context);
	GRect rect = layer_get_frame(current);
	rect.origin.x = 144;
	layer_set_frame(current, rect);
}

PropertyAnimation* makeAnimationsForLayer(TextLayer *textlayer) 
{
	Layer *layer = text_layer_get_layer(textlayer);
	GRect rect = layer_get_frame(layer);
	rect.origin.x -= 144;

	PropertyAnimation * animation = property_animation_create_layer_frame(layer, NULL, &rect);
	animation_set_duration((Animation*) animation, 400);
	animation_set_curve((Animation*) animation, AnimationCurveEaseOut);
	return animation;
}

// Animate line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
	if (line->currentAnimation != NULL) animation_unschedule((Animation*)line->currentAnimation);
	line->currentAnimation = makeAnimationsForLayer(current);
	if (line->nextAnimation != NULL) animation_unschedule((Animation*)line->nextAnimation);
	line->nextAnimation = makeAnimationsForLayer(next);
		
	animation_set_handlers((Animation*)line->currentAnimation, (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, current);

	animation_schedule((Animation*) line->currentAnimation);
	animation_schedule((Animation*) line->nextAnimation);
}

// Update line
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
	TextLayer *next, *current;
	
	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
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
	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
	currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

	if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
		(strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
		return true;
	}
	return false;
}

// Update screen based on new time
void display_time(struct tm * tm)
{
	// The current time text will be stored in the following strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	char textLine3[BUFFER_SIZE];
	char newDateText[BUFFER_SIZE];

	time_to_3words(tm->tm_hour, tm->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);
	date_to_words(tm, newDateText, BUFFER_SIZE);
	
	if (needToUpdateLine(&line1, line1Str, textLine1)) {
		updateLineTo(&line1, line1Str, textLine1);	
	}
	if (needToUpdateLine(&line2, line2Str, textLine2)) {
		updateLineTo(&line2, line2Str, textLine2);	
	}
	if (needToUpdateLine(&line3, line3Str, textLine3)) {
		updateLineTo(&line3, line3Str, textLine3);	
	}
	if (needToUpdateLine(&dateLine, dateStr, newDateText)) {
		updateLineTo(&dateLine, dateStr, newDateText);
	}
}

// Update screen without animation first time we start the watchface
void display_initial_time(struct tm *tm)
{
	time_to_3words(tm->tm_hour, tm->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);
	date_to_words(tm, dateStr[0], BUFFER_SIZE);
	
	text_layer_set_text(line1.currentLayer, line1Str[0]);
	text_layer_set_text(line2.currentLayer, line2Str[0]);
	text_layer_set_text(line3.currentLayer, line3Str[0]);
	text_layer_set_text(dateLine.currentLayer, dateStr[0]);
}

// Configure the first line of text
void configureBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for the 2nd and 3rd lines
void configureLightLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

void configureDateLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(textlayer, GTextOverflowModeFill);
}

/** 
 * Debug methods. For quickly debugging enable debug macro on top to transform the watchface into
 * a standard app and you will be able to change the time with the up and down buttons
 */ 
#if DEBUG

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	t.tm_min += 1;
	if (t.tm_min >= 60) {
		t.tm_min -= 60;
		t.tm_hour += 1;
		
		if (t.tm_hour >= 24) {
			t.tm_hour -= 24;
		}
	}
	display_time(&t);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	t.tm_min -= 1;
	if (t.tm_min < 0) {
		t.tm_min += 60;
		t.tm_hour -= 1;
		if (t.tm_hour < 0) {
			t.tm_hour +=24;
		}
	}
	display_time(&t);
}

void click_config_provider() {
	window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_single_click_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_single_click_handler);
}

#endif

#define TOP_TEXT 0
#define TEXT_SIZE (55-18)

void update_battery_layer(struct Layer *layer, GContext *ctx) {
	BatteryChargeState state = battery_state_service_peek();
	app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Battery charge is %d%%", state.charge_percent);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(0,0, state.charge_percent*(144.0/100),10), 0, GCornerNone);
}

void handle_battery_change(BatteryChargeState state) {
	layer_mark_dirty(batteryLayer);
}

void handle_init() {
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorBlack);

	batteryLayer = layer_create(GRect(0, 158, 144, 10));
	layer_set_update_proc(batteryLayer, update_battery_layer);
	layer_mark_dirty(batteryLayer);
	battery_state_service_subscribe(handle_battery_change);

	// 1st line layer
	line1.currentLayer = text_layer_create(GRect(0, TOP_TEXT, 144, 50));
	line1.nextLayer = text_layer_create(GRect(144, TOP_TEXT, 144, 50));
	configureBoldLayer(line1.currentLayer);
	configureBoldLayer(line1.nextLayer);

	// 2nd layers
	line2.currentLayer = text_layer_create(GRect(0, TOP_TEXT + TEXT_SIZE, 144, 50));
	line2.nextLayer = text_layer_create(GRect(144, TOP_TEXT + TEXT_SIZE, 144, 50));
	configureLightLayer(line2.currentLayer);
	configureLightLayer(line2.nextLayer);

	// 3rd layers
	line3.currentLayer = text_layer_create(GRect(0, TOP_TEXT + (TEXT_SIZE*2), 144, 50));
	line3.nextLayer = text_layer_create(GRect(144, TOP_TEXT + (TEXT_SIZE*2), 144, 50));
	configureLightLayer(line3.currentLayer);
	configureLightLayer(line3.nextLayer);

	// date layer
	dateLine.currentLayer = text_layer_create(GRect(0, 118, 144, 40));
	dateLine.nextLayer = text_layer_create(GRect(144, 118, 144, 40));
	configureDateLayer(dateLine.currentLayer);
	configureDateLayer(dateLine.nextLayer);

	// Configure time on init
	time_t now;
	now = time(NULL);
	struct tm *local_t = localtime(&now);
	memcpy(&t, local_t, sizeof(struct tm));
	display_initial_time(&t);

	GBitmap * white_image = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_WHITE);

	Layer * windowLayer = window_get_root_layer(window);
  	GRect bounds = layer_get_bounds(windowLayer);
	GRect image_frame = GRect(bounds.size.w - white_image->bounds.size.w, 0, white_image->bounds.size.w, white_image->bounds.size.h);

	white_image_layer = bitmap_layer_create(image_frame);
	bitmap_layer_set_bitmap(white_image_layer, white_image);

	// Load layers
  	layer_add_child(windowLayer, text_layer_get_layer(line1.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line1.nextLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line2.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line2.nextLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line3.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line3.nextLayer));
	layer_add_child(windowLayer, text_layer_get_layer(dateLine.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(dateLine.nextLayer));
	layer_add_child(windowLayer, batteryLayer);

	layer_add_child(windowLayer, bitmap_layer_get_layer(white_image_layer));

	bluetooth_connection_service_subscribe(bluetooth_connection_handler);
	bluetooth_connection_handler(bluetooth_connection_service_peek());
	
#if DEBUG
	// Button functionality
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
#endif
}

// Time handler called every minute by the system
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  display_time(tick_time);
}

int main() {
	handle_init();
	tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  	app_event_loop();
  	return 0;
}
