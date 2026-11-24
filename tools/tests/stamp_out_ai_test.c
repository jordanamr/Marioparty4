#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "src/REL/m415Dll/main.c"

int main(void)
{
    static u8 paint[600 * 600];
    s16 model[1] = { 0 };
    unkStruct3 cpu = { 0 };
    omObjData player = { 0 };
    player.model = model;
    player.data = &cpu;
    lbl_1_bss_334 = paint;
    memset(paint, 1, sizeof(paint));

    // More bare pixels at (0, 0), but only (0, 2) neighbours the CPU at (-550, -350).
    for (int z = 0; z < 50; z++) {
        memset(&paint[z * 600], 0, 50);
    }
    for (int z = 100; z < 150; z++) {
        memset(&paint[z * 600], 0, 40);
    }
    cpu.unk5D = 1;
    cpu.unk2C.x = -550;
    cpu.unk2C.z = -350;
    fn_1_4660(&player);
    assert(cpu.unk6C.x == -550 && cpu.unk6C.z == -350);
    puts("Stamp Out: CPU targets the neighbouring unpainted tile on the Z axis.");
    return 0;
}
