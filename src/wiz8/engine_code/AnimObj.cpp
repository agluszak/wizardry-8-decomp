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

/* A deep copy of everything the record owns. Every mesh - the three entries and
   every entry of the first list group - is rebuilt through CopyAniMesh004B58D0,
   every path of the second group through ClonePathAI004A98C0, and each list
   itself is a fresh PList. The three trailing allocations are sized from the
   frame count, which comes from the third mesh entry unless flag_05 says to use
   value_16 instead; +0x40 is one byte a frame and the other two a
   srVector3T<float> each. */
// FUNCTION: WIZ8 0x004a0320
W8AnimObj* CloneAnimObj004A0320(const W8AnimObj* source)
{
    W8AnimObj* copy = (W8AnimObj*)malloc(sizeof(W8AnimObj));
    int index;
    int entry;
    int count;
    int frames;

    if (copy == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x24, 0);
    }
    memset(copy, 0, sizeof(W8AnimObj));
    copy->unknown_00[0] = source->unknown_00[0];
    copy->unknown_00[1] = source->unknown_00[1];
    copy->value_02 = source->value_02;
    copy->unknown_03[0] = source->unknown_03[0];
    copy->unknown_03[1] = source->unknown_03[1];
    copy->flag_05 = source->flag_05;
    copy->playback_scale_08 = source->playback_scale_08;
    copy->unknown_0c[0] = source->unknown_0c[0];
    copy->value_10 = source->value_10;
    copy->start_frame_14 = source->start_frame_14;
    copy->end_frame_15 = source->end_frame_15;
    copy->value_16 = source->value_16;
    if (source->entries_18[0] != 0) {
        copy->entries_18[0] = CopyAniMesh004B58D0(source->entries_18[0]);
    }
    if (source->entries_18[1] != 0) {
        copy->entries_18[1] = CopyAniMesh004B58D0(source->entries_18[1]);
    }
    if (source->entries_18[2] != 0) {
        copy->entries_18[2] = CopyAniMesh004B58D0(source->entries_18[2]);
    }
    for (index = 0; index < 3; ++index) {
        if (source->meshes_28[index] != 0) {
            copy->meshes_28[index] = PLCreate();
            count = (int)PLLength(source->meshes_28[index]);
            for (entry = 0; entry < count; ++entry) {
                PLAdoptAppend(copy->meshes_28[index],
                         CopyAniMesh004B58D0((W8AniMesh*)PLGet(
                             source->meshes_28[index], entry)));
            }
        }
    }
    for (index = 0; index < 3; ++index) {
        if (source->paths_34[index] != 0) {
            copy->paths_34[index] = PLCreate();
            count = (int)PLLength(source->paths_34[index]);
            for (entry = 0; entry < count; ++entry) {
                PLAdoptAppend(copy->paths_34[index],
                         ClonePathAI004A98C0((W8PathAI*)PLGet(
                             source->paths_34[index], entry)));
            }
        }
    }
    if (source->pfKnownBBoxFrames != 0) {
        if (source == 0) {
            srAssertFail("pao", ANIM_OBJ_CPP, 0x291, 0);
        }
        if (source->flag_05 == 0) {
            frames = Function4B64F0(source->entries_18[2]) & 0xff;
        }
        else {
            frames = source->value_16;
        }
        if (frames != 0) {
            copy->pfKnownBBoxFrames =
                static_cast<unsigned char*>(malloc(frames));
            copy->pvecBoundMin = static_cast<srVector3T<float>*>(
                srHeap.allocate(frames * sizeof(srVector3T<float>)));
            copy->pvecBoundMax = static_cast<srVector3T<float>*>(
                srHeap.allocate(frames * sizeof(srVector3T<float>)));
            for (index = 0; index < frames; ++index) {
                copy->pfKnownBBoxFrames[index] =
                    source->pfKnownBBoxFrames[index];
                copy->pvecBoundMin[index] = source->pvecBoundMin[index];
                copy->pvecBoundMax[index] = source->pvecBoundMax[index];
            }
        }
    }
    return copy;
}

/* The animation's bounding box for one frame, cached per frame once computed.
   The fast path is the cache; the single-mesh form defers to the mesh's own
   box; the per-frame form walks every mesh in the list, places each through its
   path, and takes the extent.

   The transformed box goes into locals the body never reads back - the merge
   below compares against the untransformed values instead. That is what the
   original does. */
