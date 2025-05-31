#include "empire.h"

#include "assets/assets.h"
#include "building/menu.h"
#include "city/military.h"
#include "city/warning.h"
#include "core/calc.h"
#include "core/image_group.h"
#include "empire/city.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "empire/trade_route.h"
#include "empire/type.h"
#include "game/tutorial.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/grid_box.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/scrollbar.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "input/scroll.h"
#include "scenario/empire.h"
#include "scenario/invasion.h"
#include "window/advisors.h"
#include "window/city.h"
#include "window/message_dialog.h"
#include "window/popup_dialog.h"
#include "window/resource_settings.h"
#include "window/trade_opened.h"
#include "window/trade_prices.h"
#include "core/log.h"

#include <math.h>

#define WIDTH_BORDER 32
#define HEIGHT_BORDER 136
#define SIDEBAR_WIDTH_PERCENT 0.25f

#define TRADE_DOT_SPACING 10
#define MAX_SIDEBAR_CITIES 64
#define SIDEBAR_CONTENT_MARGIN (WIDTH_BORDER / 2)


typedef struct {
    int x;
    int y;
} px_point;

typedef struct {
    int city_id;
    int x;
    int y;
    int width;
    int height;
} sidebar_city_entry;


static scrollbar_type sidebar_scrollbar;
static sidebar_city_entry sidebar_cities[MAX_SIDEBAR_CITIES];
static int sidebar_city_count = 0;
static grid_box_type sidebar_grid_box;

static px_point trade_amount_px_offsets[5] = {
    { 2, 0 },
    { 5, 2 },
    { 8, 4 },
    { 0, 3 },
    { 4, 6 },
};

static void button_help(int param1, int param2);
static void button_return_to_city(int param1, int param2);
static void button_advisor(int advisor, int param2);
static void button_show_prices(int param1, int param2);
static void button_open_trade(const generic_button *button);
static void button_show_resource_window(const generic_button *button);
static void setup_sidebar_dimensions(void);

static image_button image_button_help[] = {
    {0, 0, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 0, 0, 1}
};
static image_button image_button_return_to_city[] = {
    {0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_return_to_city, button_none, 0, 0, 1}
};
static image_button image_button_advisor[] = {
    {-4, 0, 24, 24, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor, button_none, ADVISOR_TRADE, 0, 1}
};
static image_button image_button_show_prices[] = {
    {-4, 0, 24, 24, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 30, button_show_prices, button_none, 0, 0, 1}
};

static generic_button generic_button_trade_resource[] = { //will be superseeded by layout_trade_resource_row
    {0, 0, 101, 27, button_show_resource_window},
};

static generic_button generic_button_open_trade[] = {
    {30, 56, 440, 26, button_open_trade}
};
typedef struct {
    int x;
    int y;
    resource_type res;
} trade_resource_button;

int layout_trade_resource_row(
    const empire_city *city,
    int is_sell,       // 1 = sells, 0 = buys
    int x_min, int x_max, int y_base, // panel min/max X, base Y of row
    trade_resource_button *out,       // array to fill (must be at least RESOURCE_MAX in length)
    int *out_count                    // how many valid entries written
) {
    int count = 0;
    // Count resources in this row
    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;
        if (is_sell && !city->sells_resource[r]) continue;
        if (!is_sell && !city->buys_resource[r]) continue;
        count++;
    }
    *out_count = count;
    if (count == 0) return 0;

    int screen_width = x_max - x_min;
    int estimated_width_per_resource = (count > 5) ? 90 : 110;
    const int safe_margin_left = x_min + 34;
    const int safe_margin_right = x_max - 44;
    int max_draw_width = safe_margin_right - safe_margin_left;

    int total_width = estimated_width_per_resource * count + 120;
    if (total_width > max_draw_width) total_width = max_draw_width;

    int x_offset = safe_margin_left + (max_draw_width - total_width) / 2;
    // Label indent: replicate draw code logic
    int label_indent = (
        lang_text_get_width(47, 10, FONT_NORMAL_GREEN) >
        lang_text_get_width(47,  9, FONT_NORMAL_GREEN)
    ) 
        ? lang_text_get_width(47, 10, FONT_NORMAL_GREEN)
        : lang_text_get_width(47,  9, FONT_NORMAL_GREEN);
    label_indent += 10;
    int x_cursor = x_offset + label_indent;

    int i = 0;
    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;
        if (is_sell && !city->sells_resource[r]) continue;
        if (!is_sell && !city->buys_resource[r]) continue;

        int trade_max = trade_route_limit(city->route_id, r);
        int trade_now = trade_route_traded(city->route_id, r);
        int w1 = text_get_number_width(trade_now, '@', "", FONT_NORMAL_GREEN);
        int w2 = text_get_number_width(trade_max, '@', "", FONT_NORMAL_GREEN);
        int of_w = lang_text_get_width(47, 11, FONT_NORMAL_GREEN)-10;

        int icon_width = 26, spacing = 2, buffer = 14;
        int total_width = icon_width + spacing + w1 + of_w + w2 - 10 + buffer;

        out[i].x = x_cursor;
        out[i].y = y_base;
        out[i].res = r;
        i++;

        x_cursor += total_width;
    }
    return count;
}

static struct {
    unsigned int selected_button;
    int selected_city;
    int x_min, x_max, y_min, y_max;
    int x_draw_offset, y_draw_offset;
    unsigned int focus_button_id;
    int is_scrolling;
    int finished_scroll;
    resource_type focus_resource;
    struct {
        int x_min;
        int x_max;
    } panel;
    struct {
        int x_min;
        int x_max;
        int y_min;
        int y_max;
        int scroll;
        int scroll_max;

    } sidebar;
} data = { 0, 1 };

static void init(void)
{
    data.selected_button = 0;
    int selected_object = empire_selected_object();
    if (selected_object) {
        data.selected_city = empire_city_get_for_object(selected_object - 1);
    } else {
        data.selected_city = 0;
    }
    data.focus_button_id = 0;
    
}




