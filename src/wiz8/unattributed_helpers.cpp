#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"

/*
 * Recovered bodies whose original translation unit is not established yet.
 *
 * They are here rather than in bringup_gates.cpp because that file is named for
 * the bring-up path and none of these are on it; putting them there would
 * assert a grouping the evidence does not support. Each carries its own owner
 * in the boundary map, and any of them can move once its unit is identified.
 */

/* Only the virtual destructor is established: the call goes through slot 0 with
   the deleting flag set, which is what `delete` on a polymorphic object emits.
   No field is known, so none is modelled. */
struct W8Releasable {
    virtual ~W8Releasable();
};

/* Only that one method is established, and nothing here shows a field. */
struct W8Forwarded {
    void Method4C5290();
};

extern "C" {

extern W8PList** g_plist_659ab4;
extern int g_dword_687599;

extern void Function4C4EF0(void);
extern void Function4A7A70(int value);

extern int g_dword_6874f7;
extern unsigned long g_tick_65b9a8;

/* Releases through the virtual destructor, tolerating a null. */
// FUNCTION: WIZ8 0x004C5860
void Function4C5860(W8Releasable* object)
{
    if (object != NULL) {
        delete object;
    }
}

/* The original holds the argument in ecx where this holds it in eax. Declaring
   it as a pointer does not change that - VC6 picks eax either way - so the
   argument stays an int and the register is left as the wobble it is. */
// FUNCTION: WIZ8 0x004C5ED0
void Function4C5ED0(int enabled)
{
    if (enabled != 0) {
        Function4C4EF0();
    }
}

/* Records a value and stamps it with the tick it was recorded at. */
// FUNCTION: WIZ8 0x00482720
void Function482720(int value)
{
    g_dword_6874f7 = value;
    g_tick_65b9a8 = GetTickCount();
}

/* Takes an argument the decompiler does not show and hands it on in ecx, so the
   callee is a method and this is a forwarder, not the nullary call it looks
   like. Nothing follows the call, so it leaves as a jump. */
// FUNCTION: WIZ8 0x004C5810
void Function4C5810(W8Forwarded* target)
{
    target->Method4C5290();
}

/* Acts only when both arguments are set, and passes the second on. */
// FUNCTION: WIZ8 0x004C59C0
void Function4C59C0(int enabled, int value)
{
    if (enabled != 0 && value != 0) {
        Function4A7A70(value);
    }
}

/* The first argument is dead: the list comes from the global, not the caller.
   Both are reproduced because the original takes and ignores it. */
// FUNCTION: WIZ8 0x0046E5A0
void Function46E5A0(int unused, void* item)
{
    PListRemove(*g_plist_659ab4, item);
}

}
