#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/stHeap.hpp"
#include "wiz8/engine_code/stHash.hpp"
#include "wiz8/geometry.h"

#include <stddef.h>

class stModelInstance;
class GDProp;
class GDPreProp;
class OctPrePathLog;
struct CondPathNode;
class OctPreTree;
class W8Monster;
struct W8LevelFile;
struct W8LevelFileNamedPosition;

/* One pre-path prop record handed to LinkCollideableProps: the prop's path
   name plus the GDPreProp array OctPreTree.cpp builds for it (stride 0x48). */
struct W8PreProp {
    char name[0x40];
    unsigned short num_stop_meshes_40;
    /* The running base prop number this record's pStopMeshes indices are
       relative to; InsertConditionalNodes matches a GDProp m_prop_number_02
       into [first_prop_number, first_prop_number + num_stop_meshes). */
    unsigned short first_prop_number_42;
    GDPreProp* pStopMeshes;
};

static_assert(sizeof(W8PreProp) == 0x48, "W8PreProp_must_be_0x48");
static_assert(offsetof(W8PreProp, num_stop_meshes_40) == 0x40, "W8PreProp_num_stop_meshes_40");
static_assert(offsetof(W8PreProp, pStopMeshes) == 0x44, "W8PreProp_pStopMeshes");

struct W8NavigatorMovementState;

/* Retail allocates this 0x58-byte object, calls its sole observed constructor,
   and later releases it with delete. Its constructor's entire effect is
   LoadPathParameters - reading Data\Monsters\pathparms.txt into the
   path-tuning globals - so the class is the path-parameter owner. The object
   doubles as the per-step steering context StepAlongPath hands to the
   0x004CAE50-0x004CCB60 method cluster: InitializeSteeringContext
   seeds it from the movement state each step, the steering helpers accumulate
   into force_38, and IntegrateSteering applies the result. */
class W8PathParameters {
public:
    W8PathParameters(); /* 0x004CAE40 */

    /* Seeds the context from `movement`: the owning monster, its radius, the
       speed limit (the linked navigator's movement scale when linked), the
       normalized velocity or a yaw-derived default direction, the right-hand
       perpendicular, and cleared force/query state. */
    void InitializeSteeringContext(W8NavigatorMovementState* movement); /* 0x004CAE50 */
    /* Lazily fills nearby_locations_50/nearby_count_4c with the location ids
       inside a radius-scaled box around the movement position; the cached
       result is returned on repeat calls. */
    unsigned char QueryNearbyNavigators(); /* 0x004CAFC0 */
    /* Applies force_38 for one time step: clamps it to acceleration_0c,
       integrates velocity toward speed_limit_08, resolves the heading, snaps
       the new position against the path mesh and falls back to sliding or
       stopping when the snap fails. */
    void IntegrateSteering(); /* 0x004CB090 */
    /* Advances movement_00->target_yaw toward movement_00->yaw by the shorter
       arc, accelerating or decelerating the angular velocity in
       movement_00->yaw_velocity_01c. */
    void UpdateYawSteering(float time_step, char use_turn_rate); /* 0x004CB520 */
    /* Predicts a collision with another navigator or the party inside the
       prediction window; returns nonzero when one is found ahead. */
    unsigned char PredictNavigatorCollision(); /* 0x004CB620 */
    /* Steers around geometry: brakes and deflects force_38 perpendicular to a
       clipped span, pushes toward the next waypoint when it falls behind the
       heading, or uses directional clearance for oversized radii. */
    unsigned char HandleObstacleAhead(); /* 0x004CBB70 */
    /* Adds the seek force toward target_14 scaled by
       g_path_acceleration_factor into force_38, clamped to
       acceleration_0c; with a stopped movement it instead pushes along the
       2-D target direction and zeroes speed_limit_08. */
    void AccumulateSeekForce(); /* 0x004CC1A0 */
    /* Scales speed_limit_08 down by target distance through the approach
       profile, then accumulates the seek force. */
    void SeekWithApproachSpeed(); /* 0x004CC420 */
    /* Adds a repulsion force from every linked navigator inside the combined
       radius into force_38. */
    void AccumulateGroupRepulsion(); /* 0x004CC4C0 */
    /* Steers around the linked leader: lateral pass targets, following
       distance, or a blocked-path fallback that reseeds target_14. */
    unsigned char SteerAroundLeader(char allow_path_fallback); /* 0x004CC680 */
    /* The full per-step driver for a navigator at the start of its route:
       obstacle, collision, leader and waypoint steering, group repulsion,
       then integration. */
    void SteerFromPathStart(W8NavigatorMovementState* movement, char alternate); /* 0x004CCAD0 */
    /* The mid-route driver: additionally predicts the hop height along the
       path and advances the target past waypoints the step covers, returning
       the advance result. */
    unsigned char SteerAlongPath(W8NavigatorMovementState* movement,
                                 char alternate); /* 0x004CCB60 */

private:
    W8NavigatorMovementState* movement_00;
    unsigned char padding_04[4];
    float speed_limit_08;
    float acceleration_0c;
    float velocity_length_10;
    srVector3T<float> target_14;
    srVector3T<float> direction_20;
    srVector3T<float> perpendicular_2c;
    srVector3T<float> force_38;
    float radius_44;
    unsigned char nearby_queried_48;
    bool blocked_49;
    unsigned char padding_4a[2];
    unsigned int nearby_count_4c;
    unsigned long* nearby_locations_50;
    W8Monster* monster_54;
};

