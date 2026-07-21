#include "trade_ledger.h"

#include "assets/assets.h"
#include "city/finance.h"
#include "city/resource.h"
#include "core/image_group.h"
#include "core/lang.h"
#include "core/string.h"
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
#include "window/resource_settings.h"

#define PANEL_W 800
#define PANEL_H 600
#define LEDGER_HEADER_BUTTON_COUNT 6
#define LEDGER_HEADER_BUTTON_HEIGHT 20
#define LEDGER_HEADER_X_GAP 40
#define LEDGER_HEADER_STARTING_Y 65
#define LEDGER_TABLE_X 10
#define LEDGER_TABLE_WIDTH (45 * BLOCK_SIZE)
#define LEDGER_HEADER_BUTTON_WIDTH_ADJUSTMENT 10
#define LEDGER_HEADER_SMALL_WIDTH (LEDGER_HEADER_X_GAP + LEDGER_HEADER_BUTTON_WIDTH_ADJUSTMENT)
#define LEDGER_HEADER_STOCK_WIDTH (58 + LEDGER_HEADER_BUTTON_WIDTH_ADJUSTMENT)
#define LEDGER_HEADER_BALANCE_WIDTH 160
#define LEDGER_HEADER_IMPORTED_X (120 + LEDGER_HEADER_X_GAP)
#define LEDGER_HEADER_PRODUCED_X (LEDGER_HEADER_IMPORTED_X + LEDGER_HEADER_SMALL_WIDTH)
#define LEDGER_HEADER_CONSUMED_X (LEDGER_HEADER_PRODUCED_X + LEDGER_HEADER_SMALL_WIDTH)
#define LEDGER_HEADER_EXPORTED_X (LEDGER_HEADER_CONSUMED_X + LEDGER_HEADER_SMALL_WIDTH)
#define LEDGER_HEADER_STOCK_X (LEDGER_HEADER_EXPORTED_X + LEDGER_HEADER_SMALL_WIDTH)
#define LEDGER_HEADER_BALANCE_X (LEDGER_HEADER_STOCK_X + LEDGER_HEADER_STOCK_WIDTH)
#define LEDGER_TRADE_STATUS_COLUMN_X (LEDGER_HEADER_BALANCE_X + LEDGER_HEADER_BALANCE_WIDTH)
#define LEDGER_TRADE_STATUS_COLUMN_WIDTH (LEDGER_TABLE_X + LEDGER_TABLE_WIDTH - LEDGER_TRADE_STATUS_COLUMN_X)
#define LEDGER_TRADE_STATUS_ICON_WIDTH 17
#define LEDGER_TRADE_STATUS_ICON_SPACING -2
#define LEDGER_TRADE_STATUS_BUTTON_MAX (RESOURCE_MAX * 2)
#define LEDGER_TRADE_STATUS_TOOLTIP_MAX 96

typedef enum {
    LEDGER_HEADER_IMPORTED = 0,
    LEDGER_HEADER_PRODUCED,
    LEDGER_HEADER_CONSUMED,
    LEDGER_HEADER_EXPORTED,
    LEDGER_HEADER_STOCK,
    LEDGER_HEADER_BALANCE
} ledger_header_button_type;

static color_t balance_font_red = 0;
static font_t balance_font = FONT_NORMAL_RED;
static font_t balance_font_green = FONT_NORMAL_GREEN;
static color_t bg_color_window = COLOR_MASK_PASTEL_GRAY;
static color_t brown_correction = 0; // white atlas inversion test
static color_t green_correction = 0; // white atlas inversion test


static void button_close(int param1, int param2);
static void placeholder_content_draw(tab_view *view, tab *active_tab);
static void hide_irrelevant_checkbox_clicked(checkbox_button *btn);
static void refresh_irrelevant_resources(void);
static void setup_header_buttons(void);
static void ledger_header_button_click(cycling_button *button);
static void draw_trade_status_column(resource_type resource, int row_y, int row_height);
static void update_trade_status_button_focus(const mouse *m);
static void on_resource_row_click(const grid_box_item *item);
static void handle_tooltip(tooltip_context *c);

