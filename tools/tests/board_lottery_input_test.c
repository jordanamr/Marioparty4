#include <assert.h>
#include "game/board/main.h"
#include "game/audio.h"
#include "game/animdata.h"
#include "game/sprite.h"
#include <string.h>
static int IgnoreSound(int id) { return id; }
#define HuAudFXPlay IgnoreSound
#define BoardIsKill() 0
static ANIMBMP mask;
static int uploads[8];

static void RecordUpload(GXTexObj *obj, const void *pixels)
{
    assert(pixels == mask.data);
    for (int i = 0; i < 8; i++) {
        if (&mask.texData[i].tex_obj == obj) {
            assert(mask.texData[i].tex_initialized);
            uploads[i]++;
            return;
        }
    }
    assert(0);
}

static Vec cursorOrigin;
static void RecordCursorPosition(s16 group, s16 member, float x, float y)
{
    cursorOrigin = (Vec){x, y, 0};
}

#define HuSprPosSet RecordCursorPosition
#define GXInitTexObjData RecordUpload
#include "src/game/board/lottery.c"

static void TestAnimatedReveal(void)
{
    enum { WIDTH = 32, HEIGHT = 24 };
    u16 pixels[WIDTH * HEIGHT];
    memset(pixels, 0xff, sizeof(pixels));
    mask.sizeX = WIDTH;
    mask.sizeY = HEIGHT;
    mask.data = pixels;
    mask.texData[0].tex_initialized = mask.texData[6].tex_initialized = 1;

    for (int row = 0; row < HEIGHT; row++) {
        RevealTicketRow(&mask, row);
        for (int i = 0; i < 8; i++) {
            assert(uploads[i] == (i == 0 || i == 6 ? row + 1 : 0));
        }
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                assert(pixels[TicketGetPixel(WIDTH, x, y)] == (y <= row ? 0 : 0xffff));
            }
        }
    }
    assert(ScratchTicketCheckDone(&mask));
}

int main(void)
{
    omObjData cursor = {0};
    LotteryTicketPickWork *ticket = OM_GET_WORK_PTR(&cursor, LotteryTicketPickWork);
    ticket->unk02 = 2;
    ticket->unk03 = 1;
    GWSystem.player_curr = 2;
    GWPlayer[2].port = 1;
    HuPadDStkRep[1] = PAD_BUTTON_DOWN;
    HuPadBtnDown[1] = PAD_BUTTON_A;
    ExecScratchPick(&cursor);
    assert(currPrize == 6 && ticket->unk00_field0);
    for (int row = 0; row < 3; row++) {
        for (int column = 0; column < 4; column++) {
            Vec center = ScratchTicketCenter(column, row);
            assert(center.x == 107 + 110 * column && center.y == 98 + 110 * row);
            ticket->unk00_field0 = 0;
            ticket->unk02 = column;
            ticket->unk03 = row;
            HuPadDStkRep[1] = HuPadBtnDown[1] = 0;
            ExecScratchPick(&cursor);
            assert(cursorOrigin.x + 16 == center.x && cursorOrigin.y - 8 == center.y);
            HuPadBtnDown[1] = PAD_BUTTON_A;
            ExecScratchPick(&cursor);
            assert(currPrize == column + 4 * row);
        }
    }
    TestAnimatedReveal();
    return 0;
}