static_assert(sizeof(W8PathParameters) == 0x58, "W8PathParameters_must_be_0x58");
struct W8NavigatorAttachment;

/* OctPath.cpp's two compact graph records. Surface zero and edge zero are
   sentinels; live records are addressed by their unsigned-short indices. */
struct W8PathSurface {
    unsigned short flags_00;
    unsigned short index_02;
    srVector3T<float> position_04;
    unsigned short parent_10;
    unsigned char padding_12[0x02];
    /* Monotonic visit stamp: patrol selection picks the smallest value, and
       the mover writes the game-time tick (or accumulated distance) as each
       waypoint is consumed. */
    unsigned int visit_stamp_14;
    /* A* heuristic: distance to the goal scaled by g_float_005ec394, cached by
       FindPath while the surface is open. */
    float heuristic_18;
    float cost_1c;
    float remaining_cost_20;
    unsigned short first_edge_24;
    unsigned short padding_26;
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

/* One named conditional-path set. FindPathHandle compares path.name and then
   walks the zero-terminated lookup run starting at lookup_index: each lookup
   names a key, each key packs two region halfwords, and the key's parallel
   value word carries the height in its low half. */
/* The four-cell X/Z rectangle a conditional path covers, written by
   FindPathHandle from the low and high halves of its node keys. */
struct W8PathGridBounds {
    unsigned short min_x;
    unsigned short max_x;
    unsigned short min_z;
    unsigned short max_z;
};

/* The vertical span a conditional path covers: the minimum and maximum are
   filled by FindPathHandle; the middle slot stays a caller-managed sentinel
   (GDProp keeps its bound midpoint there). */
struct W8PathVerticalRange {
    float minimum;
    float sentinel;
    float maximum;
};

struct GDPropCondPaths {
    char name[0x40];
    unsigned int lookup_index; /* 0x40 */
};

static_assert(sizeof(GDPropCondPaths) == 0x44, "GDPropCondPaths_must_be_0x44");

/* The two-dimensional cell walk used by path-surface probing. It retains the
   three-component shape of the octree walker, but only X and Z participate in
   its Bresenham step; the remaining slots are zeroed by the builder. */
struct W8PathGridWalk {
    int cell_00[2];    /* 0x00: starting X/Z path cells */
    int padding_08;    /* 0x08: zero */
    int step_0c[2];    /* 0x0c: +1 or -1 per axis */
    int padding_14;    /* 0x14: zero */
    int major_axis_18; /* 0x18: 0 for X, 1 for Z */
    int minor_axis_1c; /* 0x1c: (major + 1) % 2 */
    int padding_20;    /* 0x20: zero */
    int count_24;      /* 0x24: cells to visit */
    int error_28;      /* 0x28 */
    int error_2c;      /* 0x2c */
    int cell_size_30;  /* 0x30 */
    int padding_34[3]; /* 0x34: zero */
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

static_assert(sizeof(W8PathProbeVolume) == 0x18, "W8PathProbeVolume_must_be_0x18");

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
    ~W8PathHeapHandle()
    {
        delete heap_00;
    }

    W8PathHeap* heap_00;
    unsigned int root_node_04;

