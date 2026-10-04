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

/* The .oct path hash array: one packed X/Z key and its height/state word.
   Build-time producers and the loader agree on an eight-byte stride. */
struct W8FilePathNode {
    unsigned int cell;
    unsigned int level_flags;
};
static_assert(sizeof(W8FilePathNode) == 8, "W8FilePathNode_size");

/* Header preceding the conditional path tables. The writer deliberately
   writes zero to flags while the reader accepts the stored word. */
struct W8ConditionalPathHeader {
    unsigned int path_count;
    unsigned int frame_count;
    unsigned int node_count;
    unsigned int flags;
};
static_assert(sizeof(W8ConditionalPathHeader) == 0x10, "W8ConditionalPathHeader_size");

/* One pre-path prop record handed to LinkCollideableProps: the prop's path
   name plus the GDPreProp array OctPreTree.cpp builds for it (stride 0x48). */
struct W8PreProp {
    char name[0x40];
    unsigned short num_stop_meshes;
    /* The running base prop number this record's pStopMeshes indices are
       relative to; InsertConditionalNodes matches a GDProp m_prop_number
       into [first_prop_number, first_prop_number + num_stop_meshes). */
    unsigned short first_prop_number;
    GDPreProp* pStopMeshes;
};

static_assert(sizeof(W8PreProp) == 0x48, "W8PreProp_must_be_0x48");
static_assert(offsetof(W8PreProp, num_stop_meshes) == 0x40, "W8PreProp_num_stop_meshes_40");
static_assert(offsetof(W8PreProp, pStopMeshes) == 0x44, "W8PreProp_pStopMeshes");

struct W8NavigatorMovementState;

/* Retail allocates this 0x58-byte object, calls its sole observed constructor,
   and later releases it with delete. Its constructor's entire effect is
   LoadPathParameters - reading Data\Monsters\pathparms.txt into the
   path-tuning globals - so the class is the path-parameter owner. The object
   doubles as the per-step steering context StepAlongPath hands to the
   0x004CAE50-0x004CCB60 method cluster: InitializeSteeringContext
   seeds it from the movement state each step, the steering helpers accumulate
   into force, and IntegrateSteering applies the result. */
class W8PathParameters {
public:
    W8PathParameters(); /* 0x004CAE40 */

    /* Seeds the context from `movement`: the owning monster, its radius, the
       speed limit (the linked navigator's movement scale when linked), the
       normalized velocity or a yaw-derived default direction, the right-hand
       perpendicular, and cleared force/query state. */
    void InitializeSteeringContext(W8NavigatorMovementState* movement); /* 0x004CAE50 */
    /* Lazily fills nearby_locations/nearby_count with the location ids
       inside a radius-scaled box around the movement position; the cached
       result is returned on repeat calls. */
    unsigned char QueryNearbyNavigators(); /* 0x004CAFC0 */
    /* Applies force for one time step: clamps it to acceleration,
       integrates velocity toward speed_limit, resolves the heading, snaps
       the new position against the path mesh and falls back to sliding or
       stopping when the snap fails. */
    void IntegrateSteering(); /* 0x004CB090 */
    /* Advances movement->target_yaw toward movement->yaw by the shorter
       arc, accelerating or decelerating the angular velocity in
       movement->yaw_velocity. */
    void UpdateYawSteering(float time_step, bool use_turn_rate); /* 0x004CB520 */
    /* Predicts a collision with another navigator or the party inside the
       prediction window; returns nonzero when one is found ahead. */
    unsigned char PredictNavigatorCollision(); /* 0x004CB620 */
    /* Steers around geometry: brakes and deflects force perpendicular to a
       clipped span, pushes toward the next waypoint when it falls behind the
       heading, or uses directional clearance for oversized radii. */
    unsigned char HandleObstacleAhead(); /* 0x004CBB70 */
    /* Adds the seek force toward target scaled by
       g_path_acceleration_factor into force, clamped to
       acceleration; with a stopped movement it instead pushes along the
       2-D target direction and zeroes speed_limit. */
    void AccumulateSeekForce(); /* 0x004CC1A0 */
    /* Scales speed_limit down by target distance through the approach
       profile, then accumulates the seek force. */
    void SeekWithApproachSpeed(); /* 0x004CC420 */
    /* Adds a repulsion force from every linked navigator inside the combined
       radius into force. */
    void AccumulateGroupRepulsion(); /* 0x004CC4C0 */
    /* Steers around the linked leader: lateral pass targets, following
       distance, or a blocked-path fallback that reseeds target. */
    unsigned char SteerAroundLeader(bool allow_path_fallback); /* 0x004CC680 */
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
    W8NavigatorMovementState* movement;
    unsigned char padding_04[4];
    float speed_limit;
    float acceleration;
    float velocity_length;
    srVector3T<float> target;
    srVector3T<float> direction;
    srVector3T<float> perpendicular;
    srVector3T<float> force;
    float radius;
    bool nearby_queried;
    bool blocked;
    unsigned char padding_4a[2];
    unsigned int nearby_count;
    unsigned long* nearby_locations;
    W8Monster* monster;
};

