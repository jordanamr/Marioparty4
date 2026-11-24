#include <assert.h>
#include <stdio.h>
#include "game/frand.h"
static u32 nextRoll;
static u32 Roll(void)
{
    return nextRoll++;
}
#define frand() Roll()
#define frandmod(n) 0
#include "src/REL/w04Dll/mg_item.c"

int main(void)
{
    s16 delay = 30, moves = 0;
    for (int action = 0; action < 4; ++action) {
        int wait = delay;
        for (int i = 0; i < wait; ++i) {
            assert(ItemCpuInput(&delay, &moves) == 0);
            assert(moves == action && nextRoll == (unsigned)(2 * action));
        }
        assert(ItemCpuInput(&delay, &moves) == (action == 3 ? PAD_BUTTON_A : PAD_BUTTON_LEFT));
        assert(delay == 15);
    }
    puts("Boo item CPU waits and selections passed");
    return 0;
}
