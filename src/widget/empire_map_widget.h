#ifndef WIDGET_EMPIRE_MAP_WIDGET_H
#define WIDGET_EMPIRE_MAP_WIDGET_H

#include "graphics/tooltip.h"
#include "input/mouse.h"

int widget_empire_map_widget_width_for_available(int available_width);

void widget_empire_map_initialise(int x, int y, int width);

void widget_empire_map_widget_draw(int offset_x, int offset_y);

int widget_empire_map_widget_handle_mouse(const mouse *m, int offset_x, int offset_y);

int widget_empire_map_widget_handle_tooltip(tooltip_context *c);

int widget_empire_map_widget_width(void);

int widget_empire_map_widget_height(void);

#endif // WIDGET_EMPIRE_MAP_WIDGET_H
