/*
 * Fixed on-device text, with the width rule beside it.
 *
 * WHY THIS FILE EXISTS. The status bar is TEXT_COLS wide and TEXT_COLS is 30
 * in the default grid. status_bar() clips with "%-*.*s", so a message longer
 * than 30 characters is not truncated at the edge of the SCREEN, it is
 * truncated at the edge of the SENTENCE - and the half that is lost is always
 * the half that says what to do.
 *
 * That is not hypothetical. "not a command - start the line with >" is 37
 * characters and rendered as "not a command - start the line". The owner hit
 * it, could not tell why a line would not run, and had to work it out from
 * the text of the line itself. The comment in editor.c claimed this exact
 * failure had been fixed; what was fixed was the pixel renderer clipping
 * silently, and the clip simply moved into snprintf.
 *
 * So the strings live here, next to a static assertion, and the build fails
 * rather than the sentence. A host check reads THIS header - the shipping
 * strings, not a copy of them.
 */
#ifndef UI_TEXT_H
#define UI_TEXT_H

/* The narrowest grid the device ever renders: (400 - 2*20) / 12 = 30.
 * The dense face is 60 columns, so 30 is the binding constraint. */
#define UI_NARROW_COLS 30

#define UI_NOT_A_COMMAND "not a command - start with >"
/* Enter does NOT run a line - it always inserts, so that a command can have
 * a line added after it. Ctrl+Enter runs. The old hint said "Enter runs a
 * line", which is the opposite of the truth, and an owner who believed it
 * would split their guide in half instead of running anything. */
#define UI_GUIDE_HINT    "guide: Ctrl+Enter runs a line"
#define UI_NO_GUIDE      "no guide buffer"

/* THE GUIDE IS THE TUTORIAL AND THE INSTRUMENT AT THE SAME TIME.
 *
 * The complaint about live coding environments is not that they are hard, it
 * is that they are hard ON PURPOSE - the syntax is a membrane, and getting
 * through it is treated as the point. This is the opposite choice: the first
 * thing the owner sees is a track that plays, and every line in it is one
 * they can edit while it is playing.
 *
 * Thirty columns, because that is the grid - and the check below proves it
 * rather than trusting whoever edits this next. */
#define GUIDE_TEXT \
    "Lines starting with > are\n" \
    "commands. Ctrl+Enter runs\n" \
    "the one under the cursor.\n" \
    "Enter always makes a line.\n" \
    "\n" \
    "RUN THESE, TOP TO BOTTOM\n" \
    ">bpm 124\n" \
    ">scale dmin\n" \
    ">kick X...x...X...x...\n" \
    ">hat x,x,x,x,x,x,x,x,\n" \
    ">bass 0...3...5...3...\n" \
    ">play\n" \
    "\n" \
    "x hit  X loud  , quiet\n" \
    ". rest  0-9 is a degree\n" \
    "0 is the root. Edit any\n" \
    "line and run it again -\n" \
    "it changes as it plays.\n" \
    "Run it unchanged to\n" \
    "silence that lane.\n" \
    "\n" \
    ">swing 58\n" \
    ">scale fmin\n" \
    ">stop\n" \
    "\n" \
    ">lanes  what is playing\n" \
    ">send   where it goes\n" \
    ">help   all the commands\n" \
    ">list   your documents\n"

#ifndef UI_TEXT_NO_ASSERTS
_Static_assert(sizeof(UI_NOT_A_COMMAND) - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_GUIDE_HINT)    - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_NO_GUIDE)      - 1 <= UI_NARROW_COLS, "status message is cut");
#endif

#endif /* UI_TEXT_H */
