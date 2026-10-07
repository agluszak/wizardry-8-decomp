#pragma once

#include "wiz8/layouts/gameplay_databases.h"

#include <wchar.h>

#include "surrender/srMath.h"
#include "wiz8/layouts/targeting.h"

class W8Monster;
struct W8Character;
struct W8CombatSlot;
struct W8Dice;
struct W8MonsterInfo;
struct W8SpellEffectResult;

W8Character* FindPartyMemberWithLowestResistance4(void);
unsigned int FindPartySlotWithLowestHitPoints(void);
unsigned int FindPartySlotWithLowestSpellPoints(void);

void HealCharacter(int party_slot, int amount, bool announce);
void RestoreCharacterStamina(int party_slot, int amount, bool announce);
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, bool announce);
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount);
void RestoreCharacterRealmSpellPoints(int party_slot, W8SpellRealm realm, int amount);
void FatigueCharacter(int party_slot, int amount, bool scale_by_load,
                      W8SpellEffectResult* report_to);
/* Run one queued fatigue op. The op is a combat slot: a
   character target fatigues that party slot, a monster target resolves the
   monster from its location id. The third argument is unused. */
void ApplyQueuedFatigue(W8CombatSlot* op, unsigned int amount, int);
unsigned int CharacterActionFatigueCost(int party_slot, W8ActionKind action_kind);
void DamageCharacter(int party_slot, unsigned int damage, bool announce);
void DrainCharacterRealmSpellPoints(int party_slot, W8SpellRealm realm, unsigned int amount,
                                    bool announce);
void DrainPartySpellPoints(int amount, bool announce);
int CalculateMonsterFatigueBand(int current, int maximum);
unsigned int FatigueArmorPenalty(int fatigue_band);
int SpellCastFatigueCost(int spell_id, int result);
void SpendCharacterSpellPoints(int party_slot, W8SpellRealm realm, int amount);
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info);
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount,
                    W8SpellEffectResult* report_to);
void HealMonster(W8MonsterInfo* monster_info, unsigned int amount, bool announce);
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, bool announce);
/* The monster-side effect application pass the aging producer
   drives for both sign directions. The result block, when given, collects
   the damage dealt; the applied amount is also returned. `quiet` marks
   non-provoking damage such as a poison tick: it picks the notice strings,
   skips the condition-target bookkeeping, and silences the struck reaction. */
unsigned int ApplyDamageToMonster(W8MonsterInfo* monster_info, unsigned int amount,
                                  struct W8TargetSource* source, bool quiet,
                                  unsigned char in_combat, char a,
                                  W8SpellEffectResult* result_stats, bool c);
/* The character-side counterpart - damage absorbed by the
   slot-2 enchantment first, two thirds of the rest fatigue the character, the
   remainder comes off hit points and can kill. The sixth parameter receives a
   kill-counting result block.
   `quiet` marks non-provoking damage such as a poison tick: it picks the
   shield/notice strings and the poison suffix, and keeps a sleeping character
   asleep. `announce` gates the damage notice, `short_notice` selects the terse
   ShowNoticef form over the verbose PostCharacterNotice one, and `detailed`
   selects the named FormatWideString form. */
unsigned int ApplyDamageToCharacter(int party_slot, unsigned int amount, bool quiet, bool announce,
                                    bool short_notice, W8SpellEffectResult* result_stats,
                                    bool detailed);
extern wchar_t g_poison_suffix[];
/* How a monster answers being struck - the struck cycle, a
   possible condition knock-on, and the hostility check toward the attacker. */
void MonsterReactsToBeingStruck(W8MonsterInfo* monster_info, W8TargetSource* attacker, bool quiet);
void CharacterDies(int party_slot);
void ApplyRolledHealthChangeToParty(const W8Dice* dice, W8SpellEffectResult* result, int announce);
/* Roll the dice once for every live monster within the radius of
   a point and apply each roll as damage. */
void DamageMonstersInRadius(const srVector3T<float>& center, float radius, const W8Dice* dice,
                            struct W8TargetSource* source, W8SpellEffectResult* result);
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base);
void HealPartyByDice(unsigned char count, unsigned char sides, short base);
void RestorePartySpellPoints(int amount);
void RecalculateCharacterHitPoints(W8Character* character);
int __cdecl CompareSpellPointDeficits(const void* first, const void* second);
int SumCharacterSpellPoints(const W8Character* character);
int SumCharacterSpellPointsLeft(const W8Character* character);
int GetCharacterRealmSpellPoints(const W8Character* character, W8SpellRealm realm);
void RecalculateCharacterStamina(W8Character* character);
void RecalculateRealmSpellPoints(W8Character* character);
int RebuildRealmSpellPointCeilings(W8Character* character);
