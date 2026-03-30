#include "trade_ledger.h"

#include "core/image_group.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "input/input.h"

#define PANEL_W 800
#define PANEL_H 600

static void button_close(int param1, int param2);

static image_button image_buttons[] = {
    {744, 554, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1},
};
// replace with generic_buttons or complex_buttons later

//tab system to be implemented as a standalone component, utilising the complex buttons structure

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
    graphics_in_dialog_with_size(PANEL_W, PANEL_H);

    image_buttons_draw(0, 0, image_buttons, 1);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (image_buttons_handle_mouse(mouse_in_dialog_with_size(m, PANEL_W, PANEL_H), 0, 0, image_buttons, 1, 0)) {
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
