#include <assert.h>
#include "game/board/main.h"
#include "game/audio.h"
static int IgnoreSound(int id) { return id; }
#define HuAudFXPlay IgnoreSound
#define BoardIsKill() 0
#include "src/game/board/lottery.c"

int main(void)
{
    omObjData cursor = {0};
    LotteryTicketPickWork *ticket = OM_GET_WORK_PTR(&cursor, LotteryTicketPickWork);
    ticket->unk02 = 2;
    ticket->unk03 = 1;
    GWSystem.player_curr = 2;
    GWPlayer[2].port = 1;
    HuPadDStkRep[1] = PAD_BUTTON_DOWN;
    HuPadBtnDown[1] = PAD_BUTTON_A;
    ExecScratchPick(&cursor);
    assert(currPrize == 6 && ticket->unk00_field0);
    return 0;
}
