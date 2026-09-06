#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/stHeap.hpp"
#include "wiz8/engine_code/stHash.hpp"

class stModelInstance005EC7D0;

/* Retail allocates this 0x58-byte object, calls its sole observed constructor,
   and later releases it with delete. Its storage has no proven
   semantic fields. */
class W8PathState004CAE40 {
public:
    W8PathState004CAE40();               /* 0x004CAE40 */

private:
    unsigned char positional_00[0x58];
};

static_assert(sizeof(W8PathState004CAE40) == 0x58,
              "W8PathState004CAE40_must_be_0x58");
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
    ~W8PathHeapHandle() { delete heap_00; }

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
    unsigned int FindPathCell00459D60(
        srVector3T<float>* position, unsigned int* cell, unsigned char adjust);
    unsigned char BuildAttachmentPath00460950(
        W8NavigatorAttachment* attachment, unsigned int flags);
    unsigned char PrepareLinkedNavigator00466FB0(W8NavigatorMovementState* movement);
    unsigned char LinkAttachmentTarget004612A0(
        W8NavigatorAttachment* attachment, unsigned int flags,
        const srVector3T<float>* target, float separation);
    unsigned char BuildPatrolPath00461960(
        W8NavigatorAttachment* attachment, unsigned int flags,
        const srVector3T<float>* target, float minimum,
        const srVector3T<float>* velocity, float maximum);
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
    /* Owning destructor: releases owned tables and bit sets. Single caller
       destroys the service at octree teardown. */
    ~W8PathingService();                  /* 0x00457B10 */

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
    /* Two hash indexes the loader builds and 0x00457B10 destroys. */
    W8HashTable<unsigned int, int>* m_pIndex_064; /* 0x64 */
    const char* level_name;                /* 0x68 */
    W8PathHeapHandle* path_heap_06c;     /* 0x6c */
    unsigned int m_positional_070;       /* 0x70: starts 0x501502f9 */
    W8HashTable<unsigned int, int>* m_pIndex_074; /* 0x74 */
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
    W8PathState004CAE40* path_state_214; /* 0x214 */
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

extern W8PathingService* g_pathing_00659c60;
