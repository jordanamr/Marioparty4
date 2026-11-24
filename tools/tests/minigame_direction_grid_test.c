#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "src/REL/m421Dll/player.c"

int main(void)
{
    M421DllPlayerWork work = {0};
    omObjData object = {0};
    float east[21 * 21], north[21 * 21];
    object.data = &work;
    work.unk_148 = east;
    work.unk_D8.x = 1.0f;
    fn_1_7AA0(&object);
    work.unk_148 = north;
    work.unk_D8.x = 0.0f;
    work.unk_D8.z = 1.0f;
    fn_1_7AA0(&object);
    for (int z = 0; z < 21; ++z) {
        for (int x = 0; x < 21; ++x) {
            assert(fabsf(east[z * 21 + x] - north[x * 21 + z]) < 0.00001f);
        }
    }
    puts("PASS m421 CPU movement prediction rotates with its input on all 441 cells");
}
