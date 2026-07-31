#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "wiz8/3d_code/PList.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANI_MESH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AniMesh.cpp"

extern stModelInstance005EC7D0* DuplicateModelInstance0046F680(
    stModelInstance005EC7D0* instance);
extern unsigned char Function4B5D00(int file, W8AniMesh* mesh, int load);
extern void ExpandBounds0046F510(
    srVector3T<float>* minimum,
    srVector3T<float>* maximum,
    const srVector3T<float>* candidate_minimum,
    const srVector3T<float>* candidate_maximum);
extern double g_double_005ebe80;
extern float g_float_005ebb34;
extern "C" int g_storage_state_65be80;
extern "C" int g_storage_state_65be84;
extern "C" int g_storage_limit_65be88;
extern "C" W8PList g_storage_list_65be90;

// FUNCTION: WIZ8 0x004b57e0
W8AniMesh* CreateAniMesh004B57E0()
{
    W8AniMesh* mesh = static_cast<W8AniMesh*>(malloc(sizeof(W8AniMesh)));

    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x72, 0);
    }
    memset(mesh, 0, sizeof(W8AniMesh));
    mesh->bitmap_directory_2c = static_cast<char*>(malloc(0x400));
    mesh->filename_30 = static_cast<char*>(malloc(0x80));
    if (mesh->bitmap_directory_2c == 0) {
        srAssertFail("pAniMesh->strBitmapDir", ANI_MESH_CPP, 0x7a, 0);
    }
    if (mesh->filename_30 == 0) {
        srAssertFail("pAniMesh->strFilename", ANI_MESH_CPP, 0x7b, 0);
    }
    mesh->bitmap_directory_2c[0] = '\0';
    mesh->filename_30[0] = '\0';
    return mesh;
}

// FUNCTION: WIZ8 0x004b58d0
W8AniMesh* CopyAniMesh004B58D0(const W8AniMesh* other)
{
    W8AniMesh* mesh;
    unsigned int frame;

    if (other == 0) {
        srAssertFail("pOther", ANI_MESH_CPP, 0xb0, 0);
    }
    mesh = static_cast<W8AniMesh*>(malloc(sizeof(W8AniMesh)));
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0xb4, 0);
    }
    memset(mesh, 0, sizeof(W8AniMesh));
    mesh->flags_00 = other->flags_00;
    mesh->frame_count_01 = other->frame_count_01;
    memcpy(mesh->vector_08, other->vector_08, sizeof(mesh->vector_08));
    memcpy(mesh->vector_14, other->vector_14, sizeof(mesh->vector_14));
    mesh->radius_20 = other->radius_20;
    mesh->loaded_bytes_24 = other->loaded_bytes_24;
    mesh->list_index_28 = other->list_index_28;
    mesh->file_offset_34 = other->file_offset_34;
    mesh->load_value_38 = other->load_value_38;
    mesh->last_used_3c = other->last_used_3c;

    mesh->bitmap_directory_2c = static_cast<char*>(malloc(0x400));
    mesh->filename_30 = static_cast<char*>(malloc(0x80));
    if (mesh->bitmap_directory_2c == 0) {
        srAssertFail("pAniMesh->strBitmapDir", ANI_MESH_CPP, 0xc6, 0);
    }
    if (mesh->filename_30 == 0) {
        srAssertFail("pAniMesh->strFilename", ANI_MESH_CPP, 0xc7, 0);
    }
    strcpy(mesh->bitmap_directory_2c, other->bitmap_directory_2c);
    strcpy(mesh->filename_30, other->filename_30);

    if ((other->flags_00 & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        mesh->flags_00 |= W8_ANI_MESH_SINGLE_INSTANCE;
        mesh->meshes_04 = static_cast<stModelInstance005EC7D0**>(malloc(sizeof(*mesh->meshes_04)));
        if (mesh->meshes_04 == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0xd4, 0);
        }
        mesh->meshes_04[0] = DuplicateModelInstance0046F680(other->meshes_04[0]);
        mesh->meshes_04[0]->setName("Ani Mesh Duplicate Instance");
        return mesh;
    }

    mesh->meshes_04 = static_cast<stModelInstance005EC7D0**>(
        malloc(mesh->frame_count_01 * sizeof(*mesh->meshes_04)));
    if (mesh->meshes_04 == 0) {
        srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0xde, 0);
    }
    memset(mesh->meshes_04, 0, mesh->frame_count_01 * sizeof(*mesh->meshes_04));
    for (frame = 0; frame < mesh->frame_count_01; ++frame) {
        if (other->meshes_04 != 0 && other->meshes_04[frame] != 0) {
            mesh->meshes_04[frame] = DuplicateModelInstance0046F680(other->meshes_04[frame]);
            mesh->meshes_04[frame]->setName("Ani Mesh Duplicate Instance");
        }
    }
    return mesh;
}

