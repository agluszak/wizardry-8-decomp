#include "wiz8/local_code/PartyImport.h"

#include "wiz8/character.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"

#include "random.h"

#include <string.h>

/* Retail Local Code\Party Import.cpp: converts imported Wizardry 7
   characters into the Wizardry 8 layout. The entry driver 0x005590B0 that
   calls all four functions stays in the gap. */

#define PARTY_IMPORT_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Party Import.cpp"

/* The three bonus attributes each profession grants on top of the imported
   values. */
// GLOBAL: WIZ8 0x00614FC8
int g_profession_primary_attributes_614fc8[15][3] = {
    {0, 4, 3}, {0, 4, 2}, {3, 0, 2}, {4, 1, 6}, {4, 5, 1}, {4, 5, 1}, {5, 4, 6}, {4, 5, 6},
    {4, 6, 1}, {4, 1, 6}, {2, 1, 3}, {1, 4, 2}, {1, 4, 2}, {1, 6, 2}, {1, 4, 2},
};

/* The starting spells LearnSpell grants each profession on import. */
// GLOBAL: WIZ8 0x0062A5F8
int g_profession_starting_spells_62a5f8[15][6] = {
    {0, 0, 0, 0, 0, 0},     {6, 2, 0, 0, 0, 0},   {6, 13, 0, 0, 0, 0}, {6, 1, 0, 0, 0, 0},
    {5, 12, 0, 0, 0, 0},    {1, 7, 0, 0, 0, 0},   {6, 10, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},     {6, 2, 13, 11, 0, 0}, {6, 1, 7, 12, 0, 0}, {6, 12, 5, 11, 0, 0},
    {10, 12, 11, 14, 0, 0}, {5, 12, 14, 4, 0, 0}, {0, 0, 0, 0, 0, 0},
};

/* The item database index whose legacy item number matches the imported
   one, or -1 when no record carries it. */
static int FindItemByLegacyNumber(short item_number)
{
    unsigned int index = 0;
    while (index < gXStatus.uiItemsInDatabase) {
        if (g_item_records[index].legacy_item_number_03c == item_number) {
            return index;
        }
        ++index;
    }
    return -1;
}

// FUNCTION: WIZ8 0x005592D0
void ConvertAttribute(W8Character* character, const W8Wiz7Character* imported)
{
    unsigned int imported_values[7] = {0};
    int mapped;
    int i;
    int total = 0;
    int average;
    int points;
    int added;
    int spins;
    W8CharacterAttribute* attribute;
    const int* primary;
    unsigned int add;
    unsigned int cap;
    unsigned int pick;
    int removed;

    for (i = 0; i < 8; ++i) {
        if (i != 6 && i != 7) {
            switch (i) {
            case 0:
                mapped = 0;
                break;
            case 1:
                mapped = 1;
                break;
            case 2:
                mapped = 2;
                break;
            case 3:
                mapped = 3;
                break;
            case 4:
                mapped = 4;
                break;
            case 5:
                mapped = 5;
                break;
            case 6:
                break;
            default:
                srAssertFail("FALSE", PARTY_IMPORT_CPP, 0x462,
                             "ConvertAttribute: ERROR - Invalid attribute");
            }
            imported_values[mapped] = imported->attributes[i];
            switch (i) {
            case 0:
                mapped = 0;
                break;
            case 1:
                mapped = 1;
                break;
            case 2:
                mapped = 2;
                break;
            case 3:
                mapped = 3;
                break;
            case 4:
                mapped = 4;
                break;
            case 5:
                mapped = 5;
                break;
            case 6:
                break;
            default:
                srAssertFail("FALSE", PARTY_IMPORT_CPP, 0x462,
                             "ConvertAttribute: ERROR - Invalid attribute");
            }
            total = total + imported_values[mapped];
        }
    }
    average = total / 6;
    primary = g_profession_primary_attributes_614fc8[character->current_profession];
    for (i = 3; i != 0; --i) {
        imported_values[*primary++] += 0x28;
    }
    for (i = 0; i < 7; ++i) {
        character->attributes[i].value = g_race_attribute_minimums[character->race].values[i];
    }
    points = 0;
    for (i = 0; i < 7; ++i) {
        if (character->attributes[i].value <
            (unsigned int)g_profession_attribute_minimums[character->current_profession]
                .values[i]) {
            int deficit = g_profession_attribute_minimums[character->current_profession].values[i] -
                          character->attributes[i].value;
            character->attributes[i].value += deficit;
            points += deficit;
        }
    }
    points = 0x46 - points;
    if (-1 < points) {
        added = 0;
        for (i = 0; i < 7; ++i) {
            add = (points * imported_values[i]) / (total + average + 0x78);
            cap = 100 - character->attributes[i].value;
            if (cap <= add) {
                add = cap;
            }
            added += add;
            character->attributes[i].value += add;
        }
        points -= added;
        spins = points * 3;
        i = 0;
        if (0 < spins) {
            attribute = character->attributes;
            do {
                Random(7);
                if (attribute->value < 100) {
                    --points;
                    spins -= 3;
                    attribute->value += 1;
                    if (points == 0) {
                        return;
                    }
                }
                ++i;
                ++attribute;
            } while (i < spins);
        }
        return;
    }
    removed = 0;
    if (0 < -points) {
        do {
            pick = Random(7);
            if (g_profession_attribute_minimums[character->current_profession].values[pick] != 0 &&
                (unsigned int)g_race_attribute_minimums[character->race].values[pick] <
                    character->attributes[pick].value) {
                ++removed;
                character->attributes[pick].value -= 1;
            }
        } while (removed < -points);
    }
    character->attribute_point_deficit_0199 = points;
    character->level_band_base = 1;
}

