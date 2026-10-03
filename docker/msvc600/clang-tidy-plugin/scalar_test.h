#pragma once
struct ScalarState {
    int duration;
    int stored_duration;
};
extern ScalarState g_scalar_state;
int ReadDuration();
void StoreDuration(int);
int IsIntegerPredicate();

struct OverloadState {
    unsigned char overload_ready;
};
void MutateOverload(OverloadState*);
void MutateOverload(int*);

struct FixtureRecord {
    int record_value;
};
struct FixtureOther {
    int other_value;
};
typedef unsigned int FixtureMonsterId;
typedef int (*FixtureCallback)(int);
extern FixtureCallback fixture_callback;
int FixtureImplementation(int);
void InvokeFixtureCallback();

struct FixtureReceiverBase {
    unsigned int receiver_storage;
};
struct FixtureReceiver : FixtureReceiverBase {
    void external();
};
void fixture_receiver_escape(FixtureReceiver* receiver);
