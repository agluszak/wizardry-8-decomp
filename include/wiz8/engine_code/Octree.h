#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/vector.h"

class GDProp;
class W8Prop;
class stParticle;
class stModelInstance;
class srNode;
typedef W8HashTable<unsigned int, int> W8OctreeIndex;
typedef W8HashEntry<unsigned int, int> W8OctreeEntry;

class W8PathingService;
struct W8World;
struct W8GameData;
struct W8NavigatorMovementState;
struct W8OctBuildNode00446330;
struct W8OctRegionGameData;

/* The 0x30-byte ray state the octree line/probe walks share: a segment
   (start_00, end_0c), a fixed-length march step_18 (end-start scaled by
   g_double_005ebc30 / length), the closest accepted hit distance at +0x24,
   the segment length at +0x28 and a per-probe flag word at +0x2c.
   The default constructor at 0x004577C0 seeds +0x24 with the 0x60AD78EC
   "no hit" sentinel. The three segment-seeding bodies are distinct source
   operations despite identical instructions: SegmentClear constructs first
   and then calls Seed (0x00457580), ordinary trace sites call the two-argument
   constructor (0x00457640), and existing traces call Reseed (0x00457700).
   Their call sites distinguish construction from mutation; they are not
   duplicate header emissions of one constructor. */
struct W8OctreeTrace {
    srVector3T<float> start_00;
    srVector3T<float> end_0c;
    srVector3T<float> step_18;
    float hit_limit_24;
    float length_28;
    unsigned short state_2c;
    unsigned short pad_2e;

    W8OctreeTrace(); /* 0x004577C0 */
    /* Copies the endpoints, stores the normalized direction, seeds
       hit_limit_24/length_28 and clears state_2c. */
    W8OctreeTrace(const srVector3T<float>* from, const srVector3T<float>* to); /* 0x00457640 */
    /* Identical body to the (from, to) constructor; the only retail caller is
       the OctPreTree segment-occlusion walk at 0x00467BB0. */
    void Seed(const srVector3T<float>* from, const srVector3T<float>* to);   /* 0x00457580 */
    void Reseed(const srVector3T<float>* from, const srVector3T<float>* to); /* 0x00457700 */
};

static_assert(sizeof(W8OctreeTrace) == 0x30, "W8OctreeTrace_must_be_0x30");
/* Bulk vector-array writes and reads the .oct submesh serializers share. The
   writers stage at most 0x100 records through a stack buffer per FileWrite. */
bool WriteVector4Array004372E0(int file, const srVector4T<float>* values, int count);
bool WriteVector3Array00437390(int file, const srVector3T<float>* values, int count);
bool WriteVector2Array00437430(int file, const srVector2T<float>* values, int count);
bool ReadVector4Array004374C0(int file, srVector4T<float>* values, int count);
bool ReadVector3Array004374E0(int file, srVector3T<float>* values, int count);
bool ReadVector2Array00437510(int file, srVector2T<float>* values, int count);
/* Distance from `point` to the `from`-`to` segment, shared by the trace
   resolver and the GameData surface walk. When `clamp_point` is set the
   closest segment point is written back over `point`; `out_t` returns the
   clamped [0,1] projection fraction. */
float PointToSegmentDistance00437540(srVector3T<float>* point, const srVector3T<float>* from,
                                     const srVector3T<float>* to, char clamp_point, float* out_t);
/* XY-plane sibling over two-component vectors; the pathfinding code measures
   edge distances in plan view. */
float PointToSegmentDistance2D00437760(float* point, const float* from, const float* to,
                                       char clamp_point, float* out_t); /* 0x00437760 */
/* Grow `minimum`/`maximum` to include `point`, returning whether any bound
   moved. */
char GrowBoundsByPoint(const float* point, float* minimum, float* maximum); /* 0x004378F0 */
/* Whether `point` lies within `radius` of the six-float bounds box. */
char SphereNearBounds(const float* point, float radius, const float* bounds); /* 0x004386A0 */

/* The mesh's polygon index arrays are the same raw 12-byte records as the
   float vectors and retail routes both through 0x004374E0; the inline integer
   view keeps that one cast at the boundary. */
