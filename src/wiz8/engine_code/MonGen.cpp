#include "wiz8/local_code/Sight.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/monster_generators.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/fact_state.h"
#include "wiz8/regions.h"
#include "wiz8/xstatus.h"
#include "wiz8/utility.h"
#include "random.h"
#include <math.h>
#include "wiz8/vector.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "sgp.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/MonGen.h"

#include <string.h>
#include <stdlib.h>
#include "wiz8/engine_code/GameData.h"

/* Engine Code\MonGen.cpp. InitializeEncounterTables at 0x0048A7A0 asserts
   this unit (line 211) and the 0x0048A7A0-0x0048C110 hard hull is bounded to
   it. W8MonsterGenerator's destructor and helper before the hull and the
   encounter/timer bodies after it are attribution gaps placed here
   provisionally; no assertion names their unit. */

/* These are recovered in their owning Local Code units. Retail MonGen calls
   them out of line, so keep the cross-TU seams rather than cloning their logic. */
float GetAveragePartyMemberLevel(void); /* 0x004EFB60 */
W8MonsterRecord* MonsterGroupGetRecord(W8MonsterGroup* monster_group); /* 0x00510180 */
void SetMonsterGroupFormation(W8MonsterGroup* monster_group,
                              const srVector3T<float>* formation); /* 0x0050FF40 */
unsigned char LinkMonsterGroupToLeader(W8MonsterGroup* leader,
                                       W8MonsterGroup* monster_group); /* 0x0050FC20 */

#pragma pack(push, 1)
struct W8EncounterCompanionRecord {
    short species;
    unsigned char chance;
};
#pragma pack(pop)
static_assert(sizeof(W8EncounterCompanionRecord) == 3, "W8EncounterCompanionRecord_size");

static W8EncounterCompanionRecord GetEncounterCompanion(const W8MonsterRecord* record, int index)
{
    W8EncounterCompanionRecord companion;
    memcpy(&companion, record->unknown_0c5 + index * sizeof(companion), sizeof(companion));
    return companion;
}

// FUNCTION: WIZ8 0x0048A680
W8MonsterGenerator::W8MonsterGenerator()
{
    flags = 0;
    flag_04 = 100;
    value_06 = 100;
    value_08 = 0xffff;
    node_18 = 0;
    value_1c = -1;
    m_pTimer = 0;
    memset(name, 0, sizeof(name));
    flag_44 = 1;
}

// FUNCTION: WIZ8 0x0048bdc0
W8MonsterGenerator* FindMonGenByName(const char* name)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;
    W8MonsterGenerator* generator;
    int index = 0;

    if (generators->GetCount() > 0) {
        do {
            generator = *generators->GetAt(index);

            if (strncmp(name, generator->name, sizeof(generator->name)) == 0) {
                return generator;
            }
            generators = g_world->monster_generators;
            ++index;
        } while (index < generators->GetCount());
    }
    return 0;
}

// GLOBAL: WIZ8 0x0060a6c4
int g_random_encounter_budget = 20;

// GLOBAL: WIZ8 0x0060a6c0
int g_random_encounter_limit = 10;

// GLOBAL: WIZ8 0x0065ba10
W8GrowableVector<W8MonsterGroup*> g_active_groups;

// GLOBAL: WIZ8 0x0065ba48
unsigned char g_generator_save_flag;

// GLOBAL: WIZ8 0x0060a6b6
short g_generator_default_interval = 10;

// GLOBAL: WIZ8 0x0060a6b4
short g_generator_interval_min = 100;

// GLOBAL: WIZ8 0x0060a6b8
short g_generator_interval_max = -1;

// GLOBAL: WIZ8 0x006850b6
int g_saved_encounter_budget;

// GLOBAL: WIZ8 0x0060a6c8
int g_encounter_culling_time_seconds = 180;

// GLOBAL: WIZ8 0x005ec040
const float g_generator_jitter_fraction = 0.20000000298023224f;

// GLOBAL: WIZ8 0x0065ba20
W8GrowableVector<W8EncounterTableRuntime*> g_encounter_tables;
// GLOBAL: WIZ8 0x0065ba38
W8GrowableVector<char*> g_encounter_names;
// GLOBAL: WIZ8 0x0060a6bc
int g_encounter_tables_level = -1;

/* Engine Code\\MonGen.cpp's startup loader. EncounterTables.dbs stores the
   names first, followed by a columnar record: ids, rarity, time, challenge and
   fixed 64-byte script names. Keeping those columns in their reviewed inline
   vector layout makes this useful to the later encounter path as well as to
   startup. */
