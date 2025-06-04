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
#include <stdio.h>

#define WIDTH_BORDER 32
#define HEIGHT_BORDER 136
#define SIDEBAR_WIDTH_PERCENT 0.25f

#define TRADE_DOT_SPACING 10
#define MAX_SIDEBAR_CITIES 64
#define SIDEBAR_CONTENT_MARGIN (WIDTH_BORDER / 2)
#define MAX_TRADE_BUTTONS 256
#define MAX_TRADE_OPEN_BUTTONS 20

#define NO_POSITION ((unsigned int) -1)


typedef enum {
    TRADE_STYLE_MAIN_BAR,
    TRADE_STYLE_SIDEBAR
} trade_style_variant;

typedef struct {
    int x;
    int y;
} px_point;

typedef struct {
    int sidebar_item_id; // number on the list
    int empire_object_id; // empire object id of the city
    int city_id; // to retire in favour of the city_item// index in array of cities
    //empire_city city_item;
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
typedef struct {
    // Region bounds
    int row_x_min;               // Starting x-coordinate of the trade row
    int row_y_min;               // Starting y-coordinate of the trade row
    int row_width;               // Total width of the row (x_max = x_min + width)
    int row_height;              // Total height of the row (optional: for future clipping)

    // Layout adjustments
    int y_offset_icon;       // Vertical offset to nudge icons up/down
    int y_offset_text;       // Vertical offset for text baseline alignment

    int label_indent;        // Horizontal offset where the first icon is placed (based on "Buys"/"Sells" label width)

    // Segment spacing
    int seg_space_0;         // Space before resource icon
    int seg_space_1;         // Space between icon and first number
    int seg_space_2;         // Space between first number and "of" text
    int seg_space_3;         // Space between "of" text and second number
    int seg_space_4;         // Space after second number

    int segment_width_adjust; // Extra width added/subtracted to total segment width
} trade_row_style;

typedef struct {
    // Region bounds
    int button_x_min;               // Starting x-coordinate of the button
    int button_y_min;               // Starting y-coordinate of the tbutton
    int button_width;               // Total width of the row (x_max = x_min + width)
    int button_height;              // Total height of the row (optional: for future clipping)

    // Layout adjustments
    int y_offset_icon;       // Vertical offset to nudge icons up/down
    int y_offset_text;       // Vertical offset for text baseline alignment

    // Segment spacing
    int seg_space_0;         // Space before border start
    int seg_space_1;         // Space between border start and cost string
    int seg_space_2;         // Space between cost and currency
    int seg_space_3;         // Space between currency and text
    int seg_space_4;         // Space between text and icon
    int seg_space_5;         // Space after icon

    int segment_width_adjust; // Extra width added/subtracted to total segment width
} open_trade_button_style;


typedef struct {
    int row_x_min;
    int row_y_min;
    int row_width;
    int seg_space_0;
    int seg_space_1;
    int seg_space_4;
    int label_indent;
    int icon_width;
} closed_trade_row_style;


typedef struct {
    int x, y, width, height;
    resource_type res;
    int do_highlight;
} trade_resource_button;

typedef struct {
    int x, y, width, height;
    int route_id;
    int do_highlight;
} trade_open_button;

typedef enum {
    TRADE_ICON_NONE = -1,
    TRADE_ICON_LAND = 0,
    TRADE_ICON_SEA = 1
} trade_icon_type;

static void button_help(int param1, int param2);
static void button_return_to_city(int param1, int param2);
static void button_advisor(int advisor, int param2);
static void button_show_prices(int param1, int param2);
static void image_draw_scaled_centered(int image_id, int x, int y, color_t color, int draw_scale_percent); //(image_id, data.x_draw_offset + x, data.y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE)
//static void button_open_trade(const generic_button *button);

//static void setup_sidebar_dimensions(void); //replaced by setup_sidebar
//static void draw_closed_trade_rows(const empire_city *city, int y_offset, int max_draw_width, int safe_margin_left, const closed_trade_row_style *style); //different styles used instead
static void button_open_trade_by_route(int route_id);
static void button_show_resource_window(const trade_resource_button *btn);
static int measure_trade_row_width(const empire_city *city, int is_sell, const trade_row_style *style);
//static int measure_open_trade_button_width(const empire_city *city, trade_icon_type icon_type, const open_trade_button_style *style); //shouldnt be necessary

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

// static generic_button generic_button_trade_resource[] = { //will be superseeded by layout_trade_resource_row
//     {0, 0, 101, 27, button_show_resource_window},
// };

// static generic_button generic_button_open_trade[] = {
//     {30, 56, 440, 26, button_open_trade}
// };
static trade_row_style get_trade_row_style(const empire_city *city, int is_sell, int max_draw_width, trade_style_variant variant);
static open_trade_button_style get_open_trade_button_style(int x, int y, trade_style_variant variant);

static trade_open_button trade_open_buttons[MAX_TRADE_OPEN_BUTTONS];
static int trade_open_button_count = 0;

static trade_resource_button trade_buttons[MAX_TRADE_BUTTONS];
static int trade_button_count = 0;


void register_trade_button(int x, int y, int width, int height, resource_type r, int highlight) {
    if (trade_button_count >= MAX_TRADE_BUTTONS) return;
    trade_buttons[trade_button_count++] = (trade_resource_button){ x, y, width, height, r, highlight };
}

void register_open_trade_button(int x, int y, int width, int height, int route_id, int highlight) {
    if (trade_open_button_count >= MAX_TRADE_OPEN_BUTTONS) return;
    trade_open_buttons[trade_open_button_count++] = (trade_open_button){ x, y, width, height, route_id, highlight };
}



static struct {
    unsigned int selected_button;
    int selected_city;    // this is the deciding factor on what is the FOCUS of the entire empire window and everything should be linked to it
                          // the selected city is the index in the array of cities, not the empire object id    
                          // when selected_object is used, it comes from empire_selected_object() and is the empire object id
                          // when getting city index for the empire object id, use -1 on the empire object id, which is selected_object
                          // so when searching for city index, (selected_object - 1) or (empire_object_id - 1) is used
                          // use empire_city_get_for_object(selected_object - 1) to get the city index from the empire object id
                          // use empire_city_get(city_id)->empire_object_id to get the empire object id from the city index

    int selected_trade_route; // this is the trade route id, stored in here after button is clicked
    int x_min, x_max, y_min, y_max;
    int x_draw_offset, y_draw_offset;
    unsigned int focus_button_id;
    int is_scrolling;
    int finished_scroll;
    int hovered_object;
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
    data.selected_button = NO_POSITION; // no button selected
    int selected_object = empire_selected_object();
    if (selected_object) {
        data.selected_city = empire_city_get_for_object(selected_object - 1); //data.selected_city is  array index of the empire object from the array of cities 
    } else {
        data.selected_city = 0;
    }
    data.focus_button_id = 0;
    
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

static open_trade_button_style get_open_trade_button_style(int x, int y, trade_style_variant variant)
{
    //{30, 56, 440, 26, button_open_trade} original values for reference
    int is_sidebar = (variant == TRADE_STYLE_SIDEBAR);

    open_trade_button_style style = {
        //(data.panel.x_min + data.panel.x_max - 500) / 2
        //.button_x_min =  (is_sidebar ? x + 15 : -146), 
        .button_x_min =  (is_sidebar ? x + 15 : (data.panel.x_min + data.panel.x_max - 500) / 2)+30, //15px offset somewhere got lost, * 2 for 30
        .button_y_min = y+ (is_sidebar ? 0 : -9),
        .button_width = is_sidebar ? 300 : 440,  // restored fixed widths // sidebar: main bar

        .button_height = 26,

        .y_offset_icon = is_sidebar ? 0 : 2,     
        .y_offset_text = is_sidebar ? 10: 10,

        .seg_space_0 = is_sidebar ? 10 : 60,     // should just be centered from full width - will have to adjust later
        .seg_space_1 = 0,
        .seg_space_2 = 4,
        .seg_space_3 = is_sidebar ? 10 : 10,
        .seg_space_4 = 4,
        .seg_space_5 = is_sidebar ? 4 : 4,

        .segment_width_adjust = 0
    };

    return style;
}



void draw_open_trade_button(const empire_city *city, const open_trade_button_style *style, trade_icon_type icon_type)
{
    int cost = city->cost_to_open;
    int x = style->button_x_min;
    int y = style->button_y_min;

    // Starting offset
    int cursor_x = x + style->seg_space_0;

    // Draw cost
    int cost_width = lang_text_draw_amount(8, 0, cost, cursor_x + style->seg_space_1, y + style->y_offset_text, FONT_NORMAL_GREEN);
    cursor_x += style->seg_space_1 + cost_width;

    // Draw label
    cursor_x += style->seg_space_2;
    int label_width = lang_text_draw(47, 6, cursor_x, y + style->y_offset_text, FONT_NORMAL_GREEN);
    cursor_x += label_width;

    // Optional icon
    if (icon_type != TRADE_ICON_NONE) {
        cursor_x += style->seg_space_3;
        int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - icon_type;
        image_draw(image_id, cursor_x + style->seg_space_4, y + style->y_offset_icon + 2 * icon_type, COLOR_MASK_NONE, SCALE_NONE);
        cursor_x += style->seg_space_4;
    }

    cursor_x += style->seg_space_5;

    // Register hitbox using the helper function
    button_border_draw(x, y, style->button_width,style->button_height, 0); //0 for non-focused state. focusing handled elsewhere
    register_open_trade_button(x, y, style->button_width, style->button_height, city->route_id, 0);

}


static trade_row_style get_trade_row_style(const empire_city *city, int is_sell, int max_draw_width, trade_style_variant variant) {
    // === Initial struct ===
    trade_row_style style = {
        .row_x_min = 0,
        .row_y_min = 0,
        .row_width = max_draw_width,
        .row_height = 0,
        .y_offset_icon = -9,
        .y_offset_text = 0,
    };

    int is_main_bar = (variant == TRADE_STYLE_MAIN_BAR);
    int count_sells = count_trade_resources(city, 1);
    int count_buys  = count_trade_resources(city, 0);

    // === Determine compactness ===
    int compact_sells = is_main_bar ? (count_sells > 5) : (count_sells > 2);
    int compact_buys  = is_main_bar ? (count_buys  > 5) : (count_buys  > 2);
    int any_compact   = compact_sells || compact_buys;

    int is_compact = is_main_bar
        ? (is_sell ? compact_sells : compact_buys)
        : (city->is_open ? (is_sell ? compact_sells : compact_buys) : any_compact);

    // === Label indent ===
    if (!city->is_open) {
        style.label_indent = lang_text_get_width(47, 10 + is_sell, FONT_NORMAL_GREEN) + (is_compact ? 10 : 20);
    } else {
        int width_sells = lang_text_get_width(47, 10, FONT_NORMAL_GREEN);
        int width_buys  = lang_text_get_width(47,  9, FONT_NORMAL_GREEN);
        int max_label_width = (width_sells > width_buys) ? width_sells : width_buys;
        style.label_indent = max_label_width + (any_compact ? 10 : 20);
    }

    // === Segment layout ===
    if (is_main_bar) {
        style.seg_space_0 = 0;
        style.seg_space_1 = city->is_open ? (is_compact ? 2 : 8)  : (is_compact ? 1 : 6);
        style.seg_space_2 = city->is_open ? (is_compact ? -6 : -1): (is_compact ? 1 : 3);
        style.seg_space_3 = city->is_open ? (is_compact ? -6 : -1): (is_compact ? 1 : 3);
        style.seg_space_4 = city->is_open ? (is_compact ? 0 : 14) : (is_compact ? 0 : 10);
        style.segment_width_adjust = city->is_open ? (is_compact ? -6 : 0) : (is_compact ? -4 : -2);
    } else {
        style.seg_space_0 = 0;
        style.seg_space_1 = city->is_open ? (is_compact ? -3 : 6)  : (is_compact ? -2 : 4);
        style.seg_space_2 = city->is_open ? (is_compact ? -4 : -1): (is_compact ? 2 : 5);
        style.seg_space_3 = city->is_open ? (is_compact ? -4 : -1): (is_compact ? 2 : 5);
        style.seg_space_4 = city->is_open ? (is_compact ? 3 : 10) : (is_compact ? 2 : 7);
        style.segment_width_adjust = city->is_open ? (is_compact ? -10 : -4) : (is_compact ? -8 : -2);
    }

    return style;
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

void draw_trade_resource(resource_type r, int trade_max, int x, int y) {
    graphics_draw_inset_rect(x-1, y-1, 26, 26, COLOR_INSET_DARK, COLOR_INSET_LIGHT);
    image_draw(resource_get_data(r)->image.empire, x, y, COLOR_MASK_NONE, SCALE_NONE);
    window_empire_draw_resource_shields(trade_max, x, y);
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

static int measure_trade_row_width(const empire_city *city, int is_sell, const trade_row_style *style) {
    const int ICON_WIDTH = 26;
    int width = 0;

    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;
        if ((is_sell && !city->sells_resource[r]) || (!is_sell && !city->buys_resource[r])) continue;

        int w_max = text_draw_number(trade_route_limit(city->route_id, r), '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
        int segment_width;

        if (city->is_open) {
            // Also need width of current amount and "of" label
            int w_now = text_draw_number(trade_route_traded(city->route_id, r), '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
            int w_of = lang_text_get_width(47, 11, FONT_NORMAL_GREEN);

            segment_width =
                style->seg_space_0 + ICON_WIDTH +
                style->seg_space_1 + w_now +
                style->seg_space_2 + w_of +
                style->seg_space_3 + w_max +
                style->seg_space_4 +
                style->segment_width_adjust;
        } else {
            segment_width =
                style->seg_space_0 + ICON_WIDTH +
                style->seg_space_1 + w_max +
                style->seg_space_4 +
                style->segment_width_adjust;
        }

        width += segment_width;
    }

    if ((is_sell && count_trade_resources(city, 1)) || (!is_sell && count_trade_resources(city, 0))) {
        width += style->label_indent;
    }

    return width;
}


static int draw_trade_row(const empire_city *city, int is_sell, int x_offset, int y_offset, const trade_row_style *style) {
    const int ICON_WIDTH = 26;
    const int RESOURCE_HEIGHT = 26;

    int label_id;
    if (city->is_open) {
        label_id = is_sell ? 10 : 9;
    } else {
        label_id = is_sell ? 5 : 4;
    }


    // Draw "Sells:" or "Buys:" label
    lang_text_draw(47, label_id, style->row_x_min, style->row_y_min, FONT_NORMAL_GREEN);

    int x_cursor = style->row_x_min + style->label_indent;
    

    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;
        if ((is_sell && !city->sells_resource[r]) || (!is_sell && !city->buys_resource[r])) continue;

        int trade_max = trade_route_limit(city->route_id, r);
        int trade_now = trade_route_traded(city->route_id, r);
        int icon_y = style->row_y_min + style->y_offset_icon;
        int y_text = style->row_y_min + style->y_offset_text;

        int segment_width, text_x;

        if (city->is_open) {
            // Calculate widths
            int w_now = text_draw_number(trade_now, '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
            int w_max = text_draw_number(trade_max, '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
            int w_of = lang_text_get_width(47, 11, FONT_NORMAL_GREEN);

            segment_width =
                style->seg_space_0 + ICON_WIDTH +
                style->seg_space_1 + w_now +
                style->seg_space_2 + w_of +
                style->seg_space_3 + w_max +
                style->seg_space_4 +
                style->segment_width_adjust;
        } else {
            int w_max = text_draw_number(trade_max, '@', "", 0, 0, FONT_NORMAL_GREEN, 1);
            segment_width =
                style->seg_space_0 + ICON_WIDTH +
                style->seg_space_1 + w_max +
                style->seg_space_4 +
                style->segment_width_adjust;
        }

        // Clip if segment would overflow
        if (x_cursor + segment_width > style->row_x_min + style->row_width) {
            text_draw((const uint8_t *)"(...)", x_cursor + 4, style->row_y_min, FONT_NORMAL_GREEN, 0);
            break;
        }

        // Draw resource icon
        draw_trade_resource(r, trade_max, x_cursor + style->seg_space_0, icon_y);

        // Draw numeric info
        text_x = x_cursor + style->seg_space_0 + ICON_WIDTH + style->seg_space_1;

        if (city->is_open) {
            int w_now = text_draw_number(trade_now, '@', "", text_x, y_text, FONT_NORMAL_GREEN, 0);
            int of_x = text_x + w_now + style->seg_space_2;
            int w_of = lang_text_draw(47, 11, of_x, y_text, FONT_NORMAL_GREEN);
            int max_x = of_x + w_of + style->seg_space_3;
            text_draw_number(trade_max, '@', "", max_x, y_text, FONT_NORMAL_GREEN, 0);
        } else {
            text_draw_number(trade_max, '@', "", text_x, y_text, FONT_NORMAL_GREEN, 0);
        }

        // Register hitbox
        register_trade_button(
            x_cursor,
            icon_y,
            segment_width,
            RESOURCE_HEIGHT,
            r,
            city->is_open
        );

        // Advance
        x_cursor += segment_width;
    }

    return x_cursor; // Final drawing position
}


static void draw_trade_city_info(const empire_object *object, const empire_city *city) {
    int y_offset = data.y_max - 113;
    const int safe_margin_left = data.panel.x_min + 50;
    const int safe_margin_right = data.panel.x_max - 50;
    int max_draw_width = safe_margin_right - safe_margin_left;

    //int num_sells = count_trade_resources(city, 1);
    //int num_buys = count_trade_resources(city, 0);
    //int max_resources = (num_sells > num_buys) ? num_sells : num_buys;
    //int is_compact = max_resources > 5;
    //int label_width_1 = lang_text_get_width(47, 10, FONT_NORMAL_GREEN);
    //int label_width_2 = lang_text_get_width(47,  9, FONT_NORMAL_GREEN);
    trade_row_style style_sells = get_trade_row_style(city, 1, max_draw_width, TRADE_STYLE_MAIN_BAR);
    trade_row_style style_buys  = get_trade_row_style(city, 0, max_draw_width, TRADE_STYLE_MAIN_BAR);
    // === OPEN CITY ===
    if (city->is_open) {
        int width_sells = measure_trade_row_width(city, 1, &style_sells);
        int width_buys  = measure_trade_row_width(city, 0, &style_buys);
        int total_width = (width_sells > width_buys) ? width_sells : width_buys;

        if (total_width > max_draw_width)
            total_width = max_draw_width;

        int x_offset = safe_margin_left + (max_draw_width - total_width) / 2;

        style_sells.row_x_min = x_offset;
        style_sells.row_y_min = y_offset + 40;
        style_buys.row_x_min  = x_offset;
        style_buys.row_y_min  = y_offset + 71;

        draw_trade_row(city, 1, 0, 0, &style_sells);
        draw_trade_row(city, 0, 0, 0, &style_buys);


    // === CLOSED CITY ===
    }     else {
        int width_sells = measure_trade_row_width(city, 1, &style_sells);
        int width_buys  = measure_trade_row_width(city, 0, &style_buys);
        int total_width = width_sells + width_buys + 15; //that 15 might have to go

        if (total_width > max_draw_width)
            total_width = max_draw_width;

        int x_base = safe_margin_left + (max_draw_width - total_width) / 2;

        style_sells.row_x_min = x_base;
        style_sells.row_y_min = y_offset + 42;

        style_buys.row_x_min = x_base + width_sells + 15;
        style_buys.row_y_min = y_offset + 42;

        draw_trade_row(city, 1, 0, 0, &style_sells);
        draw_trade_row(city, 0, 0, 0, &style_buys);

        // Draw cost + type icon
        open_trade_button_style style = get_open_trade_button_style(x_base, y_offset+73, TRADE_STYLE_MAIN_BAR);
        draw_open_trade_button(city, &style, (trade_icon_type)(city->is_sea_trade));

        // int x_offset_cost = (data.x_min + data.x_max - 500) / 2;
        // int cost_index = lang_text_draw_amount(8, 0, city->cost_to_open, x_offset_cost + 40, y_offset + 73, FONT_NORMAL_GREEN);
        // lang_text_draw(47, 6, x_offset_cost + cost_index + 40, y_offset + 73, FONT_NORMAL_GREEN);
        // int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
        // image_draw(image_id, x_offset_cost + 430, y_offset + 65 + 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);

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
    trade_row_style style_sells = get_trade_row_style(city, 1, item->width, TRADE_STYLE_SIDEBAR);
    trade_row_style style_buys  = get_trade_row_style(city, 0, item->width, TRADE_STYLE_SIDEBAR);
    // draw background + name + badge
    inner_panel_draw(x_offset, y_offset, x_blocks, y_blocks);
    if (item->is_focused) {
        // Shade the entire panel area with a light shade (value 0-7)
        // Lower numbers are darker, higher numbers are lighter
        graphics_shade_rect(
            item->x, 
            item->y,
            item->width - BLOCK_SIZE/2,
            item->height - BLOCK_SIZE/2,
            2  // 0-7
        );
    }
    if (entry->city_id == data.selected_city) {
        button_border_draw(item->x,item->y, item->width - BLOCK_SIZE/2, item->height - BLOCK_SIZE/2, 1);
    }
    int badge_id = assets_get_image_id("UI", "Empire_sidebar_city_badge");
    image_draw(badge_id, x_offset + 5, y_offset + 5, COLOR_MASK_NONE, SCALE_NONE);
    text_draw(name, x_offset + BLOCK_SIZE, y_offset + 9, FONT_LARGE_BLACK, 0);

    // draw trade icon
    int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
    image_draw(image_id, x_offset + 250, y_offset + 9+ 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);

    // Move y_offset down for trade info rows
    y_offset += 44;
    //will fix later
    if (city->is_open) {
            // For Sells
            style_sells.row_x_min = x_offset + 10;
            style_sells.row_y_min = y_offset + 6;
            draw_trade_row(city, 1, 0, 0, &style_sells);

            // For Buys
            style_buys.row_x_min = x_offset + 10;
            style_buys.row_y_min = y_offset + 6 + 26; //26 icon height
            draw_trade_row(city, 0, 0, 0, &style_buys);

    } else {
        // --- Closed city ---
        style_sells.row_x_min = x_offset + 10;
        style_sells.row_y_min = y_offset + 6;
        int x_cursor = draw_trade_row(city, 1, 0, 0, &style_sells);
        
        // Now set up buys correctly, to appear right after sells
        style_buys.row_x_min = x_cursor + 15;  // 15px spacing like main bar
        style_buys.row_y_min = style_sells.row_y_min;
        
        draw_trade_row(city, 0, 0, 0, &style_buys);
        

        y_offset += 35;
        open_trade_button_style style = get_open_trade_button_style(x_offset,y_offset, TRADE_STYLE_SIDEBAR);
        draw_open_trade_button(city, &style, TRADE_ICON_NONE);

        //draw_open_trade_button(city, x_offset + 10, y_offset, item->width - 20, TRADE_ICON_NONE);
        // int index = lang_text_draw_amount(8, 0, city->cost_to_open, x_offset + 10, y_offset, FONT_NORMAL_GREEN);
        // lang_text_draw(47, 6, x_offset + 10 + index, y_offset, FONT_NORMAL_GREEN);
        // int icon = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - city->is_sea_trade;
        // image_draw(icon, x_offset + 250, y_offset + 2 * city->is_sea_trade, COLOR_MASK_NONE, SCALE_NONE);
    }
}

static void on_sidebar_city_click(const grid_box_item *item)
{
    // Priority: resource buttons take precedence
    for (int i = 0; i < trade_button_count; i++) {
        const trade_resource_button *btn = &trade_buttons[i];
        if (item->mouse.x >= btn->x && item->mouse.x < btn->x + btn->width &&
            item->mouse.y >= btn->y && item->mouse.y < btn->y + btn->height) {
            return;  // Don't process background click
        }
    }

    // Get actual index
    int index = item->index;
    if (index < 0 || index >= sidebar_city_count) return;
    sidebar_city_entry *entry = &sidebar_cities[index];
    empire_city *city = empire_city_get(entry->city_id);


    if (!city) return;

    // Start log block
    // log_info("========== Sidebar City Click =========: ", NULL, item->index);
    // log_info("item index:", NULL, item->index);
    // log_info("ENTRY - its the sidebar_city_entry ->city_id", NULL, entry->city_id);
    // log_info("name fetched from entry->city_id=", empire_city_get_name(empire_city_get(entry->city_id)),0);
    // log_info("name fetched from city", empire_city_get_name(city),0);
    // log_info("city->empire_object_id", NULL, city->empire_object_id);
    // City info at that index


    // Selection logic
    //log_info("Applying data.selected_city= ", NULL, entry->city_id);
    //log_info("city->empire_object_id= ", NULL, city->empire_object_id);

    data.selected_city = entry->city_id; 
    empire_select_object_by_id(city->empire_object_id);


    // New state
    //log_info("Selection Complete. New data.selected_city", NULL, data.selected_city);
  
    //log_info("New empire_selected_object()", NULL, empire_selected_object());
    grid_box_request_refresh(&sidebar_grid_box); //refresh sidebar grid
    window_invalidate(); // Force UI redraw
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
    for (int i = 1; i < empire_city_get_array_size(); i++) { //needs to start at 1 to skip the "no city" entry?
        empire_city *city = empire_city_get(i);
        if (!city->in_use || city->type != EMPIRE_CITY_TRADE) continue;

        if (sidebar_city_count >= MAX_SIDEBAR_CITIES) break;

        sidebar_city_entry *entry = &sidebar_cities[sidebar_city_count];
        entry->sidebar_item_id = sidebar_city_count; //this is the index in the sidebar_cities array
        entry->city_id = i; //story city id which is the index in the empire city array
        entry->empire_object_id = city->empire_object_id; //this is the empire object id, which is the index in the empire object array
        entry->x = data.sidebar.x_min;
        entry->y = y;
        entry->width = sidebar_width;
        entry->height = 120;

        y += entry->height;
        sidebar_city_count++;
    }

    // Setup grid box
    const int sidebar_margin_left = 25;
    //const int sidebar_margin_right = 12;
    const int sidebar_margin_top = 6;
    const int sidebar_margin_bottom = 6;

    sidebar_grid_box.x = data.sidebar.x_min + sidebar_margin_left;
    sidebar_grid_box.y = data.sidebar.y_min + sidebar_margin_top;
    sidebar_grid_box.width = data.sidebar.x_max - data.sidebar.x_min - sidebar_margin_left; // i removed '-sidebar_margin_right'; for scrollbar i think but this jank needs refactor anyway
    sidebar_grid_box.height = data.sidebar.y_max - data.sidebar.y_min - sidebar_margin_top - sidebar_margin_bottom;
    sidebar_grid_box.item_height = 120;
    sidebar_grid_box.num_columns = 1;
    sidebar_grid_box.item_margin.horizontal = 0;
    sidebar_grid_box.item_margin.vertical = 0;
    sidebar_grid_box.draw_inner_panel = 0;
    sidebar_grid_box.extend_to_hidden_scrollbar = 1;
    sidebar_grid_box.decorate_scrollbar = 1;
    sidebar_grid_box.total_items = sidebar_city_count;
    sidebar_grid_box.draw_item = draw_sidebar_city_item;
    sidebar_grid_box.on_click = on_sidebar_city_click;
    sidebar_grid_box.handle_tooltip = NULL;

    grid_box_init(&sidebar_grid_box, sidebar_city_count);
}



static void draw_city_info(const empire_object *object)
{
    int x_offset = (data.x_min + data.x_max - 240) / 2;
    int y_offset = data.y_max - 88;

    const empire_city *city = empire_city_get(data.selected_city);
    //const empire_object *obj = empire_object_get(selected_object - 1);
    int image_id;
    // if (city->type == EMPIRE_CITY_DISTANT_FOREIGN ||
    //     city->type == EMPIRE_CITY_FUTURE_ROMAN) {
    //     image_id = image_group(GROUP_EMPIRE_FOREIGN_CITY);
    // } else if (city->type == EMPIRE_CITY_TRADE) {
    //     // Fix cases where empire map still gives a blue flag for new trade cities
    //     // (e.g. Massilia in campaign Lugdunum)
    //     image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    // }
    switch (city->type) {
        case EMPIRE_CITY_DISTANT_ROMAN:
            lang_text_draw_centered(47, 12, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
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
            image_id = image_group(GROUP_EMPIRE_CITY);
            break;
        case EMPIRE_CITY_OURS:
            lang_text_draw_centered(47, 1, x_offset, y_offset + 42, 240, FONT_NORMAL_GREEN);
            break;
        case EMPIRE_CITY_TRADE:
            draw_trade_city_info(object, city);
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
    //int usable_map_height = map_draw_y_max - map_draw_y_min;

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
    if ((data.hovered_object == obj->id+1) && obj->type ==  EMPIRE_OBJECT_CITY){       
       

        image_draw_scaled_centered(image_id, data.x_draw_offset + x,data.y_draw_offset + y, COLOR_MASK_NONE, 120 );
    }else{
        image_draw(image_id, data.x_draw_offset + x, data.y_draw_offset + y, COLOR_MASK_NONE, SCALE_NONE);
    }
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

static void image_draw_scaled_centered(int image_id, int x, int y, color_t color, int draw_scale_percent) {
    float obj_draw_scale = 100.0f/draw_scale_percent;
    const image *img = image_get(image_id);

    float scaled_x = (((x) + img->width / 2.0f) - (img->width / obj_draw_scale) / 2.0f) * obj_draw_scale;
    float scaled_y = (((y) + img->height / 2.0f) - (img->height / obj_draw_scale) / 2.0f) * obj_draw_scale;

    image_draw(image_id, scaled_x, scaled_y, color, obj_draw_scale);
}



static void draw_empire_object_scaled_centered(const empire_object *obj, int draw_scale_percent){
    // to definitely distinguish it from how scale float is used in draw and render functions
    // this function uses scale as an int of percent to draw the item at,
    // so that 100 is regular scale, 200 is 2x the size
    // the scaling will affect the starting x and y, but keep the geometrical center of the object

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
    const image *img = image_get(image_id);
    float obj_draw_scale = 100 / draw_scale_percent; // convert given int % scale into float for rendering
            
    // Ensure float division
    float scaled_w_center = ((float)img->width / obj_draw_scale) / 2.0f;
    float scaled_h_center= ((float)img->height / obj_draw_scale) / 2.0f;

    //int x_offset = data.x_draw_offset;
    //int y_offset = data.y_draw_offset;

    int default_x = data.x_draw_offset + x;
    int default_y = data.y_draw_offset + y;

    float scaled_x = default_x - scaled_w_center;
    float scaled_y = default_y - scaled_h_center;
    image_draw(
        image_id,
        (((data.x_draw_offset + x) + img->width/2) - (img->width/obj_draw_scale) /2)*obj_draw_scale,
        (((data.y_draw_offset + y) + img->height/2) - (img->height/obj_draw_scale) /2)*obj_draw_scale,
        COLOR_MASK_NONE,
        obj_draw_scale
    ); //final coordinates are / obj_draw_scale , so the entire calculation needs to be * obj_draw_scale to get the correct position I believe
               

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

static void draw_panel_buttons(void)
{
    image_buttons_draw(data.panel.x_min + 20, data.y_max - 44, image_button_help, 1);
    image_buttons_draw(data.panel.x_max - 44, data.y_max - 44, image_button_return_to_city, 1);
    image_buttons_draw(data.panel.x_max - 44, data.y_max - 100, image_button_advisor, 1);
    image_buttons_draw(data.panel.x_min + 24, data.y_max - 100, image_button_show_prices, 1);
    if (data.selected_button != NO_POSITION) {
        const trade_open_button *btn = &trade_open_buttons[data.selected_button];
        button_border_draw(btn->x - 1, btn->y - 1, btn->width + 2, btn->height + 2, 1);
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
    

    //grid_box_update_total_items(&sidebar_grid_box, sidebar_city_count);
    grid_box_draw(&sidebar_grid_box);

    graphics_reset_clip_rectangle();
}

// static void draw_open_trade_button_highlight(const mouse *m)
// {
//     int mx = m->x;
//     int my = m->y;

//     for (int i = 0; i < trade_open_button_count; ++i) {
//         const trade_open_button *btn = &trade_open_buttons[i];

//         if (mx >= btn->x && mx < btn->x + btn->width &&
//             my >= btn->y && my < btn->y + btn->height) {
//             button_border_draw(btn->x - 1, btn->y - 1, btn->width + 2, btn->height + 2, 1);
//             break;
//         }
//     }
// }


static void draw_trade_button_highlights(void) {
    for (int i = 0; i < trade_button_count; ++i) {
        const trade_resource_button *btn = &trade_buttons[i];
        if (btn->do_highlight && data.focus_resource == btn->res) {
            button_border_draw(btn->x - 1, btn->y - 1, btn->width + 2, btn->height + 2, 1);
        }
    }
}

static void draw_foreground(void)
{
    draw_map();
    trade_button_count = 0;
    trade_open_button_count = 0;
    const empire_city *city = 0;
    int selected_object = empire_selected_object();

    if (selected_object) {
        const empire_object *object = empire_object_get(selected_object - 1); // it should be -1 , thats correct
        if (object->type == EMPIRE_OBJECT_CITY) { 
            data.selected_city = empire_city_get_for_object(object->id);
            city = empire_city_get(data.selected_city);
        }
    } else {

    }
    draw_paneling();
    draw_sidebar_grid_box();  // grid_box uses usable_sidebar dimensions
    grid_box_request_refresh(&sidebar_grid_box);
    draw_city_name(city);
    draw_object_info();
    draw_panel_buttons();
    draw_trade_button_highlights();
}



static int is_outside_map(int x, int y)
{
    return (x < data.x_min + 16 || x >= data.sidebar.x_min  ||
        y < data.y_min + 16 || y >= data.y_max - 120);
}

static void determine_selected_object(const mouse *m)
{
    if (is_outside_map(m->x, m->y)) { //maybe run this before first if instead
        // Check if it's inside the sidebar instead
        if (m->x >= data.sidebar.x_min && m->x < data.sidebar.x_max &&
            m->y >= data.sidebar.y_min && m->y < data.sidebar.y_max) {
            return; // Let sidebar handle it 
        }
    
        // Not in map or sidebar — ignore
        data.finished_scroll = 0;
        return;
    }
    if (!m->left.went_up || data.finished_scroll) {
            //return; // instead of returning, determine hovered object
            int hovered_obj_id = empire_get_hovered_object(m->x - data.x_min - 16, m->y - data.y_min - 16);
            empire_object *obj = empire_object_get(hovered_obj_id);
            data.hovered_object = hovered_obj_id;
            log_info("hovered object:",NULL,data.hovered_object);
            return;
    }else{
        // this is map click - proceed
        log_info("map click detected - x:", NULL, m->x);
        log_info("map click detected - y:", NULL, m->y);
        empire_select_object(m->x - data.x_min - 16, m->y - data.y_min - 16);
        log_info("selected object:",NULL,empire_selected_object());
        window_invalidate();
    }
    
    


}

static void handle_input(const mouse *m, const hotkeys *h)
{
    pixel_offset position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    //int grid_box_result = grid_box_handle_input(&sidebar_grid_box, m, 1); 
    grid_box_handle_input(&sidebar_grid_box, m, 1); 

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
    data.hovered_object = 0;
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
    
    for (int i = 0; i < trade_button_count; i++) { //moved out of the 'selected object is empire city' to allow all buttons in the sidebar to work
        const trade_resource_button *btn = &trade_buttons[i];
        if (m->x >= btn->x && m->x < btn->x + btn->width &&
            m->y >= btn->y && m->y < btn->y + btn->height) {
            data.focus_resource = btn->res;
            if (m->left.went_up && btn->do_highlight) {
                button_show_resource_window(btn);
            }
          break;
        }
        
    }
    data.selected_button = NO_POSITION;
    for (int i = 0; i < trade_open_button_count; i++) {
        const trade_open_button *btn = &trade_open_buttons[i];
    
        if (m->x >= btn->x && m->x < btn->x + btn->width &&
            m->y >= btn->y && m->y < btn->y + btn->height) {
            data.selected_button = i;
            if (m->left.went_up) {
                button_open_trade_by_route(btn->route_id);  // <-- Trigger popup
            }
    
            break;  // Only process one button at a time
        }
    }
    
    if (selected_object) {
        //this block should no longer be necessary as buttons for sidebar and bottom panel are handled together by appropiate functions
        /*if (obj->type == EMPIRE_OBJECT_CITY) {
            data.selected_city = empire_city_get_for_object(selected_object - 1);
            const empire_city *city = empire_city_get(data.selected_city);
            if (city->type == EMPIRE_CITY_TRADE) { //&& city->is_open) {
                if (!city->is_open){
                    generic_buttons_handle_mouse(
                        m, (data.panel.x_min + data.panel.x_max - 500) / 2, data.y_max - 105,
                        generic_button_open_trade, 1, &data.selected_button);
                    }
                
            }
        } */

        const empire_object *obj = empire_object_get(selected_object - 1);
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
    
    }  else {
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
    int resource = data.focus_resource;
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

static void button_show_resource_window(const trade_resource_button *btn) 
{
    window_resource_settings_show(btn->res);
}

// static void confirmed_open_trade(int accepted, int checked)
// {
//     if (accepted) {
//         empire_city_open_trade(data.selected_city, 1);
//         building_menu_update();
//         window_trade_opened_show(data.selected_city);
//     }
// }

// static void button_open_trade(const generic_button *button2)
// {
//     window_popup_dialog_show(POPUP_DIALOG_OPEN_TRADE, confirmed_open_trade, 2);
// }

static void confirmed_open_trade_by_route(int accepted, int checked)
{
    if (accepted) {
        int city_id = empire_city_get_for_trade_route(data.selected_trade_route);
        empire_city_open_trade(city_id, 1);
        building_menu_update();
        window_trade_opened_show(city_id);
    }

    data.selected_trade_route = 0;  // Always clear
}

static void button_open_trade_by_route(int route_id)
{
    data.selected_trade_route = route_id;
    window_popup_dialog_show(POPUP_DIALOG_OPEN_TRADE, confirmed_open_trade_by_route, 2);
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
