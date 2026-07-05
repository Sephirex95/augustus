#include "complex_button.h"

#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "input/mouse.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void complex_button_ellipsized(complex_button *button, int was_ellipsized);

static const cycling_button_state *cycling_button_get_state(const cycling_button *button)
{
    if (!button || button->state_count <= 0 || button->state_count > MAX_CYCLE_BUTTON_STATES) {
        return NULL;
    }

    int index = button->state_index;
    if (index < 0) {
        index = 0;
    }
    if (index >= button->state_count) {
        index = button->state_count - 1;
    }
    return &button->states[index];
}

color_t complex_button_basic_colors(int id)
{
    switch (id) {
        case 1: return COLOR_MASK_PASTEL_GREEN;
        case 2: return COLOR_MASK_PASTEL_PURPLE;
        case 3: return COLOR_MASK_PASTEL_ORANGE;
        case 4: return COLOR_MASK_PASTEL_OLIVE;
        case 5: return COLOR_MASK_PASTEL_TURQUOISE;
        case 6: return COLOR_MASK_PASTEL_CORAL;
        case 7: return COLOR_MASK_PASTEL_GRAY;
        case 8: return COLOR_MASK_PASTEL_BLUE;
        case 9: return COLOR_MASK_PASTEL_DARK_BLUE;
        case 10: return COLOR_MASK_PASTEL_BLACK;
        case 11: return COLOR_MASK_PASTEL_BROWN;
        default: return COLOR_MASK_NONE;
    }
}

font_t complex_button_font_for_style(complex_button_style style)
{
    switch (style) {
        case COMPLEX_BUTTON_STYLE_DEFAULT_SMALL:
            return FONT_SMALL_PLAIN;
        case COMPLEX_BUTTON_STYLE_DEFAULT_WOOD:
            return FONT_NORMAL_BROWN;
        case COMPLEX_BUTTON_STYLE_LIGHT_WOOD:
            return FONT_NORMAL_GREEN;
        case COMPLEX_BUTTON_STYLE_DEFAULT:
        case COMPLEX_BUTTON_STYLE_GRAY:
        case COMPLEX_BUTTON_STYLE_COLORFUL:
        default:
            return FONT_NORMAL_BLACK;
    }
}

color_t complex_button_color_for_style(complex_button_style style)
{
    switch (style) {
        case COMPLEX_BUTTON_STYLE_COLORFUL:
            return COLOR_MASK_PASTEL_TURQUOISE;
        case COMPLEX_BUTTON_STYLE_DEFAULT_WOOD:
            return COLOR_MASK_PASTEL_BROWN2;
        case COMPLEX_BUTTON_STYLE_LIGHT_WOOD:
            return COLOR_MASK_PASTEL_BROWN4;
        case COMPLEX_BUTTON_STYLE_DEFAULT:
        case COMPLEX_BUTTON_STYLE_DEFAULT_SMALL:
        case COMPLEX_BUTTON_STYLE_GRAY:
        default:
            return COLOR_MASK_NONE;
    }
}

