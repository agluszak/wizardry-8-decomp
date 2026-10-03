#include "scalar_test.h"
void StoreDuration(int milliseconds)
{
    g_scalar_state.stored_duration = milliseconds;
}

void MutateOverload(int* value) { *value = 1; }

void InvokeFixtureCallback()
{
    int callback_argument = 7;
    int callback_result = fixture_callback(callback_argument);
}
