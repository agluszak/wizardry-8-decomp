#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"

class GDProp;

/* Released through its own deleting slot by 0x00457B10; nothing else about it
   is established. */
class W8PathOwned054 {
public:
    virtual ~W8PathOwned054();
};

/* The 0x58-byte state object the pathing constructor builds and 0x00457B10
   gives back with a plain operator delete, so it has no destructor of its own.
   Its constructor does nothing but run the one at 0x004CCCB0. */
class W8PathState004CAE40 {
public:
    W8PathState004CAE40();               /* 0x004CAE40 */
};
struct W8Position;
struct W8NavigatorMovementState;

class W8OctreeObjectRegistry {
public:
    /* Returns whether the pairing ended up recorded; every recovered caller
       discards it. */
    unsigned char RegisterObjectCell(int kind, int id, const int* point);
};

/* The cell walk 0x004362D0 builds and both line-of-sight bodies step: an
   ordinary 3D Bresenham over octree cells. One axis drives; the other two each
   carry a delta, an accumulator and the reset the accumulator takes when it
   goes negative, which is what makes the two triples symmetric. */
struct W8OctreeWalk {
    int cell_00[3];                      /* 0x00: the cell the walk starts in */
    int step_0c[3];                      /* 0x0c: +1 or -1 per axis */
    int major_axis_18;                   /* 0x18 */
    int minor_axis_1c;                   /* 0x1c: (major + 1) % 3 */
    int minor_axis_20;                   /* 0x20: (major + 2) % 3 */
    int count_24;                        /* 0x24: cells to visit */
    int error_delta_28;                  /* 0x28 */
    int error_2c;                        /* 0x2c */
    int error_reset_30;                  /* 0x30 */
    int error_delta_34;                  /* 0x34 */
    int error_38;                        /* 0x38 */
    int error_reset_3c;                  /* 0x3c */
};

static_assert(sizeof(W8OctreeWalk) == 0x40, "W8OctreeWalk_must_be_0x40");

/* The pathing service the octree builds when its file carries one. Its own
   constructor at 0x004578E0 initialises through 0x238 and ReadOctFile allocates
   0x240, which is what fixes the extent; only the fields those two bodies and
   the path lookup reach are named. */
class W8PathingService {
public:
    W8PathingService();                  /* 0x004578E0 */
    unsigned int FindPathHandle(
        const unsigned char* path_name,
        unsigned short* path_bounds,
        float* path_range);              /* 0x00457CF0 */
    /* Neither takes a prop: both walk the service's own surface and edge
       tables, and their receiver is the service. */
    /* The two operations W8Octree::AdvanceNavigator delegates to: it loads
       the service into ecx before either call, so both are its methods and
       not the free functions they were declared as. */
    unsigned int StepAlongPath004669B0(
        W8NavigatorMovementState* movement, float radius, float separation);
    unsigned int StepMonsterAlongPath00467150(
        W8NavigatorMovementState* movement, float radius, float separation);
    void LinkSurfaces00460020();          /* 0x00460020 */
    void LinkEdges004600B0();             /* 0x004600B0 */
    /* Takes the size, two loose values, the six-float bounds block out of the
       octree header, and the level name the octree already owns. */
    void ConfigureForLevel(
        int size, float grid_scale, int value_28, const float* bounds,
        const char* name);                /* 0x00458A50 */
    unsigned char Load00458CE0(int handle); /* 0x00458CE0 */
    /* Not a destructor: nothing restores a vtable and the object is left
       holding dangling pointers, exactly as BitArray::FreeIndex does. */
    void Release00457B10();               /* 0x00457B10 */

