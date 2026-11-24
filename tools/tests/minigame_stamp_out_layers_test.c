#include <assert.h>
#include "src/REL/m415Dll/map.c"

unkStruct5 lbl_1_bss_36C;

int main(void)
{
    unkSubStruct2 groups[2] = {0};
    unkSubStruct elements[2] = {0};
    unkSubStruct4 layers[2][3] = {0};
    groups[1].unk3C = elements;
    groups[1].unk2 = 2;
    lbl_1_bss_36C.unk40 = groups;
    for (int i = 0; i < 2; i++) {
        elements[i].unk33 = i + 1;
        elements[i].unk54 = layers[i];
    }
    fn_1_C488(1, 19);
    assert(layers[0][0].unk24 == 19 && layers[0][1].unk24 == 0);
    assert(layers[1][0].unk24 == 19 && layers[1][1].unk24 == 19 && layers[1][2].unk24 == 0);
    return 0;
}
