#ifndef WIZ8_ENGINE_CODE_PATH_AI_H
#define WIZ8_ENGINE_CODE_PATH_AI_H

#include "wiz8/geometry.h"
#include "wiz8/vector.h"

class stModelInstance005EC7D0;
class W8AnimRepBase005EC1D8;

/* Engine Code\PathAI.cpp.  The assertion-backed `pPathAI` identity and the
   five consumers below establish these offsets; unresolved members remain
   address-qualified rather than receiving speculative pathfinding names. */

struct W8PathAI {
    unsigned char kind_00;               /* 0x00 */
    unsigned char unknown_01[3];
    float value_04;                      /* 0x04 */
    unsigned int unknown_08;             /* 0x08 */
    W8GrowableVector<srVector3T<float>*>* nodes_0c; /* 0x0c */
    int value_10;                        /* 0x10 */
    /* 0x004A98C0 sizes both from the node count: 0x24 a record here, and a
       srVector3T<float> each in the render array. */
    srMatrix3T<float>* rotations_14;     /* 0x14 */
    srVector3T<float>* scales_18;        /* 0x18 */
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

static_assert(sizeof(W8GrowableVector<srVector3T<float>*>) == 0x10,
              "PathAI_vector_size_must_be_0x10");
static_assert(sizeof(W8PathAI) == 0x40, "W8PathAI_size_must_be_0x40");

unsigned char PathAIUpdate004A9260(W8PathAI* path, signed char direction);
void PathAIResetRecord004A9720(W8PathAI* path);
unsigned char PathAIRecordFlag004A9740(const W8PathAI* path);
void PathAIApplyToRep004A91F0(
    W8PathAI* path, W8AnimRepBase005EC1D8* representation);
/* Places one model instance through a path. AnimObj.cpp and GrCycle.cpp are
   both callers, so the declaration lives with the path's owner. */
extern "C" void PathAIApply004AA520(
    W8PathAI* path, stModelInstance005EC7D0* instance);   /* 0x004AA520 */
void DestroyPathAI004A9810(W8PathAI* path);
void PathAIClearOwned004A9BB0(W8PathAI* path);
void PathAISetFlag3A004A9B90(W8PathAI* path, unsigned char value);
void PathAIEnableTimedMode004A9BA0(W8PathAI* path);
void PathAIResetTick004A9C20(W8PathAI* path);
float PathAIGetValue004A9E70(W8PathAI* path);
unsigned char PathAINextPoint004A9E90(W8PathAI* path, srVector3T<float>* point);
unsigned char PathAIIsComplete004A9EF0(W8PathAI* path);
unsigned int PathAIEntryCount004A9F20(W8PathAI* path);
void PathAISetValue004A9F60(W8PathAI* path, float value);
void PathAIAdvanceNormalized004AA160(W8PathAI* path, float amount);
int PathAITick004AA1F0(W8PathAI* path, signed char direction);
void PathAIPosition004AA370(W8PathAI* path, srVector3T<float>* value);
void PathAISetFlag38004AA9D0(W8PathAI* path, unsigned char value);
void PathAISetScale004AA9C0(W8PathAI* path, float value);
void PathAISetFlag1C004AAA10(W8PathAI* path, unsigned char value);
unsigned char LoadPathAI004A92A0(W8PathAI** path, int handle);
unsigned char PathAIAddPoint004A9C30(
    W8PathAI* path, const srVector3T<float>* point);

/* Build a zeroed 0x40-byte path and its position-pointer vector. Every caller
   pushes an argument the factory never reads. */
W8PathAI* CreateRecord004A9750(int unused);

/* The two operations stLight applies to the path it owns at +0x244. The
   release is DestroyPathAI004A9810's body behind an extra `kind_00 == 0`
   guard; the clone allocates a fresh 0x40-byte record and deep-copies the
   node vector and both trailing arrays. Their bodies are not recovered yet -
   only the signatures stLight's lifecycle needs. */
void DestroyOwnedPathAI004A9110(W8PathAI* path);

/* The AI records W8GrObject holds at +0x0c are a tagged family: the leading
   byte selects the record type, kind 0 being W8PathAI and kind 3 being
   Missile.cpp's W8AIMissile. This is the dispatcher every copy goes through, so
   the pointer is deliberately untyped here - the tag, not the declaration,
   decides which record it is. */
void* CloneAIRecord004A91C0(void* record);
W8PathAI* ClonePathAI004A98C0(const W8PathAI* path);

#endif