static_assert(sizeof(W8PathParameters) == 0x58, "W8PathParameters_must_be_0x58");
struct W8NavigatorAttachment;

/* OctPath.cpp's two compact graph records. Surface zero and edge zero are
   sentinels; live records are addressed by their unsigned-short indices. */
struct W8PathSurface {
    unsigned short flags;
    unsigned short index;
    srVector3T<float> position;
    unsigned short parent;
    unsigned char padding_12[0x02];
    /* Monotonic visit stamp: patrol selection picks the smallest value, and
       both mover paths write elapsed game time as each waypoint is consumed. */
    unsigned int visit_stamp;
    /* A* heuristic: distance to the goal scaled by g_float_005ec394, cached by
       FindPath while the surface is open. */
    float heuristic;
    float cost;
    float remaining_cost;
    unsigned short first_edge;
    unsigned short padding_26;
};

/* The compact surface record written to a .WPT file. It retains only the
   persistent flags, first edge and world position from the live 0x28-byte
   surface. */
struct W8FileWaypoint {
    unsigned short flags;
    unsigned short first_edge;
    srVector3T<float> position;
};

#pragma pack(push, 1)
struct W8PathEdge {
    unsigned int flags;
    unsigned short source;
    unsigned short destination;
    float distance;
    unsigned short next;
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

/* The two-dimensional cell walk used by path-surface probing. The cell and
   step runs are modeled as triples like the sibling octree walker; only X/Z
   participate and the third slots are zeroed. Retail does not distinguish
   this grouping from adjacent scalar storage in the original declaration. */
struct W8PathGridWalk {
    srVector3T<int> cell; /* 0x00: destination X/Z cells; third component zero */
    srVector3T<int> step; /* 0x0c: +1 or -1 per active axis; third component zero */
    int major_axis;  /* 0x18: 0 for X, 1 for Z */
    int minor_axis0;  /* 0x1c: (major + 1) % 2 */
    int minor_axis1;  /* 0x20: unused second secondary axis, zero */
    int count;       /* 0x24: cells to visit */
    int error_delta0; /* 0x28 */
    int error0;       /* 0x2c */
    int error_reset0; /* 0x30: cell size */
    int error_delta1; /* 0x34: unused second error channel, zero */
    int error1;
    int error_reset1;
};

static_assert(sizeof(W8PathGridWalk) == 0x40, "W8PathGridWalk_must_be_0x40");
static_assert(offsetof(W8PathGridWalk, step) == 0x0c, "W8PathGridWalk_step_offset");
static_assert(offsetof(W8PathGridWalk, minor_axis1) == 0x20, "W8PathGridWalk_second_axis_offset");
static_assert(offsetof(W8PathGridWalk, error_delta1) == 0x34,
              "W8PathGridWalk_second_error_offset");
static_assert(offsetof(W8PathGridWalk, error_reset1) == 0x3c,
              "W8PathGridWalk_second_reset_offset");

/* One of the fixed probe volumes assembled by 0x004656A0. The outer radius
   is the navigator's collision radius; the inner bound is its distance from
   the movement search origin. The player entry leaves the inner bound
   untouched. 0x00465970 tests candidates against these bounds and center. */
struct W8PathProbeVolume {
    unsigned int tag;
    float outer_radius;
    float inner_radius; /* Initial distance from the search origin, not a body radius. */
    srVector3T<float> center;
};

static_assert(sizeof(W8PathProbeVolume) == 0x18, "W8PathProbeVolume_must_be_0x18");

/* The fixed 0x2c search node allocated by W8PathingService's constructor.
   Scoring at 0x00464FF0 proves the flag word, base score, current distance,
   accumulated score and world position; the remaining planner state stays
   positional until its readers are recovered. */
struct W8PathSearchNode {
    unsigned short flags;
    unsigned short node_index;
    unsigned short cell_x;
    unsigned short cell_z;
    unsigned short path_height;
    unsigned short parent_node;
    float base_score;
    float path_cost;
    float distance;
    float clearance;
    float score;
    srVector3T<float> position;
};

struct W8PathHeapEntry {
    unsigned int node;
    unsigned int priority;

