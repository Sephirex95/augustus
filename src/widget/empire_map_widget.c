#include "empire_map_widget.h"

#include "assets/assets.h"
#include "city/finance.h"
#include "core/lang.h"
#include "core/locale.h"
#include "game/time.h"
#include "game/resource.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "widget/text_block.h"
#include "widget/top_menu.h"


#include <stdio.h>
#include <stdint.h>

#define WIDGET_DESIRED_WIDTH 560
#define WIDGET_MIN_VISIBLE_WIDTH 240
#define WIDGET_MAX_AVAILABLE_PERCENT 80
#define WIDGET_HEIGHT 36
#define CENTER_RESERVED_WIDTH 80
#define TEXT_BLOCK_HEIGHT (WIDGET_HEIGHT - 8)
#define TEXT_BLOCK_MARGIN_X 20
#define TEXT_BLOCK_Y 4

static struct {
    int x;
    int y;
    int width;
    int initialised;
    uint8_t date_text[64];
    lang_fragment money_fragments[3];
    lang_sequence money_sequence;
    text_block date_block;
    text_block money_block;
} widget_data;

static void draw_background(int x, int y)
{
    graphics_set_clip_rectangle(x, y, widget_data.width, WIDGET_HEIGHT);
    inner_panel_draw_colored(x, y, widget_data.width, WIDGET_HEIGHT, COLOR_MASK_NONE);
    graphics_reset_clip_rectangle();
}

static int text_block_text_width_available(const text_block *block)
{
    int width = block->width - 2 * block->inner_padding_x;

    if (block->image_before > 0) {
        const image *img = image_get(block->image_before);
        width -= img->original.width + 2;
    }
    if (block->image_after > 0) {
        const image *img = image_get(block->image_after);
        width -= img->original.width + 2;
    }

    return width < 0 ? 0 : width;
}

static void set_date_text(int include_day, int include_month)
{
    int year = game_time_year();
    int display_year = year < 0 ? -year : year;
    const uint8_t *month = lang_get_string(25, game_time_month());
    const uint8_t *era = lang_get_string(20, year >= 0 ? 1 : 0);
    int day = widget_top_menu_get_cosmetic_day_of_month();

    if (include_day) {
        if (year >= 0 && !locale_year_before_ad()) {
            snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%d %s %s %d", day,
                (const char *) month, (const char *) era, display_year);
        } else {
            snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%d %s %d %s", day,
                (const char *) month, display_year, (const char *) era);
        }
    } else if (include_month) {
        if (year >= 0 && !locale_year_before_ad()) {
            snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%s %s %d",
                (const char *) month, (const char *) era, display_year);
        } else {
            snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%s %d %s",
                (const char *) month, display_year, (const char *) era);
        }
    } else if (year >= 0 && !locale_year_before_ad()) {
        snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%s %d", (const char *) era,
            display_year);
    } else {
        snprintf((char *) widget_data.date_text, sizeof(widget_data.date_text), "%d %s", display_year,
            (const char *) era);
    }
}

static void update_date_text(void)
{
    int max_width = text_block_text_width_available(&widget_data.date_block);

    set_date_text(1, 1);
    if (text_get_width(widget_data.date_text, widget_data.date_block.font) <= max_width) {
        return;
    }
    set_date_text(0, 1);
    if (text_get_width(widget_data.date_text, widget_data.date_block.font) <= max_width) {
        return;
    }
    set_date_text(0, 0);
}

static void update_money_text(void)
{
    lang_seq_frag_number(&widget_data.money_fragments[2], city_finance_treasury());
}

int widget_empire_map_widget_width_for_available(int available_width)
{
    int width = available_width * WIDGET_MAX_AVAILABLE_PERCENT / 100;

    if (width > WIDGET_DESIRED_WIDTH) {
        width = WIDGET_DESIRED_WIDTH;
    }
    if (width < WIDGET_MIN_VISIBLE_WIDTH) {
        width = 0;
    }

    return width;
}

