/*
 * Document name -> SD mirror path. Pure arithmetic on characters, no
 * dependencies, so tools/test_mirror_path.c exercises THE SHIPPING FUNCTION
 * rather than a copy of it that can drift - the same arrangement as
 * st7305_addr.h, and for the same reason.
 */
#ifndef MIRROR_PATH_H
#define MIRROR_PATH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifndef MIRROR_NAME_MAX
#define MIRROR_NAME_MAX 24          /* DOC_NAME_MAX */
#endif

/* True for a machine-written buffer. These are neither journalled nor
 * mirrored: the owner's documents are copied to a DGX for semantic analysis,
 * and command transcripts in that corpus are contamination, not data. */
static inline bool mirror_is_transient(const char *name)
{
    return name != NULL && name[0] == '+';
}

/* A document name is the owner's, typed with '>name'. It reaches a FAT
 * filesystem, so characters FAT cannot hold become '_' rather than being
 * dropped - dropping would silently merge "my file" and "myfile" into one
 * backup, which is the bug this whole file exists to prevent.
 *
 * Long filenames are enabled (CONFIG_FATFS_LFN_HEAP). Under 8.3 the names
 * "rustbelt" and "rustbeltsave" - both of which exist on this device right
 * now - both truncate to RUSTBELT.TXT and one silently overwrites the other.
 */
static inline void mirror_path(char *out, size_t cap, const char *mount,
                               const char *name)
{
    char safe[MIRROR_NAME_MAX + 1];
    size_t n = 0;
    for (const char *p = name; p != NULL && *p != '\0' && n < MIRROR_NAME_MAX;
         p++) {
        const bool ok = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                        (*p >= '0' && *p <= '9') || *p == '-' || *p == '_';
        safe[n++] = ok ? *p : '_';
    }
    safe[n] = '\0';
    /* The unnamed scratch buffer still deserves a backup; it is where work
     * starts, and so the buffer most likely to hold something unsaved. */
    snprintf(out, cap, "%s/%s.txt", mount, n > 0 ? safe : "scratch");
}

#endif /* MIRROR_PATH_H */
