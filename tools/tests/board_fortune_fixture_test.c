#include <assert.h>
#include "game/board/fortune.h"
#include "game/board/player.h"
#include "port/netplay_runtime.h"

static int armed, fortunePlayer = -1, fortuneSpace = -1;
static Vec destination;

static bool TakeFixture(void)
{
    int take = armed;
    armed = 0;
    return take;
}
#define PartyBoard_NetplayTakeFortuneFixture() TakeFixture()
#define BoardPlayerPosSetV(player, pos) (destination = *(pos))
#define BoardFortuneExec(player, space) (fortunePlayer = (player), fortuneSpace = (space))
#include "src/game/board/space.c"

int main(void)
{
    spaceCnt[0] = 3;
    spaceData[0][0].type = 0;
    spaceData[0][2].type = 7;
    spaceData[0][2].pos = (Vec){123, 45, 67};
    GWPlayer[1].space_curr = GWPlayer[1].space_next = 1;
    GWPlayer[1].com = 1;
    armed = 1;
    BoardSpaceLandExec(1, 1);
    assert(armed && fortunePlayer == -1);
    GWPlayer[1].com = 0;
    BoardSpaceLandExec(1, 1);
    assert(!armed && fortunePlayer == 1 && fortuneSpace == 3);
    assert(GWPlayer[1].space_curr == 3 && GWPlayer[1].space_next == 3);
    assert(GWPlayer[1].fortune_count == 1);
    assert(destination.x == 123 && destination.y == 45 && destination.z == 67);
    BoardSpaceLandExec(1, 1);
    assert(GWPlayer[1].fortune_count == 1);
    return 0;
}
