#include <assert.h>
#include "game/esprite.h"
#include "game/memory.h"

static s16 released[6];
static int count;
static int freed;

static void RecordKill(s16 id)
{
    assert(count < 6);
    released[count++] = id;
}

#define espKill RecordKill
#define HuMemDirectFree(data) ((void)(data), (void)(freed++))
#include "src/REL/m453Dll/score.c"

int main(void)
{
    omObjData score = {0};
    M453ScoreUnkStruct work = {0};
    score.data = &work;
    lbl_1_bss_C0 = &score;
    for (int i = 0; i < 6; i++) {
        work.unk_0C[i] = i + 21;
    }
    fn_1_91D8();
    assert(count == 6 && freed == 1);
    for (int i = 0; i < 6; i++) {
        assert(released[i] == i + 21);
    }
    return 0;
}
