#include <assert.h>
#include <stdio.h>
#include "src/REL/w06Dll/fire.c"

int main(void)
{
    for (int mask = 0; mask < 8; ++mask) {
        for (int i = 0; i < 3; ++i) {
            GWPlayer[0].items[i] = mask & (1 << i) ? i : BOARD_ITEM_NONE;
        }
        int slot = fn_1_7944(0);
        if (mask) {
            assert(slot >= 0 && slot < 3 && (mask & (1 << slot)));
        } else {
            assert(slot == -1);
        }
    }
    puts("Bowser inventory slot selection passed");
    return 0;
}
