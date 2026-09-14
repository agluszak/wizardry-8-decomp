#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srHeap.h"

#include <math.h>
#include <windows.h>
#include <stdlib.h>

#define PATH_AI_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\PathAI.CPP"

/* identity-alias: as in Bink.cpp, retail shares one no-op stub at 0x004023a0
   across arities (PathAIApplyToRep004A91F0 calls it with two arguments), so
   this overload only satisfies the local call and owns no separate address. */
void NoOp(W8PathAI* path, W8AnimRepBase005EC1D8* representation)
{
    (void)path;
    (void)representation;
}

/* identity-alias: retail folds this empty body with the SYNTHETIC at
   0x004023a0 in vc6_runtime.cpp. */

void NoOp(void) {}

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
        return UpdateMissileAI004A4CF0(
            reinterpret_cast< // reinterpret-ok: the kind byte selects the tagged record type
                W8AIMissile*>(path));
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
    if (!FileRead(handle, &version, 1, 0) || version != 0) {
        return 0;
    }

    point_count = 0;
    path = static_cast<W8PathAI*>(malloc(sizeof(W8PathAI)));
    if (path == 0) {
        return 0;
    }
    memset(path, 0, sizeof(W8PathAI));
    path->nodes_0c = new W8GrowableVector<srVector3T<float>*>(5);

    success = FileRead(handle, &path->unknown_01[0], 1, 0);
    success = success && FileRead(handle, &path->position, 4, 0);
    success = success && FileRead(handle, &path->unknown_08, 4, 0);
    success = success && FileRead(handle, &point_count, 4, 0);

    if (point_count == 0) {
        DestroyPathAI004A9810(path);
        path = 0;
    } else {
        path->rotations_14 =
            static_cast<srMatrix3T<float>*>(malloc(point_count * sizeof(srMatrix3T<float>)));
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
            srVector3T<float>* point =
                static_cast<srVector3T<float>*>(srHeap.allocate(sizeof(srVector3T<float>)));
            float angle;
            srVector3T<float> axis;
            srMatrix3T<float> rotation;

            FileRead(handle, &point->x, 4, 0);
            FileRead(handle, &point->y, 4, 0);
            FileRead(handle, &point->z, 4, 0);
            point->x = static_cast<float>(point->x * g_double_005ec150);
            point->y = static_cast<float>(point->y * g_double_005ec150);
            point->z = static_cast<float>(point->z * g_double_005ec150);
            PathAIAddPoint004A9C30(path, point);

            FileRead(handle, &angle, 4, 0);
            FileRead(handle, &axis.x, 4, 0);
            FileRead(handle, &axis.y, 4, 0);
            FileRead(handle, &axis.z, 4, 0);
            rotation.SetIdentity();
            if ((double)angle != g_zero_005ebb40) {
                rotation.RotateAroundAxis(sin(angle), cos(angle), axis);
            }
            path->rotations_14[index] = rotation;
            if (path->unknown_01[0] == 2) {
                success = success &&
                          FileRead(handle, &path->scales_18[index], sizeof(srVector3T<float>), 0);
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
        path->position = 0;
    }
}

// FUNCTION: WIZ8 0x004a9740
unsigned char PathAIRecordFlag004A9740(const W8PathAI* path)
{
    return path->kind_00;
}

