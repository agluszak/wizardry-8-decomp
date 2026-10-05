#include "wiz8/engine_code/3d.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "wiz8/3d_code/PList.h"
#include "DEBUG.H"
#include "FileMan.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* VC6 places `float x = 0.0f` in .bss; retail keeps this slot in initialized
   .data next to g_float_one. Const storage lands in .rdata with physical
   zeros so datacmp agrees. */
// GLOBAL: WIZ8 0x005ebb34
const float g_float_zero = 0.0f;

#define ANI_MESH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\AniMesh.cpp"

// GLOBAL: WIZ8 0x005ebe80
const double g_double_005ebe80 = 0.5;
// GLOBAL: WIZ8 0x005ebb40
extern const double g_double_zero = 0.0;

/* AniMesh cache: a generation stamp, the loaded-byte total, the 16 MiB primary
   limit, the unused 1 MiB secondary limit the initializer still writes, and
   the list EnforceAniMeshMemoryLimit walks. CreateAniMesh follows this
   initializer immediately in .text. */
// GLOBAL: WIZ8 0x0065be80
int g_animesh_cache_stamp;
// GLOBAL: WIZ8 0x0065be84
int g_animesh_cache_bytes;
// GLOBAL: WIZ8 0x0065be88
int g_animesh_cache_limit;
// GLOBAL: WIZ8 0x0065be8c
int g_animesh_cache_secondary_limit;
// GLOBAL: WIZ8 0x0065be90
W8PList g_animesh_cache_list;

/* The two caller-provided values override the original 16 MiB and 1 MiB
   defaults only when positive. Startup passes -1 for both. */
// FUNCTION: WIZ8 0x004b5780
void InitializeAniMeshCache(int primary_limit, int secondary_limit)
{
    g_animesh_cache_stamp = 0;
    g_animesh_cache_bytes = 0;
    g_animesh_cache_limit = 0x1000000;
    if (primary_limit > 0) {
        g_animesh_cache_limit = primary_limit;
    }
    g_animesh_cache_secondary_limit = 0x100000;
    if (secondary_limit > 0) {
        g_animesh_cache_secondary_limit = secondary_limit;
    }
    PListInit(&g_animesh_cache_list);
}

/* Releases every cached ani-mesh entry and clears the list. */
// FUNCTION: WIZ8 0x004B57D0
void FreeAniMeshCache(void)
{
    PListFreeData(&g_animesh_cache_list);
}

// FUNCTION: WIZ8 0x004b57e0
W8AniMesh* CreateAniMesh()
{
    W8AniMesh* mesh = static_cast<W8AniMesh*>(malloc(sizeof(W8AniMesh)));

    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x72, 0);
    }
    memset(mesh, 0, sizeof(W8AniMesh));
    mesh->bitmap_directory = static_cast<char*>(malloc(0x400));
    mesh->filename = static_cast<char*>(malloc(0x80));
    if (mesh->bitmap_directory == 0) {
        srAssertFail("pAniMesh->strBitmapDir", ANI_MESH_CPP, 0x7a, 0);
    }
    if (mesh->filename == 0) {
        srAssertFail("pAniMesh->strFilename", ANI_MESH_CPP, 0x7b, 0);
    }
    mesh->bitmap_directory[0] = '\0';
    mesh->filename[0] = '\0';
    return mesh;
}