static void draw_paneling(void)
{
    int image_base = image_group(GROUP_EMPIRE_PANELS);
    int bottom_panel_is_larger = data.x_min != data.panel.x_min;
    int vertical_y_limit = bottom_panel_is_larger ? data.y_max - 120 : data.y_max;

    graphics_set_clip_rectangle(data.panel.x_min, data.y_min,
        data.panel.x_max - data.panel.x_min, data.y_max - data.y_min);

    // bottom panel background
    for (int x = data.panel.x_min; x < data.panel.x_max; x += 70) {
        image_draw(image_base + 3, x, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base + 3, x, data.y_max - 80, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base + 3, x, data.y_max - 40, COLOR_MASK_NONE, SCALE_NONE);
    }

    // horizontal bar borders
    for (int x = data.panel.x_min; x < data.panel.x_max; x += 86) {
        image_draw(image_base + 1, x, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base + 1, x, data.y_max - 16, COLOR_MASK_NONE, SCALE_NONE);
    }

    // extra vertical bar borders
    if (bottom_panel_is_larger) {
        for (int y = vertical_y_limit + 16; y < data.y_max; y += 86) {
            image_draw(image_base, data.panel.x_min, y, COLOR_MASK_NONE, SCALE_NONE);
            image_draw(image_base, data.panel.x_max - 16, y, COLOR_MASK_NONE, SCALE_NONE);
        }
    }

    graphics_set_clip_rectangle(data.x_min, data.y_min, data.x_max - data.x_min, vertical_y_limit - data.y_min);

    for (int x = data.x_min; x < data.x_max; x += 86) {
        image_draw(image_base + 1, x, data.y_min, COLOR_MASK_NONE, SCALE_NONE);
    }

    // vertical bar borders
    for (int y = data.y_min + 16; y < vertical_y_limit; y += 86) {
        image_draw(image_base, data.x_min, y, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base, data.x_max - 16, y, COLOR_MASK_NONE, SCALE_NONE);
    }

    graphics_reset_clip_rectangle();

    // crossbars
    image_draw(image_base + 2, data.x_min, data.y_min, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, data.x_min, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, data.panel.x_min, data.y_max - 16, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, data.x_max - 16, data.y_min, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, data.x_max - 16, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
    image_draw(image_base + 2, data.panel.x_max - 16, data.y_max - 16, COLOR_MASK_NONE, SCALE_NONE);

    if (bottom_panel_is_larger) {
        image_draw(image_base + 2, data.panel.x_min, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
        image_draw(image_base + 2, data.panel.x_max - 16, data.y_max - 120, COLOR_MASK_NONE, SCALE_NONE);
    }
    // Sidebar background 
    graphics_set_clip_rectangle(data.sidebar.x_min, data.sidebar.y_min,
        data.sidebar.x_max - data.sidebar.x_min,
        data.sidebar.y_max - data.sidebar.y_min);
    int asset_id = assets_lookup_image_id(ASSET_UI_VERTICAL_EMPIRE_PANEL);

    for (int x = data.sidebar.x_min; x < data.sidebar.x_max; x += 40) {
        for (int y = data.sidebar.y_min; y < data.sidebar.y_max; y += 70) {
            image_draw(asset_id, x, y, COLOR_MASK_NONE, SCALE_NONE);
        }
    }

    //sidebar decorative border
    for (int y = data.sidebar.y_min; y < data.sidebar.y_max; y += 86) {
        image_draw(image_base, data.sidebar.x_min, y, COLOR_MASK_NONE, SCALE_NONE);
    }
    graphics_reset_clip_rectangle();
    scrollbar_draw(&sidebar_scrollbar);

}

static void draw_trade_resource(resource_type resource, int trade_max, int x_offset, int y_offset)
{
    graphics_draw_inset_rect(x_offset, y_offset, 26, 26, COLOR_INSET_DARK, COLOR_INSET_LIGHT);

    image_draw(resource_get_data(resource)->image.empire, x_offset + 1, y_offset + 1, COLOR_MASK_NONE, SCALE_NONE);

    if (data.focus_resource == resource) {
        button_border_draw(x_offset - 2, y_offset - 2, 101 + 4, 30, 1);
    }

    window_empire_draw_resource_shields(trade_max, x_offset, y_offset);
}

void window_empire_draw_resource_shields(int trade_max, int x_offset, int y_offset)
{
    int num_bronze_shields = (trade_max % 100) / 20 + 1;
    if (trade_max >= 600) {
        num_bronze_shields = 5;
    }

    int top_left_x;
    if (num_bronze_shields == 1) {
        top_left_x = x_offset + 19;
    } else if (num_bronze_shields == 2) {
        top_left_x = x_offset + 15;
    } else {
        top_left_x = x_offset + 11;
    }
    int top_left_y = y_offset - 1;
    int bronze_shield = image_group(GROUP_TRADE_AMOUNT);
    for (int i = 0; i < num_bronze_shields; i++) {
        px_point pt = trade_amount_px_offsets[i];
        image_draw(bronze_shield, top_left_x + pt.x, top_left_y + pt.y, COLOR_MASK_NONE, SCALE_NONE);
    }

    int num_gold_shields = trade_max / 100;
    if (num_gold_shields > 5) {
        num_gold_shields = 5;
    }
    top_left_x = x_offset - 1;
    top_left_y = y_offset + 22;
    int gold_shield = assets_lookup_image_id(ASSET_GOLD_SHIELD);
    for (int i = 0; i < num_gold_shields; i++) {
        image_draw(gold_shield, top_left_x + i * 3, top_left_y, COLOR_MASK_NONE, SCALE_NONE);
    }
}
static int count_trade_resources(const empire_city *city, int is_sell) {
    int count = 0;
    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (resource_is_storable(r)) {
            if ((is_sell && city->sells_resource[r]) ||
                (!is_sell && city->buys_resource[r])) {
                count++;
            }
        }
    }
    return count;
}

static void draw_trade_row(const empire_city *city, int is_sell, int x_offset, int y_offset, int max_allowed_width) {
    const int icon_width = 26;
    const int max_digits_width = text_get_number_width(99999, '@', "", FONT_NORMAL_GREEN);
    const int text_of_width = lang_text_get_width(47, 11, FONT_NORMAL_GREEN);

    int resource_count = count_trade_resources(city, is_sell);
    int squeeze_spacing = resource_count > 5;

    int spacing = squeeze_spacing ? 0 : 8;
    int buffer = squeeze_spacing ? 0 : 14;
    int label_padding = squeeze_spacing ? 0 : 10;
    int of_padding = squeeze_spacing ? -12 : 4;
    int first_padding = squeeze_spacing ? 0 : 10;
    int final_width_adjustment = squeeze_spacing ? -20 : 0;
    int random_spacing = squeeze_spacing ? -6 : -1;
    int label_indent = (
        lang_text_get_width(47, 10, FONT_NORMAL_GREEN) > 
        lang_text_get_width(47,  9, FONT_NORMAL_GREEN)
    ) 
        ? lang_text_get_width(47, 10, FONT_NORMAL_GREEN)
        : lang_text_get_width(47,  9, FONT_NORMAL_GREEN);
    int label_id = is_sell ? 10 : 9;
    lang_text_draw(47, label_id, x_offset, y_offset, FONT_NORMAL_GREEN);
    int x_cursor = x_offset + label_indent + first_padding;

    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;
        if ((is_sell && !city->sells_resource[r]) || (!is_sell && !city->buys_resource[r])) continue;
    
        int trade_max = trade_route_limit(city->route_id, r);
        int trade_now = trade_route_traded(city->route_id, r);
        int display_trade_now = (trade_max == 1000 && trade_now == 0) ? 1000 : trade_now;
    
        int w1_est = text_draw_number(display_trade_now, '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
        int w2_est = text_draw_number(trade_max, '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
        int segment_width = icon_width + spacing + w1_est + text_of_width + w2_est +final_width_adjustment;
        
        if (x_cursor + segment_width > data.panel.x_max - 44 - 3) {
            text_draw((const uint8_t *)"(...)", x_cursor + 4, y_offset, FONT_NORMAL_GREEN, 0);
            break;
        }
            
        // Draw actual resource segment
        draw_trade_resource(r, trade_max, x_cursor, y_offset - 9);
        x_cursor -= 4;
    
        int w1 = text_draw_number(display_trade_now, '@', "", x_cursor + icon_width + spacing, y_offset, FONT_NORMAL_GREEN, 0);
        int of_x_offset = x_cursor + icon_width + spacing + w1 + random_spacing;
        int w_of = lang_text_draw(47, 11, of_x_offset, y_offset, FONT_NORMAL_GREEN);
        int w2 = text_draw_number(trade_max, '@', "", of_x_offset + w_of + random_spacing*2, y_offset, FONT_NORMAL_GREEN, 0);
    
        x_cursor += segment_width;
    }
}

static void draw_trade_city_info(const empire_object *object, const empire_city *city) {
    int y_offset = data.y_max - 113;

    if (city->is_open) {
        int screen_width = data.x_max - data.x_min;
        int num_sells = count_trade_resources(city, 1);  // is_sell = 1
        int num_buys = count_trade_resources(city, 0);   // is_sell = 0
        int max_resources = (num_sells > num_buys) ? num_sells : num_buys;

        int estimated_width_per_resource = (max_resources > 5) ? 90 : 110;
        // Define visually safe boundaries based on button layout
        const int safe_margin_left = data.panel.x_min + 34;
        const int safe_margin_right = data.panel.x_max - 44;

        int max_draw_width = safe_margin_right - safe_margin_left;

        // Estimate content width
        int total_width = estimated_width_per_resource * max_resources + 120;
        if (total_width > max_draw_width) total_width = max_draw_width;

        // Align centered within the safe region
        int x_offset = safe_margin_left + (max_draw_width - total_width) / 2;

        draw_trade_row(city, 1, x_offset, y_offset + 40, total_width); // Sells
        draw_trade_row(city, 0, x_offset, y_offset + 71, total_width); // Buys

    } else {
        int y_trade_row = y_offset + 42;
        
        const int safe_margin_left = data.panel.x_min + 34;
        const int safe_margin_right = data.panel.x_max - 44;
        int max_draw_width = safe_margin_right - safe_margin_left;
    
        // Count sell and buy resources
        int sell_count = 0, buy_count = 0;
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (resource_is_storable(r) && city->sells_resource[r]) sell_count++;
            if (resource_is_storable(r) && city->buys_resource[r]) buy_count++;
        }
        int total_resources = sell_count + buy_count;
        int squeeze_spacing = total_resources > 5;
    
        // Spacing
        int icon_width = 26;
        int icon_to_number_spacing = squeeze_spacing ? 2 : 8;
        int between_resource_spacing = squeeze_spacing ? 0 : 14;
        int first_section_spacing = squeeze_spacing ? 4 : 10;
        int after_label_spacing = squeeze_spacing ? 4 : 10;
    
        // Estimate widths for segments
        int label_sells_width = lang_text_get_width(47, 5, FONT_NORMAL_GREEN);
        int label_buys_width  = lang_text_get_width(47, 4, FONT_NORMAL_GREEN);
        int label_indent = 10;

        int resource_width_sum = 0;
    
        // Sells
        if (sell_count) resource_width_sum += label_sells_width + first_section_spacing;
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!resource_is_storable(r) || !city->sells_resource[r]) continue;
            int trade_max = trade_route_limit(city->route_id, r);
            int num_width = text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
            resource_width_sum += icon_width + icon_to_number_spacing + num_width + between_resource_spacing;
        }
        // Buys

        if (buy_count) resource_width_sum += label_buys_width + after_label_spacing;
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!resource_is_storable(r) || !city->buys_resource[r]) continue;
            int trade_max = trade_route_limit(city->route_id, r);
            int num_width = text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
            resource_width_sum += icon_width + icon_to_number_spacing + num_width + between_resource_spacing;
        }
    
        // Clamp to available width
        if (resource_width_sum > max_draw_width)
            resource_width_sum = max_draw_width;
    
        // Centered x starting point
        
        int x_offset = safe_margin_left + (max_draw_width - resource_width_sum) / 2;
        x_offset += label_indent + first_section_spacing;
        int x_cursor = x_offset;
    
        // --- Draw "Sells" label and resources ---
        if (sell_count) {
            lang_text_draw(47, 5, x_cursor, y_trade_row, FONT_NORMAL_GREEN); // "Sells"
            x_cursor += label_sells_width + first_section_spacing;
        }
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!resource_is_storable(r) || !city->sells_resource[r]) continue;
            int trade_max = trade_route_limit(city->route_id, r);
            int num_width = text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
    
            int segment_width = icon_width + icon_to_number_spacing + num_width + between_resource_spacing;
    
            // Truncate if out of space
            if (x_cursor + segment_width > safe_margin_right - 20) {
                text_draw((const uint8_t *)"(...)", x_cursor + 4, y_trade_row, FONT_NORMAL_GREEN, 0);
                goto after_bar; // Finish early
            }
            draw_trade_resource(r, trade_max, x_cursor, y_trade_row - 9);
            x_cursor += icon_width + icon_to_number_spacing;
            text_draw_number(trade_max, 0, 0, x_cursor, y_trade_row, FONT_NORMAL_GREEN, 0);
            x_cursor += num_width + between_resource_spacing;
        }
        x_cursor+=15; //ad visual gap between sells and buys
        // --- Draw "Buys" label and resources ---
        if (buy_count) {
            lang_text_draw(47, 4, x_cursor, y_trade_row, FONT_NORMAL_GREEN); // "Buys"
            x_cursor += label_buys_width + after_label_spacing;
        }
        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (!resource_is_storable(r) || !city->buys_resource[r]) continue;
            int trade_max = trade_route_limit(city->route_id, r);
            int num_width = text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
    
            int segment_width = icon_width + icon_to_number_spacing + num_width + between_resource_spacing;
            if (x_cursor + segment_width > safe_margin_right - 20) {
                text_draw((const uint8_t *)"(...)", x_cursor + 4, y_trade_row, FONT_NORMAL_GREEN, 0);
                break;
            }
            draw_trade_resource(r, trade_max, x_cursor, y_trade_row - 9);
            x_cursor += icon_width + icon_to_number_spacing;
            text_draw_number(trade_max, 0, 0, x_cursor, y_trade_row, FONT_NORMAL_GREEN, 0);
            x_cursor += num_width + between_resource_spacing;
        }
    
    after_bar:
        // --- Restore original x_offset for button/cost (original style) ---
        x_offset = (data.x_min + data.x_max - 500) / 2;
        int cost_index = lang_text_draw_amount(8, 0, city->cost_to_open, x_offset + 40, y_offset + 73, FONT_NORMAL_GREEN);
        lang_text_draw(47, 6, x_offset + cost_index + 40, y_offset + 73, FONT_NORMAL_GREEN);
        int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
        image_draw(image_id, x_offset + 430, y_offset + 65 + 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);
    }
    
    
}