    bool operator<=(const W8PathHeapEntry& other) const
    {
        return priority <= other.priority;
    }
};

typedef stHeap<W8PathHeapEntry> W8PathHeap;

struct W8PathHeapHandle {
    ~W8PathHeapHandle()
    {
        delete heap;
    }

    W8PathHeap* heap;
    unsigned int root_node;

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
    void CheckConditionalWayPtStatus(unsigned short count, unsigned short* waypoints);
    void CheckConditionalLinkStatus(unsigned short count, unsigned short* edges);
    void SetConditionalPathFrame(unsigned int path_handle, unsigned short frame);
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
    int ProcessSearchNodeProps(unsigned short node_index, bool first_only);
    unsigned int CollectPathProbes(W8NavigatorMovementState* movement, float radius);
    unsigned short PlanMovement(W8NavigatorMovementState* movement, float radius, float separation);
    unsigned short PlanMovementToPosition(W8NavigatorMovementState* movement,
                                          const srVector3T<float>* target, float radius,
                                          float separation);
    float UpdateSearchNodeScore(unsigned short node, const srVector3T<float>* position, float minimum,
                                float maximum);
    unsigned short ResolveSearchNodeCollisions(W8NavigatorMovementState* movement,
                                               unsigned short node, float radius, float separation);
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
    unsigned short FindWaypoint(const srVector3T<float>* position, bool exhaustive);
    void SnapPathHeight(srVector3T<float>* position);
    void GetPathSurfaceNormal(const srVector3T<float>* position, srVector3T<float>* normal);
    void ActivateMovementTrigger(W8NavigatorMovementState* movement, bool use_path_edge);
    void UpdatePathVisualization(const srVector3T<float>* source,
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
    unsigned char SnapWaypointPosition(srVector3T<float>* position, bool snap_to_cell);
    unsigned char TestPathCellClearance(srVector3T<float>* position, float clearance,
                                        bool snap_to_cell);
    unsigned char SnapToLowerPathCell(srVector3T<float>* position, bool allow_directional);
    unsigned char ProbeAttachmentPath(W8NavigatorAttachment* attachment);
    unsigned int FindPathCell(srVector3T<float>* position, srVector2T<unsigned int>* cell,
                              bool adjust);
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
    unsigned int ComputeWaypointNeighborMask(const srVector2i* cell, unsigned int path_value);
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
    unsigned short FindPath(W8NavigatorAttachment* attachment, unsigned int flags);
    /* Depth-first patrol search from `waypoint`: accumulates per-link path
       costs against the randomized patrol_distance target, tracking the
       argmin-key fallback nodes, and returns the reached endpoint or zero.
       Retail names it in the "Too many links" assert. */
    /* Depth-first link search from `waypoint` toward the target stored in
       m_patrol_start by LinkAttachmentTarget: collects admissible
       edge destinations (filtered like FindPath), prices each by accumulated
       link cost plus distance-to-target, sorts by that key, then returns the
       first candidate beyond m_patrol_distance or the first nonzero
       recursive result. m_probe_cell_key tracks the farthest candidate. */
    unsigned short RecurseTargetLinks(unsigned short waypoint); /* 0x004615D0 */
    unsigned short RecursePatrolLinks(unsigned short waypoint);
    float MeasureDirectionalPath(const srVector2i* cell, int direction, unsigned int height,
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
                                   bool adjust_destination, bool diagonal_steps);
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
                           const srVector3T<float>* bounds, const char* name); /* 0x00458A50 */
    unsigned char ReadPathNodes(int handle);                                  /* 0x00458CE0 */
    unsigned char WritePathNodes(unsigned int handle);
    unsigned char SaveWaypointSnapshot(bool force);
    unsigned char WriteWaypointFile();
    unsigned char ReadWaypointFile();
    void BuildWaypointFileData();
    /* Owning destructor: releases owned tables and bit sets. Single caller
       destroys the service at octree teardown. */
    ~W8PathingService(); /* 0x00457B10 */