// FUNCTION: WIZ8 0x005595D0
void GrantStartingSpells005595D0(W8Character* character)
{
    unsigned char scratch[0x3dc];
    char count;
    int offset;
    int i;

    for (i = 0x72; i != 0; --i) {
        character->spell_learned[i - 1] = 0;
    }
    offset = g_profession_magic_level_offsets[character->current_profession];
    if (offset < 0 && -0xff < offset) {
        count = 2;
    } else {
        if (offset != 0) {
            return;
        }
        count = 4;
    }
    i = 0;
    do {
        LearnSpell(character, g_profession_starting_spells_62a5f8[character->current_profession][i],
                   '\0');
        --count;
        if (count == '\0') {
            break;
        }
        ++i;
    } while (i < 6);
    Function4F9600(scratch, character);
}

// FUNCTION: WIZ8 0x00559650
void ImportEquipment00559650(W8Character* character, const W8Wiz7Character* imported)
{
    W8ItemInstance item;
    /* The worthiest imported items: band 0 keeps the two most valuable finds
       (value above 3000), band 1 the next three (above 1000). */
    W8Wiz7Item candidates[2][20];
    W8Wiz7Item empty_item;
    int maximum[2];
    int counts[2];
    const W8Wiz7Item* source;
    const W8Wiz7Item* entry;
    int slot;
    int index;
    int best_index;
    int best_value;
    int item_index;
    int price;
    int give;
    const int* starting;
    int item_id;
    int equip_slot;
    W8Profession profession;

    memset(&empty_item, 0, sizeof(empty_item));
    counts[0] = 0;
    counts[1] = 0;
    EmptyAllCarriedItems(character);
    for (slot = 0; slot < 2; ++slot) {
        source = imported->items[slot];
        for (index = 0; index < 10; ++index) {
            if (source->item_number != 0) {
                item_index = FindItemByLegacyNumber(source->item_number);
                if (item_index != -1) {
                    if (item_index != 0x128) {
                        if (g_item_records[item_index].unidentified_name_index == 0x84 ||
                            4999 < (int)g_item_records[item_index].value) {
                            source++;
                            continue;
                        }
                        if (2999 < (int)g_item_records[item_index].value) {
                            candidates[0][counts[0]] = *source;
                            ++counts[0];
                            source++;
                            continue;
                        }
                        if (999 < (int)g_item_records[item_index].value) {
                            candidates[1][counts[1]] = *source;
                            ++counts[1];
                            source++;
                            continue;
                        }
                    }
                    ReplaceOrCreateItem(&item, item_index, '\x01', '\x01', '\x01');
                    if (g_item_records[item_index].binds_on_equip == '\0') {
                        StoreItemWithCharacterOrParty(character, &item, '\0', 0,
                                                      (unsigned int)(slot == 0));
                    } else {
                        AddItemToCharacter(character, &item, '\0', '\0', '\0');
                    }
                }
            }
            source++;
        }
    }
    maximum[0] = 1;
    if (counts[0] < 2) {
        maximum[0] = counts[0];
    }
    maximum[1] = 2;
    if (counts[1] < 3) {
        maximum[1] = counts[1];
    }
    for (slot = 0; slot < 2; ++slot) {
        while (0 < maximum[slot]) {
            best_value = 0;
            best_index = -1;
            for (index = 0; index < counts[slot]; ++index) {
                entry = &candidates[slot][index];
                if (0 < entry->item_number) {
                    item_index = FindItemByLegacyNumber(entry->item_number);
                    price = g_item_records[item_index].value;
                    if (ItemHasHiddenProperties(item_index) == '\0') {
                        price = price / 2;
                    }
                    if (best_value < price) {
                        best_value = g_item_records[item_index].value;
                        best_index = index;
                    }
                }
            }
            if (best_index != -1) {
                item_index = FindItemByLegacyNumber(candidates[slot][best_index].item_number);
                ReplaceOrCreateItem(&item, item_index, '\x01', '\x01', '\x01');
                if (g_item_records[item_index].binds_on_equip == '\0') {
                    StoreItemWithCharacterOrParty(character, &item, '\0', 0, 1);
                } else {
                    AddItemToCharacter(character, &item, '\0', '\0', '\0');
                }
                --maximum[slot];
                candidates[slot][best_index] = empty_item;
            }
        }
    }
    profession = W8_PROFESSION_COUNT;
    if (character->race != 5) {
        profession = character->current_profession;
    }
    starting = g_starting_equipment_61635c[profession];
    for (slot = 6; slot != 0; --slot) {
        item_id = *starting++;
        if (item_id != -1) {
            ReplaceOrCreateItem(&item, item_id, '\x01', '\x01', '\x01');
            equip_slot = GetItemDefaultEquipSlot(item_id);
            if (equip_slot == -1) {
                if (Function5213C0(character, g_item_records[item_id].unidentified_name_index, 0,
                                   2) == '\0') {
                    AddItemToCharacter(character, &item, '\x01', '\0', '\0');
                }
            } else {
                if (equip_slot == 6 && (g_item_records[item_id].flags_041 & 8) != 0) {
                    equip_slot = 7;
                }
                if (character->equipment[equip_slot].item_id == -1) {
                    AddItemToCharacter(character, &item, '\x01', '\0', '\0');
                }
            }
        }
    }
    give = -1;
    if (character->equipment[6].item_id == -1) {
        if (character->current_profession == W8_PROFESSION_FIGHTER) {
            if (character->skills[0].level < character->skills[1].level) {
                give = 0x12;
            } else {
                give = 7;
            }
        } else if (character->current_profession == W8_PROFESSION_PRIEST) {
            if (character->skills[3].level <= character->skills[5].level) {
                give = 0x52;
            } else {
                give = 0x16;
            }
        }
        if (give != -1) {
            ReplaceOrCreateItem(&item, give, '\x01', '\x01', '\x01');
            AddItemToCharacter(character, &item, '\x01', '\0', '\0');
        }
    }
    RebuildEquipmentAndDerivedStats(character);
}

