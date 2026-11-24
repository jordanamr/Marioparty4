#include <assert.h>
#include <math.h>
#include "types.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "port/widescreen.h"

static float widthScale;
static bool online;
static float LocalAspect(float aspect) { return aspect * widthScale; }
static float PictureX(float x) { return x + 24.0f; }
static float HudX(float x) { return x - 24.0f; }

#define PartyBoard_NetplayEnabled() online
#define PartyBoard_WidescreenPerspectiveAspect LocalAspect
#define PartyBoard_WidescreenPictureX PictureX
#define PartyBoard_WidescreenHudX HudX
#define Hu3DMtxRotGet UnusedHudTestRotation
#include "src/game/hsfex.c"

int main(void)
{
    Hu3DCamera[0].pos = (Vec){100, 500, 900};
    Hu3DCamera[0].target = (Vec){-50, 100, 0};
    Hu3DCamera[0].up = (Vec){0, 1, 0};
    Hu3DCamera[0].fov = 35;
    const float widths[] = {1.0f, 4.0f / 3.0f, 7.0f / 4.0f};
    const float slots[] = {112, 148, 184, 460, 496, 532};
    for (int slot = 0; slot < ARRAY_COUNT(slots); slot++) {
        Vec screen = {slots[slot], 88, 4000};
        Vec expected;
        online = true;
        widthScale = 1;
        Hu3D2Dto3D(&screen, 1, &expected);
        for (int w = 0; w < ARRAY_COUNT(widths); w++) {
            widthScale = widths[w];
            for (int live = 0; live < 2; live++) {
                online = live;
                Vec point = screen;
                Hu3D2Dto3DOriginal(&point, 1, &point);
                assert(point.x == expected.x && point.y == expected.y && point.z == expected.z);
                Hu3D3Dto2DOriginal(&point, 1, &point);
                assert(fabsf(point.x - screen.x) < 0.001f && fabsf(point.y - screen.y) < 0.001f);
                // Other callers retain the local projection offline.
                Hu3D2Dto3D(&screen, 1, &point);
                if (!online) assert(point.x != expected.x);
                Hu3D3Dto2D(&point, 1, &point);
                assert(fabsf(point.x - screen.x) < 0.001f && fabsf(point.y - screen.y) < 0.001f);
            }
        }
    }
    return 0;
}
