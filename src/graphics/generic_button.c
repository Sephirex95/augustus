#include "generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "graphics/button.h"
#include "graphics/text.h"
static unsigned int get_button(const mouse *m, int x, int y, generic_button *buttons, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        if (x + buttons[i].x <= m->x &&
            x + buttons[i].x + buttons[i].width > m->x &&
            y + buttons[i].y <= m->y &&
            y + buttons[i].y + buttons[i].height > m->y) {
            return i + 1;
        }
    }
    return 0;
}

int generic_buttons_handle_mouse(const mouse *m, int x, int y, generic_button *buttons, unsigned int num_buttons,
    unsigned int *focus_button_id)
{
    unsigned int button_id = get_button(m, x, y, buttons, num_buttons);
    if (focus_button_id) {
        *focus_button_id = button_id;
    }
    if (!button_id) {
        return 0;
    }
    generic_button *button = &buttons[button_id - 1];
    if (m->left.went_up) {
        if (button->left_click_handler) {
            button->left_click_handler(button);
            return 1;
        } else {
            return 0;
        }
    } else if (m->right.went_up) {
        if (button->right_click_handler) {
            button->right_click_handler(button);
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
void draw_generic_button(const generic_button *button, const char *label, int image_id, const generic_button_style *style, int is_focused)
{
    int label_style = is_focused ? 1 : 2;

    // Draw the background label
    label_draw(button->x, button->y, button->width / BLOCK_SIZE, label_style);

    // Optional icon
    if (image_id > 0) {
        int icon_y = button->y + style->y_offset_icon;
        int icon_x;

        if (style->is_icon_left) {
            icon_x = button->x + style->x_offset;
            image_draw(image_id, icon_x, icon_y, COLOR_MASK_NONE, SCALE_NONE);
        } else {
            int text_width = text_get_width(label, style->font);
            icon_x = button->x + button->width - style->x_offset - image_get(image_id)->width - text_width - style->space_icon_gap;
            image_draw(image_id, icon_x, icon_y, COLOR_MASK_NONE, SCALE_NONE);
        }
    }

    // Draw the centered text (offset manually to simulate vertical padding)
    text_draw_centered(label,
        button->x,
        button->y + style->y_offset_text,
        button->width,
        style->font,
        0);
}
