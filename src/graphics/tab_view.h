#ifndef GRAPHICS_TAB_VIEW_H
#define GRAPHICS_TAB_VIEW_H

#include "graphics/complex_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "input/mouse.h"

typedef struct content_area content_area;
typedef struct tab tab;
typedef struct tab_view tab_view;

typedef void (*content_draw_callback)(tab_view *, tab *);

typedef enum {
    TAB_VIEW_STYLE_DEFAULT,       // Basic style: single rectangle with red border and texture fill
    TAB_VIEW_STYLE_DEFAULT_SMALL, // like default but small font and less padding
    TAB_VIEW_STYLE_GRAY,          // main-menu-like style
    TAB_VIEW_STYLE_COLORFUL       // colorful style with gradient background
} tab_view_style;

typedef enum {
    TAB_SPREAD_NONE, // tabs tightly together
    TAB_SPREAD_SMALL,// tabs separated by small margin - 5% of the tab view width
    TAB_SPREAD_WIDE, // tabs spread across the width of the tab view area
    TAB_SPREAD_MAX,  // tabs spread across as much as possible - prioritised over the tab_position parameter
} tab_spread;

typedef enum {
    TAB_POS_LEFT,
    TAB_POS_RIGHT,
    TAB_POS_CENTER,
} tab_position; // indexing starts at 0 on the leftmost tab, regardless of the tab_position

struct content_area {
    int x;
    int y;
    int width;
    int height;
    content_draw_callback draw_callback;
};

struct tab {
    complex_button button;
    int visible;
    int enabled; //to do: disabled but visible - greyed out and unclickable
    content_draw_callback draw_callback;
    void *user_data; // optional extensibility
};

/* Sequence positioning from complex button */

struct tab_view {
    int x;
    int y;
    int width;
    int height;
    int tab_height;  // height of tab buttons

    struct {
        tab_view_style style;
        tab_position position;
        tab_spread spread;
        int count;
        int active_tab;
    } view_properties;

    content_area content;
    tab *tabs;

};

/* Public API */
void tab_view_init_simple(tab_view *view, int x, int y, int width, int height, int tab_count, tab_view_style style);
void tab_view_destroy(tab_view *view);

/* Layout and rendering */
void tab_view_layout(tab_view *view);
void tab_view_draw(tab_view *view);
int tab_view_handle_mouse(const mouse *m, tab_view *view);

/* Tab configuration */
void tab_view_init_tab(tab_view *view, int tab_index, content_draw_callback callback, const lang_fragment *frag);
void tab_view_set_tab_text(tab_view *view, int tab_index, const lang_fragment *frag);
void tab_view_set_tab_draw_callback(tab_view *view, int tab_index, content_draw_callback callback);

/* Accessors */
int tab_view_get_active_tab(const tab_view *view);
void tab_view_set_active_tab(tab_view *view, int tab_index);

#endif // GRAPHICS_TAB_VIEW_H