// FUNCTION: WIZ8 0x0048a7a0
unsigned int InitializeEncounterTables(void)
{
    int handle = FileOpen("Data\\Databases\\EncounterTables.dbs", 0x41, 0);
    int name_count;
    int table_count;
    int index;

    if (!handle) {
        return 0;
    }
    UnloadEncounterTables();
    unsigned char success = FileRead(handle, &name_count, 4, 0);
    for (index = 0; index < name_count; ++index) {
        if (!success) {
            FileClose(handle);
            return 0;
        }
        char* name = new char[0x100];
        success = FileRead(handle, name, 0x100, 0);
        g_encounter_names.Add(name);
    }
    if (!success) {
        FileClose(handle);
        return 0;
    }
    FileRead(handle, &table_count, 4, 0);
    for (index = 0; index < table_count; ++index) {
        unsigned char record_kind;
        char name[256];
        unsigned int unknown_150;
        unsigned short version;
        unsigned char entry_count;
        unsigned short species[256];
        unsigned char rarity[256];
        unsigned char time[256];
        unsigned char challenge[256];

        FileRead(handle, &record_kind, 1, 0);
        FileRead(handle, name, sizeof(name), 0);
        FileRead(handle, &unknown_150, 4, 0);
        FileRead(handle, &version, 2, 0);
        FileRead(handle, &entry_count, 1, 0);
        W8EncounterTableRuntime* table = new W8EncounterTableRuntime;
        if (!table) {
            srAssertFail("pTable", "C:\\Projects\\Wizardry 8\\Engine Code\\MonGen.cpp", 0xd3,
                         "Out of memory allocating monster generation table.");
        }
        FileRead(handle, species, entry_count * 2, 0);
        FileRead(handle, rarity, entry_count, 0);
        FileRead(handle, time, entry_count, 0);
        FileRead(handle, challenge, entry_count, 0);
        for (int entry = 0; entry < entry_count; ++entry) {
            W8EncounterScriptName* script =
                static_cast<W8EncounterScriptName*>(malloc(sizeof(W8EncounterScriptName)));
            if (!script) {
                srAssertFail("pScript", "C:\\Projects\\Wizardry 8\\Engine Code\\MonGen.cpp", 0xdd,
                             0);
            }
            FileRead(handle, script, sizeof(*script), 0);
            table->species_ids.Add(species[entry]);
            table->rarity_class.Add(rarity[entry]);
            table->time_condition.Add(time[entry]);
            table->challenge_level.Add(challenge[entry]);
            table->script_names.Add(script);
        }
        strcpy(table->name, name);
        table->unknown_150 = unknown_150;
        g_encounter_tables.Add(table);
        if (version == 1) {
            table->version_two_flags = 0;
        } else {
            unsigned char flags;
            FileRead(handle, &flags, 1, 0);
            table->version_two_flags = flags;
        }
    }
    g_encounter_tables_level = g_status_685170.current_level;
    FileClose(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0048ac60
W8EncounterTableRuntime::~W8EncounterTableRuntime()
{
    while (script_names.GetCount() != 0) {
        free(*script_names.GetAt(0));
        script_names.RemoveAt(0);
    }
}

static const char MON_GEN_CPP[] = "C:\\Projects\\Wizardry 8\\Engine Code\\MonGen.cpp";

/* Flag bits on a generator: bit 0 suppresses spawning, bit 2 is armed, bit 3
   selects the shared default chance and interval, and bit 5 adds the
   story/faction gate. */
enum {
    W8_MONGEN_DISABLED = 1,
    W8_MONGEN_ARMED = 4,
    W8_MONGEN_USE_DEFAULT_SETTINGS = 8,
    W8_MONGEN_STORY_GATED = 0x20
};

/* Ten hours of game time. Past that the elapsed span is not distributed over
   the live groups at all - a fresh roll replaces them instead. */
enum { W8_ENCOUNTER_STALE_SECONDS = 36000 };

/* Pick the encounter entries compatible with this moment. The rarity draw uses
   the four stored rarity classes (3/7/20/70). The first pass also requires the
   entry's challenge to stay within half the party's rounded average level and
   scales rarity down as that mismatch grows; when that leaves no candidate,
   retail falls back to raw rarity plus the day/night condition. A random
   candidate is moved to slot zero because GenerateEncounter consumes slot zero. */
// FUNCTION: WIZ8 0x0048B9A0
int W8MonsterGenerator::SelectEncounterCandidates(W8EncounterTableRuntime* table,
                                                  W8GrowableVector<int>* candidates)
{
    int rarity_roll;
    int rarity_class;
    int night;
    int index;
    float party_level;

    candidates->Clear();
    night = !(g_status_685170.game_time_ms > 18000000 &&
              g_status_685170.game_time_ms <= 79200000);

    rarity_roll = Random(100);
    if (rarity_roll <= 3) {
        rarity_class = 3;
    } else if (rarity_roll <= 10) {
        rarity_class = 7;
    } else if (rarity_roll <= 30) {
        rarity_class = 20;
    } else {
        rarity_class = 70;
    }

    party_level = static_cast<float>(floor(GetAveragePartyMemberLevel() + 0.49f));
    for (index = 0; index < table->species_ids.GetCount(); ++index) {
        float challenge = static_cast<float>(*table->challenge_level.GetAt(index));
        float difference = static_cast<float>(fabs(1.0f - challenge / party_level));

        if (difference < 0.5f) {
            float adjusted =
                static_cast<float>(*table->rarity_class.GetAt(index)) * (1.0f - difference * 1.8f);
            int adjusted_class;

            if (adjusted <= 3.0f) {
                adjusted_class = 3;
            } else if (adjusted <= 10.0f) {
                adjusted_class = 7;
            } else if (adjusted <= 30.0f) {
                adjusted_class = 20;
            } else {
                adjusted_class = 70;
            }

            unsigned char time = *table->time_condition.GetAt(index);
            if (adjusted_class == rarity_class && (time == 2 || time == night)) {
                candidates->Add(index);
            }
        }
    }

    if (candidates->GetCount() == 0) {
        for (index = 0; index < table->species_ids.GetCount(); ++index) {
            unsigned char time = *table->time_condition.GetAt(index);
            if (*table->rarity_class.GetAt(index) == rarity_class && (time == 2 || time == night)) {
                candidates->Add(index);
            }
        }
    }

    if (candidates->GetCount() > 1) {
        int selected = candidates->RemoveAt(Random(candidates->GetCount()));
        candidates->InsertAt(0, selected);
    }
    return candidates->GetCount();
}

/* Bias the database's group-size dice by party-relative monster level and the
   configured difficulty. Strong encounters are pushed toward the lower half on
   normal difficulty and weak tiny encounters toward the upper half; novice and
   expert take the corresponding fixed extremes in those branches. */
// FUNCTION: WIZ8 0x0048BC30
int W8MonsterGenerator::RollEncounterGroupSize(W8MonsterRecord* record)
{
    W8Dice* dice = &record->group_size_dice_0c1;
    float relative_level =
        static_cast<float>(record->unknown_250[1]) / GetAveragePartyMemberLevel();
    int minimum = dice->base + dice->count;
    int maximum = dice->base + dice->count * dice->sides;
    float midpoint = (minimum + maximum) * 0.5f;
    int rolled = RollDice(dice);

    if (relative_level >= 1.2f) {
        if (g_settings_6850c8.difficulty == 0) {
            return minimum;
        }
        if (g_settings_6850c8.difficulty == 2) {
            return rolled;
        }
        while (static_cast<float>(rolled) > midpoint) {
            rolled = RollDice(dice);
        }
        return rolled;
    }

    /* Retail really compares the midpoint here, not relative_level. */
    if (midpoint > 0.8f) {
        return rolled;
    }
    if (g_settings_6850c8.difficulty == 2) {
        return maximum;
    }
    if (g_settings_6850c8.difficulty == 0) {
        return rolled;
    }
    while (static_cast<float>(rolled) < midpoint) {
        rolled = RollDice(dice);
    }
    return rolled;
}

/* Select one table entry, spawn its main group and up to two database-defined
   companion groups, attach scripts and formation state, and register the main
   group as a live random encounter. The bit-5 form is only valid for hostile
   factions while one of the three story facts is set. */
// FUNCTION: WIZ8 0x0048AD20
unsigned char W8MonsterGenerator::GenerateEncounter(const srVector3T<float>* position)
{
    W8GrowableVector<int> candidates;
    W8EncounterCompanionRecord companion_records[2];
    unsigned char companion_active[2] = {0, 0};
    srVector3T<float> spawn_position;
    W8EncounterTableRuntime* table;
    W8MonsterRecord* record;
    W8MonsterGroup* group;
    int selected_index;
    unsigned int species;
    const char* script;
    int encounter_weight;
    int companion_count = 0;
    int index;

    if ((flags & W8_MONGEN_DISABLED) != 0 || value_1c == -1) {
        return 0;
    }

    table = *g_encounter_tables.GetAt(value_1c);
    if (SelectEncounterCandidates(table, &candidates) == 0) {
        return 0;
    }

    selected_index = *candidates.GetAt(0);
    species = *table->species_ids.GetAt(selected_index);
    script = (*table->script_names.GetAt(selected_index))->value;
    spawn_position = *position;
    record = MonsterDBFromSpecies(species);
    if (record == 0) {
        srAssertFail("pMonsterDB", MON_GEN_CPP, 0x146,
                     FormatString("MonsterDBFromSpecies failed for species %d", species));
    }
    if (record->deleted != 0) {
        FormatDebugMessage(1,
                           "MonGen::GenerateEncounter: WARNING - DBS record for monster species %d "
                           "has been DELETED!",
                           species);
        return 0;
    }

    /* Retail computes this total even though the surviving release path never
       reads it afterwards; the external calls can still populate cycle data. */
    encounter_weight = GetMonsterCycleFallbackValue004E5B50(species);
    for (index = 0; index < 2; ++index) {
        companion_records[index] = GetEncounterCompanion(record, index);
        if (companion_records[index].species > 0 && Chance(companion_records[index].chance)) {
            companion_active[index] = 1;
            encounter_weight +=
                GetMonsterCycleFallbackValue004E5B50(companion_records[index].species);
        }
    }
    static_cast<void>(encounter_weight);

    if ((flags & W8_MONGEN_STORY_GATED) != 0) {
        if (GetFactionDisposition(record->faction_id_25f) != W8_FACTION_HOSTILE ||
            (!GetFact(0x30) && !GetFact(0x22) && !GetFact(0x31))) {
            return 0;
        }
    }

    int count = RollEncounterGroupSize(record);
    if (count == 0) {
        srAssertFail("ulNumMonsters", MON_GEN_CPP, 0x17b,
                     FormatString("Error in Monster DB: zero group size (%S)", record->name_00));
    }

    group = CreateGroup(species, count, &spawn_position, 0, 0, 1);
    group->flag_c3 = 1;
    SetMonsterGroupFormation(group, &state_0c);
    if (group != 0 && group->flag_c3 != 0 && g_active_groups.IndexOf(group) == -1) {
        g_active_groups.Add(group);
    }

    W8Monster* monster = GetMonsterByLocationID(group->value_9f);
    if (monster != 0) {
        monster->SetScript004C7F10(script != 0 && script[0] != '\0' ? script : "Default.MSF", 1);
    }

    for (index = 0; index < 2; ++index) {
        if (companion_active[index] == 0) {
            continue;
        }

        unsigned int companion_species = static_cast<unsigned short>(companion_records[index].species);
        W8MonsterRecord* companion_record = MonsterDBFromSpecies(companion_species);
        if (companion_record == 0) {
            srAssertFail("pMonsterDB", MON_GEN_CPP, 0x193,
                         FormatString("MonsterDBFromSpecies failed for species %d",
                                      companion_species));
        }
        if (companion_record->deleted != 0) {
            FormatDebugMessage(
                1,
                "MonGen::GenerateEncounter: WARNING - DBS record for monster species %d has been "
                "DELETED, chum of species %d!",
                companion_species, species);
            return 0;
        }

        int companion_group_count = RollDice(&companion_record->group_size_dice_0c1);
        companion_count += companion_group_count;
        W8MonsterGroup* companion_group =
            CreateGroup(companion_species, companion_group_count, &spawn_position, 0, 0, 1);
        companion_group->flag_c3 = 1;
        SetMonsterGroupFormation(companion_group, &state_0c);
        LinkMonsterGroupToLeader(group, companion_group);
    }

    if (g_flag_689b32 != 0 && gfCapturingVideo == 0 &&
        g_current_screen_state.id != W8_SCREEN_PLEASE_WAIT) {
        W8MonsterRecord* group_record = MonsterGroupGetRecord(group);
        const wchar_t* group_name =
            group->member_count == 1 ? group_record->name_00 : group_record->name_30;
        const wchar_t* companion_word = companion_count == 1 ? L"chum" : L"chums";
        WriteGameLog(7, L"MonGen (%S): spawned %d %s & %d %s (lvl %d)", name,
                     group->member_count, group_name, companion_count, companion_word,
                     *table->challenge_level.GetAt(selected_index));
    }
    return 1;
}

/* Refuse generation while global gameplay modes block it, while the generator
   is disabled, or while its candidate point is too close/far, visible, occupied
   or over the active-encounter budget. A forced roll bypasses the spatial and
   chance tests but not the global-mode and budget gates. */
// FUNCTION: WIZ8 0x0048B200
unsigned char W8MonsterGenerator::CanGenerateEncounter(unsigned char force)
{
    srVector3T<float> camera;
    float distance;

    if (g_generator_save_flag != 0 || g_flag_006840bc != 0 || gXStatus.fCombatMode != 0 ||
        gXStatus.fNpcDialogueMode != 0 || GetFlag68F105() != 0 || flag_44 == 0) {
        return 0;
    }

    GetCameraPosition(&camera);
    srVector3T<float> delta = state_0c - camera;
    distance = delta.Length();

    if (force == 0 && g_status_685170.value_2390 == 0) {
        if (distance > 200000.0f || distance < 35000.0f) {
            return 0;
        }
        if (g_octree_6598a4 != 0 && g_octree_6598a4->HasLineOfSight(&camera, &state_0c, 1) != 0) {
            return 0;
        }
    }

    if (g_active_groups.GetCount() >= g_random_encounter_limit) {
        CullExpiredEncounters();
        if (g_active_groups.GetCount() >= g_random_encounter_limit) {
            return 0;
        }
    }

    if (force != 0) {
        return 1;
    }

    if (g_encounter_culling_scale_fast == 1.0f) {
        srVector3T<float> lower(state_0c.x - 5000.0f, state_0c.y - 5000.0f,
                                state_0c.z - 5000.0f);
        srVector3T<float> upper(state_0c.x + 5000.0f, state_0c.y + 5000.0f,
                                state_0c.z + 5000.0f);
        int* locations = 0;
        if (g_octree_6598a4->QueryLocationsInBox(&locations, &lower, &upper, 0) > 0) {
            return 0;
        }
    }

    int chance = (flags & W8_MONGEN_USE_DEFAULT_SETTINGS) != 0
                     ? g_generator_interval_min
                     : static_cast<signed char>(flag_04);
    return Chance(chance) != 0;
}

/* Advances the random-encounter budget for the current level.
 
   Normally the budget grows by the time elapsed since the level was last
   budgeted, divided by that level's period, and both the budget and the derived
   limit are then clamped into their own per-level ranges. A reset instead sets
   the budget straight to the level maximum and declares the elapsed span stale,
   which is what makes the reset path take the reroll branch below.
 
   The elapsed span is finally added to every live group's own timestamp, so a
   group that existed through the gap ages by exactly as much as the level did.
   Past ten hours that is not worth doing and the encounters are rerolled. */
// FUNCTION: WIZ8 0x0048c810
void UpdateRandomEncounterBudget(unsigned char reset_budget)
{
    W8LevelDatabaseRecord* level;
    int elapsed;
    int index;

    if (reset_budget == 0) {
        elapsed = g_status_685170.world_clock -
                  g_status_685170.level_progress[g_status_685170.current_level].sight_clock;
        g_random_encounter_budget +=
            elapsed / g_level_records[g_status_685170.current_level].encounter_budget_period;
    } else {
        g_random_encounter_budget =
            g_level_records[g_status_685170.current_level].maximum_encounter_budget;
        elapsed = W8_ENCOUNTER_STALE_SECONDS + 1;
    }
    level = &g_level_records[g_status_685170.current_level];
    ClampInteger(&g_random_encounter_budget, level->minimum_encounter_budget,
                 level->maximum_encounter_budget);
    g_random_encounter_limit = g_random_encounter_budget;
    ClampInteger(&g_random_encounter_limit, level->minimum_random_encounters,
                 level->maximum_random_encounters);
    if (elapsed > W8_ENCOUNTER_STALE_SECONDS) {
        RollRandomEncounters();
        return;
    }
    for (index = 0; index < g_active_groups.count; ++index) {
        W8MonsterGroup* group = *g_active_groups.GetAt(index);

        group->spawn_time += elapsed;
    }
}

// FUNCTION: WIZ8 0x0048c9f0
void DespawnAllActiveMonsterGroups0048C9F0(void)
{
    while (g_active_groups.count > 0) {
        DespawnMonsterGroup(g_active_groups.data[g_active_groups.count - 1]);
    }
}

/* Put the encounter-culling scale back to its fast default and rearm every
   loaded generator's interval timer. */
// FUNCTION: WIZ8 0x0048cbe0
void ResetMonsterGeneratorTimers0048CBE0(void)
{
    g_encounter_culling_scale_fast = 1.0f;
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;

    for (int index = 0; index < generators->count; ++index) {
        W8MonsterGenerator* generator = *generators->GetAt(index);

        generator->m_pTimer->ResetDurationScale();
    }
}

// GLOBAL: WIZ8 0x0060a6cc
float g_encounter_culling_scale_fast = 1.0f;

// GLOBAL: WIZ8 0x005ec918
const float g_encounter_culling_rate = 2880.0f;

// GLOBAL: WIZ8 0x005eca90
const float g_encounter_culling_distance = 0.0f;

/* Retires random encounters that have outlived their welcome.
 
   A group is a candidate once more than the level's culling span has passed
   since it was budgeted, where the span is scaled one way normally and another
   when 0x00504910 reports the faster clock. A candidate is only actually
   despawned when the party is far enough away from its lead member - or when
   the override at 0x00687500 says to drop it regardless of distance.
 
   The elapsed span is compared as unsigned, so a group whose timestamp is ahead
   of the clock reads as very old rather than as not yet due. Preserved as
   found. */
// FUNCTION: WIZ8 0x0048c8e0
void CullExpiredEncounters(void)
{
    srVector3T<float> party;
    srVector3T<float> position;
    float span;
    int index;

    GetCameraPosition(&party);
    if (IsSightRangeOverridden() == 0) {
        span = g_level_records[g_status_685170.current_level].encounter_culling_seconds *
               g_sight_default_005ec254;
    } else {
        span = g_level_records[g_status_685170.current_level].encounter_culling_seconds *
               g_encounter_culling_scale_fast * g_encounter_culling_rate;
    }
    for (index = 0; index < g_active_groups.count; ++index) {
        W8MonsterGroup* group = *g_active_groups.GetAt(index);

        if (span < static_cast<float>(static_cast<unsigned int>(g_status_685170.world_clock -
                                                                group->spawn_time))) {
            W8Monster* monster = GetMonsterByLocationID(group->value_9f);

            position = monster->GetPosition();
            srVector3T<float> delta = position - party;

            if (g_encounter_culling_distance < delta.Length() || g_status_685170.value_2390 != 0) {
                DespawnMonsterGroup(group);
            }
        }
    }
}

/* How many monster generators the loaded world carries. */
// FUNCTION: WIZ8 0x0048bd80
int GetMonsterGeneratorCount(void)
{
    return g_world->monster_generators->GetCount();
}

/* One of them by index. Out of range answers null, which is why the bounds test
   appears twice: once here, rejecting the index outright, and once inside the
   vector's own bounds-checked read, which would otherwise fall back to element
   zero. */
// FUNCTION: WIZ8 0x0048bd90
W8MonsterGenerator* GetMonsterGenerator(int index)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;

    if (index >= generators->GetCount()) {
        return 0;
    }
    return *generators->GetAt(index);
}

/* One loaded encounter table by index. */
// FUNCTION: WIZ8 0x0048ad00
W8EncounterTableRuntime* GetEncounterTable(int index)
{
    if (index < g_encounter_tables.GetCount()) {
        return *g_encounter_tables.GetAt(index);
    }
    return 0;
}

/* Appends through the world's ordinary growable vector. Add is header-visible,
   so VC6 expands both it and Grow into this caller just as the image does. */
// FUNCTION: WIZ8 0x0048be30
void AddMonsterGenerator(W8MonsterGenerator* generator)
{
    g_world->monster_generators->Add(generator);
}

/* Destroys every generator and empties the list. The count is taken once up
   front and the list is emptied by resetting it rather than by removing
   elements, so the walk indexes an array it is deleting out of - which is safe
   only because nothing shifts. */
// FUNCTION: WIZ8 0x0048bf70
void DestroyMonsterGenerators(void)
{
    int count = g_world->monster_generators->GetCount();
    W8MonsterGenerator* generator;
    int index;

    if (count < 1) {
        g_world->monster_generators->Clear();
        return;
    }
    for (index = 0; index < count; ++index) {
        delete *g_world->monster_generators->GetAt(index);
    }
    g_world->monster_generators->Clear();
}

/* Restore the full MONG encounter state. Older save versions omit later global
   fields; version four and below also have no saved encounter budget. The
   random-encounter budget is always clamped to the current level after loading,
   and the culling span is finally reset from that level's database row. */
// FUNCTION: WIZ8 0x0048c110
unsigned char W8MonsterGenerator::LoadAll(int save_handle)
{
    W8LevelDatabaseRecord* level;
    unsigned char success;
    int version;
    int count;
    int index;

    DestroyMonsterGenerators();
    FileRead(save_handle, &version, 4, 0);
    if (version > 4) {
        FileRead(save_handle, &g_saved_encounter_budget, 4, 0);
    } else {
        g_saved_encounter_budget = 100;
    }
    FileRead(save_handle, &g_random_encounter_budget, 4, 0);
    if (version > 1) {
        FileRead(save_handle, &g_encounter_culling_time_seconds, 4, 0);
    }
    if (version > 2) {
        FileRead(save_handle, &g_generator_save_flag, 1, 0);
        FileRead(save_handle, &g_generator_interval_min, 2, 0);
        FileRead(save_handle, &g_generator_default_interval, 2, 0);
        FileRead(save_handle, &g_generator_interval_max, 2, 0);
    }

    success = FileRead(save_handle, &count, 4, 0);
    for (index = 0; index < count && success != 0; ++index) {
        W8MonsterGenerator* generator = new W8MonsterGenerator;
        if (generator == 0) {
            srAssertFail("pMonGen", MON_GEN_CPP, 0x48a, "MonGen::LoadAll() out of memory");
        }
        success = generator->Load(save_handle);
        g_world->monster_generators->Add(generator);
    }

    level = &g_level_records[g_status_685170.current_level];
    ClampInteger(&g_random_encounter_budget, level->minimum_encounter_budget,
                 level->maximum_encounter_budget);
    g_random_encounter_limit = g_random_encounter_budget;
    ClampInteger(&g_random_encounter_limit, level->minimum_random_encounters,
                 level->maximum_random_encounters);
    g_encounter_culling_time_seconds = level->encounter_culling_seconds;
    return success;
}

/* Writes one generator-independent save record back onto an already loaded
   world. A record whose name no longer exists is still consumed completely: a
   temporary interval gate reads and discards the saved timer payload. */
// FUNCTION: WIZ8 0x0048c470
void LoadMonsterGenerators(int handle)
{
    char name[32];
    W8MonsterGenerator* generator;
    unsigned char flag_44;
    unsigned int flags;
    int version;
    int count;
    int index;
    int search;

    FileRead(handle, &version, 4, 0);
    FileRead(handle, &g_generator_save_flag, 1, 0);
    FileRead(handle, &count, 4, 0);
    for (index = 0; index < count; ++index) {
        FileRead(handle, name, sizeof(name), 0);
        FileRead(handle, &flag_44, 1, 0);
        FileRead(handle, &flags, 4, 0);

        generator = 0;
        for (search = 0; search < g_world->monster_generators->GetCount(); ++search) {
            W8MonsterGenerator* candidate = *g_world->monster_generators->GetAt(search);
            if (strncmp(name, candidate->name, sizeof(name)) == 0) {
                generator = candidate;
                break;
            }
        }
        if (generator != 0) {
            generator->flag_44 = flag_44;
            generator->flags = flags;
        }

        if (version > 1) {
            if (generator != 0) {
                generator->m_pTimer->Load(handle);
                generator->m_pTimer->Arm();
            } else {
                W8IntervalGate* timer = new W8IntervalGate;
                timer->m_flags &= 0xfffd;
                timer->Load(handle);
                delete timer;
            }
        }
    }
}

/* Runs one encounter roll for every generator that is armed and whose own
   precondition passes. The count is taken once, but the vector is re-read for
   each element, which is what the repeated bounds test in the original is. */
// FUNCTION: WIZ8 0x0048c600
void RunMonsterGenerators(void)
{
    int count = g_world->monster_generators->GetCount();
    W8MonsterGenerator* generator;
    int index;

    for (index = 0; index < count; ++index) {
        generator = *g_world->monster_generators->GetAt(index);
        if (generator->m_pTimer != 0 && generator->m_pTimer->PollElapsedIntervals() != 0) {
            if (generator->CanGenerateEncounter(0) != 0) {
                generator->GenerateEncounter(&generator->state_0c);
            }
            generator->Reset();
        }
    }
}

/* Remove one random-encounter leader from the live registry. During combat,
   removing an actually registered group also spends one unit of the encounter
   budget and recomputes the level-clamped active limit. */
// FUNCTION: WIZ8 0x0048c670
void UnregisterActiveEncounterGroup(W8MonsterGroup* group)
{
    if (group == 0 || group->flag_c3 == 0) {
        return;
    }
    unsigned char removed = g_active_groups.Remove(group);
    if (gXStatus.fCombatMode == 0 || removed == 0) {
        return;
    }

    W8LevelDatabaseRecord* level = &g_level_records[g_status_685170.current_level];
    --g_random_encounter_budget;
    ClampInteger(&g_random_encounter_budget, level->minimum_encounter_budget,
                 level->maximum_encounter_budget);
    g_random_encounter_limit = g_random_encounter_budget;
    ClampInteger(&g_random_encounter_limit, level->minimum_random_encounters,
                 level->maximum_random_encounters);
}

/* Register a live random-encounter leader exactly once. */
// FUNCTION: WIZ8 0x0048c750
void RegisterActiveEncounterGroup(W8MonsterGroup* group)
{
    if (group == 0 || group->flag_c3 == 0 || g_active_groups.IndexOf(group) != -1) {
        return;
    }
    g_active_groups.Add(group);
}

/* Releases every loaded encounter table and every table name, and marks no
   level loaded. Both walks re-read their base pointer and count from memory
   each iteration, because destroying a table can reallocate neither but the
   original reloads them anyway. */
// FUNCTION: WIZ8 0x0048a710
void UnloadEncounterTables(void)
{
    int index;
    for (index = 0; index < g_encounter_tables.GetCount(); ++index) {
        delete *g_encounter_tables.GetAt(index);
    }
    g_encounter_tables.Clear();
    for (index = 0; index < g_encounter_names.GetCount(); ++index) {
        delete[] *g_encounter_names.GetAt(index);
    }
    g_encounter_names.Clear();
    g_encounter_tables_level = -1;
}

/* Writes the world's generators to a save. The record version goes out first,
   then a shared flag byte, then the count, and then each generator as its name,
   its trailing flag, its own flag word and whatever 0x0043A770 appends. */
// FUNCTION: WIZ8 0x0048c3b0
void SaveMonsterGenerators(int handle)
{
    W8MonsterGenerator* generator;
    int count;
    int version;
    int index;

    version = 3;
    count = g_world->monster_generators->GetCount();
    FileWrite(handle, &version, 4, 0);
    FileWrite(handle, &g_generator_save_flag, 1, 0);
    FileWrite(handle, &count, 4, 0);
    for (index = 0; index < count; ++index) {
        generator = *g_world->monster_generators->GetAt(index);
        FileWrite(handle, generator->name, 0x20, 0);
        FileWrite(handle, &generator->flag_44, 1, 0);
        FileWrite(handle, &generator->flags, 4, 0);
        generator->m_pTimer->Save(handle);
    }
}

/* Writes one generator to a save, field by field rather than as a block: the
   record on disk is narrower than the structure and skips +0x05, +0x0A and
   +0x18. The leading byte is written uninitialised - a one-byte local the
   original never assigns. Preserved as found. */
// FUNCTION: WIZ8 0x0048b520
void W8MonsterGenerator::Save(int handle)
{
    unsigned char leading;

    FileWrite(handle, &leading, 1, 0);
    FileWrite(handle, name, 0x20, 0);
    FileWrite(handle, &flag_44, 1, 0);
    FileWrite(handle, &flags, 4, 0);
    FileWrite(handle, &flag_04, 1, 0);
    FileWrite(handle, &value_06, 2, 0);
    FileWrite(handle, &value_08, 2, 0);
    FileWrite(handle, &state_0c.x, 4, 0);
    FileWrite(handle, &state_0c.y, 4, 0);
    FileWrite(handle, &state_0c.z, 4, 0);
    FileWrite(handle, &value_1c, 4, 0);
    m_pTimer->Save(handle);
}

/* Reads one generator back. The leading byte is a record version: from 3 the
   name and its trailing flag are stored too, below that they are not, and the
   two paths converge on the same eight common fields. Every read is chained
   through the same conjunction, so the first failure abandons the rest and the
   record is reported bad; the timer is rearmed either way, and the armed bit is
   always cleared on the way out so a loaded generator starts disarmed. */
// FUNCTION: WIZ8 0x0048b5e0
unsigned char W8MonsterGenerator::Load(int handle)
{
    unsigned char version;
    unsigned char ok;
    unsigned char loaded;

    ok = FileRead(handle, &version, 1, 0);
    if (static_cast<signed char>(version) >= 3) {
        ok = ok && FileRead(handle, name, 0x20, 0);
        ok = ok && FileRead(handle, &flag_44, 1, 0);
    }
    loaded = ok && FileRead(handle, &flags, 4, 0) && FileRead(handle, &flag_04, 1, 0) &&
             FileRead(handle, &value_06, 2, 0) && FileRead(handle, &value_08, 2, 0) &&
             FileRead(handle, &state_0c.x, 4, 0) && FileRead(handle, &state_0c.y, 4, 0) &&
             FileRead(handle, &state_0c.z, 4, 0) && FileRead(handle, &value_1c, 4, 0);
    Reset();
    if (static_cast<signed char>(version) > 1) {
        m_pTimer->Load(handle);
        m_pTimer->Arm();
    }
    flags &= ~static_cast<unsigned int>(W8_MONGEN_ARMED);
    return loaded;
}

/* Removes one generator from the world's list by identity and destroys it. The
   search stops at the first match and the tail is shifted down over it; a
   generator that is not in the list is left alone entirely, so this is safe to
   call on one that has already been removed. */
// FUNCTION: WIZ8 0x0048beb0
void RemoveMonsterGenerator(W8MonsterGenerator* generator)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;
    W8MonsterGenerator* removed;
    int index = generators->IndexOf(generator);

    if (index == -1) {
        return;
    }
    removed = generators->RemoveAt(index);
    delete removed;
}

