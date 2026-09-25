#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"

class GDProp;
class W8Prop;
class stParticle;
class stModelInstance;
class srNode;
typedef W8HashTable<unsigned int, int> W8OctreeIndex;
typedef W8HashEntry<unsigned int, int> W8OctreeEntry;

class W8PathingService;
class PrePathing;
class OctMeshModel;
struct CondPathNode;
struct W8LevelFile;
struct W8PreProp;
struct W8World;
struct W8GameData;
struct W8NavigatorMovementState;
struct W8OctBuildNode;
struct W8OctPreTreeGeometry;
struct W8BoundingBox;

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
   writers stage at most 0x100 records through a stack buffer per FileWrite.

   Retail emits exactly one reader/writer pair per record width and every call
   site lives in OctMeshModel::Read/Write. The width-12 entries serve both
   srVector3T<float> arrays (vertex locations, normals, lights) and srVector3i
   index triples (poly-vertex, poly-UV); this incremental-link build performs
   no identical-function folding, so the shared entry is one source function,
   not a folded overload pair or a per-type template instantiation. The
   writer's elementwise staging copy proves the declared element was a
   complete 12-byte record rather than raw storage, and the family is keyed
   on the float vector width (srVector2/3/4 are the float typedefs), so the
   float spelling is canonical and the integer triples are the reused case. */
BOOLEAN WriteVector4Array(int file, const srVector4T<float>* values, int count);
BOOLEAN WriteVector3Array(int file, const srVector3T<float>* values, int count);
BOOLEAN WriteVector2Array(int file, const srVector2T<float>* values, int count);
bool ReadVector4Array(int file, srVector4T<float>* values, int count);
bool ReadVector3Array004374E0(int file, srVector3T<float>* values, int count);
bool ReadVector2Array(int file, srVector2T<float>* values, int count);
/* Distance from `point` to the `from`-`to` segment, shared by the trace
   resolver and the GameData surface walk. When `clamp_point` is set the
   closest segment point is written back over `point`; `out_t` returns the
   clamped [0,1] projection fraction. */
float PointToSegmentDistance(srVector3T<float>* point, const srVector3T<float>* from,
                             const srVector3T<float>* to, char clamp_point, float* out_t);
/* XY-plane sibling over two-component vectors; the pathfinding code measures
   edge distances in plan view. */
float PointToSegmentDistance2D(srVector2T<float>* point, const srVector2T<float>* from,
                               const srVector2T<float>* to, char clamp_point,
                               float* out_t); /* 0x00437760 */
/* Grow `minimum`/`maximum` to include `point`, returning whether any bound
   moved. */
char GrowBoundsByPoint(const srVector3T<float>* point, srVector3T<float>* minimum,
                       srVector3T<float>* maximum); /* 0x004378F0 */
/* Whether `point` lies within `radius` of the bounds box. */
char SphereNearBounds(const srVector3T<float>* point, float radius,
                      const W8BoundingBox* bounds); /* 0x004386A0 */

/* The polygon index triples reuse the canonical float-vector entry point
   above (see the family comment); the inline integer view keeps that one
   cast at the call-site boundary. */
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
    return ReadVector4Array(file, values, count);
}
inline bool ReadVectorArray(int file, srVector2T<float>* values, int count)
{
    return ReadVector2Array(file, values, count);
}

/* The polygon index triples serialize through the same canonical 12-byte
   float-vector writer. */
inline BOOLEAN WriteVectorArray(int file, const srVector3i* values, int count)
{
    return WriteVector3Array(
        file, reinterpret_cast<const srVector3T<float>*>(values), /* reinterpret-ok: the
            float writer's raw 12-byte record is the index-triple record */
        count);
}
inline BOOLEAN WriteVectorArray(int file, const srVector3T<float>* values, int count)
{
    return WriteVector3Array(file, values, count);
}
inline BOOLEAN WriteVectorArray(int file, const srVector4T<float>* values, int count)
{
    return WriteVector4Array(file, values, count);
}
inline BOOLEAN WriteVectorArray(int file, const srVector2T<float>* values, int count)
{
    return WriteVector2Array(file, values, count);
}

/* One 0x10-byte entry of the .oct file's submesh table. Field +4 is the index
   into W8World::psrMeshes; the visibility update and UpdateMonsterLocation both
   resolve a region through it, and the update's reset pass writes the owning
   mesh back. Field +0 carries the render flags the visibility update sets and
   clears. */
