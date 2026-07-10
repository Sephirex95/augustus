#include "empire_sidebar_sort.h"

#include "empire/city.h"
#include "empire/trade_prices.h"
#include "empire/trade_route.h"
#include "game/resource.h"
#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "core/string.h"
#include "input/mouse.h"
#include "translation/translation.h"

#include <string.h>

#define NO_POSITION ((unsigned int) -1)
#define WIDTH_BORDER 16

/* Next time start here:
* In dropdown button/complex button styles:
    *add a style for the inner_panel draw as the background - done
    *add a style that mimicks the main menu buttons - done
* Remove the sorting button draw and replace it completely with a standard dropdown button.
* Dropdown button for sorting:
    * icon before text
    * if sorting selected, leave only selected text + icon before, no 'Sort by' text
* Move arrow button to it's own border, next to the sorting dropdown. Use the new cutout arrow asset.
* Filter section - draw the static filter icon and border for the entire section.
* In filter section:
    * Open/closed cycling button - X / Y / Nothing + tooltip
    * Sea/Land cycling button - just add tooltip
    * Resource picker - dropdown as placeholder,
    *       > think about a grid selector for the future - could be used with other things, like roadblock styles
    * Sell/Buy/Any - cycling or dropdown? test both.
* Still need to make a history asset for the dropdown - maybe not? maybe use the date plain: 'AD 122' with no anchor
* Trade History - just goes on a small button in the bottom corner?
* Should also fit RESET button somewhere :
*   > add a hover on the filter icon that overlays a small red X over it- when pressed, reset filters
*/


// Forward declaration of sidebar_city_entry structure
typedef struct {
    int sidebar_item_id;
    int empire_object_id;
    int city_id;
    int x, y;
    int width;
    int height;
} sidebar_city_entry;

// Static variables for sorting and filtering state
static struct {
    sort_method current_sorting;
    filter_method current_filtering;
    resource_type selected_filter_resource;
    int hovered_sorting_button;
    int sorting_reversed;
    int expanded_main;
} sort_data = {
    .current_sorting = SORT_BY_NAME,
    .current_filtering = FILTER_NONE,
    .selected_filter_resource = RESOURCE_NONE,
    .hovered_sorting_button = NO_POSITION,
    .sorting_reversed = 0,
    .expanded_main = -1,
};

// Arrow button info structure
typedef struct {
    int x, y, width, height;
    int is_down;
} arrow_button_info;

static arrow_button_info sorting_arrow_button;
static int sorting_arrow_focused = 0;


int window_empire_sidebar_sort_count_trade_resources(const empire_city *city, int is_sell)
{
    int count = 0;
    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (resource_is_storable(r)) {
            if ((is_sell && city->sells_resource[r]) ||
                (!is_sell && city->buys_resource[r])) {
                count++;
            }
        }
    }
    return count;
}

static int get_city_trade_quota_fill(const empire_city *city, int is_sell)
{
    int total_now = 0;
    int total_max = 0;

    for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (!resource_is_storable(r)) continue;

        if ((is_sell && !city->sells_resource[r]) || (!is_sell && !city->buys_resource[r])) continue;

        int max = trade_route_limit(city->route_id, r, !is_sell);
        int now = trade_route_traded(city->route_id, r, !is_sell);

        total_max += max;
        total_now += now;
    }

    if (total_max == 0) return 0;
    return (100 * total_now) / total_max;
}

// Sorting buttons state
static sorting_button sorting_buttons[MAX_SORTING_BUTTONS];
static int sorting_button_count = 0;

// External helper functions that need to be provided by the empire window


// Initialization
void window_empire_sidebar_sort_init(void)
{
    sort_data.current_sorting = SORT_BY_NAME;
    sort_data.current_filtering = FILTER_NONE;
    sort_data.selected_filter_resource = RESOURCE_NONE;
    sort_data.hovered_sorting_button = NO_POSITION;
    sort_data.sorting_reversed = 0;
    sort_data.expanded_main = -1;
    sorting_button_count = 0;
}

