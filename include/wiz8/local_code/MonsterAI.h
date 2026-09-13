#pragma once

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8MonsterGroup;

/* The timed sight service: clears the sight-dirty flag, refreshes what
   monsters can see, and reruns group detection in combat when allowed. */
void UpdateMonsterSight(void); /* 0x00530110 */
/* The per-frame monster group pass; `staggered` spreads the maintenance a
   fifth of the list at a time with a full-distance pass every 20 frames. */
void UpdateMonsterGroups(char staggered); /* 0x00530150 */
/* The group's party-sight aggregate DoMonsterRTAI reads as its alert level. */
unsigned char GetMonsterGroupPartySightState(W8MonsterGroup* monster_group); /* 0x00530470 */
/* The real-time AI decision pass; `engage` calls may trigger the ambush and
   group-alert paths a routine tick cannot. The original name is proven by the
   "DoMonsterRTAI: ERROR - Invalid disposition" assertion. */
void DoMonsterRTAI(W8MonsterInfo* monster_info, char engage); /* 0x00530560 */
/* The orders-driven half of DoMonsterRTAI: investigate a fresh heard noise or
   let the monster's scripted order_mode_28e pick the next mode. `decision`
   receives the mode; nonzero return means applying it is worthwhile. */
char ChooseMonsterRTAIMode(W8MonsterInfo* monster_info, unsigned char* decision);   /* 0x005308C0 */
void ApplyMonsterRTAIDecision(W8MonsterInfo* monster_info, unsigned char decision); /* 0x00530F10 */
/* 0x00617AE8: 125000, the cap on how far a monster will walk to investigate a
   heard noise. */
extern const int g_int_00617ae8;
/* Whether a hostile group still has a member able to engage the party: a
   visible target to advance on, a usable attack or spell, a way to flee, or
   a short-range attack with a path to the party. */
unsigned char MonsterGroupCanEngage(W8MonsterGroup* monster_group); /* 0x00531920 */
/* Whether the monster has a living target it can see; `party_only` skips the
   monster scan, `hostility` selects the class (three and four are wildcards),
   and `within_reach` also requires the target inside engagement range. */
unsigned char MonsterHasVisibleTarget(W8MonsterInfo* monster_info, int party_only, int hostility,
                                      int within_reach);              /* 0x00534850 */
float GetGroupNearestDistance(W8MonsterGroup* group, float furthest); /* 0x005324B0 */

/* MonsterAI.cpp GLOBAL at 0x0061EEFC: two dwords per AI kind. */
extern const int g_ai_kind_table[32][2];

unsigned char AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id);
unsigned char Function5327E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5330E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* combat_slot);
unsigned char Function5353E0(W8MonsterInfo* monster_info, int spell_id, W8CombatSlot* slot);
void Function5354E0(void);
unsigned char IsMonsterActionUsable(W8MonsterInfo* monster_info); /* 0x00535150 */
/* Decide what one monster does this round: give up, hold, flee, cast, attack
   or move, then fall back to holding when the choice cannot be carried out. */
void UpdateMonsterAI(W8MonsterInfo* monster_info); /* 0x00531540 */
/* The percentage chance the monster advances on the party this round. */
unsigned int MonsterAdvanceChance(W8MonsterInfo* monster_info,
                                  W8MonsterRecord* record); /* 0x00531C00 */
/* Whether the monster can flee at all: it has a flee chance, a flee
   animation, enough of its stat left, and somewhere to run. */
unsigned char CanMonsterFlee(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                             char exclude_special); /* 0x00534A40 */
/* Whether the monster may cast `spell_id` now; `needs_target` also demands
   something to aim it at. */
unsigned char IsSpellUsableByMonster(W8MonsterInfo* monster_info, int spell_id,
                                     char needs_target); /* 0x00532550 */
/* Which of the monster's ten spells to cast, weighted by the spell table. */
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x00533260 */
/* Refill the monster's pending-action queue with everything it may do this
   round: cooperative and special actions first, then one attack entry per
   attack-mode bit for every target the attack can reach. `target_locked`
   restricts the sweep to the stored target, `attack_locked` to the committed
   attack index. */
void BuildMonsterActionQueue(W8MonsterInfo* monster_info, char target_locked,
                             char attack_locked); /* 0x00531CE0 */
/* Build the monster's list of possible actions and take one of them at
   random into its action fields and target. */
unsigned char ChooseRandomMonsterAction(W8MonsterInfo* monster_info, int arg_2, int arg_3,
                                        char set_attack_rate); /* 0x005323F0 */