    /* The active edge-filter mask for patrol/path searches. ConfigureForLevel
       loads it from the octree header; BuildPatrolPath stores its `flags` here
       for FindPatrolPath. */
    unsigned int path_flags0;
    int path_node_count; /* 0x04 */
    /* PrePathing's CreatePathNodeArray counts edge nodes here starting from
       one, and WriteOctFile serializes it beside the node count. */
    int edge_node_count;
    /* ReadOctFile tests this beside m_waypoint_editing before settling a portal. */
    unsigned int m_ulNumWayPoints;  /* 0x0c */
    unsigned int m_ulNumWayPtLinks; /* 0x10 */
    int m_unknown_014;
    /* Incremented for each edge removed by the waypoint editor; never read. */
    int m_removed_edge_count;
    /* The grid divisor both linking walks divide by. */
    float grid_scale; /* 0x1c */
    float span;       /* 0x20 */
    short cell_count; /* 0x24 */
    unsigned short m_padding_026;
    /* Path probe-clearance height, raw float bits from the octree
       header word; only ConfigureForLevel writes it. */
    int path_clearance; /* 0x28 */
    W8BoundingBox level_bounds; /* 0x2c: minimum/maximum pair */
    /* Four malloc'd tables and one polymorphic object, all released by
       0x00457B10 - the first four with free, the last through its own
       deleting slot. */
    W8FilePathNode* file_path_nodes; /* 0x44: serialized cell/height-state records */
    /* Surfaces are 0x28 bytes apart, edges 0xe; an edge names two surfaces by
       index in its two shorts at +4 and +6. */
    W8PathSurface* m_pSurfaces;        /* 0x48 */
    W8PathEdge* m_pEdges;              /* 0x4c */
    W8FileWaypoint* m_pFileWayPoints;      /* 0x50 */
    stModelInstance* m_pPathModelInstance; /* 0x54 */
    BitArray* m_visible_waypoints;       /* 0x58 */
    BitArray* m_rendered_waypoints;      /* 0x5c */
    BitArray* m_collected_waypoints;     /* 0x60 */
    /* Two hash indexes the loader builds and 0x00457B10 destroys. The path
       value words are bitfields (height in the low half, state flags in the
       high bits), so 0x64 takes unsigned values; 0x74 is the visited-cell set
       and keeps the signed value the octree registry also instantiates. The
       template only copies and compares values, so the two instantiations are
       body-equivalent and retail's linker folds them. */
    W8HashTable<unsigned int, unsigned int>* m_pPathValues; /* 0x64 */
    const char* level_name;                                     /* 0x68 */
    W8PathHeapHandle* path_heap;                            /* 0x6c */
    float m_path_cost_limit;                                  /* 0x70: starts 1.0e10f */
    W8HashTable<unsigned int, int>* m_pVisitedCells;        /* 0x74 */
    unsigned int m_probe_cell_key;                            /* 0x78 */
    srVector3T<float> m_probe_position;                       /* 0x7c */
    unsigned int m_probe_limit;                               /* 0x88 */
    bool m_probe_bounded;                                     /* 0x8c */
    unsigned char m_padding_08d[3];
    unsigned int planner_location;
    unsigned int m_path_candidate_count;
    unsigned long* m_path_candidates;
    bool explicit_target; /* 0x9c */
    unsigned char m_padding_09d[3];
    unsigned int m_waypoint_neighbor_mask; /* 0xa0 */
    bool m_trace_configured;               /* 0xa4 */
    unsigned char m_padding_0a5[3];
    float m_trace_max_distance;
    srVector3T<float> trace_offset;
    int m_trace_mode;
    float trace_height_offset;
    int m_trace_target_location;
    float m_trace_target_yaw;
    W8PathSearchNode* m_search_nodes; /* 0xc8 */
    unsigned int m_search_node_count;
    unsigned int m_search_node_capacity;
    unsigned int m_path_probe_count;
    W8PathProbeVolume m_path_probes[10];
    bool m_waypoint_editing; /* 0x1c8 */
    bool flag1;
    bool flag2;
    bool search_visualization;
    bool waypoints_dirty;
    unsigned char m_padding_1cd;
    unsigned short path_flags1; /* 0x1ce: starts 4 */
    int link_flags;
    unsigned short start_waypoint;
    unsigned short destination_waypoint;
    unsigned short saved_surface;
    bool path_direction_valid;
    unsigned char m_padding_1db;
    /* Patrol-search state laid down by BuildPatrolPath and consulted by the
       recursive FindPatrolPath: the argmin-key candidate node, the accepted
       min/max start-to-destination range, the randomized target path cost,
       the start and destination positions, and the best alternate
       candidate's cost. */
    unsigned int m_patrol_node;
    float m_patrol_min;
    float m_patrol_max;
    float m_patrol_distance;
    srVector3T<float> m_patrol_start;
    srVector3T<float> m_patrol_destination;
    unsigned char m_padding_204[0x0c];
    float m_patrol_cost;
    W8PathParameters* m_path_parameters; /* 0x214 */
    W8NavigatorAttachment* m_linked_attachment;
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
    bool span_blocked;
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

enum { W8_PREPATH_NODES_PER_CHUNK = 1000 };

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
    /* Hands out the next path-node record, allocating a new chunk when the
       current one fills. */
    W8PrePathNode* GetPathNode();
    unsigned char BuildPathList(W8PrePathNode* nodes, W8HashTable<unsigned int, int>* cell_map);
    unsigned char LinkPathNodes();
    void PropagatePathNodeClearance(W8PrePathNode* node, unsigned int depth);
    unsigned int DeleteUnreachableAreas();
    int CreatePathNodeArray();
    unsigned char CreateAutomapNodes(W8LevelFile* level);

