#include "trade_ledger.h"

#include "city/finance.h"
#include "city/resource.h"
#include "core/image_group.h"
#include "graphics/complex_button.h"
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
#include "widget/dropdown_button.h"

#define PANEL_W 800
#define PANEL_H 600

static color_t balance_font_red = 0;
static font_t balance_font = FONT_NORMAL_RED;
static font_t balance_font_green = FONT_NORMAL_GREEN;
static color_t bg_color_window = COLOR_MASK_PASTEL_GRAY;
static color_t bg_color_tabs = COLOR_MASK_NONE;
static color_t bg_color_items = COLOR_MASK_NONE;
static color_t brown_correction = 0; // white atlas inversion test
static color_t green_correction = 0; // white atlas inversion test
static color_t active_item_border = COLOR_WHITE;


static void button_close(int param1, int param2);
static void placeholder_content_draw(tab_view *view, tab *active_tab);
static void hide_irrelevant_checkbox_clicked(checkbox_button *btn);
static void refresh_irrelevant_resources(void);

static tab_view ledger_tabs;
static int tabs_initialized = 0;
static const resource_list *resources;
static resource_list filtered_resources;
static grid_box_type resource_table;
static dropdown_button ledger_year_dropdown;
static int selected_year_index = 0;
static int hide_irrelevant = 1; //default to hide

static const lang_fragment hide_irrelevant_sequence[] = {
    {.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_HIDE_IRRELEVANT_RESOURCES},
};

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

static checkbox_button hide_irrelevant_checkbox = {
        .x = 12,
        .y = 430,
        .width = 250,
        .height = 20,
        .left_click_handler = hide_irrelevant_checkbox_clicked,
    .font = FONT_NORMAL_BROWN,
    .sequence = hide_irrelevant_sequence,
    .sequence_size = 1,
};

static complex_button header_buttons[6] = { 0 }; //header buttons with sorting implemented - will need mini arrows 

static void trade_draw_content(tab_view *view, tab *active_tab);
static void draw_resource_row(const grid_box_item *item);
static void hide_irrelevant_checkbox_clicked(checkbox_button *btn);
static int handle_resource_table_mouse(const mouse *m, void *user_data);
static int handle_trade_tab_mouse(const mouse *m, void *user_data);

static int dropdown_to_years_ago(int selected_index)
{
    if (selected_index <= 1) {
        return 0; // current year
    }
    return selected_index - 1; // 1..7 years ago
}

static void dropdown_selected_callback(dropdown_button *dd)
{
    selected_year_index = dropdown_to_years_ago(dd->selected_index);
    window_invalidate();
}

static void hide_irrelevant_checkbox_clicked(checkbox_button *btn)
{
    hide_irrelevant = btn->is_checked;
    refresh_irrelevant_resources();
    int total_items = hide_irrelevant ? filtered_resources.size : resources->size;
    grid_box_update_total_items(&resource_table, total_items);
    window_invalidate();
}

static void refresh_irrelevant_resources(void)
{
    filtered_resources.size = 0;

    for (unsigned int i = 0; i < resources->size; i++) {
        resource_type current_resource = resources->items[i];

        int imported = city_finance_trade_ledger_get_imported(current_resource, selected_year_index);
        int produced = city_finance_trade_ledger_get_produced(current_resource, selected_year_index);
        int consumed = city_finance_trade_ledger_get_consumed(current_resource, selected_year_index);
        int exported = city_finance_trade_ledger_get_exported(current_resource, selected_year_index);
        int stock = city_finance_trade_ledger_get_stock(current_resource, selected_year_index);
        int balance = city_finance_trade_ledger_get_balance(current_resource, selected_year_index);

        if (imported || produced || consumed || exported || stock || balance) {
            filtered_resources.items[filtered_resources.size++] = current_resource;
        }
    }
}

