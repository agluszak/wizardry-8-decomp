#pragma once
struct ScalarState {
    int duration;
    int stored_duration;
};
extern ScalarState g_scalar_state;
int ReadDuration();
void StoreDuration(int);
int IsIntegerPredicate();

struct OverloadState { unsigned char overload_ready; };
void MutateOverload(OverloadState*);
void MutateOverload(int*);
