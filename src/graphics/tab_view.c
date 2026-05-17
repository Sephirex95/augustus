#include "tab_view.h"

#include "graphics/button.h"
#include "graphics/complex_button.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "input/mouse.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

//draft implementation of drawing made by gpt - review the algortihm he outline and make sure it's relevant and necssary
//drawing parts look good, but the naming convention will be off. Check against other UI elements to make sure API fits

/* Notes: tab view has to be informed whether its expected to draw background of the content area or not.
If yes - it should probably be an indented area, or seemeless blend with the tab buttons. Good to test on several styles.
if not - content area background drawing function is null, and the content area 'inherits' the background from the window
it's positioned in.
Then, the content drawing callback for tabs should focus on exactly that - content only. Interactive elements, etc.
Might have to check structures to ensure that the definitions match this approach.

Styles shouldnt live with complex buttons I feel like. Or if they do, we need widget_styles, button_styles and window_styles.
Either that or a global styles structure that will handle parameters for all UI elements - widgets, buttons, windows alike.

EDIT: not a bad idea tbh but adjusting all exisintg windows to use the styles might be a mammoth excercise.
Won't kill us to define a few styles for larger structures, to make sure they use consistent:
Backgroud drawing, colours, fonts, sizes, titles, etc. It already exists for windows, right? <-reserach this

Establishing blanket templates for all these will make creating new UIs very easy and consistent.
Mixing these basic 3-4 styles from basegame into different widgets should provide an experience varied enough.

Very base style of the tab_view should pull from the visuals of the tabs in settings - that's the best example of
that visual design. There are some quirks in there that I'd like to get rid of though, like changing tab width.

Other parameters, properties to consider adding to tab_view:
>Tabs of equal width? yes/no
>if no, force max/min width?
>behaviour if the text doesn't fit in the button - make sure complex_button manages this on it's own.
>how do we treat scrollbar? I think best solution is to add the dimensions of content area to tab's structure.
 Then, the tab can decide on it's own if the scrollbar is required and where it should go.
 Set tab_view's 'master' content_area's dimensions - that's the window in which you'll be viewing content.
 If the actual content's dimensions exceed that - add scrollbars and pif paf done.
>tab buttons indication of active tab. My favourite and the default style should be removing botton border to
 make the tab button blend with the content area, like in settings.
Other considerations should be style-specific I reckon. Next time, continue iterating on the main drawing style for the
tab_view, ensure that the test trade ledger looks good and proceed with specifics in the trade_ledger itself.

Next iteration - FINISH tab_view as a structure!!
*/

#define TAB_VIEW_MIN_TAB_WIDTH 50

static complex_button_style button_style_for_tab_style(tab_view_style style)
{
    switch (style) {
        case TAB_VIEW_STYLE_GRAY:
            return COMPLEX_BUTTON_STYLE_GRAY;
        case TAB_VIEW_STYLE_DEFAULT_SMALL:
            return COMPLEX_BUTTON_STYLE_DEFAULT_SMALL;
        case TAB_VIEW_STYLE_COLORFUL:
        case TAB_VIEW_STYLE_DEFAULT:
        default:
            return COMPLEX_BUTTON_STYLE_DEFAULT;
    }
}

static font_t button_font_for_tab_style(tab_view_style style)
{
    switch (style) {
        case TAB_VIEW_STYLE_DEFAULT_SMALL:
            return FONT_SMALL_PLAIN;
        case TAB_VIEW_STYLE_GRAY:
        case TAB_VIEW_STYLE_COLORFUL:
        case TAB_VIEW_STYLE_DEFAULT:
        default:
            return FONT_NORMAL_BLACK;
    }
}

static color_t color_for_active_tab(tab_view_style style, int is_active)
{
    // Only apply color differentiation in colorful mode
    if (style == TAB_VIEW_STYLE_COLORFUL) {
        if (is_active) {
            return COLOR_MASK_NONE;
        }
        // Subtle color for inactive tabs in colorful mode
        return COLOR_MASK_PASTEL_GRAY;
    }
    // For default styles, keep all tabs uniform color
    return COLOR_MASK_NONE;
}

static int tab_spread_gap(tab_spread spread)
{
    switch (spread) {
        case TAB_SPREAD_SMALL:
            return 2;
        case TAB_SPREAD_WIDE:
            return 8;
        case TAB_SPREAD_MAX:
        case TAB_SPREAD_NONE:
        default:
            return 0;
    }
}

