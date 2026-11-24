#ifndef PARTYBOARD_NETPLAY_SPECIAL_GAME_H
#define PARTYBOARD_NETPLAY_SPECIAL_GAME_H

/* Slots 0 and 1 are boards. Slot 10 is Ztar's separate volleyball entrance. */
static inline int PartyBoard_ExtraRoomSlot(int module)
{
    static const int games[] = { 0, 0, 463, 453, 451, 442, 459, 460, 461, 462, 433 };
    for (int i = 2; i < (int)(sizeof(games) / sizeof(games[0])); ++i) {
        if (games[i] == module) return i;
    }
    return -1;
}

#endif
