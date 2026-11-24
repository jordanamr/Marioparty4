#include <assert.h>
#include "game/board/player.h"
static int lookups;
static int FindItem(s32 player, s32 item)
{
    assert(player == 2);
    ++lookups;
    return item == 3 ? 1 : -1;
}
#define BoardPlayerItemFind FindItem
#include "src/game/board/com.c"

int main(void)
{
    GWPlayer[2].com = 1;
    boardTutorialUseItem = -1;
    BoardComUseItemSet(2, -1);
    assert(BoardComUseItemSlotGet(2) == -1 && !lookups);
    assert(BoardComUseItemSet(2, 3) && BoardComUseItemSlotGet(2) == 1);
    assert(lookups == 2);
    boardTutorialUseItem = 2;
    assert(BoardComUseItemSlotGet(2) == -1 && boardTutorialUseItem == -1);
    assert(BoardComUseItemSlotGet(2) == 1);
    GWPlayer[2].com = 0;
    assert(BoardComUseItemSlotGet(2) == -1 && lookups == 4);
    return 0;
}
