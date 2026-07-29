#include "wiz8/gameplay_boundaries.h"

/* The concrete identity of the second owned object is unresolved. Its slot
   zero is nevertheless the compiler-generated scalar-deleting destructor, so
   model the known virtual lifetime contract and let a typed delete emit the
   call. */
struct W8Deletable {
    virtual ~W8Deletable() = 0;
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
        delete deletable;
        g_object_685067 = 0;
    }
}

}
