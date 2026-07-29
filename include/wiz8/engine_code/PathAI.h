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
    W8GrowableVector<W8PathVector3*>* nodes_0c; /* 0x0c */
    unsigned char unknown_10[4];
    void* allocation_14;                 /* 0x14 */
    void* render_allocation_18;          /* 0x18 */
    unsigned char flag_1c;               /* 0x1c */
    unsigned char unknown_1d[3];
    unsigned int value_20;               /* 0x20 */
    float value_24;                      /* 0x24 */
    unsigned int tick_28;                /* 0x28 */
    float value_2c;                      /* 0x2c */
    float value_30;                      /* 0x30 */
    float scale_34;                      /* 0x34 */
    unsigned char flag_38;               /* 0x38 */
    unsigned char flag_39;               /* 0x39 */
    unsigned char flag_3a;               /* 0x3a */
    unsigned char unknown_3b;
    unsigned char flag_3c;               /* 0x3c */
    unsigned char unknown_3d[3];
};

struct W8PathRepresentation {
    unsigned char unknown_00[4];
    W8PathVector3 value_04;               /* 0x04 */
    unsigned char unknown_10[0x0c];
    W8PathVector3 value_1c;               /* 0x1c */
};

class W8VectorElement005ECF00;

struct W8PathRecord004A9750 {
    unsigned char flag_00;
    unsigned char unknown_01[3];
    int value_04;
    unsigned char unknown_08[4];
    W8GrowableVector<W8VectorElement005ECF00*>* list_0c;
    unsigned char unknown_10[0x30];
};

static_assert(sizeof(W8GrowableVector<W8PathVector3*>) == 0x10,
              "PathAI_vector_size_must_be_0x10");
static_assert(sizeof(W8PathAI) == 0x40, "W8PathAI_size_must_be_0x40");
static_assert(sizeof(W8PathRepresentation) == 0x28, "W8PathRepresentation_size_must_be_0x28");
static_assert(sizeof(W8PathRecord004A9750) == 0x40, "W8PathRecord004A9750_size_must_be_0x40");

unsigned char PathAIUpdate004A9260(W8PathAI* path, signed char direction);
void PathAIResetRecord004A9720(W8PathRecord004A9750* record);
unsigned char PathAIRecordFlag004A9740(const W8PathRecord004A9750* record);
void PathAIApplyToRep004A91F0(W8PathAI* path, W8PathRepresentation* representation);
void DestroyPathAI004A9810(W8PathAI* path);
void PathAIClearOwned004A9BB0(W8PathAI* path);
void PathAISetFlag3A004A9B90(W8PathAI* path, unsigned char value);
void PathAIEnableTimedMode004A9BA0(W8PathAI* path);
void PathAIResetTick004A9C20(W8PathAI* path);
float PathAIGetValue004A9E70(W8PathAI* path);
unsigned char PathAINextPoint004A9E90(W8PathAI* path, W8PathVector3* point);
unsigned char PathAIIsComplete004A9EF0(W8PathAI* path);
unsigned int PathAIEntryCount004A9F20(W8PathAI* path);
void PathAISetValue004A9F60(W8PathAI* path, float value);
void PathAIAdvanceNormalized004AA160(W8PathAI* path, float amount);
int PathAITick004AA1F0(W8PathAI* path, signed char direction);
void PathAIPosition004AA370(W8PathAI* path, W8PathVector3* value);
void PathAISetFlag38004AA9D0(W8PathAI* path, unsigned char value);
void PathAISetScale004AA9C0(W8PathAI* path, float value);
void PathAISetFlag1C004AAA10(W8PathAI* path, unsigned char value);

#endif
