#include "wiz8/gameplay_boundaries.h"
#include <math.h>
#include "wiz8/vector.h"
#include "wiz8/sr_api.h"

/* Declared rather than pulled in from the vendored SGP FileMan.h: that header
   is C and including it here re-declares the CRT wide-string overloads with C
   linkage. The signature is the header's, spelled out. */
extern "C" unsigned char FileWrite(int handle, void* source, unsigned int size,
                                  unsigned int* written);

#include <string.h>

// FUNCTION: WIZ8 0x0048BDC0
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

/* One 0x21-byte runtime row per level, at 0x00686B74. Only the timestamp the
   encounter budget measures elapsed time from is established. */
struct W8LevelRuntimeRow {
    unsigned char flag_00;                  /* 0x00 */
    unsigned char unknown_01[0xc];
    int last_budget_update;                 /* 0x0d */
    unsigned char unknown_11[0x10];
};                                          /* 0x21 */

extern W8LevelRuntimeRow g_level_runtime[]; /* 0x00686B74 */
extern int g_current_level;                 /* 0x00686A70 */
extern int g_game_time;                     /* 0x00686A48 */
extern int g_random_encounter_budget;
extern int g_random_encounter_limit;
extern int g_active_group_count;            /* 0x0065BA14 */
extern W8MonsterGroup** g_active_groups;    /* 0x0065BA1C */
extern void RollRandomEncounters(void);     /* 0x0048CA20 */
extern void Function49FA30(W8World* world);                  /* 0x0049FA30 */
extern int Function43A5D0(void);                             /* 0x0043A5D0 */
extern unsigned char Function48B200(int value);              /* 0x0048B200 */
extern void Function48B420(void);                            /* 0x0048B420 */
extern void GenerateEncounter(void* encounter_state);         /* 0x0048AD20 */
extern void DestroyEncounterTable(W8EncounterTableRuntime* table); /* 0x0048AC60 */
extern void Function43A770(int handle);                      /* 0x0043A770 */
extern unsigned char g_generator_save_flag;                  /* 0x0065BA48 */
extern W8MonsterGeneratorNode* Function43A4E0(void);         /* 0x0043A4E0 */
extern void Function439B80(float delay);                     /* 0x00439B80 */
extern void Function43A530(void);                            /* 0x0043A530 */
extern unsigned char Function49F4A0(void* context, const char* name,
                                    void* out, int value);   /* 0x0049F4A0 */
extern void Function49F720(void* state);                     /* 0x0049F720 */
extern void Function49F900(W8World* world);                  /* 0x0049F900 */
extern void Function49FAA0(void);                            /* 0x0049FAA0 */
extern short g_generator_default_interval;                   /* 0x0060A6B6 */
extern const float g_generator_jitter_fraction;              /* 0x005EC040 */

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
// FUNCTION: WIZ8 0x0048C810
void UpdateRandomEncounterBudget(unsigned char reset_budget)
{
    W8LevelDatabaseRecord* level;
    int elapsed;
    int index;

    if (reset_budget == 0) {
        elapsed = g_game_time - g_level_runtime[g_current_level].last_budget_update;
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

extern unsigned char Function504910(void);          /* 0x00504910 */
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
// FUNCTION: WIZ8 0x0048C8E0
void CullExpiredEncounters(void)
{
    srVector3T<float> party;
    srVector3T<float> position;
    float span;
    int index;

    GetPartyPosition(&party);
    if (Function504910() == 0) {
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
                       static_cast<unsigned int>(g_game_time - group->spawn_time))) {
            W8Monster* monster = GetMonsterByLocationID(group->value_9f);

            position = monster->member_18.GetPosition();
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
// FUNCTION: WIZ8 0x0048BD80
int GetMonsterGeneratorCount(void)
{
    return g_world->monster_generators->GetCount();
}

/* One of them by index. Out of range answers null, which is why the bounds test
   appears twice: once here, rejecting the index outright, and once inside the
   vector's own bounds-checked read, which would otherwise fall back to element
   zero. */
// FUNCTION: WIZ8 0x0048BD90
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
// FUNCTION: WIZ8 0x0048AD00
W8EncounterTableRuntime* GetEncounterTable(int index)
{
    if (index < static_cast<int>(g_encounter_table_count)) {
        return g_encounter_tables[index];
    }
    return 0;
}

/* Appends a generator to the world's list, growing the array by exactly one
   element when it is full - so filling the list is quadratic, which is what the
   image does. A failed allocation puts the old array back and drops the
   generator silently. The grow is written out rather than delegated because the
   original inlines it here. */
// FUNCTION: WIZ8 0x0048BE30
void AddMonsterGenerator(W8MonsterGenerator* generator)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;
    W8MonsterGenerator** previous;
    int wanted = generators->count + 1;
    int index;

    if (generators->capacity < wanted) {
        previous = generators->data;
        generators->data = static_cast<W8MonsterGenerator**>(
            ::operator new(wanted * sizeof(W8MonsterGenerator*)));
        if (generators->data == 0) {
            generators->data = previous;
            return;
        }
        generators->capacity = wanted;
        for (index = 0; index < generators->count; ++index) {
            generators->data[index] = previous[index];
        }
        ::operator delete(previous);
    }
    generators->data[generators->count] = generator;
    ++generators->count;
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
                Function49FA30(g_world);
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
// FUNCTION: WIZ8 0x0048BF70
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
// FUNCTION: WIZ8 0x0048C600
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
            Function48B420();
        }
    }
}