// FUNCTION: WIZ8 0x004b5c10
float GetAniMeshFrameRadius004B5C10(W8AniMesh* mesh, unsigned char frame)
{
    stModelInstance005EC7D0* instance = GetAniMeshFrame004B6550(mesh, frame);

    if (instance != 0) {
        stMeshModel* model = static_cast<stMeshModel*>(instance->model());

        if (model != 0) {
            srVector3T<float> minimum = {0.0f, 0.0f, 0.0f};
            srVector3T<float> maximum = {0.0f, 0.0f, 0.0f};

            do {
                srVector3T<float> model_minimum;
                srVector3T<float> model_maximum;

                model->getBoundingBox(model_minimum, model_maximum);
                ExpandBounds0046F510(
                    &minimum, &maximum, &model_minimum, &model_maximum);
                model = model->next;
            } while (model != 0);

            float x = minimum.x - maximum.x;
            float y = minimum.y - maximum.y;
            float z = minimum.z - maximum.z;
            return static_cast<float>(sqrt(x * x + y * y + z * z) *
                                      g_double_005ebe80);
        }
    }
    return g_float_005ebb34;
}

// FUNCTION: WIZ8 0x004b5880
void DestroyAniMesh004B5880(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x91, 0);
    }
    if ((mesh->flags_00 & W8_ANI_MESH_LOADED) != 0) {
        UnloadAniMesh004B63F0(mesh, 1);
    }
    free(mesh->bitmap_directory_2c);
    free(mesh->filename_30);
    free(mesh);
}

// FUNCTION: WIZ8 0x004b63f0
unsigned char UnloadAniMesh004B63F0(W8AniMesh* mesh, unsigned char force)
{
    unsigned char frame_count;
    unsigned int frame;

    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x28a, 0);
        return 0;
    }
    if (force == 0 && (mesh->flags_00 & W8_ANI_MESH_KEEP_LOADED) == 0) {
        return 0;
    }
    if ((mesh->flags_00 & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        mesh->meshes_04[0]->release();
    } else {
        if ((mesh->flags_00 & W8_ANI_MESH_FRAME_COUNT_LOADED) == 0) {
            if (Function4B5D00(0, mesh, 1) == 0) {
                srAssertFail("0", ANI_MESH_CPP, 0x2c6, 0);
                frame_count = 0xff;
            } else {
                frame_count = mesh->frame_count_01;
            }
        } else {
            frame_count = mesh->frame_count_01;
        }
        for (frame = 0; frame < frame_count; ++frame) {
            stModelInstance005EC7D0* instance = GetAniMeshFrame004B6550(mesh, frame);

            instance->setParent(0, 1);
            instance->setFlag(srNode::FLAG_POSITIONAL_0);
            instance->release();
        }
    }
    g_storage_state_65be84 -= mesh->loaded_bytes_24;
    free(mesh->meshes_04);
    mesh->flags_00 &= ~W8_ANI_MESH_LOADED;
    mesh->meshes_04 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004b6550
stModelInstance005EC7D0* GetAniMeshFrame004B6550(
    W8AniMesh* mesh, unsigned char frame)
{
    stModelInstance005EC7D0* instance;
    char message[0x80];

    if (mesh == 0 || frame >= mesh->frame_count_01) {
        sprintf(message, "AniMeshGetMeshForFrame error: frame %d, num frames %d",
                frame, mesh->frame_count_01);
        srAssertFail("0", ANI_MESH_CPP, 0x2e6, message);
        return 0;
    }
    if ((mesh->flags_00 & W8_ANI_MESH_LOADED) == 0 &&
        Function4B5D00(0, mesh, 1) == 0) {
        srAssertFail("0", ANI_MESH_CPP, 0x2ee, 0);
        return 0;
    }
    if ((mesh->flags_00 & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        instance = mesh->meshes_04[0];
        instance->frame_index_180 = frame;
    } else {
        instance = mesh->meshes_04[frame];
    }
    mesh->last_used_3c = g_storage_state_65be80;
    ++g_storage_state_65be80;
    return instance;
}

// FUNCTION: WIZ8 0x004b64f0
unsigned char AniMeshValue004B64F0(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x2be, 0);
        return 0xff;
    }
    if ((mesh->flags_00 & W8_ANI_MESH_FRAME_COUNT_LOADED) == 0) {
        if (Function4B5D00(0, mesh, 1) == 0) {
            srAssertFail("0", ANI_MESH_CPP, 0x2c6, 0);
            return 0xff;
        }
    }
    return mesh->frame_count_01;
}

