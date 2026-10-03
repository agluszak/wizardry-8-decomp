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
    operator int() const { return 1; }
    const char* operator()() const { return "value"; }
};

void TouchConversions(const ScalarConversions& converted)
{
    int numeric = converted;
    const char* text = converted();
}

// No concrete width exists before the owner template is instantiated.
// Querying MSVC member-pointer layout here crashes ASTContext.
template<class Owner> struct ScalarMemberPointers {
    int Owner::* member;
};

struct ScalarOwner { int owner_data; };
int ScalarOwner::* known_member;

struct ScalarMethodOwner {
    int method_duration;
    int GetMethodDuration() const { return method_duration; }
};

// Candidate classification must preserve the bool client's typedef handling.
typedef unsigned char ScalarByte;
ScalarByte ReadByteAlias() { return 1; }

void CheckOverload(OverloadState* state)
{
    state->overload_ready = 0;
    MutateOverload(state);
    if (state->overload_ready) {}
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
    if (state->notices_pending[index] == 0) {}
    memset(state->invalid_pending, 0, sizeof state->invalid_pending);
    state->invalid_pending[index] = 2;
    if (state->invalid_pending[index]) {}
    memset(state->partial_pending, 0, sizeof state->partial_pending - 1);
    state->partial_pending[index] = 1;
    if (state->partial_pending[index]) {}
    memset(state->escaped_pending, 0, sizeof state->escaped_pending);
    RawArray(state->escaped_pending);
    if (state->escaped_pending[index]) {}
    memset(&state->shifted_pending[1], 0, sizeof state->shifted_pending);
    if (state->shifted_pending[index]) {}
    memset(state->numeric_pending, 0, sizeof state->numeric_pending);
    ++state->numeric_pending[index];
    if (state->numeric_pending[index]) {}
}
