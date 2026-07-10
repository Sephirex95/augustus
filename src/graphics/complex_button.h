#ifndef GRAPHICS_COMPLEX_BUTTON_H
#define GRAPHICS_COMPLEX_BUTTON_H

#include "graphics/tooltip.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/text.h"
#include "input/mouse.h"

#define MAX_COMPLEX_BUTTON_PARAMETERS 10 // arbitrary 
#define MAX_CYCLE_BUTTON_STATES 10 // arbitrary

typedef enum {
    COMPLEX_BUTTON_STYLE_DEFAULT,  // Basic style: white/red border, transparent fill - inherited from background
    COMPLEX_BUTTON_STYLE_DEFAULT_SMALL,    // like default but small font and less padding
    COMPLEX_BUTTON_STYLE_DEFAULT_GRAY,     // main-menu-like style
    COMPLEX_BUTTON_STYLE_DEFAULT_WOOD,     // wood-like style
    COMPLEX_BUTTON_STYLE_LIGHT_WOOD,
    COMPLEX_BUTTON_STYLE_COLORFUL,  // colorful style with gradient background
    COMPLEX_BUTTON_STYLE_CUSTOM // custom style - bypasses the default selection of colors/fonts
} complex_button_style;

typedef enum {
    SEQUENCE_POSITION_TOP_LEFT = 1,      /*         ┌───┬───┬───┐         */
    SEQUENCE_POSITION_TOP_CENTER = 2,    /*         │ 1 │ 2 │ 3 │         */
    SEQUENCE_POSITION_TOP_RIGHT = 3,     /*         ├───┼───┼───┤         */
    SEQUENCE_POSITION_CENTER_LEFT = 4,   /*         │ 4 │ 5 │ 6 │         */
    SEQUENCE_POSITION_CENTER = 5,        /*         ├───┼───┼───┤         */
    SEQUENCE_POSITION_CENTER_RIGHT = 6,  /*         │ 7 │ 8 │ 9 │         */
    SEQUENCE_POSITION_BOTTOM_LEFT = 7,   /*         └───┴───┴───┘         */
    SEQUENCE_POSITION_BOTTOM_CENTER = 8, /*    mirroring phone keypad     */
    SEQUENCE_POSITION_BOTTOM_RIGHT = 9,  /*  OOB values will be centered  */
} sequence_positioning;

typedef struct complex_button {
    short x;
    short y;
    short width;
    short height;
    short is_focused;             // bad wording - is_hovered would be more accurate
    short is_clicked;
    short is_active;              // persists toggle/selected/checked/expanded state
    short is_hidden;              // 1 = hidden, 0 = visible
    short is_disabled;            // 1 = disabled, 0 = enabled
    short state;                  // special parameter for custom behaviours
    short is_ellipsized;          // 1 = text was ellipsized on last draw, 0 = full text shown
    void (*left_click_handler)(struct complex_button *button);
    void (*right_click_handler)(struct complex_button *button);
    void (*hover_handler)(struct complex_button *button);
    tooltip_context tooltip_c;
    const lang_fragment *sequence;     // sequence of text to draw on button
    sequence_positioning sequence_position;
    int sequence_size;
    int parameters[MAX_COMPLEX_BUTTON_PARAMETERS];
    int image_before;
    int image_after;
    int flush_with_background; // if set, bottom border is not drawn
    color_t color_mask;
    font_t font;
    complex_button_style style;
    short expanded_hitbox_radius; //not yet implemented - placeholder
    void *user_data; // custom user data pointer, e.g. can point to a parent struct
} complex_button;

typedef struct checkbox_button {
    short x;
    short y;
    short width;
    short height;
    short is_hovered;
    short is_checked;
    short fill_bg; // 1 = fill background, 0 = transparent
    void (*left_click_handler)(struct checkbox_button *button);
    void (*hover_handler)(struct checkbox_button *button);
    tooltip_context tooltip_c;
    font_t font; // font of the text next to the checkbox, the checkbox font is fixed
    short box_on_right; // box on right side of text/image instead of left
    const lang_fragment *sequence;     // sequence of text to draw on button
    int image_before; // optional image to draw before the text
    int image_after;  // optional image to draw after the text
    int sequence_size;
    color_t color_mask;
    short is_ellipsized;          // 1 = text was ellipsized on last draw, 0 = full text shown
} checkbox_button;

typedef struct cycling_button_state {
    const lang_fragment *sequence;
    int sequence_size;
    int image_before;
    int image_after;
    color_t color_mask;
    font_t font;
    tooltip_context tooltip_c;
} cycling_button_state;

typedef struct cycling_button {
    short x;
    short y;
    short width;
    short height;
    short is_hovered;
    short fill_bg; // 1 = fill background, 0 = transparent
    void (*left_click_handler)(struct cycling_button *button);
    void (*right_click_handler)(struct cycling_button *button);
    void (*hover_handler)(struct cycling_button *button);

    cycling_button_state states[MAX_CYCLE_BUTTON_STATES];
    int state_index;
    int state_count; // =< MAX_CYCLE_BUTTON_STATES
    short is_ellipsized;          // 1 = text was ellipsized on last draw, 0 = full text shown
} cycling_button;

color_t complex_button_basic_colors(int id);
font_t complex_button_font_for_style(complex_button_style style);
color_t complex_button_color_for_style(complex_button_style style);

// Complex Buttons
// drawing
void complex_button_draw(const complex_button *button);
void complex_button_array_draw(const complex_button *buttons, unsigned int num_buttons);
// input
int complex_button_handle_mouse(complex_button *btn, const mouse *m);
int complex_button_array_handle_mouse(complex_button *buttons, const mouse *m, unsigned int num_buttons);
// tooltip
int complex_button_handle_tooltip(const complex_button *button, tooltip_context *c);
int complex_button_array_handle_tooltip(const complex_button *buttons, tooltip_context *c, unsigned int num_buttons);

// Checkbox Buttons
// drawing
void checkbox_button_draw(const checkbox_button *button);
void checkbox_button_array_draw(const checkbox_button *buttons, unsigned int num_buttons);
// input
int checkbox_button_handle_mouse(checkbox_button *btn, const mouse *m);
int checkbox_button_array_handle_mouse(checkbox_button *buttons, const mouse *m, unsigned int num_buttons);
// tooltip
int checkbox_button_handle_tooltip(const checkbox_button *button, tooltip_context *c);
int checkbox_button_array_handle_tooltip(const checkbox_button *buttons, tooltip_context *c, unsigned int num_buttons);

// Cycling Buttons
// drawing
void cycling_button_draw(const cycling_button *button);
void cycling_button_array_draw(const cycling_button *buttons, unsigned int num_buttons);
// input
int cycling_button_handle_mouse(cycling_button *btn, const mouse *m);
int cycling_button_array_handle_mouse(cycling_button *buttons, const mouse *m, unsigned int num_buttons);
// tooltip
int cycling_button_handle_tooltip(const cycling_button *button, tooltip_context *c);
int cycling_button_array_handle_tooltip(const cycling_button *buttons, tooltip_context *c, unsigned int num_buttons);


#endif // GRAPHICS_COMPLEX_BUTTON_H
