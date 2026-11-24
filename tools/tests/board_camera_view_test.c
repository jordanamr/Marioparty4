#include <assert.h>
#include "game/board/player.h"
#include "game/flag.h"

static int size, reads;
static s32 PlayerSize(s32 player)
{
    assert(player == 2);
    ++reads;
    return size;
}
static void PlayerPos(s32 player, Vec *pos)
{
    assert(player == 2);
    ++reads;
    *pos = (Vec){10, 20, 30};
}
#define BoardPlayerSizeGet PlayerSize
#define BoardPlayerPosGet PlayerPos
#define _SetFlag(flag) ((void)0)
#include "src/game/board/main.c"

int main(void)
{
    omObjData object = {0};
    cameraObj = &object;
    boardCamera.target_space = 6;
    boardCamera.target_mdl = -1;
    boardCamera.target = (Vec){130, 150, -80};
    boardCamera.offset.y = 60;
    GWSystem.player_curr = -1;
    BoardCameraViewSet(2);
    assert(!reads && boardCamera.focus.zoom_end == 2100);
    assert(boardCamera.focus.target_end.x == 130 && boardCamera.focus.target_end.y == 150 && boardCamera.focus.target_end.z == -80);
    assert(boardCamera.target_space == 6 && boardCamera.target_mdl == -1 && boardCamera.offset.y == 60);

    GWSystem.player_curr = 2;
    GWPlayer[2].player_idx = 2;
    boardPlayerMdl[2] = 19;
    for (int mode = 0; mode < 3; ++mode) {
        size = mode == 1 ? 2 : 0;
        GWPlayer[2].bowser_suit = mode == 2;
        BoardCameraViewSet(2);
        assert(boardCamera.target_space == -1 && boardCamera.target_mdl == 19);
        assert(boardCamera.focus.target_end.y == (mode ? 270 : 120));
        assert(boardCamera.focus.zoom_end == (mode ? 2500 : 2100));
    }
    assert(reads == 6);
    return 0;
}
