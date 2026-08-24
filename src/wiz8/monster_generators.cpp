#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/monster_generators.h"
#include "wiz8/game_status.h"
#include "wiz8/utility.h"
#include "random.h"
#include <math.h>
#include "wiz8/vector.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"

#include <string.h>

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

extern int g_random_encounter_budget;
extern int g_random_encounter_limit;
extern int g_active_group_count;            /* 0x0065BA14 */
extern W8MonsterGroup** g_active_groups;    /* 0x0065BA1C */
extern void RollRandomEncounters(void);     /* 0x0048CA20 */
extern int Function43A5D0(void);                             /* 0x0043A5D0 */
extern unsigned char Function48B200(int value);              /* 0x0048B200 */
extern void GenerateEncounter(void* encounter_state);         /* 0x0048AD20 */
extern void DestroyEncounterTable(W8EncounterTableRuntime* table); /* 0x0048AC60 */
extern void Function43A770(int handle);                      /* 0x0043A770 */
extern unsigned char g_generator_save_flag;                  /* 0x0065BA48 */
extern W8MonsterGeneratorNode* Function43A4E0(void);         /* 0x0043A4E0 */
extern void Function439B80(float delay);                     /* 0x00439B80 */
extern void Function43A530(void);                            /* 0x0043A530 */
extern unsigned char Function49F4A0(void* context, const char* name,
                                    void* out, int value);   /* 0x0049F4A0 */
extern short g_generator_default_interval;                   /* 0x0060A6B6 */
extern short g_generator_interval_min;                       /* 0x0060A6B4 */
extern short g_generator_interval_max;                       /* 0x0060A6B8 */
extern int g_saved_encounter_budget;                         /* 0x006850B6 */
extern int g_encounter_culling_time_seconds;
extern void Function43A690(int handle);                      /* 0x0043A690 */
extern const float g_generator_jitter_fraction;              /* 0x005EC040 */

W8EncounterTableRuntime** g_encounter_tables;
char** g_encounter_names;
unsigned int g_encounter_name_count;
unsigned int g_encounter_table_count;
int g_encounter_tables_level = -1;

static unsigned int g_encounter_name_capacity;
static unsigned int g_encounter_table_capacity;

void UnloadEncounterTables(void);

static unsigned char GrowPointerArray(void*** values, unsigned int* capacity,
                                      unsigned int wanted)
{
    void** replacement;
    unsigned int next = *capacity ? *capacity + 5 : 5;

    if (next < wanted) {
        next = wanted;
    }
    replacement = static_cast<void**>(::operator new(next * sizeof(void*)));
    if (!replacement) {
        return 0;
    }
    if (*values) {
        memcpy(replacement, *values, *capacity * sizeof(void*));
        ::operator delete(*values);
    }
    *values = replacement;
    *capacity = next;
    return 1;
}

/* Engine Code\\MonGen.cpp's startup loader.  EncounterTables.dbs stores the
   names first, followed by a columnar record: ids, rarity, time, challenge and
   fixed 64-byte script names.  Keeping those columns in their reviewed inline
   vector layout makes this useful to the later encounter path as well as to
   startup. */
