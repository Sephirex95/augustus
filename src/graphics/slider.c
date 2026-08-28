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
#define MIN_THUMB_OFFSET SLIDER_BUTTON_SIDE // prevents the thumb from overlapping the decrease button

typedef enum slider_element {
    SLIDER_NONE = 0,
    SLIDER_DECREASE = 1,
    SLIDER_INCREASE = 2,
    SLIDER_THUMB = 3,
    SLIDER_BG = 4
} slider_element;

static int get_slider_image_id(const slider_t *slider, slider_element element)
{
    int id = 0;

    switch (element) {
        case SLIDER_DECREASE:
            if (slider->show_plus_minus_buttons) {
                id = ASSET_UI_SCROLLBAR_MINUS_01;
            } else {
                id = slider->is_vertical ? ASSET_UI_SCROLLBAR_UP : ASSET_UI_SCROLLBAR_LEFT_01;
            }
            break;

        case SLIDER_INCREASE:
            if (slider->show_plus_minus_buttons) {
                id = ASSET_UI_SCROLLBAR_PLUS_01;
            } else {
                id = slider->is_vertical ? ASSET_UI_SCROLLBAR_DOWN : ASSET_UI_SCROLLBAR_RIGHT_01;
            }
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

static int get_thumb_middle_section_size(int is_vertical)
{
    if (is_vertical) {
        return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01_TRIMMED))->original.height;
    }

    return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01B_TRIMMED))->original.width;
}

static int get_thumb_end_size(int is_vertical)
{
    if (is_vertical) {
        return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01_END_TOP))->original.height;
    }

    return image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MIDDLE_01B_END_LEFT))->original.width;
}

static int get_thumb_midsections_count(const slider_t *slider)
{
    if (slider->force_mini_thumb) {
        return -1;
    }

    int middle_size = get_thumb_middle_section_size(slider->is_vertical);
    int end_size = get_thumb_end_size(slider->is_vertical);

    if (middle_size <= 0) {
        return 0;
    }

    int range = slider->max_value - slider->min_value;
    if (range <= 0 || slider->value_step <= 0) {
        return 0;
    }

    int track_length = slider->length - 2 * SLIDER_BUTTON_SIDE;
    if (track_length <= 2 * end_size) {
        return 0;
    }

    /*
     * The maximum value is always treated as a valid final position,
     * even when the range is not evenly divisible by value_step.
     *
     * Example:
     *
     * min = 0, max = 10, step = 4
     *
     * 0, 4, 8, 10
     */
    int total_steps = (range + slider->value_step - 1) / slider->value_step + 1;
    int ideal_thumb_size = (track_length + total_steps / 2) / total_steps;
    int max_mid_sections = (track_length - 2 * end_size) / middle_size;

    if (max_mid_sections < 0) {
        max_mid_sections = 0;
    }

    int mid_sections_count = (ideal_thumb_size - 2 * end_size + middle_size / 2) / middle_size;

    return calc_bound(mid_sections_count, 0, max_mid_sections);
}

static int get_thumb_length(const slider_t *slider)
{
    if (slider->force_mini_thumb) {
        const image *thumb = image_get(assets_lookup_image_id(ASSET_UI_SCROLLBAR_MINI_THUMB_01));
        return slider->is_vertical ? thumb->original.height : thumb->original.width;
    }

    return 2 * get_thumb_end_size(slider->is_vertical) +
        get_thumb_midsections_count(slider) * get_thumb_middle_section_size(slider->is_vertical);
}

/*
 * Amount of space through which the TOP/LEFT edge of the thumb may move.
 *
 * Slider geometry:
 *
 * | button |<---------- track ---------->| button |
 *          |<------ thumb travel ------>|
 *
 * The thumb therefore always stays entirely between the two buttons.
 */
static int get_thumb_travel_length(const slider_t *slider)
{
    int travel_length = slider->length - 2 * SLIDER_BUTTON_SIDE - get_thumb_length(slider);

    return travel_length > 0 ? travel_length : 0;
}