    W8PrePathNode** path_node_list; /* path_node_count entries */
    OctPrePathLog* path_log;
    /* A malloc'd buffer the destructor `free`s; no surviving writer. */
    void* owned_248;
    int named_position_count;
    srVector3T<float>* named_positions;
    W8HashTable<unsigned int, int>* cell_map;
    /* Embedded chunk table: each slot is a malloc'd run of
       W8_PREPATH_NODES_PER_CHUNK path-node records. The constructor fills slot 0, and the
       destructor frees every slot through chunk_index inclusive. */
    W8PrePathNode* node_chunks[0x3e8];
    int chunk_index;
    int chunk_node_count;
    /* Components smaller than this percent of the node count get deleted
       while linking; capped at 50. */
    unsigned int min_component_percent;
};

static_assert(sizeof(PrePathing) == 0x1204, "PrePathing_must_be_0x1204");
static_assert(offsetof(PrePathing, path_node_list) == 0x240, "PrePathing_path_node_list_240");
static_assert(offsetof(PrePathing, node_chunks) == 0x258, "PrePathing_node_chunks_258");
static_assert(offsetof(PrePathing, chunk_index) == 0x11f8, "PrePathing_chunk_index_11f8");
static_assert(offsetof(PrePathing, min_component_percent) == 0x1200,
              "PrePathing_min_component_percent_1200");

/* Move an integer path cell one compass step; directions outside the
   eight-value range wrap once. */
void __stdcall StepPathCell(int* x, int* z, int direction);

extern W8PathingService* g_pathing;
extern unsigned short g_path_reserve;
extern const float g_path_span_scale;
extern const double g_double_005ec3b0;
/* The -1.0 no-route sentinel MeasurePathDistance returns. */
extern const double g_double_005ec2e8;
/* 0x005ED300: OctPrePath.cpp's vertical-link slack; retail Combat.cpp reads
   it directly when sizing a monster's move. */
extern const float g_prepath_link_height;
