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