/* Rearms the generator's timer, creating it on first use. The delay is the
   configured interval jittered by a uniform draw over twice its jitter
   fraction, so the mean is the interval itself. Bit 3 selects the shared
   default interval; a clear bit uses the generator's custom +0x06 interval. */
// FUNCTION: WIZ8 0x0048b420
void W8MonsterGenerator::Reset()
{
    short interval;
    float jitter;

    if (m_pTimer == 0) {
        m_pTimer = new W8IntervalGate;
        if (m_pTimer == 0) {
            srAssertFail("m_pTimer", MON_GEN_CPP, 0x217,
                         "MonGen::Reset() out of memory allocating m_pTimer");
        }
        m_pTimer->m_flags &= 0xfffd;
    }
    interval = (flags & W8_MONGEN_USE_DEFAULT_SETTINGS) != 0 ? g_generator_default_interval
                                                             : value_06;
    jitter = interval * g_generator_jitter_fraction;
    m_pTimer->SetDuration(static_cast<float>(Random(static_cast<int>(jitter) * 2 + 1)) + interval -
                          jitter);
    m_pTimer->Arm();
}

/* Loads the generator's marker from Data\\Items3D\\Bitmaps and hands it over.
   Written once because the arm path and the reload below both compile it. */
static __inline void LoadMonsterGeneratorMarkerInline(W8MonsterGenerator* generator)
{
    void* context[2];
    W8Item* marker = 0;

    context[1] = const_cast<char*>("Data\\Items3D\\Bitmaps");
    context[0] = g_world;
    if (Function49F4A0(context, "mongen", &marker, 0) == 0) {
        srAssertFail("fSuccess", MON_GEN_CPP, 0x309, "Couldn't load mongen.itm");
    }
    generator->node_18 = marker;
    if (marker != 0) {
        marker->SetLocation0049F720(&generator->state_0c);
        marker->ApplyRepTransform0049FAA0();
    }
}

