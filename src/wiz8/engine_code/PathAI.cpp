#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "surrender/srNode.h"
#include "surrender/srHeap.h"

#include <math.h>
#include <windows.h>
#include <stdlib.h>

#define PATH_AI_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\PathAI.CPP"

/* Retail shares one no-op stub at 0x004023a0 across arities;
   PathAIApplyToRep calls this typed overload with two arguments. */
void NoOp(W8AIRecord*, W8AnimRepBase*) {}

/* Retail shares this empty body with the stub at 0x004023a0. */

void NoOp(void) {}

// FUNCTION: WIZ8 0x004a9260
unsigned char PathAIUpdate(W8AIRecord* record, signed char direction)
{
    if (record == 0) {
        return 0;
    }
    switch (record->kind_00) {
    case 0:
        return static_cast<unsigned char>(PathAITick(static_cast<W8PathAI*>(record), direction));
    case 1:
        return 0;
    case 3:
        return UpdateMissileAI(static_cast<W8AIMissile*>(record));
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x004a92a0
bool LoadPathAI004A92A0(W8PathAI** output, int handle)
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

    success = FileRead(handle, &path->version_01, 1, 0);
    success = success && FileRead(handle, &path->position, 4, 0);
    success = success && FileRead(handle, &path->unknown_08, 4, 0);
    success = success && FileRead(handle, &point_count, 4, 0);

    if (point_count == 0) {
        DestroyPathAI(path);
        path = 0;
    } else {
        path->rotations_14 =
            static_cast<srMatrix3T<float>*>(malloc(point_count * sizeof(srMatrix3T<float>)));
        if (path->rotations_14 == 0) {
            srAssertFail("pPathAI->pRotations", PATH_AI_CPP, 0x104, 0);
        }
        if (path->version_01 == 2) {
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
            PathAIAddPoint(path, point);

            FileRead(handle, &angle, 4, 0);
            FileRead(handle, &axis.x, 4, 0);
            FileRead(handle, &axis.y, 4, 0);
            FileRead(handle, &axis.z, 4, 0);
            rotation.SetIdentity();
            if (angle != g_zero_005ebb40) {
                rotation.RotateAroundAxis(sin(angle), cos(angle), axis);
            }
            path->rotations_14[index] = rotation;
            if (path->version_01 == 2) {
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
void PathAIResetRecord(W8PathAI* path)
{
    if (path != 0 && path->kind_00 == 0) {
        path->position = 0;
    }
}

// FUNCTION: WIZ8 0x004a9740
unsigned char PathAIRecordFlag(const W8AIRecord* record)
{
    return record->kind_00;
}

// FUNCTION: WIZ8 0x004a91f0
void PathAIApplyToRep(W8AIRecord* record, W8AnimRepBase* representation)
{
    if (record->kind_00 != 0) {
        if (record->kind_00 == 3) {
            NoOp(record, representation);
        }
        return;
    }
    W8PathAI* path = static_cast<W8PathAI*>(record);
    if (path == 0 || representation == 0) {
        srAssertFail("pPathAI&&pRep", PATH_AI_CPP, 0x595, 0);
    }
    PathAIPosition(path, &representation->parent_location_01c);
    representation->location_004 = representation->parent_location_01c;
}

// FUNCTION: WIZ8 0x004a9810
void DestroyPathAI(W8PathAI* path)
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

/* The same release DestroyPathAI performs, refused for any path whose
   kind is not the plain node-list form. stLight's destructor at 0x0049C430
   reaches the owned path through this guard rather than the general entry. */
// FUNCTION: WIZ8 0x004a9110
void DestroyOwnedPathAI(W8PathAI* path)
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
   element by element at the node count. Nothing is shared, and timed_3c is the
   one field the copy does not carry over. */
// FUNCTION: WIZ8 0x004a98c0
W8PathAI* ClonePathAI(const W8PathAI* source)
{
    W8PathAI* copy = static_cast<W8PathAI*>(malloc(sizeof(W8PathAI)));
    int count;
    int index;

    if (copy == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x1ef, 0);
    }
    copy->kind_00 = source->kind_00;
    copy->version_01 = source->version_01;
    copy->position = source->position;
    copy->unknown_08 = source->unknown_08;
    copy->entry_index_10 = source->entry_index_10;
    copy->discrete_mode_1c = source->discrete_mode_1c;
    copy->point_index = source->point_index;
    copy->interpolation_fraction = source->interpolation_fraction;
    copy->last_update_tick = source->last_update_tick;
    copy->speed = source->speed;
    copy->distance_travelled = source->distance_travelled;
    copy->total_length = source->total_length;
    copy->looping = source->looping;
    copy->step_by_node_39 = source->step_by_node_39;
    copy->animated_3a = source->animated_3a;
    copy->upright_3b = source->upright_3b;
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
W8AIRecord* CloneAIRecord(const W8AIRecord* record)
{
    if (record == 0) {
        return 0;
    }
    switch (record->kind_00) {
    case 0:
        return ClonePathAI(static_cast<const W8PathAI*>(record));
    case 3:
        return CopyAIMissile(static_cast<const W8AIMissile*>(record));
    default:
        return 0;
    }
}

// FUNCTION: WIZ8 0x004a9bb0
void PathAIClearOwned(W8PathAI* path)
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
unsigned char PathAIAddPoint(W8PathAI* path, const srVector3T<float>* point)
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
        PathAISetValue(path, path->distance_travelled / total_length);
    }
    return 1;
}

// FUNCTION: WIZ8 0x004a9b90
void PathAISetAnimated(W8PathAI* path, unsigned char value)
{
    path->animated_3a = value;
}

// FUNCTION: WIZ8 0x004a9ba0
void PathAIEnableTimedMode(W8PathAI* path)
{
    path->animated_3a = 1;
    path->timed_3c = 1;
}

// FUNCTION: WIZ8 0x004a9c20
void PathAIResetTick(W8PathAI* path)
{
    path->last_update_tick = GetTickCount();
}

// FUNCTION: WIZ8 0x004a9e70
float PathAIGetValue(W8PathAI* path)
{
    if (path == 0) {
        return g_float_005ebb34;
    }
    return path->position;
}

// FUNCTION: WIZ8 0x004a9e90
unsigned char PathAINextPoint(W8PathAI* path, srVector3T<float>* point)
{
    if (path == 0 || path->nodes_0c == 0) {
        return 0;
    }
    if (path->point_index >= static_cast<unsigned int>(path->nodes_0c->GetCount())) {
        if (path->looping == 0) {
            return 0;
        }
        path->point_index = 0;
    }
    *point = **path->nodes_0c->GetAt(path->point_index);
    ++path->point_index;
    return 1;
}

// FUNCTION: WIZ8 0x004a9ef0
bool PathAIIsComplete(W8PathAI* path)
{
    if (path != 0 && path->nodes_0c != 0 &&
        (path->point_index < static_cast<unsigned int>(path->nodes_0c->GetCount()) ||
         path->looping != 0)) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004a9f20
unsigned int PathAIEntryCount(W8PathAI* path)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x43b, 0);
    }
    return path->nodes_0c->count;
}

// FUNCTION: WIZ8 0x004a9f60
void PathAISetValue(W8PathAI* path, float value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x46e, 0);
    }
    if (path->animated_3a != 0) {
        path->position = 0.0f;
        path->interpolation_fraction = 0;
        path->distance_travelled = 0;
        path->point_index = 0;
        if (path->total_length <= 0.0f) {
            path->position = value;
            return;
        }
        PathAIAdvanceByDistance(path, value * path->total_length);
        return;
    }
    path->position = value;
}