// Getter functions
int window_empire_sidebar_sort_get_current_sorting(void) { return sort_data.current_sorting; }
int window_empire_sidebar_sort_get_current_filtering(void) { return sort_data.current_filtering; }
resource_type window_empire_sidebar_sort_get_selected_filter_resource(void) { return sort_data.selected_filter_resource; }
int window_empire_sidebar_sort_get_hovered_sorting_button(void) { return sort_data.hovered_sorting_button; }
int window_empire_sidebar_sort_get_sorting_reversed(void) { return sort_data.sorting_reversed; }
int window_empire_sidebar_sort_get_expanded_main(void) { return sort_data.expanded_main; }

// Setter functions
void window_empire_sidebar_sort_set_current_sorting(int sorting) { sort_data.current_sorting = sorting; }
void window_empire_sidebar_sort_set_current_filtering(int filtering) { sort_data.current_filtering = filtering; }
void window_empire_sidebar_sort_set_selected_filter_resource(resource_type resource) { sort_data.selected_filter_resource = resource; }
void window_empire_sidebar_sort_set_hovered_sorting_button(int button) { sort_data.hovered_sorting_button = button; }
void window_empire_sidebar_sort_set_sorting_reversed(int reversed) { sort_data.sorting_reversed = reversed; }
void window_empire_sidebar_sort_set_expanded_main(int expanded) { sort_data.expanded_main = expanded; }

// Reset functions
void window_empire_sidebar_sort_reset_hovered_sorting_button(void) { sort_data.hovered_sorting_button = NO_POSITION; }
void window_empire_sidebar_sort_reset_sorting_button_count(void) { sorting_button_count = 0; }

// Button management
int window_empire_sidebar_sort_get_sorting_button_count(void) { return sorting_button_count; }
const sorting_button *window_empire_sidebar_sort_get_sorting_button(int index)
{
    if (index < 0 || index >= sorting_button_count) return 0;
    return &sorting_buttons[index];
}

void window_empire_sidebar_sort_register_sorting_button(int x, int y, int width, int height, int button_type)
{
    if (sorting_button_count >= MAX_SORTING_BUTTONS) return;
    sorting_buttons[sorting_button_count++] = (sorting_button) { x, y, width, height, button_type };
}

int window_empire_sidebar_sort_sidebar_city_sorter(const void *a, const void *b)
{
    const sidebar_city_entry *entry_a = (const sidebar_city_entry *) a;
    const sidebar_city_entry *entry_b = (const sidebar_city_entry *) b;

    const empire_city *city_a = empire_city_get(entry_a->city_id);
    const empire_city *city_b = empire_city_get(entry_b->city_id);

    // Add null pointer checks to prevent crashes
    if (!city_a || !city_b) {
        // If one is null and the other isn't, put the null one at the end
        if (!city_a && !city_b) return 0;
        if (!city_a) return 1;
        if (!city_b) return -1;
    }

    int result = 0;

    switch (sort_data.current_sorting) {
        case SORT_BY_NAME:
        {
            const char *name_a = (const char *) empire_city_get_name(city_a);
            const char *name_b = (const char *) empire_city_get_name(city_b);
            result = strcmp(name_a, name_b);
            break;
        }

        case SORT_BY_QUOTA_FILL_EXPORT:
        case SORT_BY_QUOTA_FILL_IMPORT:
        {
            int is_sell = (sort_data.current_sorting == SORT_BY_QUOTA_FILL_IMPORT);
            int quota_a = get_city_trade_quota_fill(city_a, is_sell);
            int quota_b = get_city_trade_quota_fill(city_b, is_sell);
            result = (quota_a > quota_b) - (quota_a < quota_b);
            break;
        }

        case SORT_BY_ROUTE_COST:
        {
            int cost_a = city_a->cost_to_open;
            int cost_b = city_b->cost_to_open;
            result = (cost_a > cost_b) - (cost_a < cost_b);
            break;
        }

        case SORT_BY_PROFIT:
        {
            int profit_a = 0;
            int profit_b = 0;

            for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
                if (!resource_is_storable(r)) continue;

                if (city_a->sells_resource[r]) {
                    int amount = trade_route_traded(city_a->route_id, r, 0);
                    int price = trade_price_sell(r, !city_a->is_sea_trade);
                    profit_a += amount * price;
                }
                if (city_a->buys_resource[r]) {
                    int amount = trade_route_traded(city_a->route_id, r, 1);
                    int price = trade_price_buy(r, !city_a->is_sea_trade);
                    profit_a -= amount * price;
                }

                if (city_b->sells_resource[r]) {
                    int amount = trade_route_traded(city_b->route_id, r, 0);
                    int price = trade_price_sell(r, !city_b->is_sea_trade);
                    profit_b += amount * price;
                }
                if (city_b->buys_resource[r]) {
                    int amount = trade_route_traded(city_b->route_id, r, 1);
                    int price = trade_price_buy(r, !city_b->is_sea_trade);
                    profit_b -= amount * price;
                }
            }

            result = (profit_a > profit_b) - (profit_a < profit_b);
            break;
        }

        default:
            break;
    }

    if (sort_data.sorting_reversed)
        result = -result;

    return result;
}

