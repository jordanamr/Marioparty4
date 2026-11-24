#include <assert.h>
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/audio.h"
#include "game/sprite.h"

static int draws, restarts;
static float DrawAngle(void) { ++draws; return 0.5f; }
#define frandf() DrawAngle()
#define Hu3DMotionTimeSet(model, time) (++restarts)
#define Hu3DModelAttrReset(model, attr) ((void)0)
#define Hu3DModelAttrSet(model, attr) ((void)0)
#define Hu3DMotionEndCheck(model) 0
#define HuAudFXPlay(sound) 0
#include "gpu_stubs.h"
#include "src/REL/m450Dll/main.c"

static void CheckTimer(unsigned initial, float clock, unsigned phase, unsigned expected)
{
    omObjData effect = {0}, director = {0};
    lbl_1_bss_1A4 = &director;
    director.work[0] = phase;
    lbl_1_bss_128 = clock;
    effect.work[0] = initial;
    draws = restarts = 0;
    fn_1_12688(&effect);
    assert(effect.work[0] == expected);
    assert(draws == (expected == 0) && restarts == draws);
    if (expected == 0) {
        fn_1_12688(&effect);
        assert(draws == 1 && restarts == 1);
    }
}

int main(void)
{
    CheckTimer(1, 0, 0x3EB, 0);      /* The accelerated countdown crosses zero. */
    CheckTimer(3, 30, 0x3EB, 0);     /* A positive fraction truncates to zero. */
    CheckTimer(6, 60, 0x3EB, 2);
    CheckTimer(6, 60, 0x3EC, 5);     /* Subsequent phases count one tick at a time. */
    return 0;
}
