#include <assert.h>
#include "game/hu3d.h"
#include "game/sprite.h"
#include "gpu_stubs.h"

static int orders[2], matrixId;
#undef GXSetTevOrder
#define GXSetTevOrder(stage, coord, texture, color) do { if ((stage) < 2) orders[(stage)] = (coord); } while (0)
#undef GXLoadTexMtxImm
#define GXLoadTexMtxImm(matrix, id, type) (matrixId = (id))
#define Hu3DLightSet(...) 0
#include "src/REL/m408Dll/stage.c"

int main(void)
{
    HSFOBJECT object = {0};
    HSFMATERIAL material = {0};
    HSFATTRIBUTE attrs[2] = {0};
    HU3DATTRANIM animation[2] = {0};
    UnkFn10484 draw = {0};
    intptr_t indices[2] = {0, 1};
    draw.unk4 = &object;
    object.mesh.attribute = attrs;
    material.attr = indices;
    material.attrNum = 2;
    attrs[0].animWorkP = &animation[0];
    attrs[1].animWorkP = &animation[1];
    animation[0].attr = 2;
    fn_1_10B60(&draw, &material);
    assert(matrixId == GX_TEXMTX1);
    assert(orders[0] == GX_TEXCOORD1 && orders[1] == GX_TEXCOORD0);
    return 0;
}
