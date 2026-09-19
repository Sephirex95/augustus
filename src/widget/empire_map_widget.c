#include "empire_map_widget.h"

#include "assets/assets.h"
#include "city/finance.h"
#include "game/resource.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "widget/text_block.h"


#include <stdint.h>

#define WIDGET_WIDTH 400
#define WIDGET_HEIGHT 36

static struct {
    int x;
    int y;
    int initialised;
    lang_fragment money_fragments[3];
    lang_sequence money_sequence;
    text_block date_block;
    text_block money_block;
} widget_data;

static void draw_background(int x, int y)
{
    graphics_set_clip_rectangle(x, y, WIDGET_WIDTH, WIDGET_HEIGHT);
    inner_panel_draw_colored(x, y, WIDGET_WIDTH, WIDGET_HEIGHT, COLOR_MASK_NONE);
    graphics_reset_clip_rectangle();
}

void widget_empire_map_initialise(int x, int y)
{
    widget_data.x = x;
    widget_data.y = y;
    widget_data.initialised = 1;
    lang_seq_frag_number(&widget_data.money_fragments[0], city_finance_treasury());
    lang_seq_frag_space(&widget_data.money_fragments[1], 3);
    lang_seq_frag_label(&widget_data.money_fragments[2], CUSTOM_TRANSLATION, TR_WIDGET_DN);
    widget_data.money_sequence.count = 3;
    widget_data.money_sequence.fragments = widget_data.money_fragments;

    widget_text_block_init_simple(&widget_data.money_block, x + 4, y + 4, WIDGET_WIDTH * 40 / 100, WIDGET_HEIGHT - 8,
        &widget_data.money_sequence, 5);

    resource_data *money = resource_get_data(RESOURCE_DENARII);
    widget_data.money_block.image_before = money->image.icon;
    widget_data.money_block.image_after = money->image.icon;
    widget_data.money_block.font = FONT_NORMAL_GREEN;
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
    segmented_border_draw(widget_data.x + offset_x, widget_data.y + offset_y, WIDGET_WIDTH, WIDGET_HEIGHT);

    draw_text_block_with_offset(&widget_data.money_block, offset_x, offset_y);

    int banner_id = assets_get_image_id("UI", "Victory_Banner");
    const image *banner = image_get(banner_id);
    image_draw(banner_id, widget_data.x + offset_x + (WIDGET_WIDTH - banner->width) / 2,
        widget_data.y + offset_y + (WIDGET_HEIGHT - banner->height) / 2 + 4, COLOR_MASK_NONE, SCALE_NONE);
}

int widget_empire_map_widget_width(void)
{
    return WIDGET_WIDTH;
}

int widget_empire_map_widget_height(void)
{
    return WIDGET_HEIGHT;
}
