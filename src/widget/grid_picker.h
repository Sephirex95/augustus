#ifndef WIDGET_GRID_PICKER_H
#define WIDGET_GRID_PICKER_H

#include "graphics/complex_button.h"

#define GRID_PICKER_MAX_ROWS 8
#define GRID_PICKER_MAX_COLUMNS 8
#define GRID_PICKER_MAX_OPTIONS (GRID_PICKER_MAX_ROWS * GRID_PICKER_MAX_COLUMNS) 
// can expand in the future, but realistically would need a scrollbar

typedef enum {
    GRID_PICKER_STYLE_DEFAULT,          // Basic: white/red border, default plain background fill
    GRID_PICKER_STYLE_GRAY,          // main-menu-like style
} grid_picker_style;

typedef struct {
    int index;
    lang_fragment *sequence;
    int x; // all cells have the same size, so no need for width/height here, just the position
    int y;
    int image; // if specified, will be drawn INSTEAD of text
    int image_x_offset;
    int image_y_offset;
    int image_before; // standard before-sequence image
    int image_after; // standard after-sequence image
    tooltip_context tooltip_c;
} grid_picker_cell;

typedef struct grid_picker {
    grid_picker_cell cells[GRID_PICKER_MAX_ROWS][GRID_PICKER_MAX_COLUMNS];
    unsigned int cell_count;
    int columns;
    int rows;
    int cell_width;
    int cell_height;
    int spacing_v;
    int spacing_h;
    int selected_index;
    int hovered_index;
    int picker_x_offset; // for drawing the picker relative to the anchor. Default is centered
    int picker_y_offset;
    int margin; // margin between the grid and border of the picker. default is 10px
    void (*selected_callback)(struct grid_picker *picker);
    void (*hover_callback)(struct grid_picker *picker);
    int is_expanded;
    complex_button anchor;
    grid_picker_style style;

    // geometry cache:
    int calculated_x;
    int calculated_y;
    int calculated_width;
    int calculated_height;
    int grid_x;
    int grid_y;
    int grid_width;
    int grid_height;

} grid_picker;

void grid_picker_cells_init(int count, grid_picker_cell *cells, int *images, lang_fragment *sequence);

void grid_picker_anchor_init(complex_button *anchor, int x, int y, int width, int height, const lang_fragment *sequence, int sequence_size);

void grid_picker_init(complex_button *anchor, grid_picker *picker, const grid_picker_cell *cells, unsigned int cell_count,
     int columns, int rows, int cell_width, int cell_height, int spacing);
void grid_picker_refresh_geometry(grid_picker *picker);
// call this if you change the anchor position or size, or the picker offsets

void grid_picker_draw(grid_picker *picker);
int grid_picker_handle_mouse(grid_picker *picker, const mouse *m);
int grid_picker_handle_tooltip(grid_picker *picker, tooltip_context *c);

#endif // WIDGET_GRID_PICKER_H