static tab_view ledger_tabs;
static int tabs_initialized = 0;
static const resource_list *resources;
static resource_list filtered_resources;
static grid_box_type resource_table;
static dropdown_button ledger_year_dropdown;
static int selected_year_index = 0;
static int hide_irrelevant = 1; //default to hide
static complex_button trade_status_buttons[LEDGER_TRADE_STATUS_BUTTON_MAX] = { 0 };
static uint8_t trade_status_tooltips[LEDGER_TRADE_STATUS_BUTTON_MAX][LEDGER_TRADE_STATUS_TOOLTIP_MAX] = { 0 };
static int trade_status_button_count = 0;

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
    .font = FONT_NORMAL_BLACK,
    .sequence = hide_irrelevant_sequence,
    .sequence_size = 1,
};

static cycling_button header_buttons[LEDGER_HEADER_BUTTON_COUNT] = { 0 };

static const lang_fragment header_button_sequences[LEDGER_HEADER_BUTTON_COUNT][1] = {
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_IMPORTED}},
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_PRODUCED}},
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_CONSUMED}},
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_EXPORTED}},
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_STOCK}},
    {{.type = LANG_FRAG_LABEL, .text_group = CUSTOM_TRANSLATION, .text_id = TR_UI_LEDGER_BTN_BALANCE}},
};

static const int header_button_tooltips[LEDGER_HEADER_BUTTON_COUNT] = {
    TR_UI_LEDGER_TT_IMPORTED,
    TR_UI_LEDGER_TT_PRODUCED,
    TR_UI_LEDGER_TT_CONSUMED,
    TR_UI_LEDGER_TT_EXPORTED,
    TR_UI_LEDGER_TT_STOCK,
    TR_UI_LEDGER_TT_BALANCE
};

static const int header_button_x_positions[LEDGER_HEADER_BUTTON_COUNT] = {
    LEDGER_HEADER_IMPORTED_X,
    LEDGER_HEADER_PRODUCED_X,
    LEDGER_HEADER_CONSUMED_X,
    LEDGER_HEADER_EXPORTED_X,
    LEDGER_HEADER_STOCK_X,
    LEDGER_HEADER_BALANCE_X
};

static const int header_button_widths[LEDGER_HEADER_BUTTON_COUNT] = {
    LEDGER_HEADER_SMALL_WIDTH,
    LEDGER_HEADER_SMALL_WIDTH,
    LEDGER_HEADER_SMALL_WIDTH,
    LEDGER_HEADER_SMALL_WIDTH,
    LEDGER_HEADER_STOCK_WIDTH,
    LEDGER_HEADER_BALANCE_WIDTH
};

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

static void setup_header_buttons(void)
{
    const int button_y = LEDGER_HEADER_STARTING_Y - 4;
    const int down_arrow = assets_lookup_image_id(ASSET_UI_FONT_BROWN_DOWN_ARROW);
    const int up_arrow = assets_lookup_image_id(ASSET_UI_FONT_BROWN_UP_ARROW);

    for (int i = 0; i < LEDGER_HEADER_BUTTON_COUNT; i++) {
        cycling_button *button = &header_buttons[i];
        button->x = header_button_x_positions[i];
        button->y = button_y;
        button->width = header_button_widths[i];
        button->height = LEDGER_HEADER_BUTTON_HEIGHT;
        button->style = CYCLING_BUTTON_STYLE_RAW;
        button->state_count = 3;
        button->state_index = 0;
        button->left_click_handler = ledger_header_button_click;
        button->right_click_handler = ledger_header_button_click;

        for (int state = 0; state < button->state_count; state++) {
            button->states[state].sequence = header_button_sequences[i];
            button->states[state].sequence_size = 1;
            button->states[state].image_before = 0;
            button->states[state].image_after = 0;
            button->states[state].font = FONT_NORMAL_BLACK;
            button->states[state].color_mask = brown_correction;
            button->states[state].tooltip_c.translation_key = header_button_tooltips[i];
        }

        button->states[1].image_after = down_arrow;
        button->states[2].image_after = up_arrow;
    }
}

static void ledger_header_button_click(cycling_button *button)
{
    if (button->state_index == 0) {
        button->state_index = 1;
    }
}

static int remaining_text_length(const uint8_t *cursor, const uint8_t *buffer, int buffer_size)
{
    int remaining = buffer_size - (int) (cursor - buffer);
    return remaining > 0 ? remaining : 1;
}

static uint8_t *append_trade_quantity(uint8_t *cursor, uint8_t *buffer, int buffer_size, int quantity, int show_max_for_zero)
{
    cursor = string_copy(string_from_ascii(" "), cursor, remaining_text_length(cursor, buffer, buffer_size));
    if (show_max_for_zero && quantity == 0) {
        return string_copy(translation_for(TR_ADVISOR_TRADE_NO_LIMIT), cursor,
            remaining_text_length(cursor, buffer, buffer_size));
    }
    cursor += string_from_int(cursor, quantity, 0);
    return cursor;
}

