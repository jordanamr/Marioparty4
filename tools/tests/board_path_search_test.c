/* Regression tests exercise the same graph builder and search used by board CPUs. */
#include <assert.h>
#include <stdio.h>
#include "game/board/space.h"
#include "game/board/com_path.h"
#include "game/flag.h"

#include "src/game/board/com_path.c"

static BoardSpace spaces[40];

s32 BoardSpaceCountGet(s32 layer) { return 40; }

s32 _CheckFlag(u32 flag) { return 0; }

BoardSpace *BoardSpaceGet(s32 layer, s32 id)
{
    assert(layer == 0 && id > 0 && id <= 40);
    return &spaces[id - 1];
}

u32 BoardSpaceFlagGet(s32 layer, s32 id) { return BoardSpaceGet(layer, id)->flag; }
s32 BoardSpaceTypeGet(s32 layer, s32 id) { return BoardSpaceGet(layer, id)->type; }

static void Link(int start, int end)
{
    BoardSpace *space = BoardSpaceGet(0, start);
    space->link[space->link_cnt++] = end;
}

int main(void)
{
    /* At the junction the last branch is a dead end; the first reaches the star. */
    Link(1, 2);
    Link(1, 3);
    BoardSpaceGet(0, 2)->type = 8;
    BoardSpaceGet(0, 3)->type = 1;
    assert(BoardComPathBestGet(1) == 2);
    BoardSpaceGet(0, 2)->flag = 0x200000;
    assert(BoardComPathBestGetFlag(1, 0x200000, 10) == 2);
    assert(BoardComPathBestGetFlag(1, 0x400000, 10) == -1);

    /* A junction that points back to itself is legal on a circular route. */
    Link(3, 3);
    Link(3, 4);
    assert(BoardComPathBestGetFlag(3, 0x10000000, 10) == -1);
    assert(BoardComPathLenGet(3, 8) == 0);
    BoardSpaceGet(0, 4)->flag = 0x10000000;
    assert(BoardComPathBestGetFlag(3, 0x10000000, 10) == 4);

    /* The shortcut via a pipe cannot be used by a full-size player. */
    Link(5, 6);
    Link(6, 7);
    Link(6, 8);
    BoardSpaceGet(0, 6)->flag = 0x20000000;
    BoardSpaceGet(0, 7)->type = 8;
    assert(BoardComPathShortcutLenGet(5, 8, 0) == 0);
    assert(BoardComPathShortcutLenGet(5, 8, 1) == 2);

    /* Transit spaces do not consume moves, even in a long single segment. */
    for (int i = 8; i < 40; ++i) Link(i, i + 1);
    BoardSpaceGet(0, 40)->type = 8;
    Link(4, 8);
    Link(4, 2);
    assert(BoardComPathBestGet(4) == 8);
    /* The shorter branch is not the first path on the board. */
    memset(spaces, 0, sizeof(spaces));
    Link(1, 2);
    Link(1, 5);
    Link(2, 3);
    Link(3, 4);
    Link(4, 7);
    Link(5, 7);
    BoardSpaceGet(0, 7)->type = 8;
    assert(BoardComPathLenGet(1, 7) == 2);
    assert(BoardComPathShortcutLenGet(1, 8, 1) == 2);
    BoardSpaceGet(0, 1)->link[0] = 5;
    BoardSpaceGet(0, 1)->link[1] = 2;
    assert(BoardComPathLenGet(1, 7) == 2);
    assert(BoardComPathShortcutLenGet(1, 8, 1) == 2);
    puts("PASS board CPU paths: dead ends, target flags, cycles, pipes and transit spaces");
}
