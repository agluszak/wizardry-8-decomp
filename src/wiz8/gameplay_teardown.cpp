#include "wiz8/gameplay_boundaries.h"

/* 0x0054B0B0 tears down two objects, and the second through its vtable. That
   call is a __thiscall virtual with its argument on the stack, which the
   __fastcall spelling used elsewhere in this unit cannot express: __fastcall
   would place the argument in EDX, and the canonical never writes EDX. A real
   virtual call is the only faithful spelling, so this body lives in C++ while
   the rest of the unit stays C.

   Slot 0 taking a byte flag is the scalar deleting destructor MSVC emits for a
   class with a virtual destructor; the flag asks it to free the object too. */
struct W8Deletable {
    virtual void ScalarDeletingDestructor(unsigned char free_storage) = 0;
};

extern "C" {

// FUNCTION: WIZ8 0x0054b0b0
void DestroyGameplayObjects(void)
{
    W8StartupRuntimeState* owned = g_startup_runtime_state;
    W8Deletable* deletable;

    if (owned) {
        owned->~W8StartupRuntimeState();
        operator delete(owned);
        g_startup_runtime_state = 0;
    }
    deletable = static_cast<W8Deletable*>(g_object_685067);
    if (deletable) {
        deletable->ScalarDeletingDestructor(1);
        g_object_685067 = 0;
    }
}

}
