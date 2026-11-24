#include <assert.h>
#include <stdint.h>
#include "game/memory.h"

/* Use the game allocator with an isolated outer heap. */
static uint64_t arena[16384];
static void *TestAlloc(HeapID heap, size_t size) { return HuMemMemoryAlloc(arena, size, 0); }
#define HuMemDirectMalloc TestAlloc
#include "src/game/process.c"

static int events[5], count;
static void Child(void) { events[count++] = 3; }
static void Cleanup(void) { events[count++] = 2; }
static void Parent(void)
{
    events[count++] = 1;
    Process *child = HuPrcChildCreate(Child, 1, 4096, 0, HuPrcCurrentGet());
    assert(child != NULL);
    HuPrcDestructorSet(Cleanup);
}
static void Other(void)
{
    events[count++] = 4;
    HuPrcVSleep();
    events[count++] = 5;
}

int main(void)
{
    HuMemHeapInit(arena, sizeof(arena));
    HuPrcInit();
    assert(HuPrcCreate(Parent, 3, 4096, 0));
    assert(HuPrcCreate(Other, 2, 4096, 0));
    HuPrcCall(1);
    assert(count == 3 && events[0] == 1 && events[1] == 2 && events[2] == 4);
    HuPrcCall(1);
    assert(count == 4 && events[3] == 5);
    assert(!processtop && !processcur && !processcnt);
    assert(HuMemUsedMemoryBlockGet(arena) == 0);
    return 0;
}
