#pragma once

struct W8Character;
struct W8Dice;
struct W8MonsterInfo;
struct W8Character;

W8Character* FindPartyMemberWithLowestResistance4(void);

void HealCharacter(int party_slot, int amount, char announce);
void RestoreCharacterStamina(int party_slot, int amount, char announce);
void DrainCharacterSpellPoints(int party_slot, unsigned int amount, char announce);
void RestoreCharacterSpellPointsEvenly(int party_slot, int amount);
void FatigueCharacter(int party_slot, int amount, char scale_by_load, int load_percent,
                      int report_to);
unsigned int FatigueArmorPenalty(int fatigue_band);
unsigned int SpellCastFatigueCost(int spell_id, int result);
int MonsterActionFatigueCost(const W8MonsterInfo* monster_info);
void FatigueMonster(W8MonsterInfo* monster_info, unsigned int amount, int report_to);
void HealMonster(W8MonsterInfo* monster_info, int amount, char announce);
void RestoreMonsterStamina(W8MonsterInfo* monster_info, int amount, char announce);
/* 0x0052BB60: the monster-side effect application pass the aging producer
   drives for both sign directions. */
void Function52BB60(
    W8MonsterInfo* monster_info, unsigned int amount, struct W8TargetSource* source,
    int enabled, unsigned char in_combat, int a, int b, int c);
void CharacterDies(int party_slot);
void ApplyRolledHealthChangeToParty(const W8Dice* dice, int arg_2, int arg_3);
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base);
void HealPartyByDice(unsigned char count, unsigned char sides, short base);
void RestorePartySpellPoints(int amount);
void RecalculateCharacterHitPoints(W8Character* character);
void StartBreathCycle(int party_slot, int arg_2);                 /* 0x0052FE80 */

void ApplyHealthChangeToCharacter(
    int party_slot, int amount, int arg_3, int arg_4, int arg_5, int arg_6, int arg_7);

