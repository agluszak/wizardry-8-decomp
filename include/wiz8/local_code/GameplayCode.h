#pragma once

struct W8Character;

void AdvanceCharacterToLevel(W8Character* character, unsigned int level);
bool AnyCharacterActive(void);
void CalcInitiative(W8Character* character);
void CalcAttacks(W8Character* character);
void CalcArmorClasses(W8Character* character);
/* 0x00616308: the percentage of hits that land on each of the five character
   hit locations, in the order the per-location armour classes use; the error
   text spells its name. */
extern const unsigned char gubLocalACPercent[5];
void CalcCharacterLevelBand(W8Character* character);

void CalcCharacterTableValue(W8Character* character);
int CountActiveCharacters(void);
unsigned int FindFreePartySlot(unsigned int first, unsigned int last);

void Function4EF1F0(void);
void Function4EFAD0(W8Character* character); /* 0x004EFAD0 */
int AddCharacterToParty(W8Character* character, int slot);
void Function4EF610(int party_slot, int value);

void Function4EEF10(int value, int mode); /* 0x004EEF10 */
