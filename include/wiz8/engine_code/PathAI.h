#ifndef WIZ8_ENGINE_CODE_PATH_AI_H
#define WIZ8_ENGINE_CODE_PATH_AI_H

#include "wiz8/vector.h"

/* Engine Code\PathAI.cpp.  The assertion-backed `pPathAI` identity and the
   five consumers below establish these offsets; unresolved members remain
   address-qualified rather than receiving speculative pathfinding names. */

struct W8PathVector3 {
    float x;
    float y;
    float z;
};

struct W8PathAI {
    unsigned char kind_00;               /* 0x00 */
    unsigned char unknown_01[3];
    float value_04;                      /* 0x04 */
    unsigned char unknown_08[4];
    W8GrowableVector<void*>* nodes_0c;    /* 0x0c */
    unsigned char unknown_10[4];
    void* allocation_14;                 /* 0x14 */
    void* render_allocation_18;          /* 0x18 */
    unsigned char unknown_1c[4];
    unsigned int value_20;               /* 0x20 */
    unsigned int value_24;               /* 0x24 */
    unsigned char unknown_28[8];
    unsigned int value_30;               /* 0x30 */
    float scale_34;                      /* 0x34 */
    unsigned char unknown_38[2];
    unsigned char flag_3a;               /* 0x3a */
};

struct W8PathRepresentation {
    unsigned char unknown_00[4];
    W8PathVector3 value_04;               /* 0x04 */
    unsigned char unknown_10[0x0c];
    W8PathVector3 value_1c;               /* 0x1c */
};

static_assert(sizeof(W8GrowableVector<void*>) == 0x10, "PathAI_vector_size_must_be_0x10");
static_assert(sizeof(W8PathAI) == 0x3c, "W8PathAI_size_must_be_0x3c");
static_assert(sizeof(W8PathRepresentation) == 0x28, "W8PathRepresentation_size_must_be_0x28");

void PathAIApplyToRep004A91F0(W8PathAI* path, W8PathRepresentation* representation);
void DestroyPathAI004A9810(W8PathAI* path);
void PathAIClearOwned004A9BB0(W8PathAI* path);
unsigned int PathAIEntryCount004A9F20(W8PathAI* path);
void PathAISetValue004A9F60(W8PathAI* path, float value);

#endif
