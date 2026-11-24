#include <assert.h>
#include "src/REL/instDll/font.c"

int main(void)
{
    const int fallback = DATA_MAKE_NUM(DATADIR_INST, 0x25);
    for (unsigned ch = 0; ch < 256; ++ch) {
        for (unsigned mark = 0; mark < 3; ++mark) {
            u8 suffix = mark ? 0x7f + mark : 0;
            assert(FontGlyphFile(FontCharFilePal, 0x130, ch, suffix, FALSE) == FontCharFilePal[ch]);
            s32 glyph = FontGlyphFile(FontCharFile, 0xc6, ch, suffix, TRUE);
            if (ch >= 0xc6 || (mark == 1 && ch == 0x96)) assert(glyph == fallback);
        }
    }
    return 0;
}