    void DeleteRoot(W8PathSearchNode* node);
};

static_assert(sizeof(W8PathHeapEntry) == 8, "W8PathHeapEntry_must_be_8");
static_assert(sizeof(W8PathHeap) == 0x10, "W8PathHeap_must_be_0x10");
static_assert(sizeof(W8PathHeapHandle) == 8, "W8PathHeapHandle_must_be_8");

static_assert(sizeof(W8PathSearchNode) == 0x2c, "W8PathSearchNode_must_be_0x2c");

/* The pathing service the octree builds when its file carries one. Its own
   constructor at 0x004578E0 initialises through 0x238 and ReadOctFile allocates
   0x240, which is what fixes the extent; only the fields those two bodies and
   the path lookup reach are named. */
class W8PathingService {
public:
    W8PathingService(); /* 0x004578E0 */
    unsigned int FindPathHandle(const char* path_name, W8PathGridBounds* path_bounds,
                                W8PathVerticalRange* path_range); /* 0x00457CF0 */
    /* Neither takes a prop: both walk the service's own surface and edge
       tables, and their receiver is the service. */
    /* The two operations W8Octree::AdvanceNavigator delegates to: it loads
       the service into ecx before either call, so both are its methods and
       not the free functions they were declared as. */
    unsigned int StepAlongPath(W8NavigatorMovementState* movement, float radius, float separation);
    unsigned int StepMonsterAlongPath(W8NavigatorMovementState* movement, float radius,
                                      float separation);
    void LinkSurfaces(GDProp* prop); /* 0x00460020 */
    void LinkEdges(GDProp* prop);    /* 0x004600B0 */
    void CheckConditionalWayPtStatus004601B0(unsigned short count, unsigned short* waypoints);
    void CheckConditionalLinkStatus00460250(unsigned short count, unsigned short* edges);
    void SetConditionalPathFrame(unsigned int path_handle, short frame);
    unsigned int FindConditionalPathValue(unsigned int key, unsigned int value);
    void
    LinkCollideableProps(int lNumProps, W8PreProp* pPreProps,
                         W8HashTable<unsigned int, CondPathNode*>* pCondValues); /* 0x004CE510 */
    unsigned char HandlePathEdgeTransition(W8NavigatorMovementState* movement);
    void ReduceWaypointCosts(unsigned int waypoint, float amount);
    unsigned char AdvanceAttachmentWaypoint(const srVector3T<float>* source,
                                            struct W8NavigatorAttachment* attachment);
    unsigned char MatchesPathProbe(unsigned int tag, const float* radius,
                                   const srVector3T<float>* position);
    unsigned short AllocateSearchNode();
    unsigned char CanReachSearchNode(const srVector3T<float>* position, unsigned short target_node,
                                     float clearance);
    void AdjustFinalPathEndpoint(W8NavigatorMovementState* movement, float radius,
                                 float separation);
    void UpdateConditionalPathFlags(unsigned int path_handle, unsigned short frame,
                                    unsigned int flags);
    int ProcessSearchNodeProps(unsigned int node_index, unsigned char first_only);
    unsigned int CollectPathProbes(W8NavigatorMovementState* movement, float radius);
    unsigned short PlanMovement(W8NavigatorMovementState* movement, float radius, float separation);
    unsigned short PlanMovementToPosition(W8NavigatorMovementState* movement,
                                          const srVector3T<float>* target, float radius,
                                          float separation);
    float UpdateSearchNodeScore(unsigned int node, const srVector3T<float>* position, float minimum,
                                float maximum);
    unsigned short ResolveSearchNodeCollisions(W8NavigatorMovementState* movement,
                                               unsigned int node, float radius, float separation);
    unsigned char TestSearchPositionVisibility(const srVector3T<float>* position,
                                               W8NavigatorMovementState* movement);
    unsigned short ConfigureMovementSearch(W8NavigatorMovementState* movement, int target_location,
                                           float radius, float separation, float maximum_distance,
                                           srVector3T<float> trace_offset, int trace_mode,
                                           float target_height_offset, float target_yaw,
                                           unsigned char* probe_result);
    unsigned char ResolvePathCell(unsigned int key, unsigned char allow_dynamic,
                                  unsigned int* height, float* direction, float* vertical,
                                  unsigned char* dynamic);
    unsigned short FindWaypoint(const srVector3T<float>* position, unsigned char exhaustive);
    void SnapPathHeight(srVector3T<float>* position);
    void GetPathSurfaceNormal0045B730(const srVector3T<float>* position, srVector3T<float>* normal);
    void ActivateMovementTrigger(W8NavigatorMovementState* movement, unsigned char use_path_edge);
    void UpdatePathVisualization0045BC40(const srVector3T<float>* source,
                                         const srVector3T<float>* destination);
    void DrawPathPosition(srVector3T<float> position, unsigned char mode);
    void BuildSearchVisualization();
    stModelInstance* BuildPathVisualization();
    stModelInstance* EnsurePathVisualization();
    void GetWaypointVisualizationColor(unsigned short waypoint, srVector3T<float>* color);
    short CollectPathVisualization(const srVector3T<float>* position);
    unsigned char PreparePathVisualization(const srVector3T<float>* source,
                                           const srVector3T<float>* direction);
    void AddWaypoint(const srVector3T<float>* position);
    unsigned int ClassifyWaypoint(const srVector3T<float>* position);
    unsigned char SnapWaypointPosition(srVector3T<float>* position, unsigned char snap_to_cell);
    unsigned char TestPathCellClearance(srVector3T<float>* position, float clearance,
                                        unsigned char snap_to_cell);
    unsigned char SnapToLowerPathCell(srVector3T<float>* position, unsigned char allow_directional);
    unsigned char ProbeAttachmentPath(W8NavigatorAttachment* attachment);
    unsigned int FindPathCell(srVector3T<float>* position, unsigned int* cell,
                              unsigned char adjust);
    unsigned char BuildAttachmentPath(W8NavigatorAttachment* attachment, unsigned int flags);
    unsigned char PrepareLinkedNavigator(W8NavigatorMovementState* movement);
    unsigned char LinkAttachmentTarget(W8NavigatorAttachment* attachment, unsigned int flags,
                                       const srVector3T<float>* target, float separation);
    unsigned char BuildPatrolPath(W8NavigatorAttachment* attachment, unsigned int flags,
                                  const srVector3T<float>* target, float minimum,
                                  const srVector3T<float>* velocity, float maximum);
    void ProbeWaypointArc(const srVector3T<float>* from, const srVector3T<float>* to);
    void GetPathGridStepDirections(const W8PathGridWalk* walk, int* directions);
    void BuildPathGridWalk(const srVector2T<float>* from, const srVector2T<float>* to,
                           const srVector2T<float>* origin, W8PathGridWalk* walk);
    unsigned char ProbeWaypointSegment(const srVector3T<float>* from, const srVector3T<float>* to);
    unsigned int ComputeWaypointNeighborMask(const int* cell, unsigned int path_value);
    /* Sums the blocked-direction unit vectors among the directions `delta`
       points toward and normalizes the result into `direction`; zero when
       `mask` is fully open or nothing wanted is blocked. */
    unsigned char ComputeFreeDirection(unsigned int mask, const srVector3T<float>* delta,
                                       srVector3T<float>* direction);
    /* Resolves the path cell under `position`, reads its neighbor mask and
       computes the free-direction vector away from `delta`; zero when the
       heading has no free neighbor. */
    unsigned char GetNeighborSlideDirection(const srVector3T<float>* position,
                                            const srVector3T<float>* delta,
                                            srVector3T<float>* direction);
    /* The same free-direction query against the stored waypoint neighbor mask
       rather than a live cell lookup. */
    unsigned char GetObstacleDirection(const srVector3T<float>* delta,
                                       srVector3T<float>* direction);
    /* A* from the attachment's start to its destination over the surface
       graph; returns the destination surface index, zero when unreachable. */
    unsigned int FindPath(W8NavigatorAttachment* attachment, unsigned int flags);
    /* Depth-first patrol search from `waypoint`: accumulates per-link path
       costs against the randomized patrol_distance target, tracking the
       argmin-key fallback nodes, and returns the reached endpoint or zero.
       Retail names it in the "Too many links" assert. */
    /* Depth-first link search from `waypoint` toward the target stored in
       patrol_start_1ec by LinkAttachmentTarget: collects admissible
       edge destinations (filtered like FindPath), prices each by accumulated
       link cost plus distance-to-target, sorts by that key, then returns the
       first candidate beyond patrol_distance_1e8 or the first nonzero
       recursive result. probe_cell_key_078 tracks the farthest candidate. */
    unsigned short RecurseTargetLinks(unsigned short waypoint); /* 0x004615D0 */
    unsigned short RecursePatrolLinks00461D10(unsigned short waypoint);
    float MeasureDirectionalPath(const int* cell, int direction, unsigned int height,
                                 float distance);
    float CompareDirectionalClearance(const srVector3T<float>* position,
                                      const srVector3T<float>* direction, float distance);
    void SetWaypointLinkFlags(unsigned short waypoint, unsigned int direction);
    void RemoveWaypointLink(unsigned short edge);
    void AddWaypointLink(unsigned short source, unsigned short destination, unsigned int flags);
    unsigned char UpdateWaypointLink(unsigned short source, unsigned short destination,
                                     unsigned int flags);
    unsigned char HasDirectionalWaypointLink(unsigned short source, unsigned short destination);
    unsigned char TestWaypointSpan(const srVector3T<float>* source, srVector3T<float>* destination,
                                   unsigned char adjust_destination, unsigned char diagonal_steps);
    /* `range` carries the walk budget in and the path cost back out; `hops`
       returns the reached-waypoint count. */
    unsigned char MeasureAttachmentPath(const srVector3T<float>* from, srVector3T<float>* to,
                                        float* range, int* hops); /* 0x004604B0 */
    /* Whether the hop at the attachment's current index crosses a disabled
       conditional edge whose segment box holds a door prop - and that prop's
       door-trigger action data is a type-10 record with flag bit0 clear. */
    unsigned char TestAttachmentHopDoor(W8NavigatorAttachment* attachment); /* 0x00460680 */
    unsigned int EditWaypointLinkFlags(const char* title, unsigned int* flags,
                                       unsigned int direction);
    void EditTeleportalLink(const srVector3T<float>* destination,
                            const srVector3T<float>* source); /* 0x0045F2D0 */
    /* Takes the size, two loose values, the bounds block out of the octree
       header, and the level name the octree already owns. */
    void ConfigureForLevel(int size, float grid_scale, int path_clearance,
                           const W8BoundingBox* bounds, const char* name); /* 0x00458A50 */
    unsigned char Load00458CE0(int handle);                                /* 0x00458CE0 */
    unsigned char WritePathNodes00458AD0(unsigned int handle);
    unsigned char SaveWaypointSnapshot(unsigned char force);
    unsigned char WriteWaypointFile00459540();
    unsigned char ReadWaypointFile00459650();
    void BuildWaypointFileData();
    /* Owning destructor: releases owned tables and bit sets. Single caller
       destroys the service at octree teardown. */
    ~W8PathingService(); /* 0x00457B10 */