static void tab_click_handler(const complex_button *btn)
{
    tab_view *view = (tab_view *) btn->user_data;
    if (!view) {
        return;
    }

    // Find which tab was clicked
    for (int i = 0; i < view->view_properties.count; i++) {
        if (&view->tabs[i].button == btn) {
            if (view->view_properties.active_tab != i) {
                view->view_properties.active_tab = i;
                window_request_refresh();
            }
            return;
        }
    }
}

void tab_view_init_simple(tab_view *view, int x, int y, int width, int height, int tab_count, tab_view_style style)
{
    if (!view || tab_count <= 0) { // protect from invalid parameters
        return;
    }

    memset(view, 0, sizeof(*view)); // prepare memory for view

    view->x = x;
    view->y = y;
    view->width = width > 0 ? width : 100;
    view->height = height > 0 ? height : 100;
    view->tab_height = style == TAB_VIEW_STYLE_DEFAULT_SMALL ? 20 : 26;

    view->view_properties.style = style;
    view->view_properties.position = TAB_POS_CENTER; // default position
    view->view_properties.count = tab_count;
    view->view_properties.spread = TAB_SPREAD_SMALL; // default spread
    view->view_properties.active_tab = 0;

    view->tabs = calloc(tab_count, sizeof(tab)); // allocate memory for tabs

    if (!view->tabs) { // if memory allocation failed, clean up and return
        memset(view, 0, sizeof(*view));
        return;
    }

    // Initialize tab buttons with defaults
    for (int i = 0; i < tab_count; i++) {
        memset(&view->tabs[i].button, 0, sizeof(complex_button));
        view->tabs[i].button.left_click_handler = tab_click_handler;
        view->tabs[i].button.user_data = view;
        view->tabs[i].button.style = button_style_for_tab_style(style);
        view->tabs[i].button.font = button_font_for_tab_style(style);
        view->tabs[i].button.sequence_position = SEQUENCE_POSITION_CENTER;
        view->tabs[i].button.sequence_size = 1;
        view->tabs[i].visible = 1;
        view->tabs[i].enabled = 1;
    }

    tab_view_layout(view);
}

void tab_view_destroy(tab_view *view)
{
    if (!view) {
        return; // protect from null pointer
    }

    free(view->tabs); // deallocate tab array
    memset(view, 0, sizeof(*view)); // clear all fields
}

void tab_view_layout(tab_view *view)
{
    /**
     * Layout Algorithm Overview:
     *
     * This function positions all tab buttons and calculates the content area geometry.
     * The layout respects spread and position properties to achieve flexible tab positioning.
     *
     * Steps:
     * 1. Calculate tab_width based on available space and spread mode
     * 2. Determine starting X position based on tab_position (LEFT/CENTER/RIGHT)
     * 3. Position each tab button sequentially with appropriate gaps
     * 4. Apply TAB_SPREAD_MAX remainder distribution if needed
     * 5. Calculate content area as an inset rectangle below/inside the tab_view
     * 6. Update active tab visual state (is_active flag and color_mask)
     */

    if (!view || !view->tabs || view->view_properties.count <= 0) {
        return;
    }

    int tab_count = view->view_properties.count;
    int tab_y = view->y - view->tab_height;  // Tabs positioned ABOVE the content area

    // === Step 1: Calculate total gap space between tabs based on spread mode ===
    int gap = tab_spread_gap(view->view_properties.spread);
    int total_gap = gap * (tab_count - 1);  // gaps only exist between tabs (count-1 gaps)
    int available_for_tabs = view->width - total_gap;  // width available after accounting for gaps

    // TAB_SPREAD_MAX ignores gaps and distributes tabs evenly across full width
    if (view->view_properties.spread == TAB_SPREAD_MAX) {
        gap = 0;
        total_gap = 0;
        available_for_tabs = view->width;
    }

    // === Step 2: Calculate base tab width ===
    // Each tab gets an equal slice of available space
    int tab_width = tab_count > 0 ? available_for_tabs / tab_count : 0;

    // Enforce minimum tab width to prevent tabs becoming too narrow
    if (tab_width < TAB_VIEW_MIN_TAB_WIDTH) {
        tab_width = TAB_VIEW_MIN_TAB_WIDTH;
    }

    // Calculate how much horizontal space all tabs will actually use
    int used_width = tab_width * tab_count + total_gap;
    int start_x = view->x;  // default: start at left

    // === Step 3: Determine horizontal start position based on tab_position ===
    // If tabs don't fill entire width, align them according to position property
    if (used_width < view->width) {
        int leftover = view->width - used_width;
        switch (view->view_properties.position) {
            case TAB_POS_RIGHT:
                // Tabs flush to right edge
                start_x = view->x + leftover;
                break;
            case TAB_POS_CENTER:
                // Tabs centered
                start_x = view->x + leftover / 2;
                break;
            case TAB_POS_LEFT:
            default:
                // Tabs flush to left edge (already set)
                start_x = view->x;
                break;
        }
    }

    // === Step 4: Position each tab button ===
    int x_cursor = start_x;
    for (int i = 0; i < tab_count; i++) {
        int width = tab_width;

        // TAB_SPREAD_MAX: distribute remainder pixels to leftmost tabs
        // Example: 3 tabs in 100px width = 33px base, 1px remainder for first tab
        if (view->view_properties.spread == TAB_SPREAD_MAX && tab_count > 0) {
            int remainder = view->width % tab_count;
            if (i < remainder) {
                width += 1;  // Give extra pixel to first 'remainder' tabs
            }
        }

        // Set button geometry
        view->tabs[i].button.x = x_cursor;
        view->tabs[i].button.y = tab_y;
        view->tabs[i].button.width = width;
        view->tabs[i].button.height = view->tab_height;

        // === Step 5: Update visual state for active/inactive tabs ===
        view->tabs[i].button.is_active = (view->view_properties.active_tab == i);
        view->tabs[i].button.color_mask = color_for_active_tab(
            view->view_properties.style,
            view->view_properties.active_tab == i
        );

        x_cursor += width + gap;  // Move cursor to next tab position
    }

    // === Step 6: Calculate content area geometry ===
    // Content area is flush with tab_view boundaries (no indent for default styles)
    // This keeps the background color uniform across tabs and content
    view->content.x = view->x;
    view->content.y = view->y;
    view->content.width = view->width;
    view->content.height = view->height;
}