// FUNCTION: WIZ8 0x004a91f0
void PathAIApplyToRep004A91F0(W8PathAI* path, W8AnimRepBase005EC1D8* representation)
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
    copy->position = source->position;
    copy->unknown_08 = source->unknown_08;
    copy->value_10 = source->value_10;
    copy->flag_1c = source->flag_1c;
    copy->point_index = source->point_index;
    copy->interpolation_fraction = source->interpolation_fraction;
    copy->last_update_tick = source->last_update_tick;
    copy->speed = source->speed;
    copy->distance_travelled = source->distance_travelled;
    copy->total_length = source->total_length;
    copy->looping = source->looping;
    copy->flag_39 = source->flag_39;
    copy->flag_3a = source->flag_3a;
    copy->unknown_3b = source->unknown_3b;
    if (source->nodes_0c != 0) {
        count = source->nodes_0c->GetCount();
    } else {
        count = 0;
    }
    copy->nodes_0c = 0;
    if (source->nodes_0c != 0) {
        copy->nodes_0c = new W8GrowableVector<srVector3T<float>*>();
        for (index = 0; index < count; ++index) {
            srVector3T<float>* allocated =
                static_cast<srVector3T<float>*>(srHeap.allocate(sizeof(srVector3T<float>)));
            srVector3T<float>* point;

            if (allocated != 0) {
                *allocated = **source->nodes_0c->GetAt(index);
                point = allocated;
            } else {
                point = 0;
            }
            copy->nodes_0c->Add(point);
        }
    }
    copy->rotations_14 = 0;
    if (source->rotations_14 != 0) {
        copy->rotations_14 =
            static_cast<srMatrix3T<float>*>(malloc(count * sizeof(srMatrix3T<float>)));
        for (index = 0; index < count; ++index) {
            copy->rotations_14[index] = source->rotations_14[index];
        }
    }
    copy->scales_18 = 0;
    if (source->scales_18 != 0) {
        copy->scales_18 =
            static_cast<srVector3T<float>*>(srHeap.allocate(count * sizeof(srVector3T<float>)));
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
unsigned char PathAIAddPoint004A9C30(W8PathAI* path, const srVector3T<float>* point)
{
    srVector3T<float>* copy =
        static_cast<srVector3T<float>*>(srHeap.allocate(sizeof(srVector3T<float>)));
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
        path->position = 0;
        path->interpolation_fraction = 0;
        path->distance_travelled = 0;
        path->point_index = 0;
    }
    path->nodes_0c->Add(copy);
    if (path->nodes_0c->count < 2) {
        path->total_length = 0;
        return 1;
    }

    total_length = g_float_005ebb34;
    for (index = 0; index < path->nodes_0c->count - 1; ++index) {
        const srVector3T<float>* first = *path->nodes_0c->GetAt(index);
        const srVector3T<float>* second = *path->nodes_0c->GetAt(index + 1);
        total_length += (*first - *second).Length();
    }
    path->total_length = total_length;
    if (path->position > g_float_005ebb34 && total_length > g_float_005ebb34) {
        PathAISetValue004A9F60(path, path->distance_travelled / total_length);
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
    path->last_update_tick = GetTickCount();
}

// FUNCTION: WIZ8 0x004a9e70
float PathAIGetValue004A9E70(W8PathAI* path)
{
    if (path == 0) {
        return g_float_005ebb34;
    }
    return path->position;
}

// FUNCTION: WIZ8 0x004a9e90
unsigned char PathAINextPoint004A9E90(W8PathAI* path, srVector3T<float>* point)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    srVector3T<float>* source;

    if (path == 0 || path->nodes_0c == 0) {
        return 0;
    }
    if (path->point_index >= path->nodes_0c->count) {
        if (path->looping == 0) {
            return 0;
        }
        path->point_index = 0;
    }
    srVector3T<float>** slot = path->nodes_0c->data;
    if (path->point_index < path->nodes_0c->count) {
        slot += path->point_index;
    }
    source = *slot;
    *point = *source;
    ++path->point_index;
    return 1;
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x004a9ef0
unsigned char PathAIIsComplete004A9EF0(W8PathAI* path)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    if (path != 0 && path->nodes_0c != 0 &&
        (path->point_index < path->nodes_0c->count || path->looping != 0)) {
        return 0;
    }
    return 1;
#pragma clang diagnostic pop
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
        path->position = 0.0f;
        path->interpolation_fraction = 0;
        path->distance_travelled = 0;
        path->point_index = 0;
        if (path->total_length <= 0.0f) {
            path->position = value;
            return;
        }
        PathAIAdvanceByDistance004A9FE0(path, value * path->total_length);
        return;
    }
    path->position = value;
}

/* Advance the timed path by `distance` world units: fold it into the elapsed
   accumulator, then walk the node chain past every segment the distance
   covers, ending with the in-segment fraction in interpolation_fraction. */
// FUNCTION: WIZ8 0x004a9fe0
void PathAIAdvanceByDistance004A9FE0(W8PathAI* path, float distance)
{
    srVector3T<float> position;
    srVector3T<float>* next;
    srVector3T<float>* current;
    unsigned int index;
    float remaining;
    float segment;

    if (path->total_length <= g_float_005ebb34) {
        return;
    }
    path->distance_travelled += distance;
    path->position = path->distance_travelled / path->total_length;
    if (path->position >= g_float_005ebb38) {
        if (path->looping != 0) {
            path->position = g_float_005ebb34;
            path->interpolation_fraction = g_float_005ebb34;
            path->distance_travelled = g_float_005ebb34;
            path->point_index = 0;
            return;
        }
        path->position = g_float_005ebb38;
        path->interpolation_fraction = g_float_005ebb38;
        path->distance_travelled = path->total_length;
        path->point_index = path->nodes_0c->count - 1;
        return;
    }
    remaining = distance;
    while (true) {
        PathAIPosition004AA370(path, &position);
        index = path->point_index + 1;
        next = *path->nodes_0c->GetAt(index);
        segment = (position - *next).Length();
        if (remaining < segment) {
            break;
        }
        path->point_index = index;
        remaining -= segment;
        path->interpolation_fraction = g_float_005ebb34;
        if (path->nodes_0c->count - 1U <= index) {
            path->point_index = path->nodes_0c->count - 1;
            path->position = g_float_005ebb38;
            path->interpolation_fraction = g_float_005ebb38;
            return;
        }
    }
    index = path->point_index;
    next = *path->nodes_0c->GetAt(index + 1);
    current = *path->nodes_0c->GetAt(index);
    segment = (*current - *next).Length();
    path->interpolation_fraction = (segment * path->interpolation_fraction + remaining) / segment;
}

