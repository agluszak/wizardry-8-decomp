#ifndef WIZ8_ENGINE_CODE_ANIM_OBJ_H
#define WIZ8_ENGINE_CODE_ANIM_OBJ_H

#include "wiz8/gameplay_boundaries.h"

/*
 * Engine Code\AnimObj.cpp.
 *
 * The allocator at 0x004A01A0 clears the complete 0x4c-byte record.  The
 * assertion name `pao` and the owning source path establish the AnimObj
 * identity; the members below remain positional until their consumers supply
 * semantic names.
 */
struct W8AnimObj {
    unsigned char unknown_00[5];
    unsigned char flag_05;               /* 0x05 */
    unsigned char unknown_06[0x10];
    unsigned char value_16;              /* 0x16 */
    unsigned char unknown_17;
    void* entries_18[4];                 /* 0x18 */
    W8PList* lists_28[3];                /* 0x28 */
    W8PList* lists_34[6];                /* 0x34 */
};                                       /* 0x4c */

static_assert(sizeof(W8AnimObj) == 0x4c, "W8AnimObj_size_must_be_0x4c");

W8AnimObj* CreateAnimObj004A01A0();
unsigned int AnimObjValue004A15D0(W8AnimObj* animation, signed char index);
unsigned int AnimObjListCount004A1620(W8AnimObj* animation, signed char index);
void* AnimObjListEntry004A16C0(
    W8AnimObj* animation, signed char list_index, signed char entry_index);
unsigned char AnimationIsRunning(W8AnimObj* animation);
int AnimObjDispatch004A14D0(
    W8AnimObj* animation, int unused_edx, signed char list_index, int value);
int AnimObjDispatchList004A1560(
    W8AnimObj* animation, int unused_edx, signed char list_index, signed char entry_index);
void* AnimObjEntry004A1660(
    W8AnimObj* animation, int unused_edx, signed char list_index, unsigned int entry_index);

#endif
