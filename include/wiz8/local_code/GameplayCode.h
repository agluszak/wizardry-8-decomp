#pragma once

struct W8Character;

/* W8Character::armor_class_by_location and gubLocalACPercent share this order.
   CalcArmorClasses maps equipment slots {Head,Torso,Legs,Feet,Hands} onto it;
   the retail manual independently describes those five local AC values and
   notes the same chest-heavy / hands-light hit-frequency weighting. */
enum W8ArmorLocation {
    W8_ARMOR_LOCATION_HEAD = 0,
    W8_ARMOR_LOCATION_TORSO = 1,
    W8_ARMOR_LOCATION_LEGS = 2,
    W8_ARMOR_LOCATION_FEET = 3,
    W8_ARMOR_LOCATION_HANDS = 4,
    W8_ARMOR_LOCATION_COUNT = 5
};

/* The twelve values shown by the Armor Class Modifiers panel. The first eleven
   labels are listed in this order by the manual and line up one-for-one with
   CalcArmorClasses' producers. Slot eleven is only displayed when nonzero and
   is fed exclusively by skill 0x26, Reflextion. The backing character array
   has one additional cleared-but-undisplayed entry at index twelve. */
enum W8ArmorClassComponent {
    W8_AC_COMPONENT_RACE = 0,
    W8_AC_COMPONENT_SPEED = 1,
    W8_AC_COMPONENT_STEALTH = 2,
    W8_AC_COMPONENT_SHIELD = 3,
    W8_AC_COMPONENT_MAGIC_ITEMS = 4,
    W8_AC_COMPONENT_MAGIC_SPELLS = 5,
    W8_AC_COMPONENT_VS_PENETRATION = 6,
    W8_AC_COMPONENT_ENCUMBRANCE = 7,
    W8_AC_COMPONENT_CONDITIONS = 8,
    W8_AC_COMPONENT_FATIGUE = 9,
    W8_AC_COMPONENT_DEFENSIVE_ACTION = 10,
    W8_AC_COMPONENT_REFLEXTION = 11,
    W8_AC_COMPONENT_UNUSED_12 = 12,
    W8_AC_COMPONENT_COUNT = 13
};

void AdvanceCharacterToLevel(W8Character* character, unsigned int level);
bool AnyCharacterActive(void);
void CalcInitiative(W8Character* character);
void CalcAttacks(W8Character* character);
void CalcArmorClasses(W8Character* character);
/* 0x00616308: the percentage of hits that land on each character armor
   location; the error text spells its original name. */
extern const unsigned char gubLocalACPercent[W8_ARMOR_LOCATION_COUNT];
void CalcCharacterLevelBand(W8Character* character);

void CalcCharacterTableValue(W8Character* character);
int CountActiveCharacters(void);
unsigned int FindFreePartySlot(unsigned int first, unsigned int last);

void RefreshLevelUpReadyNotices(void); /* 0x004EF1F0 */
int AddCharacterToParty(W8Character* character, int slot);
/* 0x004EF610: remove a slot's character from the party; fSaveCharData
   persists it back to its NPC record first. */
unsigned char RemoveCharacterFromParty(int party_slot, char save_character_data);

void Function4EEF10(int value, int mode); /* 0x004EEF10 */
bool IsCharacterReadyToAdvance(int party_slot);
void CalcXPGoal(W8Character* character);
void DeriveCharacterPersonality004EFA30(W8Character* character);
/* Rerolls personality_0081/voice_0085 until no party member shares the
   character's gender/personality/voice pick. */
void EnsureUniquePartyVoice004EFAD0(W8Character* character); /* 0x004EFAD0 */
/* 0x00587C80: knock-knock style spell committed against the active lock or
   trap interaction; with no interaction open it just prints the refusal
   notice. The lock-interaction owner lives past MainGameScreen.h, which this
   task does not expand. */
void CastSpellAtLockInteraction00587C80(unsigned int level, int flag, int backfire);
unsigned int GetAveragePartyLevel(void); /* 0x004EF420 */
void Function4EF7E0(W8Character*, W8Character*, int);
bool AnyMonsterEngaged(void); /* 0x004EEE20 */
