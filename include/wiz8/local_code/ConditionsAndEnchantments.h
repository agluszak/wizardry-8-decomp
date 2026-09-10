#pragma once

struct W8TargetSource;

void RemoveCharacterCondition(int party_slot, int condition, int announce);
void SetMonsterCondition(
    int location_id, int condition, int duration, int argument,
    W8TargetSource* target, int quiet);
void ClearMonsterCondition(int location_id, int condition);
void ClearMonsterEnchantmentSlot(int location_id, int slot);
/* 0x00524110: the per-condition aging tick the sight producer runs while a
   condition's countdown is live. */
void Function524110(int location_id, int condition, unsigned int minutes);
unsigned char Function5248A0(int party_slot, int condition);
void RemoveAllConditionsFromParty(void);

extern unsigned char g_flag_00683F94;
extern unsigned char g_byte_00687500;
extern unsigned char g_enchantment_six_cleared_006840bb;

void ApplyMonsterCondition(int location_id, int condition, int arg_3);
void Function5237E0(int party_slot);

