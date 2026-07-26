#include "gameplay_boundaries.h"
#include "sr_api.h"

/* Three thiscall methods invoked on each prop. Their names are not established;
   only their signatures are, from the call sites. */
struct W8Prop {
    void Method44D360(W8World* world);
    void Method44C030(void);
    void Method44C830(W8World* world);
};

// FUNCTION: WIZ8 0x0046DED0
// Source unit is Engine Code\3d.cpp; the assertion at line 344 is what names
// and types World::plsProps.
void WorldUpdateProps(W8World* world)
{
    int count;
    int index;
    W8Prop* prop;

    if (!world || !world->plsProps) {
        srAssertFail(
            "pWorld && pWorld->plsProps",
            "C:\\Projects\\Wizardry 8\\Engine Code\\3d.cpp",
            0x158,
            0);
    }
    count = (int)PListGetCount(world->plsProps);
    for (index = 0; index < count; index++) {
        prop = (W8Prop*)PListGetAt(world->plsProps, index);
        if (prop) {
            prop->Method44D360(world);
            prop->Method44C030();
            prop->Method44C830(world);
        }
    }
}
