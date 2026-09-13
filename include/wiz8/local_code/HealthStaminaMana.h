#pragma once

#include <wchar.h>

#include "surrender/srMath.h"

struct W8Character;
struct W8Dice;
struct W8MonsterInfo;
struct W8SpellEffectResult;

W8Character* FindPartyMemberWithLowestResistance4(void);

void HealCharacter(int party_slot, int amount, char announce);
void RestoreCharacterStamina(int party_slot, int amount, char announce);
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce);
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount);
void FatigueCharacter(int party_slot, int amount, char scale_by_load,
                      W8SpellEffectResult* report_to);
void DamageCharacter(int party_slot, int damage, char announce); /* 0x0052B7E0 */
void DrainCharacterRealmSpellPoints(int party_slot, int realm, unsigned int amount,
                                    char announce); /* 0x0052B6D0 */
unsigned int FatigueArmorPenalty(int fatigue_band);
unsigned int SpellCastFatigueCost(int spell_id, int result);
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info);
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount, int report_to);
void HealMonster(W8MonsterInfo* monster_info, int amount, char announce);
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, char announce);
/* 0x0052BB60: the monster-side effect application pass the aging producer
   drives for both sign directions. The result block, when given, collects
   the damage dealt. */
void ApplyDamageToMonster(W8MonsterInfo* monster_info, unsigned int amount,
                          struct W8TargetSource* source, int enabled, unsigned char in_combat,
                          int a, W8SpellEffectResult* result, int c);
/* 0x0052A890: the character-side counterpart - damage absorbed by the
   slot-2 enchantment first, two thirds of the rest fatigue the character, the
   remainder comes off hit points and can kill. Retail call sites pass exactly
   seven args; the sixth parameter receives a kill-counting result block. */
unsigned int ApplyDamageToCharacter(int party_slot, unsigned int amount, char arg_3, char arg_4,
                                    char arg_5, W8SpellEffectResult* result_stats, char arg_7);
void WriteGameLogAmount(int category, const wchar_t* format, unsigned int amount);
void RecordCharacterDamage(int party_slot, unsigned int amount);
void CharacterDies(int party_slot);
void ApplyRolledHealthChangeToParty(const W8Dice* dice, W8SpellEffectResult* result, int arg_3);
/* 0x0052BA80: roll the dice once for every live monster within the radius of
   a point and apply each roll as damage. */
void DamageMonstersInRadius(const srVector3T<float>& center, float radius, const W8Dice* dice,
                            struct W8TargetSource* source, W8SpellEffectResult* result);
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base);
void HealPartyByDice(unsigned char count, unsigned char sides, short base);
void RestorePartySpellPoints(int amount);
void RecalculateCharacterHitPoints(W8Character* character);
void StartBreathCycle(int party_slot, int arg_2); /* 0x0052FE80 */
