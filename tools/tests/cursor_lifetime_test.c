#define HUSPR_USE_OLD_DEFS
#include "REL/m409Dll.h"
#include "game/hu3d.h"
#include "game/sprite.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int liveBlocks;
static int updates;
static int paused;
static void *Allocate(size_t size)
{
    ++liveBlocks;
    return calloc(1, size);
}
static void Release(void *block)
{
    assert(block && liveBlocks > 0);
    --liveBlocks;
    free(block);
}
#define HuMemDirectMallocNum(heap, size, num) Allocate(size)
#define HuMemDirectFree Release
#define Hu3DLightSet(...) 0
#define Hu3DModelKill(id) ((void)0)
#define omPauseChk() paused
#include "gpu_stubs.h"
#include "src/REL/m409Dll/cursor.c"

unkStruct9 lbl_1_bss_F0;

static void CountUpdate(unkStruct8 *cursor)
{
    ++updates;
}

static void RunCursor(int variableDraws)
{
    unkStruct8 slots[2] = { 0 };
    unkStruct9Sub textures[2] = { 0 };
    ANIMBMP bmp = { 0 };
    ANIMDATA anim = { 0 };
    Mtx matrix;
    MTXIdentity(matrix);
    MTXIdentity(Hu3DCameraMtx);
    anim.bmp = &bmp;
    textures[1].unk0 = &anim;
    textures[1].unk8 = 123;
    textures[1].unk6 = 1;
    textures[1].unk4C = 3;
    lbl_1_bss_F0.unk0 = lbl_1_bss_F0.unk2 = 2;
    lbl_1_bss_F0.unk10 = slots;
    lbl_1_bss_F0.unkC = textures;

    s16 id = fn_1_DA48(1, 1);
    assert(id == 1 && fn_1_E2A8(id, 0, 123, 64, 64));
    slots[id].unk8 |= 2;
    slots[id].unk30 = CountUpdate;
    slots[id].unk3C->unk34 |= 0x80000000;
    slots[id].unk3C->unk54->unk8 = 0x11; /* play once and expire */
    assert(liveBlocks == 8);
    updates = 0;
    for (int tick = 0; tick < 6; ++tick) {
        paused = tick == 1;
        M409UpdateCursors();
        int expectedUpdates = tick == 0 ? 1 : tick < 3 ? tick : 3;
        assert(updates == expectedUpdates);
        assert(liveBlocks == (tick < 3 ? 8 : 0));
        int draws = variableDraws ? (tick * 2) % 5 : 1;
        for (int i = 0; i < draws; ++i) {
            fn_1_CB98(NULL, matrix);
        }
        assert(updates == expectedUpdates);
        assert(liveBlocks == (tick < 3 ? 8 : 0));
    }
}

int main(void)
{
    /* The final manager callback can still run after overlay cleanup. */
    M409UpdateCursors();
    lbl_1_bss_F0.unk0 = lbl_1_bss_F0.unk2 = 2;
    lbl_1_bss_F0.unk10 = Allocate(2 * sizeof(*lbl_1_bss_F0.unk10));
    lbl_1_bss_F0.unkC = Allocate(2 * sizeof(*lbl_1_bss_F0.unkC));
    fn_1_C0C0();
    assert(liveBlocks == 0 && lbl_1_bss_F0.unk0 == 0);
    M409UpdateCursors();
    RunCursor(0);
    RunCursor(1);
    puts("Cursor: all eight live blocks released on the same tick at both render rates");
    return 0;
}