/* Advance the timed path by `distance` world units: fold it into the elapsed
   accumulator, then walk the node chain past every segment the distance
   covers, ending with the in-segment fraction in interpolation_fraction. */
// FUNCTION: WIZ8 0x004a9fe0
void PathAIAdvanceByDistance(W8PathAI* path, float distance)
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
        PathAIPosition(path, &position);
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
void PathAIAdvanceNormalized(W8PathAI* path, float amount)
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
int PathAITick(W8PathAI* path, signed char direction)
{
    DWORD now;
    unsigned int elapsed;
    float amount;
    float point_count;

    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x520, 0);
    }
    now = GetTickCount();
    if (path->discrete_mode_1c == 0 && path->animated_3a != 0) {
        elapsed = now - path->last_update_tick;
        if (path->last_update_tick < now) {
            if (path->timed_3c != 0) {
                PathAIAdvanceNormalized(path, elapsed * g_float_005ec128);
            } else {
                point_count = (float)path->nodes_0c->count;
                PathAIAdvanceByDistance(path, path->total_length / point_count * path->speed *
                                                  elapsed * g_float_005ec128);
            }
            path->last_update_tick = now;
            return 1;
        }
    } else {
        if (path->step_by_node_39 == 0) {
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
            path->position = path->nodes_0c->count - g_float_005ebb38;
            return 1;
        }
        path->position = g_float_005ebb34;
    }
    path->last_update_tick = now;
    return 1;
}

