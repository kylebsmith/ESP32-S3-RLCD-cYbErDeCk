/*
 * ASCII visuals, as lanes.
 *
 * THE WHOLE IDEA. A visual here is not a second language and not a second
 * environment - it is a lane, exactly like a drum lane, whose destination
 * happens to be a rectangle of characters instead of a synth. So live visual
 * coding is the same Ctrl+Enter on the same line in the same document as the
 * music, and the two can sit four lines apart driving each other.
 *
 * SIX FIELDS, TEN OPERATORS, AND A NEW ONE HAS TO DELETE ITS OWN. Complexity is
 * meant to come from primitives combining, the way a model comes out of solids - so
 * each one is deliberately dumb, and the interesting part is that sixteen lanes drive
 * them at once, at different rates, with probability, routed from each other.
 *
 * A SOURCE IS A FIELD, NOT A SHAPE, and that is the whole of the current design. A
 * field answers "how far is this cell from the thing" in its own geometry and lays
 * the answer down as a tone; a shape is then a field through a THRESHOLD. Two earlier
 * sets were bags of shapes - bar, dot, wave, ring, rain - and a bag of shapes is what
 * a system looks like when nobody has decided what the operations are: a ninth shape
 * buys one more picture and nothing else.
 *
 *   disc   euclidean distance from the point      round
 *   box    chebyshev distance from the point      square - corners disc cannot have
 *   turn   the ANGLE around the point            amount is how much of the circle
 *   ramp   distance along one axis                linear
 *   grid   distance to the nearest lattice line   periodic
 *   noise  no geometry at all                     the entropy, irreducible
 *
 *   mask   keep what is at least this bright      a LEVEL through the field
 *   edge   keep where the field changes fast      a CONTOUR of it
 *
 * So a ring is 'disc edge', a rectangle outline is 'box edge', spokes are 'turn
 * edge', and a rotating radar sweep with a fading tail is 'turn spin echo' - which
 * the shape sets could not make at all. See docs/MAP.md section 9.5 for what 'star',
 * 'shake' and 'tile' were traded for and why the earlier refusal of 'edge' was wrong.
 *
 * TABLE ORDER IS THE DEFAULT DRAW ORDER AND 'route' OVERRIDES IT. A routed lane draws
 * after the lane it follows, so 'thin' then 'grow' despeckles and the reverse closes
 * gaps - both expressible now, one of them for the first time. An unrouted document
 * draws in table order whatever order its lines were typed, which tools/test_viz.c
 * checks. See chain_ranks() in viz.c.
 *
 * ROUTING is the sidechain. '>route noise bass' means the noise lane takes its
 * intensity from whatever the bass lane last played instead of from its own
 * digits. One idea - a lane can read another lane's output - and it works
 * between any two lanes, which is why it is not called sidechaining: the same
 * mechanism sends a kick to a circle's radius or a filter sweep to a mask's level.
 *
 * FRAME BY FRAME. viz_mark() is called from the sequencer's step, so the
 * animation runs on the same clock as the music and every frame is a step. The
 * frame is a character buffer; the editor draws it in a split, and '>frame'
 * sends the same buffer over OSC to whatever is rendering it elsewhere.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* THE MAXIMUM frame, not the frame. The live size is whatever the preview pane
 * can show - see viz_size() - because a fixed 32x12 frame drawn into a pane of
 * another shape is cropped on two sides and stale on the others, and the owner
 * saw exactly that: half a picture with junk under it. */
#define VIZ_W 60
#define VIZ_H 24

/* Set the live frame size. The editor calls this whenever the layout changes,
 * so the generators always draw into the rectangle that is actually visible -
 * and '>frame' sends that same rectangle, nothing padded and nothing lost. */
void viz_size(int w, int h);
int  viz_cols(void);
int  viz_rows(void);

/* WHERE THE PREVIEW GOES.
 *
 * Both the draw loop and the code that decides how wide a text line is must
 * agree about this exactly, or the cursor lands in the picture and there is no
 * way to tell. So there is ONE function, here - the same arrangement
 * seq_pattern.h uses for the playhead.
 *
 * IT ALWAYS STACKS: code full width on top, picture full width underneath.
 * Side by side was tried and removed, for two reasons that are really one.
 *
 *   A pattern line is a name and up to thirty-two steps, so about thirty
 *   columns. Side by side at thirty columns left twelve, wrapped every line,
 *   and made the document unnavigable. Full width never wraps, at any density.
 *
 *   A CELL IS TWICE AS TALL AS IT IS WIDE on both faces. A pane taking a third
 *   of the width and all of the height is therefore a portrait sliver on the
 *   glass - eighteen cells across by twenty-two down measures 108 x 264 pixels
 *   - and ASCII drawn into it comes out stretched. Stacking gives a landscape
 *   rectangle instead: the shape a screen is, the shape the RP2040's HDMI
 *   output wants, and the same shape at either density.
 *
 * One orientation also means one code path, so there is one place for the
 * arithmetic to be wrong instead of two.
 *
 * The rect INCLUDES the one-cell border; w and h are 0 when the split is off. */