int window_empire_sidebar_sort_city_matches_current_filter(const empire_city *city)
{
    if (!city) {
        return 0; // Null cities don't match any filter
    }

    filter_method filters = sort_data.current_filtering;
    if (filters == FILTER_NONE) {
        return 1;
    }

    if ((filters & FILTER_BY_OPEN) && !city->is_open) {
        return 0;
    }
    if ((filters & FILTER_BY_CLOSED) && city->is_open) {
        return 0;
    }
    if ((filters & FILTER_BY_LAND) && city->is_sea_trade) {
        return 0;
    }
    if ((filters & FILTER_BY_SEA) && !city->is_sea_trade) {
        return 0;
    }

    if (filters & (FILTER_BY_RESOURCE | FILTER_BY_RESOURCE_SELL | FILTER_BY_RESOURCE_BUY)) {
        int matches_resource = 0;

        for (resource_type r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (sort_data.selected_filter_resource != r) {
                continue;
            }
            if ((filters & FILTER_BY_RESOURCE) && (city->buys_resource[r] || city->sells_resource[r])) {
                matches_resource = 1;
            }
            if ((filters & FILTER_BY_RESOURCE_SELL) && city->sells_resource[r]) {
                matches_resource = 1;
            }
            if ((filters & FILTER_BY_RESOURCE_BUY) && city->buys_resource[r]) {
                matches_resource = 1;
            }
            break;
        }

        if (!matches_resource) {
            return 0;
        }
    }

    return 1;
}

void window_empire_sidebar_sort_draw_simple_button(int x, int y, int width, int height, int is_focused, int group1, int number1,
     int group2, int number2, int button_type, int image_id)
{
    graphics_set_clip_rectangle(x, y, width, height);
    int height_blocks = height / BLOCK_SIZE;
    //unbordered_panel_draw(x, y, width / BLOCK_SIZE + 1, height_blocks);
    inner_panel_draw(x, y, width / BLOCK_SIZE + 1, height_blocks);
    graphics_reset_clip_rectangle();
    int margin = 8;
    button_border_draw(x, y, width, height, is_focused);
    int font_height = font_definition_for(FONT_NORMAL_GREEN)->line_height;
    int y_text_offset = y + (height / 2) - (font_height / 2);
    int cursor_x = 0, text_x = 0, available_width = 0, image_width = 0, content_width = 0;
    if (image_id > 0) {
        const image *img = image_get(image_id);
        image_width = img->width;
    }
    // Calculate total text width for centering and account for image width if present
    int text1_width = lang_text_get_width(group1, number1, FONT_NORMAL_GREEN);
    int text2_width = (number2 >= 0) ? lang_text_get_width(group2, number2, FONT_NORMAL_GREEN) : 0;
    int total_text_width = text1_width + text2_width;
    available_width = width - 2 * margin;
    content_width = total_text_width + image_width;
    text_x = x + margin + (available_width - content_width) / 2; // Center horizontally
    cursor_x = text_x + lang_text_draw(group1, number1, text_x, y_text_offset, FONT_NORMAL_GREEN);
    if (number2 >= 0) {
        cursor_x += lang_text_draw(group2, number2, cursor_x, y_text_offset, FONT_NORMAL_GREEN);
    }
    if (image_id > 0) {
        const image *img = image_get(image_id);
        int img_y_offset = y + (height - img->height) / 2;
        image_draw(image_id, cursor_x + 4, img_y_offset, COLOR_MASK_NONE, SCALE_NONE); // 4px spacing
    }

    window_empire_sidebar_sort_register_sorting_button(x, y, width, height, button_type);
}

