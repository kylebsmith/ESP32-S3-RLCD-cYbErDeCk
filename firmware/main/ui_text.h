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

/* Shown when a command's output moves the view. The owner hit this exactly:
 * ran a command, was moved somewhere else, and "had no idea how to get back" -
 * so they ran another command, which piled onto the same page. A rescue key
 * nobody is told about is not a design. */
#define UI_OUT_BACK      "output - Ctrl-O goes back"

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
/* THE SETTINGS ARE A DOCUMENT, AND IT IS NOT A NEW CONCEPT.
 *
 * A document named 'boot' is RUN at startup, one line at a time, exactly as
 * if the owner had pressed Ctrl+Enter on each. So the settings file is a
 * guide that happens to run by itself - no config format, no parser, no
 * second syntax, and nothing to learn that was not already true of every
 * other line on this device. Edit it like anything else; it takes effect next
 * boot.
 *
 * It runs with GUIDE authority, not the owner's, so it cannot reach the
 * commands that change pairing, power or the USB mode. A settings file that
 * could put the deck into a state the owner then cannot type their way out of
 * would be the same trap this project has already fallen into twice. */
/* THE NAMES OF THE LANES ARE LINES IN HERE (docs/MANIFESTO.md §3.8).
 *
 * They were verbs with their numbers compiled in - a player could not add a
 * conga or move the kick to the note their drum machine wants. Now each is a
 * definition, run at startup like every other line of this document, and a
 * player edits them the way they edit anything else. The first line is also
 * the mark: a boot document written before this had none, and gets this block
 * put at its top so the names exist before any of its own lines run. */
#define BOOT_MARK "THE NAMES ARE YOURS"
#define BOOT_NAMES \
    "THE NAMES ARE YOURS. Change a\n" \
    "note, add a lane, rename one:\n" \
    ">kick = note 36\n" \
    ">snare = note 38\n" \
    ">hat = note 42\n" \
    ">ohat = note 46\n" \
    ">clap = note 39\n" \
    ">tom = note 45\n" \
    ">rim = note 37\n" \
    ">crash = note 49\n" \
    ">bass = voice 2 ch 1 gate 180\n" \
    ">lead = voice 4 ch 2 gate 120\n" \
    ">pad = voice 3 ch 3 gate 420\n" \
    ">arp = voice 5 ch 4 gate 90\n" \
    ">cut = cc 74\n" \
    ">res = cc 71\n" \
    ">mod = cc 1\n" \
    ">rev = cc 91\n" \
    "\n"

#define BOOT_TEXT \
    BOOT_NAMES \
    "Runs at startup. Edit freely.\n" \
    ">bpm 124\n" \
    ">scale dmin\n" \
    ">swing 50\n" \
    ">density chunky\n" \
    "# dense = 60 cols, smaller\n" \
    "# fewer wrapped lines\n"

/* THE GUIDE SAYS WHICH GRAMMAR IT TEACHES. A guide lives in the journal and is
 * only rewritten by ensure_guide_buffer() when it is out of date - it is the
 * owner's menu, and firmware that overwrote it would destroy the thing the
 * design is for. So the last line is a version, and a guide without this one
 * gets the new text on top with the old kept underneath it.
 *
 * "guide 2" is the grammar of docs/MANIFESTO.md §3.6: a digit is how much, '_'
 * is a tie, ',' makes a chord, and X ',' '?' and '-' are gone. A guide from
 * before it teaches lines that are now refused. */
#define GUIDE_MARK "guide 2"

#define GUIDE_TEXT \
    "Ctrl+Enter runs a line.\n" \
    "Enter always makes a line.\n" \
    "Ctrl-L / Ctrl-J switch docs.\n" \
    "Ctrl-O returns from output.\n" \
    "\n" \
    "RUN THESE, TOP TO BOTTOM\n" \
    ">bpm 124\n" \
    ">scale dmin\n" \
    ">kick 9...x...9...x...\n" \
    ">hat x3x3x3x3x3x3x3x%50\n" \
    ">bass 0__.3_..5__.3...\n" \
    ">pad [0,2,4]___[3,5,7]___\n" \
    ">cut 0..3..6..9..6.\n" \
    ">play\n" \
    "\n" \
    "x hits. . rests.\n" \
    "0-9 is how much: on a drum\n" \
    "how hard, on bass lead pad\n" \
    "arp the degree (0 the root),\n" \
    "on a cc lane the value.\n" \
    "_ holds the note before it.\n" \
    "x%15 plays 15% of the time.\n" \
    "[xx] two in one step.\n" \
    "[0,2,4] all at once - chord.\n" \
    "<3 5> one each time round.\n" \
    "x.x. /2 half speed, *2 double.\n" \
    "\n" \
    "The names are lines in the\n" \
    "boot doc. Make another:\n" \
    ">conga = note 63\n" \
    ">conga x..x..x.\n" \
    "A part is a lane too: how\n" \
    "hard, and which octave -\n" \
    ">bass:vel 9...3...\n" \
    ">bass:oct <2 3>...\n" \
    "\n" \
    "Edit any line, run it again\n" \
    "- it changes live. Run it\n" \
    "unchanged to silence it.\n" \
    "A mistake is refused: the\n" \
    "bar says why, and the wrong\n" \
    "character is boxed.\n" \
    "\n" \
    ">swing 58\n" \
    ">scale fmin\n" \
    ">stop\n" \
    "\n" \
    "mute some, solo one, then\n" \
    "all back on:\n" \
    ">mute hat bass\n" \
    ">solo kick\n" \
    ">mute\n" \
    "MIDI clock out:\n" \
    ">sync on\n" \
    "what plays, and where to:\n" \
    ">lanes\n" \
    ">send\n" \
    "one cable to a DAW:\n" \
    ">usb on\n" \
    ">wifi <ssid> <pass>\n" \
    ">host deck 12345678\n" \
    ">osc 192.168.4.2 9000\n" \
    "\n" \
    "PICTURES - same document,\n" \
    "same clock. each one is a\n" \
    "lane, exactly like a drum.\n" \
    "fields: noise disc box turn\n" \
    "  ramp grid\n" \
    "levels: mask edge\n" \
    "bends: echo move spin warp\n" \
    "  grow thin flip fold\n" \
    "disc:2 disc:3, more of one.\n" \
    ">echo 8\n" \
    "then add these:\n" \
    ">noise 2.4.2.4.\n" \
    ">move d\n" \
    "trails fall.\n" \
    ">route disc kick\n" \
    "u d l r say which way.\n" \
    "preview on or off:\n" \
    ">split\n" \
    "send the picture over osc:\n" \
    ">frame\n" \
    "your documents, and all\n" \
    "the commands:\n" \
    ">list\n" \
    ">help\n" \
    "guide 2\n"

#ifndef UI_TEXT_NO_ASSERTS
_Static_assert(sizeof(UI_NOT_A_COMMAND) - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_GUIDE_HINT)    - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_NO_GUIDE)      - 1 <= UI_NARROW_COLS, "status message is cut");
#endif

#endif /* UI_TEXT_H */