static void build_trade_status_tooltip(resource_type resource, uint8_t *buffer, int buffer_size)
{
    uint8_t *cursor = buffer;
    resource_trade_status trade_status = city_resource_trade_status(resource);
    int export_over = city_resource_export_over(resource);
    int import_over = city_resource_import_over(resource);
    int has_export = (trade_status & TRADE_STATUS_EXPORT) == TRADE_STATUS_EXPORT;
    int has_import = (trade_status & TRADE_STATUS_IMPORT) == TRADE_STATUS_IMPORT;

    buffer[0] = 0;
    if (has_export) {
        cursor = string_copy(lang_get_string(54, 6), cursor, buffer_size);
        cursor = append_trade_quantity(cursor, buffer, buffer_size, export_over, 0);
    }
    if (has_export && has_import) {
        cursor = string_copy(string_from_ascii("\n"), cursor, remaining_text_length(cursor, buffer, buffer_size));
    }
    if (has_import) {
        cursor = string_copy(lang_get_string(54, 5), cursor, remaining_text_length(cursor, buffer, buffer_size));
        append_trade_quantity(cursor, buffer, buffer_size, import_over, 1);
    }
}

static void draw_trade_status_button(resource_type resource, int image_id, int x, int y)
{
    if (trade_status_button_count >= LEDGER_TRADE_STATUS_BUTTON_MAX) {
        return;
    }

    complex_button *button = &trade_status_buttons[trade_status_button_count];
    build_trade_status_tooltip(resource, trade_status_tooltips[trade_status_button_count], LEDGER_TRADE_STATUS_TOOLTIP_MAX);

    button->x = x;
    button->y = y;
    button->width = LEDGER_TRADE_STATUS_ICON_WIDTH;
    button->height = LEDGER_TRADE_STATUS_ICON_WIDTH;
    button->style = COMPLEX_BUTTON_STYLE_RAW;

    button->image.id = image_id;
    button->image.auto_center = 1;

    button->tooltip_c.type = TOOLTIP_BUTTON;
    button->tooltip_c.precomposed_text = trade_status_tooltips[trade_status_button_count];
    complex_button_draw(button);
    trade_status_button_count++;
}

static void draw_trade_status_column(resource_type resource, int row_y, int row_height)
{
    resource_trade_status trade_status = city_resource_trade_status(resource);
    const int is_imported = (trade_status & TRADE_STATUS_IMPORT) == TRADE_STATUS_IMPORT;
    const int is_exported = (trade_status & TRADE_STATUS_EXPORT) == TRADE_STATUS_EXPORT;
    const int icon_count = is_imported + is_exported;

    if (!icon_count || LEDGER_TRADE_STATUS_COLUMN_WIDTH <= 0) {
        return;
    }

    const int icons_width = icon_count * LEDGER_TRADE_STATUS_ICON_WIDTH +
        (icon_count - 1) * LEDGER_TRADE_STATUS_ICON_SPACING;
    int x = LEDGER_TRADE_STATUS_COLUMN_X + (LEDGER_TRADE_STATUS_COLUMN_WIDTH - icons_width) / 2;
    int y = row_y + (row_height - LEDGER_TRADE_STATUS_ICON_WIDTH) / 2;

    if (is_imported) {
        draw_trade_status_button(resource, assets_lookup_image_id(ASSET_UI_TRADE_LEDGER_IMPORT), x, y);
        x += LEDGER_TRADE_STATUS_ICON_WIDTH + LEDGER_TRADE_STATUS_ICON_SPACING;
    }
    if (is_exported) {
        draw_trade_status_button(resource, assets_lookup_image_id(ASSET_UI_TRADE_LEDGER_EXPORT), x, y);
    }
}

