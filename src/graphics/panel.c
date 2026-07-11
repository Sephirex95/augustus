#include "panel.h"

#include "assets/assets.h"
#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"

void outer_panel_draw(int x, int y, int width_blocks, int height_blocks)
{
    int image_base = image_group(GROUP_DIALOG_BACKGROUND);
    int image_id;
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 11;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 12 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 13 + image_y + image_x++;
                } else {
                    image_id = 23 + image_y;
                }
                y_add = 12;
            } else {
                if (xx == 0) {
                    image_id = 132;
                } else if (xx < width_blocks - 1) {
                    image_id = 133 + image_x++;
                } else {
                    image_id = 143;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy, COLOR_MASK_NONE, SCALE_NONE);
            if (image_x >= 10) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 120) {
            image_y = 0;
        }
    }
}

void outer_panel_draw_colored(int x, int y, int width, int height, color_t color)
{
    int width_blocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int height_blocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int image_base = image_group(GROUP_DIALOG_BACKGROUND);
    int image_id;
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 11;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 12 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 13 + image_y + image_x++;
                } else {
                    image_id = 23 + image_y;
                }
                y_add = 12;
            } else {
                if (xx == 0) {
                    image_id = 132;
                } else if (xx < width_blocks - 1) {
                    image_id = 133 + image_x++;
                } else {
                    image_id = 143;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy, color, SCALE_NONE);
            if (image_x >= 10) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 120) {
            image_y = 0;
        }
    }
}

void unbordered_panel_draw_colored(int x, int y, int width_blocks, int height_blocks, color_t color)
{
    int image_base = image_group(GROUP_DIALOG_BACKGROUND);
    int image_y = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            int image_id = 13 + image_y + image_x++;
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy, color, SCALE_NONE);
            if (image_x >= 10) {
                image_x = 0;
            }
        }
        image_y += 12;
        if (image_y >= 120) {
            image_y = 0;
        }
    }
}

void unbordered_panel_draw(int x, int y, int width_blocks, int height_blocks)
{
    unbordered_panel_draw_colored(x, y, width_blocks, height_blocks, COLOR_MASK_NONE);
}

void unbordered_panel_draw_px(int x, int y, int width_px, int height_px)
{
    graphics_set_clip_rectangle(x, y, width_px, height_px);
    int width_blocks = (width_px + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int height_blocks = (height_px + BLOCK_SIZE - 1) / BLOCK_SIZE;
    unbordered_panel_draw(x, y, width_blocks, height_blocks);
    graphics_reset_clip_rectangle();
}

void bordered_panel_draw_colored(int x, int y, int width_px, int height_px, int has_focus, color_t color_bg, color_t color_border)
{
    if (width_px <= 0 || height_px <= 0) {
        return;
    }

    graphics_set_clip_rectangle(x, y, width_px, height_px);
    int width_blocks = (width_px + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int height_blocks = (height_px + BLOCK_SIZE - 1) / BLOCK_SIZE;
    unbordered_panel_draw_colored(x, y, width_blocks, height_blocks, color_bg);
    graphics_reset_clip_rectangle();
    button_border_draw_colored(x, y, width_px, height_px, has_focus, color_border);
}

void inner_panel_draw(int x, int y, int width_blocks, int height_blocks)
{
    int image_base = image_group(GROUP_SUNKEN_TEXTBOX_BACKGROUND);
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            int image_id;
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 6;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 7 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 8 + image_y + image_x++;
                } else {
                    image_id = 13 + image_y;
                }
                y_add = 7;
            } else {
                if (xx == 0) {
                    image_id = 42;
                } else if (xx < width_blocks - 1) {
                    image_id = 43 + image_x++;
                } else {
                    image_id = 48;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy, COLOR_MASK_NONE, SCALE_NONE);
            if (image_x >= 5) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 35) {
            image_y = 0;
        }
    }
}

void inner_panel_draw_colored(int x, int y, int width, int height, color_t color)
{
    int width_blocks = (width + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int height_blocks = (height + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int image_base = image_group(GROUP_SUNKEN_TEXTBOX_BACKGROUND);
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            int image_id;
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 6;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 7 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 8 + image_y + image_x++;
                } else {
                    image_id = 13 + image_y;
                }
                y_add = 7;
            } else {
                if (xx == 0) {
                    image_id = 42;
                } else if (xx < width_blocks - 1) {
                    image_id = 43 + image_x++;
                } else {
                    image_id = 48;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy, color, SCALE_NONE);
            if (image_x >= 5) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 35) {
            image_y = 0;
        }
    }
}

void label_draw(int x, int y, int width_blocks, int type)
{
    int image_base = image_group(GROUP_PANEL_BUTTON);
    for (int i = 0; i < width_blocks; i++) {
        int image_id;
        if (i == 0) {
            image_id = 3 * type + 40;
        } else if (i < width_blocks - 1) {
            image_id = 3 * type + 41;
        } else {
            image_id = 3 * type + 42;
        }
        image_draw(image_base + image_id, x + BLOCK_SIZE * i, y, COLOR_MASK_NONE, SCALE_NONE);
    }
}

void large_label_draw(int x, int y, int width_blocks, int type)
{
    int image_base = image_group(GROUP_PANEL_BUTTON);
    for (int i = 0; i < width_blocks; i++) {
        int image_id;
        if (i == 0) {
            image_id = 3 * type;
        } else if (i < width_blocks - 1) {
            image_id = 3 * type + 1;
        } else {
            image_id = 3 * type + 2;
        }
        image_draw(image_base + image_id, x + BLOCK_SIZE * i, y, COLOR_MASK_NONE, SCALE_NONE);
    }
}

