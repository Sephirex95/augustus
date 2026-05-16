#ifndef GRAPHICS_TAB_VIEW_H
#define GRAPHICS_TAB_VIEW_H

#include "graphics/complex_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "input/mouse.h"

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

/* Sequence positioning from complex button */

typedef struct tab_view {
    int x; // top-left corner of the area,
    int y; // buttons will be placed ABOVE this point
    int width;
    int height;
    struct {
        tab_view_style style;
        tab_position position;
        tab_spread spread;
        int count;
    } tab_properties;
    complex_button *tabs;
} tab_view;

#endif // GRAPHICS_TAB_VIEW_H
