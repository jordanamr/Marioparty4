#include <assert.h>
#include "game/audio.h"
#include "game/sprite.h"
#include "REL/mentDll.h"
#include "port/netplay_runtime.h"

#define HuSprBankSet(...) ((void)0)
#define HuSprAttrReset(...) ((void)0)
#define HuSprAttrSet(...) ((void)0)
#define HuSprPosSet(...) ((void)0)
#define HuAudFXPlay(...) 0
#define PartyBoard_NetplayEnabled() false
#include "src/REL/mentDll/main.c"

void fn_1_4D8(void) {}
float fn_1_32C(float from, float to, float tick, float end)
{
    return from + (to - from) * tick / end;
}

int main(void)
{
    for (int i = 0; i < 4; ++i) {
        lbl_1_bss_3114[i].unk_58 = i;
        lbl_1_bss_3114[i].unk_6C = i;
    }
    for (int from = 0; from < 8; ++from) {
        for (int occupied = 0; occupied < 8; ++occupied) {
            if (from == occupied) continue;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    lbl_1_bss_3114[0].unk_68 = from;
                    lbl_1_bss_3114[1].unk_68 = occupied;
                    lbl_1_bss_3114[2].unk_60 = lbl_1_bss_3114[3].unk_60 = 1;
                    HuPadStkX[0] = 65 * dx;
                    HuPadStkY[0] = 65 * dy;
                    fn_1_15CB4(NULL, &lbl_1_bss_3114[0]);
                    int selected = lbl_1_bss_3114[0].unk_68;
                    assert(selected >= 0 && selected < 8 && selected != occupied);
                    HuPadBtnDown[0] = HuPadBtnDown[1] = PAD_BUTTON_A;
                    fn_1_15CB4(NULL, &lbl_1_bss_3114[0]);
                    fn_1_15CB4(NULL, &lbl_1_bss_3114[1]);
                    assert(lbl_1_bss_3114[0].unk_70[0] && lbl_1_bss_3114[1].unk_70[0]);
                    assert(lbl_1_bss_3114[0].unk_68 == selected && lbl_1_bss_3114[1].unk_68 == occupied);
                    HuPadBtnDown[0] = HuPadBtnDown[1] = PAD_BUTTON_B;
                    fn_1_15CB4(NULL, &lbl_1_bss_3114[0]);
                    fn_1_15CB4(NULL, &lbl_1_bss_3114[1]);
                    assert(!lbl_1_bss_3114[0].unk_70[0] && !lbl_1_bss_3114[1].unk_70[0]);
                    HuPadBtnDown[0] = HuPadBtnDown[1] = 0;
                }
            }
        }
    }
    /* CPU cursors use pad 0 and skip all locked characters. */
    lbl_1_bss_3114[0].unk_68 = 0;
    lbl_1_bss_3114[1].unk_68 = 1;
    lbl_1_bss_3114[0].unk_70[0] = lbl_1_bss_3114[1].unk_70[0] = 1;
    lbl_1_bss_3114[2].unk_68 = 0;
    fn_1_173FC(&lbl_1_bss_3114[2]);
    assert(lbl_1_bss_3114[2].unk_68 == 2);
    HuPadStkX[0] = -65;
    HuPadStkY[0] = 0;
    fn_1_16B68(NULL, &lbl_1_bss_3114[2]);
    assert(lbl_1_bss_3114[2].unk_68 == 2);
    HuPadStkX[0] = 0;
    HuPadStkY[0] = -65;
    fn_1_16B68(NULL, &lbl_1_bss_3114[2]);
    assert(lbl_1_bss_3114[2].unk_68 == 6);
    HuPadBtnDown[0] = PAD_BUTTON_A;
    fn_1_16B68(NULL, &lbl_1_bss_3114[2]);
    assert(lbl_1_bss_3114[2].unk_70[0] == 1);
    return 0;
}
