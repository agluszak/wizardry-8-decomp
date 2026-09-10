#pragma once

bool IsVoiceMuted(void);
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8StartupStateElement005EE748;

extern int g_special_event_0068c558;

int Function52E750(void);
W8StartupStateElement005EE748* Function52E690(
    W8Character* character, int effect, int argument, int value_1,
    unsigned int value_2);

int ApplyItemEffectToRandomCharacter0052E5C0(
    unsigned int item_id, int character_filter, int value_3, int value_4);
unsigned char CharacterHasEffect(void* effect, int party_slot);   /* 0x0052DD90 */
void Function52CA60(void);
void Function52F110(int party_slot);
void Function52F430(void* character);
void Function52F790(void* character, int condition);
void Function52F890(int party_slot, int value_1, int value_2, int value_3, int value_4);

