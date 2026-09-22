/* In-process semantic scenario for the Trigger lock/trap device block
   (Trigger+0x368 .. +0x388), exercised through the two recovered helpers that
   take it by pointer: UpdateTriggerLock00445730 and
   ConsumeLockQuality004457A0.

   The W8LockState record the helpers operate on is:

     +0x00 lock_type            1 = pickable lock, 2 = trap, 3 = key lock
     +0x04 difficulty           the editor "Difficulty" value
     +0x08 device_state.completed latch
     +0x09 device_state.pins    eight tumbler bytes
     +0x14 device_id            tumbler/trap table row (-1 = roll on first use)
     +0x18 key_id               key item for key locks
     +0x1c lock_countdown       pin attempts remaining
     +0x20 last_interaction     sentinel -1, world clock once used

   UpdateTriggerLock re-rolls the eight pin bytes for a pickable lock, derives
   the countdown from the clamped difficulty, and resets the completed latch
   and the interaction sentinel; ConsumeLockQuality ticks the countdown and
   reports whether one remained. */

#include "lock_device_semantic_test.h"

#include "wiz8/engine_code/Trigger.hpp"

#include <stdio.h>
#include <string.h>

bool RunLockDeviceSemanticTest(LockDeviceSemanticResult* result)
{
    W8LockState lock_state;
    int pin;

    memset(result, 0, sizeof(*result));

    /* Pickable lock, difficulty 5: eight pins re-rolled into 0..3, countdown
       pins*3, completed latch cleared, interaction sentinel armed. */
    memset(&lock_state, 0x7f, sizeof(lock_state));
    lock_state.lock_type = 1;
    lock_state.difficulty = 5;
    UpdateTriggerLock00445730(&lock_state);
    result->pins_rerolled_in_range = 1;
    for (pin = 0; pin < 8; ++pin) {
        if (lock_state.device_state.pins[pin] > 3) {
            result->pins_rerolled_in_range = 0;
        }
    }
    result->countdown_is_pins_times_three = lock_state.lock_countdown == 15;
    result->sentinel_and_completed_reset =
        lock_state.last_interaction_clock == -1 && lock_state.device_state.completed == 0;

    /* Difficulty below 2 takes the early path: fixed countdown of 6. */
    memset(&lock_state, 0, sizeof(lock_state));
    lock_state.lock_type = 1;
    lock_state.difficulty = 1;
    UpdateTriggerLock00445730(&lock_state);
    result->low_difficulty_countdown = lock_state.lock_countdown == 6;

    /* Difficulty above the eight-pin ceiling clamps to 8: countdown 24. */
    lock_state.difficulty = 20;
    UpdateTriggerLock00445730(&lock_state);
    result->high_difficulty_clamped = lock_state.lock_countdown == 24;

    /* A trap (lock_type 2) never touches the pin bytes; only the completed
       latch and the sentinel are reset. */
    memset(&lock_state, 0, sizeof(lock_state));
    lock_state.lock_type = 2;
    lock_state.lock_countdown = 9;
    memset(lock_state.device_state.pins, 0xaa, 8);
    lock_state.device_state.completed = 1;
    lock_state.last_interaction_clock = 5;
    UpdateTriggerLock00445730(&lock_state);
    result->non_lock_pins_untouched = 1;
    for (pin = 0; pin < 8; ++pin) {
        if (lock_state.device_state.pins[pin] != 0xaa) {
            result->non_lock_pins_untouched = 0;
        }
    }
    result->non_lock_pins_untouched = result->non_lock_pins_untouched && lock_state.lock_countdown == 9 &&
                                      lock_state.device_state.completed == 0 &&
                                      lock_state.last_interaction_clock == -1;

    /* The countdown ticks down one at a time and reports exhaustion. */
    lock_state.lock_countdown = 2;
    result->decrement_ticks =
        ConsumeLockQuality004457A0(&lock_state) == 1 && lock_state.lock_countdown == 1;
    result->decrement_ticks = result->decrement_ticks &&
                              ConsumeLockQuality004457A0(&lock_state) == 1 &&
                              lock_state.lock_countdown == 0;
    result->decrement_stops_at_zero =
        ConsumeLockQuality004457A0(&lock_state) == 0 && lock_state.lock_countdown == 0;

    return result->pins_rerolled_in_range && result->countdown_is_pins_times_three &&
           result->low_difficulty_countdown && result->high_difficulty_clamped &&
           result->non_lock_pins_untouched && result->sentinel_and_completed_reset &&
           result->decrement_ticks && result->decrement_stops_at_zero;
}

void PrintLockDeviceSemanticResults(const LockDeviceSemanticResult* result)
{
    fprintf(stderr,
            "lock-device: pins_range=%d countdown=%d low_diff=%d clamped=%d "
            "trap_untouched=%d resets=%d ticks=%d stops=%d\n",
            result->pins_rerolled_in_range, result->countdown_is_pins_times_three,
            result->low_difficulty_countdown, result->high_difficulty_clamped,
            result->non_lock_pins_untouched, result->sentinel_and_completed_reset,
            result->decrement_ticks, result->decrement_stops_at_zero);
}