static void draw_default_style(const complex_button *button, font_t base_font, color_t label_color)
{
    font_t font;
    const int inner_margin = 2; // small horizontal margin for text/images
    switch (base_font) { // this bit just changes fonts for disabled buttons. It should be moved out of the drawing fnc
        case FONT_NORMAL_BLACK:
            font = !button->is_disabled ? FONT_NORMAL_BLACK : FONT_NORMAL_WHITE;
            break;
        case FONT_SMALL_PLAIN:
        case FONT_NORMAL_BROWN:
        default:
            font = base_font;
            break;
    }
    const color_t f_color = button->is_disabled ? COLOR_FONT_GRAY : COLOR_MASK_NONE;
    label_color = label_color ? label_color : COLOR_MASK_NONE;
    graphics_set_clip_rectangle(button->x, button->y, button->width, button->height);

    int height_blocks = button->height / BLOCK_SIZE;
    unbordered_panel_draw_colored(button->x, button->y, button->width / BLOCK_SIZE + 1, height_blocks + 1,
        label_color);
    int draw_red_border = !button->is_disabled ? button->is_focused : 0;    // Only draw border if enabled
    if (button->flush_with_background) {
        button_border_draw_colored_flush(button->x, button->y, button->width, button->height, draw_red_border, label_color);
    } else {
        button_border_draw_colored(button->x, button->y, button->width, button->height, draw_red_border, label_color);
    }
    sequence_positioning pos = (!button->sequence_position) ? SEQUENCE_POSITION_CENTER : button->sequence_position;
    // Y offset based on positioning enum (row: top, center, bottom)
    int text_height = font_definition_for(font)->line_height;
    int sequence_y_offset = 0;
    switch (pos) {
        case SEQUENCE_POSITION_TOP_LEFT:
        case SEQUENCE_POSITION_TOP_CENTER:
        case SEQUENCE_POSITION_TOP_RIGHT:
            sequence_y_offset = button->y + inner_margin;
            break;
        case SEQUENCE_POSITION_CENTER_LEFT:
        case SEQUENCE_POSITION_CENTER:
        case SEQUENCE_POSITION_CENTER_RIGHT:
        default:
            sequence_y_offset = button->y + (button->height - text_height) / 2;
            break;
        case SEQUENCE_POSITION_BOTTOM_LEFT:
        case SEQUENCE_POSITION_BOTTOM_CENTER:
        case SEQUENCE_POSITION_BOTTOM_RIGHT:
            sequence_y_offset = button->y + button->height - text_height - inner_margin;
            break;
    }

    // Pre-calc widths
    int seq_width = lang_text_get_sequence_width(button->sequence, button->sequence_size, font);
    seq_width = seq_width % 2 ? seq_width - 1 : seq_width; // even up for better centering
    int img_before_w = 0, img_after_w = 0;
    const image *img_before = NULL, *img_after = NULL;

    if (button->image_before > 0) {
        img_before = image_get(button->image_before);
        img_before_w = img_before->width + inner_margin;
    }
    if (button->image_after > 0) {
        img_after = image_get(button->image_after);
        img_after_w = img_after->width + inner_margin;
    }

    int total_width = img_before_w + seq_width + img_after_w;

    int cursor_x = 0;
    switch (pos) {
        case SEQUENCE_POSITION_TOP_RIGHT:
        case SEQUENCE_POSITION_CENTER_RIGHT:
        case SEQUENCE_POSITION_BOTTOM_RIGHT:
            cursor_x = button->x + button->width - inner_margin - total_width;
            break;
        case SEQUENCE_POSITION_TOP_LEFT:
        case SEQUENCE_POSITION_CENTER_LEFT:
        case SEQUENCE_POSITION_BOTTOM_LEFT:
            cursor_x = button->x + inner_margin;
            break;
        case SEQUENCE_POSITION_TOP_CENTER:
        case SEQUENCE_POSITION_CENTER:
        case SEQUENCE_POSITION_BOTTOM_CENTER:
        default:
            cursor_x = button->x + (button->width - total_width) / 2;
            break;
    }

    // Draw before-image if present
    color_t mask = !button->is_disabled ? COLOR_MASK_NONE : COLOR_MASK_GRAY;
    if (img_before) {
        int img_y = button->y + (button->height - img_before->height) / 2;
        image_draw(button->image_before, cursor_x, img_y, mask, SCALE_NONE);
        cursor_x += img_before->width + inner_margin;
    }

    // Draw sequence (centered version if enum is 2,5,8)
    int was_ellipsized = 0;
    if (button->sequence && button->sequence_size > 0) {
        if (pos == SEQUENCE_POSITION_TOP_CENTER || pos == SEQUENCE_POSITION_CENTER ||
             pos == SEQUENCE_POSITION_BOTTOM_CENTER) {
            lang_text_draw_sequence_centered_ellipsized(button->sequence, button->sequence_size, button->x,
                sequence_y_offset, button->width, font, f_color, &was_ellipsized);
        } else {
            cursor_x += lang_text_draw_sequence_ellipsized(button->sequence, button->sequence_size, cursor_x,
                sequence_y_offset, button->width, font, f_color, &was_ellipsized);
        }
    }
    complex_button_ellipsized((complex_button *) button, was_ellipsized); //de-constant button to set ellipsized flag

    // Draw after-image if present
    if (img_after) {
        int img_y = button->y + (button->height - img_after->height) / 2;
        image_draw(button->image_after, cursor_x + inner_margin, img_y, mask, SCALE_NONE);
    }

    graphics_reset_clip_rectangle();
}

static void draw_grey_style(const complex_button *button)
{
    graphics_set_clip_rectangle(button->x, button->y, button->width, button->height);
    // hold the place for the placeholder
}

static void complex_button_ellipsized(complex_button *button, int was_ellipsized)
{
    button->is_ellipsized = was_ellipsized;
}

