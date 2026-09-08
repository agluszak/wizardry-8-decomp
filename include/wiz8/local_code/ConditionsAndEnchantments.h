#pragma once

struct W8TargetSource;

void RemoveCharacterCondition(int party_slot, int condition, int announce);
void SetMonsterCondition(
    int location_id, int condition, int duration, int argument,
    W8TargetSource* target, int quiet);
void ClearMonsterCondition(int location_id, int condition);
unsigned char Function5248A0(int party_slot, int condition);
void RemoveAllConditionsFromParty(void);

extern unsigned char g_flag_00683F94;
extern unsigned char g_byte_00687500;
extern unsigned char g_enchantment_six_cleared_006840bb;
