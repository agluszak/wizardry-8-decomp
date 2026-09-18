#pragma once

#include <wchar.h>

#include "input.h"

bool IsVoiceMuted(void);
/* The audio panels pass the raw W8TextControl mask bit (0 or 2) through, so the
   transition is binary but the argument stays byte-valued. */
void SetVoiceMuted(unsigned char muted);

struct W8Character;
struct W8CharacterEvent;
struct W8MonsterManagerEntry;
struct W8Region;

extern int g_special_event_0068c558;
extern int g_special_event_0068c55c;
extern int g_special_event_0068c568;
extern int g_effect_005ee588;
extern int g_effect_005ee590;
extern int g_effect_005ee594;
extern int g_effect_005ee598;
/* Search-pulse event ids: the two found-item variants and the found-trigger
   event the pulse queues on the searcher. */
extern int g_effect_005ee5e4;
extern int g_effect_005ee5e8;
extern int g_effect_005ee5f0;
extern int g_effect_005ee5f8;
extern int g_effect_005ee60c;
extern int g_effect_005ee610;
extern int g_effect_005ee614; /* 0x005EE614: the victory-cheer character event */
extern int g_effect_005ee61c;
/* 0x005EE624: the character event an item use queues when the attempt ends
   without casting anything. */
extern int g_effect_005ee624;
extern unsigned int g_flee_hp_fraction_005ed8f8;
extern int g_item_message_005ee640;
extern int g_item_message_005ee644;
extern int g_item_message_005ee648;
extern int g_item_message_005ee64c;
extern int g_item_message_005ee5c8;
extern int g_item_message_005ee5cc;
extern int g_item_message_005ee664;
extern int g_item_message_005ee668;
extern int g_item_message_005ee68c;
extern int g_item_message_005ee690;
extern int g_item_message_005ee6fc;

int UpdateCharacterEventState(void);
W8CharacterEvent* QueueCharacterEvent(W8Character* character, int event_type, int argument,
                                      unsigned int flags, unsigned int volume);

/* 0x0052D0B0: format one character quote for the given event type into the
   shared wide text buffer. Returns zero and empties it when the type has no
   quote. */
unsigned char FormatCharacterQuoteText(W8Character* character, unsigned int event_type,
                                       unsigned int* metadata);
/* 0x005EE6F0: first entry of the -1-terminated .rdata event-id table read at
   0x00509560. */
extern const int g_value_005ee6f0;

/* True when no occupied party slot has an active portrait/voice record. */
unsigned char PartyPortraitEventsIdle(void);                            /* 0x0052E590 */
int PickRandomPartySpeaker(unsigned int event_type, int excluded_slot); /* 0x0052FEE0 */
W8CharacterEvent* ApplyItemEffectToRandomCharacter(unsigned int event_type, int excluded_slot,
                                                   int argument,
                                                   unsigned int flags); /* 0x0052E5C0 */
void MaybeStartIncapacitationEvent(unsigned int party_slot);            /* 0x0052F060 */
void QueueDamageReactionEvents(W8Character* character);                 /* 0x0052F2C0 */
/* 0x0052F110: after a character dies, pick one other party member and queue
   their reaction event with a three-second clock. */
void QueuePartyDeathReaction(unsigned int party_slot);
/* 0x0052F1D0: several members still active at turn begin; queue the shared
   reaction event on one random eligible character. */
void QueueTurnReactionEvent(void);
/* 0x0052F240: only one occupied member still standing at turn begin. */
void QueueLastSurvivorEvent(void);
/* 0x0052F430: react to a freshly recomputed highest_condition. */
void QueueConditionChangeReaction(W8Character* character);
/* 0x0052F790: react to a condition being lifted. */
void QueueConditionClearedReaction(W8Character* character, int condition);
/* 0x0052E480: requeue the selected character's stored portrait event. */
void RequeueSelectedPortraitEvent(void);
/* 0x0052F000: set the pose a party-slot portrait animates toward; clears any
   pose animation in progress and forces the incapacitated pose when the
   character is too far gone or the party is surprised. */
void SetPortraitTargetPose(struct W8MonsterManagerEntry* slot, int pose);
/* 0x0052FD80: left-click on a party portrait completes that slot's active
   event, or finishes the NPC voice playback for the player portraits. */
unsigned char PartyPortraitEventRegionEvent(const InputAtom* event, struct W8Region* region);
/* 0x0052FE00: re-blit each active portrait quote bubble when the screen comes
   back from a modal view. */
void RedrawPortraitQuoteBubbles(void);
/* 0x0052FE80: queue the character's breath/idle event unless a spell or item
   is being aimed; `force` queues it regardless. */
void StartBreathCycle(int party_slot, char force);
void RenderPartyPortrait0052EB00(int portrait, int left, int top, int flags, int value,
                                 int party_slot);
/* 0x0052EBE0: blit one animated portrait frame and its transition, returning
   whether a frame was drawn. */
char BlitPartyPortraitAnimation(int portrait, int left, int top, int flags, int party_slot,
                                char animate);

extern unsigned int g_event_flag_005ed8e0;
extern unsigned int g_event_flag_005ed8ec;
extern int g_effect_005ee58c;
extern int g_effect_005ee654;

void SetPartyPortraitEventState(unsigned int party_slot, unsigned char active,
                                unsigned int event_type, const wchar_t* quote_text, int show_quote);
void PostCharacterMessage(int party_slot, const wchar_t* format, ...);
/* 0x00590A40: the notice the weapon-set swap paths post, between the two variadic
   formatters. Its middle argument is the context the notices are posted under -
   zero while the NPC dialogue owns the screens, -1 otherwise. */
void PostCharacterNoticeInContext00590A40(int party_slot, int context, const wchar_t* format, ...);
extern int g_special_event_0068c50c;  /* 0x0068C50C */
extern unsigned int g_value_0068c554; /* 0x0068C554 */
extern int g_special_event_0068c56c;  /* 0x0068C56C: one of the three melee
                                         swing event ids StartCharacterAttack
                                         rolls between */
extern int g_special_event_0068c560;  /* 0x0068C560: one of the three blocked-hit
                                        reaction ids ContinueMonsterAttack rolls
                                        between, with 0x68c570 and 0x68c574 */
extern int g_special_event_0068c570;  /* 0x0068C570 */
extern int g_special_event_0068c574;  /* 0x0068C574 */
extern unsigned int g_value_0068c57c; /* 0x0068C57C */
extern int g_special_event_0068c530;  /* 0x0068C530: emitted when a slot's action
                                        cannot reach a monster group */
extern int g_special_event_0068c534;  /* 0x0068C534 */
extern int g_effect_argument_005ed8c8;
extern int g_effect_argument_005ed8d8;
extern unsigned int g_event_flag_005ed8e8; /* 0x005ED8E8 */
extern int g_effect_argument_005ed914;
extern int g_effect_005ee618; /* 0x005EE618: event type 36 - queued on the bound
                               party row plus one random peer when the allied
                               NPC dies (HandleScriptedNpcDeath) */
extern int g_effect_005ee630; /* 0x005EE630: event type 42 - queued on every
                               eligible party member in the same pass */
extern int g_effect_005ee638;
extern int g_effect_005ee658; /* 0x005EE658: event type 0x34 - rest-benefit
                               resolution after the surprise sequence */
