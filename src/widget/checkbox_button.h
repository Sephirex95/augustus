#ifndef WIDGET_CHECKBOX_BUTTON_H
#define WIDGET_CHECKBOX_BUTTON_H

#include "graphics/color.h"
#include "graphics/font.h"
#include "graphics/lang_text.h"
#include "graphics/tooltip.h"
#include "input/mouse.h"

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
    lang_sequence sequence;     // sequence of text to draw on button
    int image_before; // optional image to draw before the text
    int image_after;  // optional image to draw after the text
    color_t color_mask;
    short is_ellipsized;          // 1 = text was ellipsized on last draw, 0 = full text shown
} checkbox_button;

void checkbox_button_draw(const checkbox_button *button);
void checkbox_button_draw_array(const checkbox_button *buttons, unsigned int num_buttons);

int checkbox_button_handle_mouse(checkbox_button *btn, const mouse *m);
int checkbox_button_handle_mouse_array(checkbox_button *buttons, const mouse *m, unsigned int num_buttons);

int checkbox_button_handle_tooltip(const checkbox_button *button, tooltip_context *c);
int checkbox_button_handle_tooltip_array(const checkbox_button *buttons, tooltip_context *c, unsigned int num_buttons);

#endif // WIDGET_CHECKBOX_BUTTON_H
