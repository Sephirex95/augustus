#include "text_block.h"

#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/mouse.h"

#include <stddef.h>
#include <string.h>


/*
 * text_block intentionally delegates actual sequence rendering and wrapping
 * to lang_sequence.c.
 *
 * Layout performed here is concerned with:
 *
 * - the text block's content rectangle
 * - horizontal and vertical positioning
 * - clipping
 * - simple decoration
 * - hover state
 * - tooltip handling
 *
 * Multiline sequence measurement is currently approximate because
 * lang_text_draw_sequence_multiline() does not expose a non-drawing
 * measurement function.
 */


static int text_block_content_width(const text_block *block);
static int text_block_content_height(const text_block *block);

static sequence_positioning text_block_position(const text_block *block);

static int position_is_left(sequence_positioning position);
static int position_is_centered(sequence_positioning position);
static int position_is_right(sequence_positioning position);

static int position_is_top(sequence_positioning position);
static int position_is_middle(sequence_positioning position);
static int position_is_bottom(sequence_positioning position);

static int fragment_width(const lang_fragment *fragment, font_t font);

static int estimate_sequence_line_count(const text_block *block);
static int estimate_sequence_height(const text_block *block);
static int estimate_raw_text_height(const text_block *block);

static int get_text_y(const text_block *block, int text_height);
static int get_single_line_x(
    const text_block *block,
    int text_width
);

static void draw_background_and_border(const text_block *block);
static void text_block_draw_sequence(const text_block *block);
static void text_block_draw_raw(const text_block *block);


/* ------------------------------------------------------------------------- */
/* Initialization                                                            */
/* ------------------------------------------------------------------------- */

int widget_text_block_init_simple(
    text_block *block,
    int x,
    int y,
    int width,
    int height,
    const lang_fragment *sequence,
    unsigned short sequence_size,
    sequence_positioning position)
{
    if (!block) {
        return 0;
    }

    memset(block, 0, sizeof(*block));

    block->sequence = sequence;
    block->sequence_size = sequence_size;

    block->position = position ?
        position :
        SEQUENCE_POSITION_CENTER;

    block->x = x;
    block->y = y;
    block->width = width;
    block->height = height;

    block->inner_padding_x = 2;
    block->inner_padding_y = 2;

    block->font = FONT_NORMAL_BLACK;
    block->font_primary = COLOR_MASK_NONE;

    block->raw_text = NULL;

    block->draw_background = 0;
    block->draw_border = 0;

    block->is_disabled = 0;
    block->is_hidden = 0;

    block->state_is_hovered = 0;

    return 1;
}


/* ------------------------------------------------------------------------- */
/* Basic geometry                                                            */
/* ------------------------------------------------------------------------- */

static int text_block_content_width(const text_block *block)
{
    int width =
        block->width -
        2 * block->inner_padding_x;

    return width > 0 ? width : 0;
}


static int text_block_content_height(const text_block *block)
{
    int height =
        block->height -
        2 * block->inner_padding_y;

    return height > 0 ? height : 0;
}


static sequence_positioning text_block_position(const text_block *block)
{
    switch (block->position) {
        case SEQUENCE_POSITION_TOP_LEFT:
        case SEQUENCE_POSITION_TOP_CENTER:
        case SEQUENCE_POSITION_TOP_RIGHT:

        case SEQUENCE_POSITION_CENTER_LEFT:
        case SEQUENCE_POSITION_CENTER:
        case SEQUENCE_POSITION_CENTER_RIGHT:

        case SEQUENCE_POSITION_BOTTOM_LEFT:
        case SEQUENCE_POSITION_BOTTOM_CENTER:
        case SEQUENCE_POSITION_BOTTOM_RIGHT:
            return block->position;

        default:
            return SEQUENCE_POSITION_CENTER;
    }
}


/* ------------------------------------------------------------------------- */
/* Position classification                                                   */
/* ------------------------------------------------------------------------- */

static int position_is_left(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_TOP_LEFT:
        case SEQUENCE_POSITION_CENTER_LEFT:
        case SEQUENCE_POSITION_BOTTOM_LEFT:
            return 1;

        default:
            return 0;
    }
}


