#include <assert.h>
#include <fenv.h>
#include "game/object.h"
#define omOutView(object) ((void)0)
#include "src/REL/m407dll/camera.c"

int main(void)
{
    unkDominationData2 state = {0};
    omObjData object = {0};
    Vec target = {10, 20, 30};
    float zoom = 300;
    object.data = &state;
    lbl_1_bss_30 = &object;

    feclearexcept(FE_ALL_EXCEPT);
    fn_1_1DB0(&target, NULL, &zoom, 0, 0);
    fn_1_2024(&object);
    assert(!fetestexcept(FE_DIVBYZERO | FE_INVALID));
    assert(!state.unk_04 && CZoom == zoom && Center.x == 10);

    target.x = 110;
    fn_1_1DB0(&target, NULL, NULL, 100, 1);
    for (int i = 0; i < 100; i++) {
        assert(state.unk_04);
        fn_1_2024(&object);
        if (i == 50) assert(Center.x >= 59 && Center.x <= 61);
    }
    assert(!state.unk_04 && Center.x == 110);
    return 0;
}
