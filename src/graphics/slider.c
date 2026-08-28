#include "slider.h"

#include "assets/assets.h"
#include "core/calc.h"
#include "core/image.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "graphics/window.h"

#include <string.h>

#define SLIDER_BUTTON_SIDE 24
#define SLIDER_BOX_HEIGHT 48
#define SLIDER_BOX_WIDTH 24
#define TOTAL_SLIDER_BUTTON_HEIGHT (2 * SLIDER_BUTTON_SIDE + SLIDER_BOX_HEIGHT)

typedef enum slider_element {
    SLIDER_DECREASE = 0,
    SLIDER_INCREASE = 1,
    SLIDER_THUMB = 2,
    SLIDER_BG = 3
} slider_element;

static int get_slider_image_id(const slider_t *slider, slider_element element)
{
    // returns the 00 frame of the needed element - add +n to get the nth frame
    int id = 0;
    switch (element) {
        case SLIDER_DECREASE:
            id = slider->show_plus_minus_buttons ? ASSET_UI_SCROLLBAR_PLUS_01 : ASSET_UI_ARROW_MASKED_UP;
            break;
        case SLIDER_INCREASE:
            id = slider->show_plus_minus_buttons ? ASSET_UI_SCROLLBAR_MINUS_01 : ASSET_UI_ARROW_MASKED_DOWN;
            break;
        case SLIDER_THUMB:
            id = slider->force_mini_thumb ? ASSET_UI_SCROLLBAR_MINI_THUMB_01 : 0;
            break;
        case SLIDER_BG:
            id = ASSET_UI_SCROLL_BG_01;
            break;
        default:
            return 0;
    }
    return assets_lookup_image_id(id);
}

static int get_thumb_middle_section_size(void)
{
    return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01_TRIMMED))->original.height;
}

static int get_thumb_end_size(void)
{
    return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01_END_TOP))->original.height;
}

static int get_thumb_midsections_count(const slider_t *slider)
{
    if (slider->force_mini_thumb) {
        return -1;
    }

    int tmsh = get_thumb_middle_section_size();
    int tesh = get_thumb_end_size();
    if (tmsh <= 0) {
        return 1;
    }

    int range = slider->max_value - slider->min_value;
    if (range <= 0 || slider->value_step <= 0) {
        return 1;
    }

    int total_scrollable_px = slider->length - 2 * SLIDER_BUTTON_SIDE;
    if (total_scrollable_px <= 0) {
        return 1;
    }

    int total_steps = (range + slider->value_step - 1) / slider->value_step + 1;
    int ideal_thumb_size = (total_scrollable_px + total_steps / 2) / total_steps;
    int max_mid_sections = (total_scrollable_px - 2 * tesh) / tmsh;
    if (max_mid_sections < 1) {
        max_mid_sections = 1;
    }

    int mid_sections_count = (ideal_thumb_size - 2 * tesh + tmsh / 2) / tmsh;
    return calc_bound(mid_sections_count, 1, max_mid_sections);
}

static int get_thumb_length(const slider_t *slider)
{
    if (slider->force_mini_thumb) {
        return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MINI_THUMB_01))->original.height;
    }
    return 2 * get_thumb_end_size() + get_thumb_midsections_count(slider) * get_thumb_middle_section_size();
}

static int get_track_x(const slider_t *slider)
{
    return slider->x + (slider->length - SLIDER_BOX_WIDTH) / 2;
}

static int get_thumb_y(const slider_t *slider)
{
    int range = slider->max_value - slider->min_value;
    int travel_length = slider->length - 2 * SLIDER_BUTTON_SIDE - get_thumb_length(slider);
    if (range <= 0 || travel_length <= 0) {
        return slider->y + SLIDER_BUTTON_SIDE;
    }

    int value = calc_bound(slider->value, slider->min_value, slider->max_value) - slider->min_value;
    return slider->y + SLIDER_BUTTON_SIDE + calc_adjust_with_percentage(travel_length, calc_percentage(value, range));
}

void slider_init(slider_t *slider, int x, int y, int length, int min_value, int max_value,
    int value_step, int initial_value, unsigned char is_vertical)
{
    memset(slider, 0, sizeof(*slider));
    if (max_value <= min_value || value_step <= 0) {
        return; // invalid
    }

    slider->x = x;
    slider->y = y;
    slider->length = length;

    slider->min_value = min_value;
    slider->max_value = max_value;
    slider->value_step = value_step;
    slider->value = calc_bound(initial_value, min_value, max_value);
    slider->draw_background = 1;
    slider->is_vertical = is_vertical;
}

void slider_draw(const slider_t *slider)
{
    if (slider->is_hidden) {
        return;
    }

    int x = get_track_x(slider);
    if (slider->draw_background) {
        scrollbar_panel_draw(x, slider->y, slider->length);
    }
    int decrease = get_slider_image_id(slider, SLIDER_DECREASE);
    int increase = get_slider_image_id(slider, SLIDER_INCREASE);
    image_draw(decrease, x, slider->y, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(increase, x, slider->y + slider->length - SLIDER_BUTTON_SIDE, COLOR_MASK_NONE, SCALE_NONE);

    int frame = 1;
    if (slider->is_disabled) {
        frame = 4;
    } else if (slider->is_clicked) {
        frame = 3;
    } else if (slider->is_focused) {
        frame = 2;
    }
    scrollbar_thumb_draw(x, get_thumb_y(slider), get_thumb_midsections_count(slider), 1, frame);
    window_invalidate();
}

void slider_draw_array(const slider_t *sliders, unsigned int num_sliders)
{
    for (unsigned int i = 0; i < num_sliders; i++) {
        slider_draw(&sliders[i]);
    }
}

int slider_handle_mouse(slider_t *slider, const mouse *m)
{
    (void) slider;
    (void) m;

    return 0;
}

int slider_handle_mouse_array(slider_t *sliders, const mouse *m, unsigned int num_sliders)
{
    (void) sliders;
    (void) m;
    (void) num_sliders;

    return 0;
}

int slider_handle_tooltip(const slider_t *slider, tooltip_context *c)
{
    (void) slider;
    (void) c;

    return 0;
}

int slider_handle_tooltip_array(const slider_t *sliders, tooltip_context *c, unsigned int num_sliders)
{
    (void) sliders;
    (void) c;
    (void) num_sliders;

    return 0;
}
