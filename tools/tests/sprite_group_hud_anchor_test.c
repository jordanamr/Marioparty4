#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "src/game/sprman.c"

/* Supply the displacement of a known window. The real layout decides which
 * coordinate to use and whether every member receives the same transform. */
static float extraWidth;
float PartyBoard_WidescreenHudShift(float x)
{
    float weight = fminf(1.0f, fmaxf(0.0f, (fabsf(x - 288.0f) / 288.0f - 0.5f) * 4.0f));
    return copysignf(extraWidth * weight, x - 288.0f);
}

int main(void)
{
    ANIMFRAME frame = {0};
    ANIMBANK bank = {.frame = &frame};
    ANIMPAT pattern = {.sizeX = 32, .sizeY = 32, .centerX = 16, .centerY = 16};
    ANIMDATA data = {.pat = &pattern, .bank = &bank};
    s16 members[] = {1, 2, 3};
    HuSprGrpData[0].capacity = 3;
    HuSprGrpData[0].members = members;
    HuSprRenderMtx[0][0][0] = HuSprRenderMtx[0][1][1] = 1;
    for (int i = 0; i < 3; i++) {
        HuSprData[members[i]].data = &data;
        HuSprData[members[i]].scale = (HuVec2f){1, 1};
        HuSprData[members[i]].pos.x = (float)(i * 28);
    }
    const float scales[] = {1.0f, 4.0f / 3.0f, 7.0f / 4.0f};
    for (int i = 0; i < 3; i++) {
        for (int originalHud = 0; originalHud <= 1; originalHud++) {
            extraWidth = originalHud ? 0.0f : 288.0f * (scales[i] - 1.0f);
            /* Right player in the announcement's two-column layout. */
            HuSprRenderMtx[0][0][3] = 406;
            HuSprGrpHudAnchorToOrigin(0);
            HuSprWidescreenLayout();
            for (int m = 0; m < 3; m++) assert(HuSprWideShift[members[m]] == 0);
            /* The same group leaves the expanded screen as one panel. */
            HuSprRenderMtx[0][0][3] = 674;
            HuSprWidescreenLayout();
            for (int m = 0; m < 3; m++) assert(HuSprWideShift[members[m]] == extraWidth);
            /* A world-projected reward above the outside player stays over
             * that player even though its digits reach the right HUD edge. */
            HuSprRenderMtx[0][0][3] = 475;
            HuSprGrpHudAnchorSet(0, HU_DISP_CENTERX);
            HuSprWidescreenLayout();
            for (int m = 0; m < 3; m++) {
                assert(HuSprWideShift[members[m]] == 0);
                assert(HuSprWideStretch[members[m]][0] == 1);
            }
            HuSprGrpHudAnchorReset(0);
            HuSprWidescreenLayout();
            assert(HuSprWideShift[1] == extraWidth);
        }
    }
    puts("announcement and reward group anchors: six layouts passed");
    return 0;
}