static int slider_snap_value(const slider_t *slider, int value)
{
    value = calc_bound(value, slider->min_value, slider->max_value);

    if (value == slider->max_value) {
        return slider->max_value;
    }

    int offset = value - slider->min_value;
    int lower_step = offset / slider->value_step;
    int lower_value = slider->min_value + lower_step * slider->value_step;
    int upper_value = lower_value + slider->value_step;

    if (upper_value > slider->max_value) {
        upper_value = slider->max_value;
    }

    if (value - lower_value < upper_value - value) {
        return lower_value;
    }

    return upper_value;
}

static int slider_get_thumb_offset_from_value(const slider_t *slider)
{
    int range = slider->max_value - slider->min_value;
    int travel_length = get_thumb_travel_length(slider);

    if (range <= 0 || travel_length <= 0) {
        return MIN_THUMB_OFFSET;
    }

    int value_offset = calc_bound(slider->value, slider->min_value, slider->max_value) - slider->min_value;

    /*
     * Round to the nearest pixel instead of always truncating towards
     * the beginning of the slider.
     */
    long long scaled_offset = (long long) value_offset * travel_length + range / 2;
    int thumb_offset = MIN_THUMB_OFFSET + (int) (scaled_offset / range);

    /*
     * This final bound is deliberately expressed using the thumb's
     * complete length. The right/bottom edge of the thumb therefore
     * cannot enter the increase button.
     */
    return calc_bound(thumb_offset, MIN_THUMB_OFFSET,
        slider->length - SLIDER_BUTTON_SIDE - get_thumb_length(slider));
}

static void slider_update_cached_thumb_offset(slider_t *slider)
{
    slider->cached_thumb_offset = slider_get_thumb_offset_from_value(slider);
}

static void slider_decrease_value(slider_t *slider)
{
    if (slider->value <= slider->min_value) {
        slider->value = slider->min_value;
        return;
    }

    /*
     * max_value may be the special final value which does not lie
     * directly on the regular step grid.
     *
     * Example:
     *
     * 0, 4, 8, 10
     *
     * Decreasing from 10 must therefore go to 8, not 6.
     */
    int range = slider->max_value - slider->min_value;

    if (slider->value == slider->max_value && range % slider->value_step != 0) {
        slider->value = slider->min_value + (range / slider->value_step) * slider->value_step;
        return;
    }

    slider->value -= slider->value_step;

    if (slider->value < slider->min_value) {
        slider->value = slider->min_value;
    }
}

static void slider_increase_value(slider_t *slider)
{
    if (slider->value >= slider->max_value) {
        slider->value = slider->max_value;
        return;
    }

    if (slider->max_value - slider->value <= slider->value_step) {
        slider->value = slider->max_value;
        return;
    }

    slider->value += slider->value_step;

    if (slider->value > slider->max_value) {
        slider->value = slider->max_value;
    }
}

static int mouse_is_inside_rect(const mouse *m, int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return 0;
    }

    return m->x >= x && m->x < x + width && m->y >= y && m->y < y + height;
}

