#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/registry_classes.h"
#include "surrender/srModelInstance.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"

#include <stdlib.h>
#include <string.h>

#define ANIM_OBJ_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AnimObj.cpp"

/* The callee returns its byte value in an int-sized result; this wrapper is
   the narrowing boundary, as shown by its explicit `and eax, 0xff`. */
extern int Function4B64F0(void* entry);                        /* 0x004B64F0 */
extern const double g_anim_obj_world_scale_005ec150;
extern const float g_world_scale_005ebc40;

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

// FUNCTION: WIZ8 0x004a05c0
unsigned char AnimObjReadFromFile004A05C0(
    W8ReadLevelInfo* info,
    W8AnimObj* animation,
    int load_all,
    W8GrowableVector<stLight*>* light_list,
    int unused)
{
    unsigned char version = 0;
    unsigned char success;
    unsigned char discarded[50];
    unsigned char channel_bytes[8];
    int handle;
    int index;

    (void)unused;
    if (info == 0 || info->hFile == 0 || animation == 0) {
        srAssertFail("pInfo && pInfo->hFile && pao", ANIM_OBJ_CPP, 0xef, 0);
    }
    handle = info->hFile;
    success = ReadVirtualFile(handle, &version, 1, 0);
    success = success && ReadVirtualFile(handle, &animation->unknown_00[0], 1, 0);
    success = success && ReadVirtualFile(handle, &animation->unknown_00[1], 1, 0);
    success = success && ReadVirtualFile(handle, &animation->value_02, 1, 0);
    success = success && ReadVirtualFile(handle, &animation->unknown_03[0], 1, 0);
    success = success && ReadVirtualFile(handle, &animation->unknown_03[1], 1, 0);
    success = success && ReadVirtualFile(handle, &animation->flag_05, 1, 0);

    if (version < 3) {
        animation->playback_scale_08 = 15.0f;
    }
    else {
        success = success && ReadVirtualFile(
            handle, &animation->playback_scale_08, 4, 0);
    }
    if (version < 5) {
        animation->start_frame_14 = 0;
    }
    else {
        success = success && ReadVirtualFile(
            handle, &animation->start_frame_14, 1, 0);
    }
    if (version < 11) {
        animation->end_frame_15 = 0;
    }
    else {
        success = success && ReadVirtualFile(
            handle, &animation->end_frame_15, 1, 0);
    }
    if (version < 6) {
        animation->unknown_0c[0] = 0;
        animation->value_10 = 1.0f;
    }
    else {
        success = success && ReadVirtualFile(
            handle, &animation->unknown_0c[0], 1, 0);
        success = success && ReadVirtualFile(handle, &animation->value_10, 4, 0);
    }
    success = success && ReadVirtualFile(handle, discarded, sizeof(discarded), 0);
    if (!success) {
        srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x117, 0);
    }
    for (index = 0; index < (signed char)animation->unknown_00[0]; ++index) {
        success = success && ReadVirtualFile(handle, &channel_bytes[index], 1, 0);
    }

    if (version > 6) {
        unsigned char frames;
        ReadVirtualFile(handle, &frames, 1, 0);
        if (animation->pfKnownBBoxFrames == 0 && frames != 0) {
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
                    ANIM_OBJ_CPP, 300, 0);
            }
            memset(animation->pfKnownBBoxFrames, 1, frames);
            for (index = 0; index < frames; ++index) {
                ReadVirtualFile(handle, &animation->pvecBoundMin[index],
                                sizeof(srVector3T<float>), 0);
                ReadVirtualFile(handle, &animation->pvecBoundMax[index],
                                sizeof(srVector3T<float>), 0);
                animation->pvecBoundMin[index] *=
                    static_cast<float>(g_anim_obj_world_scale_005ec150);
                animation->pvecBoundMax[index] *=
                    static_cast<float>(g_anim_obj_world_scale_005ec150);
            }
        }
    }

    if (version > 7) {
        unsigned char light_count;
        srVector3T<float> color(1.0f, 1.0f, 1.0f);
        ReadVirtualFile(handle, &light_count, 1, 0);
        for (index = 0; index < (signed char)light_count; ++index) {
            unsigned char light_version;
            unsigned char definition_kind = 0;
            unsigned char ignored;
            srVector3T<float> position;
            float range;
            float intensity;
            stLightDefinition* definition = 0;

            ReadVirtualFile(handle, &light_version, 1, 0);
            ReadVirtualFile(handle, &position, sizeof(position), 0);
            ReadVirtualFile(handle, &color, sizeof(color), 0);
            ReadVirtualFile(handle, &intensity, 4, 0);
            ReadVirtualFile(handle, &range, 4, 0);
            position *= static_cast<float>(g_anim_obj_world_scale_005ec150);
            if (light_version < 3) {
                definition_kind = light_version == 2 ? 1 : 0;
            }
            else {
                ReadVirtualFile(handle, &definition_kind, 1, 0);
            }

            if (definition_kind == 1) {
                stLightDefinition005ECDBC* typed =
                    new stLightDefinition005ECDBC;
                ReadVirtualFile(handle, &typed->flags_08, 4, 0);
                ReadVirtualFile(handle, &typed->value_0c, 4, 0);
                ReadVirtualFile(handle, &typed->color_10, 12, 0);
                ReadVirtualFile(handle, &typed->value_1c, 4, 0);
                ReadVirtualFile(handle, &typed->value_20, 4, 0);
                ReadVirtualFile(handle, &typed->value_24, 4, 0);
                ReadVirtualFile(handle, &typed->intensity_28, 4, 0);
                ReadVirtualFile(handle, &typed->value_2c, 4, 0);
                ReadVirtualFile(handle, &typed->value_30, 4, 0);
                ReadVirtualFile(handle, &typed->value_34, 4, 0);
                ReadVirtualFile(handle, &typed->path_value_38, 4, 0);
                ReadVirtualFile(handle, &typed->value_3c, 4, 0);
                ReadVirtualFile(handle, &typed->value_40, 4, 0);
                definition = typed;
            }
            else if (definition_kind == 2) {
                stLightDefinition005ECDA0* typed =
                    new stLightDefinition005ECDA0;
                int previous = 0;
                int key;

                ReadVirtualFile(handle, &ignored, 1, 0);
                ReadVirtualFile(handle, &typed->value_50, 4, 0);
                ReadVirtualFile(handle, &typed->value_54, 4, 0);
                for (key = 0; key < 6; ++key) {
                    int frame;
                    float value;
                    srVector3T<float> vector;

                    ReadVirtualFile(handle, &frame, 4, 0);
                    ReadVirtualFile(handle, &value, 4, 0);
                    ReadVirtualFile(handle, &vector, sizeof(vector), 0);
                    if (frame < previous) {
                        frame = previous + 1;
                    }
                    if (typed->value_54 <= frame) {
                        frame = static_cast<int>(typed->value_54);
                    }
                    previous = frame;
                    typed->values_18.Add(frame);
                    typed->values_28.Add(value);
                    typed->values_38.Add(vector);
                    if (typed->values_18.count == 1) {
                        typed->values_08.Add(frame);
                    }
                    else {
                        typed->values_08.Add(
                            *typed->values_08.GetAt(typed->values_08.count - 1) +
                            frame);
                    }
                }
                definition = typed;
            }

            if (light_list == 0) {
                srAssertFail(
                    "0", ANIM_OBJ_CPP, 0x1b2,
                    "AnimObjReadFromFile: Where is the light list?");
            }
            else {
                stLight* light = CreateWorldLight0046E030(0, "MonsterLight");
                light->m_color_6c = color;
                light->m_position_78 = srVector3T<float>(0.0f, 0.0f, 0.0f);
                ConfigureWorldLight0046E300(light, range * g_world_scale_005ebc40);
                light->m_positional_98 = intensity;
                light->setLocation(position.x, position.y, position.z);
                light->m_positional_228 = position;
                light->m_definition_234 = definition;
                light->setGroupMask(2);
                light_list->Add(light);
            }
        }
    }

    if (animation->flag_05 == 0 && version > 8) {
        unsigned char has_path;
        ReadVirtualFile(handle, &has_path, 1, 0);
        if (has_path != 0) {
            W8PathAI* path = 0;
            success = LoadPathAI004A92A0(&path, handle);
            if (!success) {
                srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x1c5, 0);
            }
            path->flag_1c = 1;
            path->flag_3a = 0;
            path->value_2c = animation->playback_scale_08;
            animation->path_24 = path;
            animation->value_16 =
                static_cast<unsigned char>(path->nodes_0c->count);
        }
    }
    if (version > 9) {
        unsigned char ignored;
        success = success && ReadVirtualFile(handle, &ignored, 1, 0);
    }

    if (animation->flag_05 == 0) {
        int mesh_index;
        for (mesh_index = 0;
             mesh_index < (signed char)animation->unknown_00[0]; ++mesh_index) {
            W8AniMesh* mesh = CreateAniMesh004B57E0();
            signed char channel;

            success = success && ReadVirtualFile(handle, &channel, 1, 0);
            mesh->list_index_28 = channel;
            if (!LoadAniMeshFromInfo004B5B30(info, mesh, load_all)) {
                animation->entries_18[channel] = 0;
            }
            else {
                animation->entries_18[channel] = mesh;
            }
        }
    }
    else {
        int group;
        for (index = 0; index < 3; ++index) {
            animation->meshes_28[index] = PLCreate();
            animation->paths_34[index] = PLCreate();
        }
        for (group = 0;
             group < (signed char)animation->unknown_00[0]; ++group) {
            signed char entry_count;
            int entry;
            success = ReadVirtualFile(handle, &entry_count, 1, 0);
            if (!success) {
                srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x200, 0);
            }
            for (entry = 0; entry < entry_count; ++entry) {
                W8AniMesh* mesh = CreateAniMesh004B57E0();
                W8PathAI* path = 0;
                signed char channel;
                const char* saved_filename;

                if (!ReadVirtualFile(handle, &channel, 1, 0)) {
                    srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x208, 0);
                }
                mesh->list_index_28 = channel;
                saved_filename = info->mesh_filename;
                info->mesh_filename = 0;
                success = LoadAniMeshFromInfo004B5B30(info, mesh, 1);
                info->mesh_filename = saved_filename;
                if (!success) {
                    srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x20f, 0);
                }
                PListInsert(animation->meshes_28[channel], entry, mesh);
                success = LoadPathAI004A92A0(&path, handle);
                if (!success) {
                    srAssertFail("fSuccess", ANIM_OBJ_CPP, 0x217, 0);
                }
                PListInsert(animation->paths_34[channel], entry, path);
                path->flag_1c = 1;
                path->flag_3a = 0;
                path->value_2c = animation->playback_scale_08;
                animation->value_16 =
                    static_cast<unsigned char>(path->nodes_0c->count);
            }
        }
    }

    if (animation->end_frame_15 == 0) {
        if (animation->entries_18[2] != 0) {
            animation->end_frame_15 = static_cast<unsigned char>(
                AniMeshValue004B64F0(animation->entries_18[2]) - 1);
        }
        else if (animation->entries_18[1] != 0) {
            animation->end_frame_15 = static_cast<unsigned char>(
                AniMeshValue004B64F0(animation->entries_18[1]) - 1);
        }
        else if (animation->entries_18[0] != 0) {
            animation->end_frame_15 = static_cast<unsigned char>(
                AniMeshValue004B64F0(animation->entries_18[0]) - 1);
        }
        else if (animation->flag_05 == 0) {
            animation->end_frame_15 = animation->start_frame_14;
        }
        else {
            animation->end_frame_15 = animation->value_16 - 1;
        }
    }
    return success;
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
    stModelInstance* instance;
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

