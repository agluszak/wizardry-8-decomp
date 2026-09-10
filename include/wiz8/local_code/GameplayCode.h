#pragma once

struct W8Character;

bool AnyCharacterActive(void);
void CalcInitiative(W8Character* character);
void CalcAttacks(W8Character* character);
void CalcArmorClasses(W8Character* character);
void CalcCharacterLevelBand(W8Character* character);

void CalcCharacterTableValue(W8Character* character);
int CountActiveCharacters(void);
unsigned int FindFreePartySlot(unsigned int first, unsigned int last);

extern unsigned char g_in_combat_00683f94;
