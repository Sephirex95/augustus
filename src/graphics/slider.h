#ifndef GRAPHICS_SLIDER_H
#define GRAPHICS_SLIDER_H

#include "graphics/lang_text.h"
#include "graphics/tooltip.h"
#include "input/mouse.h"

#include <stdint.h>

typedef enum slider_display_text { // ____________________________________
    SLIDER_DISPLAY_TEXT_NONE = 0,  //                  3
    SLIDER_DISPLAY_TEXT_LEFT = 1,  //                  ^
    SLIDER_DISPLAY_TEXT_RIGHT = 2, //   1  <--- |<|----O----|>| --->  2
    SLIDER_DISPLAY_TEXT_ABOVE = 3, //                  v
    SLIDER_DISPLAY_TEXT_BELOW = 4  //                  4
} slider_display_text;             //_____________________________________

typedef struct slider {
    short x;
    short y;
    short length; // scrollable dimensions length - can be either height or width depending on orientation
    unsigned char is_vertical; // 1 = vertical slider, 0 = horizontal slider
    unsigned char is_disabled;
    unsigned char is_hidden;
    slider_display_text display_text; // whether to display the slider's value as text, and position of it
    const lang_fragment *sequence; // text fragments to display next to the slider (e.g. "Volume: 50%")
    unsigned short sequence_size;
    unsigned char show_value_after_sequence; // print ": <value>" after the sequence text
    unsigned char show_percentage_after_sequence; // print ": <(value/max_value)*100>%" after the sequence text
    unsigned char show_plus_minus_buttons; // 1 = +/- buttons, 0 = arrows
    unsigned char force_mini_thumb; // 1 = force the 24x24 square thumb, 0 = scale the thumb based on the range
    unsigned char draw_background;
    int min_value;
    int max_value;
    int value_step;
    int value;
    void *slider_variable; // variable that will be automatically updated to match the slider's value
    lang_fragment *(*convert_value_to_string)(struct slider *slider); // converts slider value into lang fragments
    uint8_t *raw_text; // optional raw text if you dont want to deal with lang_fragment - set it via callback fnc
    void (*left_click_handler)(struct slider *slider);
    void (*right_click_handler)(struct slider *slider);
    void (*hover_handler)(struct slider *slider);
    void (*on_slide_callback)(struct slider *slider);
    unsigned char is_focused;
    unsigned char is_clicked;
    unsigned char focused_element; // 0 = none, 1 = left button, 2 = right button, 3 = thumb, 4 = track
    unsigned char focused_element_animation_frame; // index of the animation frame 
    tooltip_context tooltip_c;
    void *user_data;
} slider;

typedef slider slider_t;

void slider_init(slider_t *slider, int x, int y, int length, int min_value, int max_value,
    int value_step, int initial_value, unsigned char is_vertical);

void slider_draw(const slider_t *slider);
void slider_draw_array(const slider_t *sliders, unsigned int num_sliders);

int slider_handle_mouse(slider_t *slider, const mouse *m);
int slider_handle_mouse_array(slider_t *sliders, const mouse *m, unsigned int num_sliders);

int slider_handle_tooltip(const slider_t *slider, tooltip_context *c);
int slider_handle_tooltip_array(const slider_t *sliders, tooltip_context *c, unsigned int num_sliders);

#endif // GRAPHICS_SLIDER_H
