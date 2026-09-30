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
 * before it teaches lines that are now refused.
 *
 * "guide 3": a password is asked for and never typed on a line (ask.h), so the
 * '>wifi <ssid> <pass>' and '>host deck 12345678' that "guide 2" taught are
 * refused - and cut - now. */
#define GUIDE_MARK "guide 5"

/* "guide 4", 2026-09-29: the sections are blocks - a heading run is its
 * section run (editor_block.h) - and the guide says what changed: a line run
 * unchanged is gone, toggle waits for the bar, a sound moves channel. A guide
 * that is still "guide 3" exactly as the firmware wrote it, which is how it is
 * known unedited, is replaced outright; an edited one keeps its text below. */
#define GUIDE3_FNV 0xe79bb7f4u

/* "guide 5", 2026-09-29: a block run again is a switch, a key and a colour
 * wait for the one as a toggle does, and a controller is set with '>send cut
 * 3'. "guide 4" exactly as the firmware wrote it is replaced the same way. */
#define GUIDE4_FNV 0xb6a9751eu

#define GUIDE_TEXT \
    "GUIDE. ctrl+enter runs a line\n" \
    "and the lines tabbed under it,\n" \
    "so a heading runs its section.\n" \
    "enter makes a line, tab\n" \
    "indents it. ctrl-l / ctrl-j:\n" \
    "next document. ctrl-o: output\n" \
    "and back. ctrl-g: here.\n" \
    "\n" \
    "-- a beat\n" \
    "  >bpm 124\n" \
    "  >scale dmin\n" \
    "  >kick 9...8...9...8...\n" \
    "  >hat ..7...7...7...7.\n" \
    "  >bass 0__.3_..5__.3...\n" \
    "  >pad [0,2,4]___[3,5,7]___\n" \
    "  >play\n" \
    "\n" \
    "-- steps\n" \
    "  x hits. . rests. 0-9 is how\n" \
    "  hard on a drum, the degree\n" \
    "  on bass lead pad arp, the\n" \
    "  value on a cc. _ holds it.\n" \
    "  x%15: 15 times in 100.\n" \
    "  [xx] two in a step. [0,4,7]\n" \
    "  a chord. <3 5> one a bar.\n" \
    "  <000 777> a word a bar.\n" \
    "  /2 half. *2 double. !2\n" \
    "  twice, then it stops.\n" \
    "  >hat ..7...7...7.[77]%50.7.\n" \
    "\n" \
    "-- change it live\n" \
    "  edit a line, run it: it\n" \
    "  changes. run it unchanged:\n" \
    "  gone. once more: back. a\n" \
    "  name alone: gone.\n" \
    "  >bass 0__.5_..7__.5...\n" \
    "\n" \
    "-- on the one\n" \
    "  toggle, key and colour wait\n" \
    "  for the bar. a block run\n" \
    "  again: its lanes out. again:\n" \
    "  back.\n" \
    "  >toggle hat bass\n" \
    "\n" \
    "-- names\n" \
    "  make one, or move one to\n" \
    "  another midi channel:\n" \
    "  >conga = note 63\n" \
    "  >conga ..x..x..x..x.x..\n" \
    "  >bass = ch 5\n" \
    "  >bass = ch 1\n" \
    "  a controller, set now:\n" \
    "  >send cut 3\n" \
    "\n" \
    "-- endings\n" \
    "  a lane that ends starts\n" \
    "  another:\n" \
    "  >tom x.x.x.x. !2\n" \
    "  >crash x !1\n" \
    "  >route crash tom:end\n" \
    "\n" \
    "-- pictures\n" \
    "  a picture is a lane. fields:\n" \
    "  noise disc box turn ramp\n" \
    "  grid. levels: mask edge.\n" \
    "  bends: echo move spin warp\n" \
    "  grow thin flip fold.\n" \
    "  >echo 8\n" \
    "  >route disc kick\n" \
    "  >route disc:x bass\n" \
    "  >noise 2.4.2.4.\n" \
    "  >send view on\n" \
    "\n" \
    "-- stop\n" \
    "  >clear\n" \
    "  >stop\n" \
    "\n" \
    "-- more\n" \
    "  >lanes\n" \
    "  >send\n" \
    "  >usb on\n" \
    "  >list\n" \
    "  >help\n" \
    "  midi learn: the midi page.\n" \
    "guide 5\n" \
    ""

