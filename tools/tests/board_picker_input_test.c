#include <assert.h>
#include "game/audio.h"
static int IgnoreSound(int id) { return id; }
#define HuAudFXPlay IgnoreSound
#include "src/game/board/ui.c"

int main(void)
{
    omObjData cursor = {0};
    UnkUiWork01 picker = {0};
    pickerChoiceDefault = 3;
    picker.unk02 = 2;
    assert(CheckPickerButton(&picker, &cursor, PAD_BUTTON_A | PAD_BUTTON_RIGHT));
    assert(pickerChoice == 2 && picker.unk02 == 2);
    assert(CheckPickerButton(&picker, &cursor, PAD_BUTTON_B | PAD_BUTTON_LEFT));
    assert(pickerChoice == -1 && picker.unk00_bit0 && picker.unk00_bit7);
    _Static_assert(ARRAY_COUNT(pickerPos) == 2 * ARRAY_COUNT(GWPlayer[0].items), "team inventory cursor capacity");
    picker = (UnkUiWork01){0};
    pickerChoiceDefault = ARRAY_COUNT(pickerPos);
    picker.unk02 = pickerChoiceDefault - 2;
    assert(CheckPickerButton(&picker, &cursor, PAD_BUTTON_RIGHT));
    assert(picker.unk02 == pickerChoiceDefault - 1);
    assert(CheckPickerButton(&picker, &cursor, PAD_BUTTON_A));
    assert(pickerChoice == pickerChoiceDefault - 1);
    return 0;
}
