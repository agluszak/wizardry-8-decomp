#include "wiz8/virtual_file.h"
#include "wiz8/local_code/CombatSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "FileMan.h"
#include "random.h"
#include "soundman.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMBAT_SOUND_CPP "C:\\Projects\\Wizardry 8\\Local Code\\Combat Sound.cpp"

// GLOBAL: WIZ8 0x0068DD90
char* g_weapon_attack_sounds_68dd90[38];
// GLOBAL: WIZ8 0x0068D850
char* g_material_impact_sounds_68d850[28][12];

/* Play one combat sound under Data\Sound\Combat\.  When the name carries more
   than one recorded variant a random 1..n digit is appended onto the caller's
   buffer before the .wav path is built; callers only reach that branch with
   writable names.  A positive volume is scaled by the configured effects
   level; zero and below play at the default.  With the flag set the handle is
   registered on the combat state so the service can poll SoundIsPlaying. */
// FUNCTION: WIZ8 0x005499D0
void PlayCombatSound005499D0(char* sound_name, unsigned int variant_count, bool store_handle,
                             int volume)
{
    SOUNDPARMS parms;
    char zSoundFileName[0x60];
    unsigned int handle;

    if (variant_count > 1) {
        strcat(sound_name, FormatString("%d", Random(variant_count) + 1));
    }
    sprintf(zSoundFileName, "Data\\Sound\\Combat\\%s.wav", sound_name);
    if (!FileExists(zSoundFileName)) {
        srAssertFail("FileExists(zSoundFileName)", COMBAT_SOUND_CPP, 104,
                     FormatString("CombatSound: ERROR - Sound file %s not found", zSoundFileName));
    }
    if (volume > 0) {
        memset(&parms, 0xff, sizeof(parms));
        parms.uiVolume =
            static_cast<unsigned int>(g_settings_6850c8.sound_effects_volume * volume) / 127;
        handle = SoundPlay(zSoundFileName, &parms);
    } else {
        handle = SoundPlay(zSoundFileName, 0);
    }
    if (handle != 0xffffffff && g_combat_state != 0 && store_handle) {
        g_combat_state->hit_sound_7bc = handle;
        g_combat_state->hit_sound_active_7c0 = 1;
    }
}

static unsigned char ReadHitSoundLine(int handle, char* line, unsigned int capacity)
{
    unsigned int length = 0;
    unsigned int done;
    char value;

    while (length + 1 < capacity) {
        done = 0;
        if (!FileRead(handle, &value, 1, &done) || done == 0) {
            line[length] = '\0';
            return length != 0;
        }
        if (value == '\n') {
            break;
        }
        if (value != '\r') {
            line[length++] = value;
        }
    }
    line[length] = '\0';
    return 1;
}

static void TrimHitSoundLine(char* line)
{
    char* comment = strchr(line, '*');
    size_t length;

    if (comment) {
        *comment = '\0';
    }
    length = strlen(line);
    while (length && isspace((unsigned char)line[length - 1])) {
        line[--length] = '\0';
    }
}

static char* DuplicateHitSound(const char* source)
{
    char* copy = static_cast<char*>(malloc(strlen(source) + 1));
    if (copy) {
        strcpy(copy, source);
    }
    return copy;
}

/* Local Code\\Combat Sound.cpp reads the attack list followed by twelve
   material columns of up to twenty-eight impact sounds.  A hash line advances
   the material column; an asterisk starts an inline comment. */