/* Releases every loaded encounter table and every table name, and marks no
   level loaded. Both walks re-read their base pointer and count from memory
   each iteration, because destroying a table can reallocate neither but the
   original reloads them anyway. */
// FUNCTION: WIZ8 0x0048A710
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
// FUNCTION: WIZ8 0x0048C3B0
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
// FUNCTION: WIZ8 0x0048B520
void SaveMonsterGenerator(W8MonsterGenerator* generator, int handle)
{
    unsigned char leading;

    FileWrite(handle, &leading, 1, 0);
    FileWrite(handle, generator->name, 0x20, 0);
    FileWrite(handle, &generator->flag_44, 1, 0);
    FileWrite(handle, &generator->flags, 4, 0);
    FileWrite(handle, &generator->flag_04, 1, 0);
    FileWrite(handle, &generator->value_06, 2, 0);
    FileWrite(handle, &generator->value_08, 2, 0);
    FileWrite(handle, &generator->state_0c, 4, 0);
    FileWrite(handle, &generator->state_10, 4, 0);
    FileWrite(handle, &generator->state_14, 4, 0);
    FileWrite(handle, &generator->value_1c, 4, 0);
    Function43A770(handle);
}

/* Removes one generator from the world's list by identity and destroys it. The
   search stops at the first match and the tail is shifted down over it; a
   generator that is not in the list is left alone entirely, so this is safe to
   call on one that has already been removed. */
// FUNCTION: WIZ8 0x0048BEB0
void RemoveMonsterGenerator(W8MonsterGenerator* generator)
{
    W8GrowableVector<W8MonsterGenerator*>* generators = g_world->monster_generators;
    W8MonsterGenerator** scan;
    W8MonsterGenerator* removed;
    int index = 0;

    if (generators->count <= 0) {
        return;
    }
    scan = generators->data;
    while (*scan != generator) {
        ++index;
        ++scan;
        if (index >= generators->count) {
            return;
        }
    }
    if (index == -1 || index >= generators->count) {
        return;
    }
    removed = *generators->GetAt(index);
    if (index < generators->count && index >= 0) {
        if (index < generators->count - 1) {
            do {
                generators->data[index] = generators->data[index + 1];
                ++index;
            } while (index < generators->count - 1);
        }
        --generators->count;
    }
    DestroyMonsterGeneratorInline(removed);
}

/* Rearms the generator's timer, creating it on first use. The delay is the
   configured interval jittered by a uniform draw over twice its jitter
   fraction, so the mean is the interval itself; a generator flagged at bit 3
   uses the shared default interval instead of its own.
 
   Allocating the timer is what gives this body its unwind frame, and the
   assertion that guards it is where m_pTimer, MonGen and Reset all come from. */
// FUNCTION: WIZ8 0x0048B420
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

/* Arms or disarms the generator. Arming loads its marker from
   Data\\Items3D\\Bitmaps the first time and hands the caller's node over;
   disarming drops the flag and notifies the world. Both directions are no-ops
   when the flag already reads as asked. */
// FUNCTION: WIZ8 0x0048B770
void W8MonsterGenerator::SetActive(unsigned char active, W8MonsterGeneratorNode* node)
{
    void* context[2];
    unsigned char loaded;

    if (((flags >> 2) & 1) == active) {
        return;
    }
    if (active == 0) {
        flags &= ~static_cast<unsigned int>(W8_MONGEN_ARMED);
        if (node_18 != 0) {
            Function49FA30(g_world);
        }
        return;
    }
    flags |= W8_MONGEN_ARMED;
    if (node_18 == 0) {
        loaded = 0;
        context[0] = g_world;
        context[1] = const_cast<char*>("Data\\Items3D\\Bitmaps");
        if (Function49F4A0(context, "mongen", &loaded, 0) == 0) {
            srAssertFail(
                "fSuccess", MON_GEN_CPP, 0x309, "Couldn't load mongen.itm");
        }
        node_18 = node;
        if (node != 0) {
            Function49F720(&state_0c);
            Function49FAA0();
        }
    }
    Function49F900(g_world);
}