void window_empire_sidebar_sort_draw_sorting_arrow_button(int button_x, int button_y, int button_width, int button_height)
{
    int margin = 10; // Margin from right edge to keep arrow fully inside
    sorting_arrow_button.is_down = window_empire_sidebar_sort_get_sorting_reversed() ? 0 : 1; // Down when not reversed
    int image_id = sorting_arrow_button.is_down ? 17 : 15; // 17 for down, 15 for up

    // Update arrow button info for hit detection
    sorting_arrow_button.x = button_x + button_width - image_get(image_id)->width - margin;
    sorting_arrow_button.y = button_y + (button_height - image_get(image_id)->height) / 2; // Center vertically
    sorting_arrow_button.width = image_get(image_id)->width;
    sorting_arrow_button.height = image_get(image_id)->height;
    if (sorting_arrow_focused) {
        button_border_draw(sorting_arrow_button.x - 3, sorting_arrow_button.y - 3,
             sorting_arrow_button.width + 5, sorting_arrow_button.height + 5, sorting_arrow_focused);
        // -3 + 5 to account for the 1px innate border of the button 
    }
    image_draw(image_id, sorting_arrow_button.x, sorting_arrow_button.y, COLOR_MASK_NONE, SCALE_NONE);
}

void window_empire_sidebar_sort_draw_expanding_buttons(int sidebar_x_min, int sidebar_y_min, int sidebar_width, int has_scrollbar)
{
    int button_height = 2 * BLOCK_SIZE;
    int v_margin = 4; // 4px universal vertical margin 
    int button_v_spacing = button_height + v_margin; // 4px standard spacing
    int button_h_spacing = 10; // 10px horizontal spacing between buttons
    int available_width = sidebar_width - (has_scrollbar ? 4 * BLOCK_SIZE : WIDTH_BORDER); // 4 *blocksize for scrollbar
    int button_width = (available_width - 2 * button_h_spacing) / 3; // Three sections for buttons
    int base_x = sidebar_x_min + button_h_spacing;
    int base_y = sidebar_y_min + v_margin; // small margin from the top

    // Sort main button
    int x_sort = base_x;
    window_empire_sidebar_sort_reset_sorting_button_count(); // Reset count for sorting buttons

    // Sort main button with current selection displayed
    window_empire_sidebar_sort_draw_simple_button(x_sort, base_y, button_width, button_height,
        window_empire_sidebar_sort_get_hovered_sorting_button() == BUTTON_INDEX_SORT_MAIN && !sorting_arrow_focused,
        CUSTOM_TRANSLATION, TR_EMPIRE_SIDE_BAR_SORT, // Base text: "Sort by:"
        CUSTOM_TRANSLATION, TR_EMPIRE_SIDE_BAR_SORT_BY_NAME + window_empire_sidebar_sort_get_current_sorting(), 0, 0);
    window_empire_sidebar_sort_draw_sorting_arrow_button(x_sort, base_y, button_width, button_height);

    if (window_empire_sidebar_sort_get_expanded_main() == 0) {
        for (int i = 0; i < MAX_SORTING_KEY; ++i) {
            int button_type = BUTTON_INDEX_FIRST_SORT_METHOD + i;  // Children start at 2
            int y = base_y + v_margin + button_height + i * button_v_spacing;
            window_empire_sidebar_sort_draw_simple_button(x_sort, y, button_width, button_height,
                window_empire_sidebar_sort_get_hovered_sorting_button() == button_type, //hovered state
                CUSTOM_TRANSLATION, TR_EMPIRE_SIDE_BAR_SORT_BY_NAME + i, -1, -1, button_type, 0);
        }
    }
}

