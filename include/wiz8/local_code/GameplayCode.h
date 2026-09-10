#pragma once

struct W8Character;

void AdvanceCharacterToLevel(W8Character* character, unsigned int level);
bool AnyCharacterActive(void);
void CalcInitiative(W8Character* character);
void CalcAttacks(W8Character* character);
void CalcArmorClasses(W8Character* character);
void CalcCharacterLevelBand(W8Character* character);

void CalcCharacterTableValue(W8Character* character);
int CountActiveCharacters(void);
unsigned int FindFreePartySlot(unsigned int first, unsigned int last);

extern unsigned char g_in_combat_00683f94;

void Function4EF1F0(void);
int Function4EF4A0(W8Character* character, int slot);
void Function4EF610(int party_slot, int value);

struct W8IList;
extern W8IList* g_combat_monster_list_00683fad; /* 0x00683FAD */
extern W8IList* g_combat_group_list_00683fb1;   /* 0x00683FB1 */
void Function4EEF10(int value, int mode);       /* 0x004EEF10 */