inline bool ReadVectorArray(int file, srVector3i* values, int count)
{
    return ReadVector3Array004374E0(
        file, reinterpret_cast<srVector3T<float>*>(values), /* reinterpret-ok: the
            float reader's raw 12-byte record is the index-triple record */
        count);
}
inline bool ReadVectorArray(int file, srVector3T<float>* values, int count)
{
    return ReadVector3Array004374E0(file, values, count);
}
inline bool ReadVectorArray(int file, srVector4T<float>* values, int count)
{
    return ReadVector4Array004374C0(file, values, count);
}
inline bool ReadVectorArray(int file, srVector2T<float>* values, int count)
{
    return ReadVector2Array00437510(file, values, count);
}

/* The polygon index arrays serialize through the same 12-byte vector writer. */
inline bool WriteVectorArray(int file, const srVector3i* values, int count)
{
    return WriteVector3Array00437390(
        file, reinterpret_cast<const srVector3T<float>*>(values), /* reinterpret-ok: the
            float writer's raw 12-byte record is the index-triple record */
        count);
}
inline bool WriteVectorArray(int file, const srVector3T<float>* values, int count)
{
    return WriteVector3Array00437390(file, values, count);
}
inline bool WriteVectorArray(int file, const srVector4T<float>* values, int count)
{
    return WriteVector4Array004372E0(file, values, count);
}
inline bool WriteVectorArray(int file, const srVector2T<float>* values, int count)
{
    return WriteVector2Array00437430(file, values, count);
}

/* One 0x10-byte entry of the .oct file's submesh table. Field +4 is the index
   into W8World::psrMeshes; the visibility update and UpdateMonsterLocation both
   resolve a region through it, and the update's reset pass writes the owning
   mesh back. Field +0 carries the render flags the visibility update sets and
   clears. */
struct W8OctSubmesh {
    unsigned long flags_00;
    int mesh_04;
    unsigned long positional_08;
    unsigned int positional_0c;
};

static_assert(sizeof(W8OctSubmesh) == 0x10, "W8OctSubmesh_must_be_0x10");

/* The object classes the octree tracks. Kind 3 is not dynamic: it names the
   static GD-surface polygon streams stored inside each leaf record. The
   dynamic kinds pair an object id with a cell in W8OctreeObjectRegistry —
   collidable props (8), path waypoints (9), location objects such as
   monsters (12) and the navigator position record each location carries
   alongside its kind-12 entry (13). CollectObjectsInCell also treats any
   query kind above 12 as "both 12 and 13". */
enum W8OctreeObjectKind {
    W8_OCTREE_KIND_SURFACE = 3,
    W8_OCTREE_KIND_PROP = 8,
    W8_OCTREE_KIND_WAYPOINT = 9,
    W8_OCTREE_KIND_LOCATION = 0xc,
    W8_OCTREE_KIND_NAVIGATOR = 0xd,
};

/* Registry keys. An object key carries the kind in its high half and the
   object id in its low half; a cell key packs the three coordinates a byte
   apart with a +1 in the low byte so that cell (0,0,0) never collides with
   an empty slot. */
inline unsigned int PackOctreeObjectKey(unsigned int kind, unsigned int id)
{
    return kind * 0x10000 + (id & 0xffff);
}
inline unsigned int PackOctreeCellKey(int x, int y, int z)
{
    return (x * 0x100 + y) * 0x100 + 1 + z;
}
inline unsigned int OctreeKeyKind(unsigned int key)
{
    return key >> 0x10;
}
inline unsigned int OctreeKeyId(unsigned int key)
{
    return key & 0xffff;
}
inline int OctreeCellKeyX(int key)
{
    return (key - 1) >> 0x10;
}
inline int OctreeCellKeyY(int key)
{
    return ((key - 1) >> 8) & 0xff;
}
inline int OctreeCellKeyZ(int key)
{
    return (key - 1) & 0xff;
}

class W8OctreeObjectRegistry {
public:
    /* Retail emits both lifecycle bodies out of line in the Octree TU: the
       constructor at 0x00436840 and the destructor at 0x00436B20. */
    W8OctreeObjectRegistry();
    ~W8OctreeObjectRegistry();

    W8OctreeIndex* by_cell;
    W8OctreeIndex* by_object;

    /* Returns whether the pairing ended up recorded; every recovered caller
       discards it. */
    unsigned char RegisterObjectCell(int kind, int id, const int* point);
    unsigned char MoveObjectToCell(int kind, int id, const int* point);
    unsigned char UnregisterObject(int kind, int id);
};