void widget_empire_map_initialise(int x, int y, int width)
{
    int text_block_width = (width - CENTER_RESERVED_WIDTH - 2 * TEXT_BLOCK_MARGIN_X) / 2;

    widget_data.x = x;
    widget_data.y = y;
    widget_data.width = width;
    widget_data.initialised = 1;
    lang_seq_frag_label(&widget_data.money_fragments[0], CUSTOM_TRANSLATION, TR_WIDGET_DN);
    lang_seq_frag_space(&widget_data.money_fragments[1], 3);
    lang_seq_frag_number(&widget_data.money_fragments[2], city_finance_treasury());
    widget_data.money_sequence.count = 3;
    widget_data.money_sequence.fragments = widget_data.money_fragments;

    widget_text_block_init_simple(&widget_data.money_block, x + TEXT_BLOCK_MARGIN_X, y + TEXT_BLOCK_Y,
        text_block_width, TEXT_BLOCK_HEIGHT, &widget_data.money_sequence, SEQUENCE_POSITION_CENTER);

    resource_data *money = resource_get_data(RESOURCE_DENARII);
    widget_data.money_block.image_before = money->image.icon;
    widget_data.money_block.image_after = money->image.icon;
    widget_data.money_block.font = FONT_NORMAL_GREEN;
    widget_data.money_block.text_offset_x = 4;
    widget_data.money_block.text_offset_y = 1;
    widget_data.money_block.tooltip_c.type = TOOLTIP_BUTTON;
    widget_data.money_block.tooltip_c.text_group = 68;
    widget_data.money_block.tooltip_c.text_id = 60;

    widget_text_block_init_simple(&widget_data.date_block,
        x + width - TEXT_BLOCK_MARGIN_X - text_block_width, y + TEXT_BLOCK_Y,
        text_block_width, TEXT_BLOCK_HEIGHT, NULL, SEQUENCE_POSITION_CENTER);
    widget_data.date_block.raw_text = widget_data.date_text;
    widget_data.date_block.image_before = assets_lookup_image_id(ASSET_UI_HOURGLASS_ICON);
    widget_data.date_block.image_after = assets_lookup_image_id(ASSET_UI_HOURGLASS_ICON);
    widget_data.date_block.font = FONT_NORMAL_GREEN;
    widget_data.date_block.text_offset_y = 1;
    widget_data.date_block.tooltip_c.type = TOOLTIP_BUTTON;
    widget_data.date_block.tooltip_c.text_group = 68;
    widget_data.date_block.tooltip_c.text_id = 62;
    update_date_text();
}

static void draw_text_block_with_offset(text_block *block, int offset_x, int offset_y)
{
    block->x += offset_x;
    block->y += offset_y;
    widget_text_block_draw(block);
    block->x -= offset_x;
    block->y -= offset_y;
}

void widget_empire_map_widget_draw(int offset_x, int offset_y)
{
    if (!widget_data.initialised) {
        return;
    }

    draw_background(widget_data.x + offset_x, widget_data.y + offset_y);
    segmented_border_draw(widget_data.x + offset_x, widget_data.y + offset_y, widget_data.width, WIDGET_HEIGHT);

    update_money_text();
    update_date_text();
    draw_text_block_with_offset(&widget_data.money_block, offset_x, offset_y);
    draw_text_block_with_offset(&widget_data.date_block, offset_x, offset_y);

    int banner_id = assets_get_image_id("UI", "Victory_Banner");
    const image *banner = image_get(banner_id);
    image_draw(banner_id, widget_data.x + offset_x + (widget_data.width - banner->width) / 2,
        widget_data.y + offset_y + (WIDGET_HEIGHT - banner->height) / 2 + 4, COLOR_MASK_NONE, SCALE_NONE);
}

static int handle_text_block_mouse_with_offset(text_block *block, const mouse *m, int offset_x, int offset_y)
{
    int handled;

    block->x += offset_x;
    block->y += offset_y;
    handled = widget_text_block_handle_mouse(block, m);
    block->x -= offset_x;
    block->y -= offset_y;

    return handled;
}

int widget_empire_map_widget_handle_mouse(const mouse *m, int offset_x, int offset_y)
{
    if (!widget_data.initialised) {
        return 0;
    }

    handle_text_block_mouse_with_offset(&widget_data.money_block, m, offset_x, offset_y);
    handle_text_block_mouse_with_offset(&widget_data.date_block, m, offset_x, offset_y);

    return 0;
}

int widget_empire_map_widget_handle_tooltip(tooltip_context *c)
{
    if (!widget_data.initialised) {
        return 0;
    }

    return widget_text_block_handle_tooltip(&widget_data.money_block, c) ||
        widget_text_block_handle_tooltip(&widget_data.date_block, c);
}

int widget_empire_map_widget_width(void)
{
    return widget_data.initialised ? widget_data.width : WIDGET_DESIRED_WIDTH;
}

int widget_empire_map_widget_height(void)
{
    return WIDGET_HEIGHT;
}
