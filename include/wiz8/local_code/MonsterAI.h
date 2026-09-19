#pragma once

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8MonsterGroup;
template <class T> class W8GrowableVector;

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
bool MonsterGroupCanEngage(W8MonsterGroup* monster_group); /* 0x00531920 */
/* Whether the monster has a living target it can see; `party_only` skips the
   monster scan, `hostility` selects the class (three and four are wildcards),
   and `within_reach` also requires the target inside engagement range. */
bool MonsterHasVisibleTarget(W8MonsterInfo* monster_info, int party_only, int hostility,
                             int within_reach);       /* 0x00534850 */
float GetGroupNearestDistance(W8MonsterGroup* group); /* 0x005324B0 */

/* MonsterAI.cpp GLOBAL at 0x0061EEFC: two dwords per special attack kind. */
extern const int g_special_attack_table[32][2];

bool AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id); /* 0x005326F0 */
/* Whether the combat slot accepts `spell_id` from this caster; the original
   name is proven by the "WARNING: MonsterSpellTargetOK" debug message. */
bool MonsterSpellTargetOK(W8MonsterInfo* monster_info, int spell_id,
                          W8CombatSlot* combat_slot); /* 0x005327E0 */
/* Whether the spell's area effect would catch a disposition-neutral monster,
   which vetoes it - the AI does not turn neutrals hostile by accident. */
unsigned char SpellAreaHitsNeutralMonster(W8MonsterInfo* monster_info, int spell_id,
                                          W8CombatSlot* combat_slot); /* 0x005330E0 */
/* Whether the spell's markers catch at least one party member when cast at
   `slot`. */
unsigned char MonsterSpellHasPartyTarget(W8MonsterInfo* monster_info, int spell_id,
                                         W8CombatSlot* slot); /* 0x005353E0 */
/* Whether any live member of the group has a visible target; the arguments
   forward to MonsterHasVisibleTarget. */
bool MonsterGroupHasVisibleTarget(W8MonsterGroup* monster_group, int party_only, int hostility,
                                  int within_reach); /* 0x005347A0 */
/* The out-of-combat sweep: refreshes sight, alerts same-faction groups of
   groups already fighting, and enters combat for the groups that should. */
void CheckMonsterGroupsEnterCombat(void); /* 0x005354E0 */
/* The in-combat sweep: drops combat for groups with nothing left to fight,
   and for every group at once once nobody is still engaged. */
void CheckMonsterGroupsLeaveCombat(void); /* 0x00534300 */
/* Whether a group's members still count as fighting; propagates the answer
   to the group and its allies through the engagement-state pair. */
void UpdateMonsterGroupEngagement(void);                 /* 0x00535200 */
bool IsMonsterActionUsable(W8MonsterInfo* monster_info); /* 0x00535150 */
/* Decide what one monster does this round: give up, hold, flee, cast, attack
   or move, then fall back to holding when the choice cannot be carried out. */
void UpdateMonsterAI(W8MonsterInfo* monster_info); /* 0x00531540 */
/* The percentage chance the monster advances on the party this round. */
unsigned int MonsterAdvanceChance(W8MonsterInfo* monster_info,
                                  W8MonsterRecord* record); /* 0x00531C00 */
/* Whether the monster can flee at all: it has a flee chance, a flee
   animation, enough of its stat left, and somewhere to run. */
bool CanMonsterFlee(W8MonsterInfo* monster_info, W8MonsterRecord* record,
                    char exclude_special); /* 0x00534A40 */
/* Pick the direction a fleeing monster runs; returns whether a heading was
   found. */
unsigned char AimFleeingMonster(W8MonsterInfo* monster_info,
                                const W8MonsterRecord* record); /* 0x00534CB0 */
/* Whether the monster may cast `spell_id` now; `needs_target` also demands
   something to aim it at. */
bool IsSpellUsableByMonster(W8MonsterInfo* monster_info, int spell_id,
                            char needs_target); /* 0x00532550 */
/* 0x00534290: whether a monster can aim the spell it wants to cast - area
   target types aim at the world, the rest pass the slot check. */
bool CanMonsterAimSpell(W8MonsterInfo* monster_info, int spell_id);
/* 0x00534D50: whether the control spell's visual anchor sits in range of the
   monster, falling back on where the party stands. */
short IsMonsterControlPointInRange(W8MonsterInfo* monster_info);
/* Which of the monster's ten spells to cast, weighted by the spell table. */
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x00533260 */
/* Whether the monster sees no living enemy: a set threat flag answers at
   once, a hostile party member in play counts, and `party_only` zero also
   scans the monsters it is hostile to that it can see. */
unsigned char MonsterHasNoVisibleEnemy(W8MonsterInfo* monster_info,
                                       int party_only); /* 0x00534690 */
/* Fill `targets` with the combat slots `spell_id` may be cast at by this
   monster; the monster's action fields carry the spell while the probe runs
   and are restored after. */
void CollectMonsterSpellTargets(W8MonsterInfo* monster_info, int spell_id,
                                W8GrowableVector<W8CombatSlot>* targets); /* 0x00533320 */
/* Whether at least half of the group's live in-combat members the caster is
   hostile to accept the spell. */
unsigned char MonsterGroupHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id,
                                                W8MonsterGroup* target_group); /* 0x00534DD0 */
/* Whether at least half of the party members the caster is hostile to accept
   the spell. */
unsigned char PartyHalfSpellTargetsValid(W8MonsterInfo* monster_info,
                                         int spell_id); /* 0x00534EF0 */
/* Whether a live hostile monster already fighting stands nearer to one of the
   group's members than to the party - reinforcement keeps the group in or
   brings it into combat. */
bool MonsterGroupHasReinforcement(W8MonsterGroup* monster_group); /* 0x00534FC0 */
/* Whether a group outside combat should join it: neutral groups need a
   reinforcement, hostile ones a member with a visible target inside the
   leader's reach or a rendered member near the party. */
bool ShouldMonsterGroupEnterCombat(W8MonsterGroup* monster_group); /* 0x005355D0 */
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
void UpdateAllMonsterAI(void);                                 /* 0x005314F0 */
