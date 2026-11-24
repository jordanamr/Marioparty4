#include <assert.h>
#include "src/game/board/item.c"

int main(void)
{
    s16 delay = 2;
    GWPlayer[2].port = 3;
    HuPadBtnDown[2] = PAD_BUTTON_A;
    assert(!ItemBagJumpPressed(2, &delay));
    HuPadBtnDown[3] = PAD_BUTTON_A | PAD_BUTTON_RIGHT;
    assert(ItemBagJumpPressed(2, &delay));
    assert(delay == 2);

    GWPlayer[2].com = 1;
    for (int reward = 0; reward < 3; reward++) {
        while (delay > 1) {
            assert(!ItemBagJumpPressed(2, &delay));
        }
        assert(ItemBagJumpPressed(2, &delay));
        assert(delay >= 30 && delay < 70);
    }
    return 0;
}
