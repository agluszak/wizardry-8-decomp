#include "wiz8/character.h"

/* Address quarantine 004ef950-004efe70; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

/* 0x006164F4: personality and voice values by faction and profession class,
   two dwords per row. It ends exactly where the faction/race/profession table
   at 0x00616604 begins. */
// GLOBAL: WIZ8 0x006164F4
const int g_character_value_table_006164f4[34][2] = {
    {0, 2}, {0, 1}, {6, 1}, {0, 1}, {0, 1}, {2, 2}, {0, 1}, {8, 1},
    {7, 2}, {7, 2}, {7, 2}, {7, 2}, {7, 2}, {7, 2}, {4, 1}, {4, 1},
    {1, 1}, {5, 2}, {7, 1}, {6, 2}, {6, 2}, {6, 1}, {3, 2}, {3, 2},
    {5, 1}, {1, 1}, {1, 2}, {5, 2}, {1, 1}, {1, 2}, {2, 1}, {2, 1},
    {2, 2}, {2, 2}
};

/* Derive the character's personality and voice from faction and profession.
   Unaligned characters pick a class through the race shortcut first. */
// FUNCTION: WIZ8 0x004EFA30
void Function4EFA30(W8Character* character)
{
    int faction = character->faction;
    int value = character->current_profession;
    int index;
    int flag;

    if (faction == 0) {
        switch (value) {
        case 0:
        case 1:
        case 3:
        case 7:
            if (character->race == 2) {
                value = 0x10;
            }
            else if (character->race == 6) {
                value = 0x0f;
            }
            break;
        }
    }
    index = faction + value * 2;
    flag = g_character_value_table_006164f4[index][1];
    character->personality_0081 = g_character_value_table_006164f4[index][0];
    if (flag == 1) {
        character->voice_0085 = 0;
    }
    else {
        character->voice_0085 = 1;
    }
    *(int*)character->unknown_007d = 0;
}
