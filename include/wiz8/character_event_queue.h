#ifndef WIZ8_CHARACTER_EVENT_QUEUE_H
#define WIZ8_CHARACTER_EVENT_QUEUE_H

#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/vector.h"

struct W8Character;

/* W8CharacterEvent::flags bits. Bit 0x01 is written by several producers but
   never tested anywhere in retail. */
enum W8CharacterEventFlag {
    /* Dispatch skips the eligibility and condition checks entirely. */
    W8_EVENT_BYPASS_CHECKS = 0x04,
    /* QueueEntry never diverts the event to npc_deferred_events while NPC
       scripting is active. */
    W8_EVENT_NO_NPC_DEFER = 0x08,
    /* The portrait quote/subtitle is suppressed; the dispatch path tests it
       through the g_character_event_flags_mask_005ed8e4 global. */
    W8_EVENT_SUPPRESS_QUOTE = 0x10,
    /* Events above the ordinary range normally preempt the slot's active
       event and dispatch immediately; this bit queues them normally. */
    W8_EVENT_NO_PREEMPT = 0x20,
    /* The event is an NPC-script line: Dispatch reloads the NPC's script
       resources around RunNpcScriptLine and Complete skips the item notice. */
    W8_EVENT_NPC_SCRIPT = 0x40
};

/* 0x005ED8E4: mask gating the portrait quote/subtitle flag; dispatch tests
   W8_EVENT_SUPPRESS_QUOTE through it. */
extern unsigned char g_character_event_flags_mask_005ed8e4;

/* One queued character-event entry. The ctor, quote formatter, and process
   method live with QueueCharacterEvent in Health Stamina Mana.cpp; the
   original type name is unknown. */
struct W8CharacterEvent {
    W8CharacterEvent(W8Character* character, unsigned int event_type, int value_0c,
                     unsigned int flags, int volume);

    /* 0x00: set before the sound is stopped on synchronous shutdown; the
       sound-end callback bails when it is set. */
    unsigned char sound_end_handled;
    unsigned char unknown_01[3];
    W8Character* character;
    unsigned int event_type;
    int value_0c;
    unsigned int flags;
    /* 0x14: the 0-127 multiplier used for SOUNDPARMS::uiVolume. */
    int volume;
    int value_18;
    int value_1c;
    /* 0x20: the original event type TryAdjustQueuedEvent stores before
       rewriting event_type to the shared follow-up id 10. */
    unsigned int original_event_type;
    /* 0x24: an embedded item instance: the event consumer passes it to
       GetItemDisplayName, which reads both the id and the identified flag.
       Only the id is ever established here; the remaining bytes keep whatever
       the allocation gave them, exactly as before. */
    W8ItemInstance item;
    /* 0x30/0x34: ProcessDeferredCharacterEvents delays dispatch until this many
       milliseconds have elapsed since the queued GetTickCount stamp. */
    int dispatch_delay_ms;
    unsigned int dispatch_delay_start;

    /* Applies this entry's queued runtime consequence. */
    void Complete(); /* 0x0052CED0 */

    /* Returns this entry's formatted quote text in the shared wide buffer. */
    wchar_t* GetQuoteText(); /* 0x0052D240 */

    /* Starts portrait/voice dispatch for this queued entry. */
    unsigned char Dispatch(); /* 0x0052CA60 */

    unsigned char IsConditionMet(unsigned int event_type); /* 0x0052C910 */
    unsigned char PlayEventSound();                        /* 0x0052D260 */
};

static_assert(sizeof(W8CharacterEvent) == 0x38, "W8CharacterEvent_must_be_0x38");

/* The character-event queue. Construction is at 0x0052D460; QueueEntry and the
   later methods occupy the following 0x52Dxxx block. The whole 0x52C810-0x52FEE0
   span sits inside Local Code\Health Stamina Mana.cpp's emission range: 0x52C500
   asserts that file's name before it and 0x52FE80 is placed after it, so the
   bodies live in Health Stamina Mana.cpp. */
struct W8CharacterEventQueue {
    /* 0x00/0x20: no retail code path ever enqueues into these; they are still
       drained and scanned alongside the live vectors. */
    W8GrowableVector<W8CharacterEvent*> vector_00;
    W8GrowableVector<W8CharacterEvent*> pending_events;
    W8GrowableVector<W8CharacterEvent*> vector_20;
    W8GrowableVector<W8CharacterEvent*> npc_deferred_events;
    W8GrowableVector<W8CharacterEvent*> active_events;
    int active_event_type;
    int active_party_slot;
    /* 0x58: a five-second countdown armed by every successful Dispatch; while
       it ticks, active_event_type/active_party_slot still describe the last
       dispatched event for duplicate coalescing. */
    int recent_event_clock;
    int follow_up_flags;
    int follow_up_clock;
    /* 0x64: the party slot that spoke the last follow-up event; the response
       pick excludes it. */
    int follow_up_speaker_slot;
    unsigned char* event_character_masks;

    W8CharacterEventQueue();
    ~W8CharacterEventQueue();
    void DestroyAllEvents();
    int QueueEntry(W8CharacterEvent* entry);
    void SetEventCharacterMask(unsigned int event_type, unsigned int party_slot, bool enabled);
    bool HasEventCharacter(unsigned int event_type, unsigned int party_slot); /* 0x0052DD90 */
    void ProcessDeferredCharacterEvents();                                    /* 0x0052DDD0 */
    unsigned char TryAdjustQueuedEvent(W8CharacterEvent* entry);              /* 0x0052DC80 */
    unsigned char IsMainQueueEmpty() const;                                   /* 0x0052E470 */
    void CompleteActiveEvent(W8CharacterEvent* entry);
    void CompleteFirstActiveEvent();
    /* Restarts the follow-up clock for entries of the middle event band while
       the state flag selects it. QueueEntry calls the emitted body; Dispatch,
       CompleteActiveEvent, and CompleteFirstActiveEvent inline copies of the
       same logic in retail, so the source repeats it there. */
    void RestartFollowUpClock(W8CharacterEvent* entry);
    /* Removes every queued event belonging to a character; active ones are
       completed, the rest deleted. Runs when a character dies. */
    void RemoveCharacterEvents(W8Character* character); /* 0x0052D970 */
    /* Drains active_events, completing each entry without deleting it. */
    void CompleteAllActiveEvents();  /* 0x0052DB30 */
    unsigned char HasActiveEvents(); /* 0x0052E460 */
    /* Advances the ambient follow-up exchange: arms it, then on each clock
       expiry queues another middle-band event on a random member, excluding
       the previous speaker. */
    void ProcessFollowUpEvents(); /* 0x0052E1C0 */
};

static_assert(sizeof(W8CharacterEventQueue) == 0x6c, "W8CharacterEventQueue_must_be_0x6c");

#endif
