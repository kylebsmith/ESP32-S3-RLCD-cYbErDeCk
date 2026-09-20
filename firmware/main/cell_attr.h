/*
 * How a cell's attributes compose. One function, two callers.
 *
 * The cursor is a solid inverse block; the sequencer playhead is a bar across
 * the bottom of the cell. They are independent bits, so a cursor sitting on
 * the playhead must show BOTH - the bar inverts back out of the block.
 *
 * WHY THIS IS A FUNCTION AND NOT AN EXPRESSION. It was an expression, written
 * twice: once in editor_draw and once in editor_blink. The blink copy dropped
 * the playhead bit, so every time the cursor blinked on a cell the playhead
 * was passing through, THE BAR WAS ERASED FROM THAT CELL. The owner's report
 * was "it's very easy to lose your cursor selection thing" while editing a
 * running lane - which is exactly and only the cell where the two meet.
 *
 * Two call sites composing the same two flags is two chances to disagree, and
 * the disagreement is invisible in code review because each line is correct
 * on its own. One function, and the host check binds to it.
 */
#ifndef CELL_ATTR_H
#define CELL_ATTR_H

#include <stdbool.h>

#include "textgrid.h"

static inline int cell_attr(bool inverse, bool under)
{
    return (inverse ? TG_INVERSE : TG_NORMAL) | (under ? TG_UNDER : 0);
}

#endif /* CELL_ATTR_H */
