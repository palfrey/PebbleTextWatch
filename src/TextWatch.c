#include "pebble.h"
#include "pebble_app_info.h"
#include "pebble_fonts.h"

#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

Window *window;

typedef struct {
	TextLayer* currentLayer;
	TextLayer* nextLayer;	
	PropertyAnimation* currentAnimation;
	PropertyAnimation* nextAnimation;
} Line;

Line line1;
Line line2;
Line line3;

struct tm t;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];
	
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
	// The current time text will be stored in the following 3 strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	char textLine3[BUFFER_SIZE];
	
	time_to_3words(tm->tm_hour, tm->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);
	
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
void display_initial_time(struct tm *tm)
{
	time_to_3words(tm->tm_hour, tm->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);
	
	text_layer_set_text(line1.currentLayer, line1Str[0]);
	text_layer_set_text(line2.currentLayer, line2Str[0]);
	text_layer_set_text(line3.currentLayer, line3Str[0]);
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

void handle_init() {
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorBlack);

	// 1st line layer
	line1.currentLayer = text_layer_create(GRect(0, 18, 144, 50));
	line1.nextLayer = text_layer_create(GRect(144, 18, 144, 50));
	configureBoldLayer(line1.currentLayer);
	configureBoldLayer(line1.nextLayer);

	// 2nd layers
	line2.currentLayer = text_layer_create(GRect(0, 55, 144, 50));
	line2.nextLayer = text_layer_create(GRect(144, 55, 144, 50));
	configureLightLayer(line2.currentLayer);
	configureLightLayer(line2.nextLayer);

	// 3rd layers
	line3.currentLayer = text_layer_create(GRect(0, 92, 144, 50));
	line3.nextLayer = text_layer_create(GRect(144, 92, 144, 50));
	configureLightLayer(line3.currentLayer);
	configureLightLayer(line3.nextLayer);

	// Configure time on init
	time_t now;
	now = time(NULL);
	struct tm *local_t = localtime(&now);
	memcpy(&t, local_t, sizeof(struct tm));
	display_initial_time(&t);
	
	// Load layers
	Layer * windowLayer = window_get_root_layer(window);
  	layer_add_child(windowLayer, text_layer_get_layer(line1.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line1.nextLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line2.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line2.nextLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line3.currentLayer));
	layer_add_child(windowLayer, text_layer_get_layer(line3.nextLayer));
	
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