/* Type-two light definitions own four synchronized keyframe vectors.  The
   clone iterates the second vector's count, which is the retail authority for
   the shared extent, then copies the four scalar playback fields. */
// FUNCTION: WIZ8 0x004a2230
stLightDefinition* stLightDefinition005ECDA0::Clone() const
{
    stLightDefinition005ECDA0* copy = new stLightDefinition005ECDA0;
    int index;

    if (copy == 0) {
        srAssertFail("pNew", "..\\Engine Code\\Include\\stLight.hpp", 0x93, 0);
    }
    for (index = 0; index < values_18.GetCount(); ++index) {
        copy->values_08.Add(*values_08.GetAt(index));
        copy->values_18.Add(*values_18.GetAt(index));
        copy->values_28.Add(*values_28.GetAt(index));
        copy->values_38.Add(*values_38.GetAt(index));
    }
    copy->value_48 = value_48;
    copy->time_4c = time_4c;
    copy->value_50 = value_50;
    copy->value_54 = value_54;
    return copy;
}

// FUNCTION: WIZ8 0x004a2580
unsigned char stLightDefinition005ECDA0::IsEnabledForSubcycle(
    unsigned char subcycle)
{
    if (static_cast<float>(*values_18.GetAt(0)) <= time_4c &&
        time_4c <= static_cast<float>(
            *values_18.GetAt(values_18.GetCount() - 1))) {
        return 1;
    }
    return 0;
}

// SYNTHETIC: WIZ8 0x004a25c0
// stLightDefinition005ECDA0::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004a25e0
stLightDefinition005ECDA0::~stLightDefinition005ECDA0()
{
}
