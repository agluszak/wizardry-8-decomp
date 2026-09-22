#ifndef WIZ8_ENGINE_CODE_PATH_AI_H
#define WIZ8_ENGINE_CODE_PATH_AI_H

#include "wiz8/geometry.h"
#include "wiz8/vector.h"

class srNode;
class W8AnimRepBase005EC1D8;

/* Engine Code\PathAI.cpp.  The assertion-backed `pPathAI` identity and the
   five consumers below establish these offsets; unresolved members remain
   address-qualified rather than receiving speculative pathfinding names. */

/* The tagged AI record family W8GrObject stores at +0x0c: the leading byte
   selects the record type, kind 0 being W8PathAI and kind 3 being
   Missile.cpp's W8AIMissile. The dispatcher entry points below take this base;
   operations on record-specific tails keep the concrete type. */
struct W8AIRecord {
    unsigned char kind_00; /* 0x00 */
};

struct W8PathAI : W8AIRecord {
    /* Serialized record selector: value 2 adds a per-point scale array. */
    unsigned char version_01;
    unsigned char padding_02[2];
    /* Normalized for interpolated movement, point units in discrete mode. */
    float position;                                 /* 0x04 */
    unsigned int unknown_08;                        /* 0x08: serialized; no recovered consumer */
    W8GrowableVector<srVector3T<float>*>* nodes_0c; /* 0x0c */
    int value_10;                                   /* 0x10 */
    /* 0x004A98C0 sizes both from the node count: 0x24 a record here, and a
       srVector3T<float> each in the render array. */
    srMatrix3T<float>* rotations_14; /* 0x14 */
    srVector3T<float>* scales_18;    /* 0x18 */
    unsigned char discrete_mode_1c;  /* 0x1c */
    unsigned char padding_1d[3];
    unsigned int point_index;      /* 0x20 */
    float interpolation_fraction;  /* 0x24 */
    unsigned int last_update_tick; /* 0x28 */
    float speed;                   /* 0x2c */
    float distance_travelled;      /* 0x30 */
    float total_length;            /* 0x34 */
    unsigned char looping;         /* 0x38 */
    unsigned char step_by_node_39; /* 0x39 */
    unsigned char animated_3a;     /* 0x3a */
    /* When set the emitter target is pitched upright (rotateX pi/2); camera
       paths raise it. */
    unsigned char upright_3b;
    unsigned char timed_3c; /* 0x3c */
    unsigned char padding_3d[3];
};

static_assert(sizeof(W8GrowableVector<srVector3T<float>*>) == 0x10,
              "PathAI_vector_size_must_be_0x10");
static_assert(sizeof(W8PathAI) == 0x40, "W8PathAI_size_must_be_0x40");

/* Tagged-record dispatchers: the body switches on kind_00 and hands the
   record to the path or missile implementation. */
unsigned char PathAIUpdate004A9260(W8AIRecord* record, signed char direction);
void PathAIResetRecord004A9720(W8PathAI* path);
unsigned char PathAIRecordFlag004A9740(const W8AIRecord* record);
void PathAIApplyToRep004A91F0(W8AIRecord* record, W8AnimRepBase005EC1D8* representation);
/* Places one srNode (model instance, light, camera, …) through a path. The
   body only calls srNode child/location/rotation/scale APIs; retail callers
   pass those node kinds interchangeably. */
void PathAIApply004AA520(W8PathAI* path, srNode* node); /* 0x004AA520 */
float PathAIGetScale004AAA50(W8PathAI* path);           /* 0x004AAA50 */
void DestroyPathAI004A9810(W8PathAI* path);
void PathAIClearOwned004A9BB0(W8PathAI* path);
void PathAISetAnimated004A9B90(W8PathAI* path, unsigned char value);
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
void PathAISetLooping004AA9D0(W8PathAI* path, unsigned char value);
void PathAISetScale004AA9C0(W8PathAI* path, float value);
void PathAISetDiscreteMode004AAA10(W8PathAI* path, unsigned char value);
unsigned char LoadPathAI004A92A0(W8PathAI** path, int handle);
unsigned char PathAIAddPoint004A9C30(W8PathAI* path, const srVector3T<float>* point);

/* Build a zeroed 0x40-byte path and its position-pointer vector. Every caller
   pushes an argument the factory never reads. */
W8PathAI* CreateRecord004A9750(int unused);

/* The two operations stLight applies to the path it owns at +0x244. The
   release is DestroyPathAI004A9810's body behind an extra `kind_00 == 0`
   guard; the clone allocates a fresh 0x40-byte record and deep-copies the
   node vector and both trailing arrays. */
void DestroyOwnedPathAI004A9110(W8PathAI* path);

/* The dispatcher every AI-record copy goes through; the kind_00 tag, not the
   declaration, decides which concrete record it clones. */
W8AIRecord* CloneAIRecord004A91C0(const W8AIRecord* record);
W8PathAI* ClonePathAI004A98C0(const W8PathAI* path);

void PathAIAdvanceByDistance004A9FE0(W8PathAI* path, float value);

#endif