void tab_view_init_tab(tab_view *view, int tab_index, content_draw_callback callback, const lang_fragment *frag)
{
    if (!view || !view->tabs) {
        return;
    }

    if (tab_index < 0 || tab_index >= view->view_properties.count) {
        return;
    }

    view->tabs[tab_index].draw_callback = callback;
    view->tabs[tab_index].button.sequence = frag;
    view->tabs[tab_index].button.sequence_size = frag ? 1 : 0;
    view->tabs[tab_index].visible = 1;
    view->tabs[tab_index].enabled = 1;
}

void tab_view_set_tab_text(tab_view *view, int tab_index, const lang_fragment *frag)
{
    if (!view || !view->tabs) {
        return;
    }

    if (tab_index < 0 || tab_index >= view->view_properties.count) {
        return;
    }

    view->tabs[tab_index].button.sequence = frag;
}

void tab_view_set_tab_draw_callback(tab_view *view, int tab_index, content_draw_callback callback)
{
    if (!view || !view->tabs) {
        return;
    }

    if (tab_index < 0 || tab_index >= view->view_properties.count) {
        return;
    }

    view->tabs[tab_index].draw_callback = callback;
}

void tab_view_draw(tab_view *view)
{
    if (!view || !view->tabs || view->view_properties.count <= 0) {
        return;
    }

    // Draw inner panel for content area (no outer border for tab_view itself)
    inner_panel_draw_colored(
        view->content.x,
        view->content.y,
        view->content.width,
        view->content.height,
        COLOR_MASK_NONE
    );

    // Draw all visible tab buttons
    for (int i = 0; i < view->view_properties.count; i++) {
        if (view->tabs[i].visible) {
            complex_button_draw(&view->tabs[i].button);
        }
    }

    // Draw content for active tab
    int active_tab = view->view_properties.active_tab;
    if (active_tab >= 0 && active_tab < view->view_properties.count) {
        tab *active = &view->tabs[active_tab];
        if (active->draw_callback) {
            active->draw_callback(view, active);
        }
    }
}

int tab_view_handle_mouse(const mouse *m, tab_view *view)
{
    if (!m || !view || !view->tabs || view->view_properties.count <= 0) {
        return 0;
    }

    int handled = 0;

    // Handle mouse for all visible tabs
    for (int i = 0; i < view->view_properties.count; i++) {
        if (view->tabs[i].visible && view->tabs[i].enabled) {
            if (complex_button_handle_mouse(m, &view->tabs[i].button)) {
                handled = 1;
            }
        }
    }

    return handled;
}

int tab_view_get_active_tab(const tab_view *view)
{
    if (!view || view->view_properties.count <= 0) {
        return -1;
    }
    return view->view_properties.active_tab;
}

void tab_view_set_active_tab(tab_view *view, int tab_index)
{
    if (!view || tab_index < 0 || tab_index >= view->view_properties.count) {
        return;
    }
    if (view->view_properties.active_tab != tab_index) {
        view->view_properties.active_tab = tab_index;
        tab_view_layout(view);
        window_request_refresh();
    }
}