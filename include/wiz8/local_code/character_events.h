#pragma once

bool IsVoiceMuted(void);
/* The audio panels pass the raw W8TextControl mask bit (0 or 2) through, so the
   transition is binary but the argument stays byte-valued. */
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8CharacterEvent;

extern int g_special_event_0068c558;

int UpdateCharacterEventState(void);
W8CharacterEvent* QueueCharacterEvent(W8Character* character, int effect, int argument, int value_1,
                                      unsigned int value_2);

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. Returns zero and empties it when the type has no
   quote. */
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int type,
                                       unsigned int* metadata);
extern int g_effect_005ee588;

/* True when no occupied party slot has an active portrait/voice record. */
unsigned char PartyPortraitEventsIdle(void); /* 0x0052E590 */
int ApplyItemEffectToRandomCharacter0052E5C0(unsigned int item_id, int character_filter,
                                             int value_3, int value_4);
void MaybeStartIncapacitationEvent(unsigned int party_slot); /* 0x0052F060 */
void QueueDamageReactionEvents(W8Character* character);      /* 0x0052F2C0 */
void Function52F110(int party_slot);
void Function52F430(W8Character* character);
void Function52F790(W8Character* character, int condition);
void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const wchar_t* quote_text, int show_quote);
void PostCharacterMessage(int party_slot, const wchar_t* format, ...);