static void trade_ledger_init(void)
{
    static lang_fragment dd_fragments[9] = { 0 };

    for (int i = 0; i < 3; i++) {
        dd_fragments[i].type = LANG_FRAG_LABEL;
        dd_fragments[i].text_group = CUSTOM_TRANSLATION;
    }
    dd_fragments[0].text_id = TR_UI_SELECT_TRADE_LEDGER_YEAR; // anchor
    dd_fragments[1].text_id = TR_UI_CURRENT_YEAR;
    dd_fragments[2].text_id = TR_UI_LAST_YEAR;

    for (int i = 3; i < 9; i++) {
        dd_fragments[i].type = LANG_FRAG_AMOUNT;
        dd_fragments[i].text_group = CUSTOM_TRANSLATION;
        dd_fragments[i].text_id = TR_UI_YEAR_AGO;
        dd_fragments[i].number = i - 1;
    }

    selected_year_index = 0;
    dropdown_button_init_simple(580, 430, 0, 0, dd_fragments, 9, &ledger_year_dropdown, DD_BUTTON_STYLE_DEFAULT, 0);
    ledger_year_dropdown.show_origin = 1; // show anchor button when expanded
    ledger_year_dropdown.selected_callback = dropdown_selected_callback;
    ledger_year_dropdown.selected_index = 1; // default to "Current Year"
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
    filtered_resources = *resources;
    hide_irrelevant_checkbox.is_checked = hide_irrelevant;
    refresh_irrelevant_resources();
    if (hide_irrelevant) {
        grid_box_init(&resource_table, filtered_resources.size);
    } else {
        grid_box_init(&resource_table, resources->size);
    }
}

static void draw_background(void)
{
    window_draw_underlying_window();
    graphics_in_dialog_with_size(PANEL_W, PANEL_H);
    outer_panel_draw_colored(0, 0, PANEL_W, PANEL_H, bg_color_window);
    inner_panel_draw_colored(8, 8, PANEL_W - 16, 40, COLOR_MASK_NONE);
    lang_text_draw_centered(CUSTOM_TRANSLATION, TR_UI_TRADE_LEDGER_HEADER, 0, 17, PANEL_W, FONT_LARGE_BROWN);
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    if (!tabs_initialized) {
        // Initialize tabs on first draw
        trade_ledger_init();
        tab_view_init_simple(&ledger_tabs, 22, 80, 753, 464, 3, TAB_VIEW_STYLE_WOOD);
        ledger_tabs.view_properties.width_mode = TAB_WIDTH_MAX; // make tabs take up all available width
        tab_view_init_tab(&ledger_tabs, 0, trade_draw_content, tab_text_trade);
        tab_view_init_tab(&ledger_tabs, 1, placeholder_content_draw, tab_text_production);
        tab_view_init_tab(&ledger_tabs, 2, placeholder_content_draw, tab_text_summary);

        tabs_initialized = tab_view_layout(&ledger_tabs) == TAB_LAYOUT_OK; // layout tabs and set initialized flag based on success  
    }

    graphics_in_dialog_with_size(PANEL_W, PANEL_H);
    image_buttons_draw(0, 0, image_buttons, 1);
    tab_view_draw(&ledger_tabs);

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
    if (checkbox_button_handle_mouse(&hide_irrelevant_checkbox, m_dialog)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_go_back();
    }
    tab_view_handle_content_mouse(&ledger_tabs, m_dialog, PANEL_W, PANEL_H, handle_trade_tab_mouse, 0);
}

static int handle_resource_table_mouse(const mouse *m, void *user_data)
{
    grid_box_type *grid = user_data;
    if (!grid) {
        return 0;
    }
    return grid_box_handle_input(grid, m, 1);
}

static int handle_trade_tab_mouse(const mouse *m, void *user_data)
{
    (void) user_data;
    if (tab_view_get_active_tab(&ledger_tabs) != 0) {
        return 0;
    }
    if (dropdown_button_handle_mouse(&ledger_year_dropdown, m)) {
        return 1;
    }
    if (checkbox_button_handle_mouse(&hide_irrelevant_checkbox, m)) {
        return 1;
    }
    return handle_resource_table_mouse(m, &resource_table);
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
    lang_text_draw_sequence(view->tabs[active].button.sequence, 1, 20, 20, FONT_LARGE_BROWN, brown_correction);
}

