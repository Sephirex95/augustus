#include "tab_view.h"

#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "input/mouse.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

tab_view tab_view_init_simple(int x, int y, int width, int height, int tab_count, tab_view_style style)
{
    tab_view view = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .tab_properties.style = style,
        .tab_properties.position = TAB_POS_CENTER, // default position
        .tab_properties.count = tab_count,
        .tab_properties.spread = TAB_SPREAD_SMALL, // default spread
        .tabs = calloc(tab_count, sizeof(complex_button))
    };
    return view;
}

void tab_view_set_tab_text(tab_view *view, int tab_index, const lang_fragment *frag)
{
    if (tab_index < 0 || tab_index >= view->tab_properties.count) {
        return; // out of bounds
    }
    view->tabs[tab_index].sequence = frag;
}