// FUNCTION: WIZ8 0x004b58d0
W8AniMesh* CopyAniMesh(const W8AniMesh* other)
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
    mesh->flags = other->flags;
    mesh->frame_count = other->frame_count;
    mesh->bounds_minimum = other->bounds_minimum;
    mesh->bounds_maximum = other->bounds_maximum;
    mesh->radius = other->radius;
    mesh->loaded_bytes = other->loaded_bytes;
    mesh->list_index = other->list_index;
    mesh->file_offset = other->file_offset;
    mesh->world = other->world;
    mesh->last_used = other->last_used;

    mesh->bitmap_directory = static_cast<char*>(malloc(0x400));
    mesh->filename = static_cast<char*>(malloc(0x80));
    if (mesh->bitmap_directory == 0) {
        srAssertFail("pAniMesh->strBitmapDir", ANI_MESH_CPP, 0xc6, 0);
    }
    if (mesh->filename == 0) {
        srAssertFail("pAniMesh->strFilename", ANI_MESH_CPP, 0xc7, 0);
    }
    strcpy(mesh->bitmap_directory, other->bitmap_directory);
    strcpy(mesh->filename, other->filename);

    if ((other->flags & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        mesh->flags |= W8_ANI_MESH_SINGLE_INSTANCE;
        mesh->meshes = static_cast<stModelInstance**>(malloc(sizeof(*mesh->meshes)));
        if (mesh->meshes == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0xd4, 0);
        }
        mesh->meshes[0] = DuplicateModelInstance(other->meshes[0]);
        mesh->meshes[0]->setName("Ani Mesh Duplicate Instance");
        return mesh;
    }

    mesh->meshes =
        static_cast<stModelInstance**>(malloc(mesh->frame_count * sizeof(*mesh->meshes)));
    if (mesh->meshes == 0) {
        srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0xde, 0);
    }
    memset(mesh->meshes, 0, mesh->frame_count * sizeof(*mesh->meshes));
    for (frame = 0; frame < mesh->frame_count; ++frame) {
        if (other->meshes != 0 && other->meshes[frame] != 0) {
            mesh->meshes[frame] = DuplicateModelInstance(other->meshes[frame]);
            mesh->meshes[frame]->setName("Ani Mesh Duplicate Instance");
        }
    }
    return mesh;
}

unsigned char LoadAniMeshFrameCount(int file, W8AniMesh* mesh);

// FUNCTION: WIZ8 0x004b5b30
unsigned char LoadAniMeshFromInfo(W8ReadLevelInfo* info, W8AniMesh* mesh, unsigned char load_all)
{
    if (info == 0 || mesh == 0) {
        srAssertFail("pInfo&&pAniMesh", ANI_MESH_CPP, 0x105, 0);
    }
    if (info == 0 || mesh == 0)
        return 0;
    strcpy(mesh->bitmap_directory, info->bitmap_folder);
    mesh->world = info->world;
    mesh->flags = 0;
    mesh->last_used = 0;
    if (info->mesh_filename != 0)
        strcpy(mesh->filename, info->mesh_filename);
    mesh->file_offset = FileGetPos(info->hFile);
    if (load_all == 0) {
        unsigned char result = LoadAniMeshFrameCount(info->hFile, mesh);
        mesh->flags |= W8_ANI_MESH_FRAME_COUNT_LOADED | W8_ANI_MESH_KEEP_LOADED;
        return result;
    }
    return LoadAniMesh(info->hFile, mesh, true);
}

// FUNCTION: WIZ8 0x004b5c10
float GetAniMeshFrameRadius(W8AniMesh* mesh, unsigned char frame)
{
    stModelInstance* instance = GetAniMeshFrame(mesh, frame);

    if (instance != 0) {
        stMeshModel* model = static_cast<stMeshModel*>(instance->getModel());

        if (model != 0) {
            srVector3T<float> minimum(0.0f, 0.0f, 0.0f);
            srVector3T<float> maximum(0.0f, 0.0f, 0.0f);

            do {
                srVector3T<float> model_minimum;
                srVector3T<float> model_maximum;

                model->getBoundingBox(model_minimum, model_maximum);
                ExpandBounds(&minimum, &maximum, &model_minimum, &model_maximum);
                model = model->next;
            } while (model != 0);

            return static_cast<float>((minimum - maximum).Length() * g_double_005ebe80);
        }
    }
    return g_float_zero;
}