/* Arms or disarms the generator. Arming loads its marker from
   Data\\Items3D\\Bitmaps the first time and hands the caller's node over;
   disarming drops the flag and notifies the world. Both directions are no-ops
   when the flag already reads as asked. */
// FUNCTION: WIZ8 0x0048b770
void W8MonsterGenerator::SetActive(unsigned char active, W8Item* node)
{
    (void)node;
    if (((flags >> 2) & 1) == active) {
        return;
    }
    if (active == 0) {
        flags &= ~static_cast<unsigned int>(W8_MONGEN_ARMED);
        if (node_18 != 0) {
            node_18->DetachMesh0049FA30(g_world);
        }
        return;
    }
    flags |= W8_MONGEN_ARMED;
    if (node_18 == 0) {
        LoadMonsterGeneratorMarkerInline(this);
    }
    node_18->AttachMesh0049F900(g_world);
}

/* Writes the encounter subsystem's own state to a save, ahead of the generator
   records themselves: the record version, the level's saved budget, the running
   budget and culling span, the shared flag byte, three shared interval words,
   and then the count followed by that many generator records. */
// FUNCTION: WIZ8 0x0048c020
void SaveEncounterState(int handle)
{
    int count;
    int version;
    int index;

    count = g_world->monster_generators->GetCount();
    version = 5;
    FileWrite(handle, &version, 4, 0);
    FileWrite(handle, &g_saved_encounter_budget, 4, 0);
    FileWrite(handle, &g_random_encounter_budget, 4, 0);
    FileWrite(handle, &g_encounter_culling_time_seconds, 4, 0);
    FileWrite(handle, &g_generator_save_flag, 1, 0);
    FileWrite(handle, &g_generator_interval_min, 2, 0);
    FileWrite(handle, &g_generator_default_interval, 2, 0);
    FileWrite(handle, &g_generator_interval_max, 2, 0);
    FileWrite(handle, &count, 4, 0);
    for (index = 0; index < count; ++index) {
        (*g_world->monster_generators->GetAt(index))->Save(handle);
    }
}