struct W8OctSubmesh {
    unsigned long flags_00;
    int mesh_04;
    /* The same-chain successor's record index (the build record's
       next_link_10). */
    unsigned long next_link_08;
    /* Reader max-scans this to size the g_octree_storage identity table. */
    unsigned int polygon_count_0c;
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
    unsigned short provisional_region_00;
    /* The region/owner id VerifyPolygonRegions and the runtime region reads
       compare against a polygon's region_32. */
    unsigned short region_02;
    unsigned long children_04[8];
};

struct W8OctPreTreeLeaf {
    /* Bit 0 is a runtime flag WriteOctFile clears before serialization. */
    unsigned long flags_00;
    unsigned long region_offset_04;
    unsigned long polygon_offset_08;
    unsigned long gd_polygon_offset_0c;
    /* Stream offsets for object kinds 4-9: QueryKinds indexes the leaf as a
       flat ten-dword table (kind + leaf_index * 10). */
    unsigned long kind_offsets_10[6];
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
    void Initialize(const W8OctFileHeader* header);
    ~W8Octree();
    void AddLoadedProp(W8Prop* prop);
    void AddLoadedParticle(stParticle* particle);
    /* Store the prop-sunlight bit array once it has been given a size. */
    void SetPropSunBits(BitArray* bits);
    /* Whether prop `offset` past prop_sun_base_184 has its sunlight bit; a
       negative offset checkpoints the shared index into the base. */
    int TestPropSunBit(int offset);
    void AddCollidablePropBounds(int index, const W8BoundingBox* bounds);
    void VisitPointCopy(unsigned short location_id, srVector3T<float>* position);
    /* Writes the cell coordinates and returns `point`, or null when the
       position is outside the octree bounds. */
    int* WorldPositionToCell(const srVector3T<float>* position, int* point); /* 0x00431440 */
    unsigned long FindLeaf(const int* point);
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
    int CollectObjectsAlongSegment(unsigned long** results, const srVector3T<float>* from,
                                   const srVector3T<float>* to, float extent,
                                   unsigned short kind); /* 0x0042ED60 */
    /* Kind-12 box query; `exclusion` 0 maps to none. */
    unsigned int QueryLocationsInBox(unsigned long** results, const srVector3T<float>* lower,
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
    unsigned char TestNoiseLineOfSight(const srVector3T<float>* from, srVector3T<float>* to,
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
       (mapped through m_aulPolyLookup into (mesh<<16)|polygon keys). */
    int ProbeCellForBlockers(const int* cell);       /* 0x00435C40 */
    int ProbeCellForBlockersAppend(const int* cell); /* 0x00435DA0 */
    /* Test every buffered (mesh<<16)|polygon key's triangle against the trace
       ray; on a closer hit, end_0c returns the contact point. */
    unsigned char TestProbeResult(W8OctreeTrace* trace); /* 0x00435F00 */
    int TraceAgainstProps(const srVector3T<float>* from, srVector3T<float>* to, int skip_flag,
                          int gate); /* 0x00436510 */
    /* Nearest ray-vs-sphere hit across the kind-12 objects in the segment
       box, then against the camera sphere; writes the hit position into `to`
       and the hit location id into `hit_location` (or -1/0). `excluded`
       skips one location id, `location` carries the in/out location id used
       for the pathing-probe set, `flags` masks navigator trace_mask_090, and
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
    unsigned int FindScatterPositions(const srVector3T<float>* position, float yaw, float spacing,
                                      unsigned int count, srVector3T<float>* positions,
                                      char proximity_check, char flatten_y);
    unsigned int FindNavigatorPosition(srVector3T<float>* source, float yaw, float radius,
                                       unsigned int count, srVector3T<float>* positions,
                                       char first_only, char settle_any_height, char avoid_triggers,
                                       int mode, char require_waypoint_span); /* 0x00437F30 */
    unsigned int AdvanceNavigator(W8NavigatorMovementState* movement, float radius,
                                  float separation);
    unsigned char PrepareNavigatorTarget00434250(W8NavigatorMovementState* movement, float radius,
                                                 float separation);
    unsigned char PrepareNavigatorPatrol(W8NavigatorMovementState* movement, float minimum,
                                         float maximum);
    unsigned char LinkNavigatorTarget(W8NavigatorMovementState* movement,
                                      const srVector3T<float>* target, float separation);
    void GetPathSurfaceNormal00433A70(const srVector3T<float>* position, srVector3T<float>* normal);
    float SettleToGround(srVector3T<float>* position, unsigned char* out_hit, char mode,
                         float limit); /* 0x00433820 */
    /* Clamp `position` to the clipped ceiling, probe the ground one
       world-scale unit lower and keep the settled height on a hit. */
    /* Returns the ground-hit flag in AL; pathing callers test it. */
    bool SnapToGround(srVector3T<float>* position, char mode); /* 0x00431D20 */
    void QueueOctreeKind13(int id, const srVector3T<float>* position);
    /* Box query over the shared query buffer: `*objects` carries the
       destination buffer in and out (null selects m_aulGDObjs), `excluded`
       is an object id pre-marked in the dedupe set (-1 = none). Returns the
       entry count. */
    int QueryObjects(unsigned long** objects, const srVector3T<float>* lower,
                     const srVector3T<float>* upper, unsigned short kind,
                     int excluded); /* 0x0042F280 */
    void AdjustPosition00431DA0(srVector3T<float>* position, unsigned int mode);
    /* Refresh the pathing service's debug preview from the world cursor,
       falling back to the camera eye when the cursor is unset. */
    void UpdatePathVisualization(); /* 0x00434170 */
    void UpdateCameraVisibility();
    void UpdateVisibility();
    unsigned char UpdateWorldTrace();
    /* Store `path` with its extension stripped into m_owned_0c0; the sibling
       data files are then derived from the stem. */
    bool SetPathStem(const char* path); /* 0x0042CF90 */
    BOOLEAN SavePoints(char* path);
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
    BOOLEAN SaveRegionLinks(char* path);
    unsigned char ValidateRegionMeshLinks();
    /* Collect the live (mesh<<16)|polygon keys whose triangles overlap the
       (x±radius, y-height..y, z±radius) box, sorted and zero-terminated in
       the shared query buffer. */
    unsigned long* CollectPolygonsNearPoint(srVector3T<float>* center, float radius,
                                            float height); /* 0x00438780 */
    int CountBadRegionMeshLinks(W8OctSpatialState* spatial);
    void ToggleUpdateSuspension(W8World* world);
    void MarkMeshLinksVisible(unsigned int mesh);
    /* Collect the model instances whose bounds reach within `radius` of
       `point`, through the region cells and volumes the sphere touches. */
    int CollectModelsNearPoint(W8GrowableVector<stModelInstance*>* out,
                               const srVector3T<float>* point, float radius, unsigned int flags,
                               char only_accumulated); /* 0x0042F9A0 */
    unsigned char CollectVisibleRegions(srVector3T<float>* location, int* cells, float* depth,
                                        unsigned char mode);
    void CollectVisibleCells();
    /* Project every candidate region volume against the frustum planes and
       mark the visible ones in the current region set. */
    void MarkVisibleRegions004301C0(); /* 0x004301C0 */
    /* Build the six frustum planes from the camera basis and far clip. */
    void BuildFrustumPlanes004302E0(); /* 0x004302E0 */
    short ProjectLinkedRegionsForLocation(srVector3T<float>* location,
                                          unsigned short* region_list); /* 0x00431050 */

    bool HasLoadError() const
    {
        return (spatial_000.flags_00 & 0x80000000) != 0;
    }
    unsigned long GetMeshCount() const
    {
        return spatial_000.submesh_count_74;
    }

public:
    /* Same proven 0x9c value used by the level build tree.  Construction and
       teardown operate on the offset-zero subobject, but current evidence does
       not distinguish first-member composition from inheritance, so the
       declaration makes the narrower composition claim. */
    W8OctSpatialState spatial_000;
    W8OctPreTreeBranch* m_owned_09c;
    W8OctPreTreeLeaf* m_owned_0a0;
    unsigned long m_leaf_grid_dim_x_0a4;
    unsigned long m_leaf_grid_dim_y_0a8;
    unsigned long m_leaf_grid_dim_z_0ac;
    unsigned long* m_owned_0b0;
    unsigned long m_branch_count_0b4;
    unsigned long m_leaf_count_0b8;
    W8OctreeObjectRegistry* object_registry;
    char* m_owned_0c0;
    bool m_fAccumulating;
    unsigned char m_padding_0c5[3];
    unsigned long m_vertex_count_0c8;
    unsigned long m_leaf_polygon_stream_len_0cc;
    unsigned long* m_owned_0d0;
    /* ReadOctFile's allocation assertion calls this the "Poly Lookup table":
       polygon index to (kind<<16)|id object key. */
    unsigned long* m_aulPolyLookup;
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
    /* Registered prop id the last prop trace or test_props snap hit; -1 when
       the ground resolve touched only static geometry. */
    int current_prop;
    unsigned long m_gd_surface_stream_len_124;
    unsigned long m_trigger_count_128;
    unsigned long* m_owned_12c;
    /* Trigger list: serialized as 2-byte elements (ReadOctFile allocates
       count * 2 + 4) even though WriteOctFile emits them four bytes wide. */
    unsigned short* m_owned_130;
    unsigned long m_trace_skip_flag_134;
    unsigned long m_region_list_len_138;
    unsigned long m_padding_13c;
    /* The leaf-level mask: VerifyPolygonRegions rebuilds it as
       (1 << leaf_level_52) - 1 and the packed-cell writers emit it as the top
       byte of each (mask<<24 | x<<16 | y<<8 | z) key. */
    unsigned long m_region_mask_140;
    unsigned long m_depth_mask_144;
    unsigned short* m_owned_148;
    unsigned char* m_pfRegsVisited;
    W8HashTable<unsigned int, unsigned short>* m_pRegionLinks_150;
    BitArray* m_owned_154;
    unsigned long m_padding_158;
    BitArray* m_projected_regions_15c;
    BitArray* m_current_regions_160;
    BitArray* m_previous_regions_164;
    unsigned char m_reset_visibility_168;
    bool m_region_links_ready_169;
    bool m_projected_regions_valid_16a;
    unsigned char m_positional_16b;
    bool m_region_links_dirty_16c;
    bool m_points_dirty_16d;
    unsigned char m_padding_16e[2];
    unsigned long m_point_count_170;
    srVector3T<float>* m_sample_points_174;
    /* Region-link sample cell size read from .oct offset 0xac; the link
       builder strides the x/z grid by it (times three for a sparse pass). */
    float m_region_cell_178;
    unsigned long m_path_clearance_17c;
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
    unsigned long m_root_mesh_count_1a8;
    unsigned long m_kind1_submesh_count_1ac;
    unsigned long m_alpha_polygon_count_1b0;
    unsigned long m_meshCount_1b4;
    unsigned long m_gd_result_count_1b8;
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
    int m_visible_cells_204[6];
    /* The six frustum planes 0x004302E0 builds; 0x0046D880 tests a point
       against all six. */
    W8Plane m_frustum_planes_21c[6]; /* 0x21c */
    unsigned long m_padding_27c[6];
    bool m_visibility_suspended_294;
    unsigned char m_padding_295;
    /* The build's directional-sun count: the driver stores the light total
       and CreateSubMeshes emits it as each OctMeshModel's version_00 and
       sizes the per-sun vertex light arrays from it. */
    unsigned short m_sun_count_296;
    unsigned char m_padding_298;
    unsigned char m_padding_299;
    unsigned char m_padding_29a[2];
};

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");

/* Engine Code\OctPreTree.cpp's build-time runtime tree.  The constructor at
   0x004679E0 invokes W8Octree's constructor at offset zero, and its sole
   caller allocates 0x3bc bytes before invoking it.  Only the suffix reached by
   the destructive OctBuildPreTree conversion is named here.
   Also non-polymorphic: the constructor's only vtable stores (0x005EC3F4
   then 0x005EC3F0, the vector construction-phase and final tables) land in
   the separately allocated props_3b8 vector, never in this object
   itself. */
class OctPreTree : public W8Octree {
public:
    OctPreTree();
    ~OctPreTree();

    /* Automesh index -> packed cell (z | y<<8 | x<<16 | mask<<24) map the
       verify passes walk to bound-check each automesh's vertices. */
    W8HashTable<unsigned short, unsigned long>* automesh_cells_29c;
    PrePathing* pre_pathing_2a0;
    /* The path-node scratch block BuildPathLists/PathNodeObstructed fill:
       created-node count, then the runs of registered prop ids the node
       rests on (supports) and that overlap its clearance box (blocks).
       m_lNumSupports/m_lNumBlocks are the original names from the
       PathNodeObstructed assertion text.  Both appends write the slot before
       the `> 29` assertion runs, so a 30th entry overruns the array exactly
       like retail. */
    int path_node_count_2a4;
    int m_lNumSupports_2a8;
    int m_lNumBlocks_2ac;
    int m_lSupports_2b0[30];
    int m_lBlocks_328[30];
    unsigned long polygon_cursor_3a0;
    W8OctPreTreeGeometry* game_data_3a4;
    unsigned long padding_3a8;
    unsigned long padding_3ac;
    unsigned long deepest_link_list_3b0;
    /* Path-node grid pitch: BuildPathLists sets it to m_region_cell_178 * 2. */
    float path_node_extent_3b4;
    /* The registered prop objects the path-bounds test collides against;
       0x0046BEC0 reads m_surface_count_14 and the collidable flag on each. */
    W8GrowableVector<GDProp*>* props_3b8;

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
    bool TestCollectedPolygons(W8OctreeTrace* trace);
    /* Serializes the finished octree to NewLevel.oct. */
    unsigned char WriteOctFile004683F0(W8OctPreTreeGeometry* geometry, W8GameData* game_data);
    /* Partitions the geometry into submesh records, emits the OctMeshModel
       array and fills m_pSubmeshes/m_aulPolyLookup. */
    OctMeshModel* CreateSubMeshes00468C30(W8OctPreTreeGeometry* geometry);
    unsigned long SplitMeshes00469670(W8OctPreTreeGeometry* geometry, W8OctSubmeshBuild* records);
    unsigned long AllocateSubMesh0046A790(W8OctSubmeshBuild* records);
    unsigned long SplitUVMaps0046A4B0(W8OctSubmeshBuild* record, W8OctPreTreeGeometry* geometry);
    void VerifyPolygonRegions0046ABF0();
    void VerifyAutoMeshes(W8OctPreTreeGeometry* geometry, W8OctSubmeshBuild* records);
    unsigned char BuildPathLists0046B060(W8GameData* game_data, W8LevelFile* level,
                                         unsigned int min_component_percent);
    char PathNodeObstructed0046B700(const srVector3T<float>* node_position);
    unsigned char InsertConditionalNodes0046B9D0(W8HashTable<unsigned int, CondPathNode*>* nodes,
                                                 unsigned int cell, unsigned int node,
                                                 W8PreProp* preprops, int preprop_count);
    /* Tests the bounds box against static surfaces and registered props;
       0 clear, 1 blocked, 3 clear but prop ids were recorded in m_lBlocks_328. */
    char TestPathPropBounds0046BEC0(const srVector3T<float>* minimum,
                                    const srVector3T<float>* maximum);
    int CreatePathProps0046C0F0(W8LevelFile* level, W8PreProp** preprops);
};

static_assert(sizeof(OctPreTree) == 0x3bc, "OctPreTree_must_be_0x3bc");

extern W8Octree* g_octree_6598a4;
extern OctPreTree* g_oct_pre_tree_659c74;

/* The SGP /NOOCT startup switch sets this flag; an Octree-unit body reads it. */
extern "C" void NoOct(void); // C-LINKAGE: src/sgp/sgp.c invokes the /NOOCT switch
extern bool g_octree_disabled_6598a8;

bool __stdcall IsNavigatorAtTarget(W8NavigatorMovementState* movement);

static_assert(sizeof(W8Octree) == 0x29c, "W8Octree_must_be_0x29c");

extern unsigned int* g_octree_storage_00659770;
extern unsigned long* g_octree_state_00659890;
extern stModelInstance* g_octree_trace_node_00659894;
extern float g_octree_cell_scale_005ebcd0;
extern unsigned long g_octree_bytes_read_00659888;
extern int g_prop_sun_index_006598ac;
extern bool g_octree_update_suspended_00659898;
extern bool g_octree_trace_enabled_00659899;
/* Renderer switches the region-link build toggles: suppress baked vertex
   lighting, force front-face culling and strip textures while sampling. */
extern unsigned char g_render_unlit_0065a0ec;
extern unsigned char g_render_cull_front_0065a0ed;
/* Inverted-depth / alternate pass-compare mode; renderTriMesh forces GEQUAL
   and the frame clear path uses a zero clear-depth while this is set. */
extern unsigned char g_inverted_depth_render_0065a0ee;
extern unsigned char g_render_untextured_0065a146;

int CheckLevelAssetSet0042CCC0(const char* level_path);

unsigned long* __fastcall PackColour00433FB0(unsigned long* color, double red, double green,
                                             double blue, double alpha);