    unsigned int m_positional_000;
    int size_004;                        /* 0x04 */
    int m_positional_008;
    /* ReadOctFile tests this beside flag_1c8 before settling a portal. */
    unsigned int m_ulNumSurfaces;        /* 0x0c */
    unsigned int m_ulNumEdges;           /* 0x10 */
    int m_positional_014;
    int m_positional_018;
    /* The grid divisor both linking walks divide by. */
    float grid_scale_01c;                /* 0x1c */
    float span_020;                      /* 0x20 */
    short cell_count_024;                /* 0x24 */
    unsigned short m_padding_026;
    int value_028;                       /* 0x28 */
    float level_bounds[6];                 /* 0x2c */
    /* Four malloc'd tables and one polymorphic object, all released by
       0x00457B10 - the first four with free, the last through its own
       deleting slot. */
    void* m_owned_044;                   /* 0x44 */
    /* Surfaces are 0x28 bytes apart, edges 0xe; an edge names two surfaces by
       index in its two shorts at +4 and +6. */
    unsigned char* m_pSurfaces_048;      /* 0x48 */
    unsigned char* m_pEdges_04c;         /* 0x4c */
    void* m_owned_050;                   /* 0x50 */
    class W8PathOwned054* m_owned_054;   /* 0x54 */
    BitArray* m_owned_058;               /* 0x58 */
    BitArray* m_owned_05c;               /* 0x5c */
    BitArray* m_owned_060;               /* 0x60 */
    /* Two hash indexes the loader builds and 0x00457B10 tears down the same
       way DestroyIndex does. */
    void* m_pIndex_064;                  /* 0x64 */
    const char* level_name;                /* 0x68 */
    void* m_owned_06c;                   /* 0x6c */
    unsigned int m_positional_070;       /* 0x70: starts 0x501502f9 */
    void* m_pIndex_074;                  /* 0x74 */
    unsigned char m_positional_078[0x14];
    unsigned char flag_08c;              /* 0x8c */
    unsigned char m_positional_08d[0xf];
    unsigned char flag_09c;              /* 0x9c */
    unsigned char m_positional_09d[7];
    unsigned char flag_0a4;              /* 0xa4 */
    unsigned char m_padding_0a5[3];
    int m_positional_0a8;
    int m_positional_0ac;
    int m_positional_0b0;
    int m_positional_0b4;
    int m_positional_0b8;
    int m_positional_0bc;
    int m_positional_0c0;
    int m_positional_0c4;
    void* m_owned_0c8;                   /* 0xc8 */
    int m_positional_0cc;
    int m_positional_0d0;
    unsigned char m_positional_0d4[0xf4];
    unsigned char flag_1c8;              /* 0x1c8 */
    unsigned char flag_1c9;
    unsigned char flag_1ca;
    unsigned char flag_1cb;
    unsigned char flag_1cc;
    unsigned char m_padding_1cd;
    unsigned short value_1ce;            /* 0x1ce: starts 4 */
    int m_positional_1d0;
    unsigned short value_1d4;
    unsigned short value_1d6;
    unsigned short value_1d8;
    unsigned char m_positional_1da[0x3a];
    void* m_owned_214;                   /* 0x214 */
    int m_positional_218;
    /* The conditional path tables. ReadPathNodes at 0x00458CE0 asserts on the
       first by name and names the other four in its own failure messages: a
       lookup, a frame, a key and a value array, sized from the two counts. The
       entries are 0x44 bytes and FindPathHandle scans them by name. */
    unsigned char* m_pCondPaths;         /* 0x21c */
    int m_ulNumCondPaths;                /* 0x220 */
    int m_ulNumCondLookup;               /* 0x224 */
    int m_ulNumCondKeys;                 /* 0x228 */
    int* m_pCondLookup;                  /* 0x22c */
    short* m_pCondFrames;                /* 0x230 */
    int* m_pCondKeys;                    /* 0x234 */
    int* m_pCondValues;                  /* 0x238 */
    int m_positional_23c;
};

static_assert(sizeof(W8PathingService) == 0x240,
              "W8PathingService_must_be_0x240");

/* Engine Code\Octree.cpp. LoadWorld allocates exactly 0x29c bytes. This object
   is deliberately non-polymorphic: every owner calls the complete teardown at
   0x0042DE60 and then operator delete separately. */
