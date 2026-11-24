#include <assert.h>
#include <stdio.h>
#include "src/REL/m462Dll/main.c"

int main(void)
{
    StructBss70 *moving = &lbl_1_bss_70[0];
    for (int i = 0; i < 10; ++i) {
        lbl_1_bss_70[i].unk0C = i;
        lbl_1_bss_70[i].unk30 = 45.0f;
        lbl_1_bss_70[i].unk14.x = 250.0f + i;
    }
    moving->unk34 = 4.0f;
    moving->unk14.x = 4.0f;
    lbl_1_bss_340[0].unk84 = 50.0f;
    assert(fn_1_1880(4.0f, 0.0f, 50.0f, 10, 0));
    fn_1_1E10(moving);
    assert(moving->unk14.x == 4.0f && moving->unk14.z == 0.0f);
    puts("PASS proximity to player without creature collision preserves movement");
}