    /* The active edge-filter mask for patrol/path searches. ConfigureForLevel
       loads it from the octree header; BuildPatrolPath stores its `flags` here
       for FindPatrolPath. */
    unsigned int path_flags_000;
    int size_004; /* 0x04 */
    /* PrePathing's CreatePathNodeArray counts edge nodes here starting from
       one, and WriteOctFile serializes it beside the node count. */
    int edge_node_count_008;
    /* ReadOctFile tests this beside waypoint_editing_1c8 before settling a portal. */
    unsigned int m_ulNumWayPoints;  /* 0x0c */
    unsigned int m_ulNumWayPtLinks; /* 0x10 */
    int m_padding_014;
    /* Incremented for each edge removed by the waypoint editor; never read. */
    int m_removed_edge_count_018;
    /* The grid divisor both linking walks divide by. */
    float grid_scale_01c; /* 0x1c */
    float span_020;       /* 0x20 */
    short cell_count_024; /* 0x24 */
    unsigned short m_padding_026;
    /* Path probe-clearance height, raw float bits from the octree
       header word; only ConfigureForLevel writes it. */
    int path_clearance_028; /* 0x28 */
    float level_bounds[6];  /* 0x2c: serialized minimum/maximum pair; the ctor's
                             counted six-store loop proves the authored array */
    /* Four malloc'd tables and one polymorphic object, all released by
       0x00457B10 - the first four with free, the last through its own
       deleting slot. */
    unsigned int* path_nodes_044; /* 0x44: serialized key/value pairs */
    /* Surfaces are 0x28 bytes apart, edges 0xe; an edge names two surfaces by
       index in its two shorts at +4 and +6. */
    W8PathSurface* m_pSurfaces_048;        /* 0x48 */
    W8PathEdge* m_pEdges_04c;              /* 0x4c */
    W8FileWaypoint* m_pFileWayPoints;      /* 0x50 */
    stModelInstance* m_pPathModelInstance; /* 0x54 */
    BitArray* visible_waypoints_058;       /* 0x58 */
    BitArray* rendered_waypoints_05c;      /* 0x5c */
    BitArray* collected_waypoints_060;     /* 0x60 */
    /* Two hash indexes the loader builds and 0x00457B10 destroys. The path
       value words are bitfields (height in the low half, state flags in the
       high bits), so 0x64 takes unsigned values; 0x74 is the visited-cell set
       and keeps the signed value the octree registry also instantiates. The
       template only copies and compares values, so the two instantiations are
       body-equivalent and retail's linker folds them. */
    W8HashTable<unsigned int, unsigned int>* m_pPathValues_064; /* 0x64 */
    const char* level_name;                                     /* 0x68 */
    W8PathHeapHandle* path_heap_06c;                            /* 0x6c */
    float path_cost_limit_070;                                  /* 0x70: starts 1.0e10f */
    W8HashTable<unsigned int, int>* m_pVisitedCells_074;        /* 0x74 */
    unsigned int probe_cell_key_078;                            /* 0x78 */
    srVector3T<float> probe_position_07c;                       /* 0x7c */
    unsigned int probe_limit_088;                               /* 0x88 */
    bool probe_bounded_08c;                                     /* 0x8c */
    unsigned char m_padding_08d[3];
    unsigned int planner_location_090;
    unsigned int path_candidate_count_094;
    unsigned long* path_candidates_098;
    bool explicit_target_09c; /* 0x9c */
    unsigned char m_padding_09d[3];
    unsigned int waypoint_neighbor_mask_0a0; /* 0xa0 */
    bool trace_configured_0a4;               /* 0xa4 */
    unsigned char m_padding_0a5[3];
    float trace_max_distance_0a8;
    srVector3T<float> trace_offset_0ac;
    int trace_mode_0b8;
    float trace_height_offset_0bc;
    int trace_target_location_0c0;
    float trace_target_yaw_0c4;
    W8PathSearchNode* m_owned_0c8; /* 0xc8 */
    unsigned int search_node_count_0cc;
    unsigned int search_node_capacity_0d0;
    unsigned int path_probe_count_0d4;
    W8PathProbeVolume path_probes_0d8[10];
    bool waypoint_editing_1c8; /* 0x1c8 */
    bool flag_1c9;
    bool flag_1ca;
    bool search_visualization_1cb;
    bool waypoints_dirty_1cc;
    unsigned char m_padding_1cd;
    unsigned short path_flags_1ce; /* 0x1ce: starts 4 */
    int link_flags_1d0;
    unsigned short start_waypoint_1d4;
    unsigned short destination_waypoint_1d6;
    unsigned short saved_surface_1d8;
    bool path_direction_valid_1da;
    unsigned char m_padding_1db;
    /* Patrol-search state laid down by BuildPatrolPath and consulted by the
       recursive FindPatrolPath: the argmin-key candidate node, the accepted
       min/max start-to-destination range, the randomized target path cost,
       the start and destination positions, and the best alternate
       candidate's cost. */
    unsigned int patrol_node_1dc;
    float patrol_min_1e0;
    float patrol_max_1e4;
    float patrol_distance_1e8;
    srVector3T<float> patrol_start_1ec;
    srVector3T<float> patrol_destination_1f8;
    unsigned char m_padding_204[0x0c];
    float patrol_cost_210;
    W8PathParameters* path_parameters_214; /* 0x214 */
    W8NavigatorAttachment* linked_attachment_218;
    /* The conditional path tables. ReadPathNodes at 0x00458CE0 asserts on the
       first by name and names the other four in its own failure messages: a
       lookup, a frame, a key and a value array, sized from the two counts.
       FindPathHandle scans the 0x44-byte path records by name. */
    GDPropCondPaths* m_pCondPaths;       /* 0x21c */
    int m_ulNumCondPaths;                /* 0x220 */
    int m_ulNumCondFrames;               /* 0x224 */
    int m_ulNumCondNodes;                /* 0x228 */
    unsigned int* m_pulCondLookup;       /* 0x22c */
    unsigned short* m_pusCondNodeFrames; /* 0x230 */
    unsigned int* m_pulCondNodeKeys;     /* 0x234 */
    unsigned int* m_pulCondNodeValues;   /* 0x238 */
    bool span_blocked_23c;
    unsigned char m_padding_23d[3];
};

static_assert(sizeof(W8PathingService) == 0x240, "W8PathingService_must_be_0x240");

/* The 8-byte record InsertConditionalNodes hangs on the cond map under a
   (frame << 16 | preprop index + 1) key: the node's serialized flag word
   (height level + 1 in the low half, |0x02000000 when it came from the
   blocker run) and its packed cell.  LinkCollideableProps drains the map into
   the serialized key/value tables and frees each record.  The name is the
   original spelling from the allocation-failure assertion text. */
struct CondPathNode {
    unsigned int value; /* 0x00 */
    unsigned int cell;  /* 0x04 */
};

/* The 0x10-byte build-time path-node record PrePathing::GetPathNode hands out
   of its 1000-record chunks: floor index in the low bits plus link, clearance
   and state flags; the packed cell; world height; and the same-cell chain. */
struct W8PrePathNode {
    unsigned int level_flags; /* 0x00 */
    unsigned int cell;        /* 0x04: z << 16 | x */
    float y;                  /* 0x08 */
    W8PrePathNode* next;      /* 0x0c: allocation order, same-cell runs */
};

static_assert(sizeof(W8PrePathNode) == 0x10, "W8PrePathNode_must_be_0x10");

/* OctPrePath.cpp's 0x1204-byte build-time pathing service ("PrePathing" in its
   own assertions): its constructor runs the W8PathingService constructor then
   initialises scratch state through +0x1200, and the pre-tree stores it at
   +0x2a0. */
class PrePathing : public W8PathingService {
public:
    PrePathing();  /* 0x004CCFD0 */
    ~PrePathing(); /* 0x004CD030 */

