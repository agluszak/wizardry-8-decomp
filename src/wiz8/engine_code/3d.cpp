#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Three thiscall methods invoked on each prop. Their names are not established;
   only their signatures are, from the call sites. */
struct W8Prop {
    void Method44D360(W8World* world);
    void Method44C030(void);
    void Method44C830(W8World* world);
};

// Source unit is Engine Code\3d.cpp; the assertion at line 344 is what names
// and types World::plsProps.
// FUNCTION: WIZ8 0x0046DED0
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

/* 0x00659AB4: the world being rendered, which the list wrappers below reach
   through. Every one of them ignores the caller's own first argument and uses
   this global instead. */
extern W8World* g_world_00659ab4;
extern void SetHeapFree(void* block);
extern void Function46E750(void* target, int argument);
extern void Function46E640(void* target, int argument);

/* Add to and remove from the world's two unnamed lists. The add on the first
   list has no matching remove here, which is what separates it from the
   second. */
// FUNCTION: WIZ8 0x0046E580
void WorldAddToList00(W8World* unused, void* entry)
{
    PListAdd(g_world_00659ab4->plsList00, entry);
}

// FUNCTION: WIZ8 0x0046E5C0
void WorldAddToList04(W8World* unused, void* entry)
{
    PListAdd(g_world_00659ab4->plsList04, entry);
}

// FUNCTION: WIZ8 0x0046E5E0
void WorldRemoveFromList04(W8World* unused, void* entry)
{
    PListRemove(g_world_00659ab4->plsList04, entry);
}

/* How many props the world holds, and the one at a position - both answers
   discarded by the wrapper itself, which is what makes these thin forwarders
   rather than accessors. */
// FUNCTION: WIZ8 0x0046E600
void WorldGetPropCount(void)
{
    PListGetCount(g_world_00659ab4->plsProps);
}

// FUNCTION: WIZ8 0x0046E620
void WorldGetPropAt(W8World* unused, int index)
{
    PListGetAt(g_world_00659ab4->plsProps, index);
}

/* Two wrappers that reach one member along before forwarding, which is what
   places that member at 0x3c of whatever they are called on. */
// FUNCTION: WIZ8 0x0046E860
void ForwardThroughMember3C_46E750(void* owner, int argument)
{
    Function46E750(*(void**)((char*)owner + 0x3c), argument);
}

// FUNCTION: WIZ8 0x0046E880
void ForwardThroughMember3C_46E640(void* owner, int argument)
{
    Function46E640(*(void**)((char*)owner + 0x3c), argument);
}

/* Release one block back to the renderer's heap rather than the CRT's. */
// FUNCTION: WIZ8 0x0046F3F0
void FreeThroughRenderHeap(void* block)
{
    SetHeapFree(block);
}

/* Walk a chain through its link at 0x134 and set the same field on every node
   of it. */
// FUNCTION: WIZ8 0x0046F4F0
void SetChainValue15C(char* node, int value)
{
    for (; node != 0; node = *(char**)(node + 0x134)) {
        *(int*)(node + 0x15c) = value;
    }
}
