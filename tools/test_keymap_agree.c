/* The mapper's constants must equal kbd.h's enum. If they ever diverge, the
 * serial keyboard silently produces the wrong event - a backspace that moves
 * the cursor, an arrow that types. This is the check that they agree. */
#include <stdio.h>
#include "kbd.h"
#include "serialkbd_map.h"
int main(void){
    int f=0;
#define SAME(a,b) do{ if((int)(a)!=(int)(b)){printf("[FAIL] %s != %s\n",#a,#b);f++;} }while(0)
    SAME(SKB_CHAR, KBD_EV_CHAR);      SAME(SKB_ENTER, KBD_EV_ENTER);
    SAME(SKB_BACKSPACE, KBD_EV_BACKSPACE); SAME(SKB_TAB, KBD_EV_TAB);
    SAME(SKB_ESC, KBD_EV_ESC);        SAME(SKB_LEFT, KBD_EV_LEFT);
    SAME(SKB_RIGHT, KBD_EV_RIGHT);    SAME(SKB_UP, KBD_EV_UP);
    SAME(SKB_DOWN, KBD_EV_DOWN);      SAME(SKB_MOD_LCTRL, KBD_MOD_LCTRL);
    printf(f?"[FAIL] %d constant(s) diverged\n":"[PASS] keymap constants agree with kbd.h\n",f);
    return f!=0;
}
