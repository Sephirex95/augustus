#include "grid_picker.h"

#include "graphics/button.h"
#include "graphics/complex_button.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include <string.h>

static int debug_shader = 2;

static void grid_picker_cell_click(const complex_button *button)
{
    grid_picker *picker = (grid_picker *) button->user_data;
    if (!picker) {
        return;
    }

    picker->selected_index = button->parameters[0];
    if (picker->selected_callback) {
        picker->selected_callback(picker);
    }
}

void grid_picker_cells_init(int count, grid_picker_cell *cells, int *images, lang_fragment *sequence)
{
    memset(cells, 0, sizeof(*cells) * count);

    for (int i = 0; i < count; i++) {
        cells[i].index = i;
        cells[i].image = images ? images[i] : -1;
        cells[i].sequence = sequence ? &sequence[i] : NULL;
    }
}

void grid_picker_anchor_init(complex_button *anchor, int x, int y, int width, int height,
    const lang_fragment *sequence, int sequence_size)
{
    if (!anchor) {
        return;
    }

    memset(anchor, 0, sizeof(*anchor));
    anchor->x = x;
    anchor->y = y;
    anchor->width = width;
    anchor->height = height;
    anchor->sequence = sequence;
    anchor->sequence_size = sequence_size;
    anchor->sequence_position = SEQUENCE_POSITION_CENTER;
}

static void grid_picker_geometry(grid_picker *picker)
{
    // calculate the grid width and height:
    picker->grid_width = picker->columns * picker->cell_width + (picker->columns - 1) * picker->spacing_h;
    picker->grid_height = picker->rows * picker->cell_height + (picker->rows - 1) * picker->spacing_v;
    picker->calculated_width = picker->grid_width + 2 * picker->margin;
    picker->calculated_height = picker->grid_height + 2 * picker->margin;

    int anchor_center_x = picker->anchor.x + picker->anchor.width / 2;
    if (picker->picker_y_offset == 0) {
        picker->picker_y_offset = 1; // always move at least 1px down to avoid overlapping the anchor button
    }
    picker->grid_x = anchor_center_x - picker->calculated_width / 2 + picker->picker_x_offset;
    picker->grid_y = picker->anchor.y + picker->anchor.height + picker->picker_y_offset;

    for (unsigned int i = 0; i < picker->cell_count; i++) {
        int row = (int) i / picker->columns;
        int column = (int) i % picker->columns;
        grid_picker_cell *cell = &picker->cells[row][column];

        cell->x = picker->grid_x + picker->margin + column * (picker->cell_width + picker->spacing_h);
        cell->y = picker->grid_y + picker->margin + row * (picker->cell_height + picker->spacing_v);
    }
}
void grid_picker_refresh_geometry(grid_picker *picker)
{
    if (!picker) {
        return;
    }
    grid_picker_geometry(picker);
}

// simple init should set picker_y_offset to like 5 or smth for base
void grid_picker_init(complex_button *anchor, grid_picker *picker, const grid_picker_cell *cells, unsigned int cell_count,
    int columns, int rows, int cell_width, int cell_height, int spacing)
{
    if (!picker || !cells || !anchor || cell_count == 0 || cell_count > GRID_PICKER_MAX_OPTIONS) {
        return;
    }

    if (columns <= 0 || columns > GRID_PICKER_MAX_COLUMNS || rows <= 0 || rows > GRID_PICKER_MAX_ROWS) {
        return;
    }

    if (cell_width <= 0 || cell_height <= 0 || spacing < 0 || cell_count >(unsigned int) (columns * rows)) {
        return;
    }

    memset(picker, 0, sizeof(*picker));

    memcpy(&picker->anchor, anchor, sizeof(picker->anchor));
    // pointer assignment wont work because tooltip_c contains const's - memcpy 

    picker->cell_count = cell_count;
    picker->columns = columns;
    picker->rows = rows;
    picker->cell_width = cell_width;
    picker->cell_height = cell_height;
    picker->spacing_h = spacing;
    picker->spacing_v = spacing;
    picker->selected_index = -1;
    picker->hovered_index = -1;
    picker->margin = 10;

    for (unsigned int i = 0; i < cell_count; i++) {
        int row = (int) i / columns;
        int column = (int) i % columns;
        grid_picker_cell *cell = &picker->cells[row][column];

        memcpy(cell, &cells[i], sizeof(*cell));

        cell->index = (int) i;
        cell->x = column * (cell_width + spacing);
        cell->y = row * (cell_height + spacing);
    }
    grid_picker_geometry(picker);
}

