#include <assert.h>
#include <math.h>
#include "src/REL/m444dll/pinball.c"

int main(void)
{
    Vec position = {20, 25, 0};
    Vec velocity = {0, -0.3f, 0};
    lbl_1_bss_770 = (Vec){-1000, -1000, 0};
    lbl_1_bss_77C = (Vec){1000, 1000, 0};

    /* Away from pockets, gravity first brings the ball to rest. The REV2
     * anti-stall rule then accelerates it for the next physics step. */
    assert(fn_1_B1E8(&position, &velocity, 0) == -1);
    assert(fabsf(velocity.y - 0.3f) < 0.000001f);
    assert(velocity.x == 0 && velocity.z == 0);

    velocity = (Vec){0, 0.7f, 0};
    assert(fn_1_B1E8(&position, &velocity, 0) == -1);
    assert(fabsf(velocity.y - 1.0f) < 0.000001f);
    return 0;
}
