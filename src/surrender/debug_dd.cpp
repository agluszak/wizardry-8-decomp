#include "surrender/srDebugDD.h"

// FUNCTION: SURRENDER 0x10016d60
srDebugDD::~srDebugDD() {}

// FUNCTION: SURRENDER 0x10016cc0
void srDebugDD::resetInternalStatistics()
{
    int command;

    for (command = 0; command < 0x2b; ++command) {
        call_times_18[command] = 0.0;
        call_counts_170[command] = 0;
    }
}

// FUNCTION: SURRENDER 0x10017740
unsigned long srDebugDD::getFunctionCallCount(e_command command) const
{
    return call_counts_170[command];
}

// FUNCTION: SURRENDER 0x10017760
double srDebugDD::getFunctionCallTime(e_command command) const
{
    double time = call_times_18[command] - call_counts_170[command] * time_scale_10;
    if (time <= 0.0) {
        time = 0.0;
    }
    return time;
}

// FUNCTION: SURRENDER 0x100177a0
void srDebugDD::increaseCallCount(e_command command)
{
    ++call_counts_170[command];
}
