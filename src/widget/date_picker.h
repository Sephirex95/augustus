#ifndef WIDGET_DATE_PICKER_H
#define WIDGET_DATE_PICKER_H

#include "graphics/complex_button.h"
#include "graphics/lang_sequence.h"

#include <stdint.h>

#define DATE_PICKER_BUTTON_WIDTH 39
#define DATE_PICKER_BUTTON_HEIGHT 24

typedef struct date_picker {
    int x;
    int y;
    int date_field_height;
    int date_field_width;
    int padding;
    int years_back;
    int years_forward;
    int selected_year_offset;
    int changed;
    complex_button buttons[3];
    lang_date_sequence date;
    uint8_t tooltip_text[256];
} date_picker;

typedef enum {
    WIDGET_DATE_PICKER_DATE_FULL,
    WIDGET_DATE_PICKER_DATE_MONTH_YEAR,
    WIDGET_DATE_PICKER_DATE_YEAR
} widget_date_picker_date_format;

void widget_date_picker_init(date_picker *picker, int x, int y, int date_field_height, int date_field_width,
    complex_button_style style, int padding, int years_back, int years_forward);
void widget_date_picker_date_sequence(lang_date_sequence *date, int year, int month, int cosmetic_day, int game_day,
    widget_date_picker_date_format format);
void widget_date_picker_draw(date_picker *picker);
int widget_date_picker_handle_input(date_picker *picker, const mouse *m);
int widget_date_picker_handle_tooltip(date_picker *picker, tooltip_context *c);

#endif // WIDGET_DATE_PICKER_H
