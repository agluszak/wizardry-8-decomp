#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "wiz8/3d_code/PList.h"

#include "DEBUG.H"
#include "FileMan.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANI_MESH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AniMesh.cpp"

extern stModelInstance005EC7D0* DuplicateModelInstance0046F680(
    stModelInstance005EC7D0* instance);
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
    mesh->bounds_minimum_08 = other->bounds_minimum_08;
    mesh->bounds_maximum_14 = other->bounds_maximum_14;
    mesh->radius_20 = other->radius_20;
    mesh->loaded_bytes_24 = other->loaded_bytes_24;
    mesh->list_index_28 = other->list_index_28;
    mesh->file_offset_34 = other->file_offset_34;
    mesh->world_38 = other->world_38;
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

// FUNCTION: WIZ8 0x004b5d00
unsigned char LoadAniMesh004B5D00(
    int file, W8AniMesh* mesh, unsigned char load_all)
{
    int handle = file;
    char* instance_name = 0;
    unsigned char frame_count;
    unsigned char frame_index;
    unsigned char loaded_count;
    srModelInstance* loaded_instance = 0;
    W8ReadLevelInfo info;

    if (handle == 0) {
        handle = FileOpen(mesh->filename_30, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
        if (handle == 0) {
            srAssertFail(
                "0", ANI_MESH_CPP, 0x199,
                reinterpret_cast<const char*>(
                    String("Couldn't open %s", mesh->filename_30)));
            return 0;
        }
    }

    info.world = mesh->world_38;
    info.hFile = handle;
    info.bitmap_folder = mesh->bitmap_directory_2c;
    if (file == 0) {
        FileSeek(handle, mesh->file_offset_34, FILE_SEEK_FROM_START);
    }

    if (!ReadVirtualFile(handle, &frame_count, sizeof(frame_count), 0)) {
        srAssertFail("fSuccess", ANI_MESH_CPP, 0x1ad, 0);
        CloseVirtualFile(handle);
        return 0;
    }
    mesh->frame_count_01 = frame_count;
    mesh->flags_00 |= W8_ANI_MESH_FRAME_COUNT_LOADED;

    if (mesh->filename_30[0] != '\0') {
        instance_name = new char[strlen(mesh->filename_30) + 8];
        sprintf(instance_name, "%s_%d_%d", mesh->filename_30, 0,
                mesh->list_index_28);
    }

    if (!ReadVirtualFile(handle, &frame_index, sizeof(frame_index), 0) ||
        !ReadSingleLevelMesh00485B20(
            &info, &loaded_instance, 0, 0, instance_name, 1)) {
        srAssertFail("fSuccess", ANI_MESH_CPP, 0x1c5, 0);
        delete[] instance_name;
        CloseVirtualFile(handle);
        return 0;
    }

    loaded_instance->setName("AniMeshReallyReadFromFile");
    stModelInstance005EC7D0* instance =
        static_cast<stModelInstance005EC7D0*>(loaded_instance);
    stMeshModel* model = static_cast<stMeshModel*>(instance->model());

    if (model->vertex_count > 1) {
        mesh->flags_00 |= W8_ANI_MESH_SINGLE_INSTANCE;
        mesh->meshes_04 = static_cast<stModelInstance005EC7D0**>(
            malloc(sizeof(*mesh->meshes_04)));
        if (mesh->meshes_04 == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0x1d0, 0);
        }
        mesh->meshes_04[0] = instance;
    } else {
        mesh->meshes_04 = static_cast<stModelInstance005EC7D0**>(
            malloc(frame_count * sizeof(*mesh->meshes_04)));
        if (mesh->meshes_04 == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0x1db, 0);
        }
        if (mesh->meshes_04 == 0) {
            delete[] instance_name;
            CloseVirtualFile(handle);
            return 0;
        }
        memset(mesh->meshes_04, 0, frame_count * sizeof(*mesh->meshes_04));
        mesh->meshes_04[0] = instance;

        loaded_count = 1;
        while (loaded_count < frame_count) {
            loaded_instance = 0;
            if (instance_name != 0) {
                sprintf(instance_name, "%s_%d_%d", mesh->filename_30,
                        loaded_count, mesh->list_index_28);
            }
            if (!load_all ||
                !ReadVirtualFile(handle, &frame_index, sizeof(frame_index), 0) ||
                !ReadSingleLevelMesh00485B20(
                    &info, &loaded_instance, 0, 0, instance_name, load_all)) {
                srAssertFail("fSuccess", ANI_MESH_CPP, 0x1f1, 0);
                delete[] instance_name;
                CloseVirtualFile(handle);
                return 0;
            }
            if (frame_index >= frame_count) {
                delete[] instance_name;
                CloseVirtualFile(handle);
                return 0;
            }
            loaded_instance->setName("AniMeshReallyReadFromFile");
            mesh->meshes_04[frame_index] =
                static_cast<stModelInstance005EC7D0*>(loaded_instance);
            ++loaded_count;
        }
    }

    mesh->flags_00 |= W8_ANI_MESH_LOADED;
    mesh->last_used_3c = g_storage_state_65be80++;
    g_storage_state_65be84 += mesh->loaded_bytes_24;

    mesh->radius_20 = 0.0f;
    for (frame_index = 0; frame_index < mesh->frame_count_01; ++frame_index) {
        float radius = GetAniMeshFrameRadius004B5C10(mesh, frame_index);
        if (mesh->radius_20 <= radius) {
            mesh->radius_20 = radius;
        }
    }

    mesh->bounds_minimum_08.x = 0.0f;
    mesh->bounds_minimum_08.y = 0.0f;
    mesh->bounds_minimum_08.z = 0.0f;
    mesh->bounds_maximum_14.x = 0.0f;
    mesh->bounds_maximum_14.y = 0.0f;
    mesh->bounds_maximum_14.z = 0.0f;
    for (frame_index = 0; frame_index < mesh->frame_count_01; ++frame_index) {
        stModelInstance005EC7D0* frame =
            GetAniMeshFrame004B6550(mesh, frame_index);
        if (frame != 0) {
            stMeshModel* frame_model = static_cast<stMeshModel*>(frame->model());
            while (frame_model != 0) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;

                frame_model->getBoundingBox(minimum, maximum);
                ExpandBounds0046F510(
                    &mesh->bounds_minimum_08, &mesh->bounds_maximum_14,
                    &minimum, &maximum);
                frame_model = frame_model->next;
            }
        }
    }
    mesh->flags_00 |= W8_ANI_MESH_RADIUS_LOADED;

    if (file == 0) {
        CloseVirtualFile(handle);
    }
    delete[] instance_name;
    if ((mesh->flags_00 & W8_ANI_MESH_KEEP_LOADED) != 0) {
        PListAdd(&g_storage_list_65be90, mesh);
    }
    EnforceAniMeshMemoryLimit004B6770(mesh);
    return 1;
}

/* Hands back the mesh's cached bounding box, loading the mesh first if it has
   not been. Touching it also stamps the storage clock, so asking for bounds
   counts as a use for the memory-pressure walk. */
// FUNCTION: WIZ8 0x004b6640
unsigned char GetAniMeshBounds004B6640(
    W8AniMesh* mesh, srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x317, 0);
        return 0;
    }
    if ((mesh->flags_00 & 4) == 0) {
        if (LoadAniMesh004B5D00(0, mesh, 1) == 0) {
            srAssertFail("0", ANI_MESH_CPP, 0x31f, 0);
            return 0;
        }
    }
    *minimum = mesh->bounds_minimum_08;
    *maximum = mesh->bounds_maximum_14;
    mesh->last_used_3c = g_storage_state_65be80++;
    return 1;
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
            if (LoadAniMesh004B5D00(0, mesh, 1) == 0) {
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
        LoadAniMesh004B5D00(0, mesh, 1) == 0) {
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
        if (LoadAniMesh004B5D00(0, mesh, 1) == 0) {
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
            if (LoadAniMesh004B5D00(0, mesh, 1) == 0) {
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
