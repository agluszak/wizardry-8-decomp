#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/sr_api.h"
#include "surrender/srScene.h"

#define THREE_D_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\3d.cpp"

/* Three thiscall methods invoked on each prop. Their names are not established;
   only their signatures are, from the call sites. */
struct W8Prop {
    void Method44D360(W8World* world);
    void Method44C030(void);
    void Method44C830(W8World* world);
};

// Source unit is Engine Code\3d.cpp; the assertion at line 344 is what names
// and types World::plsProps.
// FUNCTION: WIZ8 0x0046ded0
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
extern void Function4836A0(void);
extern void SetHeapFree(void* block);
extern void Function46E750(void* target, int argument);
extern void Function46E640(void* target, int argument);

/* Add to and remove from the world's two unnamed lists. The add on the first
   list has no matching remove here, which is what separates it from the
   second. */
// FUNCTION: WIZ8 0x0046e580
void WorldAddToList00(W8World* unused, void* entry)
{
    PListAdd(g_world_00659ab4->plsMonsters, entry);
}

// FUNCTION: WIZ8 0x0046e5c0
void WorldAddToList04(W8World* unused, void* entry)
{
    PListAdd(g_world_00659ab4->plsItems, entry);
}

// FUNCTION: WIZ8 0x0046e5e0
void WorldRemoveFromList04(W8World* unused, void* entry)
{
    PListRemove(g_world_00659ab4->plsItems, entry);
}

/* How many props the world holds, and the one at a position - both answers
   discarded by the wrapper itself, which is what makes these thin forwarders
   rather than accessors. */
// FUNCTION: WIZ8 0x0046e600
void WorldGetPropCount(void)
{
    PListGetCount(g_world_00659ab4->plsProps);
}

// FUNCTION: WIZ8 0x0046e620
void WorldGetPropAt(W8World* unused, int index)
{
    PListGetAt(g_world_00659ab4->plsProps, index);
}

/* Two wrappers that reach one member along before forwarding, which is what
   places that member at 0x3c of whatever they are called on. */
// FUNCTION: WIZ8 0x0046e860
void ForwardThroughMember3C_46E750(void* owner, int argument)
{
    Function46E750(*(void**)((char*)owner + 0x3c), argument);
}

// FUNCTION: WIZ8 0x0046e880
void ForwardThroughMember3C_46E640(void* owner, int argument)
{
    Function46E640(*(void**)((char*)owner + 0x3c), argument);
}

/* Release one block back to the renderer's heap rather than the CRT's. */
// FUNCTION: WIZ8 0x0046f3f0
void FreeThroughRenderHeap(void* block)
{
    SetHeapFree(block);
}

/* Walk a chain through its link at 0x134 and set the same field on every node
   of it. */
// FUNCTION: WIZ8 0x0046f4f0
void SetChainValue15C(char* node, int value)
{
    for (; node != 0; node = *(char**)(node + 0x134)) {
        *(int*)(node + 0x15c) = value;
    }
}

/* The world's own float pair at 0x74 and 0x78. Both are assigned from the one
   argument, and only zero is rejected, so the guard is a "leave it alone"
   rather than a range check. The assertion at 3d.cpp:651 is what names the
   receiver pWorld. */
// FUNCTION: WIZ8 0x0046e350
void WorldSetValue74(W8World* world, float value)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x28b, 0);
    }
    if (value != 0.0f) {
        world->value_74 = value;
        world->value_78 = value;
        MarkRendererReady();
    }
}

/* Reads back only the second of the pair, which is what makes 0x78 the live
   copy and 0x74 the one nothing here consumes. */
// FUNCTION: WIZ8 0x0046e3a0
float WorldGetValue78(W8World* world)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x297, 0);
    }
    return world->value_78;
}

/* Pushes the world's view distance into the camera as the far clip plane; the
   near plane is the fixed 62.5 the original materialises inline. The guard is
   against a denormal-scale epsilon rather than zero, so a distance that has
   collapsed leaves the camera as it was. */
// FUNCTION: WIZ8 0x0046e3d0
void WorldSetFarClip(W8World* world, float distance)
{
    if (!world) {
        srAssertFail("pWorld", THREE_D_CPP, 0x29e, 0);
    }
    if ((double)distance > 5.9604644775390625e-008) {
        world->camera->setClipRange(62.5, (double)distance);
        Function4836A0();
        MarkRendererReady();
    }
}

/* The matching reader. Both planes come back, and only the far one is
   returned, which is the same asymmetry the setter has. A world without a
   camera answers zero rather than reading through it. */
// FUNCTION: WIZ8 0x0046e440
double WorldGetFarClip(W8World* world)
{
    double near_plane = 0;
    double far_plane = 0;

    if (world != 0 && world->camera != 0) {
        world->camera->getClipRange(&near_plane, &far_plane);
    }
    return far_plane;
}

extern "C" {
// FUNCTION: WIZ8 0x0046E5A0
void Function46E5A0(int unused, void* item)
{
    PListRemove(g_world_00659ab4->plsMonsters, item);
}
}
