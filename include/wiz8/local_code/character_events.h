#pragma once

bool IsVoiceMuted(void);
/* The audio panels pass the raw W8TextControl mask bit (0 or 2) through, so the
   transition is binary but the argument stays byte-valued. */
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8StartupStateElement005EE748;

extern int g_special_event_0068c558;

int UpdateCharacterEventState(void);
W8StartupStateElement005EE748* QueueCharacterEvent(W8Character* character, int effect, int argument,
                                                   int value_1, unsigned int value_2);

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. Returns zero and empties it when the type has no
   quote. */
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int type,
                                       unsigned int* metadata);
extern int g_effect_005ee588;

int ApplyItemEffectToRandomCharacter0052E5C0(unsigned int item_id, int character_filter,
                                             int value_3, int value_4);
unsigned char CharacterHasEffect(void* effect, int party_slot); /* 0x0052DD90 */
void Function52CA60(void);
void MaybeStartIncapacitationEvent(unsigned int party_slot); /* 0x0052F060 */
void Function52F110(int party_slot);
void Function52F430(void* character);
void Function52F790(void* character, int condition);
void Function52F890(int party_slot, int value_1, int value_2, int value_3, int value_4);
void PostCharacterMessage(int party_slot, const wchar_t* format, ...);
