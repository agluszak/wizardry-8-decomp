#pragma once
struct ScalarState {
    int duration;
    int stored_duration;
};
extern ScalarState g_scalar_state;
int ReadDuration();
void StoreDuration(int);
int IsIntegerPredicate();