/* THE MIDI PAGE: every controller the deck and its pieces send, one line each.
 * A DAW learns the next controller it hears, and with a set playing it hears
 * everything: run a line, and its block plays that cc alone for eight bars
 * (a count, so it stops itself) with every other MIDI lane muted by '>map'.
 * The owner, 2026-09-29: "its own persistent page like guide". */
#define MIDI_MARK "midi 2"

/* "midi 2" adds the lead's and the arp's filters; "midi 1" exactly as the
 * firmware wrote it is replaced, an edited one kept below the new. */
#define MIDI1_FNV 0xb4c63f33u

#define MIDI_TEXT \
    "MIDI LEARN. put the DAW in\n" \
    "map mode, pick a control, run\n" \
    "a line: that cc plays alone\n" \
    "for 8 bars. learn it, run the\n" \
    "next. >map at the end.\n" \
    ">play\n" \
    ">cut = cc 74 ch 1\n" \
    "  >cut 1357 /2 !16\n" \
    "  >map cut\n" \
    ">res = cc 71 ch 1\n" \
    "  >res 1357 /2 !16\n" \
    "  >map res\n" \
    ">mod = cc 1 ch 1\n" \
    "  >mod 1357 /2 !16\n" \
    "  >map mod\n" \
    ">rev = cc 91 ch 1\n" \
    "  >rev 1357 /2 !16\n" \
    "  >map rev\n" \
    ">glow = cc 74 ch 3\n" \
    "  >glow 1357 /2 !16\n" \
    "  >map glow\n" \
    ">bite = cc 74 ch 5\n" \
    "  >bite 1357 /2 !16\n" \
    "  >map bite\n" \
    ">drive = cc 20 ch 6\n" \
    "  >drive 1357 /2 !16\n" \
    "  >map drive\n" \
    ">shine = cc 74 ch 2\n" \
    "  >shine 1357 /2 !16\n" \
    "  >map shine\n" \
    ">spark = cc 74 ch 4\n" \
    "  >spark 1357 /2 !16\n" \
    "  >map spark\n" \
    "the screen, channel 16:\n" \
    ">ink = cc 1 ch 16\n" \
    "  >ink 1357 /2 !16\n" \
    "  >map ink\n" \
    ">paper = cc 2 ch 16\n" \
    "  >paper 1357 /2 !16\n" \
    "  >map paper\n" \
    ">sat = cc 3 ch 16\n" \
    "  >sat 1357 /2 !16\n" \
    "  >map sat\n" \
    ">day = cc 4 ch 16\n" \
    "  >day 1357 /2 !16\n" \
    "  >map day\n" \
    ">inv = cc 5 ch 16\n" \
    "  >inv 1357 /2 !16\n" \
    "  >map inv\n" \
    ">glint = cc 6 ch 16\n" \
    "  >glint 1357 /2 !16\n" \
    "  >map glint\n" \
    ">skew = cc 7 ch 16\n" \
    "  >skew 1357 /2 !16\n" \
    "  >map skew\n" \
    ">lines = cc 8 ch 16\n" \
    "  >lines 1357 /2 !16\n" \
    "  >map lines\n" \
    "all back on:\n" \
    ">map\n" \
    ">stop\n" \
    "midi 2\n" \
    ""

#ifndef UI_TEXT_NO_ASSERTS
_Static_assert(sizeof(UI_NOT_A_COMMAND) - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_GUIDE_HINT)    - 1 <= UI_NARROW_COLS, "status message is cut");
_Static_assert(sizeof(UI_NO_GUIDE)      - 1 <= UI_NARROW_COLS, "status message is cut");
#endif

#endif /* UI_TEXT_H */