// FUNCTION: WIZ8 0x004aa370
void PathAIPosition(W8PathAI* path, srVector3T<float>* value)
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
    if (path->animated_3a == 0) {
        if (path->discrete_mode_1c == 0) {
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
                *value = *first;
            }
        }
        return;
    }

    index = path->point_index;
    if ((unsigned int)index < (unsigned int)(path->nodes_0c->count - 1)) {
        second = *path->nodes_0c->GetAt(index + 1);
        first = *path->nodes_0c->GetAt(index);
        first_weight = g_float_005ebb38 - path->interpolation_fraction;
        *value = *first * first_weight + *second * path->interpolation_fraction;
        return;
    }
    first = *path->nodes_0c->GetAt(index);
    if (first != 0) {
        *value = *first;
    }
}

// GLOBAL: WIZ8 0x005EC1E8
double g_double_005ec1e8 = 2.0;

/* FLT_EPSILON: the dot-product closeness bound at which the keyframe slerps
   fall back to a linear blend. */
// GLOBAL: WIZ8 0x005EC1F0
double g_double_005ec1f0 = 1.1920928955078125e-07;

// FUNCTION: WIZ8 0x004aa520
void PathAIApply004AA520(W8PathAI* path, srNode* target)
{
    int index;
    float blend;
    srNode* node;
    srVector3T<float>* scale_vector;
    srVector3T<float> position;
    srVector3T<double> location;
    srMatrix3T<float> rotation;
    srMatrix3T<float> next;

    if (target == 0) {
        return;
    }
    PathAIPosition(path, &position);

    node = target->firstChild();
    if (node == 0) {
        location.SetFromFloat(&position);
        target->setLocation(location);
    } else {
        do {
            location.SetFromFloat(&position);
            node->setLocation(location);
            node = node->nextSibling();
        } while (node != 0);
    }

    if (0 < path->nodes_0c->count) {
        if (path->discrete_mode_1c == 0) {
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
                W8Quaternion::InterpolateRotation(rotation, next, blend, &rotation);
            }
        }

        node = target->firstChild();
        if (node == 0) {
            target->setRotation(rotation);
        } else {
            do {
                node->setRotation(rotation);
                node = node->nextSibling();
            } while (node != 0);
        }

        if (path->scales_18 != 0) {
            scale_vector = &path->scales_18[index];
            node = target->firstChild();
            if (node == 0) {
                location.SetFromFloat(scale_vector);
                target->setScale(location);
            } else {
                do {
                    location.SetFromFloat(scale_vector);
                    node->setScale(location);
                    node = node->nextSibling();
                } while (node != 0);
            }
        }

        if (path->upright_3b != 0) {
            target->rotateX(1.5707963);
        }
    }
}

// FUNCTION: WIZ8 0x004aa9c0
void PathAISetScale(W8PathAI* path, float value)
{
    path->speed = value;
}

// FUNCTION: WIZ8 0x004aa9d0
void PathAISetLooping(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6dc, 0);
    }
    path->looping = value;
}

// FUNCTION: WIZ8 0x004aaa10
void PathAISetDiscreteMode(W8PathAI* path, unsigned char value)
{
    if (path == 0) {
        srAssertFail("pPathAI", PATH_AI_CPP, 0x6ee, 0);
    }
    path->discrete_mode_1c = value;
}

// FUNCTION: WIZ8 0x004AAA50
float PathAIGetScale(W8PathAI* path)
{
    return path->speed;
}

// FUNCTION: WIZ8 0x004a9750
W8PathAI* CreateRecord(int unused)
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