// FUNCTION: WIZ8 0x004aa160
void PathAIAdvanceNormalized004AA160(W8PathAI* path, float amount)
{
    unsigned int index;
    float position;

    position = amount * path->speed / path->nodes_0c->count + path->position;
    path->position = position;
    if (position < g_float_005ebb38) {
        position *= path->nodes_0c->count;
        index = static_cast<unsigned int>(position);
        path->point_index = index;
        path->interpolation_fraction = position - index;
        return;
    }
    if (path->looping == 0) {
        path->position = g_float_005ebb38;
        path->interpolation_fraction = g_float_005ebb38;
        path->distance_travelled = path->total_length;
        path->point_index = path->nodes_0c->count - 1;
        return;
    }
    path->position = g_float_005ebb34;
    path->interpolation_fraction = g_float_005ebb34;
    path->distance_travelled = g_float_005ebb34;
    path->point_index = 0;
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
        elapsed = now - path->last_update_tick;
        if (path->last_update_tick < now) {
            if (path->flag_3c != 0) {
                PathAIAdvanceNormalized004AA160(path, elapsed * g_float_005ec128);
            } else {
                point_count = (float)path->nodes_0c->count;
                PathAIAdvanceByDistance004A9FE0(path, path->total_length / point_count *
                                                          path->speed * elapsed * g_float_005ec128);
            }
            path->last_update_tick = now;
            return 1;
        }
    } else {
        if (path->flag_39 == 0) {
            amount = (now - path->last_update_tick) * g_float_005ec128 * direction * path->speed;
        } else {
            amount = g_negative_one_005ebc38;
            if (direction > 0) {
                amount = g_float_005ebb38;
            }
        }
        path->position += amount;
        if (path->position >= g_float_005ebb34) {
            point_count = (float)path->nodes_0c->count;
            if (path->position < point_count) {
                path->last_update_tick = now;
                return 1;
            }
            if (path->looping == 0) {
                path->last_update_tick = now;
                path->position = point_count - g_float_005ebb38;
                return 1;
            }
        } else if (path->looping != 0) {
            path->last_update_tick = now;
            path->position = static_cast<float>(path->nodes_0c->count) - g_float_005ebb38;
            return 1;
        }
        path->position = g_float_005ebb34;
    }
    path->last_update_tick = now;
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
    if (path->position < g_float_005ebb34) {
        srAssertFail("pPathAI->flPosition>=0.0f", PATH_AI_CPP, 0x5e8, 0);
    }
    if (path->flag_3a == 0) {
        if (path->flag_1c == 0) {
            index =
                static_cast<int>((path->nodes_0c->count - 1) * path->position + g_double_005ebe80);
        } else {
            index = static_cast<int>(path->position + g_double_005ec3b0);
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

    index = path->point_index;
    if ((unsigned int)index < (unsigned int)(path->nodes_0c->count - 1)) {
        second = *path->nodes_0c->GetAt(index + 1);
        first = *path->nodes_0c->GetAt(index);
        first_weight = g_float_005ebb38 - path->interpolation_fraction;
        value->x = first->x * first_weight + second->x * path->interpolation_fraction;
        value->y = first->y * first_weight + second->y * path->interpolation_fraction;
        value->z = first->z * first_weight + second->z * path->interpolation_fraction;
        return;
    }
    first = *path->nodes_0c->GetAt(index);
    if (first != 0) {
        value->x = first->x;
        value->y = first->y;
        value->z = first->z;
    }
}

// GLOBAL: WIZ8 0x005EC1E8
double g_double_005ec1e8 = 2.0;

/* GLOBAL: WIZ8 0x005EC1F0 - FLT_EPSILON; the dot-product closeness bound at
   which the keyframe slerps fall back to a linear blend. */
double g_double_005ec1f0 = 1.1920928955078125e-07;

// FUNCTION: WIZ8 0x004aa520
void PathAIApply004AA520(W8PathAI* path, stModelInstance* instance)
{
    int index;
    float blend;
    srNode* node;
    srVector3T<float>* scale_vector;
    srVector3T<float> position;
    srVector3T<double> location;
    srMatrix3T<float> rotation;
    srMatrix3T<float> next;
    W8Quaternion first;
    W8Quaternion second;
    W8Quaternion adjusted;
    double amount;
    double dot;
    double angle;
    double sine;
    double w;
    double x;
    double y;
    double z;
    double scale;
    double sx;
    double sy;
    double sz;
    double xx;
    double xy;
    double xz;
    double yy;
    double yz;
    double zz;
    double xw;
    double yw;
    double zw;

    if (instance == 0) {
        return;
    }
    PathAIPosition004AA370(path, &position);

    node = instance->firstChild();
    if (node == 0) {
        location.SetFromFloat(&position);
        instance->setLocation(location);
    } else {
        do {
            location.SetFromFloat(&position);
            node->setLocation(location);
            node = node->nextSibling();
        } while (node != 0);
    }

    if (0 < path->nodes_0c->count) {
        if (path->flag_1c == 0) {
            blend = path->interpolation_fraction;
            index = path->point_index;
        } else {
            index = static_cast<int>(path->position + g_double_005ec3b0);
            blend = -1.0f;
        }
        if (path->nodes_0c->count - 1 <= index) {
            index = path->nodes_0c->count - 1;
            blend = -1.0f;
        }

        rotation = path->rotations_14[index];
        if (g_float_005ebb34 < blend) {
            next = path->rotations_14[index + 1];
            if (!(rotation == next)) {
                first.SetFromMatrix(rotation);
                second.SetFromMatrix(next);
                amount = blend;
                adjusted = second;
                dot = first.w * second.w + first.v.x * second.v.x + first.v.y * second.v.y +
                      first.v.z * second.v.z;
                if (dot < g_zero_005ebb40) {
                    dot = -dot;
                    adjusted.v = -adjusted.v;
                    adjusted.w = -adjusted.w;
                }
                if (g_double_005ebc30 - dot <= g_double_005ec1f0) {
                    dot = g_double_005ebc30 - amount;
                } else {
                    angle = acos(dot);
                    sine = sin(angle);
                    dot = sin((g_double_005ebc30 - amount) * angle) / sine;
                    amount = sin(angle * amount) / sine;
                }
                adjusted.v = dot * first.v + amount * adjusted.v;
                w = first.w * dot + adjusted.w * amount;
                x = adjusted.v.x;
                y = adjusted.v.y;
                z = adjusted.v.z;
                scale = g_double_005ec1e8 / (w * w + x * x + y * y + z * z);
                sx = scale * x;
                sy = scale * y;
                sz = scale * z;
                xw = sx * w;
                yw = sy * w;
                zw = sz * w;
                xx = sx * x;
                xy = sx * y;
                xz = sx * z;
                yy = sy * y;
                yz = sy * z;
                zz = sz * z;
                rotation.vectors[0].x = static_cast<float>(g_double_005ebc30 - (yy + zz));
                rotation.vectors[1].x = static_cast<float>(xy + zw);
                rotation.vectors[2].x = static_cast<float>(xz - yw);
                rotation.vectors[0].y = static_cast<float>(xy - zw);
                rotation.vectors[1].y = static_cast<float>(g_double_005ebc30 - (xx + zz));
                rotation.vectors[2].y = static_cast<float>(yz + xw);
                rotation.vectors[0].z = static_cast<float>(xz + yw);
                rotation.vectors[1].z = static_cast<float>(yz - xw);
                rotation.vectors[2].z = static_cast<float>(g_double_005ebc30 - (xx + yy));
            }
        }

        node = instance->firstChild();
        if (node == 0) {
            instance->setRotation(rotation);
        } else {
            do {
                node->setRotation(rotation);
                node = node->nextSibling();
            } while (node != 0);
        }

        if (path->scales_18 != 0) {
            scale_vector = &path->scales_18[index];
            node = instance->firstChild();
            if (node == 0) {
                location.SetFromFloat(scale_vector);
                instance->setScale(location);
            } else {
                do {
                    location.SetFromFloat(scale_vector);
                    node->setScale(location);
                    node = node->nextSibling();
                } while (node != 0);
            }
        }

        if (path->unknown_3b != 0) {
            instance->rotateX(1.5707963);
        }
    }
}

// FUNCTION: WIZ8 0x004aa9c0
void PathAISetScale004AA9C0(W8PathAI* path, float value)
{
    path->speed = value;
}

// FUNCTION: WIZ8 0x004aa9d0
void PathAISetFlag38004AA9D0(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6dc, 0);
    }
    path->looping = value;
}

// FUNCTION: WIZ8 0x004aaa10
void PathAISetFlag1C004AAA10(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6ee, 0);
    }
    path->flag_1c = value;
}

// FUNCTION: WIZ8 0x004a9750
W8PathAI* CreateRecord004A9750(int unused)
{
    W8PathAI* path;

    path = (W8PathAI*)malloc(sizeof(W8PathAI));
    if (!path) {
        return 0;
    }
    memset(path, 0, sizeof(W8PathAI));
    path->nodes_0c = new W8GrowableVector<srVector3T<float>*>();
    return path;
}
