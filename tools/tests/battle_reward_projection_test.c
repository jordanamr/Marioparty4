#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "src/REL/resultDll/battle.c"

s32 resultFastF = 1;
s16 resultBonusPlayer = 1;
static Vec shown[4];
static float aspectScale;
static bool online;
static bool originalHud;

bool PartyBoard_NetplayEnabled(void) { return online; }
float PartyBoard_WidescreenPerspectiveAspect(float aspect) { return aspect * aspectScale; }
float PartyBoard_WidescreenHudX(float x) { return originalHud ? x : x + 10.0f; }

void ResultCoinNumGet(s16 *values)
{
    const s16 amounts[] = {0, 8, 42, 105, 1};
    memcpy(values, amounts, sizeof(amounts));
}
void HuPrcVSleep(void) {}
void HuSprBankSet(s16 group, s16 member, s16 bank) {}
void HuSprAttrSet(s16 group, s16 member, s32 attr) {}
void HuSprAttrReset(s16 group, s16 member, s32 attr) {}
void HuSprGrpScaleSet(s16 group, float sx, float sy) {}
void HuSprGrpPosSet(s16 group, float x, float y) { shown[group] = (Vec){ x, y, 0 }; }

int main(void)
{
    Hu3DCamera[0].pos = (Vec){0, 0, 600};
    Hu3DCamera[0].target = (Vec){0, 0, 0};
    Hu3DCamera[0].up = (Vec){0, 1, 0};
    Hu3DCamera[0].fov = 90;
    for (int i = 0; i < 4; i++) {
        coinNumGrpId[i] = i;
        resultBattleOrder[i] = i;
        resultBattleRank[i] = 3 - i;
        GWPlayerCfg[i].character = i;
    }
    /* Characters are 600 world units away. A 90 degree FOV maps 600
     * vertical units to half the 480-line original picture. */
    const float scales[] = {1.0f, 4.0f / 3.0f, 7.0f / 4.0f};
    for (int i = 0; i < 3; i++) {
        aspectScale = scales[i];
        for (int mode = 0; mode < 4; mode++) {
            online = (mode & 1) != 0;
            originalHud = (mode & 2) != 0;
            ResultBattleCoinAddShow();
            for (int player = 0; player < 4; player++) {
                float head = player == 0 ? coinBlankYOfs[player] : coinAddYOfs[player];
                assert(fabsf(shown[player].x - (288 + playerPosTbl[player].x * 0.4f)) < 0.001f);
                assert(fabsf(shown[player].y - (240 - (playerPosTbl[3 - player].y + head) * 0.4f)) < 0.001f);
            }
        }
    }
    return 0;
}