// FUNCTION: WIZ8 0x004b66e0
unsigned char AniMeshRadius004B66E0(W8AniMesh* mesh, float* radius)
{
    if (mesh == 0 || radius == 0) {
        srAssertFail("pAniMesh&&pflRadius", ANI_MESH_CPP, 0x348, 0);
    }
    if (mesh != 0 && radius != 0) {
        if ((mesh->flags_00 & W8_ANI_MESH_RADIUS_LOADED) == 0) {
            if (Function4B5D00(0, mesh, 1) == 0) {
                srAssertFail("0", ANI_MESH_CPP, 0x350, 0);
                return 0;
            }
        }
        *radius = mesh->radius_20;
        mesh->last_used_3c = g_storage_state_65be80;
        ++g_storage_state_65be80;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004b6860
void AniMeshSetFlag10004B6860(W8AniMesh* mesh, int, signed char enabled)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x3b7, 0);
    }
    if (enabled != 0) {
        mesh->flags_00 |= W8_ANI_MESH_FLAG_10;
        return;
    }
    mesh->flags_00 &= ~W8_ANI_MESH_FLAG_10;
}

// FUNCTION: WIZ8 0x004b6770
void EnforceAniMeshMemoryLimit004B6770(W8AniMesh* current)
{
    while (g_storage_state_65be84 > g_storage_limit_65be88 &&
           PListGetCount(&g_storage_list_65be90) != 0) {
        W8AniMesh* oldest = 0;
        int oldest_index = -1;
        unsigned int count = PListGetCount(&g_storage_list_65be90);

        for (unsigned int index = 0; index < count; ++index) {
            W8AniMesh* candidate = static_cast<W8AniMesh*>(
                PListGetAt(&g_storage_list_65be90, index));

            if (candidate != current) {
                if (candidate == 0) {
                    srAssertFail("pAniMesh", ANI_MESH_CPP, 0x3d0, 0);
                }
                if ((candidate->flags_00 & W8_ANI_MESH_FLAG_10) == 0 &&
                    (oldest_index == -1 ||
                     static_cast<unsigned int>(candidate->last_used_3c) <
                         static_cast<unsigned int>(oldest->last_used_3c))) {
                    oldest = candidate;
                    oldest_index = index;
                }
            }
        }
        if (oldest != current) {
            UnloadAniMesh004B63F0(oldest, 0);
            PListRemoveAt(&g_storage_list_65be90, oldest_index);
        } else if (oldest_index == -1) {
            srAssertFail(
                "0", ANI_MESH_CPP, 0x39d,
                "mimp.cpp -> Tell a programmer : running out of monster memory.");
            return;
        }
    }
}
