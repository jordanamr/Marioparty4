#include <assert.h>
#include "src/REL/m423Dll/main.c"

int main(void)
{
    Vec face[] = {{-10, 0, -10}, {0, 0, 0}, {10, 0, 10}};
    Vec start = {3, 5, -2}, delta = {0, -10, 0}, hit;
    assert(fn_1_137C4(face, &start, &delta, &hit, 1, 0));
    assert(hit.x == 3 && hit.y == 3 && hit.z == -2);
    return 0;
}