// === Draw a single button ===
void complex_button_draw(const complex_button *button)
{
    if (button->is_hidden) {
        return;
    }
    if (button->style == COMPLEX_BUTTON_STYLE_CUSTOM) {
        // Custom style - bypasses the default selection of colors/fonts
        draw_default_style(button, button->font, button->color_mask);
        return;
    }
    color_t base_color = complex_button_color_for_style(button->style);
    font_t base_font = complex_button_font_for_style(button->style);
    switch (button->style) {
        case COMPLEX_BUTTON_STYLE_GRAY:
            draw_grey_style(button);
            break;
        default: // all others use the default function 
            draw_default_style(button, base_font, base_color);
    }
}

void complex_button_array_draw(const complex_button *buttons, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        complex_button_draw(&buttons[i]);
    }
}

void checkbox_button_draw(const checkbox_button *button)
{
    if (!button) {
        return;
    }

    const int spacing = 6;
    font_t font = button->font ? button->font : FONT_NORMAL_BROWN;
    color_t text_color = button->color_mask ? button->color_mask : COLOR_MASK_NONE;
    color_t image_color = COLOR_MASK_NONE;

    int box_size = button->height;
    if (box_size < 12) {
        box_size = 12;
    }
    if (box_size > button->width) {
        box_size = button->width;
    }

    int box_x = button->box_on_right ? button->x + button->width - box_size : button->x;
    int box_y = button->y;
    int content_x = button->box_on_right ? button->x : box_x + box_size + spacing;
    int content_width = button->width - box_size - spacing;
    if (content_width < 0) {
        content_width = 0;
    }

    const image *img_before = NULL;
    const image *img_after = NULL;
    int img_before_w = 0;
    int img_after_w = 0;

    if (button->image_before > 0) {
        img_before = image_get(button->image_before);
        img_before_w = img_before->width + spacing;
    }
    if (button->image_after > 0) {
        img_after = image_get(button->image_after);
        img_after_w = img_after->width + spacing;
    }

    graphics_set_clip_rectangle(button->x, button->y, button->width, button->height);
    if (button->fill_bg) {
        unbordered_panel_draw_px(button->x, button->y, button->width, button->height);
    }
    button_border_draw(box_x, box_y, box_size, box_size, button->is_hovered);
    if (button->is_checked) {
        int mark_x = box_x + (box_size * 6) / 20;
        int mark_y = box_y + (box_size * 3) / 20;
        text_draw((const uint8_t *) "x", mark_x, mark_y, FONT_NORMAL_BROWN, COLOR_MASK_NONE);
    }

    int cursor_x = content_x;
    if (img_before) {
        int img_y = button->y + (button->height - img_before->height) / 2;
        image_draw(button->image_before, cursor_x, img_y, image_color, SCALE_NONE);
        cursor_x += img_before_w;
    }

    int text_y = button->y + (button->height - font_definition_for(font)->line_height) / 2;
    int max_text_width = content_width - img_before_w - img_after_w;
    if (max_text_width < 0) {
        max_text_width = 0;
    }
    int was_ellipsized = 0;
    if (button->sequence && button->sequence_size > 0) {
        cursor_x += lang_text_draw_sequence_ellipsized(button->sequence, button->sequence_size, cursor_x, text_y,
            max_text_width,
            font, text_color, &was_ellipsized);
    }
    ((checkbox_button *) button)->is_ellipsized = was_ellipsized;

    if (img_after) {
        int img_y = button->y + (button->height - img_after->height) / 2;
        image_draw(button->image_after, cursor_x, img_y, image_color, SCALE_NONE);
    }

    graphics_reset_clip_rectangle();
}

void checkbox_button_array_draw(const checkbox_button *buttons, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        checkbox_button_draw(&buttons[i]);
    }
}

