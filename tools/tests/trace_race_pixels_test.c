#include "game/hu3d.h"
#include "game/memory.h"
#include "game/sprite.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t allocated[3];
static int allocations;
static void *Allocate(size_t size)
{
    assert(allocations < 3);
    allocated[allocations++] = size;
    return malloc(size);
}

#define HuMemDirectMallocNum(heap, size, num) Allocate(size)
#include "gpu_stubs.h"
#include "src/REL/m404Dll/main.c"

static unsigned Value(int x, int y, int bits)
{
    return (x * 41 + y * 159 + 0xa35b) & ((1u << bits) - 1);
}

static void CheckImage(int format, int bits, int width, int height, int tileWidth, int tileHeight)
{
    u8 expected[512] = { 0 };
    size_t next = 0;
    allocations = 0;
    ANIMDATA *anim = fn_1_5D64(width, height, format);
    assert(allocations == 3 && allocated[0] == sizeof(*anim) && allocated[1] == sizeof(*anim->bmp));
    assert(anim->bmpNum == 1 && anim->bmp->pixSize == bits);

    /* Construct the GX stream sequentially, independently of the game's offset calculation. */
    for (int top = 0; top < height; top += tileHeight) {
        for (int left = 0; left < width; left += tileWidth) {
            for (int row = 0; row < tileHeight; ++row) {
                for (int col = 0; col < tileWidth; ++col) {
                    int x = left + col, y = top + row;
                    unsigned v = 0;
                    if (x < width && y < height) {
                        v = Value(x, y, bits);
                        fn_1_5E74(anim, x, y, v);
                    }
                    if (bits == 4) {
                        if (col & 1) {
                            expected[next++] |= v;
                        } else {
                            expected[next] = v << 4;
                        }
                    } else if (bits == 8) {
                        expected[next++] = v;
                    } else {
                        expected[next++] = v >> 8;
                        expected[next++] = v;
                    }
                }
            }
        }
    }
    assert(next == allocated[2] && next == anim->bmp->dataSize && next <= sizeof(expected));
    assert(memcmp(anim->bmp->data, expected, next) == 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            assert(fn_1_607C(anim, x, y) == Value(x, y, bits));
        }
    }
    if (bits == 8) {
        fn_1_5E74(anim, 1, 1, 3);
        assert(fn_1_607C(anim, 1, 1) == (Value(1, 1, bits) | 3));
    }
    assert(fn_1_607C(anim, width, height - 1) == 0);
    free(anim->bmp->data);
    free(anim->bmp);
    free(anim);
}

int main(void)
{
    for (int layout = 0; layout < 2; ++layout) {
        int width = layout ? 10 : 16, height = layout ? 9 : 16;
        CheckImage(0, 4, width, height, 8, 8);
        CheckImage(1, 8, width, height, 8, 4);
        CheckImage(3, 16, width, height, 4, 4);
    }
    assert(fn_1_5D64(8, 8, -1) == NULL);
    assert(fn_1_5D64(0, 8, 0) == NULL);
    puts("Trace Race: GX tile bytes and pixel lookup agree for complete and partial tiles");
    return 0;
}