/* Destroy every current random group, make an exact-count copy of the world's
   generator vector, randomize it with count independent pair swaps, then force
   one generation attempt from every copied entry. */
// FUNCTION: WIZ8 0x0048ca20
void RollRandomEncounters(void)
{
    while (g_active_groups.GetCount() > 0) {
        DespawnMonsterGroup(*g_active_groups.GetAt(g_active_groups.GetCount() - 1));
    }

    W8GrowableVector<W8MonsterGenerator*> generators(*g_world->monster_generators);
    int count = generators.GetCount();
    if (count > 1) {
        for (int remaining = count; remaining > 0; --remaining) {
            int first = Random(count);
            int second = Random(count);
            if (first != second) {
                W8MonsterGenerator* swap = *generators.GetAt(first);
                generators.SetAt(first, *generators.GetAt(second));
                generators.SetAt(second, swap);
            }
        }
    }

    for (int index = 0; index < count; ++index) {
        W8MonsterGenerator* generator = *generators.GetAt(index);
        if (generator->CanGenerateEncounter(1) != 0) {
            generator->GenerateEncounter(&generator->state_0c);
        }
    }
}

/* Apply one duration scale to the encounter culling clock and every loaded
   generator timer. */
// FUNCTION: WIZ8 0x0048cb80
void SetMonsterGeneratorDurationScale(float scale)
{
    g_encounter_culling_scale_fast = scale;
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;

    for (int index = 0; index < generators->GetCount(); ++index) {
        (*generators->GetAt(index))->m_pTimer->SetDurationScale(scale);
    }
}

