#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srHeap.h"

#include <math.h>
#include <windows.h>
#include <stdlib.h>

#define PATH_AI_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\PathAI.CPP"

extern "C" void NoOp(
    W8PathAI* path, W8AnimRepBase005EC1D8* representation);
extern unsigned char Function4A4CF0(W8PathAI* path);
extern void Function4A9FE0(W8PathAI* path, float value);
extern float g_float_005ebb34;
extern float g_float_005ebc38;
extern double g_double_005ebe80;
extern float g_float_005ec128;
extern double g_double_005ec3b0;
extern const double g_path_ai_world_scale_005ec150;
extern float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);

// FUNCTION: WIZ8 0x004a9260
unsigned char PathAIUpdate004A9260(W8PathAI* path, signed char direction)
{
    if (path == 0) {
        return 0;
    }
    switch (path->kind_00) {
    case 0:
        return static_cast<unsigned char>(PathAITick004AA1F0(path, direction));
    case 1:
        return 0;
    case 3:
        return Function4A4CF0(path);
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x004a92a0
unsigned char LoadPathAI004A92A0(W8PathAI** output, int handle)
{
    unsigned char version;
    unsigned char success;
    W8PathAI* path;
    int point_count;
    int index;

    if (output == 0) {
        return 0;
    }
    if (!ReadVirtualFile(handle, &version, 1, 0) || version != 0) {
        return 0;
    }

    point_count = 0;
    path = static_cast<W8PathAI*>(malloc(sizeof(W8PathAI)));
    if (path == 0) {
        return 0;
    }
    memset(path, 0, sizeof(W8PathAI));
    path->nodes_0c = new W8GrowableVector<srVector3T<float>*>(5);

    success = ReadVirtualFile(handle, &path->unknown_01[0], 1, 0);
    success = success && ReadVirtualFile(handle, &path->value_04, 4, 0);
    success = success && ReadVirtualFile(handle, &path->unknown_08, 4, 0);
    success = success && ReadVirtualFile(handle, &point_count, 4, 0);

    if (point_count == 0) {
        DestroyPathAI004A9810(path);
        path = 0;
    }
    else {
        path->rotations_14 = static_cast<srMatrix3T<float>*>(
            malloc(point_count * sizeof(srMatrix3T<float>)));
        if (path->rotations_14 == 0) {
            srAssertFail("pPathAI->pRotations", PATH_AI_CPP, 0x104, 0);
        }
        if (path->unknown_01[0] == 2) {
            path->scales_18 = static_cast<srVector3T<float>*>(
                srHeap.allocate(point_count * sizeof(srVector3T<float>)));
            if (path->scales_18 == 0) {
                srAssertFail("pPathAI->pvecScales", PATH_AI_CPP, 0x10a, 0);
            }
        }

        for (index = 0; index < point_count; ++index) {
            srVector3T<float>* point = static_cast<srVector3T<float>*>(
                srHeap.allocate(sizeof(srVector3T<float>)));
            float angle;
            srVector3T<float> axis;
            srMatrix3T<float> rotation;

            ReadVirtualFile(handle, &point->x, 4, 0);
            ReadVirtualFile(handle, &point->y, 4, 0);
            ReadVirtualFile(handle, &point->z, 4, 0);
            point->x = static_cast<float>(point->x * g_path_ai_world_scale_005ec150);
            point->y = static_cast<float>(point->y * g_path_ai_world_scale_005ec150);
            point->z = static_cast<float>(point->z * g_path_ai_world_scale_005ec150);
            PathAIAddPoint004A9C30(path, point);

            ReadVirtualFile(handle, &angle, 4, 0);
            ReadVirtualFile(handle, &axis.x, 4, 0);
            ReadVirtualFile(handle, &axis.y, 4, 0);
            ReadVirtualFile(handle, &axis.z, 4, 0);
            rotation.SetIdentity00467310();
            if ((double)angle != g_zero_005ebb40) {
                RotateMatrixAroundAxis0042B910(
                    &rotation.vectors[0].x,
                    sin(angle), cos(angle), &axis.x);
            }
            path->rotations_14[index] = rotation;
            if (path->unknown_01[0] == 2) {
                success = success && ReadVirtualFile(
                    handle, &path->scales_18[index],
                    sizeof(srVector3T<float>), 0);
            }
            srHeap.free(point);
        }
    }

    *output = path;
    return 1;
}

// FUNCTION: WIZ8 0x004a9720
void PathAIResetRecord004A9720(W8PathAI* path)
{
    if (path != 0 && path->kind_00 == 0) {
        path->value_04 = 0;
    }
}

// FUNCTION: WIZ8 0x004a9740
unsigned char PathAIRecordFlag004A9740(const W8PathAI* path)
{
    return path->kind_00;
}

// FUNCTION: WIZ8 0x004a91f0
void PathAIApplyToRep004A91F0(
    W8PathAI* path, W8AnimRepBase005EC1D8* representation)
{
    if (path->kind_00 != 0) {
        if (path->kind_00 == 3) {
            NoOp(path, representation);
        }
        return;
    }
    if (path == 0 || representation == 0) {
        srAssertFail("pPathAI&&pRep", PATH_AI_CPP, 0x595, 0);
    }
    PathAIPosition004AA370(path, &representation->parent_location_01c);
    representation->location_004 = representation->parent_location_01c;
}

// FUNCTION: WIZ8 0x004a9810
void DestroyPathAI004A9810(W8PathAI* path)
{
    W8GrowableVector<srVector3T<float>*>* nodes;

    if (path != 0) {
        nodes = path->nodes_0c;
        if (nodes != 0) {
            while (nodes->count != 0) {
                srHeap.free(nodes->RemoveAt(nodes->GetCount() - 1));
                nodes = path->nodes_0c;
            }
            if (path->rotations_14 != 0) {
                free(path->rotations_14);
                path->rotations_14 = 0;
            }
            delete path->nodes_0c;
        }
        if (path->rotations_14 != 0) {
            free(path->rotations_14);
        }
        if (path->scales_18 != 0) {
            srHeap.free(path->scales_18);
        }
        free(path);
    }
}

/* The same release DestroyPathAI004A9810 performs, refused for any path whose
   kind is not the plain node-list form. stLight's destructor at 0x0049C430
   reaches the owned path through this guard rather than the general entry. */
// FUNCTION: WIZ8 0x004a9110
void DestroyOwnedPathAI004A9110(W8PathAI* path)
{
    W8GrowableVector<srVector3T<float>*>* nodes;

    if (path != 0 && path->kind_00 == 0) {
        nodes = path->nodes_0c;
        if (nodes != 0) {
            while (nodes->count != 0) {
                srHeap.free(nodes->RemoveAt(nodes->GetCount() - 1));
                nodes = path->nodes_0c;
            }
            if (path->rotations_14 != 0) {
                free(path->rotations_14);
                path->rotations_14 = 0;
            }
            delete path->nodes_0c;
        }
        if (path->rotations_14 != 0) {
            free(path->rotations_14);
        }
        if (path->scales_18 != 0) {
            srHeap.free(path->scales_18);
        }
        free(path);
    }
}

/* A deep copy. Everything the source owns is rebuilt: each node point gets its
   own srHeap allocation, and both trailing arrays are reallocated and copied
   element by element at the node count. Nothing is shared, and flag_3c is the
   one field the copy does not carry over. */
// FUNCTION: WIZ8 0x004a98c0
W8PathAI* ClonePathAI004A98C0(const W8PathAI* source)
{
    W8PathAI* copy = static_cast<W8PathAI*>(malloc(sizeof(W8PathAI)));
    int count;
    int index;

    if (copy == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x1ef, 0);
    }
    copy->kind_00 = source->kind_00;
    copy->unknown_01[0] = source->unknown_01[0];
    copy->value_04 = source->value_04;
    copy->unknown_08 = source->unknown_08;
    copy->value_10 = source->value_10;
    copy->flag_1c = source->flag_1c;
    copy->value_20 = source->value_20;
    copy->value_24 = source->value_24;
    copy->tick_28 = source->tick_28;
    copy->value_2c = source->value_2c;
    copy->value_30 = source->value_30;
    copy->scale_34 = source->scale_34;
    copy->flag_38 = source->flag_38;
    copy->flag_39 = source->flag_39;
    copy->flag_3a = source->flag_3a;
    copy->unknown_3b = source->unknown_3b;
    if (source->nodes_0c != 0) {
        count = source->nodes_0c->GetCount();
    }
    else {
        count = 0;
    }
    copy->nodes_0c = 0;
    if (source->nodes_0c != 0) {
        copy->nodes_0c = new W8GrowableVector<srVector3T<float>*>();
        for (index = 0; index < count; ++index) {
            srVector3T<float>* allocated = static_cast<srVector3T<float>*>(
                srHeap.allocate(sizeof(srVector3T<float>)));
            srVector3T<float>* point;

            if (allocated != 0) {
                *allocated = **source->nodes_0c->GetAt(index);
                point = allocated;
            }
            else {
                point = 0;
            }
            copy->nodes_0c->Add(point);
        }
    }
    copy->rotations_14 = 0;
    if (source->rotations_14 != 0) {
        copy->rotations_14 = static_cast<srMatrix3T<float>*>(
            malloc(count * sizeof(srMatrix3T<float>)));
        for (index = 0; index < count; ++index) {
            copy->rotations_14[index] = source->rotations_14[index];
        }
    }
    copy->scales_18 = 0;
    if (source->scales_18 != 0) {
        copy->scales_18 = static_cast<srVector3T<float>*>(
            srHeap.allocate(count * sizeof(srVector3T<float>)));
        for (index = 0; index < count; ++index) {
            copy->scales_18[index] = source->scales_18[index];
        }
    }
    return copy;
}

/* Clone whichever AI record the tag selects. An unknown tag copies nothing and
   returns null rather than aliasing the source. */
// FUNCTION: WIZ8 0x004a91c0
void* CloneAIRecord004A91C0(void* record)
{
    if (record == 0) {
        return 0;
    }
    switch (*static_cast<const unsigned char*>(record)) {
    case 0:
        return ClonePathAI004A98C0(static_cast<const W8PathAI*>(record));
    case 3:
        return CopyAIMissile004A53A0(static_cast<const W8AIMissile*>(record));
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x004a9bb0
void PathAIClearOwned004A9BB0(W8PathAI* path)
{
    W8GrowableVector<srVector3T<float>*>* nodes;

    if (path != 0) {
        nodes = path->nodes_0c;
        if (nodes != 0) {
            while (nodes->count != 0) {
                srHeap.free(nodes->RemoveAt(nodes->GetCount() - 1));
                nodes = path->nodes_0c;
            }
        }
        if (path->rotations_14 != 0) {
            free(path->rotations_14);
            path->rotations_14 = 0;
        }
    }
}

// FUNCTION: WIZ8 0x004a9c30
unsigned char PathAIAddPoint004A9C30(
    W8PathAI* path, const srVector3T<float>* point)
{
    srVector3T<float>* copy = static_cast<srVector3T<float>*>(
        srHeap.allocate(sizeof(srVector3T<float>)));
    float total_length;
    int index;

    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x296, 0);
    }
    if (path->nodes_0c == 0) {
        srAssertFail("pPathAI->plsPoints", PATH_AI_CPP, 0x297, 0);
    }
    *copy = *point;
    if (path->nodes_0c->count == 0) {
        path->value_04 = 0;
        path->value_24 = 0;
        path->value_30 = 0;
        path->value_20 = 0;
    }
    path->nodes_0c->Add(copy);
    if (path->nodes_0c->count < 2) {
        path->scale_34 = 0;
        return 1;
    }

    total_length = g_float_005ebb34;
    for (index = 0; index < path->nodes_0c->count - 1; ++index) {
        const srVector3T<float>* first = *path->nodes_0c->GetAt(index);
        const srVector3T<float>* second = *path->nodes_0c->GetAt(index + 1);
        float x = first->x - second->x;
        float y = first->y - second->y;
        float z = first->z - second->z;
        total_length += static_cast<float>(sqrt(x * x + y * y + z * z));
    }
    path->scale_34 = total_length;
    if (path->value_04 > g_float_005ebb34 && total_length > g_float_005ebb34) {
        PathAISetValue004A9F60(path, path->value_30 / total_length);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004a9b90
void PathAISetFlag3A004A9B90(W8PathAI* path, unsigned char value)
{
    path->flag_3a = value;
}

// FUNCTION: WIZ8 0x004a9ba0
void PathAIEnableTimedMode004A9BA0(W8PathAI* path)
{
    path->flag_3a = 1;
    path->flag_3c = 1;
}

// FUNCTION: WIZ8 0x004a9c20
void PathAIResetTick004A9C20(W8PathAI* path)
{
    path->tick_28 = GetTickCount();
}

// FUNCTION: WIZ8 0x004a9e70
float PathAIGetValue004A9E70(W8PathAI* path)
{
    if (path == 0) {
        return g_float_005ebb34;
    }
    return path->value_04;
}

// FUNCTION: WIZ8 0x004a9e90
unsigned char PathAINextPoint004A9E90(W8PathAI* path, srVector3T<float>* point)
{
    srVector3T<float>* source;

    if (path == 0 || path->nodes_0c == 0) {
        return 0;
    }
    if (path->value_20 >= path->nodes_0c->count) {
        if (path->flag_38 == 0) {
            return 0;
        }
        path->value_20 = 0;
    }
    srVector3T<float>** slot = path->nodes_0c->data;
    if (path->value_20 < path->nodes_0c->count) {
        slot += path->value_20;
    }
    source = *slot;
    *point = *source;
    ++path->value_20;
    return 1;
}

// FUNCTION: WIZ8 0x004a9ef0
unsigned char PathAIIsComplete004A9EF0(W8PathAI* path)
{
    if (path != 0 && path->nodes_0c != 0 &&
        (path->value_20 < path->nodes_0c->count || path->flag_38 != 0)) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004a9f20
unsigned int PathAIEntryCount004A9F20(W8PathAI* path)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x43b, 0);
    }
    return path->nodes_0c->count;
}

// FUNCTION: WIZ8 0x004a9f60
void PathAISetValue004A9F60(W8PathAI* path, float value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x46e, 0);
    }
    if (path->flag_3a != 0) {
        path->value_04 = 0.0f;
        path->value_24 = 0;
        path->value_30 = 0;
        path->value_20 = 0;
        if (path->scale_34 <= 0.0f) {
            path->value_04 = value;
            return;
        }
        Function4A9FE0(path, value * path->scale_34);
        return;
    }
    path->value_04 = value;
}

