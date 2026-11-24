#include <assert.h>
#include <setjmp.h>
#include "game/board/boo.h"
#include "game/board/model.h"

static omObjData boo;
static jmp_buf atModelLoad;
static int created;

static s16 CreateItemBoo(s32 player, Vec *origin)
{
    assert(player == 2 && origin == NULL);
    return BoardBooCreate(player, origin);
}

#define BoardBooCreate CreateItemBoo
#include "src/game/board/item.c"

void BoardAudSeqPause(s32 seq, s32 pause, u16 speed) {}

omObjData *omAddObjEx(Process *manager, s16 prio, u16 models, u16 motions, s16 group, omObjFunc update)
{
    assert(prio == 0x1000 && models == 0 && motions == 0 && group == -1 && update);
    created++;
    return &boo;
}

s16 BoardModelCreate(s32 file, s32 *motions, s32 link)
{
    /* The real Boo event object now exists. Stop at the first asset load. */
    assert(file == DATA_MAKE_NUM(DATADIR_BYOKODORI, 2));
    longjmp(atModelLoad, 1);
}

int main(void)
{
    currItemRestore = 2;
    boo.trans = (Vec){1, 2, 3};
    if (!setjmp(atModelLoad)) {
        ExecItemBooBall();
        return 1;
    }
    assert(created == 1);
    assert(boo.trans.x == 0 && boo.trans.y == 0 && boo.trans.z == 0);
    return 0;
}