void grid_picker_draw(grid_picker *picker)
{
    if (!picker) {
        return;
    }
    complex_button_draw(&picker->anchor);
    if (!picker->is_expanded) {
        return;
    }
    switch (picker->style) {
        case GRID_PICKER_STYLE_GRAY:
            grid_picker_draw_gray_style(picker);
            break;
        default:
            grid_picker_draw_default_style(picker);
    }
}

static void grid_picker_draw_default_style(grid_picker *picker)
{

    bordered_panel_draw_colored(picker->grid_x, picker->grid_y, picker->calculated_width, picker->calculated_height,
        0, COLOR_MASK_NONE, COLOR_MASK_NONE);

    for (unsigned int i = 0; i < picker->cell_count; i++) {
        int row = (int) i / picker->columns;
        int column = (int) i % picker->columns;
        grid_picker_cell *cell = &picker->cells[row][column];

        int has_focus = cell->index == picker->hovered_index;
        button_border_draw_colored(cell->x, cell->y, picker->cell_width, picker->cell_height, has_focus, 0);

    }
}

static void grid_picker_draw_gray_style(grid_picker *picker)
{

    large_label_draw_custom_size(picker->grid_x, picker->grid_y, picker->calculated_width, picker->calculated_height);

    for (unsigned int i = 0; i < picker->cell_count; i++) {
        int row = (int) i / picker->columns;
        int column = (int) i % picker->columns;
        grid_picker_cell *cell = &picker->cells[row][column];

        int has_focus = cell->index == picker->hovered_index;
        if (cell->image) {
            int x = cell->x + cell->image_x_offset;
            int y = cell->y + cell->image_y_offset;
            image_draw(cell->image, x, y, COLOR_MASK_NONE, SCALE_NONE);
        }
        large_label_draw_border(cell->x, cell->y, picker->cell_width, picker->cell_height);
        if (has_focus) {
            graphics_shade_rect(cell->x, cell->y, picker->cell_width, picker->cell_height, debug_shader);
        }

    }
}

int grid_picker_handle_mouse(grid_picker *picker, const mouse *m)
{
    int handled = 0; // if input is handled, return 1 to stop further input processing
    picker->hovered_index = -1; // reset hovered index each frame, will be set if mouse is over a cell
    complex_button *anchor_btn = &picker->anchor;
    if (complex_button_handle_mouse(anchor_btn, m)) {
        if (anchor_btn->is_clicked) {
            picker->is_expanded = !picker->is_expanded;
            window_request_refresh();
        }
        return 1;
    }
    if (m->right.went_up) { // always close the picker on right click, even if the mouse is outside the picker
        if (picker->is_expanded) {
            picker->is_expanded = 0;
            window_request_refresh();
        }
        return 1;
    }
    if (picker->is_expanded) {
        int inside = (m->x >= picker->grid_x && m->x < picker->grid_x + picker->calculated_width &&
            m->y >= picker->grid_y && m->y < picker->grid_y + picker->calculated_height);
        if (inside) {
            handled = 1;

        }
        for (unsigned int i = 0; i < picker->rows; i++) {
            for (unsigned int j = 0; j < picker->columns; j++) {
                int x = picker->cells[i][j].x;
                int y = picker->cells[i][j].y;
                int inside_cell = (m->x >= x && m->x < x + picker->cell_width &&
                    m->y >= y && m->y < y + picker->cell_height);
                if (inside_cell) {
                    picker->hovered_index = picker->cells[i][j].index;
                    if (m->left.went_up) {
                        picker->selected_index = picker->cells[i][j].index;
                        if (picker->selected_callback) {
                            picker->selected_callback(picker);
                        }
                        picker->is_expanded = 0;
                    }
                    if (picker->hover_callback) {
                        picker->hover_callback(picker);
                    }
                }
            }
        }
    }

    return handled;
}

int grid_picker_handle_tooltip(grid_picker *picker, tooltip_context *c)
{


    return 1;
}