void cycling_button_draw(const cycling_button *button)
{
    if (!button) {
        return;
    }

    const cycling_button_state *state = cycling_button_get_state(button);
    if (!state) {
        return;
    }

    const int inner_margin = 2;
    font_t font = state->font ? state->font : FONT_NORMAL_BLACK;
    color_t text_color = state->color_mask ? state->color_mask : COLOR_MASK_NONE;
    color_t image_color = COLOR_MASK_NONE;

    int img_before_w = 0;
    int img_after_w = 0;
    const image *img_before = NULL;
    const image *img_after = NULL;

    if (state->image_before > 0) {
        img_before = image_get(state->image_before);
        img_before_w = img_before->width + inner_margin;
    }
    if (state->image_after > 0) {
        img_after = image_get(state->image_after);
        img_after_w = img_after->width + inner_margin;
    }

    int text_max_width = button->width - 2 * inner_margin - img_before_w - img_after_w;
    if (text_max_width < 0) {
        text_max_width = 0;
    }

    int seq_width = 0;
    if (state->sequence && state->sequence_size > 0) {
        seq_width = lang_text_get_sequence_width(state->sequence, state->sequence_size, font);
    }
    int visible_seq_width = seq_width < text_max_width ? seq_width : text_max_width;

    int total_width = img_before_w + visible_seq_width + img_after_w;
    int cursor_x = button->x + (button->width - total_width) / 2;
    if (cursor_x < button->x + inner_margin) {
        cursor_x = button->x + inner_margin;
    }
    int text_y = button->y + (button->height - font_definition_for(font)->line_height) / 2;

    graphics_set_clip_rectangle(button->x, button->y, button->width, button->height);
    if (button->fill_bg) {
        unbordered_panel_draw_px(button->x, button->y, button->width, button->height);
    }
    if (img_before) {
        int img_y = button->y + (button->height - img_before->height) / 2;
        image_draw(state->image_before, cursor_x, img_y, image_color, SCALE_NONE);
        cursor_x += img_before_w;
    }

    if (state->sequence && state->sequence_size > 0) {
        cursor_x += lang_text_draw_sequence_ellipsized(state->sequence, state->sequence_size, cursor_x,
            text_y, text_max_width, font, text_color, 0);
    }

    if (img_after) {
        int img_y = button->y + (button->height - img_after->height) / 2;
        image_draw(state->image_after, cursor_x + inner_margin, img_y, image_color, SCALE_NONE);
    }
    button_border_draw(button->x, button->y, button->width, button->height, button->is_hovered);
    graphics_reset_clip_rectangle();
}

void cycling_button_array_draw(const cycling_button *buttons, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        cycling_button_draw(&buttons[i]);
    }
}

int complex_button_handle_mouse(complex_button *btn, const mouse *m)
{
    if (btn->is_hidden || btn->is_disabled) {
        btn->is_focused = 0;
        btn->is_clicked = 0;
        return 0;
    }

    int handled = 0;

    // Expanded hitbox
    int left = btn->x - btn->expanded_hitbox_radius;
    int right = btn->x + btn->width + btn->expanded_hitbox_radius;
    int top = btn->y - btn->expanded_hitbox_radius;
    int bottom = btn->y + btn->height + btn->expanded_hitbox_radius;

    int inside = (m->x >= left && m->x < right && m->y >= top && m->y < bottom);
    if (btn->is_focused != inside) {
        window_request_refresh(); // redraw to show focus change
    }
    btn->is_focused = inside;
    if (btn->is_ellipsized && btn->is_focused) { //if the button is ellipsized, show tooltip
        static uint8_t tooltip_text[512];
        lang_text_concatenate_sequence(btn->sequence, btn->sequence_size, tooltip_text, 512);
        btn->tooltip_c.type = TOOLTIP_BUTTON;
        btn->tooltip_c.precomposed_text = tooltip_text; // reset precomposed text to force re-generation
    }

    if (inside) {

        if (btn->hover_handler) {
            btn->hover_handler(btn);
            // hover handler does not consume the event, but it doesn't request refresh either
            // if needed, the handler should call window_request_refresh() or window_invalidate()
        }

        // --- Left click ---

        if (m->left.went_up) {
            btn->is_clicked = 1;
            btn->is_active = !btn->is_active; // persistent toggle
            handled = 1;
            if (btn->left_click_handler) {
                btn->left_click_handler(btn);
            }

        }
        // --- Right click ---
        if (m->right.went_up) {
            btn->is_clicked = 1;
            handled = 1;
            if (btn->right_click_handler) {
                btn->right_click_handler(btn);
            }
        }
    } else {
        btn->is_clicked = 0;
    }

    return handled;
}

int checkbox_button_handle_mouse(checkbox_button *btn, const mouse *m)
{
    if (!btn) {
        return 0;
    }

    int inside = (m->x >= btn->x && m->x < btn->x + btn->width && m->y >= btn->y && m->y < btn->y + btn->height);
    if (btn->is_hovered != inside) {
        window_request_refresh();
    }
    btn->is_hovered = inside;

    if (btn->is_ellipsized && btn->is_hovered) {
        static uint8_t tooltip_text[512];
        lang_text_concatenate_sequence(btn->sequence, btn->sequence_size, tooltip_text, 512);
        btn->tooltip_c.type = TOOLTIP_BUTTON;
        btn->tooltip_c.precomposed_text = tooltip_text;
    }

    if (inside && btn->hover_handler) {
        btn->hover_handler(btn);
    }

    if (inside && m->left.went_up) {
        btn->is_checked = !btn->is_checked;
        if (btn->left_click_handler) {
            btn->left_click_handler(btn);
        }
        return 1;
    }

    return 0;
}