static void update_trade_status_button_focus(const mouse *m)
{
    for (int i = 0; i < trade_status_button_count; i++) {
        complex_button *button = &trade_status_buttons[i];
        int is_focused = m->x >= button->x && m->x < button->x + button->width &&
            m->y >= button->y && m->y < button->y + button->height;
        if (button->is_focused != is_focused) {
            button->is_focused = is_focused;
            window_request_refresh();
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
    setup_header_buttons();
    resource_table = (grid_box_type) {
            .x = LEDGER_TABLE_X,
            .y = 80,
            .width = LEDGER_TABLE_WIDTH,
            .height = 22 * BLOCK_SIZE,
            .num_columns = 1,
            .item_height = 40,
            .item_margin.horizontal = 10,
            .item_margin.vertical = 5,
            .draw_inner_panel = 1,
            .extend_to_hidden_scrollbar = 1,
            .on_click = on_resource_row_click,
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
        tab_view_init_simple(&ledger_tabs, 22, 80, 753, 464, 3, TAB_VIEW_STYLE_DEFAULT);
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
    if (cycling_button_handle_mouse_array(header_buttons, m, LEDGER_HEADER_BUTTON_COUNT)) {
        return 1;
    }
    update_trade_status_button_focus(m);
    if (checkbox_button_handle_mouse(&hide_irrelevant_checkbox, m)) {
        return 1;
    }
    return handle_resource_table_mouse(m, &resource_table);
}

static void button_close(int param1, int param2)
{
    window_go_back();
}

static void on_resource_row_click(const grid_box_item *item)
{
    unsigned int real_index = item->index;
    unsigned int total_items = hide_irrelevant ? filtered_resources.size : resources->size;
    if (real_index >= total_items) {
        return;
    }
    resource_type resource = hide_irrelevant ? filtered_resources.items[real_index] : resources->items[real_index];
    window_resource_settings_show(resource);
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

    int number_y = item->y + 10;
    int is_focused = item->is_focused;
    inner_panel_draw_colored(item->x, item->y, item->width, item->height, COLOR_MASK_NONE);
    button_border_draw(item->x, item->y, item->width, item->height, is_focused);

    image_draw(resource_img_id, item->x + 5, item->y + 5, COLOR_MASK_NONE, SCALE_NONE);
    text_draw(name, item->x + 40, item->y + 10, FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(imported, header_button_x_positions[LEDGER_HEADER_IMPORTED], number_y,
        header_button_widths[LEDGER_HEADER_IMPORTED], FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(produced, header_button_x_positions[LEDGER_HEADER_PRODUCED], number_y,
        header_button_widths[LEDGER_HEADER_PRODUCED], FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(consumed, header_button_x_positions[LEDGER_HEADER_CONSUMED], number_y,
        header_button_widths[LEDGER_HEADER_CONSUMED], FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(exported, header_button_x_positions[LEDGER_HEADER_EXPORTED], number_y,
        header_button_widths[LEDGER_HEADER_EXPORTED], FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(stock, header_button_x_positions[LEDGER_HEADER_STOCK], number_y,
        header_button_widths[LEDGER_HEADER_STOCK], FONT_NORMAL_GREEN, brown_correction);
    text_draw_number_centered_colored(balance, header_button_x_positions[LEDGER_HEADER_BALANCE], number_y,
        header_button_widths[LEDGER_HEADER_BALANCE], bal_font, bal_color);
    draw_trade_status_column(current_resource, item->y, item->height);

}

static void trade_draw_content(tab_view *view, tab *active_tab)
{
    // start with resource overview, second function for by city view
    (void) view;
    (void) active_tab;
    // int active = view->state.active_tab;

    // header row
    lang_text_draw(CUSTOM_TRANSLATION, TR_PARAMETER_TYPE_RESOURCE, 20, LEDGER_HEADER_STARTING_Y, FONT_NORMAL_BLACK);
    cycling_button_draw_array(header_buttons, LEDGER_HEADER_BUTTON_COUNT);

    trade_status_button_count = 0;
    grid_box_request_refresh(&resource_table);
    grid_box_draw(&resource_table);
    checkbox_button_draw(&hide_irrelevant_checkbox);
    dropdown_button_draw(&ledger_year_dropdown);

}

static void handle_tooltip(tooltip_context *c)
{
    if (tab_view_get_active_tab(&ledger_tabs) != 0) {
        return;
    }
    if (complex_button_handle_tooltip_array(trade_status_buttons, c, trade_status_button_count)) {
        return;
    }
    cycling_button_handle_tooltip_array(header_buttons, c, LEDGER_HEADER_BUTTON_COUNT);
}

void window_trade_ledger_show(void)
{

    window_type window = {
        WINDOW_TRADE_LEDGER,
        draw_background,
        draw_foreground,
        handle_input,
        handle_tooltip
    };
    window_show(&window);
}
