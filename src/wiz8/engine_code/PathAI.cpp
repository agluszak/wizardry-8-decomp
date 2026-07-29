#include "wiz8/engine_code/PathAI.h"
#include "wiz8/sr_api.h"
#include "surrender/srHeap.h"

#include <windows.h>
#include <stdlib.h>

#define PATH_AI_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\PathAI.CPP"

extern "C" void NoOp(W8PathAI* path, W8PathRepresentation* representation);
extern unsigned char Function4A4CF0(W8PathAI* path);
extern void Function4A9FE0(W8PathAI* path, float value);
extern float g_float_005ebb34;
extern float g_float_005ebb38;
extern float g_float_005ebc38;
extern double g_double_005ebe80;
extern float g_float_005ec128;
extern double g_double_005ec3b0;

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

// FUNCTION: WIZ8 0x004a9720
void PathAIResetRecord004A9720(W8PathRecord004A9750* record)
{
    if (record != 0 && record->flag_00 == 0) {
        record->value_04 = 0;
    }
}

// FUNCTION: WIZ8 0x004a9740
unsigned char PathAIRecordFlag004A9740(const W8PathRecord004A9750* record)
{
    return record->flag_00;
}

// FUNCTION: WIZ8 0x004a91f0
void PathAIApplyToRep004A91F0(W8PathAI* path, W8PathRepresentation* representation)
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
    PathAIPosition004AA370(path, &representation->value_1c);
    representation->value_04 = representation->value_1c;
}

// FUNCTION: WIZ8 0x004a9810
void DestroyPathAI004A9810(W8PathAI* path)
{
    W8GrowableVector<W8PathVector3*>* nodes;

    if (path != 0) {
        nodes = path->nodes_0c;
        if (nodes != 0) {
            while (nodes->count != 0) {
                srHeap.free(nodes->RemoveAt(nodes->GetCount() - 1));
                nodes = path->nodes_0c;
            }
            if (path->allocation_14 != 0) {
                free(path->allocation_14);
                path->allocation_14 = 0;
            }
            delete path->nodes_0c;
        }
        if (path->allocation_14 != 0) {
            free(path->allocation_14);
        }
        if (path->render_allocation_18 != 0) {
            srHeap.free(path->render_allocation_18);
        }
        free(path);
    }
}

// FUNCTION: WIZ8 0x004a9bb0
void PathAIClearOwned004A9BB0(W8PathAI* path)
{
    W8GrowableVector<W8PathVector3*>* nodes;

    if (path != 0) {
        nodes = path->nodes_0c;
        if (nodes != 0) {
            while (nodes->count != 0) {
                srHeap.free(nodes->RemoveAt(nodes->GetCount() - 1));
                nodes = path->nodes_0c;
            }
        }
        if (path->allocation_14 != 0) {
            free(path->allocation_14);
            path->allocation_14 = 0;
        }
    }
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
unsigned char PathAINextPoint004A9E90(W8PathAI* path, W8PathVector3* point)
{
    W8PathVector3* source;

    if (path == 0 || path->nodes_0c == 0) {
        return 0;
    }
    if (path->value_20 >= path->nodes_0c->count) {
        if (path->flag_38 == 0) {
            return 0;
        }
        path->value_20 = 0;
    }
    W8PathVector3** slot = path->nodes_0c->data;
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
void PathAIPosition004AA370(W8PathAI* path, W8PathVector3* value)
{
    int index;
    W8PathVector3* first;
    W8PathVector3* second;
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
                *value = *first;
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
        *value = *first;
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
