#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/learned_spells.h"

#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"

#include "random.h"
#include "FileMan.h"

#include <string.h>
#include <wchar.h>

/* Retail Local Code\Party Import.cpp: converts imported Wizardry 7
   characters into the Wizardry 8 layout. */

#define PARTY_IMPORT_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Party Import.cpp"

/* The three bonus attributes each profession grants on top of the imported
   values. */
// GLOBAL: WIZ8 0x00614FC8
int g_profession_primary_attributes[15][3] = {
    {0, 4, 3}, {0, 4, 2}, {3, 0, 2}, {4, 1, 6}, {4, 5, 1}, {4, 5, 1}, {5, 4, 6}, {4, 5, 6},
    {4, 6, 1}, {4, 1, 6}, {2, 1, 3}, {1, 4, 2}, {1, 4, 2}, {1, 6, 2}, {1, 4, 2},
};

/* The starting spells LearnSpell grants each profession on import. */
// GLOBAL: WIZ8 0x0062A5F8
int g_profession_starting_spells[15][6] = {
    {0, 0, 0, 0, 0, 0},   {6, 2, 0, 0, 0, 0},     {6, 13, 0, 0, 0, 0},  {6, 1, 0, 0, 0, 0},
    {5, 12, 0, 0, 0, 0},  {1, 7, 0, 0, 0, 0},     {6, 10, 0, 0, 0, 0},  {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},   {0, 0, 0, 0, 0, 0},     {6, 2, 13, 11, 0, 0}, {6, 1, 7, 12, 0, 0},
    {6, 12, 5, 11, 0, 0}, {10, 12, 11, 14, 0, 0}, {5, 12, 14, 4, 0, 0},
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

/* The Wizardry 7 import file state filled by the loader below: the record
   count, whether the save carries an ending selector, that selector's two
   nibbles decoded, the ninety-six flag bits and the character records. */
// GLOBAL: WIZ8 0x0068DE48
int g_import_character_count;

// GLOBAL: WIZ8 0x0068DE4C
unsigned char g_import_ending_record;

// GLOBAL: WIZ8 0x0068DE50
int g_wiz7_ending;

// GLOBAL: WIZ8 0x0068DE54
int g_import_difficulty;

// GLOBAL: WIZ8 0x0068DE58
unsigned char g_import_flags[0x60];

// GLOBAL: WIZ8 0x0068DEB8
W8Wiz7Character g_imported_characters[6];

/* Load a Wizardry 7 save for import: a 0x34c-byte header (two record-skip
   counts at 0x2cc/0x2ce, the flag bitmask at 0x200), thirty-two skipped
   0x100-byte blocks, the 0x4c-byte party block whose last short is the
   character count and whose first marks the ending selector, five skipped
   sections, then the character records. Every record must carry the previous
   one's tag byte. On success the ending and difficulty nibbles decode from
   the shared tag and the flag bits unpack into g_import_flags. */
// FUNCTION: WIZ8 0x00558D00
unsigned char LoadWizardry7ImportFile(char* path)
{
    unsigned int bytes_read;
    unsigned char header[0x34c];
    short party_block[0x26];
    unsigned char skip_80[0x80];
    unsigned char skip_90[0x90];
    unsigned char skip_68[0x68];
    unsigned char skip_14a[0x14a];
    unsigned char skip_344[0x344];
    unsigned char skip_42[0x42];
    HWFILE file;
    int index;

    short record_skip;
    short bank_skip;

    file = FileOpen(path, FILE_ACCESS_READ, 0);
    if (file == 0) {
        return 0;
    }
    if (FileRead(file, header, 0x34c, &bytes_read) != 0) {
        // reinterpret-ok: raw serialized file image; unaligned header short
        record_skip = *reinterpret_cast<short*>(&header[0x2cc]);
        // reinterpret-ok: raw serialized file image; unaligned header short
        bank_skip = *reinterpret_cast<short*>(&header[0x2ce]);
        if (FileSeek(file, record_skip * 6, FILE_SEEK_FROM_CURRENT) != 0 &&
            FileSeek(file, bank_skip * 8, FILE_SEEK_FROM_CURRENT) != 0) {
            for (index = 0; index < 0x20; ++index) {
                if (FileSeek(file, 0x100, FILE_SEEK_FROM_CURRENT) == 0) {
                    goto fail;
                }
            }
            if (FileRead(file, party_block, 0x4c, &bytes_read) != 0 && party_block[0x25] != 0 &&
                party_block[0x25] < 7 && FileRead(file, skip_80, 0x80, &bytes_read) != 0 &&
                FileRead(file, skip_90, 0x90, &bytes_read) != 0 &&
                FileRead(file, skip_68, 0x68, &bytes_read) != 0 &&
                FileRead(file, skip_14a, 0x14a, &bytes_read) != 0 &&
                FileRead(file, skip_344, 0x344, &bytes_read) != 0 &&
                FileRead(file, skip_42, 0x42, &bytes_read) != 0 &&
                FileSeek(file, 100, FILE_SEEK_FROM_CURRENT) != 0) {
                for (index = 0; index < party_block[0x25]; ++index) {
                    if (FileRead(file, &g_imported_characters[index], 0x248, &bytes_read) == 0) {
                        goto fail;
                    }
                    if (index != 0 && g_imported_characters[index].party_tag_232 !=
                                          g_imported_characters[index - 1].party_tag_232) {
                        goto fail;
                    }
                }
                FileClose(file);
                g_import_character_count = party_block[0x25];
                g_import_ending_record = party_block[0] == -1;
                if (g_import_ending_record != 0) {
                    switch (g_imported_characters[0].party_tag_232 & 0xf0) {
                    case 0x10:
                        g_wiz7_ending = 0;
                        break;
                    case 0x20:
                        g_wiz7_ending = 1;
                        break;
                    case 0x40:
                        g_wiz7_ending = 2;
                        break;
                    case 0x80:
                        g_wiz7_ending = 3;
                        break;
                    default:
                        return 0;
                    }
                } else {
                    g_wiz7_ending = -1;
                }
                switch (g_imported_characters[0].party_tag_232 & 0xf) {
                case 1:
                    g_import_difficulty = 0;
                    break;
                case 2:
                    g_import_difficulty = 1;
                    break;
                case 4:
                    g_import_difficulty = 2;
                    break;
                default:
                    g_import_difficulty = -1;
                }
                for (index = 0; index < 0x60; ++index) {
                    g_import_flags[index] = (header[0x200 + (index >> 3)] >> (index & 7)) & 1;
                }
                return 1;
            }
        }
    }
fail:
    FileClose(file);
    return 0;
}

/* Apply the loaded Wizardry 7 import: reset the run, seed the party gold,
   mark the imported-party path and convert each record into a regular member.
   Reports 1 when the file does not load or a slot cannot take the character,
   2 when the ending selector holds the value three, 0 otherwise. */
// FUNCTION: WIZ8 0x00558C40
unsigned char ImportWizardry7Party(char* path)
{
    W8Character scratch;
    int index;

    if (LoadWizardry7ImportFile(path) == 0) {
        return 1;
    }
    ResetForNewGame();
    g_status.party_gold = 2500;
    g_status.skip_loose_character_check_2444 = 1;
    if (g_import_character_count < 7) {
        for (index = 0; index < g_import_character_count; ++index) {
            ImportWizardry7Character(&scratch, &g_imported_characters[index]);
            if (AddCharacterToParty(&scratch, -1) == -1) {
                return 1;
            }
        }
        return (g_wiz7_ending != 3) - 1 & 2;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005590B0
void ImportWizardry7Character(W8Character* character, W8Wiz7Character* imported)
{
    W8Profession profession;
    unsigned int level;
    unsigned int skill_id;
    int status;

    memset(character, 0, sizeof(W8Character));
    swprintf(character->name, g_combat_log_format, TitleCaseString(imported->name_000));
    wcscpy(character->name_part_2, character->name);
    character->iRace = imported->race_237;
    character->gender = (W8Gender)imported->gender_238;
    switch (imported->profession_239) {
    default:
        profession = W8_PROFESSION_FIGHTER;
        break;
    case 1:
        profession = W8_PROFESSION_MAGE;
        break;
    case 2:
        profession = W8_PROFESSION_PRIEST;
        break;
    case 3:
        profession = W8_PROFESSION_ROGUE;
        break;
    case 4:
        profession = W8_PROFESSION_RANGER;
        break;
    case 5:
        profession = W8_PROFESSION_ALCHEMIST;
        break;
    case 6:
        profession = W8_PROFESSION_BARD;
        break;
    case 7:
        profession = W8_PROFESSION_PSIONIC;
        break;
    case 8:
        profession = W8_PROFESSION_VALKYRIE;
        break;
    case 9:
        profession = W8_PROFESSION_BISHOP;
        break;
    case 10:
        profession = W8_PROFESSION_LORD;
        break;
    case 11:
        profession = W8_PROFESSION_SAMURAI;
        break;
    case 12:
        profession = W8_PROFESSION_MONK;
        break;
    case 13:
        profession = W8_PROFESSION_NINJA;
        break;
    }
    character->iProfession = profession;
    CalcCharacterTableValue(character);
    level = (unsigned short)imported->level_024;
    if (imported->level_024 > 0) {
        level = 1;
    }
    AdvanceCharacterToLevel(character, level);
    character->experience = 13000;
    character->kill_count_09f9 = imported->kill_count_010;
    character->death_count_09fd = imported->deaths_026 - 1;
    character->profession_levels[character->iProfession] = character->uiExpLevel;
    character->original_profession = character->iProfession;
    character->level_band_base = 0;
    status = imported->status_23b;
    if (status == 2 || status == 3) {
        character->uiCondition[0x12] = 9999;
        character->highest_condition = 0x12;
    } else {
        character->highest_condition = 0;
    }
    character->enchantment_top = 0;
    ConvertAttribute(character, imported);
    GrantStartingSpells(character, imported);
    for (skill_id = 0; skill_id < 0x29; ++skill_id) {
        character->skills[skill_id].active_00 = 0;
        character->skills[skill_id].points_02 = ConvertSkill(skill_id, character, imported);
    }
    RefreshCharacterSkillAvailability(character);
    ImportEquipment(character, imported);
    DeriveCharacterPersonality(character);
    EnsureUniquePartyVoice(character);
    CalcCharacterLevelBand(character);
    RecalculateCharacterDerivedStats(character);
    character->stamina = character->uiStaminaMax;
    character->hp_current = character->uiHPMax;
    for (skill_id = 0; skill_id < 6; ++skill_id) {
        character->iSPLeft[skill_id] = character->sp_max[skill_id];
    }
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
    primary = g_profession_primary_attributes[character->iProfession];
    for (i = 3; i != 0; --i) {
        imported_values[*primary++] += 0x28;
    }
    for (i = 0; i < 7; ++i) {
        character->attributes[i].value = g_race_attribute_minimums[character->iRace].values[i];
    }
    points = 0;
    for (i = 0; i < 7; ++i) {
        if (character->attributes[i].value <
            static_cast<unsigned int>(
                g_profession_attribute_minimums[character->iProfession].values[i])) {
            int deficit = g_profession_attribute_minimums[character->iProfession].values[i] -
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
            if (g_profession_attribute_minimums[character->iProfession].values[pick] != 0 &&
                static_cast<unsigned int>(
                    g_race_attribute_minimums[character->iRace].values[pick]) <
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
void GrantStartingSpells(W8Character* character, const W8Wiz7Character*)
{
    W8LearnedSpellState scratch;
    char count;
    int offset;
    int i;

    for (i = 0x72; i != 0; --i) {
        character->spell_learned[i - 1] = 0;
    }
    offset = g_profession_magic_level_offsets[character->iProfession];
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
        LearnSpell(character, g_profession_starting_spells[character->iProfession][i], '\0');
        --count;
        if (count == '\0') {
            break;
        }
        ++i;
    } while (i < 6);
    BuildLearnedSpellState(&scratch, character);
}

// FUNCTION: WIZ8 0x00559650
void ImportEquipment(W8Character* character, const W8Wiz7Character* imported)
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
    if (character->iRace != 5) {
        profession = character->iProfession;
    }
    starting = g_starting_equipment[profession];
    for (slot = 6; slot != 0; --slot) {
        item_id = *starting++;
        if (item_id != -1) {
            ReplaceOrCreateItem(&item, item_id, '\x01', '\x01', '\x01');
            equip_slot = GetItemDefaultEquipSlot(item_id);
            if (equip_slot == -1) {
                if (FindCharacterItemByDatabaseKind005213C0(
                        character, g_item_records[item_id].unidentified_name_index, 0, 2) == '\0') {
                    AddItemToCharacter(character, &item, '\x01', '\0', '\0');
                }
            } else {
                if (equip_slot == 6 && (g_item_records[item_id].flags_041 & 8) != 0) {
                    equip_slot = 7;
                }
                if (character->EquippedItem[equip_slot].iItemNo == -1) {
                    AddItemToCharacter(character, &item, '\x01', '\0', '\0');
                }
            }
        }
    }
    give = -1;
    if (character->EquippedItem[6].iItemNo == -1) {
        if (character->iProfession == W8_PROFESSION_FIGHTER) {
            if (character->skills[0].level < character->skills[1].level) {
                give = 0x12;
            } else {
                give = 7;
            }
        } else if (character->iProfession == W8_PROFESSION_PRIEST) {
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

/* Retail fell through the failed assert and read `imported->skills` by
   whatever `mapped` held; a deterministic zero models that defect path. */
// FUNCTION: WIZ8 0x00559BC0
unsigned int ConvertSkill(unsigned int skill_id, W8Character* character,
                          const W8Wiz7Character* imported)
{
    int mapped = 0;
    unsigned int unlocks;
    unsigned int roll;
    unsigned int base_value;
    char routed = 0;
    int i;

    if (g_skill_attributes[skill_id].attribute_1_04 == 2) {
        if (g_profession_skill_availability[skill_id][character->iProfession] != 1) {
            return 0;
        }
    } else if (g_skill_attributes[skill_id].attribute_1_04 == 3) {
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
            if (g_profession_skill_availability[0x12][character->iProfession] != 1) {
                base_value = 0;
                break;
            }
            base_value = imported->skills[0];
            for (i = 1; i < 5; ++i) {
                if (base_value <= imported->skills[i]) {
                    base_value = imported->skills[i];
                }
            }
            if (100 < base_value) {
                base_value = 100;
            }
            if (g_profession_bonus_skills[character->iProfession] != 0x12) {
                for (i = 0; i < 4 && g_profession_skills[character->iProfession][i] != 0x12; ++i) {
                }
                if (i == 4 || 3 < i) {
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
            if (character->iProfession == 0xc && (skill_id == 0x1a || skill_id == 0x1b) &&
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
