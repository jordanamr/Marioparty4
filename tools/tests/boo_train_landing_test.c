#include <assert.h>
#include <stdio.h>
#include "game/board/player.h"
#include "game/board/space.h"
#include "dolphin.h"

#undef OSReport
#define OSReport(...) ((void)0)
#define BoardPlayerPosGet(player, pos) ((void)(*(pos) = (Vec){0}))
#define BoardPlayerRotGet BoardPlayerPosGet
#define BoardPlayerPosSetV(player, pos) ((void)0)
#define BoardPlayerRotSetV BoardPlayerPosSetV
#include "src/REL/w04Dll/boo_event.c"

static BoardSpace board[] = { { .link_cnt = 1, .link = {2} }, { .flag = 2 } };
BoardSpace *BoardSpaceGet(s32 layer, s32 id) { return &board[id - 1]; }
void BoardSpaceCornerPosGet(s32 space, s32 corner, Vec *pos)
{
    assert(space == 2 && corner >= 0 && corner < 4);
    assert(pos->x == -1);
    pos->x = corner;
}
s32 BoardSpacePosGet(s32 layer, s32 space, Vec *pos)
{
    assert(space == 2);
    pos->x = 4;
    return 0;
}

int main(void)
{
    BssE4Data slots[4] = {0};
    Process processes[4] = {0};
    for (int i = 0; i < 4; i++) {
        lbl_1_bss_E4[i] = &processes[i];
        processes[i].user_data = &slots[i];
    }
    for (int current = 0; current < 4; current++) {
        GWSystem.player_curr = current;
        for (int code = 0; code < 625; code++) {
            int owners = 0;
            int digits = code;
            for (int i = 0; i < 4; i++, digits /= 5) {
                slots[i].unk08 = digits % 5 - 1;
                if (slots[i].unk08 >= 0) owners |= 1 << slots[i].unk08;
            }
            for (int occupied = 0; occupied < 16; occupied++) {
                Vec result[4];
                for (int i = 0; i < 4; i++) {
                    GWPlayer[i].space_curr = (occupied & (1 << i)) ? 2 : 1;
                    result[i].x = -1;
                }
                fn_1_52A0(1, result);
                int used = 0;
                for (int i = 0; i < 4; i++) {
                    if (i == current) {
                        assert(result[i].x == 4);
                    } else if (owners & (1 << i)) {
                        int corner = (int)result[i].x;
                        assert(corner >= 0 && corner < 4 && !(used & (1 << corner)));
                        used |= 1 << corner;
                    } else {
                        assert(result[i].x == -1);
                    }
                }
            }
        }
    }
    puts("PASS Boo train landing: 40000 occupancy and passenger combinations");
}
