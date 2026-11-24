#include <assert.h>
#include <math.h>
#include "src/REL/m414Dll/main.c"

static float aspectScale;
static Vec shown;
static s16 shownGroup;

float PartyBoard_WidescreenAspectScale(void) { return aspectScale; }
void HuSprGrpPosSet(s16 group, float x, float y)
{
    shownGroup = group;
    shown = (Vec){x, y, 0};
}

int main(void)
{
    const float widths[] = {640, 854, 1120};
    const Vec offsets[] = {{0, 0, 0}, {-120, 80, 0}, {240, -140, 0}};
    UnkM414BssF0Struct player = {0};

    for (int i = 0; i < 4; i++) {
        const float vx = (i & 1) ? 322 : 10;
        const float cameraX = i * 2000;
        HU3DCAMERA *cam = &Hu3DCamera[i];
        cam->pos = (Vec){cameraX, 0, 600};
        cam->target = (Vec){cameraX, 0, 0};
        cam->up = (Vec){0, 1, 0};
        cam->fov = 90;
        cam->aspect = HU_DISP_ASPECT;
        cam->viewportX = vx;
        cam->viewportW = 309;
        player.unk_44 = player.unk_48 = i;
        player.unk_2C = 10 + i;

        for (int w = 0; w < 3; w++) {
            aspectScale = widths[w] / HU_FB_WIDTHF;
            for (int j = 0; j < 3; j++) {
                Vec aim = offsets[j];
                aim.x += cameraX;
                shownGroup = -1;
                ClawReticlePositionSet(&player, &aim);
                assert(shownGroup == 10 + i);
                /* Compare final pixels with the independent 90-degree perspective and
                 * viewport transform. The optical axis lands at each view's center. */
                const float pixels = widths[w] * 0.5f + (shown.x - HU_DISP_CENTERX)
                    * widths[w] / (HU_DISP_WIDTHF * aspectScale);
                const float expected = aspectScale * (vx + 309 * 0.5f)
                    + offsets[j].x / (600 * HU_DISP_ASPECT) * 309 * 0.5f;
                assert(fabsf(pixels - expected) < 0.001f);
                assert(fabsf(shown.y - (225 * (i / 2) + 105 - offsets[j].y * 0.2f)) < 0.001f);
            }
        }
    }
    return 0;
}
