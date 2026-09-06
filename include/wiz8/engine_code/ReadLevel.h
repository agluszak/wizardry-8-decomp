#pragma once

#include "wiz8/engine_code/materials.h"
#include "wiz8/geometry.h"

struct W8World;
class srModelInstance;
class srNode;
class stParticle;
template <class T> class W8GrowableVector;

#pragma pack(push, 1)

/* ReadWorldParticles reads this record as one of four growing on-disk
   versions. The 0x225-byte extent and every named offset below come directly
   from the version-sized reads and subsequent uses in 0x004BD0D0. */
struct W8LevelParticleRecord004BD0D0 {
    char name[64];                         /* 0x000 */
    srVector3T<float> location;            /* 0x040 */
    float rotation_angle;                  /* 0x04c */
    srVector3T<float> rotation_axis;        /* 0x050 */
    unsigned char positional_05c[0x0c];    /* 0x05c */
    unsigned int particle_count;           /* 0x068 */
    float source_06c;                      /* 0x06c */
    float source_070;                      /* 0x070 */
    float source_074;                      /* 0x074 */
    int has_acceleration;                  /* 0x078 */
    srVector3T<float> acceleration;        /* 0x07c */
    int positional_088;                    /* 0x088 */
    int bounds_mode;                       /* 0x08c */
    srVector3T<float> bounds_origin;        /* 0x090 */
    float bounds_radius;                   /* 0x09c */
    srVector3T<float> bounds_extent;        /* 0x0a0 */
    unsigned int lifetime;                 /* 0x0ac */
    int velocity_mode;                     /* 0x0b0 */
    unsigned int emission_interval;        /* 0x0b4 */
    int positional_0b8;                    /* 0x0b8 */
    int placement_mode;                    /* 0x0bc */
    float placement_0c0;                   /* 0x0c0 */
    float placement_0c4;                   /* 0x0c4 */
    float placement_0c8;                   /* 0x0c8 */
    float particle_value;                  /* 0x0cc */
    int flutter_mode;                      /* 0x0d0 */
    float flutter_value;                   /* 0x0d4 */
    float flutter_period;                  /* 0x0d8 */
    int direction_mode;                    /* 0x0dc */
    float direction_0e0;                   /* 0x0e0 */
    float direction_0e4;                   /* 0x0e4 */
    int initially_active;                  /* 0x0e8 */
    W8MaterialRecord004B8A70 material;     /* 0x0ec */
    short value_216;                       /* 0x216 */
    int state_218;                         /* 0x218 */
    unsigned char value_21c;               /* 0x21c */
    int start_frame_21d;                   /* 0x21d, unaligned */
    int end_frame_221;                     /* 0x221 */
};

/* The preprocessing owner retains the version byte beside each particle
   record. Its 0x226 stride is walked directly by 0x004B3820. */
struct W8VersionedLevelParticleRecord {
    unsigned char version_000;
    W8LevelParticleRecord004BD0D0 particle_001;
};

#pragma pack(pop)

static_assert(sizeof(W8LevelParticleRecord004BD0D0) == 0x225,
              "W8LevelParticleRecord004BD0D0_size_must_be_0x225");
static_assert(sizeof(W8VersionedLevelParticleRecord) == 0x226,
              "W8VersionedLevelParticleRecord_size_must_be_0x226");

struct W8ReadLevelInfo {
    W8World* world;                      /* 0x00 */
    int hFile;                           /* 0x04 */
    const char* bitmap_folder;           /* 0x08 */
    const char* mesh_filename;           /* 0x0c */
};

unsigned char ReadLevel(
    W8World* world, int handle, unsigned char use_octree,
    const char* bitmap_folder);
unsigned char ReadSingleLevelMesh00485B20(
    W8ReadLevelInfo* info, srModelInstance** instance,
    int positional_0, int positional_1, const char* name,
    unsigned char load_materials);
unsigned char ReadMultipleLevelMeshes00488240(
    W8ReadLevelInfo* info, srModelInstance** instances,
    unsigned long count, const char* name);
unsigned char SkipSingleLevelMesh00487BD0(W8ReadLevelInfo* info);
unsigned char ReadWorldParticles004BD0D0(
    W8ReadLevelInfo* info,
    srNode* scene,
    W8GrowableVector<stParticle*>* particles);
void ReleaseReadMeshScratch004881D0();
unsigned char IsReadMeshMaterial00489AC0(const srClass* material);