class W8Octree {
public:
    W8Octree(const char* path, void** game_data);
    void Reset();
    void Initialize(const void* header);
    ~W8Octree();
    void AddLoadedProp(void* prop);
    void AddLoadedParticle(void* particle);
    void SetVisitedSet0042E3E0(BitArray* visited);
    int MarkVisited0042E400(int offset);
    void AddCollidablePropBounds(
        int index, const srVector3T<float>* bounds);
    void VisitPointCopy0042E620(
        unsigned short location_id, srVector3T<float>* position);
    void UpdateMonsterLocation(
        unsigned short location_id, const W8Position* position);
    bool HasLineOfSight(
        const W8Position* from, W8Position* to, char allow_fallback);
    short TraceLineOfSight(
        const W8Position* from, const W8Position* to, char trace_world,
        int from_location_id, int to_location_id, char visit_octree,
        int trace_mode);
    void AdjustPortalDestination(
        W8Position* destination, const W8Position* source);
    void BuildCellWalk(
        const W8Position* from, const W8Position* to, W8OctreeWalk* walk);
    unsigned int AdvanceNavigator(
        W8NavigatorMovementState* movement,
        float radius, float separation);
    void QueueOctreeKind130042E810(
        int id, const srVector3T<float>* position);
    void Function0042F7E0();

