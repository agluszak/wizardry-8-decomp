#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/stHeap.hpp"

class GDProp;
class stModelInstance005EC7D0;

/* The 0x58-byte state object the pathing constructor builds and 0x00457B10
   gives back with a plain operator delete, so it has no destructor of its own.
   Its constructor does nothing but run the one at 0x004CCCB0. */
class W8PathState004CAE40 {
public:
    W8PathState004CAE40();               /* 0x004CAE40 */
};
struct W8NavigatorMovementState;
struct W8NavigatorAttachment;

/* OctPath.cpp's two compact graph records. Surface zero and edge zero are
   sentinels; live records are addressed by their unsigned-short indices. */
struct W8PathSurface {
    unsigned short flags_00;
    unsigned short index_02;
    srVector3T<float> position_04;
    unsigned short parent_10;
    unsigned char positional_12[0x0a];
    float cost_1c;
    float remaining_cost_20;
    unsigned short first_edge_24;
    unsigned short positional_26;
};

/* The compact surface record written to a .WPT file. It retains only the
   persistent flags, first edge and world position from the live 0x28-byte
   surface. */
struct W8FileWaypoint {
    unsigned short flags_00;
    unsigned short first_edge_02;
    srVector3T<float> position_04;
};

#pragma pack(push, 1)
struct W8PathEdge {
    unsigned int flags_00;
    unsigned short source_04;
    unsigned short destination_06;
    float distance_08;
    unsigned short next_0c;
};
#pragma pack(pop)

static_assert(sizeof(W8PathSurface) == 0x28, "W8PathSurface_must_be_0x28");
static_assert(sizeof(W8PathEdge) == 0x0e, "W8PathEdge_must_be_0x0e");
static_assert(sizeof(W8FileWaypoint) == 0x10, "W8FileWaypoint_must_be_0x10");

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

/* The two-dimensional cell walk used by path-surface probing. It retains the
   three-component shape of the octree walker, but only X and Z participate in
   its Bresenham step; the remaining slots are zeroed by the builder. */
struct W8PathGridWalk {
    int cell_00[2];                      /* 0x00: starting X/Z path cells */
    int value_08;                        /* 0x08: zero */
    int step_0c[2];                      /* 0x0c: +1 or -1 per axis */
    int value_14;                        /* 0x14: zero */
    int major_axis_18;                   /* 0x18: 0 for X, 1 for Z */
    int minor_axis_1c;                   /* 0x1c: (major + 1) % 2 */
    int value_20;                        /* 0x20: zero */
    int count_24;                        /* 0x24: cells to visit */
    int error_28;                        /* 0x28 */
    int error_2c;                        /* 0x2c */
    int cell_size_30;                    /* 0x30 */
    int value_34[3];                     /* 0x34: zero */
};

static_assert(sizeof(W8PathGridWalk) == 0x40, "W8PathGridWalk_must_be_0x40");

/* One of the fixed probe volumes assembled by 0x004656A0. The matcher at
   0x00465970 proves the tag, outer and inner radii, and center. */
struct W8PathProbeVolume {
    unsigned int tag_00;
    float outer_radius_04;
    float inner_radius_08;
    srVector3T<float> center_0c;
};

static_assert(sizeof(W8PathProbeVolume) == 0x18,
              "W8PathProbeVolume_must_be_0x18");

/* The fixed 0x2c search node allocated by W8PathingService's constructor.
   Scoring at 0x00464FF0 proves the flag word, base score, current distance,
   accumulated score and world position; the remaining planner state stays
   positional until its readers are recovered. */
struct W8PathSearchNode {
    unsigned short flags_00;
    unsigned short node_index_02;
    unsigned short cell_x_04;
    unsigned short cell_z_06;
    unsigned short path_height_08;
    unsigned short parent_node_0a;
    float base_score_0c;
    float path_cost_10;
    float distance_14;
    float clearance_18;
    float score_1c;
    srVector3T<float> position_20;
};

struct W8PathHeapEntry {
    unsigned int node_00;
    unsigned int priority_04;

    bool operator<=(const W8PathHeapEntry& other) const
    {
        return priority_04 <= other.priority_04;
    }
};

typedef stHeap<W8PathHeapEntry> W8PathHeap;

struct W8PathHeapHandle {
    W8PathHeap* heap_00;
    unsigned int root_node_04;

    void DeleteRoot004577F0(W8PathSearchNode* node);
};

static_assert(sizeof(W8PathHeapEntry) == 8, "W8PathHeapEntry_must_be_8");
static_assert(sizeof(W8PathHeap) == 0x10, "W8PathHeap_must_be_0x10");
static_assert(sizeof(W8PathHeapHandle) == 8, "W8PathHeapHandle_must_be_8");

