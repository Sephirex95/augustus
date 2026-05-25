#include "trade_ledger.h"

#include "core/image_group.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/tab_view.h"
#include "graphics/window.h"
#include "input/input.h"

#define PANEL_W 800
#define PANEL_H 600

static void button_close(int param1, int param2);
static void placeholder_content_draw(tab_view *view, tab *active_tab);

static tab_view ledger_tabs;
static int tabs_initialized = 0;

static const lang_fragment tab_text_imports[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Imports"},
};

static const lang_fragment tab_text_exports[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Exports"},
};

static const lang_fragment tab_text_summary[] = {
    {.type = LANG_FRAG_TEXT, .text = (const uint8_t *) "Summary"},
};

static image_button image_buttons[] = {
    {744, 554, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1},
};

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
        tab_view_init_simple(&ledger_tabs, 24, 80, 752, 448, 4, TAB_VIEW_STYLE_DEFAULT);
        ledger_tabs.view_properties.width_mode = TAB_WIDTH_MAX; // make tabs take up all available width
        tab_view_init_tab(&ledger_tabs, 0, placeholder_content_draw, tab_text_imports);
        tab_view_init_tab(&ledger_tabs, 1, placeholder_content_draw, tab_text_exports);
        tab_view_init_tab(&ledger_tabs, 2, placeholder_content_draw, tab_text_summary);
        tab_view_init_tab(&ledger_tabs, 3, placeholder_content_draw, tab_text_imports);

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
