#pragma once

struct W8MonsterInfo;
struct W8MonsterRecord;
struct W8CombatSlot;
struct W8MonsterGroup;
template <class T> class W8GrowableVector;

void UpdateMonsterSight(void); /* 0x00530110 */
void UpdateMonsterGroups(char staggered); /* 0x00530150 */
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
extern int g_int_00617ae8;
bool MonsterGroupCanEngage(W8MonsterGroup* monster_group); /* 0x00531920 */
/* Whether the monster has a living target it can see; `party_only` skips the
   monster scan, `hostility` selects the class (three and four are wildcards),
   and `within_reach` also requires the target inside engagement range. */
bool MonsterHasVisibleTarget(W8MonsterInfo* monster_info, int party_only, int hostility,
                             int within_reach);       /* 0x00534850 */
float GetGroupNearestDistance(W8MonsterGroup* group); /* 0x005324B0 */

/* MonsterAI.cpp GLOBAL at 0x0061EEFC: two dwords per special attack kind. */
extern int g_special_attack_table[32][2];

bool AimMonsterAtSpellTarget(W8MonsterInfo* monster_info, int spell_id); /* 0x005326F0 */
/* Whether the combat slot accepts `spell_id` from this caster; the original
   name is proven by the "WARNING: MonsterSpellTargetOK" debug message. */
bool MonsterSpellTargetOK(W8MonsterInfo* monster_info, int spell_id,
                          W8CombatSlot* combat_slot); /* 0x005327E0 */
unsigned char SpellAreaHitsNeutralMonster(W8MonsterInfo* monster_info, int spell_id,
                                          W8CombatSlot* combat_slot); /* 0x005330E0 */
bool MonsterSpellHasPartyTarget(W8MonsterInfo* monster_info, int spell_id,
                                W8CombatSlot* slot); /* 0x005353E0 */
bool MonsterGroupHasVisibleTarget(W8MonsterGroup* monster_group, int party_only, int hostility,
                                  int within_reach); /* 0x005347A0 */
void CheckMonsterGroupsEnterCombat(void); /* 0x005354E0 */
void CheckMonsterGroupsLeaveCombat(void); /* 0x00534300 */
/* Whether a group's members still count as fighting; propagates the answer
   to the group and its allies through the engagement-state pair. */
void UpdateMonsterGroupEngagement(void);                 /* 0x00535200 */
bool IsMonsterActionUsable(W8MonsterInfo* monster_info); /* 0x00535150 */
void UpdateMonsterAI(W8MonsterInfo* monster_info); /* 0x00531540 */
unsigned int MonsterAdvanceChance(W8MonsterInfo* monster_info,
                                  W8MonsterRecord* record); /* 0x00531C00 */
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
int ChooseMonsterSpell(W8MonsterInfo* monster_info, W8MonsterRecord* record); /* 0x00533260 */
/* Whether the monster sees no living enemy: a set threat flag answers at
   once, a hostile party member in play counts, and `party_only` zero also
   scans the monsters it is hostile to that it can see. */
bool MonsterHasNoVisibleEnemy(W8MonsterInfo* monster_info, int party_only); /* 0x00534690 */
/* Fill `targets` with the combat slots `spell_id` may be cast at by this
   monster; the monster's action fields carry the spell while the probe runs
   and are restored after. */
void CollectMonsterSpellTargets(W8MonsterInfo* monster_info, int spell_id,
                                W8GrowableVector<W8CombatSlot>* targets); /* 0x00533320 */
bool MonsterGroupHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id,
                                       W8MonsterGroup* target_group); /* 0x00534DD0 */
bool PartyHalfSpellTargetsValid(W8MonsterInfo* monster_info, int spell_id); /* 0x00534EF0 */
bool MonsterGroupHasReinforcement(W8MonsterGroup* monster_group); /* 0x00534FC0 */
bool ShouldMonsterGroupEnterCombat(W8MonsterGroup* monster_group); /* 0x005355D0 */
void BuildMonsterActionQueue(W8MonsterInfo* monster_info, char target_locked,
                             char attack_locked); /* 0x00531CE0 */
unsigned char ChooseRandomMonsterAction(W8MonsterInfo* monster_info, int arg_2, int arg_3,
                                        char set_attack_rate); /* 0x005323F0 */
void UpdateAllMonsterAI(void);                                 /* 0x005314F0 */
