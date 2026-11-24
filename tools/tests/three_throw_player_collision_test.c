#include <assert.h>
#include "src/REL/m410Dll/game.c"

int main(void)
{
    omObjData player = {0};
    UnkM410Struct2 ball = {0};
    player.trans = (Vec){150, 10, -80};
    ball.unk_04 = (Vec){150, 50, -60};
    ball.unk_1C = (Vec){2, -4, -8};
    fn_1_66B8(&ball, &player);
    assert(ball.unk_1C.x == 1 && ball.unk_1C.y == -2 && ball.unk_1C.z == 4);

    ball.unk_04 = (Vec){120, -50, -80};
    ball.unk_1C = (Vec){6, 2, 4};
    fn_1_66B8(&ball, &player);
    assert(ball.unk_1C.x == -3 && ball.unk_1C.y == 1 && ball.unk_1C.z == 2);
    return 0;
}