static void draw_sidebar_city_item(const grid_box_item *item)
{
    sidebar_city_entry *entry = &sidebar_cities[item->index];
    empire_city *city = empire_city_get(entry->city_id);
    const uint8_t *name = empire_city_get_name(city);
    int x_blocks = item->width / BLOCK_SIZE;
    int y_blocks = item->height / BLOCK_SIZE;

    // base offset for all content in the box
    int x_offset = item->x;
    int y_offset = item->y;

    // draw background + name + badge
    inner_panel_draw(x_offset, y_offset, x_blocks, y_blocks);
    int badge_id = assets_get_image_id("UI", "Empire_sidebar_city_badge");
    image_draw(badge_id, x_offset + 5, y_offset + 5, COLOR_MASK_NONE, SCALE_NONE);
    text_draw(name, x_offset + BLOCK_SIZE, y_offset + 9, FONT_LARGE_BLACK, 0);

    // draw trade icon
    int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
    image_draw(image_id, x_offset + 250, y_offset + 9+ 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);

    // Move y_offset down for trade info rows
    y_offset += 44;

    if (city->is_open) {
        // --- Sells ---
        int label_indent = (
            lang_text_get_width(47, 10, FONT_NORMAL_GREEN) > 
            lang_text_get_width(47,  9, FONT_NORMAL_GREEN)
        ) 
            ? lang_text_get_width(47, 10, FONT_NORMAL_GREEN)
            : lang_text_get_width(47,  9, FONT_NORMAL_GREEN);
        
        label_indent += 10;
        

        lang_text_draw(47, 10, x_offset + 10, y_offset + 6, FONT_NORMAL_GREEN);
        int x_cursor = x_offset + label_indent;
        for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (city->sells_resource[r] && resource_is_storable(r)) {
                int trade_max = trade_route_limit(city->route_id, r);
                int trade_now = trade_route_traded(city->route_id, r);
                if (trade_now > trade_max) trade_max = trade_now;

                // Draw icon
                draw_trade_resource(r, trade_max, x_cursor, y_offset);

                int text_x = x_cursor + 26 + 2;  // icon + spacing
                int text_y = y_offset + 5;       // align vertically with icon
                
                int w1 = text_draw_number(trade_now, '@', "", text_x, text_y+3, FONT_NORMAL_GREEN, 0);
                
                // draw "of"
                int of_x = text_x + w1-2;
                int of_w = lang_text_get_width(47, 11, FONT_NORMAL_GREEN)-10;
                lang_text_draw(47, 11, of_x, text_y+3, FONT_NORMAL_GREEN);
                
                // draw max
                int w2 = text_draw_number(trade_max, '@', "", of_x + of_w, text_y+3, FONT_NORMAL_GREEN, 0);
                
                // now calculate total width, but manually squeeze the padding between each section
                int total_width = 26 /*icon*/ + 2 /*space*/ + w1 + of_w + w2 - 10 /*manual squeeze*/ + 14 /*buffer*/;
                x_cursor += total_width;
                

            }
        }

        // --- Buys ---
        y_offset += 32;
        lang_text_draw(47, 9, x_offset + 10, y_offset + 8, FONT_NORMAL_GREEN);
        x_cursor = x_offset + label_indent;
        for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (city->buys_resource[r] && resource_is_storable(r)) {
                int trade_max = trade_route_limit(city->route_id, r);
                int trade_now = trade_route_traded(city->route_id, r);
                if (trade_now > trade_max) trade_max = trade_now;

                // Draw icon
                draw_trade_resource(r, trade_max, x_cursor, y_offset);

                int text_x = x_cursor + 26 + 2;  // icon + spacing
                int text_y = y_offset + 5;       // align vertically with icon
                
                int w1 = text_draw_number(trade_now, '@', "", text_x, text_y+3, FONT_NORMAL_GREEN, 0);
                
                // draw "of"
                int of_x = text_x + w1-2;
                int of_w = lang_text_get_width(47, 11, FONT_NORMAL_GREEN)-10;
                lang_text_draw(47, 11, of_x, text_y+3, FONT_NORMAL_GREEN);
                
                // draw max
                int w2 = text_draw_number(trade_max, '@', "", of_x + of_w, text_y+3, FONT_NORMAL_GREEN, 0);
                
                // now calculate total width, but manually squeeze the padding between each section
                int total_width = 26 /*icon*/ + 2 /*space*/ + w1 + of_w + w2 - 10 /*manual squeeze*/ + 14 /*buffer*/;
                x_cursor += total_width;

            }
        }

    } else {
        // --- Closed city ---
        y_offset += 56;
        int index = lang_text_draw_amount(8, 0, city->cost_to_open, x_offset + 10, y_offset, FONT_NORMAL_GREEN);
        lang_text_draw(47, 6, x_offset + 10 + index, y_offset, FONT_NORMAL_GREEN);
        int icon = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
        image_draw(icon, x_offset + 250, y_offset + 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);
    }
}


