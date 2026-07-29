#include "wiz8/engine_code/PathAI.h"
#include "wiz8/sr_api.h"
#include "surrender/srHeap.h"

#include <stdlib.h>

#define PATH_AI_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\PathAI.CPP"

extern "C" void NoOp(W8PathAI* path, W8PathRepresentation* representation);
extern void Function4AA370(W8PathAI* path, W8PathVector3* value);
extern void Function4A9FE0(W8PathAI* path, float value);

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
    Function4AA370(path, &representation->value_1c);
    representation->value_04 = representation->value_1c;
}

// FUNCTION: WIZ8 0x004a9810
void DestroyPathAI004A9810(W8PathAI* path)
{
    W8GrowableVector<void*>* nodes;

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
    W8GrowableVector<void*>* nodes;

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