    /* Copies each named position (scaled to world units) into +0x250 and
       snaps it to the ground through `octree`. */
    int SnapNamedPositions(W8LevelFileNamedPosition* positions, int count,
                           unsigned int min_component_percent, OctPreTree* octree);
    /* Hands out the next 0x10-byte path-node record, allocating a new
       0x3e80-byte chunk (1000 records) when the current one fills. */
    W8PrePathNode* GetPathNode();
    unsigned char BuildPathList(W8PrePathNode* nodes, W8HashTable<unsigned int, int>* cell_map);
    unsigned char LinkPathNodes();
    void PropagatePathNodeClearance(W8PrePathNode* node, unsigned int depth);
    unsigned int DeleteUnreachableAreas();
    int CreatePathNodeArray();
    unsigned char CreateAutomapNodes(W8LevelFile* level);

    W8PrePathNode** path_node_list_240; /* size_004 entries */
    OctPrePathLog* path_log_244;
    /* A malloc'd buffer the destructor `free`s; no surviving writer. */
    void* owned_248;
    int named_position_count_24c;
    srVector3T<float>* named_positions_250;
    W8HashTable<unsigned int, int>* cell_map_254;
    /* Embedded chunk table: each slot is a malloc'd 0x3e80-byte run of
       0x10-byte path-node records. The constructor fills slot 0, and the
       destructor frees every slot through chunk_index_11f8 inclusive. */
    W8PrePathNode* node_chunks_258[0x3e8];
    int chunk_index_11f8;
    int chunk_node_count_11fc;
    /* Components smaller than this percent of the node count get deleted
       while linking; capped at 50. */
    unsigned int min_component_percent_1200;
};

static_assert(sizeof(PrePathing) == 0x1204, "PrePathing_must_be_0x1204");
static_assert(offsetof(PrePathing, path_node_list_240) == 0x240, "PrePathing_path_node_list_240");
static_assert(offsetof(PrePathing, node_chunks_258) == 0x258, "PrePathing_node_chunks_258");
static_assert(offsetof(PrePathing, chunk_index_11f8) == 0x11f8, "PrePathing_chunk_index_11f8");
static_assert(offsetof(PrePathing, min_component_percent_1200) == 0x1200,
              "PrePathing_min_component_percent_1200");

/* Move an integer path cell one compass step; directions outside the
   eight-value range wrap once. */
void __stdcall StepPathCell(int* x, int* z, int direction);

extern W8PathingService* g_pathing;
extern unsigned short g_path_reserve;
extern float g_path_span_scale;
extern double g_double_005ec3b0;
/* The -1.0 no-route sentinel MeasurePathDistance returns. */
extern const double g_double_005ec2e8;
/* 0x005ED300: OctPrePath.cpp's vertical-link slack; retail Combat.cpp reads
   it directly when sizing a monster's move. */
extern float g_prepath_link_height;
