#pragma once

struct W8ItemInstance;
struct W8TargetSource;
struct W8Character;
struct W8MonsterInfo;

void CopyCharacterConditionsToTarget(const W8Character* character, const int* target);
void CopyMonsterConditionsToCharacter(W8Character* character, const W8MonsterInfo* monster_info);

#pragma pack(push, 2)
struct W8ConditionImmunity {
    unsigned char kind;
    unsigned char unknown_01;
    int conditions[20];
};
#pragma pack(pop)

extern W8ConditionImmunity g_condition_immunities_006171A8[3];
extern unsigned short g_condition_notices_0061E570[128];

void RemoveCharacterCondition(int party_slot, int condition, int announce);
void SetMonsterCondition(int location_id, int condition, int duration, int argument,
                         W8TargetSource* target, char announce);
void ClearMonsterCondition(int location_id, int condition);
void ClearMonsterEnchantmentSlot(int location_id, int slot);
void ClearCharacterEnchantmentSlot(int party_slot, int slot);
/* 0x00523B30/0x00524400: run one enchantment slot down by some turns,
   emptying it when nothing is left. */
void TickCharacterEnchantmentSlot(int party_slot, int slot, unsigned int turns);
void TickMonsterEnchantmentSlot(int location_id, int slot, unsigned int turns);
/* 0x00524110: the per-condition aging tick the sight producer runs while a
   condition's countdown is live. */
void TickMonsterCondition(int location_id, int condition, unsigned int minutes);
unsigned char GetConditionRecordFlag(int party_slot, int condition);
void RemoveAllConditionsFromParty(void);

void ApplyMonsterCondition(int location_id, int condition, int arg_3);
/* 0x005237E0: rescan condition_turns from slot 0x13 downward and write the
   first live index into W8Character::highest_condition. */
void RecomputeCharacterHighestCondition(int party_slot);

unsigned char SetCharacterCondition(int party_slot, int condition, int duration, int argument,
                                    char value_5, char value_6);

void RemoveConditionFromEveryone(int condition); /* 0x005244A0 */
void RemoveConditionFromParty(int condition);    /* 0x005246C0 */
void RemoveAllEnchantments(void);                /* 0x00524540 */

void NormalizeItemQuantityKind(W8ItemInstance* item);