// FUNCTION: WIZ8 0x004aa160
void PathAIAdvanceNormalized004AA160(W8PathAI* path, float amount)
{
    unsigned int index;
    float position;

    position = amount * path->value_2c / path->nodes_0c->count + path->value_04;
    path->value_04 = position;
    if (position < g_float_005ebb38) {
        index = (unsigned int)position;
        path->value_20 = index;
        path->value_24 = position - index;
        return;
    }
    if (path->flag_38 == 0) {
        path->value_04 = g_float_005ebb38;
        path->value_24 = g_float_005ebb38;
        path->value_30 = path->scale_34;
        path->value_20 = path->nodes_0c->count - 1;
        return;
    }
    path->value_04 = g_float_005ebb34;
    path->value_24 = g_float_005ebb34;
    path->value_30 = g_float_005ebb34;
    path->value_20 = 0;
}

// FUNCTION: WIZ8 0x004aa1f0
int PathAITick004AA1F0(W8PathAI* path, signed char direction)
{
    DWORD now;
    unsigned int elapsed;
    float amount;
    float point_count;

    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x520, 0);
    }
    now = GetTickCount();
    if (path->flag_1c == 0 && path->flag_3a != 0) {
        elapsed = now - path->tick_28;
        if (path->tick_28 < now) {
            if (path->flag_3c != 0) {
                PathAIAdvanceNormalized004AA160(path, elapsed * g_float_005ec128);
            }
            else {
                point_count = (float)path->nodes_0c->count;
                Function4A9FE0(path,
                               path->scale_34 / point_count * path->value_2c * elapsed *
                                   g_float_005ec128);
            }
            path->tick_28 = now;
            return 1;
        }
    }
    else {
        if (path->flag_39 == 0) {
            amount = (now - path->tick_28) * g_float_005ec128 * direction * path->value_2c;
        }
        else {
            amount = g_float_005ebc38;
            if (direction > 0) {
                amount = g_float_005ebb38;
            }
        }
        path->value_04 += amount;
        if (path->value_04 >= g_float_005ebb34) {
            point_count = (float)path->nodes_0c->count;
            if (path->value_04 < point_count) {
                path->tick_28 = now;
                return 1;
            }
            if (path->flag_38 == 0) {
                path->tick_28 = now;
                path->value_04 = point_count - g_float_005ebb38;
                return 1;
            }
        }
        else if (path->flag_38 != 0) {
            path->tick_28 = now;
            path->value_04 = (float)path->nodes_0c->count - g_float_005ebb38;
            return 1;
        }
        path->value_04 = g_float_005ebb34;
    }
    path->tick_28 = now;
    return 1;
}

