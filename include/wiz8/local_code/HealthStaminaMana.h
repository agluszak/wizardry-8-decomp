#pragma once

struct W8Dice;
struct W8MonsterInfo;

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
void CharacterDies(int party_slot);
void ApplyRolledHealthChangeToParty(const W8Dice* dice, int arg_2, int arg_3);
void RestorePartyStaminaByDice(unsigned char count, unsigned char sides, short base);
void HealPartyByDice(unsigned char count, unsigned char sides, short base);
void RestorePartySpellPoints(int amount);
