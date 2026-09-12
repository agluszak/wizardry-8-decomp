#ifndef WIZ8_ENGINE_CODE_ANIM_OBJ_H
#define WIZ8_ENGINE_CODE_ANIM_OBJ_H

#include "wiz8/3d_code/PList.h"
#include "surrender/srMath.h"

struct W8AniMesh;
struct W8PathAI;
struct W8ReadLevelInfo;
class stLight;
template <class T> class W8GrowableVector;
class srModelInstance;

/*
 * Engine Code\AnimObj.cpp.
 *
 * The allocator at 0x004A01A0 clears the complete 0x4c-byte record.  The
 * assertion name `pao` and the owning source path establish the AnimObj
 * identity. Leading serialized bytes are named from load/clone/dispatch
 * consumers; remaining bytes stay positional until those uses are recovered.
 */
struct W8AnimObj {
    unsigned char group_count; /* 0x00: mesh/list group count, max 3 */
    unsigned char unknown_01;  /* 0x01: copied onto monster/prop flag_06d */
    unsigned char value_02;    /* 0x02: copied onto flag_06f */
    unsigned char unknown_03;  /* 0x03: copied onto flag_070 */
    unsigned char cycle;       /* 0x04: default cycle/emitter index */
    unsigned char flag_05;     /* 0x05: 0 = mesh entries, nonzero = path lists */
    unsigned char unknown_06[2];
    float playback_scale_08; /* 0x08 */
    unsigned char flag_0c;   /* 0x0c: copied onto prop flag_0a5 */
    unsigned char unknown_0d[3];
    /* Serialized as a float; 0x004A0320 copies its four-byte representation. */
    float value_10; /* 0x10 */
    unsigned char start_frame_14;
    unsigned char end_frame_15;
    unsigned char value_16; /* 0x16 */
    unsigned char unknown_17;
    W8AniMesh* entries_18[3]; /* 0x18 */
    W8PathAI* path_24;        /* 0x24 */
    /* Six lists in two groups of three, not nine. The first group's entries are
       meshes, released the same way; the second group's are paths, released
       through DestroyPathAI004A9810. */
    W8PList* meshes_28[3]; /* 0x28 */
    W8PList* paths_34[3];  /* 0x34 */
    /* Named by 0x004A1710's own assertion,
       "pao->pfKnownBBoxFrames && pao->pvecBoundMin && pao->pvecBoundMax".
       One byte a frame saying whether that frame's bounds are already known,
       and the cached minimum and maximum for it. The flags come from malloc,
       the two vectors from srHeap. */
    unsigned char* pfKnownBBoxFrames; /* 0x40 */
    srVector3T<float>* pvecBoundMin;  /* 0x44 */
    srVector3T<float>* pvecBoundMax;  /* 0x48 */
}; /* 0x4c */

static_assert(sizeof(W8AnimObj) == 0x4c, "W8AnimObj_size_must_be_0x4c");

W8AnimObj* CreateAnimObj004A01A0();
void DestroyAnimObj004A01E0(W8AnimObj* animation);
W8AnimObj* CloneAnimObj004A0320(const W8AnimObj* source);
void TransformBounds004A1DF0(const srMatrix3T<float>* rotation,
                             const srVector3T<float>* translation, const srVector3T<float>* scale,
                             srVector3T<float>* minimum, srVector3T<float>* maximum);
unsigned int AnimObjValue004A15D0(W8AnimObj* animation, signed char index);
unsigned int AnimObjListCount004A1620(W8AnimObj* animation, signed char index);
W8PathAI* AnimObjListEntry004A16C0(W8AnimObj* animation, signed char list_index,
                                   signed char entry_index);
unsigned char AnimationIsRunning(W8AnimObj* animation);
unsigned char AnimObjReadFromFile004A05C0(W8ReadLevelInfo* info, W8AnimObj* animation, int load_all,
                                          W8GrowableVector<stLight*>* light_list, int unused);
srModelInstance* AnimObjDispatch004A14D0(W8AnimObj* animation, signed char list_index,
                                         unsigned char value);
srModelInstance* AnimObjDispatchList004A1560(W8AnimObj* animation, signed char list_index,
                                             signed char entry_index);
void* AnimObjEntry004A1660(W8AnimObj* animation, signed char list_index, unsigned int entry_index);
unsigned char AnimObjGetBounds004A1710(W8AnimObj* animation, signed char list_index,
                                       unsigned int frame, srVector3T<float>* minimum,
                                       srVector3T<float>* maximum);

#endif
