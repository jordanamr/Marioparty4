#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "port/netplay_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gpu_stubs.h"

static float waterY[5];
static int paused;

static void RecordModelPosition(s16 model, float x, float y, float z)
{
    if (model >= 10 && model < 15) {
        waterY[model - 10] = y;
    }
}

#define Hu3DModelPosSet RecordModelPosition
#define omPauseChk() paused
#include "src/REL/m456Dll/stage.c"

static void RunStage(int variableDraws, float reference[32][5])
{
    Work2FAC stage = { 0 };
    StructBss90 lighting = { 0 };
    omObjData object = { 0 };
    HU3DMODEL model = { 0 };
    HU3DPARTICLE flare = { 0 };
    HU3DPARTICLEDATA particles[10] = { 0 };
    s16 flareWork[4] = { 0, 1, 1, 0 };
    s16 models[15];
    Mtx matrix;

    for (int i = 0; i < 15; ++i) {
        models[i] = i;
    }
    object.data = &stage;
    object.model = models;
    model.hookData = &stage;
    /* The paired reports enter their first unequal wave update at angle 4. */
    stage.unkA50 = 4;
    stage.unk74[0].unk1C = stage.unk74[0].unk1E = 10;
    stage.unk74[0].unk18 = 0.1f;
    lbl_1_bss_90 = &lighting;
    flare.data = particles;
    flare.maxCnt = 10;
    flare.work = flareWork;
    MTXIdentity(matrix);
    MTXIdentity(Hu3DCameraMtx);
    Hu3DCamera[0].fov = 45;
    Hu3DCameraNo = 0;

    frand_state_set(0xde292ba6u);
    PartyBoard_NetplayRandomCountersReset();
    InitLensFlare(&flare, 1);
    assert(PartyBoard_NetplayFrandCalls() == 60);
    for (int tick = 0; tick < 32; ++tick) {
        float uv[26][2];
        int draws = variableDraws ? (tick * 3) % 7 : 1;
        paused = tick % 4 == 1;
        fn_1_390C(&object);
        assert(PartyBoard_NetplayFrandCalls() == 60u + (tick + 1) * 53u);
        if (tick == 0) {
            u32 heightBits;
            memcpy(&heightBits, &waterY[0], sizeof(heightBits));
            assert(heightBits == 0x41cb7c7au);
        }
        for (int i = 1; i < 5; ++i) {
            assert(waterY[i] == waterY[0]);
        }
        if (variableDraws) {
            assert(memcmp(reference[tick], waterY, sizeof(waterY)) == 0);
        } else {
            memcpy(reference[tick], waterY, sizeof(waterY));
        }

        u32 seed = frand_state_get();
        u16 life = stage.unk74[0].unk1E;
        memcpy(uv, stage.unk874, sizeof(uv));
        for (int draw = 0; draw < draws; ++draw) {
            fn_1_3CB0(&model, matrix);
            fn_1_4994(&model, matrix);
            fn_1_8CB8(&model, &flare, matrix);
        }
        assert(frand_state_get() == seed);
        assert(stage.unk74[0].unk1E == life);
        assert(memcmp(uv, stage.unk874, sizeof(uv)) == 0);
    }
    assert(stage.unk74[0].unk1E == 0);
    assert(frand_state_get() == 0xc5aaf8e4u);
}

int main(void)
{
    float reference[32][5];
    RunStage(0, reference);
    RunStage(1, reference);
    puts("Take a Breather: identical RNG and five water heights with zero to six draws per tick");
    return 0;
}
