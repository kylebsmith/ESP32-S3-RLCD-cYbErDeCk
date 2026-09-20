/*
 * The SD mirror used to write every buffer to one file, /sdcard/notes.txt.
 * Switching documents therefore overwrote the previous document's backup, and
 * a '+out' command transcript was mirrored as though it were a document.
 *
 * Each case below FAILS against that behaviour, which is the point: a fix
 * without a check that the old state fails is a claim, not a repair.
 */
#include <stdio.h>
#include <string.h>

#include "mirror_path.h"

static int fails;

static void eq(const char *what, const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        printf("[FAIL] %s: got %s want %s\n", what, got, want);
        fails++;
    }
}

int main(void)
{
    char a[64], b[64];

    /* 1. Two documents must not share a file. Under the old single-file
     *    mirror these were both /sdcard/notes.txt. */
    mirror_path(a, sizeof a, "/sdcard", "rustbelt");
    mirror_path(b, sizeof b, "/sdcard", "rustbeltsave");
    eq("rustbelt", a, "/sdcard/rustbelt.txt");
    eq("rustbeltsave", b, "/sdcard/rustbeltsave.txt");
    if (strcmp(a, b) == 0) {
        printf("[FAIL] two documents collide on %s\n", a);
        fails++;
    }

    /* 2. State WHY long filenames had to be turned on, as a check rather
     *    than a comment: these two real document names are identical in their
     *    first eight characters, so 8.3 truncation maps both to RUSTBELT.TXT.
     *    If someone sets CONFIG_FATFS_LFN_NONE again, the collision this file
     *    exists to prevent comes straight back. */
    if (strncmp("rustbelt", "rustbeltsave", 8) != 0) {
        printf("[FAIL] the 8.3 collision case is no longer a collision\n");
        fails++;
    }

    /* 3. Machine-written buffers are not mirrored at all. */
    if (!mirror_is_transient("+out")) {
        printf("[FAIL] +out is not recognised as transient\n");
        fails++;
    }
    if (mirror_is_transient("rustbelt")) {
        printf("[FAIL] a real document was called transient\n");
        fails++;
    }

    /* 4. A name FAT cannot hold is mangled, never shortened - dropping the
     *    offending characters would merge "my file" into "myfile". */
    mirror_path(a, sizeof a, "/sdcard", "my file");
    mirror_path(b, sizeof b, "/sdcard", "myfile");
    eq("spaces become underscores", a, "/sdcard/my_file.txt");
    if (strcmp(a, b) == 0) {
        printf("[FAIL] mangling merged two distinct names\n");
        fails++;
    }

    /* 5. The unnamed scratch buffer still gets a backup. */
    mirror_path(a, sizeof a, "/sdcard", "");
    eq("unnamed scratch", a, "/sdcard/scratch.txt");
    mirror_path(a, sizeof a, "/sdcard", NULL);
    eq("null name", a, "/sdcard/scratch.txt");

    /* 6. An over-long name is bounded, and cannot run off the buffer. */
    mirror_path(a, sizeof a, "/sdcard",
                "abcdefghijklmnopqrstuvwxyz0123456789");
    eq("clamped to DOC_NAME_MAX", a, "/sdcard/abcdefghijklmnopqrstuvwx.txt");

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] SD mirror paths\n",
           fails);
    return fails != 0;
}