void large_label_draw_custom_size(int x, int y, int width, int height)
{
    if (width < 32 || height < 16) {
        return;
    }
    large_label_draw_bg(x, y, width, height);
    large_label_draw_border(x, y, width, height);
}

void large_label_draw_bg(int x, int y, int width, int height)
{
    graphics_set_clip_rectangle(x, y, width, height);
    const int panel_width_left = 13;
    const int panel_width_middle = 16;
    const int panel_height = 19;

    int panel_base = assets_lookup_image_id(ASSET_UI_BTN_MENU_LEFT_PANEL);
    int panel_mirror_base = assets_lookup_image_id(ASSET_UI_BTN_MENU_LEFT_PANEL_MIRROR_V);

    int panel_rows = (height + panel_height - 1) / panel_height;
    int panel_middle_blocks = (width - 2 * panel_width_left + panel_width_middle - 1) / panel_width_middle;


    // Draw normal panel rows.
    for (int i = 0; i < panel_rows; i++) {
        int row_y = y + i * panel_height;

        image_draw(panel_base, x, row_y, COLOR_MASK_NONE, SCALE_NONE);
        for (int j = 0; j < panel_middle_blocks; j++) {
            image_draw(panel_base + 1, x + panel_width_left + j * panel_width_middle, row_y, COLOR_MASK_NONE, SCALE_NONE);
        }
        image_draw(panel_base + 2, x + width - panel_width_left, row_y, COLOR_MASK_NONE, SCALE_NONE);
    }

    // Draw a mirrored half-opacity row across each seam.
    for (int i = 1; i < panel_rows; i++) {
        int row_y = y + i * panel_height - panel_height / 2;

        image_draw(panel_mirror_base, x, row_y, COLOR_MASK_50_OPACITY, SCALE_NONE);
        for (int j = 0; j < panel_middle_blocks; j++) {
            image_draw(panel_mirror_base + 1, x + panel_width_left + j * panel_width_middle, row_y, COLOR_MASK_50_OPACITY, SCALE_NONE);
        }
        image_draw(panel_mirror_base + 2, x + width - panel_width_left, row_y, COLOR_MASK_50_OPACITY, SCALE_NONE);
    }
    graphics_reset_clip_rectangle();
}

void large_label_draw_border(int x, int y, int width, int height)
{
    graphics_set_clip_rectangle(x, y, width, height);
    const int frame_size = 16;
    int frame_base = assets_lookup_image_id(ASSET_UI_BTN_MENU_FRAME_01);

    int horizontal_blocks = (width - 2 * frame_size + frame_size - 1) / frame_size;
    int vertical_blocks = (height - 2 * frame_size + frame_size - 1) / frame_size;

    // Top
    image_draw(frame_base, x, y, COLOR_MASK_NONE, SCALE_NONE); // left
    for (int i = 0; i < horizontal_blocks; i++) {
        image_draw(frame_base + 1, x + frame_size + i * frame_size, y, COLOR_MASK_NONE, SCALE_NONE); // mid
    }
    image_draw(frame_base + 2, x + width - frame_size, y, COLOR_MASK_NONE, SCALE_NONE); // right

    // Sides
    for (int i = 0; i < vertical_blocks; i++) {
        int frame_y = y + frame_size + i * frame_size;
        image_draw(frame_base + 3, x, frame_y, COLOR_MASK_NONE, SCALE_NONE); // left
        image_draw(frame_base + 4, x + width - frame_size, frame_y, COLOR_MASK_NONE, SCALE_NONE); // right
    }

    // Bottom
    image_draw(frame_base + 5, x, y + height - frame_size, COLOR_MASK_NONE, SCALE_NONE); // left
    for (int i = 0; i < horizontal_blocks; i++) {
        image_draw(frame_base + 6, x + frame_size + i * frame_size, y + height - frame_size, COLOR_MASK_NONE, SCALE_NONE); // mid
    }
    image_draw(frame_base + 7, x + width - frame_size, y + height - frame_size, COLOR_MASK_NONE, SCALE_NONE); // right

    graphics_reset_clip_rectangle();
}

int top_menu_black_panel_draw(int x, int y, int width)
{
    int blocks = ((width + BLACK_PANEL_BLOCK_WIDTH - 1) / BLACK_PANEL_BLOCK_WIDTH) - 2;
    if (blocks < BLACK_PANEL_MIDDLE_BLOCKS) {
        blocks = BLACK_PANEL_MIDDLE_BLOCKS;
    }
    int actual_width = (blocks + 2) * BLACK_PANEL_BLOCK_WIDTH;

    image_draw(image_group(GROUP_TOP_MENU) + 14, x, y, COLOR_MASK_NONE, SCALE_NONE);
    x += BLACK_PANEL_BLOCK_WIDTH;

    int black_panel_base_id = assets_get_image_id("UI", "Top_UI_Panel");

    for (int i = 0; i < blocks; i++) {
        image_draw(black_panel_base_id + (i % BLACK_PANEL_MIDDLE_BLOCKS) + 1, x, y,
            COLOR_MASK_NONE, SCALE_NONE);
        x += BLACK_PANEL_BLOCK_WIDTH;
    }

    image_draw(black_panel_base_id + 5, x, y, COLOR_MASK_NONE, SCALE_NONE);

    return actual_width;
}
