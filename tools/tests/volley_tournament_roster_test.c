#include <assert.h>
#include "src/REL/ztardll/main.c"

int main(void)
{
    for (int first = 0; first < 8; ++first) {
        for (int second = 0; second < 8; ++second) {
            if (first == second) continue;
            GWPlayerCfg[0].group = GWPlayerCfg[2].group = 0;
            GWPlayerCfg[1].group = GWPlayerCfg[3].group = 1;
            GWPlayerCfg[0].character = first;
            GWPlayerCfg[2].character = second;
            fn_1_40E4();
            unsigned used = (1u << first) | (1u << second);
            for (int i = 0; i < 6; ++i) {
                assert(mgIndexList[i] < 8);
                unsigned bit = 1u << mgIndexList[i];
                assert(!(used & bit));
                used |= bit;
            }
            assert(used == 0xff);
            assert(GWPlayerCfg[1].character == mgIndexList[0]);
            assert(GWPlayerCfg[3].character == mgIndexList[1]);
            assert(mgIndexList[12] == 0xff && mgIndexList[13] == 0xff);
        }
    }
    return 0;
}