// FUNCTION: WIZ8 0x004b5d00
unsigned char LoadAniMesh(int file, W8AniMesh* mesh, bool load_all)
{
    int handle = file;
    char* instance_name = 0;
    unsigned char frame_count;
    unsigned char frame_index;
    unsigned char loaded_count;
    srModelInstance* loaded_instance = 0;
    W8ReadLevelInfo info;

    if (handle == 0) {
        handle = FileOpen(mesh->filename, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
        if (handle == 0) {
            srAssertFail(
                "0", ANI_MESH_CPP, 0x199,
                reinterpret_cast<const char*>(String("Couldn't open %s", mesh->filename)));
            return 0;
        }
    }

    info.world = mesh->world;
    info.hFile = handle;
    info.bitmap_folder = mesh->bitmap_directory;
    if (file == 0) {
        FileSeek(handle, mesh->file_offset, FILE_SEEK_FROM_START);
    }

    if (!FileRead(handle, &frame_count, sizeof(frame_count), 0)) {
        srAssertFail("fSuccess", ANI_MESH_CPP, 0x1ad, 0);
        FileClose(handle);
        return 0;
    }
    mesh->frame_count = frame_count;
    mesh->flags |= W8_ANI_MESH_FRAME_COUNT_LOADED;

    if (mesh->filename[0] != '\0') {
        instance_name = new char[strlen(mesh->filename) + 8];
        sprintf(instance_name, "%s::%d::%d", mesh->filename, 0, mesh->list_index);
    }

    if (!FileRead(handle, &frame_index, sizeof(frame_index), 0) ||
        !ReadSingleLevelMesh(&info, &loaded_instance, 0, 0, instance_name, true)) {
        srAssertFail("fSuccess", ANI_MESH_CPP, 0x1c5, 0);
        delete[] instance_name;
        FileClose(handle);
        return 0;
    }

    loaded_instance->setName("AniMeshReallyReadFromFile");
    stModelInstance* instance = static_cast<stModelInstance*>(loaded_instance);
    stMeshModel* model = static_cast<stMeshModel*>(instance->getModel());

    if (model->frame_count > 1) {
        mesh->flags |= W8_ANI_MESH_SINGLE_INSTANCE;
        mesh->meshes = static_cast<stModelInstance**>(malloc(sizeof(*mesh->meshes)));
        if (mesh->meshes == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0x1d0, 0);
        }
        mesh->meshes[0] = instance;
    } else {
        mesh->meshes =
            static_cast<stModelInstance**>(malloc(frame_count * sizeof(*mesh->meshes)));
        if (mesh->meshes == 0) {
            srAssertFail("pAniMesh->ppsrMeshes", ANI_MESH_CPP, 0x1db, 0);
        }
        if (mesh->meshes == 0) {
            delete[] instance_name;
            FileClose(handle);
            return 0;
        }
        memset(mesh->meshes, 0, frame_count * sizeof(*mesh->meshes));
        mesh->meshes[0] = instance;

        loaded_count = 1;
        while (loaded_count < frame_count) {
            loaded_instance = 0;
            if (instance_name != 0) {
                sprintf(instance_name, "%s::%d::%d", mesh->filename, loaded_count,
                        mesh->list_index);
            }
            if (!load_all || !FileRead(handle, &frame_index, sizeof(frame_index), 0) ||
                !ReadSingleLevelMesh(&info, &loaded_instance, 0, 0, instance_name, load_all)) {
                srAssertFail("fSuccess", ANI_MESH_CPP, 0x1f1, 0);
                delete[] instance_name;
                FileClose(handle);
                return 0;
            }
            if (frame_index >= frame_count) {
                delete[] instance_name;
                FileClose(handle);
                return 0;
            }
            loaded_instance->setName("AniMeshReallyReadFromFile");
            mesh->meshes[frame_index] = static_cast<stModelInstance*>(loaded_instance);
            ++loaded_count;
        }
    }

    mesh->flags |= W8_ANI_MESH_LOADED;
    mesh->last_used = g_animesh_cache_stamp++;
    g_animesh_cache_bytes += mesh->loaded_bytes;

    mesh->radius = 0.0f;
    for (frame_index = 0; frame_index < mesh->frame_count; ++frame_index) {
        float radius = GetAniMeshFrameRadius(mesh, frame_index);
        if (mesh->radius <= radius) {
            mesh->radius = radius;
        }
    }

    mesh->bounds_minimum.SetZero();
    mesh->bounds_maximum.SetZero();
    for (frame_index = 0; frame_index < mesh->frame_count; ++frame_index) {
        stModelInstance* frame = GetAniMeshFrame(mesh, frame_index);
        if (frame != 0) {
            stMeshModel* frame_model = static_cast<stMeshModel*>(frame->getModel());
            while (frame_model != 0) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;

                frame_model->getBoundingBox(minimum, maximum);
                ExpandBounds(&mesh->bounds_minimum, &mesh->bounds_maximum, &minimum,
                             &maximum);
                frame_model = frame_model->next;
            }
        }
    }
    mesh->flags |= W8_ANI_MESH_RADIUS_LOADED;

    if (file == 0) {
        FileClose(handle);
    }
    delete[] instance_name;
    if ((mesh->flags & W8_ANI_MESH_KEEP_LOADED) != 0) {
        PLAdoptAppend(&g_animesh_cache_list, mesh);
    }
    EnforceAniMeshMemoryLimit(mesh);
    return 1;
}

