#include <assert.h>
#include <stdio.h>
#include "game/hu3d.h"
#include "game/object.h"
#include "src/REL/m433Dll/player.c"

int main(void)
{
    HU3DPARTICLEDATA data = {0};
    HU3DPARTICLE effect = {0};
    omObjData object = {0};
    s16 models[9] = {0};
    Vec hit = {12.0f, 3.0f, -4.0f};
    HU3DMODEL *model = &Hu3DData[0];

    lbl_1_bss_7CC = &object;
    object.model = models;
    model->hookData = &effect;
    effect.maxCnt = 1;
    effect.data = &data;
    for (int activation = 0; activation < 2; ++activation) {
        fn_1_DD70(&hit);
        for (int remaining = REFRESH_RATE; remaining > 0; --remaining) {
            assert(!(model->attr & HU3D_ATTR_DISPOFF));
            fn_1_DE28(model, &effect, NULL);
            assert(data.scale == 100.0f && data.pos.z == hit.z);
            assert(data.color.a == remaining * 255 / REFRESH_RATE);
            assert(effect.emitCnt == remaining - 1);
        }
        assert(model->attr & HU3D_ATTR_DISPOFF);
    }
    puts("Impact particle fade and reactivation passed");
    return 0;
}
