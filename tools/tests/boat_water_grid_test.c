#include <assert.h>
#include "src/REL/m427Dll/map.c"

int main(void)
{
    Vec river[] = {{0, 1, 0}, {10, 3, 0}, {0, 5, 10}, {10, 7, 10}};
    Vec finish[] = {{10, 7, 10}, {0, 5, 10}, {10, 3, 0}, {0, 1, 0}};
    M427DllMapStruct6 grid = {0};
    grid.unk_2C = river;
    grid.unk_38 = 4;
    grid.unk_3A = 2;
    lbl_1_bss_158.unk4[0].unkAC = &grid;
    assert(fn_1_AE78(0, -1, 0) == 21);
    assert(fn_1_AE78(0, 5, 0) == 22);
    assert(fn_1_AE78(0, 0, 5) == 25);
    assert(fn_1_AE78(0, 5, 5) == 26);
    assert(lbl_1_data_1EC[0] == 0);
    lbl_1_data_1EC[1] = 100;
    lbl_1_bss_158.unk0 = 1;
    lbl_1_bss_98.unk4 = finish;
    lbl_1_bss_98.unk14 = 4;
    lbl_1_bss_98.unk16 = 2;
    assert(fn_1_AE78(0, 12, 15) == 27);
    assert(fn_1_AE78(1, 5, 15) == 26);
    return 0;
}