typedef struct {
    int x, y, w, h;        /* in text cells, border included */
} viz_pane_t;

viz_pane_t viz_pane(int cols, int rows);


/* THE PRIMITIVES, AS A TABLE OF NAMES.
 *
 * viz no longer owns lanes. A drawing lane lives in seq's one lane table with
 * SEQ_BIND_VIZ and a primitive index, so a circle and a kick are the same
 * sentence with different destinations - see docs/MAP.md. What is left here is
 * the drawing: thirteen primitives, a frame, and a pane to show it in.
 *
 * This is the collapse that removed two lane structs, two compile loops over
 * the same pattern walk, two mute mechanisms, two budgets and two listings.
 * Every bug in that area used to have to be found twice. */
int         viz_prim_count(void);
const char *viz_prim_name(int i);
int         viz_prim_index(const char *name);   /* -1 if there is no such one */

/* A lane bound to `prim` fired. CALLED FROM THE CLOCK CALLBACK, so this only
 * records - generating a frame is a pass over the whole picture and doing that
 * between two ticks is what docs/OS.md forbids. */
void viz_mark(int prim, int amt, char dir, uint32_t tick);

/* WHERE A PRIMITIVE DRAWS, AS A LANE OF ITS OWN.
 *
 * '>disc[x] 0..9..' is a lane like any other - it has a pattern, it alternates,
 * it nests, it can be routed - and what it carries is the circle's position
 * across the frame rather than a note or an amount. That is the whole design:
 * positioning is not a feature bolted onto disc, it is the SAME sentence pointed
 * at a different part of it.
 *
 * The alternatives were worse. 'disc 5,3' breaks one-character-per-step, which is
 * what keeps the playhead on the character that is sounding. An '>at 3,7' verb
 * pairs lanes by convention and adds a name that deletes nothing. And a
 * 'discleft'/'discright' family is how a vocabulary rots.
 *
 * VIZ_PARAM_X and _Y are the only two for now, deliberately: a short list per
 * binding, or this becomes the flag grammar docs/COMMANDS.md exists to refuse.
 *
 * 0 is the left or top edge and 9 is the right or bottom; the shape's CENTRE goes
 * there, so '>disc[x] 0..9..' sweeps it across. Set from the clock callback like
 * any other mark, and applied before the frame is drawn. */
#define VIZ_PARAM_NONE 0
#define VIZ_PARAM_X    1
#define VIZ_PARAM_Y    2

/* Which parameter a name selects, or VIZ_PARAM_NONE. `name` is the text inside
 * the brackets: viz owns this list because viz owns what a primitive has. */
int viz_param_index(const char *name);

void viz_mark_param(int prim, int param, int amt);

/* Draw the frame the marks asked for, if any. Called from the main loop;
 * returns true when a new frame was generated.
 *
 * Marks are replayed in PRIMITIVE order, not the order they arrived, because
 * the order is a pipeline: history, then motion, then sources, then repetition.
 * That is what makes echo plus move read as a trail rather than a judder, and
 * it is a property of the table rather than of the lanes. */
bool viz_service(void);

/* Is anything drawing? True while a primitive has been marked and not yet
 * blanked, so the split knows whether the pane is live. */
bool viz_active(void);

/* Blank the frame and forget every pending mark. A new document is a blank
 * picture as well as a blank score; the LANES are seq's to forget. */
void viz_forget_all(void);



/* One row of the frame, NUL-terminated at the live width. Rows at or past
 * viz_rows() are empty, so a caller cannot read stale ink out of the buffer. */
const char *viz_row(int y);

/* The whole frame as one newline-separated string, for '>frame'. */
int viz_text(char *out, int max);

/* Split-screen preview. The picture takes about half the rows; '>split 8' asks
 * for a number of rows instead, because now that the split always stacks, rows
 * are the only dimension there is to give away. Code is the thing being edited
 * and the preview is a monitor, so when the two disagree the code wins: the
 * picture shrinks before the code drops below four visible lines. */
void viz_split(bool on);
void viz_split_rows(int rows);       /* 0 = about half */
bool viz_split_on(void);
