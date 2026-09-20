/* HID usage codes to characters. Table is the unshifted/shifted pair for the
 * printable range; everything else is handled as a named event. */
#include "keymap.h"

typedef struct { char plain, shift; } pair_t;

/* Indexed by HID usage 0x04..0x38. */
static const pair_t k[] = {
    /* 04 */ {'a','A'}, {'b','B'}, {'c','C'}, {'d','D'}, {'e','E'}, {'f','F'},
    /* 0a */ {'g','G'}, {'h','H'}, {'i','I'}, {'j','J'}, {'k','K'}, {'l','L'},
    /* 10 */ {'m','M'}, {'n','N'}, {'o','O'}, {'p','P'}, {'q','Q'}, {'r','R'},
    /* 16 */ {'s','S'}, {'t','T'}, {'u','U'}, {'v','V'}, {'w','W'}, {'x','X'},
    /* 1c */ {'y','Y'}, {'z','Z'},
    /* 1e */ {'1','!'}, {'2','@'}, {'3','#'}, {'4','$'}, {'5','%'},
    /* 23 */ {'6','^'}, {'7','&'}, {'8','*'}, {'9','('}, {'0',')'},
    /* 28 */ {0,0},     /* Enter      */
    /* 29 */ {0,0},     /* Escape     */
    /* 2a */ {0,0},     /* Backspace  */
    /* 2b */ {0,0},     /* Tab        */
    /* 2c */ {' ',' '}, /* Space      */
    /* 2d */ {'-','_'}, {'=','+'}, {'[','{'}, {']','}'}, {'\\','|'},
    /* 32 */ {'#','~'}, /* non-US hash, same key as backslash on some boards */
    /* 33 */ {';',':'}, {'\'','"'}, {'`','~'},
    /* 36 */ {',','<'}, {'.','>'}, {'/','?'},
};

char keymap_char(uint8_t usage, bool shift)
{
    if (usage < 0x04 || usage > 0x38) {
        return 0;
    }
    const pair_t p = k[usage - 0x04];
    return shift ? p.shift : p.plain;
}