// FUNCTION: WIZ8 0x004a1710
unsigned char AnimObjGetBounds004A1710(
    W8AnimObj* animation,
    int channel,
    signed char list_index,
    unsigned int frame,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum)
{
    srMatrix3T<float> rotation;
    srVector3T<float> translation;
    srVector3T<float> scale;
    srVector3T<float> transformed_minimum;
    srVector3T<float> transformed_maximum;
    srVector3T<float> mesh_minimum;
    srVector3T<float> mesh_maximum;
    stModelInstance005EC7D0* instance;
    W8AniMesh* mesh;
    W8PathAI* path;
    unsigned int count;
    int index;
    unsigned int frames;

    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x2ff, 0);
    }
    if (animation->pfKnownBBoxFrames != 0 &&
        animation->pfKnownBBoxFrames[frame & 0xff] != 0) {
        *minimum = animation->pvecBoundMin[frame & 0xff];
        *maximum = animation->pvecBoundMax[frame & 0xff];
        return 1;
    }
    if (animation->flag_05 == 0) {
        if (animation == 0) {
            srAssertFail("pao", ANIM_OBJ_CPP, 0x2bd, 0);
        }
        if (animation->flag_05 == 0) {
            mesh = animation->entries_18[list_index];
        }
        else {
            mesh = (W8AniMesh*)PLGet(
                animation->meshes_28[list_index], frame & 0xff);
        }
        return GetAniMeshBounds004B6640(mesh, minimum, maximum);
    }
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x291, 0);
    }
    if (animation->flag_05 == 0) {
        frames = AniMeshValue004B64F0(animation->entries_18[list_index]);
    }
    else {
        frames = animation->value_16;
    }
    if (animation->pfKnownBBoxFrames == 0) {
        animation->pfKnownBBoxFrames =
            static_cast<unsigned char*>(malloc(frames));
        animation->pvecBoundMin = static_cast<srVector3T<float>*>(
            srHeap.allocate(frames * sizeof(srVector3T<float>)));
        animation->pvecBoundMax = static_cast<srVector3T<float>*>(
            srHeap.allocate(frames * sizeof(srVector3T<float>)));
        if (animation->pfKnownBBoxFrames == 0 ||
            animation->pvecBoundMin == 0 || animation->pvecBoundMax == 0) {
            srAssertFail(
                "pao->pfKnownBBoxFrames && pao->pvecBoundMin && "
                "pao->pvecBoundMax",
                ANIM_OBJ_CPP, 0x31e, 0);
        }
        memset(animation->pfKnownBBoxFrames, 0, frames);
        for (index = 0; (unsigned int)index < frames; ++index) {
            animation->pvecBoundMin[index].x = 0.0f;
            animation->pvecBoundMin[index].y = 0.0f;
            animation->pvecBoundMin[index].z = 0.0f;
            animation->pvecBoundMax[index].x = 0.0f;
            animation->pvecBoundMax[index].y = 0.0f;
            animation->pvecBoundMax[index].z = 0.0f;
        }
    }
    if (animation == 0) {
        srAssertFail("pao", ANIM_OBJ_CPP, 0x2a7, 0);
    }
    if (animation->meshes_28[list_index] == 0) {
        count = 0;
    }
    else {
        count = PLLength(animation->meshes_28[list_index]);
    }
    if (PLGet(animation->meshes_28[list_index], 0) == 0) {
        return 0;
    }
    GetAniMeshBounds004B6640(
        (W8AniMesh*)PLGet(animation->meshes_28[list_index], 0),
        minimum, maximum);
    for (index = 0; (unsigned int)index < count; ++index) {
        if (index != 0) {
            GetAniMeshBounds004B6640(
                (W8AniMesh*)PLGet(
                    animation->meshes_28[list_index], index),
                &mesh_minimum, &mesh_maximum);
        }
        if (animation == 0) {
            srAssertFail("pao", ANIM_OBJ_CPP, 0x26c, 0);
        }
        if (animation->flag_05 == 1 &&
            animation->meshes_28[list_index] != 0 &&
            (mesh = (W8AniMesh*)PLGet(
                 animation->meshes_28[list_index],
                 (signed char)index)) != 0) {
            mesh->list_index_28 = list_index;
            instance = GetAniMeshFrame004B6550(mesh, 0);
        }
        else {
            instance = 0;
        }
        if (animation == 0) {
            srAssertFail("pao", ANIM_OBJ_CPP, 0x2d3, 0);
        }
        if (animation->flag_05 != 0 &&
            (path = (W8PathAI*)PLGet(
                 animation->paths_34[list_index],
                 (signed char)index)) != 0) {
            float saved = PathAIGetValue004A9E70(path);

            PathAISetValue004A9F60(path, (float)index);
            PathAIApply004AA520(path, instance);
            PathAISetValue004A9F60(path, saved);
        }
        ((srNode*)instance)->getRotation(rotation);
        translation.x = (float)((srNode*)instance)->getLocation().x;
        translation.y = (float)((srNode*)instance)->getLocation().y;
        translation.z = (float)((srNode*)instance)->getLocation().z;
        scale.x = (float)((srNode*)instance)->getScale().x;
        scale.y = (float)((srNode*)instance)->getScale().y;
        scale.z = (float)((srNode*)instance)->getScale().z;
        transformed_minimum = *minimum;
        transformed_maximum = *maximum;
        TransformBounds004A1DF0(&rotation, &translation, &scale,
                                &transformed_minimum, &transformed_maximum);
        if (index != 0) {
            if (mesh_minimum.x < minimum->x) {
                minimum->x = mesh_minimum.x;
            }
            if (mesh_minimum.y < minimum->y) {
                minimum->y = mesh_minimum.y;
            }
            if (mesh_minimum.z < minimum->z) {
                minimum->z = mesh_minimum.z;
            }
            if (maximum->x < mesh_maximum.x) {
                maximum->x = mesh_maximum.x;
            }
            if (maximum->y < mesh_maximum.y) {
                maximum->y = mesh_maximum.y;
            }
            if (maximum->z < mesh_maximum.z) {
                maximum->z = mesh_maximum.z;
            }
        }
    }
    animation->pvecBoundMin[frame & 0xff] = *minimum;
    animation->pvecBoundMax[frame & 0xff] = *maximum;
    animation->pfKnownBBoxFrames[frame & 0xff] = 1;
    return 1;
}

