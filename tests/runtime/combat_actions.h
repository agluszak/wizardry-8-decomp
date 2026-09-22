#pragma once

#include "runtime_case.h"

/* Combat case operations. Every product inspection or mutation runs through
   the case's game-thread executor; the driver thread holds only copied
   values and injected OS input. An unresponsive executor fails the case at
   the step that stalled - no off-thread fallback. */

/* Everything a combat driver loop needs from live game state, filled
   entirely on the game thread. */
struct HostileEngagementSnapshot {
    int screen;
    int pending;
    unsigned int hostile_count;
    unsigned int active_monsters;
    unsigned int engaged_hostiles;
    unsigned int hostile_condition_monsters;
    float nearest_engaged_distance;
    unsigned int party_hp_total;
    unsigned int incapacitated_members;
    unsigned int provoked_active;
    int provoked_hp;
    int provoked_condition;
    int provoked_in_combat;
    float provoked_distance;
    unsigned int combat_mode;
    unsigned int round_active;
    int action_status;
    int action_monster;
    int action_char;
    int queued_attacks;
    int provoked_dead;
    unsigned int engaged_hp_total;
    unsigned int engaged_dead;
    int provoked_threat_state;
    int first_target_type;
    int first_target_monster;
    unsigned int report_count;
    unsigned int report_amount;
    unsigned int report_missed;
    unsigned int aim_active;
    int aim_hp;
    int aim_dead;
    int report_target_type;
    int report_target_monster;
};

struct HostileSnapshotQuery {
    int location_id;
    int aim_location_id;
    HostileEngagementSnapshot snapshot;
};

struct CombatAttackQuery {
    int location_id;
    int eligible;
    int queued;
    int aimed;
    int aim_location_id;
    int aim_hp;
};

struct CombatSpellQuery {
    int location_id;
    int spell_id;
    int queued;
    int aimed;
    int aim_location_id;
    int aim_hp;
};

struct CombatFleeQuery {
    int eligible;
    int queued;
};

/* Provoke an active monster group hostile beside the party and wait for
   combat mode. Returns the provoked monster's location_id (the stable actor
   ID assertions key on), or -1 with the case already failed. */
int EngageHostile(RuntimeCase& test);

/* Re-copy the engagement state for the query's provoked/aimed monster IDs.
   Fails the case on an unresponsive executor. */
bool SnapshotEngagement(RuntimeCase& test, HostileSnapshotQuery& query, const char* step);

bool WaitForCombatMode(RuntimeCase& test, bool enabled, unsigned long timeout_ms = 3000);

/* Toggle combat through the product's own dispatch, nudging the party
   forward when manual entry is still waiting on ground contact. */
bool RequestCombatMode(RuntimeCase& test, bool enabled, unsigned long timeout_ms);

/* Queue party actions on the game thread - the same ChooseAction/AimAtTarget
   calls the matching keyboard commands and combat UI dispatch. Each fills
   its out query for the case's assertion and trace lines. */
bool QueuePartyAttack(RuntimeCase& test, CombatAttackQuery& out, const char* step);
bool QueuePartySpell(RuntimeCase& test, CombatSpellQuery& out, const char* step);
int QueuePartyDefend(RuntimeCase& test); /* queued slot count, or -1 on executor failure */
int WeakenParty(RuntimeCase& test);      /* weakened slot count, or -1 on executor failure */
bool QueuePartyFlee(RuntimeCase& test, CombatFleeQuery& out);

/* Drop the party beside the nearest engaged monster when the standoff is
   outside melee reach. */
bool MovePartyNearTarget(RuntimeCase& test, float nearest_engaged_distance);

/* The injected START_COMBAT_ROUND input must be consumed into an active
   round, then the round must resolve - each wait bounded separately so a
   stall fails at the stage that stopped. */
bool StartCombatRound(RuntimeCase& test, const char* step);
bool WaitRoundActive(RuntimeCase& test, HostileSnapshotQuery& query, unsigned long budget_ms,
                     const char* step);
/* Polls until round_active clears or combat drops; the last snapshot stays
   in query so the caller can distinguish a resolved round from combat
   ending. observe, when given, runs against each snapshot and returns true
   to end the wait early (e.g. a latched durable-effect assertion).
   Returns false with the case failed on budget expiry or executor loss. */
bool WaitRoundFinished(RuntimeCase& test, HostileSnapshotQuery& query, unsigned long budget_ms,
                       const char* step,
                       bool (*observe)(const HostileEngagementSnapshot& state, void* ctx) = 0,
                       void* ctx = 0);

/* Durable-effect checks - "the aimed monster lost HP or died", "a monster
   actor executed", "a party member is down". Volatile status fields alone
   are never evidence. */
bool ExpectTargetDamaged(const HostileEngagementSnapshot& state, int aim_location_id,
                         int baseline_hp);
bool MonsterAttackExecuted(const HostileEngagementSnapshot& state);
bool PartyTookCasualty(const HostileEngagementSnapshot& state);

/* Case entry points for the scenario registry. */
bool CombatRoundtripCase(RuntimeCase& test);
bool HostileEncounterCase(RuntimeCase& test);
bool CombatAttackCase(RuntimeCase& test);
bool CombatSpellCase(RuntimeCase& test);
