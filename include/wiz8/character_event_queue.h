#ifndef WIZ8_CHARACTER_EVENT_QUEUE_H
#define WIZ8_CHARACTER_EVENT_QUEUE_H

#include "wiz8/item_instance.h"
#include "wiz8/vector.h"

struct W8Character;

/* One queued character-event entry. The ctor, quote formatter, and process
   method live with QueueCharacterEvent in character_events.cpp; the original
   type name is unknown. */
struct W8CharacterEvent {
    W8CharacterEvent(W8Character* character, unsigned int type, int value_0c, unsigned int flags,
                     int value_14);

    unsigned char handled_00;
    unsigned char unknown_01[3];
    W8Character* character_04;
    unsigned int type_08;
    int value_0c;
    unsigned int flags_10;
    int value_14;
    int value_18;
    int value_1c;
    /* 0x20: the pending event type TryAdjustQueuedEvent stores before rewriting
       type_08 to the shared follow-up id 10. */
    unsigned int pending_event_type_20;
    /* 0x24: an embedded item instance: the event consumer passes it to
       GetItemDisplayName, which reads both the id and the identified flag.
       Only the id is ever established here; the remaining bytes keep whatever
       the allocation gave them, exactly as before. */
    W8ItemInstance item_24;
    int value_30;
    /* 0x34: the GetTickCount stamp the sight code pairs with value_30. */
    unsigned int clock_34;

    /* Applies this entry's queued runtime consequence. */
    void Process0052CED0(); /* 0x0052CED0 */

    /* Returns this entry's formatted quote text in the shared wide buffer. */
    wchar_t* GetQuoteText(); /* 0x0052D240 */

    /* Starts portrait/voice dispatch for this queued entry. */
    unsigned char DispatchCharacterEventEntry(); /* 0x0052CA60 */

    unsigned char CharacterEventConditionMet(unsigned int event_type); /* 0x0052C910 */
    unsigned char PlayEventSound();                                    /* 0x0052D260 */
};

static_assert(sizeof(W8CharacterEvent) == 0x38, "W8CharacterEvent_must_be_0x38");

/* The character-event queue. Construction is at 0x0052D460; QueueEntry and the
   later methods occupy the following 0x52Dxxx block. Those bodies live in
   character_events.cpp, an unresolved fragment until TU evidence names an
   original owner. */
struct W8CharacterEventQueue {
    W8GrowableVector<W8CharacterEvent*> vector_00;
    W8GrowableVector<W8CharacterEvent*> vector_10;
    W8GrowableVector<W8CharacterEvent*> vector_20;
    W8GrowableVector<W8CharacterEvent*> vector_30;
    W8GrowableVector<W8CharacterEvent*> vector_40;
    int value_50;
    int value_54;
    int unknown_58;
    int value_5c;
    int unknown_60;
    int value_64;
    unsigned char* bytes_68;

    W8CharacterEventQueue();
    ~W8CharacterEventQueue();
    void ClearOwnedEntries();
    int QueueEntry(W8CharacterEvent* entry);
    void SetEventCharacterMask(unsigned int event_type, unsigned int party_slot, bool enabled);
    bool HasEventCharacter(unsigned int event_type, unsigned int party_slot); /* 0x0052DD90 */
    void ProcessDeferredCharacterEvents();                                    /* 0x0052DDD0 */
    unsigned char TryAdjustQueuedEvent(W8CharacterEvent* entry);              /* 0x0052DC80 */
    unsigned char IsMainQueueEmpty() const;                                   /* 0x0052E470 */
    void ProcessOwnedEntry(W8CharacterEvent* entry);
    void ProcessNextPendingEntry();
    /* Restarts the follow-up clock for entries of the middle event band while
       the state flag selects it. QueueEntry reaches it for stolen entries. */
    void RestartFollowUpClock(W8CharacterEvent* entry);
};

static_assert(sizeof(W8CharacterEventQueue) == 0x6c, "W8CharacterEventQueue_must_be_0x6c");

#endif
