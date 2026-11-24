#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include "game/board/space.h"
#include "game/board/player.h"
#include "game/board/main.h"

static jmp_buf positioned;
static int placements[4];

/* Stop after the board has parked its players, before creating the roulette UI. */
#define BoardCameraMoveSet(enable) longjmp(positioned, 1)
#include "src/game/board/mg_setup.c"

s32 BoardSpaceRotGet(s32 layer, s32 space, Vec *rot)
{
    *rot = (Vec){0};
    return 0;
}

void BoardSpaceCornerPosGet(s32 space, s32 corner, Vec *pos)
{
    *pos = (Vec){.x = space, .z = corner};
}

s32 BoardModelPosSetV(s16 player, Vec *pos)
{
    assert(pos->x == GWPlayer[player].space_curr);
    placements[player] = (int)pos->z;
    return 0;
}

static void EnterMinigame(s32 lastPlayer)
{
    for (int i = 0; i < 4; ++i) {
        placements[i] = -1;
        GWPlayer[i].player_idx = i;
        boardPlayerMdl[i] = i;
    }
    GWSystem.player_curr = lastPlayer;
    if (!setjmp(positioned)) {
        ExecMGSetup();
    }
    assert(GWSystem.player_curr == -1);
}

int main(void)
{
    /* The Koopa report has spaces 4, 5, 5 and 1 in turn order. */
    GWPlayer[0].space_curr = 4;
    GWPlayer[1].space_curr = GWPlayer[2].space_curr = 5;
    GWPlayer[3].space_curr = 1;
    EnterMinigame(3);
    assert(placements[0] == -1 && placements[1] == -1 && placements[2] == -1);
    assert(placements[3] == 0);

    GWPlayer[0].space_curr = GWPlayer[1].space_curr = GWPlayer[2].space_curr = 5;
    EnterMinigame(1);
    assert(placements[2] == 0 && placements[0] == 1 && placements[1] == 2);
    assert(placements[3] == -1);

    EnterMinigame(-1);
    for (int i = 0; i < 4; ++i) {
        assert(placements[i] == -1);
    }
    puts("Board minigame transition parks players on the last player's space");
    return 0;
}