static int position_is_centered(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_TOP_CENTER:
        case SEQUENCE_POSITION_CENTER:
        case SEQUENCE_POSITION_BOTTOM_CENTER:
            return 1;

        default:
            return 0;
    }
}


static int position_is_right(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_TOP_RIGHT:
        case SEQUENCE_POSITION_CENTER_RIGHT:
        case SEQUENCE_POSITION_BOTTOM_RIGHT:
            return 1;

        default:
            return 0;
    }
}


static int position_is_top(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_TOP_LEFT:
        case SEQUENCE_POSITION_TOP_CENTER:
        case SEQUENCE_POSITION_TOP_RIGHT:
            return 1;

        default:
            return 0;
    }
}


static int position_is_middle(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_CENTER_LEFT:
        case SEQUENCE_POSITION_CENTER:
        case SEQUENCE_POSITION_CENTER_RIGHT:
            return 1;

        default:
            return 0;
    }
}


static int position_is_bottom(sequence_positioning position)
{
    switch (position) {
        case SEQUENCE_POSITION_BOTTOM_LEFT:
        case SEQUENCE_POSITION_BOTTOM_CENTER:
        case SEQUENCE_POSITION_BOTTOM_RIGHT:
            return 1;

        default:
            return 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Sequence measurement                                                      */
/* ------------------------------------------------------------------------- */

/*
 * Keep this consistent with the fragment-width calculations used by
 * lang_text_draw_sequence_multiline().
 *
 * This helper exists only for layout estimation. Actual rendering remains
 * the responsibility of lang_text.c.
 */
static int fragment_width(const lang_fragment *fragment, font_t font)
{
    if (!fragment) {
        return 0;
    }

    switch (fragment->type) {
        case LANG_FRAG_LABEL:
            return lang_text_get_width(
                fragment->text_group,
                fragment->text_id,
                font);

        case LANG_FRAG_AMOUNT:
            return lang_text_get_amount_width(
                fragment->text_group,
                fragment->text_id,
                fragment->number,
                font);

        case LANG_FRAG_NUMBER:
            return text_get_number_width(
                fragment->number,
                '\0',
                "\0",
                font);

        case LANG_FRAG_TEXT:
            if (!fragment->text) {
                return 0;
            }
            return text_get_width(fragment->text, font);

        case LANG_FRAG_SPACE:
            return fragment->space_width;

        case LANG_FRAG_FLOAT:
            return text_get_number_float_width(
                fragment->float_number,
                fragment->decimal_places,
                '\0',
                "",
                font);

        default:
            return 0;
    }
}


/*
 * Estimate the number of lines that lang_text_draw_sequence_multiline()
 * will require.
 *
 * This intentionally follows the existing renderer at fragment level
 * instead of introducing a separate wrapping system.
 *
 * An individually oversized LABEL or TEXT fragment can internally wrap
 * at word boundaries in text_draw_multiline(). We don't have a pure
 * measurement API for that operation, so this part is necessarily an
 * estimate.
 */
static int estimate_sequence_line_count(const text_block *block)
{
    int width = text_block_content_width(block);

    if (!block ||
        !block->sequence ||
        block->sequence_size == 0 ||
        width <= 0) {
        return 0;
    }

    int line_count = 1;
    int current_width = 0;

    for (unsigned short i = 0; i < block->sequence_size; i++) {
        const lang_fragment *fragment = &block->sequence[i];
        int width_fragment = fragment_width(fragment, block->font);

        /*
         * This mirrors the first wrap performed by
         * lang_text_draw_sequence_multiline().
         */
        if (current_width > 0 &&
            current_width + width_fragment > width) {

            line_count++;
            current_width = 0;
        }

        /*
         * LABEL and TEXT fragments are allowed to use text_draw_multiline()
         * when the fragment itself is too wide.
         *
         * We approximate the additional number of lines here. The actual
         * word-breaking behavior remains owned by text_draw_multiline().
         */
        if (width_fragment > width &&
            (fragment->type == LANG_FRAG_LABEL ||
                fragment->type == LANG_FRAG_TEXT)) {

            int fragment_lines =
                (width_fragment + width - 1) / width;

            if (fragment_lines > 1) {
                line_count += fragment_lines - 1;
            }

            current_width = 0;
            continue;
        }

        current_width += width_fragment;
    }

    return line_count;
}


static int estimate_sequence_height(const text_block *block)
{
    if (!block) {
        return 0;
    }

    int lines = estimate_sequence_line_count(block);

    if (lines <= 0) {
        return 0;
    }

    return lines *
        font_definition_for(block->font)->line_height;
}


static int estimate_raw_text_height(const text_block *block)
{
    if (!block || !block->raw_text) {
        return 0;
    }

    int width = text_block_content_width(block);

    if (width <= 0) {
        return 0;
    }

    int text_width =
        text_get_width(block->raw_text, block->font);

    int line_height =
        font_definition_for(block->font)->line_height;

    if (text_width <= width) {
        return line_height;
    }

    /*
     * This is only used for vertical placement.
     *
     * text_draw_multiline() remains responsible for the real word
     * wrapping during rendering.
     */
    int lines =
        (text_width + width - 1) / width;

    return lines * line_height;
}


/* ------------------------------------------------------------------------- */
/* Position calculation                                                      */
/* ------------------------------------------------------------------------- */

static int get_text_y(
    const text_block *block,
    int text_height)
{
    sequence_positioning position =
        text_block_position(block);

    int content_top =
        block->y + block->inner_padding_y;

    int content_height =
        text_block_content_height(block);

    if (position_is_top(position)) {
        return content_top;
    }

    if (position_is_bottom(position)) {
        return content_top +
            content_height -
            text_height;
    }

    /*
     * Middle alignment is also the fallback for invalid positioning.
     */
    return content_top +
        (content_height - text_height) / 2;
}


static int get_single_line_x(
    const text_block *block,
    int text_width)
{
    sequence_positioning position =
        text_block_position(block);

    int content_left =
        block->x + block->inner_padding_x;

    int content_width =
        text_block_content_width(block);

    if (position_is_left(position)) {
        return content_left;
    }

    if (position_is_right(position)) {
        return content_left +
            content_width -
            text_width;
    }

    return content_left +
        (content_width - text_width) / 2;
}


/* ------------------------------------------------------------------------- */
/* Decoration                                                                */
/* ------------------------------------------------------------------------- */

static void draw_background_and_border(const text_block *block)
{
    if (block->draw_background) {
        unbordered_panel_draw_px(
            block->x,
            block->y,
            block->width,
            block->height);
    }

    if (block->draw_border) {
        button_border_draw(
            block->x,
            block->y,
            block->width,
            block->height,
            0);
    }
}


/* ------------------------------------------------------------------------- */
/* Sequence drawing                                                          */
/* ------------------------------------------------------------------------- */

static void text_block_draw_sequence(const text_block *block)
{
    if (!block->sequence ||
        block->sequence_size == 0) {
        return;
    }

    int content_width =
        text_block_content_width(block);

    if (content_width <= 0) {
        return;
    }

    int content_left =
        block->x + block->inner_padding_x;

    int sequence_width =
        lang_text_get_sequence_width(
            block->sequence,
            block->sequence_size,
            block->font);

    int line_height =
        font_definition_for(block->font)->line_height;

    color_t color = block->font_primary;

    if (block->is_disabled) {
        color = COLOR_FONT_GRAY;
    }

    /*
     * Single-line sequence.
     *
     * This is the case where all nine alignment positions can be honored
     * exactly without needing additional layout information from
     * lang_text.c.
     */
    if (sequence_width <= content_width) {
        int x =
            get_single_line_x(block, sequence_width);

        int y =
            get_text_y(block, line_height);

        lang_text_draw_sequence(
            block->sequence,
            block->sequence_size,
            x,
            y,
            block->font,
            color);

        return;
    }

    /*
     * Multiline sequence.
     *
     * lang_text_draw_sequence_multiline() performs the actual wrapping.
     * We estimate its height only so the complete wrapped region can be
     * vertically positioned.
     */
    int estimated_height =
        estimate_sequence_height(block);

    int y =
        get_text_y(block, estimated_height);

    /*
     * Current lang_text multiline drawing uses one fixed starting X.
     *
     * Consequently, wrapped lines are left-aligned inside their layout
     * region. We can still shift that region based on the requested
     * horizontal position, but we cannot independently center/right-align
     * each wrapped line without extending lang_text.c.
     */
    int x = content_left;

    sequence_positioning position =
        text_block_position(block);

    if (position_is_centered(position)) {
        /*
         * The multiline region occupies the full content width, so its
         * centered origin is also content_left.
         *
         * Kept explicit to make the intended behavior clear.
         */
        x = content_left;
    } else if (position_is_right(position)) {
        /*
         * Same limitation as centered multiline content: the existing API
         * wraps relative to a single left edge.
         */
        x = content_left;
    }

    lang_text_draw_sequence_multiline(
        block->sequence,
        block->sequence_size,
        x,
        y,
        content_width,
        0,
        block->font,
        color);
}


/* ------------------------------------------------------------------------- */
/* Raw text drawing                                                          */
/* ------------------------------------------------------------------------- */

static void text_block_draw_raw(const text_block *block)
{
    if (!block->raw_text) {
        return;
    }

    int content_width =
        text_block_content_width(block);

    if (content_width <= 0) {
        return;
    }

    int text_width =
        text_get_width(
            block->raw_text,
            block->font);

    int line_height =
        font_definition_for(block->font)->line_height;

    color_t color = block->font_primary;

    if (block->is_disabled) {
        color = COLOR_FONT_GRAY;
    }

    /*
     * Use ordinary text drawing when the string fits on one line so the
     * requested horizontal alignment can be honored exactly.
     */
    if (text_width <= content_width) {
        int x =
            get_single_line_x(block, text_width);

        int y =
            get_text_y(block, line_height);

        text_draw(
            block->raw_text,
            x,
            y,
            block->font,
            color);

        return;
    }

    /*
     * As with sequence multiline drawing, text_draw_multiline() owns the
     * actual wrapping.
     */
    int text_height =
        estimate_raw_text_height(block);

    int x =
        block->x + block->inner_padding_x;

    int y =
        get_text_y(block, text_height);

    text_draw_multiline(
        block->raw_text,
        x,
        y,
        content_width,
        0,
        block->font,
        color);
}


/* ------------------------------------------------------------------------- */
/* Main drawing entry point                                                  */
/* ------------------------------------------------------------------------- */

void text_block_draw(const text_block *block)
{
    if (!block ||
        block->is_hidden ||
        block->width <= 0 ||
        block->height <= 0) {
        return;
    }

    /*
     * Clip everything belonging to the block.
     *
     * This is important even though the multiline text APIs perform
     * wrapping: numeric fragments, unusually large fragments, or future
     * content types can still exceed the available area.
     */
    graphics_set_clip_rectangle(
        block->x,
        block->y,
        block->width,
        block->height);

    draw_background_and_border(block);

    /*
     * Sequence content takes precedence when both sequence and raw_text
     * have been supplied. This preserves the behavior of the original
     * implementation.
     */
    if (block->sequence &&
        block->sequence_size > 0) {

        text_block_draw_sequence(block);

    } else if (block->raw_text) {

        text_block_draw_raw(block);
    }

    graphics_reset_clip_rectangle();
}


/* ------------------------------------------------------------------------- */
/* Mouse handling                                                            */
/* ------------------------------------------------------------------------- */

int text_block_handle_mouse(
    text_block *block,
    const mouse *m)
{
    if (!block || !m) {
        return 0;
    }

    if (block->is_hidden) {
        if (block->state_is_hovered) {
            block->state_is_hovered = 0;
            window_request_refresh();
        }

        return 0;
    }

    int inside =
        m->x >= block->x &&
        m->x < block->x + block->width &&
        m->y >= block->y &&
        m->y < block->y + block->height;

    if (block->state_is_hovered != inside) {
        block->state_is_hovered = inside;
        window_request_refresh();
    }

    /*
     * A text block currently has no click handlers and shouldn't consume
     * mouse input simply because the pointer is over it.
     *
     * Return 0 until interaction behavior is added to the structure.
     */
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Tooltip handling                                                          */
/* ------------------------------------------------------------------------- */

int text_block_handle_tooltip(
    const text_block *block,
    tooltip_context *c)
{
    if (!block ||
        !c ||
        block->is_hidden ||
        !block->state_is_hovered) {
        return 0;
    }

    if (tooltip_context_is_empty(&block->tooltip_c)) {
        return 0;
    }

    tooltip_copy_context(
        c,
        &block->tooltip_c);

    return 1;
}