// FUNCTION: WIZ8 0x00549b00
unsigned char LoadHitSoundDatabase(void)
{
    char path[] = "Data\\Databases\\HitSounds.txt";
    char line[256];
    int handle;
    int row = 0;
    int column = -1;

    memset(g_weapon_attack_sounds_68dd90, 0, sizeof(g_weapon_attack_sounds_68dd90));
    memset(g_material_impact_sounds_68d850, 0, sizeof(g_material_impact_sounds_68d850));
    handle = FileOpen(path, 0x41, 0);
    if (!handle) {
        return 0;
    }
    while (row < 38 && ReadHitSoundLine(handle, line, sizeof(line))) {
        char* marker;
        TrimHitSoundLine(line);
        marker = strchr(line, '#');
        if (marker) {
            *marker = '\0';
            TrimHitSoundLine(line);
        }
        if (line[0]) {
            g_weapon_attack_sounds_68dd90[row++] = DuplicateHitSound(line);
        }
    }
    if (row != 38) {
        FileClose(handle);
        return 0;
    }
    row = 0;
    while (ReadHitSoundLine(handle, line, sizeof(line))) {
        TrimHitSoundLine(line);
        if (strchr(line, '#')) {
            ++column;
            row = 0;
            continue;
        }
        if (!line[0]) {
            continue;
        }
        if (column < 0 || column >= 12 || row >= 28) {
            FileClose(handle);
            return 0;
        }
        g_material_impact_sounds_68d850[row++][column] = DuplicateHitSound(line);
    }
    FileClose(handle);
    // Several impact materials intentionally provide one catch-all sound rather than one
    // entry per weapon class.  The retail loader accepts EOF after any valid final entry.
    return 1;
}

/* Free the two string tables populated by LoadHitSoundDatabase. */
// FUNCTION: WIZ8 0x00549e50
void ReleaseHitSoundDatabase(void)
{
    int row;
    int column;

    for (row = 0; row < 38; ++row) {
        if (g_weapon_attack_sounds_68dd90[row]) {
            free(g_weapon_attack_sounds_68dd90[row]);
            g_weapon_attack_sounds_68dd90[row] = 0;
        }
    }
    for (column = 0; column < 12; ++column) {
        for (row = 0; row < 28; ++row) {
            if (g_material_impact_sounds_68d850[row][column]) {
                free(g_material_impact_sounds_68d850[row][column]);
                g_material_impact_sounds_68d850[row][column] = 0;
            }
        }
    }
}

/* Look up a weapon-class/target-material impact sound.  Missing
   weapon-specific entries inherit weapon class zero; out-of-range indices use
   the retail "HIT" fallback.  The buffer comes back writable because
   PlayCombatSound may append a variant digit. */
/* The impact lookup shared by the emitted body below and by the sibling
   callers, where retail folds it inline. */
static inline char* LookupMaterialImpactSound(int weapon_class, int target_material)
{
    char* sound;

    if (weapon_class < 0 || weapon_class >= 28 || target_material < 0 || target_material >= 12) {
        return const_cast<char*>("HIT");
    }
    sound = g_material_impact_sounds_68d850[weapon_class][target_material];
    if (sound == 0) {
        sound = g_material_impact_sounds_68d850[0][target_material];
    }
    return sound;
}

// FUNCTION: WIZ8 0x00549EB0
char* GetMaterialImpactSound(int weapon_class, int target_material)
{
    return LookupMaterialImpactSound(weapon_class, target_material);
}

/* The two missile/monster siblings spell the same lookup as three leaves
   that each call PlayCombatSound rather than sharing one tail call. */
static inline void PlayMaterialImpactSound(int weapon_class, int target_material, int volume)
{
    char* sound;

    if (weapon_class < 0 || weapon_class >= 28 || target_material < 0 || target_material >= 12) {
        PlayCombatSound005499D0(const_cast<char*>("HIT"), 1, 1, volume);
        return;
    }
    sound = g_material_impact_sounds_68d850[weapon_class][target_material];
    if (sound) {
        PlayCombatSound005499D0(sound, 1, 1, volume);
    } else {
        PlayCombatSound005499D0(g_material_impact_sounds_68d850[0][target_material], 1, 1, volume);
    }
}

/* The equipment slot covering one armour-class hit location, then the item
   worn there (-1 when that location is bare).  The inlined copies share the
   line-168 assertion. */
static inline int PCItemInACSlot(const W8Character* character, int hit_location)
{
    int slot = 0;

    switch (hit_location) {
    case 0:
        slot = W8_EQUIP_SLOT_HEAD;
        break;
    case 1:
        slot = W8_EQUIP_SLOT_TORSO;
        break;
    case 2:
        slot = W8_EQUIP_SLOT_LEGS;
        break;
    case 3:
        slot = W8_EQUIP_SLOT_FEET;
        break;
    case 4:
        slot = W8_EQUIP_SLOT_HANDS;
        break;
    default:
        srAssertFail("FALSE", COMBAT_SOUND_CPP, 168, "PCItemInACSlot: ERROR - Invalid AC location");
    }
    return character->EquippedItem[slot].iItemNo;
}