static void setup_sidebar(void)
{
    // Calculate sidebar bounds
    int s_width = screen_width();
    int s_height = screen_height();
    int map_width, map_height;
    empire_get_map_size(&map_width, &map_height);

    int max_width = map_width + WIDTH_BORDER;
    int max_height = map_height + HEIGHT_BORDER;

    data.x_min = s_width <= max_width ? 0 : (s_width - max_width) / 2;
    data.x_max = s_width <= max_width ? s_width : data.x_min + max_width;
    data.y_min = s_height <= max_height ? 0 : (s_height - max_height) / 2;
    data.y_max = s_height <= max_height ? s_height : data.y_min + max_height;

    int map_draw_x_min = data.x_min + WIDTH_BORDER / 2;
    int map_draw_x_max = data.x_max - WIDTH_BORDER / 2;
    int map_draw_y_min = data.y_min + WIDTH_BORDER / 2;
    int map_draw_y_max = data.y_max - 120;

    int usable_map_width = map_draw_x_max - map_draw_x_min;

    int raw_sidebar_width = (int)(usable_map_width * SIDEBAR_WIDTH_PERCENT);
    int sidebar_width = (raw_sidebar_width / 40) * 40;

    data.sidebar.x_min = map_draw_x_max - sidebar_width;
    data.sidebar.x_max = map_draw_x_max;
    data.sidebar.y_min = map_draw_y_min;
    data.sidebar.y_max = map_draw_y_max;

    // Prepare sidebar entries
    int y = data.sidebar.y_min;
    sidebar_city_count = 0;
    for (int i = 0; i < empire_city_get_array_size(); i++) {
        empire_city *city = empire_city_get(i);
        if (!city->in_use || city->type != EMPIRE_CITY_TRADE) continue;

        if (sidebar_city_count >= MAX_SIDEBAR_CITIES) break;

        sidebar_city_entry *entry = &sidebar_cities[sidebar_city_count++];
        entry->city_id = i;
        entry->x = data.sidebar.x_min;
        entry->y = y;
        entry->width = sidebar_width;
        entry->height = 120;

        y += entry->height;
    }

    // Setup grid box
    const int sidebar_margin_left = 25;
    const int sidebar_margin_right = 12;
    const int sidebar_margin_top = 6;
    const int sidebar_margin_bottom = 6;

    sidebar_grid_box.x = data.sidebar.x_min + sidebar_margin_left;
    sidebar_grid_box.y = data.sidebar.y_min + sidebar_margin_top;
    sidebar_grid_box.width = data.sidebar.x_max - data.sidebar.x_min - sidebar_margin_left - sidebar_margin_right;
    sidebar_grid_box.height = data.sidebar.y_max - data.sidebar.y_min - sidebar_margin_top - sidebar_margin_bottom;
    sidebar_grid_box.item_height = 120;
    sidebar_grid_box.num_columns = 1;
    sidebar_grid_box.item_margin.horizontal = 0;
    sidebar_grid_box.item_margin.vertical = 0;
    sidebar_grid_box.draw_inner_panel = 0;
    sidebar_grid_box.extend_to_hidden_scrollbar = 0;
    sidebar_grid_box.decorate_scrollbar = 1;
    sidebar_grid_box.total_items = sidebar_city_count;
    sidebar_grid_box.draw_item = draw_sidebar_city_item;
    sidebar_grid_box.on_click = NULL;
    sidebar_grid_box.handle_tooltip = NULL;

    grid_box_init(&sidebar_grid_box, sidebar_city_count);
}



