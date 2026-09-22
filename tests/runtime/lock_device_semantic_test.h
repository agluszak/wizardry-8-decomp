#pragma once

struct LockDeviceSemanticResult {
    unsigned char pins_rerolled_in_range;
    unsigned char countdown_is_pins_times_three;
    unsigned char low_difficulty_countdown;
    unsigned char high_difficulty_clamped;
    unsigned char non_lock_pins_untouched;
    unsigned char sentinel_and_completed_reset;
    unsigned char decrement_ticks;
    unsigned char decrement_stops_at_zero;
};

bool RunLockDeviceSemanticTest(LockDeviceSemanticResult* result);
void PrintLockDeviceSemanticResults(const LockDeviceSemanticResult* result);
