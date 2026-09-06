#pragma once

struct W8Character;

bool AnyCharacterActive(void);
void CalcInitiative(W8Character* character);
void CalcAttacks(W8Character* character);
void CalcArmorClasses(W8Character* character);