/* The `mapped` index is deliberately left without a value on the asserted
   default path: retail falls through the failed assert and reads the
   imported skill by whatever index it held. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
// FUNCTION: WIZ8 0x00559BC0
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character,
                          const W8Wiz7Character* same_record, const W8Wiz7Character* imported,
                          unsigned int base_value)
{
    int mapped;
    unsigned int unlocks;
    unsigned int roll;
    char routed = 0;
    int i;

    if (g_skill_attributes[skill_id].unknown_04 == 2) {
        if (g_profession_skill_availability[skill_id][character->current_profession] != 1) {
            return 0;
        }
    } else if (g_skill_attributes[skill_id].unknown_04 == 3) {
        return 0;
    }
    switch (skill_id) {
    case 0:
        mapped = 1;
        break;
    case 1:
        mapped = 2;
        break;
    case 2:
    case 5:
        mapped = 4;
        break;
    case 3:
        mapped = 3;
        break;
    case 4:
        mapped = 0;
        break;
    case 6:
        mapped = 8;
        break;
    case 7:
        mapped = 0x12;
        break;
    case 8:
        mapped = 7;
        break;
    case 10:
        mapped = 0x10;
        break;
    case 0xb:
        mapped = 0x11;
        break;
    case 0xc:
        mapped = 0xd;
        break;
    case 0xd:
        mapped = 0xf;
        break;
    case 0xe:
        mapped = 9;
        break;
    case 0xf:
        mapped = 0xc;
        break;
    case 0x13:
        mapped = 0x21;
        break;
    case 0x15:
        mapped = 0x19;
        break;
    case 0x16:
        mapped = 0x1c;
        break;
    case 0x18:
        mapped = 0x20;
        break;
    case 0x19:
        mapped = 0x1e;
        break;
    case 0x1a:
        mapped = 0x1d;
        break;
    case 0x1b:
        mapped = 0x1f;
        break;
    case 9:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x14:
    case 0x17:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x20:
    case 0x21:
        routed = 1;
        switch (skill_id) {
        case 9:
            base_value = imported->skills[5];
            if (base_value <= imported->skills[6]) {
                base_value = imported->skills[6];
            }
            break;
        case 0x10:
            base_value = imported->skills[0];
            for (i = 1; i < 5; ++i) {
                if (base_value <= imported->skills[i]) {
                    base_value = imported->skills[i];
                }
            }
            for (i = 8; i < 10; ++i) {
                if (base_value <= imported->skills[i]) {
                    base_value = imported->skills[i];
                }
            }
            if (100 < base_value) {
                base_value = 100;
            }
            break;
        case 0x11:
            base_value = imported->skills[7];
            if (base_value <= imported->skills[6]) {
                base_value = imported->skills[6];
            }
            if (base_value <= imported->skills[5]) {
                base_value = imported->skills[5];
            }
            if (100 < base_value) {
                base_value = 100;
            }
            break;
        case 0x12:
            base_value = 0;
            if (g_profession_skill_availability[0x12][character->current_profession] == 1 &&
                g_profession_bonus_skills[character->current_profession] != 0x12) {
                int profession_slot = -1;
                base_value = imported->skills[0];
                for (i = 1; i < 5; ++i) {
                    if (base_value <= imported->skills[i]) {
                        base_value = imported->skills[i];
                    }
                }
                if (100 < base_value) {
                    base_value = 100;
                }
                /* Retail falls into the zero-value case unless the profession
                   carries the skill in one of its four table slots; a late
                   slot halves the imported value on the way out. */
                for (i = 0; i < 4; ++i) {
                    if (g_profession_skills[character->current_profession][i] == 0x12) {
                        profession_slot = i;
                        break;
                    }
                }
                if (profession_slot == -1) {
                    base_value = 0;
                } else if (3 < profession_slot) {
                    base_value >>= 1;
                }
            }
            break;
        case 0x14:
            base_value =
                ((unsigned int)(imported->skills[0x1b] + imported->skills[0x18] * 4) * 0x14) / 100;
            break;
        case 0x17:
        case 0x23:
        case 0x25:
            base_value = 0;
            break;
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x1f:
        case 0x20:
        case 0x21:
            unlocks = character->skill_unlocks[skill_id];
            if (unlocks == 0) {
                base_value = 0;
                break;
            }
            if (imported->profession_239 == '\x05' || imported->profession_239 == '\r' ||
                imported->profession_239 == '\x04') {
                base_value = imported->skills[0x1d];
            } else {
                base_value = imported->skills[0xe];
            }
            if (unlocks < 3) {
                base_value = ((unlocks + 1) * base_value) / 3;
            }
            if (100 < base_value) {
                base_value = 100;
            }
            break;
        default:
            srAssertFail("FALSE", PARTY_IMPORT_CPP, 0x56c,
                         "ConvertSkill: ERROR - Invalid NEW skill");
            break;
        }
        break;
    default:
        srAssertFail("FALSE", PARTY_IMPORT_CPP, 0x4b3, "ConvertSkill: ERROR - Invalid skill");
        break;
    }
    if (!routed) {
        base_value = imported->skills[mapped];
        if (0x17 < skill_id && skill_id < 0x1c) {
            if (character->current_profession == 0xc && (skill_id == 0x1a || skill_id == 0x1b) &&
                base_value == 0) {
                base_value = ((unsigned int)(imported->skills[0x20] + imported->skills[0x1e])) / 2;
            }
            unlocks = 0;
            for (i = 0x1c; i < 0x22; ++i) {
                unlocks += character->skill_unlocks[i];
            }
            if (unlocks == 0) {
                roll = Random(5);
                base_value = base_value / (roll + 5);
            } else if (unlocks < 5) {
                base_value = (unlocks * base_value) / 5;
            }
        }
    }
    unlocks = (base_value * 2) / 10;
    if (0x14 < unlocks) {
        unlocks = 0x14;
    }
    return unlocks;
}
#pragma clang diagnostic pop