static void draw_resource_row(const grid_box_item *item)
{
    // Placeholder for drawing a resource row in the trade ledger
    // This function will be called for each item in the grid box
    // You can access the resource data using item->index
    // For example, you might want to draw the resource name, quantity, etc.


    int real_index = item->index;
    resource_type current_resource = hide_irrelevant ? filtered_resources.items[real_index] : resources->items[real_index];

    int resource_img_id = resource_get_data(current_resource)->image.icon;
    const uint8_t *name = resource_get_data(current_resource)->text;
    int imported = city_finance_trade_ledger_get_imported(current_resource, selected_year_index);
    int produced = city_finance_trade_ledger_get_produced(current_resource, selected_year_index);
    int consumed = city_finance_trade_ledger_get_consumed(current_resource, selected_year_index);
    int exported = city_finance_trade_ledger_get_exported(current_resource, selected_year_index);
    int stock = city_finance_trade_ledger_get_stock(current_resource, selected_year_index);
    int balance = city_finance_trade_ledger_get_balance(current_resource, selected_year_index);
    // sort this out or wrap it into some helper
    font_t bal_font;
    color_t bal_color;

    if (balance < 0) {
        bal_font = balance_font;
        bal_color = balance_font_red;
    } else {
        bal_color = green_correction;
        bal_font = balance_font_green;
    }

    int x_gap = 40;
    int number_y = item->y + 10;
    // unbordered_panel_draw_colored(item->x, item->y, item->width / BLOCK_SIZE, item->height / BLOCK_SIZE, bg_color_items);
    // button_border_draw(item->x, item->y, item->width, item->height, 0);
    int is_focused = item->is_focused;
    bordered_panel_draw_colored(item->x, item->y, item->width, item->height, is_focused, bg_color_items, active_item_border);


    image_draw(resource_img_id, item->x + 5, item->y + 5, COLOR_MASK_NONE, SCALE_NONE);
    text_draw(name, item->x + 40, item->y + 10, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(imported, 120 + x_gap, number_y, x_gap, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(produced, 120 + 2 * x_gap, number_y, x_gap, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(consumed, 120 + 3 * x_gap, number_y, x_gap, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(exported, 120 + 4 * x_gap, number_y, x_gap, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(stock, 120 + 6 * x_gap, number_y, x_gap, FONT_NORMAL_BROWN, brown_correction);
    text_draw_number_centered_colored(balance, 120 + 8 * x_gap, number_y, x_gap, bal_font, bal_color);

}

static void trade_draw_content(tab_view *view, tab *active_tab)
{
    // start with resource overview, second function for by city view
    (void) view;
    (void) active_tab;
    // int active = view->state.active_tab;

    // header row  - replace with borderless buttons for each column, with sorting func
    int x_gap = 40;
    int starting_y = 65;
    lang_text_draw(CUSTOM_TRANSLATION, TR_PARAMETER_TYPE_RESOURCE, 20, starting_y, FONT_NORMAL_BROWN);
    //text_draw((const uint8_t *) "#", 120, starting_y, FONT_NORMAL_BROWN, COLOR_MASK_NONE);
    text_draw((const uint8_t *) "Imp", 120 + x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);
    text_draw((const uint8_t *) "Pro", 120 + 2 * x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);
    text_draw((const uint8_t *) "Con", 120 + 3 * x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);
    text_draw((const uint8_t *) "Exp", 120 + 4 * x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);
    text_draw((const uint8_t *) "Stock", 120 + 6 * x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);
    text_draw((const uint8_t *) "Dn", 120 + 8 * x_gap, starting_y, FONT_NORMAL_BROWN, brown_correction);


    grid_box_request_refresh(&resource_table);
    grid_box_draw(&resource_table);
    checkbox_button_draw(&hide_irrelevant_checkbox);
    dropdown_button_draw(&ledger_year_dropdown);

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