static_assert(sizeof(W8PathSearchNode) == 0x2c,
              "W8PathSearchNode_must_be_0x2c");

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
    void CheckConditionalWaypointStatus004601B0(
        unsigned short count, unsigned short* waypoints);
    void CheckConditionalLinkStatus00460250(
        unsigned short count, unsigned short* edges);
    void SetConditionalPathFrame00457EA0(
        unsigned int path_handle, short frame);
    unsigned int FindConditionalPathValue00458970(
        unsigned int key, unsigned int value);
    unsigned char HandlePathEdgeTransition00460350(
        W8NavigatorMovementState* movement);
    void ReduceWaypointCosts00462220(
        unsigned int waypoint, float amount);
    unsigned char AdvanceAttachmentWaypoint00462DE0(
        const srVector3T<float>* source,
        struct W8NavigatorAttachment* attachment);
    unsigned char MatchesPathProbe00465970(
        unsigned int tag,
        const float* radius,
        const srVector3T<float>* position);
    unsigned short AllocateSearchNode00465A00();
    unsigned char CanReachSearchNode00465AF0(
        const srVector3T<float>* position,
        unsigned short target_node,
        float clearance);
    void AdjustFinalPathEndpoint00465D70(
        W8NavigatorMovementState* movement,
        float radius,
        float separation);
    void UpdateConditionalPathFlags00465FB0(
        unsigned int path_handle,
        unsigned short frame,
        unsigned int flags);
    int ProcessSearchNodeProps004663D0(
        unsigned int node_index, unsigned char first_only);
    unsigned int CollectPathProbes004656A0(
        W8NavigatorMovementState* movement, float radius);
    unsigned short PlanMovement00463460(
        W8NavigatorMovementState* movement, float radius, float separation);
    unsigned short PlanMovementToPosition00464AB0(
        W8NavigatorMovementState* movement,
        const srVector3T<float>* target,
        float radius,
        float separation);
    float UpdateSearchNodeScore00464FF0(
        unsigned int node,
        const srVector3T<float>* position,
        float minimum,
        float maximum);
    unsigned short ResolveSearchNodeCollisions00465130(
        W8NavigatorMovementState* movement,
        unsigned int node,
        float radius,
        float separation);
    unsigned char TestSearchPositionVisibility00464CC0(
        const srVector3T<float>* position,
        W8NavigatorMovementState* movement);
    unsigned short ConfigureMovementSearch00464B00(
        W8NavigatorMovementState* movement,
        int target_location,
        float radius,
        float separation,
        float maximum_distance,
        float offset_x,
        float offset_y,
        float offset_z,
        int trace_mode,
        float target_height_offset,
        float target_yaw,
        unsigned char* probe_result);
    unsigned char ResolvePathCell004648D0(
        unsigned int key,
        unsigned char allow_dynamic,
        unsigned int* height,
        float* direction,
        float* vertical,
        unsigned char* dynamic);
    unsigned short FindWaypoint0045B120(
        const srVector3T<float>* position, unsigned char exhaustive);
    void SnapPathHeight0045B5A0(srVector3T<float>* position);
    void GetPathSurfaceNormal0045B730(
        const srVector3T<float>* position, srVector3T<float>* normal);
    void ActivateMovementTrigger0045B880(
        W8NavigatorMovementState* movement, unsigned char use_path_edge);
    void UpdatePathVisualization0045BC40(
        const srVector3T<float>* source,
        const srVector3T<float>* destination);
    void DrawPathPosition0045C9A0(
        srVector3T<float> position, unsigned char mode);
    void BuildSearchVisualization0045CFD0();
    stModelInstance005EC7D0* BuildPathVisualization0045BE30();
    stModelInstance005EC7D0* EnsurePathVisualization0045D530();
    void GetWaypointVisualizationColor0045D490(
        unsigned short waypoint, srVector3T<float>* color);
    short CollectPathVisualization0045D880(
        const srVector3T<float>* position);
    unsigned char PreparePathVisualization0045E840(
        const srVector3T<float>* source,
        const srVector3T<float>* direction);
    void AddWaypoint0045DDB0(const srVector3T<float>* position);
    unsigned int ClassifyWaypoint00459C00(const srVector3T<float>* position);
    unsigned char SnapWaypointPosition00462E60(
        srVector3T<float>* position, unsigned char snap_to_cell);
    unsigned char TestPathCellClearance00463040(
        srVector3T<float>* position,
        float clearance,
        unsigned char snap_to_cell);
    unsigned char SnapToLowerPathCell00463290(
        srVector3T<float>* position, unsigned char allow_directional);
    unsigned char ProbeAttachmentPath00462360(
        W8NavigatorAttachment* attachment);
    void ProbeWaypointArc00462570(
        const srVector3T<float>* from, const srVector3T<float>* to);
    void GetPathGridStepDirections0045AEE0(
        const W8PathGridWalk* walk, int* directions);
    void BuildPathGridWalk0045AF60(
        const float* from,
        const float* to,
        const float* origin,
        W8PathGridWalk* walk);
    unsigned char ProbeWaypointSegment00462750(
        const srVector3T<float>* from, const srVector3T<float>* to);
    unsigned int ComputeWaypointNeighborMask004667A0(
        const int* cell, unsigned int path_value);
    float MeasureDirectionalPath0045AC70(
        const int* cell,
        int direction,
        unsigned int height,
        float distance);
    float CompareDirectionalClearance0045AAC0(
        const srVector3T<float>* position,
        const srVector3T<float>* direction,
        float distance);
    void SetWaypointLinkFlags0045E030(unsigned short waypoint, unsigned int direction);
    void RemoveWaypointLink0045E360(unsigned short edge);
    void AddWaypointLink0045EC30(
        unsigned short source, unsigned short destination, unsigned int flags);
    unsigned char UpdateWaypointLink0045F200(
        unsigned short source, unsigned short destination, unsigned int flags);
    unsigned char HasDirectionalWaypointLink0045EF90(
        unsigned short source, unsigned short destination);
    unsigned char TestWaypointSpan0045A1B0(
        const srVector3T<float>* source,
        srVector3T<float>* destination,
        unsigned char adjust_destination,
        unsigned char diagonal_steps);
    unsigned int EditWaypointLinkFlags0045F530(
        const char* title, unsigned int* flags, unsigned int direction);
    void EditTeleportalLink(
        const srVector3T<float>* destination,
        const srVector3T<float>* source); /* 0x0045F2D0 */
    /* Takes the size, two loose values, the six-float bounds block out of the
       octree header, and the level name the octree already owns. */
    void ConfigureForLevel(
        int size, float grid_scale, int value_28, const float* bounds,
        const char* name);                /* 0x00458A50 */
    unsigned char Load00458CE0(int handle); /* 0x00458CE0 */
    unsigned char WritePathNodes00458AD0(unsigned int handle);
    unsigned char SaveWaypointSnapshot00459400(unsigned char force);
    unsigned char WriteWaypointFile00459540();
    unsigned char ReadWaypointFile00459650();
    void BuildWaypointFileData0045E440();
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
    unsigned int* path_nodes_044;        /* 0x44: serialized key/value pairs */
    /* Surfaces are 0x28 bytes apart, edges 0xe; an edge names two surfaces by
       index in its two shorts at +4 and +6. */
    W8PathSurface* m_pSurfaces_048;      /* 0x48 */
    W8PathEdge* m_pEdges_04c;            /* 0x4c */
    W8FileWaypoint* file_waypoints_050; /* 0x50 */
    stModelInstance005EC7D0* m_owned_054; /* 0x54 */
    BitArray* visible_waypoints_058;     /* 0x58 */
    BitArray* rendered_waypoints_05c;    /* 0x5c */
    BitArray* collected_waypoints_060;   /* 0x60 */
    /* Two hash indexes the loader builds and 0x00457B10 tears down the same
       way DestroyIndex does. */
    void* m_pIndex_064;                  /* 0x64 */
    const char* level_name;                /* 0x68 */
    W8PathHeapHandle* path_heap_06c;     /* 0x6c */
    unsigned int m_positional_070;       /* 0x70: starts 0x501502f9 */
    void* m_pIndex_074;                  /* 0x74 */
    unsigned int probe_cell_key_078;     /* 0x78 */
    srVector3T<float> probe_position_07c; /* 0x7c */
    unsigned int probe_limit_088;        /* 0x88 */
    unsigned char flag_08c;              /* 0x8c */
    unsigned char m_positional_08d[3];
    unsigned int planner_location_090;
    unsigned int path_candidate_count_094;
    int* path_candidates_098;
    unsigned char flag_09c;              /* 0x9c */
    unsigned char m_positional_09d[3];
    unsigned int waypoint_neighbor_mask_0a0; /* 0xa0 */
    unsigned char flag_0a4;              /* 0xa4 */
    unsigned char m_padding_0a5[3];
    float trace_max_distance_0a8;
    srVector3T<float> trace_offset_0ac;
    int trace_mode_0b8;
    float trace_height_offset_0bc;
    int trace_target_location_0c0;
    float trace_target_yaw_0c4;
    W8PathSearchNode* m_owned_0c8;       /* 0xc8 */
    unsigned int search_node_count_0cc;
    unsigned int search_node_capacity_0d0;
    unsigned int path_probe_count_0d4;
    W8PathProbeVolume path_probes_0d8[10];
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
    unsigned char path_direction_valid_1da;
    unsigned char m_positional_1db[0x39];
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
    unsigned char flag_23c;
    unsigned char m_padding_23d[3];
};

static_assert(sizeof(W8PathingService) == 0x240,
              "W8PathingService_must_be_0x240");

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
    unsigned char positional_29c[0x104];
    unsigned long polygon_cursor_3a0;
    void* positional_3a4;
    unsigned long positional_3a8;
    unsigned long positional_3ac;
    unsigned long positional_3b0;
    unsigned long positional_3b4;
    void* positional_3b8;
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