int checkbox_button_array_handle_mouse(checkbox_button *buttons, const mouse *m, unsigned int num_buttons)
{
    int handled = 0;

    for (unsigned int i = 0; i < num_buttons; i++) {
        if (checkbox_button_handle_mouse(&buttons[i], m)) {
            handled = 1;
        }
    }

    return handled;
}

int cycling_button_handle_mouse(cycling_button *btn, const mouse *m)
{
    if (!btn) {
        return 0;
    }

    int inside = (m->x >= btn->x && m->x < btn->x + btn->width && m->y >= btn->y && m->y < btn->y + btn->height);
    if (btn->is_hovered != inside) {
        window_request_refresh();
    }
    btn->is_hovered = inside;

    if (inside && btn->hover_handler) {
        btn->hover_handler(btn);
    }

    int handled = 0;

    if (inside && m->left.went_up) {
        if (btn->state_count > 0) {
            btn->state_index = (btn->state_index + 1) % btn->state_count;
            window_request_refresh();
        }
        if (btn->left_click_handler) {
            btn->left_click_handler(btn);
        }
        handled = 1;
    }

    if (inside && m->right.went_up) {
        if (btn->state_count > 0) {
            btn->state_index = (btn->state_index + btn->state_count - 1) % btn->state_count;
            window_request_refresh();
        }
        if (btn->right_click_handler) {
            btn->right_click_handler(btn);
        }
        handled = 1;
    }

    return handled;
}

int cycling_button_array_handle_mouse(cycling_button *buttons, const mouse *m, unsigned int num_buttons)
{
    int handled = 0;

    for (unsigned int i = 0; i < num_buttons; i++) {
        if (cycling_button_handle_mouse(&buttons[i], m)) {
            handled = 1;
        }
    }

    return handled;
}

int checkbox_button_handle_tooltip(const checkbox_button *button, tooltip_context *c)
{
    if (button->is_hovered) {
        c->type = button->tooltip_c.type;
        c->precomposed_text = button->tooltip_c.precomposed_text;
        return 1;
    }
    return 0;
}

int checkbox_button_array_handle_tooltip(const checkbox_button *buttons, tooltip_context *c, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        if (checkbox_button_handle_tooltip(&buttons[i], c)) {
            return 1;
        }
    }
    return 0;
}

int cycling_button_handle_tooltip(const cycling_button *button, tooltip_context *c)
{
    if (!button || !c || !button->is_hovered) {
        return 0;
    }

    const cycling_button_state *state = cycling_button_get_state(button);
    if (!state) {
        return 0;
    }

    c->type = state->tooltip_c.type;
    c->precomposed_text = state->tooltip_c.precomposed_text;

    if (c->type || c->precomposed_text) {
        return 1;
    }

    if (state->sequence && state->sequence_size > 0) {
        static uint8_t tooltip_text[512];
        lang_text_concatenate_sequence(state->sequence, state->sequence_size, tooltip_text, 512);
        c->type = TOOLTIP_BUTTON;
        c->precomposed_text = tooltip_text;
        return 1;
    }

    return 0;
}

int cycling_button_array_handle_tooltip(const cycling_button *buttons, tooltip_context *c, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        if (cycling_button_handle_tooltip(&buttons[i], c)) {
            return 1;
        }
    }
    return 0;
}

int complex_button_array_handle_mouse(complex_button *buttons, const mouse *m, unsigned int num_buttons)
{
    int handled = 0;

    for (unsigned int i = 0; i < num_buttons; i++) {
        if (complex_button_handle_mouse(&buttons[i], m)) {
            handled = 1;
        }
    }

    return handled;
}

//TO SOLVE: manually set tooltips will be overwritten if the button is ellipsized. 
int complex_button_handle_tooltip(const complex_button *button, tooltip_context *c)
{
    if (button->is_focused) {
        c->type = button->tooltip_c.type;
        c->precomposed_text = button->tooltip_c.precomposed_text;
        return 1;
    }
    return 0;
}

int complex_button_array_handle_tooltip(const complex_button *buttons, tooltip_context *c, unsigned int num_buttons)
{
    for (unsigned int i = 0; i < num_buttons; i++) {
        if (complex_button_handle_tooltip(&buttons[i], c)) {
            return 1;
        }
    }
    return 0;
}
