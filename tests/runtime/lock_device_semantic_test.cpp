/* In-process semantic scenario for the Trigger lock/trap device block
   (Trigger+0x368 .. +0x388), exercised through the two recovered helpers that
   address it as a raw int blob: UpdateTriggerLock00445730 and
   ConsumeLockQuality004457A0.

   The blob layout the helpers rely on is:

     [0] lock_type            1 = pickable lock, 2 = trap, 3 = key lock
     [1] difficulty           the editor "Difficulty" value
     byte 8                   device_state.completed latch
     bytes 9..16              device_state.pins (eight tumbler bytes)
     [7] lock_countdown       pin attempts remaining
     [8] last_interaction     sentinel -1, world clock once used

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
    int lock_state[9];
    unsigned char* bytes = reinterpret_cast<unsigned char*>(lock_state);
    int pin;

    memset(result, 0, sizeof(*result));

    /* Pickable lock, difficulty 5: eight pins re-rolled into 0..3, countdown
       pins*3, completed latch cleared, interaction sentinel armed. */
    memset(lock_state, 0x7f, sizeof(lock_state));
    lock_state[0] = 1;
    lock_state[1] = 5;
    UpdateTriggerLock00445730(lock_state);
    result->pins_rerolled_in_range = 1;
    for (pin = 0; pin < 8; ++pin) {
        if (bytes[9 + pin] > 3) {
            result->pins_rerolled_in_range = 0;
        }
    }
    result->countdown_is_pins_times_three = lock_state[7] == 15;
    result->sentinel_and_completed_reset = lock_state[8] == -1 && bytes[8] == 0;

    /* Difficulty below 2 takes the early path: fixed countdown of 6. */
    memset(lock_state, 0, sizeof(lock_state));
    lock_state[0] = 1;
    lock_state[1] = 1;
    UpdateTriggerLock00445730(lock_state);
    result->low_difficulty_countdown = lock_state[7] == 6;

    /* Difficulty above the eight-pin ceiling clamps to 8: countdown 24. */
    lock_state[1] = 20;
    UpdateTriggerLock00445730(lock_state);
    result->high_difficulty_clamped = lock_state[7] == 24;

    /* A trap (lock_type 2) never touches the pin bytes; only the completed
       latch and the sentinel are reset. */
    memset(lock_state, 0, sizeof(lock_state));
    lock_state[0] = 2;
    lock_state[7] = 9;
    memset(bytes + 9, 0xaa, 8);
    bytes[8] = 1;
    lock_state[8] = 5;
    UpdateTriggerLock00445730(lock_state);
    result->non_lock_pins_untouched = 1;
    for (pin = 0; pin < 8; ++pin) {
        if (bytes[9 + pin] != 0xaa) {
            result->non_lock_pins_untouched = 0;
        }
    }
    result->non_lock_pins_untouched = result->non_lock_pins_untouched && lock_state[7] == 9 &&
                                      bytes[8] == 0 && lock_state[8] == -1;

    /* The countdown ticks down one at a time and reports exhaustion. */
    lock_state[7] = 2;
    result->decrement_ticks = ConsumeLockQuality004457A0(lock_state) == 1 && lock_state[7] == 1;
    result->decrement_ticks = result->decrement_ticks &&
                              ConsumeLockQuality004457A0(lock_state) == 1 && lock_state[7] == 0;
    result->decrement_stops_at_zero =
        ConsumeLockQuality004457A0(lock_state) == 0 && lock_state[7] == 0;

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
