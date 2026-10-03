#include "scalar_test.h"
void StoreDuration(int milliseconds)
{
    g_scalar_state.stored_duration = milliseconds;
}

void MutateOverload(int* value) { *value = 1; }
