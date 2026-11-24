#include <assert.h>
#include <stdio.h>
#include "game/board/main.h"
#include "game/board/space.h"
#include "game/board/player.h"
#include "game/audio.h"
static unsigned roll;
#define BoardIsKill() 0
#define HuAudFXPlay(id) 0
#define BoardSpacePosGet(layer, index, pos) (*(pos) = (Vec){0})
#define BoardPlayerPosGet(player, pos) (*(pos) = (Vec){0})
#define BoardPlayerPosLerpStart(...) ((void)0)
#define BoardRandMod(n) roll
#include "src/REL/w06Dll/mg_item.c"

int main(void)
{
    omObjData obj = {0};
    Bss24Work *cursor = OM_GET_WORK_PTR(&obj, Bss24Work);
    lbl_1_bss_24 = &obj;
    for (int i = 0; i < 3; ++i) {
        lbl_1_data_1AA[i] = 10; /* An available zero-priority item. */
    }
    for (int mask = 0; mask < 7; ++mask) {
        for (int i = 0; i < 3; ++i) {
            lbl_1_bss_30[i] = (mask >> i) & 1;
        }
        assert(!lbl_1_bss_30[fn_1_2A1C(0)]);
    }
    GWPlayer[0].port = 0;
    GWPlayer[0].com = 0;
    for (int side = 0; side < 2; ++side) {
        lbl_1_bss_30[0] = lbl_1_bss_30[2] = 0;
        lbl_1_bss_30[1] = 1;
        lbl_1_bss_1C = 1;
        cursor->unk02 = 1;
        cursor->unk01 = 2;
        roll = side ? 99 : 0;
        fn_1_2EF4(&obj);
        assert(cursor->unk02 == side * 2);
        assert(cursor->unk01 == 1);
        cursor->unk01 = 2;
        lbl_1_bss_1C = 0;
        HuPadDStk[0] = side ? PAD_BUTTON_LEFT : PAD_BUTTON_RIGHT;
        HuPadBtnDown[0] = PAD_BUTTON_A;
        fn_1_2EF4(&obj);
        assert(cursor->unk01 == 3 && lbl_1_bss_33 == side * 2);
        HuPadDStk[0] = HuPadBtnDown[0] = 0;
    }
    puts("Bowser item choices passed");
    return 0;
}