    bool HasLoadError() const { return (m_flags_000 & 0x80000000) != 0; }
    unsigned long GetMeshCount() const { return m_mesh_count_074; }

public:
    unsigned long m_flags_000;
    unsigned char m_positional_004[8];
    srVector3T<float> octree_origin_00c;
    unsigned char m_positional_018[0x1c];
    /* 0x00434A30 clamps a world-space Y against this before settling it, which
       is what makes it a height ceiling rather than one more opaque dword. */
    float height_limit_034;              /* 0x34 */
    unsigned char m_positional_038[0x24];
    /* ReadOctFile allocates the region array here. */
    void* m_pRegions_05c;                /* 0x5c */
    unsigned char m_positional_060[0x10];
    float octree_cell_size_070;
    unsigned long m_mesh_count_074;
    unsigned char m_positional_078[0x24];
    void* m_owned_09c;
    void* m_owned_0a0;
    unsigned long m_positional_0a4;
    unsigned long m_positional_0a8;
    unsigned long m_positional_0ac;
    void* m_owned_0b0;
    unsigned long m_positional_0b4;
    unsigned long m_positional_0b8;
    W8OctreeObjectRegistry* object_registry;
    void* m_owned_0c0;
    unsigned char m_fAccumulating;
    unsigned char m_positional_0c5[3];
    unsigned long m_positional_0c8;
    unsigned long m_positional_0cc;
    void* m_owned_0d0;
    void* m_owned_0d4;
    void* m_owned_0d8;
    /* Six original member names, from ReadOctFile's own assertion text at
       0x0042C68A, 0x0042C70C, 0x0042C7AB, 0x0042C850, 0x0042C8F5 and
       0x0042CAA4. The us prefix is the image's own, so the four lookup and
       link tables are unsigned short arrays. */
    BitArray* m_pAlphaBits;              /* 0xdc */
    unsigned short* m_pusMeshParticleLookup; /* 0xe0 */
    unsigned short* m_pusMeshParticles;   /* 0xe4 */
    unsigned short m_positional_0e8;
    unsigned short m_padding_0ea;
    unsigned short* m_pusMeshPropLookup;  /* 0xec */
    unsigned short* m_pusMeshProps;       /* 0xf0 */
    unsigned short m_positional_0f4;
    unsigned short m_padding_0f6;
    unsigned long m_ulNumParticles;
    BitArray* m_owned_0fc;
    BitArray* m_owned_100;
    BitArray* m_owned_104;
    BitArray* m_owned_108;
    BitArray* m_owned_10c;
    BitArray* m_owned_110;
    void** m_papProps;
    void** m_papParticles;
    unsigned short m_usNumPropsLoaded;
    unsigned short m_usNumParticlesLoaded;
    int current_sector;
    unsigned long m_positional_124;
    unsigned long m_positional_128;
    void* m_owned_12c;
    void* m_owned_130;
    unsigned long m_positional_134;
    unsigned long m_positional_138;
    unsigned long m_positional_13c;
    unsigned long m_positional_140;
    unsigned long m_positional_144;
    void* m_owned_148;
    unsigned char* m_pfRegsVisited;
    void* m_owned_150;
    BitArray* m_owned_154;
    unsigned long m_positional_158;
    BitArray* m_owned_15c;
    BitArray* m_owned_160;
    BitArray* m_owned_164;
    unsigned char m_positional_168;
    unsigned char m_positional_169;
    unsigned char m_positional_16a;
    unsigned char m_padding_16b;
    unsigned char m_positional_16c;
    unsigned char m_positional_16d;
    unsigned char m_padding_16e[2];
    unsigned long m_positional_170;
    void* m_sr_owned_174;
    unsigned long m_positional_178;
    unsigned long m_positional_17c;
    W8PathingService* pathing_180;
    int mark_base_184;
    unsigned long m_ulNumProps;
    /* Named m_pPropSunBits by ReadOctFile's assertion at 0x0042CAA4. The
       earlier `visited` reading came from 0x0042E3E0's parameter, not from
       the image, and the assertion outranks it. */
    BitArray* m_pPropSunBits;            /* 0x18c */
    BitArray* m_owned_190;
    BitArray* m_owned_194;
    BitArray* m_owned_198;
    BitArray* m_owned_19c;
    BitArray* m_owned_1a0;
    BitArray* m_owned_1a4;
    unsigned long m_positional_1a8;
    unsigned long m_positional_1ac;
    unsigned long m_positional_1b0;
    unsigned long m_positional_1b4;
    unsigned long m_positional_1b8;
    /* The two line-of-sight bodies hand this pair to their result test
       together with the byte at +0x134, so 0x1bc is storage rather than
       the GD object table that used to be declared here. */
    unsigned long m_positional_1bc;
    unsigned long* m_aulGDObjs;          /* 0x1c0 */
    unsigned char m_positional_1c4[0xb8];
    unsigned long m_positional_27c;
    unsigned long m_positional_280;
    unsigned long m_positional_284;
    unsigned long m_positional_288;
    unsigned long m_positional_28c;
    unsigned long m_positional_290;
    unsigned char m_positional_294;
    unsigned char m_padding_295;
    unsigned short m_positional_296;
    unsigned char m_padding_298;
    unsigned char m_positional_299;
    unsigned char m_padding_29a[2];
};

/* The shared spatial-service pointer at 0x006598A4 is the active W8Octree.
   Its callers reach fields at +0x70/+0x120/+0x180, while 0x0042E620 proves
   that the same receiver dispatches ordinary W8Octree methods. */
/* The octree's open-chained hash map: bucket heads, {next_index, key, value}
   entries, a free-list head threaded through the same next_index field, and a power-of-two
   bucket count. Both Octree.cpp and OctPath.cpp build and walk these. */
struct W8OctreeIndex {
    void* bucket_heads;
    void* entries;
    long free_head;
    unsigned long bucket_count;
};

struct W8OctreeEntry {
    int next_index;
    unsigned int key;
    int value;
};

void InsertEntry0055DBB0(
    W8OctreeIndex* index, const unsigned int* key, const int* value);

extern W8Octree* g_octree_6598a4;

/* Drops one world-space position onto the ground below it, reporting through
   the second argument whether anything was hit - a byte, and a float limit:
   0x00434A30 stores zero into that slot with a byte move and pushes its 500 as
   a single. ItemManager.cpp settles dropped items with it and Octree.cpp
   settles portal endpoints, so the name stays neutral. */
unsigned char SettleToGround00433820(
    float* position, unsigned char* out_hit, int mode, float limit);

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");