// FUNCTION: WIZ8 0x004aa370
void PathAIPosition004AA370(W8PathAI* path, srVector3T<float>* value)
{
    int index;
    srVector3T<float>* first;
    srVector3T<float>* second;
    float first_weight;

    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x5e7, 0);
    }
    if (path->value_04 < g_float_005ebb34) {
        srAssertFail("pPathAI->flPosition>=0.0f", PATH_AI_CPP, 0x5e8, 0);
    }
    if (path->flag_3a == 0) {
        if (path->flag_1c == 0) {
            index = (int)((path->nodes_0c->count - 1) * path->value_04 + g_double_005ebe80);
        }
        else {
            index = (int)(path->value_04 + g_double_005ec3b0);
        }
        if (index >= path->nodes_0c->count) {
            index = path->nodes_0c->count - 1;
        }
        if (index >= 0) {
            first = *path->nodes_0c->GetAt(index);
            if (first != 0) {
                value->x = first->x;
                value->y = first->y;
                value->z = first->z;
            }
        }
        return;
    }

    index = path->value_20;
    if ((unsigned int)index < (unsigned int)(path->nodes_0c->count - 1)) {
        second = *path->nodes_0c->GetAt(index + 1);
        first = *path->nodes_0c->GetAt(index);
        first_weight = g_float_005ebb38 - path->value_24;
        value->x = first->x * first_weight + second->x * path->value_24;
        value->y = first->y * first_weight + second->y * path->value_24;
        value->z = first->z * first_weight + second->z * path->value_24;
        return;
    }
    first = *path->nodes_0c->GetAt(index);
    if (first != 0) {
        value->x = first->x;
        value->y = first->y;
        value->z = first->z;
    }
}

// FUNCTION: WIZ8 0x004aa9c0
void PathAISetScale004AA9C0(W8PathAI* path, float value)
{
    path->value_2c = value;
}

// FUNCTION: WIZ8 0x004aa9d0
void PathAISetFlag38004AA9D0(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6dc, 0);
    }
    path->flag_38 = value;
}

// FUNCTION: WIZ8 0x004aaa10
void PathAISetFlag1C004AAA10(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6ee, 0);
    }
    path->flag_1c = value;
}