// FUNCTION: WIZ8 0x0048a7a0
extern "C" unsigned int InitializeEncounterTables(void)
{
    char archive_path[] = "Data\\Databases\\EncounterTables.dbs";
    int handle = FileOpen(archive_path, 0x41, 0);
    int name_count;
    int table_count;
    int index;

    if (!handle) {
        return 0;
    }
    UnloadEncounterTables();
    if (!ReadVirtualFile(handle, &name_count, 4, 0)) {
        CloseVirtualFile(handle);
        return 0;
    }
    for (index = 0; index < name_count; ++index) {
        char* name = static_cast<char*>(::operator new(0x100));
        if (!name || !ReadVirtualFile(handle, name, 0x100, 0)) {
            CloseVirtualFile(handle);
            return 0;
        }
        if (g_encounter_name_count == g_encounter_name_capacity &&
            !GrowPointerArray(reinterpret_cast<void***>(&g_encounter_names),
                              &g_encounter_name_capacity,
                              g_encounter_name_count + 1)) {
            CloseVirtualFile(handle);
            return 0;
        }
        g_encounter_names[g_encounter_name_count++] = name;
    }
    if (!ReadVirtualFile(handle, &table_count, 4, 0)) {
        CloseVirtualFile(handle);
        return 0;
    }
    for (index = 0; index < table_count; ++index) {
        unsigned char record_kind;
        char name[256];
        unsigned int unknown_150;
        unsigned short version;
        unsigned char entry_count;
        unsigned char flags = 0;
        int initial_capacity;
        unsigned short species[256];
        unsigned char rarity[256];
        unsigned char time[256];
        unsigned char challenge[256];
        W8EncounterTableRuntime* table;
        int entry;

        if (!ReadVirtualFile(handle, &record_kind, 1, 0) ||
            !ReadVirtualFile(handle, name, sizeof(name), 0) ||
            !ReadVirtualFile(handle, &unknown_150, 4, 0) ||
            !ReadVirtualFile(handle, &version, 2, 0) ||
            !ReadVirtualFile(handle, &entry_count, 1, 0)) {
            CloseVirtualFile(handle);
            return 0;
        }
        table = new W8EncounterTableRuntime;
        if (!table) {
            CloseVirtualFile(handle);
            return 0;
        }
        initial_capacity = entry_count > 5 ? entry_count : 5;
        if (!table->species_ids.Grow(initial_capacity) ||
            !table->rarity_class.Grow(initial_capacity) ||
            !table->time_condition.Grow(initial_capacity) ||
            !table->challenge_level.Grow(initial_capacity) ||
            !table->script_names.Grow(initial_capacity) ||
            !ReadVirtualFile(handle, species, entry_count * 2, 0) ||
            !ReadVirtualFile(handle, rarity, entry_count, 0) ||
            !ReadVirtualFile(handle, time, entry_count, 0) ||
            !ReadVirtualFile(handle, challenge, entry_count, 0)) {
            CloseVirtualFile(handle);
            return 0;
        }
        for (entry = 0; entry < entry_count; ++entry) {
            W8EncounterScriptName* script = new W8EncounterScriptName;
            if (!script || !ReadVirtualFile(handle, script, 0x40, 0)) {
                CloseVirtualFile(handle);
                return 0;
            }
            table->species_ids.Add(species[entry]);
            table->rarity_class.Add(rarity[entry]);
            table->time_condition.Add(time[entry]);
            table->challenge_level.Add(challenge[entry]);
            table->script_names.Add(script);
        }
        memcpy(table->name, name, sizeof(name));
        table->unknown_150 = unknown_150;
        if (version != 1 && !ReadVirtualFile(handle, &flags, 1, 0)) {
            CloseVirtualFile(handle);
            return 0;
        }
        table->version_two_flags = flags;
        if (g_encounter_table_count == g_encounter_table_capacity &&
            !GrowPointerArray(reinterpret_cast<void***>(&g_encounter_tables),
                              &g_encounter_table_capacity,
                              g_encounter_table_count + 1)) {
            CloseVirtualFile(handle);
            return 0;
        }
        g_encounter_tables[g_encounter_table_count++] = table;
        (void)record_kind;
    }
    g_encounter_tables_level = g_current_level;
    CloseVirtualFile(handle);
    return 1;
}

static const char MON_GEN_CPP[] = "C:\\Projects\\Wizardry 8\\Engine Code\\MonGen.cpp";

/* Flag bits on a generator: bit 2 is armed, bit 3 selects the shared default
   interval over the generator's own. */
enum { W8_MONGEN_ARMED = 4, W8_MONGEN_USE_DEFAULT_INTERVAL = 8 };

/* Ten hours of game time. Past that the elapsed span is not distributed over
   the live groups at all - a fresh roll replaces them instead. */
enum { W8_ENCOUNTER_STALE_SECONDS = 36000 };

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
        elapsed = g_world_clock_00686a48 -
                  g_level_progress[g_current_level].sight_clock;
        g_random_encounter_budget +=
            elapsed / g_level_records[g_current_level].encounter_budget_period;
    } else {
        g_random_encounter_budget =
            g_level_records[g_current_level].maximum_encounter_budget;
        elapsed = W8_ENCOUNTER_STALE_SECONDS + 1;
    }
    level = &g_level_records[g_current_level];
    ClampInteger(&g_random_encounter_budget, level->minimum_encounter_budget,
                 level->maximum_encounter_budget);
    g_random_encounter_limit = g_random_encounter_budget;
    ClampInteger(&g_random_encounter_limit, level->minimum_random_encounters,
                 level->maximum_random_encounters);
    if (elapsed > W8_ENCOUNTER_STALE_SECONDS) {
        RollRandomEncounters();
        return;
    }
    for (index = 0; index < g_active_group_count; ++index) {
        W8MonsterGroup* group =
            index < g_active_group_count ? g_active_groups[index] : g_active_groups[0];

        group->spawn_time += elapsed;
    }
}