/* Hands back the mesh's cached bounding box, loading the mesh first if it has
   not been. Touching it also stamps the storage clock, so asking for bounds
   counts as a use for the memory-pressure walk. */
// FUNCTION: WIZ8 0x004b6640
unsigned char GetAniMeshBounds(W8AniMesh* mesh, srVector3T<float>* minimum,
                               srVector3T<float>* maximum)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x317, 0);
        return 0;
    }
    if ((mesh->flags & W8_ANI_MESH_RADIUS_LOADED) == 0) {
        if (LoadAniMesh(0, mesh, true) == 0) {
            srAssertFail("0", ANI_MESH_CPP, 0x31f, 0);
            return 0;
        }
    }
    *minimum = mesh->bounds_minimum;
    *maximum = mesh->bounds_maximum;
    mesh->last_used = g_animesh_cache_stamp++;
    return 1;
}

// FUNCTION: WIZ8 0x004b5880
void DestroyAniMesh(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x91, 0);
    }
    if ((mesh->flags & W8_ANI_MESH_LOADED) != 0) {
        UnloadAniMesh(mesh, true);
    }
    free(mesh->bitmap_directory);
    free(mesh->filename);
    free(mesh);
}

// FUNCTION: WIZ8 0x004b6290
unsigned char LoadAniMeshFrameCount(int file, W8AniMesh* mesh)
{
    int handle = file;
    unsigned char frame_count, frame_index, loaded_count, success;
    W8ReadLevelInfo info;
    if (handle == 0) {
        handle = FileOpen(mesh->filename, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
        if (handle == 0) {
            srAssertFail(
                "fi.hFile", ANI_MESH_CPP, 0x23f,
                reinterpret_cast<const char*>(String("Couldn't open %s", mesh->filename)));
            return 0;
        }
    }
    info.world = mesh->world;
    info.hFile = handle;
    info.bitmap_folder = mesh->bitmap_directory;
    info.mesh_filename = mesh->filename;
    if (file == 0)
        FileSeek(handle, mesh->file_offset, FILE_SEEK_FROM_START);
    success = FileRead(handle, &frame_count, 1, 0);
    if (success == 0) {
        srAssertFail("fSuccess", ANI_MESH_CPP, 0x253, 0);
        FileClose(handle);
        return 0;
    }
    mesh->flags |= W8_ANI_MESH_FRAME_COUNT_LOADED;
    mesh->frame_count = frame_count;
    for (loaded_count = 0; loaded_count < frame_count; ++loaded_count) {
        if (success != 0)
            success = FileRead(handle, &frame_index, 1, 0);
        if (success != 0 && SkipSingleLevelMesh(&info) == 2)
            break;
    }
    if (file == 0)
        FileClose(handle);
    return success;
}

// FUNCTION: WIZ8 0x004b63f0
unsigned char UnloadAniMesh(W8AniMesh* mesh, bool force)
{
    unsigned char frame_count;
    unsigned int frame;

    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x28a, 0);
        return 0;
    }
    if (!force && (mesh->flags & W8_ANI_MESH_KEEP_LOADED) == 0) {
        return 0;
    }
    if ((mesh->flags & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        mesh->meshes[0]->release();
    } else {
        if ((mesh->flags & W8_ANI_MESH_FRAME_COUNT_LOADED) == 0) {
            if (LoadAniMesh(0, mesh, true) == 0) {
                srAssertFail("0", ANI_MESH_CPP, 0x2c6, 0);
                frame_count = 0xff;
            } else {
                frame_count = mesh->frame_count;
            }
        } else {
            frame_count = mesh->frame_count;
        }
        for (frame = 0; frame < frame_count; ++frame) {
            stModelInstance* instance = GetAniMeshFrame(mesh, frame);

            instance->setParent(0, 1);
            instance->setFlag(srNode::FLAG_DISABLE);
            instance->release();
        }
    }
    g_animesh_cache_bytes -= mesh->loaded_bytes;
    free(mesh->meshes);
    mesh->flags &= ~W8_ANI_MESH_LOADED;
    mesh->meshes = 0;
    return 1;
}

