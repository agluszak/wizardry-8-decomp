#pragma once

#include "wiz8/text_types.h"

void Function518510(void* notice);
struct W8MonsterManagerEntry;
struct W8Character;

int GetNextCharacter(int require_primary, int require_secondary, int previous_slot);
int RPCPtrToPCSlot(const W8MonsterManagerEntry* rpc);
void StripMonsterNameSuffix(W8WideChar* name);
unsigned int CharacterPointerToPartySlot(const W8Character* character);
bool IsPartyCharacterPointer(const W8Character* character);
