#ifndef GRAPHICS_GENERIC_BUTTON_H
#define GRAPHICS_GENERIC_BUTTON_H

#include "input/mouse.h"

typedef struct generic_button {
    short x;
    short y;
    short width;
    short height;
    void (*left_click_handler)(const struct generic_button *button);
    void (*right_click_handler)(const struct generic_button *button);
    int parameter1;
    int parameter2;
} generic_button;

typedef struct {
    int y_offset_text;
    int y_offset_icon;
    int x_offset;     // padding before first element (icon or text)
    int space_icon_gap;     // space between icon and text
    int font;
    int is_icon_left; // 1 if icon is on the left, 0 if on the right
} generic_button_style;

int generic_buttons_handle_mouse(const mouse *m, int x, int y, generic_button *buttons, unsigned int num_buttons, unsigned int *focus_button_id);
void draw_generic_button(const generic_button *button, const char *label, int image_id, const generic_button_style *style, int is_focused);

#endif // GRAPHICS_GENERIC_BUTTON_H