/* The cell walk 0x004362D0 builds and both line-of-sight bodies step: an
   ordinary 3D Bresenham over octree cells. One axis drives; the other two each
   carry a delta, an accumulator and the reset the accumulator takes when it
   goes negative, which is what makes the two triples symmetric. */
struct W8OctreeWalk {
    int cell_00[3];     /* 0x00: the cell the walk starts in */
    int step_0c[3];     /* 0x0c: +1 or -1 per axis */
    int major_axis_18;  /* 0x18 */
    int minor_axis_1c;  /* 0x1c: (major + 1) % 3 */
    int minor_axis_20;  /* 0x20: (major + 2) % 3 */
    int count_24;       /* 0x24: cells to visit */
    int error_delta_28; /* 0x28 */
    int error_2c;       /* 0x2c */
    int error_reset_30; /* 0x30 */
    int error_delta_34; /* 0x34 */
    int error_38;       /* 0x38 */
    int error_reset_3c; /* 0x3c */
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

static_assert(sizeof(W8OctPreTreeBranch) == 0x24, "W8OctPreTreeBranch_must_be_0x24");
static_assert(sizeof(W8OctPreTreeLeaf) == 0x28, "W8OctPreTreeLeaf_must_be_0x28");

/* Engine Code\Octree.cpp. LoadWorld allocates exactly 0x29c bytes. This object
   is deliberately non-polymorphic: neither the constructor at 0x0042BC10 nor
   the destructor at 0x0042DE60 stores a vptr, and every owner calls the
   complete teardown and then operator delete separately. The vtables at
   0x005EBFE0/0x005EBFE4/0x005EBFE8/0x005EBFEC near the TU boundary are the
   emitted scalar-deleting-destructor slots for W8GrowableVector<int>,
   W8GrowableVector<W8SpellVisual_#>, W8GrowableVector<W8Missile_#> (the
   construction-phase table for 0x005EC27C) and
   W8GrowableVector<W8SpellDamageReport_#>, i.e. compiler template material,
   not object polymorphism; the same holds for the emitted vector machinery
   next to W8OctPreTree and W8OctBuildTree. */
class W8Octree {
public:
    W8Octree(const char* path, W8GameData** game_data);
    void Reset();
    void Initialize(const void* header);
    ~W8Octree();
    void AddLoadedProp(W8Prop* prop);
    void AddLoadedParticle(stParticle* particle);
    /* Store the prop-sunlight bit array once it has been given a size. */
    void SetPropSunBits(BitArray* bits);
    /* Whether prop `offset` past prop_sun_base_184 has its sunlight bit; a
       negative offset checkpoints the shared index into the base. */
    int TestPropSunBit(int offset);
    void AddCollidablePropBounds(int index, const srVector3T<float>* bounds);
    void VisitPointCopy0042E620(unsigned short location_id, srVector3T<float>* position);
    /* Writes the cell coordinates and returns `point`, or null when the
       position is outside the octree bounds. */
    int* WorldPositionToCell(const srVector3T<float>* position, int* point); /* 0x00431440 */
    unsigned long FindLeaf00433660(const int* point);
    void UpdateMonsterLocation(unsigned short location_id, const srVector3T<float>* position);
    /* Object-kind values the query machinery dispatches on: 3 = GD triangle,
       8 = collidable-prop polygon reference, 9 = path waypoint, 12 = location entry,
       13 = secondary location entry. Registry values pack kind into the high
       half and id+1 into the low half; cell keys pack x/y/z bytes with a +1
       sentinel. */
    void UnregisterLocationObjects(unsigned int location_id);          /* 0x0042E650 */
    void UnregisterLocationObject(unsigned int location_id, int kind); /* 0x0042E880 */
    /* Collect object ids of `kind` from every cell under the `from`-`to`
       segment grown by `extent` (the extent also takes the segment length as
       a floor). `*results` carries the destination buffer in and out; a null
       incoming buffer selects the internal m_aulGDObjs store. Returns the
       entry count. */
    int CollectObjectsAlongSegment(int** results, const srVector3T<float>* from,
                                   const srVector3T<float>* to, float extent,
                                   unsigned short kind); /* 0x0042ED60 */
    /* Kind-12 box query; `exclusion` 0 maps to none. */
    unsigned int QueryLocationsInBox(int** results, const srVector3T<float>* lower,
                                     const srVector3T<float>* upper,
                                     unsigned short exclusion); /* 0x0042EF00 */
    /* AABB occupancy test: GD triangles, kind-12 location objects (with each
       monster's navigator radius) and collidable-prop surfaces. */
    unsigned char TestBoxOccupied(const srVector3T<float>* lower,
                                  const srVector3T<float>* upper); /* 0x0042EF30 */
    /* Append the objects of `kind` inside one cell to the shared query
       buffer; the registry path deduplicates through m_owned_194. */
    unsigned int CollectObjectsInCell(const int* cell, unsigned short kind); /* 0x0042F400 */
    /* Bounds-checked cell -> leaf index: the direct leaf grid when present,
       else a masked descent through the branch tree. */
    unsigned int LeafIndexForCell(const int* cell); /* 0x00433730 */
    /* Descend the branch tree while `masked_cell`'s leading mask word keeps
       the current level bit set, choosing the octant from the three
       coordinate words. */
    int DescendByMask(const unsigned int* masked_cell); /* 0x004336D0 */
    unsigned int GetSectorForPosition(const srVector3T<float>* position);
    bool HasLineOfSight(const srVector3T<float>* from, srVector3T<float>* to, char allow_fallback);
    /* Paths `from` toward `to`; on success `range` returns the path cost and
       `hops` the reached-waypoint count. */
    unsigned char TestNoiseLineOfSight00434220(const srVector3T<float>* from, srVector3T<float>* to,
                                               float* range, int* hops); /* 0x00434220 */
    short TraceLineOfSight(const srVector3T<float>* from, srVector3T<float>* to, char trace_world,
                           int from_location_id, int to_location_id, char visit_octree,
                           int trace_mode);
    void AdjustPortalDestination(srVector3T<float>* destination, const srVector3T<float>* source);
    void BuildCellWalk(const srVector3T<float>* from, const srVector3T<float>* to,
                       W8OctreeWalk* walk);
    /* The by-value overload retail emits at 0x00436280. */
    void BuildCellWalk(srVector3T<float> from, srVector3T<float> to,
                       W8OctreeWalk* walk); /* 0x00436280 */
    /* Reset the shared buffer and collect one cell's leaf object ids. */
    int ProbeCellForTrace(const int* cell); /* 0x00435B00 */
    /* Reset vs append variants collecting one cell's leaf polygon references
       (mapped through m_owned_0d4 into (mesh<<16)|polygon keys). */
    int ProbeCellForBlockers(const int* cell);       /* 0x00435C40 */
    int ProbeCellForBlockersAppend(const int* cell); /* 0x00435DA0 */
    /* Test every buffered (mesh<<16)|polygon key's triangle against the trace
       ray; on a closer hit, end_0c returns the contact point. */
    unsigned char TestProbeResult(W8OctreeTrace* trace); /* 0x00435F00 */
    int TraceAgainstProps(const srVector3T<float>* from, srVector3T<float>* to, int value_3,
                          int value_4); /* 0x00436510 */
    /* Nearest ray-vs-sphere hit across the kind-12 objects in the segment
       box, then against the camera sphere; writes the hit position into `to`
       and the hit location id into `hit_location` (or -1/0). `excluded`
       skips one location id, `location` carries the in/out location id used
       for the pathing-probe set, `flags` masks navigator unknown_090, and
       `noise_adjust` applies the g_float_005ebc3c/noise penalty. */
    char ResolveTraceHit(const srVector3T<float>* from, srVector3T<float>* to, int excluded,
                         int* hit_location, int location, unsigned int flags,
                         char noise_adjust); /* 0x004353F0 */
    /* Navigator placement query: retail callers pass modes such as 5, 10, 20
       and 30; retail callers load the octree into ecx and the body returns
       with ret 0x28, so this is an ordinary member. `source`'s y may be
       snapped to the settled ground height. */
    /* Sibling scatter query to FindNavigatorPosition: walks fixed lateral
       columns over ten rings instead of the mode-driven cell grid, and can
       flatten every accepted position back to the source height. */
    unsigned int FindScatterPositions00437980(float* position, float yaw, float spacing,
                                              unsigned int count, float* positions,
                                              char proximity_check, char flatten_y);
    unsigned int FindNavigatorPosition(srVector3T<float>* source, float yaw, float radius,
                                       unsigned int count, srVector3T<float>* positions,
                                       char first_only, char flag_2, char flag_3, int mode,
                                       char flag_4); /* 0x00437F30 */
    unsigned int AdvanceNavigator(W8NavigatorMovementState* movement, float radius,
                                  float separation);
    unsigned char PrepareNavigatorTarget00434250(W8NavigatorMovementState* movement, float radius,
                                                 float separation);
    unsigned char PrepareNavigatorPatrol00434880(W8NavigatorMovementState* movement, float minimum,
                                                 float maximum);
    unsigned char LinkNavigatorTarget00434A00(W8NavigatorMovementState* movement,
                                              const srVector3T<float>* target, float separation);
    void GetPathSurfaceNormal00433A70(const srVector3T<float>* position, srVector3T<float>* normal);
    float SettleToGround(srVector3T<float>* position, unsigned char* out_hit, char mode,
                         float limit); /* 0x00433820 */
    /* Clamp `position` to the clipped ceiling, probe the ground one
       world-scale unit lower and keep the settled height on a hit. */
    void SnapToGround(srVector3T<float>* position, char mode); /* 0x00431D20 */
    void QueueOctreeKind130042E810(int id, const srVector3T<float>* position);
    /* Box query over the shared query buffer: `*objects` carries the
       destination buffer in and out (null selects m_aulGDObjs), `excluded`
       is an object id pre-marked in the dedupe set (-1 = none). Returns the
       entry count. */
    int QueryObjects(int** objects, const srVector3T<float>* lower, const srVector3T<float>* upper,
                     unsigned short kind, int excluded); /* 0x0042F280 */
    void AdjustPosition00431DA0(srVector3T<float>* position, unsigned int mode);
    /* Refresh the pathing service's debug preview from the world cursor,
       falling back to the camera eye when the cursor is unset. */
    void UpdatePathVisualization(); /* 0x00434170 */
    void UpdateCameraVisibility0042F7E0();
    void UpdateVisibility004304A0();
    unsigned char UpdateWorldTrace00433EB0();
    /* Store `path` with its extension stripped into m_owned_0c0; the sibling
       data files are then derived from the stem. */
    bool SetPathStem(const char* path); /* 0x0042CF90 */
    bool SavePoints00432D60(char* path);
    bool LoadPointFiles(const char* level_name);
    bool ReadRegionLinkFile(const char* level_name);
    /* Region containing `point` (1-based index into the volume array), else
       the packed auto-region key (level<<24)|cell. */
    unsigned int RegionKeyForPoint(const srVector3T<float>* point); /* 0x00432720 */
    /* Link `region_key` to every mesh marked in m_projected_regions_15c,
       inserting unseen (region, mesh) pairs into m_pRegionLinks_150. */
    void RecordRegionMeshLinks(unsigned int region_key); /* 0x004327F0 */
    /* Sweep the camera around `point`, mark cells whose meshes still draw
       into m_projected_regions_15c and return `point`'s region key (zero when
       nothing linked). */
    unsigned int SampleRegionLinks(const srVector3T<float>* point, char descend, char clear_sets,
                                   unsigned int region_key); /* 0x00431E10 */
    /* Rebuild the region-link table by camera-sampling the region grid;
       `rebuild_all` sweeps every cell and discards the saved point list, a
       zero value samples a sparse checkerboard plus the stored points. */
    void BuildRegionLinks(char rebuild_all); /* 0x004314C0 */
    bool SaveRegionLinks004331F0(char* path);
    unsigned char ValidateRegionMeshLinks00433AB0();
    /* Collect the live (mesh<<16)|polygon keys whose triangles overlap the
       (x±radius, y-height..y, z±radius) box, sorted and zero-terminated in
       the shared query buffer. */
    unsigned long* CollectPolygonsNearPoint(srVector3T<float>* center, float radius,
                                            float height); /* 0x00438780 */
    int CountBadRegionMeshLinks00433B90(W8OctSpatialState0046CCC0* spatial);
    void ToggleUpdateSuspension00434020(W8World* world);
    void MarkMeshLinksVisible00430A70(unsigned int mesh);
    /* Collect the model instances whose bounds reach within `radius` of
       `point`, through the region cells and volumes the sphere touches. */
    int CollectModelsNearPoint(W8GrowableVector<stModelInstance*>* out,
                               const srVector3T<float>* point, float radius, unsigned int flags,
                               char only_accumulated); /* 0x0042F9A0 */
    unsigned char CollectVisibleRegions00430D50(srVector3T<float>* location, int* cells,
                                                float* depth, unsigned char mode);
    void CollectVisibleCells0042FE90();
    /* Project every candidate region volume against the frustum planes and
       mark the visible ones in the current region set. */
    void MarkVisibleRegions004301C0(); /* 0x004301C0 */
    /* Build the six frustum planes from the camera basis and far clip. */
    void BuildFrustumPlanes004302E0(); /* 0x004302E0 */
    short ProjectLinkedRegionsForLocation00431050(srVector3T<float>* location,
                                                  unsigned short* region_list); /* 0x00431050 */

