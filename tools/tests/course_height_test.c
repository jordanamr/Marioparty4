#include <assert.h>
#include <stdio.h>
#include "src/REL/m427Dll/player.c"

int main(void)
{
    enum { pointCount = sizeof(lbl_1_data_260) / sizeof(lbl_1_data_260[0]) };
    Vec centers[pointCount];
    M427DllBss590Struct path[pointCount] = { 0 };
    lbl_1_bss_580 = pointCount;
    lbl_1_bss_590 = path;
    for (int i = 0; i < pointCount; i++) {
        centers[i].z = (lbl_1_data_260[i].z + lbl_1_data_3BC[i].z) / 2;
        centers[i].y = (lbl_1_data_260[i].y + lbl_1_data_3BC[i].y) / 2;
        path[i].unk_80 = &centers[i];
    }

    const Vec start = centers[0];
    const Vec end = centers[pointCount - 1];
    float fraction = -1;
    assert(fn_1_155E0(0, 0, start.z + 1, &fraction) == 0 && fraction == 0);
    assert(fn_1_15714(0, start.z) == start.y);
    assert(fn_1_15714(0, start.z + 1) == start.y);
    for (int i = 0; i < pointCount - 1; i++) {
        float midpoint = (centers[i].z + centers[i + 1].z) / 2;
        assert(fn_1_155E0(0, 0, midpoint, &fraction) == i);
        assert(fraction > 0.49f && fraction < 0.51f);
    }
    assert(fn_1_15714(0, end.z - 1) == end.y);
    puts("Course height: start, finish and all intermediate segments.");
    return 0;
}