/* Rotate, translate and scale the eight corners of a box and take the extent of
   the result. The first corner seeds both ends rather than starting from an
   infinite box, which is why the loop carries the is-first test instead of
   pre-filling. */
// FUNCTION: WIZ8 0x004a1df0
void TransformBounds004A1DF0(
    const srMatrix3T<float>* rotation,
    const srVector3T<float>* translation,
    const srVector3T<float>* scale,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum)
{
    float corner[6];
    int i;
    int j;
    int k;

    corner[0] = minimum->x;
    corner[1] = minimum->y;
    corner[2] = minimum->z;
    corner[3] = maximum->x;
    corner[5] = maximum->z;
    corner[4] = maximum->y;
    for (i = 0; i < 2; ++i) {
        for (j = 0; j < 2; ++j) {
            for (k = 0; k < 2; ++k) {
                float x = corner[i * 3];
                float y = corner[j * 3 + 1];
                float z = corner[k * 3 + 2];
                float tx = (x * rotation->vectors[0].x +
                            y * rotation->vectors[0].y +
                            z * rotation->vectors[0].z + translation->x) *
                           scale->x;
                float ty = (x * rotation->vectors[1].x +
                            y * rotation->vectors[1].y +
                            z * rotation->vectors[1].z + translation->y) *
                           scale->y;
                float tz = (y * rotation->vectors[2].y +
                            z * rotation->vectors[2].z +
                            x * rotation->vectors[2].x + translation->z) *
                           scale->z;

                if (i == 0 && j == 0 && k == 0) {
                    maximum->x = tx;
                    maximum->y = ty;
                    maximum->z = tz;
                    minimum->x = tx;
                    minimum->y = ty;
                    minimum->z = tz;
                }
                else {
                    if (tx < minimum->x) {
                        minimum->x = tx;
                    }
                    if (ty < minimum->y) {
                        minimum->y = ty;
                    }
                    if (tz < minimum->z) {
                        minimum->z = tz;
                    }
                    if (maximum->x < tx) {
                        maximum->x = tx;
                    }
                    if (maximum->y < ty) {
                        maximum->y = ty;
                    }
                    if (maximum->z < tz) {
                        maximum->z = tz;
                    }
                }
            }
        }
    }
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

    if (animation->pfKnownBBoxFrames != 0) {
        free(animation->pfKnownBBoxFrames);
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
                count = (int)PLLength(meshes);
                for (entry = 0; entry < count; ++entry) {
                    W8AniMesh* mesh =
                        (W8AniMesh*)PLGet(meshes, entry);

                    if (mesh != 0) {
                        DestroyAniMesh004B5880(mesh);
                    }
                    count = (int)PLLength(meshes);
                }
                PLDestroy(meshes);
            }
            paths = animation->paths_34[index];
            if (paths != 0) {
                count = (int)PLLength(paths);
                for (entry = 0; entry < count; ++entry) {
                    W8PathAI* path = (W8PathAI*)PLGet(paths, entry);

                    if (path != 0) {
                        DestroyPathAI004A9810(path);
                    }
                    count = (int)PLLength(paths);
                }
                PLDestroy(paths);
            }
        }
    }
    if (animation->pvecBoundMin != 0) {
        srHeap.free(animation->pvecBoundMin);
    }
    if (animation->pvecBoundMax != 0) {
        srHeap.free(animation->pvecBoundMax);
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
        return PLLength(list);
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
    return PLGet(animation->paths_34[list_index], entry_index);
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
        entry = (W8AniMesh*)PLGet(animation->meshes_28[list_index], 0);
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
            entry = (W8AniMesh*)PLGet(list, entry_index);
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
    return PLGet(animation->meshes_28[list_index], entry_index & 0xff);
}