// FUNCTION: WIZ8 0x00549EF0
void MakePCAttackSound00549EF0(W8CombatCharacterRow* row, const W8HandAttack* hand_attack,
                               int arg_3, bool store_handle, int volume)
{
    int weapon_class;

    if (hand_attack->uiHolds == HOLDS_NOTHING) {
        weapon_class = 9;
    } else {
        weapon_class = g_item_records[row->weapon_item_id_78].weapon_sound_class_0c5;
        if (weapon_class < 0 || weapon_class >= 38) {
            return;
        }
    }
    PlayCombatSound005499D0(g_weapon_attack_sounds_68dd90[weapon_class], 1, store_handle, volume);
}

// FUNCTION: WIZ8 0x00549F50
void MakePCMeleeHitSound(int iChar, const W8HandAttack* hand_attack, W8CombatSlot* target,
                         int hit_location, int volume)
{
    int weapon_class;
    int target_material = -1;

    if (hand_attack->uiHolds == HOLDS_NOTHING) {
        weapon_class = 9;
    } else {
        weapon_class = g_item_records[g_combat_state->characters[iChar].paired_item_id_7c]
                           .weapon_sound_class_0c5;
    }
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        const W8Character* character = &g_status_685170.buffers.Char[target->iChar];
        int item = PCItemInACSlot(character, hit_location);
        target_material = item == -1 ? 0 : g_item_records[item].material_0c1;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        const W8MonsterRecord* record = GetMonsterDataByLocationID(target->iMonsterID);
        target_material = record == 0 ? 0 : record->material_263;
    } else {
        srAssertFail("FALSE", COMBAT_SOUND_CPP, 415, "MakePCHitSound : Unknown target type");
    }
    PlayCombatSound005499D0(LookupMaterialImpactSound(weapon_class, target_material), 1, 1, volume);
}

// FUNCTION: WIZ8 0x0054A0E0
void MakePCHitSound(W8Missile* missile, W8CombatSlot* target, int hit_location, int volume)
{
    int weapon_class =
        g_missile_table_65bde0[missile->missile_table_index_1d8].weapon_sound_class_165;
    /* Defined so the assert-failure path still reaches the material lookup;
       -1 is out of range and yields the retail "HIT" fallback. */
    int target_material = -1;

    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        const W8Character* character = &g_status_685170.buffers.Char[target->iChar];
        int item = PCItemInACSlot(character, hit_location);
        target_material = item == -1 ? 0 : g_item_records[item].material_0c1;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        const W8MonsterRecord* record = GetMonsterDataByLocationID(target->iMonsterID);
        target_material = record == 0 ? 0 : record->material_263;
    } else {
        srAssertFail("FALSE", COMBAT_SOUND_CPP, 465, "MakePCHitSound : Unknown target type");
    }
    PlayMaterialImpactSound(weapon_class, target_material, volume);
}

// FUNCTION: WIZ8 0x0054A270
void MakeMonsterHitSound(const W8MonsterAttack* attack, W8CombatSlot* target, int hit_location,
                         int volume)
{
    int weapon_class;
    /* Defined so the assert-failure path still reaches the material lookup;
       -1 is out of range and yields the retail "HIT" fallback. */
    int target_material = -1;

    if (attack == 0) {
        return;
    }
    if (target == 0) {
        return;
    }
    weapon_class = attack->weapon_class_1c;
    if (target->iType == W8_TARGET_KIND_CHARACTER) {
        const W8Character* character = &g_status_685170.buffers.Char[target->iChar];
        int item = PCItemInACSlot(character, hit_location);
        target_material = item == -1 ? 0 : g_item_records[item].material_0c1;
    } else if (target->iType == W8_TARGET_KIND_MONSTER) {
        const W8MonsterRecord* record = GetMonsterDataByLocationID(target->iMonsterID);
        target_material = record == 0 ? 0 : record->material_263;
    } else {
        srAssertFail("FALSE", COMBAT_SOUND_CPP, 504, "MakePCHitSound : Unknown target type");
    }
    PlayMaterialImpactSound(weapon_class, target_material, volume);
}
