#pragma once
#include "wiz8/conditions.h"

struct W8ItemInstance;
struct W8TargetSource;
struct W8Character;
struct W8MonsterInfo;

void CopyCharacterConditionsToTarget(const W8Character* character, const int* target);
void CopyMonsterConditionsToCharacter(W8Character* character, const W8MonsterInfo* monster_info);


#pragma pack(push, 2)
struct W8ConditionImmunity {
    unsigned char kind;
    /* 0x01: the catch-all flag the monster-info immunity list prints as its
       final "all" entry. */
    unsigned char immune_all;
    W8Condition conditions[20];
};
#pragma pack(pop)
static_assert(sizeof(W8ConditionImmunity) == 0x52, "W8ConditionImmunity_size");

extern W8ConditionImmunity g_condition_immunities[3];
extern unsigned short g_condition_notices[128];

void RemoveCharacterCondition(int party_slot, W8Condition condition, bool announce);
void SetMonsterCondition(int location_id, int condition, int duration, int argument,
                         W8TargetSource* target, char announce);
void ClearMonsterCondition(int location_id, W8Condition condition);
void ClearMonsterEnchantmentSlot(int location_id, W8EnchantmentSlot slot);
void ClearCharacterEnchantmentSlot(int party_slot, W8EnchantmentSlot slot);
/* 0x00523B30/0x00524400: run one enchantment slot down by some turns,
   emptying it when nothing is left. */
void TickCharacterEnchantmentSlot(int party_slot, W8EnchantmentSlot slot, unsigned int turns);
void TickMonsterEnchantmentSlot(int location_id, W8EnchantmentSlot slot, unsigned int turns);
/* 0x00524110: the per-condition aging tick the sight producer runs while a
   condition's countdown is live. */
void TickMonsterCondition(int location_id, W8Condition condition, unsigned int minutes);
/* 0x005236A0: the character-side counterpart of TickMonsterCondition. */
void TickCharacterCondition(unsigned int party_slot, W8Condition condition, unsigned int minutes);
/* 0x00523940/0x005242B0: settle an enchantment on a character or monster with
   its argument, rolled duration and definition percentage. */
void ApplyCharacterCondition(int party_slot, W8EnchantmentSlot slot, int argument,
                             unsigned int duration, unsigned int percent);
void ApplyMonsterCondition(int location_id, W8EnchantmentSlot slot, int argument,
                           unsigned int duration, unsigned int percent);
unsigned char GetConditionRecordFlag(int party_slot, int condition);
/* 0x00524780: record a bound monster in a party member's condition record -
   the level the binding was made on and the monster's location id, with the
   record's flag byte raised. Slot one also retires the monster's group and
   its allies. */
void RemoveAllConditionsFromParty(void);
/* 0x00524780: bind a monster into one of a character's two dependence
   condition records; a slot-one binding also retires the monster's group. */
void BindMonsterToCharacterDependence(unsigned int party_slot, unsigned int dependence_slot,
                                      int monster_id);

/* 0x005237E0: rescan uiCondition from slot 0x13 downward and write the
   first live index into W8Character::highest_condition. */
void RecomputeCharacterHighestCondition(int party_slot);
/* 0x005248D0: clear the character condition records a dead monster sourced,
   over both dependence slots and the per-character tables; RecordMonsterKill
   runs it before the faction fallout. Its own TU sits in the gap after
   Conditions & Enchantments.cpp. */
void ReleaseMonsterConditionBindings(W8MonsterInfo* monster_info);

unsigned char SetCharacterCondition(int party_slot, W8Condition condition, int duration,
                                    int argument, char value_5, char value_6);

void RemoveConditionFromEveryone(W8Condition condition); /* 0x005244A0 */
void RemoveConditionFromParty(W8Condition condition);    /* 0x005246C0 */
void RemoveAllEnchantments(void);                /* 0x00524540 */

void NormalizeItemQuantityKind(W8ItemInstance* item);
/* 0x00522EF0: the post-load repair LoadGame runs - unequip unusable items on
   every character and normalize quantity kinds on carried and pooled items. */
void SanitizeLoadedItems(void);
