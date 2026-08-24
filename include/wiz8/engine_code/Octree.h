#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/vector.h"

class GDProp;
class W8PathingService;
struct W8NavigatorMovementState;
struct W8OctBuildNode00446330;

class W8OctreeObjectRegistry {
public:
    /* Returns whether the pairing ended up recorded; every recovered caller
       discards it. */
    unsigned char RegisterObjectCell(int kind, int id, const int* point);
    unsigned char UpdateObjectCell00436B90(int kind, int id, const int* point);
    unsigned char RemoveObjectCell00436DC0(int kind, int id);
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

/* The two compact records stored in an OCT file.  A branch is its two shorts
   followed by eight child indices; a leaf retains offsets into the region and
   two polygon-index streams.  The rest of the leaf is still positional. */
struct W8OctPreTreeBranch {
    unsigned short positional_00;
    unsigned short positional_02;
    unsigned long children_04[8];
};

struct W8OctPreTreeLeaf {
    unsigned long positional_00;
    unsigned long region_offset_04;
    unsigned long polygon_offset_08;
    unsigned long gd_polygon_offset_0c;
    unsigned char positional_10[0x18];
};

static_assert(sizeof(W8OctPreTreeBranch) == 0x24,
              "W8OctPreTreeBranch_must_be_0x24");
static_assert(sizeof(W8OctPreTreeLeaf) == 0x28,
              "W8OctPreTreeLeaf_must_be_0x28");

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
    void WorldPositionToCell00431440(
        const srVector3T<float>* position, int* point);
    unsigned long FindLeaf00433660(const int* point);
    void UpdateMonsterLocation(
        unsigned short location_id, const srVector3T<float>* position);
    bool HasLineOfSight(
        const srVector3T<float>* from, srVector3T<float>* to, char allow_fallback);
    short TraceLineOfSight(
        const srVector3T<float>* from, const srVector3T<float>* to, char trace_world,
        int from_location_id, int to_location_id, char visit_octree,
        int trace_mode);
    void AdjustPortalDestination(
        srVector3T<float>* destination, const srVector3T<float>* source);
    void BuildCellWalk(
        const srVector3T<float>* from, const srVector3T<float>* to, W8OctreeWalk* walk);
    unsigned int AdvanceNavigator(
        W8NavigatorMovementState* movement,
        float radius, float separation);
    void QueueOctreeKind130042E810(
        int id, const srVector3T<float>* position);
    int QueryObjects0042F280(
        int** objects,
        const srVector3T<float>* lower,
        const srVector3T<float>* upper,
        int kind,
        int excluded);
    void AdjustPosition00431DA0(
        srVector3T<float>* position, unsigned int mode);
    void Function0042F7E0();

    bool HasLoadError() const {
        return (spatial_000.flags_00 & 0x80000000) != 0;
    }
    unsigned long GetMeshCount() const { return spatial_000.positional_74; }

public:
    /* Same proven 0x9c value used by the level build tree.  Construction and
       teardown operate on the offset-zero subobject, but current evidence does
       not distinguish first-member composition from inheritance, so the
       declaration makes the narrower composition claim. */
    W8OctSpatialState0046CCC0 spatial_000;
    W8OctPreTreeBranch* m_owned_09c;
    W8OctPreTreeLeaf* m_owned_0a0;
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
    unsigned long* m_owned_0d0;
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
    unsigned long* m_owned_12c;
    void* m_owned_130;
    unsigned long m_positional_134;
    unsigned long m_positional_138;
    unsigned long m_positional_13c;
    unsigned long m_positional_140;
    unsigned long m_positional_144;
    unsigned short* m_owned_148;
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

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");

/* Engine Code\OctPreTree.cpp's build-time runtime tree.  The constructor at
   0x004679E0 invokes W8Octree's constructor at offset zero, and its sole
   caller allocates 0x3bc bytes before invoking it.  Only the suffix reached by
   the destructive OctBuildPreTree conversion is named here. */
class W8OctPreTree004679E0 : public W8Octree {
public:
    W8OctPreTree004679E0();

    W8HashTable<unsigned short, unsigned long>* positional_29c;
    unsigned long positional_2a0;
    unsigned char positional_2a4[0xfc];
    unsigned long polygon_cursor_3a0;
    void* positional_3a4;
    unsigned long positional_3a8;
    unsigned long positional_3ac;
    unsigned long positional_3b0;
    unsigned long positional_3b4;
    W8GrowableVector<void*>* positional_3b8;
};

static_assert(sizeof(W8OctPreTree004679E0) == 0x3bc,
              "W8OctPreTree004679E0_must_be_0x3bc");

/* The shared spatial-service pointer at 0x006598A4 is the active W8Octree.
   Its callers reach fields at +0x70/+0x120/+0x180, while 0x0042E620 proves
   that the same receiver dispatches ordinary W8Octree methods. */
/* The octree's open-chained hash map: bucket heads, {next_index, key, value}
   entries, a free-list head threaded through the same next_index field, and a power-of-two
   bucket count. Both Octree.cpp and OctPath.cpp build and walk these. */
struct W8OctreeIndex {
    int FindNextEntry00438D50(const unsigned int* key, int previous);

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
void RemoveEntry00438C90(
    W8OctreeIndex* index, const unsigned int* key, const int* value);
void GrowIndex00439290(W8OctreeIndex* index);
unsigned int __stdcall OctreeTraverseKind12(
    void* walker,
    void* lower,
    void* upper,
    unsigned short limit);

extern W8Octree* g_octree_6598a4;

/* Drops one world-space position onto the ground below it, reporting through
   the second argument whether anything was hit - a byte, and a float limit:
   0x00434A30 stores zero into that slot with a byte move and pushes its 500 as
   a single. ItemManager.cpp settles dropped items with it and Octree.cpp
   settles portal endpoints, so the name stays neutral. */
unsigned char SettleToGround00433820(
    float* position, unsigned char* out_hit, int mode, float limit);

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");