static void draw_city_info(const empire_object *object)
{
    int x_offset = (data.x_min + data.x_max - 240) / 2;
    int y_offset = data.y_max - 88;

    const empire_city *city = empire_city_get(data.selected_city);
    switch (city->type) {
        case EMPIRE_CITY_DISTANT_ROMAN:
            lang_text_draw_centered(47, 12, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_VULNERABLE_ROMAN:
            if (city_military_distant_battle_city_is_roman()) {
                lang_text_draw_centered(47, 12, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            } else {
                lang_text_draw_centered(47, 13, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            }
            break;
        case EMPIRE_CITY_FUTURE_TRADE:
        case EMPIRE_CITY_DISTANT_FOREIGN:
        case EMPIRE_CITY_FUTURE_ROMAN:
            lang_text_draw_centered(47, 0, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_OURS:
            lang_text_draw_centered(47, 1, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_TRADE:
            draw_trade_city_info(object, city);
            const empire_city *city = empire_city_get(data.selected_city);
            break;
    }
}

static void draw_roman_army_info(const empire_object *object)
{
    int x_offset = (data.x_min + data.x_max - 240) / 2;
    int y_offset = data.y_max - 68;
    int text_id;
    if (city_military_distant_battle_roman_army_is_traveling_forth()) {
        text_id = 15;
    } else {
        text_id = 16;
    }
    lang_text_draw_multiline(47, text_id, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
}

static void draw_enemy_army_info(const empire_object *object)
{
    lang_text_draw_multiline(47, 14,
        (data.x_min + data.x_max - 240) / 2,
        data.y_max - 68,
        240, FONT_NORMAL_GREEN);
}

static void draw_object_info(void)
{
    int selected_object = empire_selected_object();
    if (selected_object) {
        const empire_object *object = empire_object_get(selected_object - 1);
        switch (object->type) {
            case EMPIRE_OBJECT_CITY:
                draw_city_info(object);
                break;
            case EMPIRE_OBJECT_ROMAN_ARMY:
                if (city_military_distant_battle_roman_army_is_traveling()) {
                    if (city_military_distant_battle_roman_months_traveled() == object->distant_battle_travel_months) {
                        draw_roman_army_info(object);
                    }
                }
                break;
            case EMPIRE_OBJECT_ENEMY_ARMY:
                if (city_military_months_until_distant_battle() > 0) {
                    if (city_military_distant_battle_enemy_months_traveled() == object->distant_battle_travel_months) {
                        draw_enemy_army_info(object);
                    }
                }
                break;
            default:
                lang_text_draw_centered(47, 8, data.panel.x_min, data.y_max - 48,
                    data.panel.x_max - data.panel.x_min, FONT_NORMAL_GREEN);
                break;
        }
    } else {
        lang_text_draw_centered(47, 8, data.panel.x_min, data.y_max - 48,
            data.panel.x_max - data.panel.x_min, FONT_NORMAL_GREEN);
    }
}

static void draw_background(void)
{
    int s_width = screen_width();
    int s_height = screen_height();
    int map_width, map_height;
    empire_get_map_size(&map_width, &map_height);
    int max_width = map_width + WIDTH_BORDER;
    int max_height = map_height + HEIGHT_BORDER;
    
    data.x_min = s_width <= max_width ? 0 : (s_width - max_width) / 2;
    data.x_max = s_width <= max_width ? s_width : data.x_min + max_width;
    data.y_min = s_height <= max_height ? 0 : (s_height - max_height) / 2;
    data.y_max = s_height <= max_height ? s_height : data.y_min + max_height;

    int bottom_panel_width = data.x_max - data.x_min;
    if (bottom_panel_width < 608) {
        bottom_panel_width = 640;
        int difference = bottom_panel_width - (data.x_max - data.x_min);
        int odd = difference % 1;
        difference /= 2;
        data.panel.x_min = data.x_min - difference - odd;
        data.panel.x_max = data.x_max + difference;
    } else {
        data.panel.x_min = data.x_min;
        data.panel.x_max = data.x_max;
    }

    if (data.x_min || data.y_min) {
        image_draw_blurred_fullscreen(image_group(GROUP_EMPIRE_MAP), 3);
        graphics_shade_rect(0, 0, screen_width(), screen_height(), 7);
    }
    // --- Calculate usable map area inside decorative borders ---
    int map_draw_x_min = data.x_min + 16;
    int map_draw_x_max = data.x_max - 16;
    int map_draw_y_min = data.y_min + 16;
    int map_draw_y_max = data.y_max - 120;  // Already done for bottom panel

    int usable_map_width = map_draw_x_max - map_draw_x_min;
    int usable_map_height = map_draw_y_max - map_draw_y_min;

    // --- Reserve 20% of usable map WIDTH for sidebar ---
    int raw_sidebar_width = (int)(usable_map_width * SIDEBAR_WIDTH_PERCENT);
    int tile_width = 40;
    int sidebar_width = (raw_sidebar_width / tile_width) * tile_width;  // round down to nearest multiple of 40


    // --- Store final sidebar coordinates in new variables ---
    data.sidebar.x_min = map_draw_x_max - sidebar_width;
    data.sidebar.x_max = map_draw_x_max;
    data.sidebar.y_min = map_draw_y_min;
    data.sidebar.y_max = map_draw_y_max;

    
}

static int draw_images_at_interval(int image_id, int x_draw_offset, int y_draw_offset,
    int start_x, int start_y, int end_x, int end_y, int interval, int remaining)
{
    int x_diff = end_x - start_x;
    int y_diff = end_y - start_y;
    int dist = (int) sqrt(x_diff * x_diff + y_diff * y_diff);
    int x_factor = calc_percentage(x_diff, dist);
    int y_factor = calc_percentage(y_diff, dist);
    int offset = interval - remaining;
    if (offset > dist) {
        return offset;
    }
    dist -= offset;
    int num_dots = dist / interval;
    remaining = dist % interval;
    if (image_id) {
        for (int j = 0; j <= num_dots; j++) {
            int x = calc_adjust_with_percentage(j * interval + offset, x_factor) + start_x;
            int y = calc_adjust_with_percentage(j * interval + offset, y_factor) + start_y;
            image_draw(image_id, x_draw_offset + x, y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE);
        }
    }
    return remaining;
}

void window_empire_draw_trade_waypoints(const empire_object *trade_route, int x_offset, int y_offset)
{
    const empire_object *our_city = empire_object_get_our_city();
    const empire_object *trade_city = empire_object_get_trade_city(trade_route->trade_route_id);
    int last_x = our_city->x + 25;
    int last_y = our_city->y + 25;
    int remaining = TRADE_DOT_SPACING;
    int image_id = trade_route->type == EMPIRE_OBJECT_LAND_TRADE_ROUTE ?
        assets_get_image_id("UI", "LandRouteDot") :
        assets_get_image_id("UI", "SeaRouteDot");
    for (int i = trade_route->id + 1; i < empire_object_count(); i++) {
        empire_object *obj = empire_object_get(i);
        if (obj->type != EMPIRE_OBJECT_TRADE_WAYPOINT || obj->trade_route_id != trade_route->trade_route_id) {
            break;
        }
        remaining = draw_images_at_interval(image_id, x_offset, y_offset, last_x, last_y, obj->x, obj->y,
            TRADE_DOT_SPACING, remaining);
        last_x = obj->x;
        last_y = obj->y;
    }
    draw_images_at_interval(image_id, x_offset, y_offset, last_x, last_y, trade_city->x + 25, trade_city->y + 25,
        TRADE_DOT_SPACING, remaining);
}

void window_empire_draw_border(const empire_object *border, int x_offset, int y_offset)
{
    const empire_object *first_edge = empire_object_get(border->id + 1);
    if (first_edge->type != EMPIRE_OBJECT_BORDER_EDGE) {
        return;
    }
    int last_x = first_edge->x;
    int last_y = first_edge->y;
    int image_id = first_edge->image_id;
    int remaining = border->width;

    // Align the coordinate to the base of the border flag's mast
    x_offset -= 0;
    y_offset -= 14;

    for (int i = first_edge->id + 1; i < empire_object_count(); i++) {
        empire_object *obj = empire_object_get(i);
        if (obj->type != EMPIRE_OBJECT_BORDER_EDGE) {
            break;
        }
        int animation_offset = 0;
        int x = x_offset;
        int y = y_offset;
        if (image_id) {
            const image *img = image_get(image_id);
            draw_images_at_interval(image_id, x, y, last_x, last_y, obj->x, obj->y, border->width, remaining);
            if (img->animation && img->animation->speed_id) {
                animation_offset = empire_object_update_animation(obj, image_id);
                x += img->animation->sprite_offset_x;
                y += img->animation->sprite_offset_y;
            }
            remaining = draw_images_at_interval(image_id + animation_offset, x, y, last_x, last_y, obj->x, obj->y,
                border->width, remaining);
        } else {
            remaining = border->width;
        }
        last_x = obj->x;
        last_y = obj->y;
        image_id = obj->image_id;
    }
    if (!image_id) {
        return;
    }
    int animation_offset = 0;
    const image *img = image_get(image_id);
    if (img->animation && img->animation->speed_id) {
        animation_offset = empire_object_update_animation(border, image_id);
    }
    draw_images_at_interval(image_id, x_offset, y_offset, last_x, last_y, first_edge->x, first_edge->y,
        border->width, remaining);
    if (animation_offset) {
        draw_images_at_interval(image_id + animation_offset,
                x_offset + img->animation->sprite_offset_x, y_offset + img->animation->sprite_offset_y,
                last_x, last_y, first_edge->x, first_edge->y, border->width, remaining);
    }
}

static void draw_empire_object(const empire_object *obj)
{
    if (obj->type == EMPIRE_OBJECT_TRADE_WAYPOINT || obj->type == EMPIRE_OBJECT_BORDER_EDGE) {
        return;
    }
    if (obj->type == EMPIRE_OBJECT_LAND_TRADE_ROUTE || obj->type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
        if (!empire_city_is_trade_route_open(obj->trade_route_id)) {
            return;
        }
        if (scenario_empire_id() == SCENARIO_CUSTOM_EMPIRE) {
            window_empire_draw_trade_waypoints(obj, data.x_draw_offset, data.y_draw_offset);
        }
    }
    int x, y, image_id;
    if (scenario_empire_is_expanded()) {
        x = obj->expanded.x;
        y = obj->expanded.y;
        image_id = obj->expanded.image_id;
    } else {
        x = obj->x;
        y = obj->y;
        image_id = obj->image_id;
    }
    if (obj->type == EMPIRE_OBJECT_BORDER) {
        window_empire_draw_border(obj, data.x_draw_offset, data.y_draw_offset);
    }
    if (obj->type == EMPIRE_OBJECT_CITY) {
        const empire_city *city = empire_city_get(empire_city_get_for_object(obj->id));
        if (city->type == EMPIRE_CITY_DISTANT_FOREIGN ||
            city->type == EMPIRE_CITY_FUTURE_ROMAN) {
            image_id = image_group(GROUP_EMPIRE_FOREIGN_CITY);
        } else if (city->type == EMPIRE_CITY_TRADE) {
            // Fix cases where empire map still gives a blue flag for new trade cities
            // (e.g. Massilia in campaign Lugdunum)
            image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        }
    }
    if (obj->type == EMPIRE_OBJECT_BATTLE_ICON) {
        // handled later
        return;
    }
    if (obj->type == EMPIRE_OBJECT_ENEMY_ARMY) {
        if (city_military_months_until_distant_battle() <= 0) {
            return;
        }
        if (city_military_distant_battle_enemy_months_traveled() != obj->distant_battle_travel_months) {
            return;
        }
    }
    if (obj->type == EMPIRE_OBJECT_ROMAN_ARMY) {
        if (!city_military_distant_battle_roman_army_is_traveling()) {
            return;
        }
        if (city_military_distant_battle_roman_months_traveled() != obj->distant_battle_travel_months) {
            return;
        }
    }
    if (obj->type == EMPIRE_OBJECT_ORNAMENT) {
        if (image_id < 0) {
            image_id = assets_lookup_image_id(ASSET_FIRST_ORNAMENT) - 1 - image_id;
        }
    }
    image_draw(image_id, data.x_draw_offset + x, data.y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE);
    const image *img = image_get(image_id);
    if (img->animation && img->animation->speed_id) {
        int new_animation = empire_object_update_animation(obj, image_id);
        image_draw(image_id + new_animation,
            data.x_draw_offset + x + img->animation->sprite_offset_x,
            data.y_draw_offset + y + img->animation->sprite_offset_y,
            COLOR_MASK_NONE, SCALE_NONE);
    }
    // Manually fix the Hagia Sophia
    if (obj->image_id == 8122) {
        image_id = assets_lookup_image_id(ASSET_HAGIA_SOPHIA_FIX);
        image_draw(image_id, data.x_draw_offset + x, data.y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE);
    }
}

static void draw_invasion_warning(int x, int y, int image_id)
{
    image_draw(image_id, data.x_draw_offset + x, data.y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE);
}

static void draw_map(void)
{
    // Recalculate inner bounds (same as draw_background)
    int map_clip_x_min = data.x_min + 16;
    int map_clip_y_min = data.y_min + 16;
    int map_clip_x_max = data.sidebar.x_min;  // Stop before sidebar starts
    int map_clip_y_max = data.y_max - 120;

    graphics_set_clip_rectangle(
        map_clip_x_min,
        map_clip_y_min,
        map_clip_x_max - map_clip_x_min,
        map_clip_y_max - map_clip_y_min);

    empire_set_viewport(map_clip_x_max - map_clip_x_min, map_clip_y_max - map_clip_y_min);

    data.x_draw_offset = map_clip_x_min;
    data.y_draw_offset = map_clip_y_min;
    empire_adjust_scroll(&data.x_draw_offset, &data.y_draw_offset);

    image_draw(empire_get_image_id(), data.x_draw_offset, data.y_draw_offset, COLOR_MASK_NONE, SCALE_NONE);

    empire_object_foreach(draw_empire_object);
    scenario_invasion_foreach_warning(draw_invasion_warning);

    graphics_reset_clip_rectangle();
}


static void draw_city_name(const empire_city *city)
{
    int image_base = image_group(GROUP_EMPIRE_PANELS);
    int draw_ornaments_outside = data.x_min - data.panel.x_min > 90;
    int base_x_min = draw_ornaments_outside ? data.panel.x_min : data.x_min;
    int base_x_max = draw_ornaments_outside ? data.panel.x_max : data.x_max;
    image_draw(image_base + 6, base_x_min + 2, data.y_max - 199, COLOR_MASK_NONE, SCALE_NONE);//left bird
    image_draw(image_base + 7, base_x_max - 84, data.y_max - 199, COLOR_MASK_NONE, SCALE_NONE);//right bird
    image_draw(image_base + 8, (data.x_min + data.x_max - 332) / 2, data.y_max - 181, COLOR_MASK_NONE, SCALE_NONE); //city badge big
    if (city) {
        int x_offset = (data.panel.x_min + data.panel.x_max - 332) / 2 + 64;
        int y_offset = data.y_max - 118;
        const uint8_t *city_name = empire_city_get_name(city);
        text_draw_centered(city_name, x_offset, y_offset, 268, FONT_LARGE_BLACK, 0);
    }
}

static void draw_panel_buttons(const empire_city *city)
{
    image_buttons_draw(data.panel.x_min + 20, data.y_max - 44, image_button_help, 1);
    image_buttons_draw(data.panel.x_max - 44, data.y_max - 44, image_button_return_to_city, 1);
    image_buttons_draw(data.panel.x_max - 44, data.y_max - 100, image_button_advisor, 1);
    image_buttons_draw(data.panel.x_min + 24, data.y_max - 100, image_button_show_prices, 1);
    if (city) {
        if (city->type == EMPIRE_CITY_TRADE && !city->is_open) {
            button_border_draw((data.panel.x_min + data.panel.x_max - 500) / 2 + 30, data.y_max - 49, 440,
                26, data.selected_button);
        }
    }
}
static void draw_sidebar_grid_box(void)
{
    graphics_set_clip_rectangle(
        data.sidebar.x_min,
        data.sidebar.y_min,
        data.sidebar.x_max - data.sidebar.x_min,
        data.sidebar.y_max - data.sidebar.y_min
    );
    

    grid_box_update_total_items(&sidebar_grid_box, sidebar_city_count);
    grid_box_draw(&sidebar_grid_box);

    graphics_reset_clip_rectangle();
}


static void draw_foreground(void)
{
    draw_map();

    const empire_city *city = 0;
    int selected_object = empire_selected_object();

    if (selected_object) {
        const empire_object *object = empire_object_get(selected_object - 1);
        if (object->type == EMPIRE_OBJECT_CITY) {
            data.selected_city = empire_city_get_for_object(object->id);
            city = empire_city_get(data.selected_city);
        }
    }

    draw_paneling();
    draw_sidebar_grid_box();  // grid_box uses usable_sidebar dimensions
    draw_city_name(city);
    draw_panel_buttons(city);
    draw_object_info();
}


static int is_outside_map(int x, int y)
{
    return (x < data.x_min + 16 || x >= data.x_max - 16 ||
        y < data.y_min + 16 || y >= data.y_max - 120);
}

static void determine_selected_object(const mouse *m)
{
    if (!m->left.went_up || data.finished_scroll || is_outside_map(m->x, m->y)) {
        data.finished_scroll = 0;
        return;
    }
    empire_select_object(m->x - data.x_min - 16, m->y - data.y_min - 16);
    window_invalidate();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    pixel_offset position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    grid_box_handle_input(&sidebar_grid_box, m, sidebar_grid_box.item_height / 2); // or 1/3  

    if (m->is_touch) {
        const touch *t = touch_get_earliest();
        if (!is_outside_map(t->current_point.x, t->current_point.y)) {
            if (t->has_started) {
                data.is_scrolling = 1;
                scroll_drag_start(1);
            }
        }
        if (t->has_ended) {
            data.is_scrolling = 0;
            data.finished_scroll = !touch_was_click(t);
            scroll_drag_end();
        }
    }
    data.focus_button_id = 0;
    data.focus_resource = 0;
    unsigned int button_id;
    image_buttons_handle_mouse(m, data.panel.x_min + 20, data.y_max - 44, image_button_help, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 1;
    }
    image_buttons_handle_mouse(m, data.panel.x_max - 44, data.y_max - 44, image_button_return_to_city, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 2;
    }
    image_buttons_handle_mouse(m, data.panel.x_max - 44, data.y_max - 100, image_button_advisor, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 3;
    }
    image_buttons_handle_mouse(m, data.panel.x_min + 24, data.y_max - 100, image_button_show_prices, 1, &button_id);
    if (button_id) {
        data.focus_button_id = 4;
    }
    button_id = 0;
    determine_selected_object(m);
    int selected_object = empire_selected_object();
    if (selected_object) {
        const empire_object *obj = empire_object_get(selected_object - 1);
        if (obj->type == EMPIRE_OBJECT_CITY) {
            data.selected_city = empire_city_get_for_object(selected_object - 1);
            const empire_city *city = empire_city_get(data.selected_city);
            if (city->type == EMPIRE_CITY_TRADE) {
                if (city->is_open) {
                    int x_offset = (data.panel.x_min + data.panel.x_max - 500) / 2;
                    int y_offset = data.y_max - 113;
                    int index_sell = 0;
                    int index_buy = 0;

                    // we only want to handle resource buttons that the selected city trades
                    for (int resource = RESOURCE_MIN; resource < RESOURCE_MAX; resource++) {
                        if (!resource_is_storable(resource)) {
                            continue;
                        }
                        data.focus_resource = resource;
                        if (city->sells_resource[resource]) {
                            generic_buttons_handle_mouse(m, x_offset + 120 + 124 * index_sell, y_offset + 31,
                                generic_button_trade_resource, 1, &button_id);
                            index_sell++;
                        } else if (city->buys_resource[resource]) {
                            generic_buttons_handle_mouse(m, x_offset + 120 + 124 * index_buy, y_offset + 62,
                                generic_button_trade_resource, 1, &button_id);
                            index_buy++;
                        }

                        if (button_id) {
                            data.focus_resource = resource;
                            // if we're focusing any button we can skip further checks
                            break;
                        } else {
                            data.focus_resource = 0;
                        }
                    }
                } else {
                    generic_buttons_handle_mouse(
                        m, (data.panel.x_min + data.panel.x_max - 500) / 2, data.y_max - 105,
                        generic_button_open_trade, 1, &data.selected_button);
                }
            }
        }
        // allow de-selection only for objects that are currently selected/drawn, otherwise exit empire map
        if (input_go_back_requested(m, h)) {
            switch (obj->type) {
                case EMPIRE_OBJECT_CITY:
                    empire_clear_selected_object();
                    window_invalidate();
                    break;
                case EMPIRE_OBJECT_ROMAN_ARMY:
                    if (city_military_distant_battle_roman_army_is_traveling()) {
                        if (city_military_distant_battle_roman_months_traveled() == obj->distant_battle_travel_months) {
                            empire_clear_selected_object();
                            window_invalidate();
                        }
                    }
                    break;
                case EMPIRE_OBJECT_ENEMY_ARMY:
                    if (city_military_months_until_distant_battle() > 0) {
                        if (city_military_distant_battle_enemy_months_traveled() == obj->distant_battle_travel_months) {
                            empire_clear_selected_object();
                            window_invalidate();
                        }
                    }
                    break;
                default:
                    window_city_show();
                    break;
            }
        }
    } else {
        if (m->right.went_down) {
            scroll_drag_start(0);
        }
        if (m->right.went_up) {
            int has_scrolled = scroll_drag_end();
            if (!has_scrolled && input_go_back_requested(m, h)) {
                window_city_show();
            }
        }
    }
}

static int is_mouse_hit(tooltip_context *c, int x, int y, int size)
{
    int mx = c->mouse_x;
    int my = c->mouse_y;
    return x <= mx && mx < x + size && y <= my && my < y + size;
}

static int get_tooltip_resource(tooltip_context *c)
{
    const empire_city *city = empire_city_get(data.selected_city);
    // we only want to check tooltips on our own closed cities.
    // open city resource tooltips are handled by their respective buttons directly
    if (city->type != EMPIRE_CITY_TRADE || city->is_open) {
        return 0;
    }
    int x_offset = (data.panel.x_min + data.panel.x_max - 500) / 2;
    int y_offset = data.y_max - 113;

    int item_offset = lang_text_get_width(47, 5, FONT_NORMAL_GREEN);
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (city->sells_resource[r] && resource_is_storable(r)) {
            if (is_mouse_hit(c, x_offset + 60 + item_offset, y_offset + 33, 26)) {
                return r;
            }
            int trade_max = trade_route_limit(city->route_id, r);
            item_offset += 32 + text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
        }
    }
    item_offset += lang_text_get_width(47, 4, FONT_NORMAL_GREEN);
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (city->buys_resource[r] && resource_is_storable(r)) {
            if (is_mouse_hit(c, x_offset + 110 + item_offset, y_offset + 33, 26)) {
                return r;
            }
            int trade_max = trade_route_limit(city->route_id, r);
            item_offset += 32 + text_get_number_width(trade_max, 0, 0, FONT_NORMAL_GREEN);
        }
    }

    return 0;
}

static void get_tooltip_trade_route_type(tooltip_context *c)
{
    int selected_object = empire_selected_object();
    if (!selected_object || empire_object_get(selected_object - 1)->type != EMPIRE_OBJECT_CITY) {
        return;
    }

    data.selected_city = empire_city_get_for_object(selected_object - 1);
    const empire_city *city = empire_city_get(data.selected_city);
    if (city->type != EMPIRE_CITY_TRADE || city->is_open) {
        return;
    }

    int x_offset = (data.panel.x_min + data.panel.x_max + 355) / 2;
    int y_offset = data.y_max - 41;
    int y_offset_max = y_offset + 22 - 2 * city->is_sea_trade;
    if (c->mouse_x >= x_offset && c->mouse_x < x_offset + 32 &&
        c->mouse_y >= y_offset && c->mouse_y < y_offset_max) {
        c->type = TOOLTIP_BUTTON;
        c->text_group = 44;
        c->text_id = 28 + city->is_sea_trade;
    }
}

static void get_tooltip(tooltip_context *c)
{
    int resource = data.focus_resource ? data.focus_resource : get_tooltip_resource(c);
    if (resource) {
        c->type = TOOLTIP_BUTTON;
        c->precomposed_text = resource_get_data(resource)->text;
    } else if (data.focus_button_id) {
        c->type = TOOLTIP_BUTTON;
        switch (data.focus_button_id) {
            case 1: c->text_id = 1; break;
            case 2: c->text_id = 2; break;
            case 3: c->text_id = 69; break;
            case 4:
                c->text_group = 54;
                c->text_id = 2;
                break;
        }
    } else {
        get_tooltip_trade_route_type(c);
    }
}

static void button_help(int param1, int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_EMPIRE_MAP, 0);
}

static void button_return_to_city(int param1, int param2)
{
    window_city_show();
}

static void button_advisor(int advisor, int param2)
{
    window_advisors_show_advisor(advisor);
}

static void button_show_prices(int param1, int param2)
{
    window_trade_prices_show(0, 0, screen_width(), screen_height());
}

static void button_show_resource_window(const generic_button *button)
{
    window_resource_settings_show(data.focus_resource);
}

static void confirmed_open_trade(int accepted, int checked)
{
    if (accepted) {
        empire_city_open_trade(data.selected_city, 1);
        building_menu_update();
        window_trade_opened_show(data.selected_city);
    }
}

static void button_open_trade(const generic_button *button2)
{
    window_popup_dialog_show(POPUP_DIALOG_OPEN_TRADE, confirmed_open_trade, 2);
}

void window_empire_show(void)
{
    
    init();
    setup_sidebar();

    window_type window = {
        WINDOW_EMPIRE,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    window_show(&window);
}


void window_empire_show_checked(void)
{
    tutorial_availability avail = tutorial_advisor_empire_availability();
    if (avail == AVAILABLE) {
        window_empire_show();
    } else {
        city_warning_show(avail == NOT_AVAILABLE ? WARNING_NOT_AVAILABLE : WARNING_NOT_AVAILABLE_YET, NEW_WARNING_SLOT);
    }
}
