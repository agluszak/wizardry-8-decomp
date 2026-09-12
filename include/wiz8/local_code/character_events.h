#pragma once

struct W8StartupRuntimeState;

bool IsVoiceMuted(void);
/* The audio panels pass the raw W8TextControl mask bit (0 or 2) through, so the
   transition is binary but the argument stays byte-valued. */
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8StartupStateElement005EE748;

struct W8CharacterEventDescriptor {
    unsigned int category_00;
    unsigned char followup_remap_04;
    unsigned char defer_off_char_screen_05;
};

static_assert(sizeof(W8CharacterEventDescriptor) == 8, "W8CharacterEventDescriptor_stride");

extern const W8CharacterEventDescriptor g_character_event_descriptors_005ee000[0x92];

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
extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed914;

/* True when no occupied party slot has an active portrait/voice record. */
unsigned char PartyPortraitEventsIdle(void); /* 0x0052E590 */
unsigned char __fastcall
StartupRuntimeDeferredQueueEmpty(W8StartupRuntimeState* state); /* 0x0052E470 */
int ApplyItemEffectToRandomCharacter0052E5C0(unsigned int item_id, int character_filter,
                                             int value_3, int value_4);
unsigned char CharacterHasEffect(void* effect, int party_slot); /* 0x0052DD90 */
unsigned char DispatchQueuedCharacterEvent(W8StartupStateElement005EE748* entry);
void UpdateNpcDialogueVoiceIdle(void);                       /* 0x00524DA0 */
void ProcessNpcScriptingIdlePass(void);                      /* 0x00524EB0 */
void MaybeStartIncapacitationEvent(unsigned int party_slot); /* 0x0052F060 */
void Function52F110(int party_slot);
void Function52F430(void* character);
void Function52F790(void* character, int condition);
void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const void* quote_text, int show_quote);
void PostCharacterMessage(int party_slot, const wchar_t* format, ...);
