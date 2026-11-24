#include <assert.h>
#include "game/esprite.h"
static s16 hidden[25];
static int count;
static void RecordHide(s16 id)
{
    assert(count < 25);
    hidden[count++] = id;
}
#define espDispOff RecordHide
#include "src/REL/m447dll/main.c"

int main(void)
{
    UnkM447Struct_00 board = {0};
    for (int i = 0; i < 25; i++) {
        board.unk34[i] = 70 + i;
    }
    fn_1_1CE0(&board);
    assert(count == 25);
    for (int i = 0; i < 25; i++) {
        assert(hidden[i] == 70 + i);
    }
    return 0;
}