extern unsigned char IsSightRangeOverridden(void);          /* 0x00504910 */
extern void DespawnMonsterGroup(W8MonsterGroup* group); /* 0x00510930 */
extern unsigned char g_force_encounter_culling;     /* 0x00687500 */
extern void GetPartyPosition(srVector3T<float>* position); /* 0x00421070 */

/* Read rather than spelled as literals: each is a relocated .rdata load, so the
   value does not affect the match and inventing one would claim a number the
   port has not established. */
extern const float g_encounter_culling_scale;       /* 0x005EC254 */
extern const float g_encounter_culling_scale_fast;  /* 0x0060A6CC */
extern const float g_encounter_culling_rate;        /* 0x005EC918 */
extern const float g_encounter_culling_distance;    /* 0x005ECA90 */

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

    GetPartyPosition(&party);
    if (IsSightRangeOverridden() == 0) {
        span = g_level_records[g_current_level].encounter_culling_seconds *
               g_encounter_culling_scale;
    } else {
        span = g_level_records[g_current_level].encounter_culling_seconds *
               g_encounter_culling_scale_fast * g_encounter_culling_rate;
    }
    for (index = 0; index < g_active_group_count; ++index) {
        W8MonsterGroup* group =
            index < g_active_group_count ? g_active_groups[index] : g_active_groups[0];

        if (span < static_cast<float>(
                       static_cast<unsigned int>(g_world_clock_00686a48 - group->spawn_time))) {
            W8Monster* monster = GetMonsterByLocationID(group->value_9f);

            position = monster->GetPosition();
            float dx = position.x - party.x;
            float dy = position.y - party.y;
            float dz = position.z - party.z;

            if (g_encounter_culling_distance <
                    static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz)) ||
                g_force_encounter_culling != 0) {
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

/* One loaded encounter table by index. Unlike the generator lookup this one has
   no second bounds test, because the tables are a plain array rather than a
   vector. */
// FUNCTION: WIZ8 0x0048ad00
W8EncounterTableRuntime* GetEncounterTable(int index)
{
    if (index < static_cast<int>(g_encounter_table_count)) {
        return g_encounter_tables[index];
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

/* Releases one generator and everything it hangs off itself. Both the list
   teardown and the single remove compile this, so it is written once.
   The world is notified only when the generator was holding the flag it clears,
   which is why the notify sits inside the first pointer's guard rather than
   beside it. */
static __inline void DestroyMonsterGeneratorInline(W8MonsterGenerator* generator)
{
    if (generator != 0) {
        if (generator->node_18 != 0) {
            if ((generator->flags >> 2 & 1) != 0) {
                generator->flags &= ~4u;
                generator->node_18->DetachMesh0049FA30(g_world);
            }
            delete generator->node_18;
        }
        delete generator->m_pTimer;
        ::operator delete(generator);
    }
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
        DestroyMonsterGeneratorInline(*g_world->monster_generators->GetAt(index));
    }
    g_world->monster_generators->Clear();
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
        if (generator->m_pTimer != 0 && Function43A5D0() != 0) {
            if (Function48B200(0) != 0) {
                GenerateEncounter(&generator->state_0c);
            }
            generator->Reset();
        }
    }
}

/* Releases every loaded encounter table and every table name, and marks no
   level loaded. Both walks re-read their base pointer and count from memory
   each iteration, because destroying a table can reallocate neither but the
   original reloads them anyway. */
// FUNCTION: WIZ8 0x0048a710
void UnloadEncounterTables(void)
{
    W8EncounterTableRuntime* table;
    int index;

    for (index = 0; index < static_cast<int>(g_encounter_table_count); ++index) {
        table = index < static_cast<int>(g_encounter_table_count)
                    ? g_encounter_tables[index]
                    : g_encounter_tables[0];
        if (table != 0) {
            DestroyEncounterTable(table);
            ::operator delete(table);
        }
    }
    g_encounter_table_count = 0;
    for (index = 0; index < static_cast<int>(g_encounter_name_count); ++index) {
        ::operator delete(index < static_cast<int>(g_encounter_name_count)
                              ? g_encounter_names[index]
                              : g_encounter_names[0]);
    }
    g_encounter_name_count = 0;
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
        Function43A770(handle);
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
    FileWrite(handle, &state_0c, 4, 0);
    FileWrite(handle, &state_10, 4, 0);
    FileWrite(handle, &state_14, 4, 0);
    FileWrite(handle, &value_1c, 4, 0);
    Function43A770(handle);
}

/* Reads one generator back. The leading byte is a record version: from 3 the
   name and its trailing flag are stored too, below that they are not, and the
   two paths converge on the same eight common  Every read is chained
   through the same conjunction, so the first failure abandons the rest and the
   record is reported bad; the timer is rearmed either way, and the armed bit is
   always cleared on the way out so a loaded generator starts disarmed. */
// FUNCTION: WIZ8 0x0048b5e0
unsigned char W8MonsterGenerator::Load(int handle)
{
    unsigned char version;
    unsigned char ok;
    unsigned char loaded;

    ok = ReadVirtualFile(handle, &version, 1, 0);
    if (static_cast<signed char>(version) >= 3) {
        ok = ok && ReadVirtualFile(handle, name, 0x20, 0);
        ok = ok && ReadVirtualFile(handle, &flag_44, 1, 0);
    }
    loaded = ok && ReadVirtualFile(handle, &flags, 4, 0) &&
             ReadVirtualFile(handle, &flag_04, 1, 0) &&
             ReadVirtualFile(handle, &value_06, 2, 0) &&
             ReadVirtualFile(handle, &value_08, 2, 0) &&
             ReadVirtualFile(handle, &state_0c, 4, 0) &&
             ReadVirtualFile(handle, &state_10, 4, 0) &&
             ReadVirtualFile(handle, &state_14, 4, 0) &&
             ReadVirtualFile(handle, &value_1c, 4, 0);
    Reset();
    if (static_cast<signed char>(version) > 1) {
        Function43A690(handle);
        Function43A530();
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
    DestroyMonsterGeneratorInline(removed);
}

/* Rearms the generator's timer, creating it on first use. The delay is the
   configured interval jittered by a uniform draw over twice its jitter
   fraction, so the mean is the interval itself; a generator flagged at bit 3
   uses the shared default interval instead of its own.
 
   Allocating the timer is what gives this body its unwind frame, and the
   assertion that guards it is where m_pTimer, MonGen and Reset all come from. */
// FUNCTION: WIZ8 0x0048b420
void W8MonsterGenerator::Reset()
{
    short interval;
    float jitter;

    if (m_pTimer == 0) {
        m_pTimer = Function43A4E0();
        if (m_pTimer == 0) {
            srAssertFail(
                "m_pTimer",
                MON_GEN_CPP,
                0x217,
                "MonGen::Reset() out of memory allocating m_pTimer");
        }
        *reinterpret_cast<unsigned short*>(
            reinterpret_cast<unsigned char*>(m_pTimer) + 8) &= 0xfffd;
    }
    interval = (flags & W8_MONGEN_USE_DEFAULT_INTERVAL) != 0
                   ? value_06
                   : g_generator_default_interval;
    jitter = interval * g_generator_jitter_fraction;
    Function439B80(
        static_cast<float>(Random(static_cast<int>(jitter) * 2 + 1)) + interval - jitter);
    Function43A530();
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
        marker->Function49F720(
            reinterpret_cast<const srVector3T<float>*>(&generator->state_0c));
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
    node_18->Function49F900(g_world);
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

/* Moves the generator. The scene is only told when the generator is armed and
   therefore actually has something placed. */
// FUNCTION: WIZ8 0x0048b730
void W8MonsterGenerator::SetState(const srVector3T<float>* state)
{
    state_0c = *reinterpret_cast<const int*>(&state->x);
    state_10 = *reinterpret_cast<const int*>(&state->y);
    state_14 = *reinterpret_cast<const int*>(&state->z);
    if (node_18 != 0) {
        node_18->Function49F720(state);
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
    node_18->Function49F900(g_world);
}