// FUNCTION: WIZ8 0x004b6550
stModelInstance* GetAniMeshFrame(W8AniMesh* mesh, unsigned char frame)
{
    stModelInstance* instance;
    char message[0x80];

    if (mesh == 0 || frame >= mesh->frame_count) {
        sprintf(message, "AniMeshGetMeshForFrame error, frame %d, num %d", frame,
                mesh->frame_count);
        srAssertFail("0", ANI_MESH_CPP, 0x2e6, message);
        return 0;
    }
    if ((mesh->flags & W8_ANI_MESH_LOADED) == 0 && LoadAniMesh(0, mesh, true) == 0) {
        srAssertFail("0", ANI_MESH_CPP, 0x2ee, 0);
        return 0;
    }
    if ((mesh->flags & W8_ANI_MESH_SINGLE_INSTANCE) != 0) {
        instance = mesh->meshes[0];
        instance->frame_index = frame;
    } else {
        instance = mesh->meshes[frame];
    }
    mesh->last_used = g_animesh_cache_stamp;
    ++g_animesh_cache_stamp;
    return instance;
}

// FUNCTION: WIZ8 0x004b64f0
unsigned char AniMeshValue(W8AniMesh* mesh)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x2be, 0);
        return 0xff;
    }
    if ((mesh->flags & W8_ANI_MESH_FRAME_COUNT_LOADED) == 0) {
        if (LoadAniMesh(0, mesh, true) == 0) {
            srAssertFail("0", ANI_MESH_CPP, 0x2c6, 0);
            return 0xff;
        }
    }
    return mesh->frame_count;
}

// FUNCTION: WIZ8 0x004b66e0
bool AniMeshRadius(W8AniMesh* mesh, float* radius)
{
    if (mesh == 0 || radius == 0) {
        srAssertFail("pAniMesh&&pflRadius", ANI_MESH_CPP, 0x348, 0);
    }
    if (mesh != 0 && radius != 0) {
        if ((mesh->flags & W8_ANI_MESH_RADIUS_LOADED) == 0) {
            if (LoadAniMesh(0, mesh, true) == 0) {
                srAssertFail("0", ANI_MESH_CPP, 0x350, 0);
                return false;
            }
        }
        *radius = mesh->radius;
        mesh->last_used = g_animesh_cache_stamp;
        ++g_animesh_cache_stamp;
        return true;
    }
    return false;
}

// FUNCTION: WIZ8 0x004b6860
void SetAniMeshCacheProtected(W8AniMesh* mesh, bool enabled)
{
    if (mesh == 0) {
        srAssertFail("pAniMesh", ANI_MESH_CPP, 0x3b7, 0);
    }
    if (enabled) {
        mesh->flags |= W8_ANI_MESH_CACHE_PROTECTED;
        return;
    }
    mesh->flags &= ~W8_ANI_MESH_CACHE_PROTECTED;
}

// FUNCTION: WIZ8 0x004b6770
void EnforceAniMeshMemoryLimit(W8AniMesh* current)
{
    while (g_animesh_cache_bytes > g_animesh_cache_limit && PLLength(&g_animesh_cache_list) != 0) {
        W8AniMesh* oldest = 0;
        int oldest_index = -1;
        unsigned int count = PLLength(&g_animesh_cache_list);

        for (unsigned int index = 0; index < count; ++index) {
            W8AniMesh* candidate = static_cast<W8AniMesh*>(PLGet(&g_animesh_cache_list, index));

            if (candidate != current) {
                if (candidate == 0) {
                    srAssertFail("pAniMesh", ANI_MESH_CPP, 0x3d0, 0);
                }
                if ((candidate->flags & W8_ANI_MESH_CACHE_PROTECTED) == 0 &&
                    (oldest_index == -1 || static_cast<unsigned int>(candidate->last_used) <
                                               static_cast<unsigned int>(oldest->last_used))) {
                    oldest = candidate;
                    oldest_index = index;
                }
            }
        }
        if (oldest != current) {
            UnloadAniMesh(oldest, false);
            PLRemoveAt(&g_animesh_cache_list, oldest_index);
        } else if (oldest_index == -1) {
            srAssertFail("0", ANI_MESH_CPP, 0x39d,
                         "mimp.cpp -> Tell a programmer : running out of monster memory.");
            return;
        }
    }
}
