#include "trade_ledger.h"

#include "city/resource.h"
#include "core/image_group.h"

#include "graphics/graphics.h"
#include "graphics/grid_box.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/tab_view.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"

#define PANEL_W 800
#define PANEL_H 600

static void button_close(int param1, int param2);
static void placeholder_content_draw(tab_view *view, tab *active_tab);

static tab_view ledger_tabs;
static int tabs_initialized = 0;
static resource_list *resources;
static grid_box_type resource_table;

static const lang_fragment tab_text_trade[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Trade"},
};

static const lang_fragment tab_text_production[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Production"},
};

static const lang_fragment tab_text_summary[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Summary"},
};

static image_button image_buttons[] = {
    {744, 554, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1},
};

static void trade_draw_content(tab_view *view, tab *active_tab);
static void draw_resource_row(const grid_box_item *item);

static void trade_ledger_init(void)
{
    resource_table = (grid_box_type) {
        .x = 10,
        .y = 80,
        .width = 45 * BLOCK_SIZE,
        .height = 22 * BLOCK_SIZE,
        .num_columns = 1,
        .item_height = 40,
        .item_margin.horizontal = 10,
        .item_margin.vertical = 5,
        .extend_to_hidden_scrollbar = 1,
        .on_click = 0,
        .draw_item = draw_resource_row,
    };
    // get resource list for the scenario
    city_resource_determine_available(1);
    resources = city_resource_get_available();
    grid_box_init(&resource_table, resources->size);
}

static void draw_background(void)
{
    window_draw_underlying_window();
    graphics_in_dialog_with_size(PANEL_W, PANEL_H);
    outer_panel_draw_colored(0, 0, PANEL_W, PANEL_H, COLOR_MASK_NONE);
    inner_panel_draw_colored(8, 8, PANEL_W - 16, 40, COLOR_MASK_NONE);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_UI_TRADE_LEDGER_HEADER, 0, 12, PANEL_W, FONT_LARGE_BROWN);
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    if (!tabs_initialized) {
        // Initialize tabs on first draw
        trade_ledger_init();
        tab_view_init_simple(&ledger_tabs, 22, 80, 753, 448, 3, TAB_VIEW_STYLE_DEFAULT);
        ledger_tabs.view_properties.width_mode = TAB_WIDTH_MAX; // make tabs take up all available width
        tab_view_init_tab(&ledger_tabs, 0, trade_draw_content, tab_text_trade);
        tab_view_init_tab(&ledger_tabs, 1, placeholder_content_draw, tab_text_production);
        tab_view_init_tab(&ledger_tabs, 2, placeholder_content_draw, tab_text_summary);

        tabs_initialized = tab_view_layout(&ledger_tabs) == TAB_LAYOUT_OK; // layout tabs and set initialized flag based on success  
    }


    graphics_in_dialog_with_size(PANEL_W, PANEL_H);
    tab_view_draw(&ledger_tabs);
    graphics_in_dialog_with_size(PANEL_W, PANEL_H);
    image_buttons_draw(0, 0, image_buttons, 1);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog_with_size(m, PANEL_W, PANEL_H);

    if (tab_view_handle_mouse(m_dialog, &ledger_tabs)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, image_buttons, 1, 0)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_go_back();
    }
    grid_box_handle_input(&resource_table, m_dialog, 1);
}

static void button_close(int param1, int param2)
{
    window_go_back();
}

static void placeholder_content_draw(tab_view *view, tab *active_tab)
{
    (void) view;
    (void) active_tab;
    int active = view->state.active_tab;
    lang_text_draw_sequence(view->tabs[active].button.sequence, 1, 20, 20, FONT_LARGE_BROWN, COLOR_MASK_NONE);
}

static void draw_resource_row(const grid_box_item *item)
{
    // Placeholder for drawing a resource row in the trade ledger
    // This function will be called for each item in the grid box
    // You can access the resource data using item->index
    // For example, you might want to draw the resource name, quantity, etc.


    int real_index = item->index;
    resource_type current_resource = resources->items[real_index];
    int resource_img_id = resource_get_data(current_resource)->image.icon;
    const uint8_t *name = resource_get_data(current_resource)->text;
    image_draw(resource_img_id, item->x + 5, item->y + 5, COLOR_MASK_NONE, SCALE_NONE);
    text_draw(name, item->x + 40, item->y + 10, FONT_NORMAL_BLACK, COLOR_MASK_NONE);

}

static void trade_draw_content(tab_view *view, tab *active_tab)
{
    // start with resource overview, second function for by city view
    (void) view;
    (void) active_tab;
    int active = view->state.active_tab;

    // header row  - replace with borderless buttons for each column, with sorting func
    int x_gap = 40;
    int starting_y = 65;
    lang_text_draw(CUSTOM_TRANSLATION, TR_PARAMETER_TYPE_RESOURCE, 20, starting_y, FONT_NORMAL_BLACK);
    text_draw((const uint8_t *) "#", 120, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Imp", 120 + x_gap, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Pro", 120 + 2 * x_gap, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Con", 120 + 3 * x_gap, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Exp", 120 + 4 * x_gap, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Dn", 120 + 7 * x_gap, starting_y, FONT_NORMAL_BLACK, COLOR_MASK_NONE);


    grid_box_request_refresh(&resource_table);
    grid_box_draw(&resource_table);

}

void window_trade_ledger_show(void)
{

    window_type window = {
        WINDOW_TRADE_LEDGER,
        draw_background,
        draw_foreground,
        handle_input
    };
    window_show(&window);
}
