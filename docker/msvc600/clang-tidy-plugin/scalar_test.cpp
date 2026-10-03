#include "scalar_test.h"
ScalarState g_scalar_state;
int ReadDuration()
{
    return g_scalar_state.duration;
}
void CopyDuration()
{
    int value = ReadDuration();
    StoreDuration(value);
}
int IsIntegerPredicate()
{
    return g_scalar_state.duration == 0;
}

// Existing enum identity is distinct from the integer storage along its chain.
enum W8FixtureMode { ModeReady = 1, ModeBusy };
int g_fixture_mode;
int ReadMode()
{
    return ModeReady;
}
void CopyMode()
{
    int mode = ReadMode();
    g_fixture_mode = mode;
}

// Operator names are not Clang simple identifiers; inventory them without
// calling NamedDecl::getName(), which asserts for these declarations.
struct ScalarConversions {
    operator int() const
    {
        return 1;
    }
    const char* operator()() const
    {
        return "value";
    }
};

void TouchConversions(const ScalarConversions& converted)
{
    int numeric = converted;
    const char* text = converted();
}

// No concrete width exists before the owner template is instantiated.
// Querying MSVC member-pointer layout here crashes ASTContext.
template <class Owner> struct ScalarMemberPointers {
    int Owner::*member;
};

struct ScalarOwner {
    int owner_data;
};
int ScalarOwner::*known_member;

struct ScalarMethodOwner {
    int method_duration;
    int GetMethodDuration() const
    {
        return method_duration;
    }
};

// Candidate classification must preserve the bool client's typedef handling.
typedef unsigned char ScalarByte;
ScalarByte ReadByteAlias()
{
    return 1;
}

void CheckOverload(OverloadState* state)
{
    state->overload_ready = 0;
    MutateOverload(state);
    if (state->overload_ready) {
    }
}

extern "C" void* memset(void*, int, unsigned int);
void RawArray(unsigned char*);
struct ArrayState {
    unsigned char notices_pending[4];
    unsigned char invalid_pending[4];
    unsigned char partial_pending[4];
    unsigned char escaped_pending[4];
    unsigned char shifted_pending[4];
    unsigned char numeric_pending[4];
};
void CheckArrays(ArrayState* state, int index)
{
    memset(&state->notices_pending[0], 0, sizeof state->notices_pending);
    state->notices_pending[index] = 1;
    if (state->notices_pending[index] == 0) {
    }
    memset(state->invalid_pending, 0, sizeof state->invalid_pending);
    state->invalid_pending[index] = 2;
    if (state->invalid_pending[index]) {
    }
    memset(state->partial_pending, 0, sizeof state->partial_pending - 1);
    state->partial_pending[index] = 1;
    if (state->partial_pending[index]) {
    }
    memset(state->escaped_pending, 0, sizeof state->escaped_pending);
    RawArray(state->escaped_pending);
    if (state->escaped_pending[index]) {
    }
    memset(&state->shifted_pending[1], 0, sizeof state->shifted_pending);
    if (state->shifted_pending[index]) {
    }
    memset(state->numeric_pending, 0, sizeof state->numeric_pending);
    ++state->numeric_pending[index];
    if (state->numeric_pending[index]) {
    }
}

// Mask use, mutually exclusive comparisons and negative sentinels are separate
// observations: none of them alone recovers an authored enum/bool declaration.
int fixture_flags;
int fixture_status;
void ObserveDomains()
{
    fixture_flags |= 4;
    if (fixture_flags & 8)
        fixture_status = -1;
    else
        fixture_status = 1;
    if (fixture_status == -1)
        fixture_status = 0;
    switch (fixture_status) {
    case 0:
        break;
    case 1:
        break;
    }
}

FixtureRecord* fixture_owner;
void* fixture_storage;
FixtureRecord* ReadFixtureRecord()
{
    fixture_storage = fixture_owner;
    return static_cast<FixtureRecord*>(fixture_storage);
}
FixtureOther* fixture_other;
void* fixture_mixed_storage;
FixtureRecord* fixture_mixed_owner;
void ObserveMixedPointers()
{
    fixture_mixed_storage = fixture_mixed_owner;
    fixture_mixed_storage = fixture_other;
}

FixtureMonsterId fixture_monster_id;
unsigned int fixture_id_copy;
void CopyFixtureId()
{
    fixture_id_copy = fixture_monster_id;
}

FixtureCallback fixture_callback = FixtureImplementation;
int FixtureImplementation(int code)
{
    return code;
}

void fixture_receiver_escape(FixtureReceiver* receiver)
{
    receiver->external();
}

FixtureCallback fixture_callback_table[2] = {FixtureImplementation, FixtureImplementation};
struct FixtureCallbackRow {
    int tag;
    FixtureCallback callback;
};
FixtureCallbackRow fixture_callback_rows[2] = {{1, FixtureImplementation},
                                               {2, FixtureImplementation}};
int fixture_array_values[3] = {1, 2, 3};
char fixture_text[5] = "text";
int invoke_callback_table(int index, int value)
{
    fixture_array_values[index] = value;
    return fixture_callback_table[index](fixture_array_values[index]);
}
#pragma pack(push, 1)
struct FixturePackedRecord {
    char prefix;
    int payload;
};
#pragma pack(pop)
FixturePackedRecord fixture_packed;