static slider_element slider_get_hovered_element(slider_t *slider, const mouse *m)
{
    slider_element hovered_element = SLIDER_NONE;

    /*
     * Disabled sliders have no interactive elements.
     */
    if (!slider->is_hidden && !slider->is_disabled && m->is_inside_window) {
        if (slider->is_vertical) {
            if (mouse_is_inside_rect(m, slider->x, slider->y, SLIDER_BUTTON_SIDE, SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_DECREASE;
            } else if (mouse_is_inside_rect(m, slider->x, slider->y + slider->length - SLIDER_BUTTON_SIDE,
                SLIDER_BUTTON_SIDE, SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_INCREASE;
            } else if (mouse_is_inside_rect(m, slider->x, slider->y + slider->cached_thumb_offset,
                SLIDER_BUTTON_SIDE, get_thumb_length(slider))) {
                hovered_element = SLIDER_THUMB;
            } else if (mouse_is_inside_rect(m, slider->x, slider->y + SLIDER_BUTTON_SIDE,
                SLIDER_BUTTON_SIDE, slider->length - 2 * SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_BG;
            }
        } else {
            if (mouse_is_inside_rect(m, slider->x, slider->y, SLIDER_BUTTON_SIDE, SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_DECREASE;
            } else if (mouse_is_inside_rect(m, slider->x + slider->length - SLIDER_BUTTON_SIDE, slider->y,
                SLIDER_BUTTON_SIDE, SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_INCREASE;
            } else if (mouse_is_inside_rect(m, slider->x + slider->cached_thumb_offset, slider->y,
                get_thumb_length(slider), SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_THUMB;
            } else if (mouse_is_inside_rect(m, slider->x + SLIDER_BUTTON_SIDE, slider->y,
                slider->length - 2 * SLIDER_BUTTON_SIDE, SLIDER_BUTTON_SIDE)) {
                hovered_element = SLIDER_BG;
            }
        }
    }

    slider->is_hovered = hovered_element != SLIDER_NONE;
    slider->hovered_element = hovered_element;

    return hovered_element;
}

static int get_slider_frame(const slider_t *slider)
{
    if (slider->is_disabled) {
        return 4;
    }

    /*
     * The frame returned here is used to draw the thumb, so button or
     * background interaction should not make the thumb appear hovered
     * or clicked.
     */
    if (slider->is_clicked && slider->hovered_element == SLIDER_THUMB) {
        return 3;
    }

    if (slider->hovered_element == SLIDER_THUMB) {
        return 2;
    }

    return 1;
}

static void draw_slider_horizontal(const slider_t *slider)
{
    int frame = get_slider_frame(slider);

    if (slider->draw_background) {
        scrollbar_panel_draw(slider->x, slider->y, slider->length, 0);
    }

    image_draw(get_slider_image_id(slider, SLIDER_DECREASE), slider->x, slider->y, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(get_slider_image_id(slider, SLIDER_INCREASE), slider->x + slider->length - SLIDER_BUTTON_SIDE,
        slider->y, COLOR_MASK_NONE, SCALE_NONE);

    /*
     * value remains the definitive source of truth.
     *
     * The calculated position is used here rather than trusting the
     * cache blindly, which also makes externally changed values draw
     * correctly on the next refresh.
     */
    int thumb_offset = slider_get_thumb_offset_from_value(slider);

    scrollbar_thumb_draw(slider->x + thumb_offset, slider->y, get_thumb_midsections_count(slider), 0, frame);
}

static void draw_slider_vertical(const slider_t *slider)
{
    int frame = get_slider_frame(slider);

    if (slider->draw_background) {
        scrollbar_panel_draw(slider->x, slider->y, slider->length, 1);
    }

    image_draw(get_slider_image_id(slider, SLIDER_DECREASE), slider->x, slider->y, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(get_slider_image_id(slider, SLIDER_INCREASE), slider->x, slider->y + slider->length - SLIDER_BUTTON_SIDE,
        COLOR_MASK_NONE, SCALE_NONE);

    int thumb_offset = slider_get_thumb_offset_from_value(slider);

    scrollbar_thumb_draw(slider->x, slider->y + thumb_offset, get_thumb_midsections_count(slider), 1, frame);
}

void slider_init(slider_t *slider, int x, int y, int length, int min_value, int max_value,
    int value_step, int initial_value, unsigned char is_vertical)
{
    memset(slider, 0, sizeof(*slider));

    if (max_value <= min_value || value_step <= 0 || length <= 2 * SLIDER_BUTTON_SIDE) {
        /*
         * Leave an invalid slider in a harmless state rather than as
         * a zeroed object which could accidentally still be drawn.
         */
        slider->is_hidden = 1;
        slider->is_disabled = 1;

        return;
    }

    slider->x = x;
    slider->y = y;
    slider->length = length;

    slider->min_value = min_value;
    slider->max_value = max_value;
    slider->value_step = value_step;

    slider->draw_background = 1;
    slider->is_vertical = is_vertical;

    slider->value = slider_snap_value(slider, initial_value);
    slider_update_cached_thumb_offset(slider);
}

void slider_text_block_init(text_block *block, const lang_fragment *sequence, unsigned short sequence_size,
    sequence_positioning position, int x, int y, int width, int height)
{
    memset(block, 0, sizeof(*block));

    block->sequence = sequence;
    block->sequence_size = sequence_size;
    block->position = position;
    block->x = x;
    block->y = y;
    block->width = width;
    block->height = height;
}

void slider_draw(const slider_t *slider)
{
    if (slider->is_hidden) {
        return;
    }

    if (slider->is_vertical) {
        draw_slider_vertical(slider);
    } else {
        draw_slider_horizontal(slider);
    }
}

void slider_draw_array(const slider_t *sliders, unsigned int num_sliders)
{
    for (unsigned int i = 0; i < num_sliders; i++) {
        slider_draw(&sliders[i]);
    }
}

int slider_get_value_from_thumb_offset(const slider_t *slider, int thumb_offset)
{
    int range = slider->max_value - slider->min_value;
    int travel_length = get_thumb_travel_length(slider);

    if (range <= 0 || travel_length <= 0) {
        return slider->min_value;
    }

    /*
     * Convert from an absolute thumb offset inside the slider to
     * relative travel inside the track.
     */
    int relative_offset = calc_bound(thumb_offset - MIN_THUMB_OFFSET, 0, travel_length);

    /*
     * Convert the thumb's relative pixel position into the logical
     * value range, rounding to the nearest logical value first.
     */
    long long scaled_value = (long long) relative_offset * range + travel_length / 2;
    int value = slider->min_value + (int) (scaled_value / travel_length);

    /*
     * The logical result must then obey the slider's discrete value
     * step policy.
     */
    return slider_snap_value(slider, value);
}

int slider_handle_mouse(slider_t *slider, const mouse *m)
{
    int previous_value = slider->value;
    int previous_is_clicked = slider->is_clicked;
    slider_element previous_hovered_element = slider->hovered_element;
    unsigned char previous_animation_frame = slider->hovered_element_animation_frame;

    /*
     * value is authoritative, so rebuild the helper cache before using
     * it for hit testing. This also catches values changed externally
     * since the previous mouse update.
     */
    slider->value = slider_snap_value(slider, slider->value);
    slider_update_cached_thumb_offset(slider);

    /*
     * is_clicked represents a click occurring during this input update,
     * rather than becoming permanently latched after the first click.
     */
    slider->is_clicked = 0;

    slider_get_hovered_element(slider, m);

    if (slider->hovered_element != SLIDER_NONE && slider->hovered_element == previous_hovered_element) {
        slider->hovered_element_animation_frame++;

        if (slider->hovered_element_animation_frame > 4) {
            slider->hovered_element_animation_frame = 4;
        }
    } else {
        slider->hovered_element_animation_frame = 0;
    }

    if (!slider->is_disabled && m->left.went_down && slider->hovered_element != SLIDER_NONE) {
        slider->is_clicked = 1;

        if (slider->hovered_element == SLIDER_DECREASE) {
            slider_decrease_value(slider);
        } else if (slider->hovered_element == SLIDER_INCREASE) {
            slider_increase_value(slider);
        } else if (slider->hovered_element == SLIDER_BG) {
            int click_offset = slider->is_vertical ? m->y - slider->y : m->x - slider->x;

            /*
             * Centre the thumb on the clicked point before clamping it
             * to the legal track area.
             */
            int thumb_offset = click_offset - get_thumb_length(slider) / 2;
            int max_thumb_offset = slider->length - SLIDER_BUTTON_SIDE - get_thumb_length(slider);

            thumb_offset = calc_bound(thumb_offset, MIN_THUMB_OFFSET, max_thumb_offset);
            slider->value = slider_get_value_from_thumb_offset(slider, thumb_offset);
        }

        /*
         * Never allow cached geometry to become a second source of
         * truth. Recalculate it from the resulting logical value.
         */
        slider_update_cached_thumb_offset(slider);
    }

    if (slider->value != previous_value ||
        slider->hovered_element != previous_hovered_element ||
        slider->hovered_element_animation_frame != previous_animation_frame ||
        slider->is_clicked != previous_is_clicked) {
        window_request_refresh();
    }

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