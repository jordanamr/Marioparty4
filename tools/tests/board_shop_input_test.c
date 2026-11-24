#include <assert.h>
#include "game/audio.h"
#include "port/version_runtime.h"
#undef VERSION_RT_NTSC
#define VERSION_RT_NTSC 0
static int sound;
#define HuAudFXPlay(id) (sound = (id))
#include "src/game/board/shop.c"

int main(void)
{
    omObjData cursor = {0};
    ItemChoiceWork choice = {0};
    choice.unk00_field1 = 2;
    choice.unk02 = 3;
    GWPlayer[2].port = 1;
    HuPadDStkRep[1] = PAD_BUTTON_DOWN;
    HuPadBtnDown[2] = PAD_BUTTON_A;
    MoveShopItemChoice(&cursor, &choice);
    assert(choice.unk02 == 4 && sound == 0);

    HuPadBtnDown[1] = PAD_BUTTON_A;
    choiceEnableTbl[4] = 0;
    MoveShopItemChoice(&cursor, &choice);
    assert(choice.unk02 == 4 && sound == 4 && !choice.unk05);
    choiceEnableTbl[4] = 1;
    MoveShopItemChoice(&cursor, &choice);
    assert(itemChoice == 4 && sound == 2 && choice.unk05 == 5);

    HuPadBtnDown[1] = PAD_BUTTON_B;
    MoveShopItemChoice(&cursor, &choice);
    assert(itemChoice == 0x7F && sound == 3 && choice.unk02 == 4);
    return 0;
}