    bool HasLoadError() const
    {
        return (spatial_000.flags_00 & 0x80000000) != 0;
    }
    unsigned long GetMeshCount() const
    {
        return spatial_000.positional_74;
    }

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
    unsigned long* m_owned_0b0;
    unsigned long m_positional_0b4;
    unsigned long m_positional_0b8;
    W8OctreeObjectRegistry* object_registry;
    char* m_owned_0c0;
    unsigned char m_fAccumulating;
    unsigned char m_positional_0c5[3];
    unsigned long m_positional_0c8;
    unsigned long m_positional_0cc;
    unsigned long* m_owned_0d0;
    /* ReadOctFile's allocation assertion calls this the "Poly Lookup table":
       polygon index to (kind<<16)|id object key. */
    unsigned long* m_owned_0d4;
    W8OctSubmesh* m_pSubmeshes;
    /* Six original member names, from ReadOctFile's own assertion text at
       0x0042C68A, 0x0042C70C, 0x0042C7AB, 0x0042C850, 0x0042C8F5 and
       0x0042CAA4. The us prefix is the image's own, so the four lookup and
       link tables are unsigned short arrays. */
    BitArray* m_pAlphaBits;                  /* 0xdc */
    unsigned short* m_pusMeshParticleLookup; /* 0xe0 */
    unsigned short* m_pusMeshParticles;      /* 0xe4 */
    unsigned short m_usMeshParticlesLen_0e8;
    unsigned short m_padding_0ea;
    unsigned short* m_pusMeshPropLookup; /* 0xec */
    unsigned short* m_pusMeshProps;      /* 0xf0 */
    unsigned short m_usMeshPropsLen_0f4;
    unsigned short m_padding_0f6;
    unsigned long m_ulNumParticles;
    BitArray* m_linked_particles_0fc;
    BitArray* m_visible_particles_100;
    BitArray* m_linked_props_104;
    BitArray* m_visible_props_108;
    BitArray* m_particles_to_disable_10c;
    BitArray* m_props_to_disable_110;
    W8Prop** m_papProps;
    stParticle** m_papParticles;
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
    W8HashTable<unsigned int, unsigned short>* m_pRegionLinks_150;
    BitArray* m_owned_154;
    unsigned long m_positional_158;
    BitArray* m_projected_regions_15c;
    BitArray* m_current_regions_160;
    BitArray* m_previous_regions_164;
    unsigned char m_reset_visibility_168;
    unsigned char m_positional_169;
    unsigned char m_projected_regions_valid_16a;
    unsigned char m_positional_16b;
    unsigned char m_positional_16c;
    unsigned char m_positional_16d;
    unsigned char m_padding_16e[2];
    unsigned long m_positional_170;
    srVector3T<float>* m_sr_owned_174;
    /* Region-link sample cell size read from .oct offset 0xac; the link
       builder strides the x/z grid by it (times three for a sparse pass). */
    float m_region_cell_178;
    unsigned long m_positional_17c;
    W8PathingService* pathing_180;
    int prop_sun_base_184; /* 0x184: this octree's base index into the shared
                              prop-sunlight bit stream */
    unsigned long m_ulNumProps;
    /* Named m_pPropSunBits by ReadOctFile's assertion at 0x0042CAA4. The
       earlier `visited` reading came from 0x0042E3E0's parameter, not from
       the image, and the assertion outranks it. */
    BitArray* m_pPropSunBits; /* 0x18c */
    BitArray* m_owned_190;
    BitArray* m_owned_194;
    BitArray* m_accumulated_regions_198;
    BitArray* m_owned_19c;
    BitArray* m_owned_1a0;
    BitArray* m_owned_1a4;
    unsigned long m_positional_1a8;
    unsigned long m_positional_1ac;
    unsigned long m_positional_1b0;
    unsigned long m_meshCount_1b4;
    unsigned long m_positional_1b8;
    unsigned long* m_aulGDObjs; /* 0x1bc */
    srVector3T<float> camera_location_1c0;
    srVector3T<float> camera_dof_1cc;
    srVector3T<float> rotation_column_1d8;
    srVector3T<float> rotation_column_1e4;
    float horizontal_fov_1f0;
    float vertical_fov_1f4;
    float horizontal_fov_cosine_1f8;
    float vertical_fov_cosine_1fc;
    float far_clip_200;
    int m_positional_204[6];
    /* The six frustum planes 0x004302E0 builds; 0x0046D880 tests a point
       against all six. */
    srVector4T<float> m_frustum_planes_21c[6]; /* 0x21c */
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
   the destructive OctBuildPreTree conversion is named here.
   Also non-polymorphic: the constructor's only vtable stores (0x005EC3F4
   then 0x005EC3F0, the vector construction-phase and final tables) land in
   the separately allocated positional_3b8 vector, never in this object
   itself. */
class W8OctPreTree004679E0 : public W8Octree {
public:
    W8OctPreTree004679E0();

