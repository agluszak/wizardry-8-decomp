#include "wiz8/gameplay_boundaries.h"
#include <math.h>

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
