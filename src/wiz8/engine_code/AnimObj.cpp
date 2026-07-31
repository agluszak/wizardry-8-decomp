#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/registry_classes.h"
#include "surrender/srModelInstance.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define ANIM_OBJ_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AnimObj.cpp"

/* The callee returns its byte value in an int-sized result; this wrapper is
   the narrowing boundary, as shown by its explicit `and eax, 0xff`. */
extern int Function4B64F0(void* entry);                        /* 0x004B64F0 */
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

/* Everything an animation owns. The three mesh entries and the first list group
   go back through the AniMesh destroy, the second list group through the PathAI
   one, so the two groups hold different things - which is what the two release
   calls establish. The lists themselves are destroyed with their contents; the
   three trailing allocations are not lists at all and use two different
   allocators. */
// FUNCTION: WIZ8 0x004a01e0
void DestroyAnimObj004A01E0(W8AnimObj* animation)
{
    int index;
    int entry;
    int count;

    if (animation->allocation_40 != 0) {
        free(animation->allocation_40);
    }
    if (animation->entries_18 != 0) {
        if (animation->entries_18[0] != 0) {
            DestroyAniMesh004B5880(animation->entries_18[0]);
        }
        if (animation->entries_18[1] != 0) {
            DestroyAniMesh004B5880(animation->entries_18[1]);
        }
        if (animation->entries_18[2] != 0) {
            DestroyAniMesh004B5880(animation->entries_18[2]);
        }
    }
    if (animation->meshes_28 != 0) {
        for (index = 0; index < 3; ++index) {
            W8PList* meshes = animation->meshes_28[index];
            W8PList* paths;

            if (meshes != 0) {
                count = (int)PListGetCount(meshes);
                for (entry = 0; entry < count; ++entry) {
                    W8AniMesh* mesh =
                        (W8AniMesh*)PListGetAt(meshes, entry);

                    if (mesh != 0) {
                        DestroyAniMesh004B5880(mesh);
                    }
                    count = (int)PListGetCount(meshes);
                }
                PListDestroy(meshes);
            }
            paths = animation->paths_34[index];
            if (paths != 0) {
                count = (int)PListGetCount(paths);
                for (entry = 0; entry < count; ++entry) {
                    W8PathAI* path = (W8PathAI*)PListGetAt(paths, entry);

                    if (path != 0) {
                        DestroyPathAI004A9810(path);
                    }
                    count = (int)PListGetCount(paths);
                }
                PListDestroy(paths);
            }
        }
    }
    if (animation->allocation_44 != 0) {
        srHeap.free(animation->allocation_44);
    }
    if (animation->allocation_48 != 0) {
        srHeap.free(animation->allocation_48);
    }
    free(animation);
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
    list = animation->meshes_28[index];
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
    return PListGetAt(animation->paths_34[list_index], entry_index);
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
srModelInstance* AnimObjDispatch004A14D0(
    W8AnimObj* animation, signed char list_index, unsigned char value)
{
    W8AniMesh* entry;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x243, 0);
    }
    if (animation->flag_05 == 0) {
        entry = (W8AniMesh*)animation->entries_18[list_index];
        if (entry != (W8AniMesh*)0xdddddddd && entry != 0) {
            entry->list_index_28 = list_index;
            return GetAniMeshFrame004B6550(entry, value);
        }
    }
    else {
        entry = (W8AniMesh*)PListGetAt(animation->meshes_28[list_index], 0);
        if (entry != 0) {
            entry->list_index_28 = list_index;
            return GetAniMeshFrame004B6550(entry, value);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a1560
srModelInstance* AnimObjDispatchList004A1560(
    W8AnimObj* animation, signed char list_index, signed char entry_index)
{
    W8AniMesh* entry;
    W8PList* list;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x26c, 0);
    }
    if (animation->flag_05 == 1) {
        list = animation->meshes_28[list_index];
        if (list != 0) {
            entry = (W8AniMesh*)PListGetAt(list, entry_index);
            if (entry != 0) {
                entry->list_index_28 = list_index;
                return GetAniMeshFrame004B6550(entry, 0);
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
    return PListGetAt(animation->meshes_28[list_index], entry_index & 0xff);
}