    W8HashTable<unsigned short, unsigned long>* positional_29c;
    unsigned long positional_2a0;
    unsigned char positional_2a4[0xfc];
    unsigned long polygon_cursor_3a0;
    W8OctRegionGameData* game_data_3a4;
    unsigned long positional_3a8;
    unsigned long positional_3ac;
    unsigned long positional_3b0;
    unsigned long positional_3b4;
    W8GrowableVector<void*>* positional_3b8; /* vector-void-ok: retail emits
        this vector's construction-phase and final tables; its element type
        is unresolved */

    /* Walks the `from`-`to` segment through the leaf grid, collecting each
       visited leaf's region-polygon ids and plane/slab-testing them. Answers
       whether the segment is unobstructed; the light-visibility callers
       accumulate its result. */
    bool SegmentClear00467BB0(const srVector3T<float>* from, const srVector3T<float>* to);
    /* Resets the collected-id run and appends every not-yet-seen polygon id
       the leaf under `cell` lists. */
    void CollectLeafPolygons(const int* cell);
    /* Tests the collected region polygons' planes against the trace segment;
       a polygon blocks only when the ray pierces at least 5.0f past its plane
       (or starts within 1.0f in front) and the contact lands inside it. */
    bool TestCollectedPolygons004681E0(W8OctreeTrace* trace);
};

static_assert(sizeof(W8OctPreTree004679E0) == 0x3bc, "W8OctPreTree004679E0_must_be_0x3bc");

extern W8Octree* g_octree_6598a4;
extern W8OctPreTree004679E0* g_oct_pre_tree_659c74;

/* The SGP /NOOCT startup switch sets this flag; an Octree-unit body reads it. */
extern "C" void NoOct(void); // C-LINKAGE: src/sgp/sgp.c invokes the /NOOCT switch
extern unsigned char g_flag_6598a8;

unsigned char __stdcall IsNavigatorAtTarget004347D0(W8NavigatorMovementState* movement);

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");

extern unsigned int* g_octree_storage_00659770;
extern int* g_octree_state_00659890;
extern srNode* g_octree_trace_node_00659894;
extern float g_octree_cell_scale_005ebcd0;
extern unsigned long g_octree_bytes_read_00659888;
extern int g_prop_sun_index_006598ac;
extern unsigned char g_octree_update_suspended_00659898;
extern unsigned char g_octree_trace_enabled_00659899;
/* Renderer switches the region-link build toggles; their other consumers are
   unrecovered render routines, so the names stay address-qualified. */
extern unsigned char g_flag_0065a0ec;
extern unsigned char g_flag_0065a0ed;
extern unsigned char g_flag_0065a146;

int CheckLevelAssetSet0042CCC0(const char* level_path);

unsigned long* __fastcall PackColour00433FB0(unsigned long* color, double red, double green,
                                             double blue, double alpha);
