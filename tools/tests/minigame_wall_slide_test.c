#include <assert.h>
#include "game/mapspace.h"
static int wallChecks;
static void CheckWall(float radius, float x, float y, float z)
{
    ++wallChecks;
    AddX = 1.0f;
    AddZ = -1.0f;
}
#define MapWall CheckWall
#include "src/REL/m412Dll/main.c"

int main(void)
{
    for (int i = 0; i < 4; i++) {
        lbl_1_bss_4D4[i].unk24 = i;
        lbl_1_bss_4D4[i].unk40.z = 500 * i;
        lbl_1_bss_4D4[i].unk60.x = 20;
    }
    unkStruct3 *player = &lbl_1_bss_4D4[0];
    player->unk40.z = 10;
    player->unk60.y = 10;
    lbl_1_bss_4D4[1].unk40.z = 45;
    fn_1_5838(player);
    assert(wallChecks == 1 && player->unk40.x == 0 && player->unk40.z == 0);
    return 0;
}