int window_empire_sidebar_sort_handle_expanding_buttons_input(const mouse *m)
{
    window_empire_sidebar_sort_set_hovered_sorting_button(NO_POSITION); // Reset hovered button
    sorting_arrow_focused = 0; // Reset arrow focus

    // Check if mouse is over the arrow button first (highest priority)
    if (m->x >= sorting_arrow_button.x && m->x < sorting_arrow_button.x + sorting_arrow_button.width &&
        m->y >= sorting_arrow_button.y && m->y < sorting_arrow_button.y + sorting_arrow_button.height) {
        sorting_arrow_focused = 1; // Take focus away from main button
        if (m->left.went_up) {
            // Toggle sorting order
            window_empire_sidebar_sort_set_sorting_reversed(!window_empire_sidebar_sort_get_sorting_reversed());
            return 1; // Block further input
        }
        return 1; // Block further input when hovering over arrow
    }

    // If right-clicked and something is expanded: collapse it and exit
    if (m->right.went_up && window_empire_sidebar_sort_get_expanded_main() != NO_POSITION) {
        window_empire_sidebar_sort_set_expanded_main(NO_POSITION);
        return 1; // Block further input
    }

    for (int i = 0; i < window_empire_sidebar_sort_get_sorting_button_count(); ++i) {
        const sorting_button *btn = window_empire_sidebar_sort_get_sorting_button(i);

        if (m->x >= btn->x && m->x < btn->x + btn->width &&
            m->y >= btn->y && m->y < btn->y + btn->height) {
            window_empire_sidebar_sort_set_hovered_sorting_button(btn->button_type);
            // Only handle left clicks here
            if (m->left.went_up) {
                if (btn->button_type == BUTTON_INDEX_SORT_MAIN) {
                    window_empire_sidebar_sort_set_expanded_main((window_empire_sidebar_sort_get_expanded_main() == BUTTON_INDEX_SORT_MAIN) ?
                     NO_POSITION : BUTTON_INDEX_SORT_MAIN);
                    return 1;
                } else if (btn->button_type >= BUTTON_INDEX_FIRST_SORT_METHOD &&
                           btn->button_type < BUTTON_INDEX_FIRST_SORT_METHOD + MAX_SORTING_KEY) {
                    window_empire_sidebar_sort_set_current_sorting(btn->button_type - BUTTON_INDEX_FIRST_SORT_METHOD);
                    window_empire_sidebar_sort_set_expanded_main(NO_POSITION);
                    return 1;
                }
                window_empire_sidebar_sort_set_expanded_main(NO_POSITION);
                return 1; //clicked away
            }
            break;
        }
    }
    if (m->left.went_up && window_empire_sidebar_sort_get_expanded_main() != NO_POSITION) {
        // If left-clicked outside any button, collapse the expanded section
        window_empire_sidebar_sort_set_expanded_main(NO_POSITION);
        return 1; // Block further input
    }
    return (window_empire_sidebar_sort_get_expanded_main() != NO_POSITION) ? 1 : 0;
}

int window_empire_sidebar_sort_get_sorting_arrow_focused(void)
{
    return sorting_arrow_focused;
}

int window_empire_sidebar_sort_get_sorting_arrow_is_down(void)
{
    return sorting_arrow_button.is_down;
}
