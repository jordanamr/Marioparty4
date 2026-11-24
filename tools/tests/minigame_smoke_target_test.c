#include <assert.h>
#include "game/hu3d.h"
#include "game/object.h"
#include "game/sprite.h"
#include "REL/m431Dll.h"

static int paused;
#define omOutView(object) ((void)0)
#define omPauseChk() paused
#include "gpu_stubs.h"
#include "src/REL/m431Dll/main.c"

int main(void)
{
    omObjData owner = {0};
    s16 modelId = HU3D_MODELID_NONE;
    ANIMBMP bitmap = {0};
    ANIMDATA anim = { .bmp = &bitmap };
    Unk72B4Data18 particle = { .unk2A = 2, .unk2C = 1, .unk30 = fn_1_3FD0, .unk56 = 1 };
    Vec vertices[4] = {0};
    GXColor color = {0};
    HuVec2f texCoords[4] = {0}, frames[4] = {0};
    Work72B4 smoke = { .unk0 = 1, .unkC = fn_1_6030, .unk10 = &anim, .unk14 = 4, .unk18 = &particle,
        .unk1C = vertices, .unk20 = texCoords, .unk24 = &color, .unk5C = frames };
    Mtx matrix;
    MTXIdentity(matrix);
    owner.model = &modelId;
    lbl_1_bss_30 = &owner;
    M431UpdateView(NULL);
    modelId = 0;
    Hu3DData[0].hookData = &smoke;
    Hu3DData[1].pos.y = 50;
    Hu3DData[1].attr = 0;
    for (int tick = 0; tick <= 8; ++tick) {
        paused = tick == 0;
        M431UpdateView(NULL);
        for (int draw = 0; draw < tick % 3; ++draw) {
            fn_1_72B4(&Hu3DData[0], matrix);
        }
        assert(particle.unk54 == tick / 2);
        assert(particle.unk56 == (tick < 8));
        assert(Hu3DData[1].pos.y == (tick < 4 ? 50 : 1000));
        assert(!!(Hu3DData[1].attr & HU3D_ATTR_DISPOFF) == (tick >= 4));
    }
    modelId = HU3D_MODELID_NONE;
    Hu3DData[0].hookData = NULL;
    M431UpdateView(NULL);
    return 0;
}