/* Releases what a generator hangs off itself. The world is notified only when
   the generator was still holding the armed bit, which is why that clear sits
   inside the first node's guard rather than beside it. The generator's own
   storage is not freed here - the callers do that. */
// FUNCTION: WIZ8 0x0048a6c0
W8MonsterGenerator::~W8MonsterGenerator()
{
    if (node_18 != 0) {
        if ((flags >> 2 & 1) != 0) {
            flags &= ~static_cast<unsigned int>(W8_MONGEN_ARMED);
            node_18->DetachMesh0049FA30(g_world);
        }
        delete node_18;
    }
    delete m_pTimer;
}

/* Moves the generator. The scene is only told when the generator has a marker. */
// FUNCTION: WIZ8 0x0048b730
void W8MonsterGenerator::SetState(const srVector3T<float>* state)
{
    state_0c = *state;
    if (node_18 != 0) {
        node_18->SetLocation0049F720(state);
        node_18->ApplyRepTransform0049FAA0();
    }
}

/* Reloads the generator's marker and then applies an armed state to it. The
   marker load is unconditional here - unlike the arm path, which only loads one
   when the generator has none - so this is what replaces a marker rather than
   what installs the first. The state application afterwards is the same body
   SetActive is, inlined, including its own conditional second load. */
// FUNCTION: WIZ8 0x0048b850
void W8MonsterGenerator::Reload(int unused, unsigned char active)
{
    (void)unused;
    LoadMonsterGeneratorMarkerInline(this);
    if (((flags >> 2) & 1) == active) {
        return;
    }
    if (active == 0) {
        flags &= ~static_cast<unsigned int>(W8_MONGEN_ARMED);
        if (node_18 != 0) {
            node_18->DetachMesh0049FA30(g_world);
        }
        return;
    }
    flags |= W8_MONGEN_ARMED;
    if (node_18 == 0) {
        LoadMonsterGeneratorMarkerInline(this);
    }
    node_18->AttachMesh0049F900(g_world);
}

// FUNCTION: WIZ8 0x0048cc30
void W8MonsterGenerator::SetName(const char* new_name)
{
    strncpy(name, new_name, sizeof(name));
    name[sizeof(name) - 1] = '\0';
}

/* Store the table index and mark HARASSMENT tables with bit 5. Retail never
   clears that bit here when a later table is not HARASSMENT. */
// FUNCTION: WIZ8 0x0048cc50
void W8MonsterGenerator::SetEncounterTable(int index)
{
    value_1c = index;
    if (index < g_encounter_tables.GetCount()) {
        W8EncounterTableRuntime* table = *g_encounter_tables.GetAt(index);
        if (table != 0 && strncmp(table->name, "HARASSMENT", 10) == 0) {
            flags |= W8_MONGEN_STORY_GATED;
        }
    }
}

/* Return the first table whose name starts with the complete caller string. */
// FUNCTION: WIZ8 0x0048cca0
int FindEncounterTableByName(const char* name)
{
    for (int index = 0; index < g_encounter_tables.GetCount(); ++index) {
        W8EncounterTableRuntime* table = *g_encounter_tables.GetAt(index);
        if (strncmp(table->name, name, strlen(name)) == 0) {
            return index;
        }
    }
    return -1;
}
