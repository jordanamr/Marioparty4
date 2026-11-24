#include <assert.h>
#include "game/object.h"
static omObjData *entrants[4];
#define omGetGroupMemberListEx(proc, group) entrants
#include "src/REL/m456Dll/main.c"

int main(void)
{
    omObjData objects[4] = {0};
    M456DllMainWork players[4] = {0};
    for (int i = 0; i < 4; i++) {
        entrants[i] = &objects[i];
        objects[i].data = &players[i];
        lbl_1_bss_70[i] = i;
    }
    assert(fn_1_D2C() == 0);
    for (int i = 0; i < 4; i++) {
        assert(lbl_1_bss_70[i] == -1);
    }

    players[1].unk_30 = 6;
    players[3].unk_30 = 6;
    assert(fn_1_D2C() == 2);
    assert(lbl_1_bss_70[0] == 1 && lbl_1_bss_70[1] == 3);
    return 0;
}
