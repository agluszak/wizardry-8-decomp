#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define ANIM_OBJ_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AnimObj.cpp"

/* The callee returns its byte value in an int-sized result; this wrapper is
   the narrowing boundary, as shown by its explicit `and eax, 0xff`. */
extern int Function4B64F0(void* entry);                        /* 0x004B64F0 */
extern int Function4B6550(void* entry, int value);              /* 0x004B6550 */

struct W8AnimObjEntry004B6550 {
    unsigned char unknown_00[0x28];
    signed char list_index_28;
};

// FUNCTION: WIZ8 0x004a01a0
W8AnimObj* CreateAnimObj004A01A0()
{
    W8AnimObj* animation = (W8AnimObj*)malloc(sizeof(W8AnimObj));

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x24, 0);
    }
    memset(animation, 0, sizeof(W8AnimObj));
    return animation;
}

/* While the object is inactive the selected owned entry supplies the value;
   once active, AnimObj keeps the live value in its own byte at 0x16. */
// FUNCTION: WIZ8 0x004a15d0
unsigned int AnimObjValue004A15D0(W8AnimObj* animation, signed char index)
{
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x291, 0);
    }
    if (animation->flag_05 == 0) {
        return (unsigned char)Function4B64F0(animation->entries_18[index]);
    }
    return animation->value_16;
}

// FUNCTION: WIZ8 0x004a1620
unsigned int AnimObjListCount004A1620(W8AnimObj* animation, signed char index)
{
    W8PList* list;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x2a7, 0);
    }
    list = animation->lists_28[index];
    if (list != 0) {
        return PListGetCount(list);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a16c0
void* AnimObjListEntry004A16C0(
    W8AnimObj* animation, signed char list_index, signed char entry_index)
{
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x2d3, 0);
    }
    if (animation->flag_05 == 0) {
        return 0;
    }
    return PListGetAt(animation->lists_34[list_index], entry_index);
}

// FUNCTION: WIZ8 0x004a1dc0
unsigned char AnimationIsRunning(W8AnimObj* animation)
{
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x3b1, 0);
    }
    return animation->flag_05;
}

// FUNCTION: WIZ8 0x004a14d0
int AnimObjDispatch004A14D0(
    W8AnimObj* animation, int, signed char list_index, int value)
{
    W8AnimObjEntry004B6550* entry;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x243, 0);
    }
    if (animation->flag_05 == 0) {
        entry = (W8AnimObjEntry004B6550*)animation->entries_18[list_index];
        if (entry != (W8AnimObjEntry004B6550*)0xdddddddd && entry != 0) {
            entry->list_index_28 = list_index;
            return Function4B6550(entry, value);
        }
    }
    else {
        entry = (W8AnimObjEntry004B6550*)PListGetAt(animation->lists_28[list_index], 0);
        if (entry != 0) {
            entry->list_index_28 = list_index;
            return Function4B6550(entry, value);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a1560
int AnimObjDispatchList004A1560(
    W8AnimObj* animation, int, signed char list_index, signed char entry_index)
{
    W8AnimObjEntry004B6550* entry;
    W8PList* list;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x26c, 0);
    }
    if (animation->flag_05 == 1) {
        list = animation->lists_28[list_index];
        if (list != 0) {
            entry = (W8AnimObjEntry004B6550*)PListGetAt(list, entry_index);
            if (entry != 0) {
                entry->list_index_28 = list_index;
                return Function4B6550(entry, 0);
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a1660
void* AnimObjEntry004A1660(
    W8AnimObj* animation, int, signed char list_index, unsigned int entry_index)
{
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x2bd, 0);
    }
    if (animation->flag_05 == 0) {
        return animation->entries_18[list_index];
    }
    return PListGetAt(animation->lists_28[list_index], entry_index & 0xff);
}
