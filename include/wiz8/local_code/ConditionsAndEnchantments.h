#pragma once

struct W8ItemInstance;
struct W8TargetSource;
struct W8Character;
struct W8MonsterInfo;

void CopyCharacterConditionsToTarget(const W8Character* character, const int* target);
void CopyMonsterConditionsToCharacter(W8Character* character, const W8MonsterInfo* monster_info);

/* Semantic names for the condition slots that were still represented by
   behavior-only aliases or raw indices in layouts/character.h. The ordering is
   fixed independently by the retail condition icon catalog and the spell/
   condition name table: both contain ... Webbed, Asleep, Paralyzed,
   Unconscious, Dead ... in slot order. The monster visual path additionally
   maps condition N to icon N-1 for slots 1..18. Poisoned (7), Asleep (15),
   Dead (18), the count and duration sentinel remain declared in
   layouts/character.h. */
enum {
    W8_CONDITION_DRAINED = 1,
    W8_CONDITION_DISEASED = 2,
    W8_CONDITION_IRRITATED = 3,
    W8_CONDITION_NAUSEATED = 4,
    W8_CONDITION_SLOWED = 5,
    W8_CONDITION_AFRAID = 6,
    W8_CONDITION_SILENCED = 8,
    W8_CONDITION_HEXED = 9,
    W8_CONDITION_INFATUATED = 10,
    W8_CONDITION_INSANE = 11,
    W8_CONDITION_BLIND = 12,
    W8_CONDITION_TURNCOAT = 13,
    W8_CONDITION_WEBBED = 14,
    W8_CONDITION_PARALYZED = 16,
    W8_CONDITION_UNCONSCIOUS = 17,
    W8_CONDITION_MISSING = 19
};

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
/* 0x005236A0: one condition's share of the aging tick; POISONED also drains
   its strength argument pro rata over the remaining duration. */
void TickCharacterCondition(unsigned int party_slot, unsigned int condition,
                            unsigned int minutes);
/* 0x00523B30/0x00524400: run one enchantment slot down by some turns,
   emptying it when nothing is left. */
void TickCharacterEnchantmentSlot(int party_slot, int slot, unsigned int turns);
void TickMonsterEnchantmentSlot(int location_id, int slot, unsigned int turns);
/* 0x00524110: the per-condition aging tick the sight producer runs while a
   condition's countdown is live. */
void TickMonsterCondition(int location_id, int condition, unsigned int minutes);
unsigned char GetConditionRecordFlag(int party_slot, int condition);
/* 0x005248D0: the dying monster's side of the character binding records; for
   each slot kind its binding mask still names, drop every party member's
   record pointing back to the monster and lift the linked condition. */
void ReleaseMonsterConditionBindings(W8MonsterInfo* monster_info);
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
