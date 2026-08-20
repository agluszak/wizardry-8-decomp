#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"

#include "surrender/srNode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

extern unsigned char g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
extern W8PathingService* g_pathing_00659c60;
extern W8Navigator* g_startup_world_659c0c;
extern unsigned char g_flag_00659c5c;
extern void ConstructPathState004CCCB0(void* state);
extern void* CreateOctPathIndex();
extern void* g_path_scratch_00659c64;
extern void RegisterPathSurface004B7730(unsigned int index, const int* point);
extern void RegisterPathVertex004B7830(
    unsigned int index, const int* point, const int* second);
extern const double g_path_waypoint_snap_distance_005ec150;
extern double g_double_005ec3a8;
extern double g_double_005ec3b0;
extern float g_path_direction_threshold_0_005ec348;
extern float g_path_direction_threshold_1_005ec34c;
extern float g_path_direction_threshold_2_005ec350;
extern float g_path_direction_threshold_3_005ec354;
extern float g_path_cardinal_scale_005ec358;
extern float g_path_waypoint_query_vertical_005ec35c;
extern float g_path_waypoint_query_horizontal_005ec360;
extern double g_path_waypoint_exact_distance_005ebc64;
extern float g_float_005ebb34;
extern double g_double_005ebe80;
extern void Function58AAD0(int channel, const char* format, ...);

#define OCTPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPath.cpp"

/* Read the path graph out of the octree file.

   Two parts. The hash array pairs a key with a value for every node the service
   was sized for and goes straight into the first index. Then a four-dword block
   gives the three conditional-table counts and one loose value, and a graph with
   fewer than two lookup or key entries is treated as having none at all rather
   than allocated. Every conditional table's allocation failure asserts against
   m_pCondPaths rather than against itself, which is the original's own
   shorthand and is reproduced. */
// FUNCTION: WIZ8 0x00458ce0
unsigned char W8PathingService::Load00458CE0(int handle)
{
    char acMessage[256];
    unsigned int block[4];
    unsigned int uiRead;
    unsigned int* buffer;
    unsigned int* scan;
    unsigned char fSuccess = 0;
    unsigned int index;

    if (size_004 != 0) {
        m_pIndex_064 = CreateOctPathIndex();
        m_pIndex_074 = CreateOctPathIndex();
        buffer = static_cast<unsigned int*>(malloc(size_004 * 8));
        if (m_pIndex_064 == 0 || buffer == 0) {
            strcpy(acMessage, "ReadPathNodes: Couldn't allocate path hash array.");
        } else {
            fSuccess = ReadVirtualFile(handle, buffer, size_004 * 8, &uiRead);
            if (fSuccess == 0) {
                strcpy(acMessage, "ReadPathNodes: Couldn't read path hash array.");
                free(buffer);
            } else {
                scan = buffer;
                for (index = 0; index < (unsigned int)size_004; ++index) {
                    InsertEntry0055DBB0(
                        static_cast<W8OctreeIndex*>(m_pIndex_064),
                        &scan[0], reinterpret_cast<const int*>(&scan[1]));
                    scan += 2;
                }
                free(buffer);
            }
        }
    }
    fSuccess = ReadVirtualFile(handle, block, 0x10, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8fa,
                     "ReadPathNodes: Couldn't write Conditional Counts.\n");
    }
    m_ulNumCondPaths = block[0];
    m_ulNumCondLookup = block[1];
    m_ulNumCondKeys = block[2];
    m_positional_000 = block[3];
    if (m_ulNumCondLookup < 2 || m_ulNumCondKeys < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondLookup = 0;
        m_ulNumCondKeys = 0;
        return fSuccess;
    }
    m_pCondPaths = static_cast<unsigned char*>(malloc(block[0] * 0x44));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x903,
                     "ReadPathNodes: Couldn't allocate Conditional Prop array.\n");
    }
    m_pCondLookup = static_cast<int*>(malloc(m_ulNumCondLookup << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x905,
                     "ReadPathNodes: Couldn't allocate Conditional Lookup array.\n");
    }
    m_pCondFrames = static_cast<short*>(malloc(m_ulNumCondLookup << 1));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x907,
                     "ReadPathNodes: Couldn't allocate Conditional Frame array.\n");
    }
    m_pCondKeys = static_cast<int*>(malloc(m_ulNumCondKeys << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x909,
                     "ReadPathNodes: Couldn't allocate Conditional Key array.\n");
    }
    m_pCondValues = static_cast<int*>(malloc(m_ulNumCondKeys << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x90b,
                     "ReadPathNodes: Couldn't allocate Conditional Value array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondPaths, m_ulNumCondPaths * 0x44, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x90e,
                     "ReadPathNodes: Couldn't write Conditional Prop array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondLookup, m_ulNumCondLookup << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x910,
                     "ReadPathNodes: Couldn't write Conditional Lookup array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondFrames, m_ulNumCondLookup << 1, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x912,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondKeys, m_ulNumCondKeys << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x914,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = ReadVirtualFile(
        handle, m_pCondValues, m_ulNumCondKeys << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x916,
                     "ReadPathNodes: Couldn't write Conditional Value array.\n");
    }
    return fSuccess;
}

/* Offer every flagged surface to the path builder.

   Surfaces are 0x28 bytes apart and the walk starts at index one, so entry zero
   is never a real surface. The point handed on is the surface's own position
   converted to the graph's integer grid - x from the bounds floor, z from the
   third bound - and the two conversions happen in the order the point's fields
   do not. */
// FUNCTION: WIZ8 0x00460020
void W8PathingService::LinkSurfaces00460020()
{
    unsigned int index = 1;
    int offset = 0x28;
    int point[2];
    unsigned char* surface;
    int converted;

    if (m_ulNumSurfaces <= index) {
        return;
    }
    do {
        surface = reinterpret_cast<unsigned char*>(m_pSurfaces_048) + offset;
        if ((*surface & 0x40) != 0) {
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            point[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                             grid_scale_01c);
            point[1] = converted;
            RegisterPathSurface004B7730(index, point);
        }
        offset += 0x28;
        ++index;
    } while (index < m_ulNumSurfaces);
}

/* The same for the edges, which are 0xe bytes apart, gated by a different flag,
   and which name two surfaces by index in their shorts at +4 and +6. Each of
   those surfaces contributes one converted point, so the builder receives the
   edge as a pair. */
// FUNCTION: WIZ8 0x004600b0
void W8PathingService::LinkEdges004600B0()
{
    unsigned int index = 1;
    int offset = 0xe;
    int first[2];
    int second[2];
    unsigned char* edge;
    unsigned char* surface;
    unsigned int surface_index;
    int converted;

    if (m_ulNumEdges <= index) {
        return;
    }
    do {
        edge = reinterpret_cast<unsigned char*>(m_pEdges_04c) + offset;
        if ((*reinterpret_cast<unsigned int*>(edge) & 0x20000000) != 0) {
            surface_index = *reinterpret_cast<unsigned short*>(edge + 4);
            surface = reinterpret_cast<unsigned char*>(m_pSurfaces_048) + surface_index * 0x28;
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            first[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                             grid_scale_01c);
            first[1] = converted;

            surface_index = *reinterpret_cast<unsigned short*>(edge + 6);
            surface = reinterpret_cast<unsigned char*>(m_pSurfaces_048) + surface_index * 0x28;
            converted = (int)((*reinterpret_cast<float*>(surface + 0xc) - level_bounds[2]) /
                              grid_scale_01c);
            second[0] = (int)((*reinterpret_cast<float*>(surface + 4) - level_bounds[0]) /
                              grid_scale_01c);
            second[1] = converted;
            RegisterPathVertex004B7830(index, first, second);
        }
        offset += 0xe;
        ++index;
    } while (index < m_ulNumEdges);
}

/* Refresh the disabled bit for a conditional list of waypoints. */
// FUNCTION: WIZ8 0x004601b0
void W8PathingService::CheckConditionalWaypointStatus004601B0(
    unsigned short count,
    unsigned short* waypoints)
{
    while (count != 0) {
        unsigned short waypoint = *waypoints;
        if (waypoint >= (unsigned short)m_ulNumSurfaces) {
            srAssertFail(
                "pusWayPts[i] < (UINT16)m_ulNumWayPoints",
                OCTPATH_CPP,
                0x1a4e,
                "Pathing::CheckConditionalWayPtStatus: WayPt Index out of range.");
        }

        W8PathSurface* surface = &m_pSurfaces_048[waypoint];
        if ((ClassifyWaypoint00459C00(&surface->position_04) & 0x10000000) == 0) {
            surface->flags_00 &= 0xffdf;
        }
        else {
            surface->flags_00 |= 0x20;
        }
        ++waypoints;
        --count;
    }
}

/* Refresh the disabled bit for conditional path edges. Only edges carrying
   the conditional-span flag participate. A disabled source always disables
   its edge; otherwise the current span test decides the bit. */
// FUNCTION: WIZ8 0x00460250
void W8PathingService::CheckConditionalLinkStatus00460250(
    unsigned short count,
    unsigned short* edges)
{
    while (count != 0) {
        unsigned short edge_index = *edges;
        if (edge_index >= (unsigned short)m_ulNumEdges) {
            srAssertFail(
                "pusLinks[i] < (UINT16)m_ulNumWayPtLinks",
                OCTPATH_CPP,
                0x1a6c,
                "Pathing::CheckConditionalLinkStatus: Link Index out of range.");
        }

        W8PathEdge* edge = &m_pEdges_04c[edge_index];
        if ((edge->flags_00 & 0x20000000) != 0) {
            if ((m_pSurfaces_048[edge->source_04].flags_00 & 0x20) != 0 ||
                TestWaypointSpan0045A1B0(
                    &m_pSurfaces_048[edge->source_04].position_04,
                    &m_pSurfaces_048[edge->destination_06].position_04,
                    0,
                    0) == 0) {
                edge->flags_00 |= 0x80000000;
            }
            else {
                edge->flags_00 &= 0x7fffffff;
            }
        }
        ++edges;
        --count;
    }
}

/* Resolve the navigator attachment's current directed edge and apply the
   transition encoded by its flags.

   Disabled ordinary edges stop movement. Teleportal edges consume the current
   pair, move both live and attachment positions to its destination, and toggle
   the attachment's transition mode. */
// FUNCTION: WIZ8 0x00460350
unsigned char W8PathingService::HandlePathEdgeTransition00460350(
    W8NavigatorMovementState* movement)
{
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    unsigned short cursor = attachment->value_04;
    unsigned int flags = 0;
    unsigned short destination;

    if (cursor < attachment->value_08) {
        unsigned short* pairs =
            static_cast<unsigned short*>(attachment->allocation_50);
        unsigned short source = pairs[cursor];
        destination = pairs[cursor + 1];
        unsigned short edge_index = m_pSurfaces_048[source].first_edge_24;

        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            if (edge->destination_06 == destination) {
                flags = edge->flags_00;
                break;
            }
            edge_index = edge->next_0c;
        }
    }

    if ((flags & 0x80000000) != 0 &&
        ((flags & 0x10000000) == 0 ||
         (movement->unknown_000 & 0x10000000) == 0)) {
        return 0;
    }
    if ((flags & 0x01000000) == 0) {
        return 1;
    }

    attachment->value_04 += 2;
    movement->position_040 = m_pSurfaces_048[destination].position_04;
    attachment->position_34 = m_pSurfaces_048[destination].position_04;
    if ((attachment->flags_00 & 0x01000000) == 0) {
        attachment->flags_00 |= 0x01000000;
    }
    else {
        attachment->flags_00 &= 0xfeffffff;
    }
    return 2;
}

/* Reduce both accumulated costs for one accepted waypoint and continue down
   the selected parent tree. A child participates only while it remains in the
   active waypoint bit set and names the current waypoint as its parent. The
   cost reaching zero is the retail recursion boundary. */
// FUNCTION: WIZ8 0x00462220
void W8PathingService::ReduceWaypointCosts00462220(
    unsigned int waypoint,
    float amount)
{
    W8PathSurface* surface = &m_pSurfaces_048[waypoint];
    surface->cost_1c -= amount;
    if (surface->cost_1c >= g_float_005ebb34) {
        surface->remaining_cost_20 -= amount;

        unsigned short edge_index = surface->first_edge_24;
        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short child = edge->destination_06;
            if (child != 0 && m_owned_058->Test(child) != 0 &&
                m_pSurfaces_048[child].parent_10 == waypoint) {
                ReduceWaypointCosts00462220(child, amount);
            }
            edge_index = edge->next_0c;
        }
    }
}

/* Move an integer path cell one compass step. Directions immediately outside
   the eight-value range wrap once; values still outside it leave the cell
   untouched. The jump-table order is north through north-west. */
// FUNCTION: WIZ8 0x004622d0
void __stdcall StepPathCell004622D0(int* x, int* z, int direction)
{
    if (direction < 0) {
        direction += 8;
    }
    else if (direction > 7) {
        direction -= 8;
    }

    switch (direction) {
    case 0:
        ++*z;
        break;
    case 1:
        ++*x;
        ++*z;
        break;
    case 2:
        ++*x;
        break;
    case 3:
        ++*x;
        --*z;
        break;
    case 4:
        --*z;
        break;
    case 5:
        --*x;
        --*z;
        break;
    case 6:
        --*x;
        break;
    case 7:
        --*x;
        ++*z;
        break;
    }
}

/* Advance the attachment's probe cursor and accept its next stored waypoint
   only when the live path grid permits the span from the supplied position.
   Flag 0x80000 makes the first probe repeat the current path index. */
// FUNCTION: WIZ8 0x00462de0
unsigned char W8PathingService::AdvanceAttachmentWaypoint00462DE0(
    const srVector3T<float>* source,
    W8NavigatorAttachment* attachment)
{
    unsigned short cursor = attachment->value_04;
    if ((attachment->flags_00 & 0x00080000) != 0) {
        --cursor;
    }
    attachment->unknown_06 = cursor;
    ++attachment->unknown_06;

    if (attachment->unknown_06 < attachment->value_08) {
        srVector3T<float> destination =
            attachment->position_4c[attachment->unknown_06];
        if (TestWaypointSpan0045A1B0(source, &destination, 0, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Match a tag against the path probes collected for the current search. A
   tag-only query succeeds immediately. A spatial query must lie strictly
   outside the probe's inner radius and strictly inside its outer radius plus
   the caller's own radius. */
// FUNCTION: WIZ8 0x00465970
unsigned char W8PathingService::MatchesPathProbe00465970(
    unsigned int tag,
    const float* radius,
    const srVector3T<float>* position)
{
    for (unsigned int index = 0; index < path_probe_count_0d4; ++index) {
        W8PathProbeVolume* probe = &path_probes_0d8[index];
        if (probe->tag_00 == tag) {
            if (radius == 0) {
                return 1;
            }

            float delta_x = probe->center_0c.x - position->x;
            float delta_y = probe->center_0c.y - position->y;
            float delta_z = probe->center_0c.z - position->z;
            float distance = (float)sqrt(
                delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
            if (distance < probe->outer_radius_04 + *radius &&
                probe->inner_radius_08 < distance) {
                return 1;
            }
        }
    }
    return 0;
}

/* Collect the player and nearby active monsters whose collision volumes can
   overlap the movement search radius. The octree supplies location tags; the
   monster manager remains authoritative for resolving each live navigator.
   Retail caps the resulting probe table at five entries even though its fixed
   storage has room for ten. */
// FUNCTION: WIZ8 0x004656a0
unsigned int W8PathingService::CollectPathProbes004656A0(
    W8NavigatorMovementState* movement,
    float radius)
{
    path_probe_count_0d4 = 0;

    float extent = g_path_limit_006081e8 + radius;
    srVector3T<float> lower;
    srVector3T<float> upper;
    lower.x = movement->position_040.x - extent;
    lower.y = movement->position_040.y - extent;
    lower.z = movement->position_040.z - extent;
    upper.x = movement->position_040.x + extent;
    upper.y = movement->position_040.y + extent;
    upper.z = movement->position_040.z + extent;

    path_candidates_098 = 0;
    path_candidate_count_094 = OctreeTraverseKind12(
        &path_candidates_098, &lower, &upper, movement->location_id_004);

    srVector3T<float> player_position =
        g_startup_world_659c0c->GetPosition();
    float delta_x = player_position.x - movement->position_040.x;
    float delta_y = player_position.y - movement->position_040.y;
    float delta_z = player_position.z - movement->position_040.z;
    float distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
    float player_radius = g_startup_world_659c0c->
        fields.movement_0c0.alternate_radius_0b4;
    float overlap_radius = radius;
    if (player_radius < radius) {
        overlap_radius = player_radius;
    }
    if (distance < (radius - overlap_radius * g_float_005ebc7c) +
                       player_radius) {
        W8PathProbeVolume* probe = &path_probes_0d8[path_probe_count_0d4];
        probe->tag_00 = 0;
        probe->outer_radius_04 = player_radius;
        probe->center_0c = g_startup_world_659c0c->GetPosition();
        ++path_probe_count_0d4;
    }

    for (unsigned int index = 0;
         index < path_candidate_count_094 && path_probe_count_0d4 < 5;
         ++index) {
        int location_id = path_candidates_098[index];
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x26ae, OCTPATH_CPP, location_id, 0);
        if (monster_index != (unsigned int)-1) {
            monster_index = MonsterGetIndexByLocationID(
                0x26b1, OCTPATH_CPP, location_id, 1);
            W8MonsterInfo* info =
                MonsterGetScriptPartByLocationIndex(monster_index);
            if (info != 0 && info->monster != 0 &&
                info->monster->fields.state_088 != 0) {
                W8Monster* monster = info->monster;
                srVector3T<float> monster_position = monster->GetPosition();
                delta_x = monster_position.x - movement->position_040.x;
                delta_y = monster_position.y - movement->position_040.y;
                delta_z = monster_position.z - movement->position_040.z;
                distance = (float)sqrt(
                    delta_x * delta_x + delta_y * delta_y +
                    delta_z * delta_z);
                float monster_radius = monster->
                    fields.movement_0c0.alternate_radius_0b4;
                overlap_radius = radius;
                if (monster_radius < radius) {
                    overlap_radius = monster_radius;
                }
                if (distance <
                    (radius - overlap_radius * g_float_005ebc7c) +
                        monster_radius) {
                    W8PathProbeVolume* probe =
                        &path_probes_0d8[path_probe_count_0d4];
                    probe->tag_00 = location_id;
                    probe->outer_radius_04 = monster_radius;
                    probe->inner_radius_08 = distance;
                    probe->center_0c = monster->GetPosition();
                    ++path_probe_count_0d4;
                }
            }
        }
    }
    return path_probe_count_0d4;
}

/* Plan with an explicit target position. The service flag suppresses the core
   planner's ordinary post-search callback for exactly this nested call, while
   the planner's status is passed straight back to the navigator caller. */
// FUNCTION: WIZ8 0x00464ab0
unsigned short W8PathingService::PlanMovementToPosition00464AB0(
    W8NavigatorMovementState* movement,
    const srVector3T<float>* target,
    float radius,
    float separation)
{
    flag_09c = 1;
    movement->target_position_04c = *target;
    unsigned short result =
        PlanMovement00463460(movement, radius, separation);
    flag_09c = 0;
    return result;
}

/* Refresh one planner node's distance and accumulated score. Explicit-target
   mode scores from the shared ceiling; ordinary mode starts from the node's
   base score and adds a range penalty only when the adjusted gap is positive.
   Flag 0x2000 applies the final fixed penalty in either mode. */
// FUNCTION: WIZ8 0x00464ff0
float W8PathingService::UpdateSearchNodeScore00464FF0(
    unsigned int node_index,
    const srVector3T<float>* position,
    float minimum,
    float maximum)
{
    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    float delta_x = position->x - node->position_20.x;
    float delta_y = position->y - node->position_20.y;
    float delta_z = position->z - node->position_20.z;
    float distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
    node->distance_14 = distance;

    if (flag_09c == 0) {
        node->score_1c = distance * g_float_005ec3b8 + node->base_score_0c;
    }
    else {
        node->score_1c = g_float_005ec3c0 - distance;
    }

    float gap = maximum - minimum;
    if (flag_09c == 0) {
        float adjusted_gap = gap;
        if (distance <= gap) {
            adjusted_gap = (gap - distance) * g_float_005ec390;
        }
        if (adjusted_gap > g_float_005ebb34) {
            node->score_1c += gap * g_float_005ec3bc;
        }
    }
    else if (gap > g_float_005ebb34) {
        node->score_1c += g_float_005ec3c0;
    }

    if ((node->flags_00 & 0x2000) != 0) {
        node->score_1c += g_float_005ec3c0;
    }
    return node->score_1c;
}

/* Resolve dynamic navigator overlap for one candidate search node.

   The player is tag zero; a target navigator receives the caller's separation
   allowance, while every other live monster uses only the two radii. Probe
   volumes can mark the node as hard-blocked before the live object lookup.
   Shallow overlaps move the node outward and set flag 0x200; deeper or
   directionally conflicting overlaps return the retail collision state. */
// FUNCTION: WIZ8 0x00465130
unsigned short W8PathingService::ResolveSearchNodeCollisions00465130(
    W8NavigatorMovementState* movement,
    unsigned int node_index,
    float radius,
    float separation)
{
    unsigned short result = 0;
    if (flag_09c != 0) {
        separation = 0.0f;
    }

    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    srVector3T<float> blocking_direction;
    srVector3T<float> player_position = g_startup_world_659c0c->GetPosition();
    float delta_x = player_position.x - node->position_20.x;
    float delta_y = player_position.y - node->position_20.y;
    float delta_z = player_position.z - node->position_20.z;
    float distance_squared =
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z;
    float threshold = g_startup_world_659c0c->
        fields.movement_0c0.alternate_radius_0b4 + radius;
    if (movement->value_010 == 0) {
        threshold += separation;
    }

    if ((float)sqrt(distance_squared) < threshold) {
        if (movement->value_010 != 0 || flag_09c != 0) {
            return 1;
        }
        blocking_direction.x = delta_x;
        blocking_direction.y = delta_y;
        blocking_direction.z = delta_z;
        if ((double)distance_squared != g_zero_005ebb40) {
            float scale = (float)(g_double_005ebc30 / sqrt(distance_squared));
            blocking_direction.x *= scale;
            blocking_direction.y *= scale;
            blocking_direction.z *= scale;
        }
        result = 3;
    }

    for (unsigned int candidate = 0;
         candidate < path_candidate_count_094;
         ++candidate) {
        int location_id = path_candidates_098[candidate];
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x2622, OCTPATH_CPP, location_id, 0);
        if (monster_index == (unsigned int)-1) {
            continue;
        }

        unsigned int probe_index;
        for (probe_index = 0; probe_index < path_probe_count_0d4;
             ++probe_index) {
            W8PathProbeVolume* probe = &path_probes_0d8[probe_index];
            if (probe->tag_00 == (unsigned int)location_id) {
                float probe_x = probe->center_0c.x - node->position_20.x;
                float probe_y = probe->center_0c.y - node->position_20.y;
                float probe_z = probe->center_0c.z - node->position_20.z;
                float probe_distance = (float)sqrt(
                    probe_x * probe_x + probe_y * probe_y + probe_z * probe_z);
                if (probe_distance < radius + probe->outer_radius_04 &&
                    probe->inner_radius_08 < probe_distance) {
                    node->flags_00 |= 0x2000;
                    break;
                }
            }
        }
        if (probe_index < path_probe_count_0d4) {
            continue;
        }

        monster_index = MonsterGetIndexByLocationID(
            0x262b, OCTPATH_CPP, location_id, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info == 0 || info->monster == 0 ||
            info->monster->fields.state_088 == 0) {
            continue;
        }

        W8Monster* monster = info->monster;
        srVector3T<float> monster_position = monster->GetPosition();
        delta_x = node->position_20.x - monster_position.x;
        delta_y = node->position_20.y - monster_position.y;
        delta_z = node->position_20.z - monster_position.z;
        distance_squared =
            delta_x * delta_x + delta_y * delta_y + delta_z * delta_z;
        float distance = (float)sqrt(distance_squared);
        threshold = monster->fields.movement_0c0.alternate_radius_0b4 + radius;
        if (location_id == movement->value_010 && flag_09c == 0) {
            threshold += separation;
        }

        if (distance < threshold) {
            if (location_id == movement->value_010 && flag_09c == 0) {
                blocking_direction.x = delta_x;
                blocking_direction.y = delta_y;
                blocking_direction.z = delta_z;
                if ((double)distance_squared != g_zero_005ebb40) {
                    float scale =
                        (float)(g_double_005ebc30 / sqrt(distance_squared));
                    blocking_direction.x *= scale;
                    blocking_direction.y *= scale;
                    blocking_direction.z *= scale;
                }
                result = 3;
                continue;
            }

            unsigned char adjust = 0;
            if ((node->flags_00 & 0x0200) == 0 &&
                threshold <= distance + g_float_005ec020) {
                if (result == 3) {
                    srVector3T<float> direction;
                    direction.x = delta_x;
                    direction.y = delta_y;
                    direction.z = delta_z;
                    if ((double)distance_squared != g_zero_005ebb40) {
                        float scale = (float)(
                            g_double_005ebc30 / sqrt(distance_squared));
                        direction.x *= scale;
                        direction.y *= scale;
                        direction.z *= scale;
                    }
                    float dot =
                        direction.x * blocking_direction.x +
                        direction.y * blocking_direction.y +
                        direction.z * blocking_direction.z;
                    if (dot <= g_float_005ec3d0 ||
                        g_float_005ec3c8 <= dot) {
                        adjust = 1;
                    }
                }
                else if (result != 1) {
                    adjust = 1;
                }
            }

            if (adjust != 0) {
                if ((double)distance_squared != g_zero_005ebb40) {
                    float scale =
                        (threshold - distance) / (float)sqrt(distance_squared);
                    delta_x *= scale;
                    delta_y *= scale;
                    delta_z *= scale;
                }
                node->position_20.x += delta_x;
                node->position_20.y += delta_y;
                node->position_20.z += delta_z;
                node->flags_00 |= 0x0200;
                if (result != 1) {
                    continue;
                }
            }

            if (g_flag_00659c5c == 0) {
                return 1;
            }
            result = 2;
        }
    }
    return result;
}

/* Give everything the service owns back.

   Not a destructor: nothing restores a vtable and the object is left holding
   dangling pointers, which is the same shape BitArray::FreeIndex has. The four
   malloc'd tables and the conditional path tables go back through free, the bit
   sets and the two hash indexes through their own teardown, and the global slot
   the constructor claimed is cleared last. */
// FUNCTION: WIZ8 0x00457b10
void W8PathingService::Release00457B10()
{
    void** index;

    if (m_owned_044 != 0) {
        free(m_owned_044);
    }
    if (m_pSurfaces_048 != 0) {
        free(m_pSurfaces_048);
    }
    if (m_pEdges_04c != 0) {
        free(m_pEdges_04c);
    }
    if (m_owned_050 != 0) {
        free(m_owned_050);
    }
    if (m_owned_054 != 0) {
        delete m_owned_054;
    }
    if (m_owned_058 != 0) {
        m_owned_058->FreeIndex();
        ::operator delete(m_owned_058);
    }
    if (m_owned_05c != 0) {
        m_owned_05c->FreeIndex();
        ::operator delete(m_owned_05c);
    }
    if (m_owned_060 != 0) {
        m_owned_060->FreeIndex();
        ::operator delete(m_owned_060);
    }
    index = static_cast<void**>(m_pIndex_064);
    if (index != 0) {
        if (index[0] != 0) {
            ::operator delete(index[0]);
        }
        if (index[1] != 0) {
            ::operator delete(index[1]);
        }
        ::operator delete(index);
    }
    index = static_cast<void**>(m_pIndex_074);
    if (index != 0) {
        if (index[0] != 0) {
            ::operator delete(index[0]);
        }
        if (index[1] != 0) {
            ::operator delete(index[1]);
        }
        ::operator delete(index);
    }
    index = static_cast<void**>(m_owned_06c);
    if (index != 0) {
        void** held = static_cast<void**>(index[0]);

        if (held != 0) {
            if (held[1] == 0) {
                ::operator delete(held[0]);
            }
            ::operator delete(held);
        }
        ::operator delete(index);
    }
    if (m_owned_0c8 != 0) {
        ::operator delete(m_owned_0c8);
    }
    if (m_owned_214 != 0) {
        ::operator delete(m_owned_214);
    }
    if (g_path_scratch_00659c64 != 0) {
        free(g_path_scratch_00659c64);
    }
    g_path_scratch_00659c64 = 0;
    if (m_pCondPaths != 0) {
        free(m_pCondPaths);
    }
    if (m_pCondLookup != 0) {
        free(m_pCondLookup);
    }
    if (m_pCondFrames != 0) {
        free(m_pCondFrames);
    }
    if (m_pCondKeys != 0) {
        free(m_pCondKeys);
    }
    if (m_pCondValues != 0) {
        free(m_pCondValues);
    }
    g_pathing_00659c60 = 0;
}

/* Build the pathing service.

   Everything starts cleared except three hundred-bit sets, a reserve table
   sized from the shared bound, and one state object. The service registers
   itself in the global slot as it is built, which is what lets the rest of the
   engine reach it without the octree handing it over. */
// FUNCTION: WIZ8 0x004578e0
W8PathingService::W8PathingService()
{
    int index;

    grid_scale_01c = 0;
    span_020 = 0;
    for (index = 0; index < 6; ++index) {
        level_bounds[index] = 0;
    }
    m_owned_044 = 0;
    size_004 = 0;
    m_positional_008 = 0;
    m_ulNumSurfaces = 0;
    m_ulNumEdges = 0;
    m_positional_014 = 0;
    m_positional_018 = 0;
    m_pSurfaces_048 = 0;
    m_pEdges_04c = 0;
    m_owned_050 = 0;
    m_owned_054 = 0;
    m_owned_058 = new BitArray(100);
    m_owned_05c = new BitArray(100);
    m_owned_060 = new BitArray(100);
    m_pIndex_064 = 0;
    m_pIndex_074 = 0;
    level_name = 0;
    flag_08c = 0;
    m_owned_06c = 0;
    m_positional_070 = 0x501502f9;
    flag_1c8 = 0;
    flag_1c9 = 0;
    flag_1ca = 0;
    flag_1cb = 0;
    flag_1cc = 0;
    value_1ce = 4;
    m_positional_1d0 = 0;
    value_1d4 = 0;
    value_1d6 = 0;
    value_1d8 = 0;
    m_owned_0c8 = static_cast<W8PathSearchNode*>(
        ::operator new((g_path_reserve_0060827a + 0x14) * 0x2c));
    m_positional_0cc = 0;
    m_positional_0d0 = 0;
    flag_09c = 0;
    flag_0a4 = 0;
    m_positional_0c0 = 0;
    m_positional_0a8 = 0;
    m_positional_0ac = 0;
    m_positional_0b0 = 0;
    m_positional_0b4 = 0;
    m_positional_0b8 = 0;
    m_positional_0bc = 0;
    m_positional_0c4 = 0;
    m_owned_214 = new W8PathState004CAE40();
    m_positional_218 = 0;
    m_pCondPaths = 0;
    m_ulNumCondPaths = 0;
    m_ulNumCondLookup = 0;
    m_ulNumCondKeys = 0;
    m_pCondLookup = 0;
    m_pCondFrames = 0;
    m_pCondKeys = 0;
    m_pCondValues = 0;
    g_pathing_00659c60 = this;
    g_path_limit_006081e8 = 500.0f;
}

/* Take the octree's own bounds and level name. The span is the vertical extent
   of that box scaled, and the cell count is that span plus one. */
// FUNCTION: WIZ8 0x00458a50
void W8PathingService::ConfigureForLevel(
    int size, float grid_scale, int value_28, const float* bounds, const char* name)
{
    size_004 = size;
    grid_scale_01c = grid_scale;
    value_028 = value_28;
    level_bounds[0] = bounds[0];
    level_bounds[1] = bounds[1];
    level_bounds[2] = bounds[2];
    level_bounds[3] = bounds[3];
    level_bounds[4] = bounds[4];
    level_bounds[5] = bounds[5];
    span_020 = (level_bounds[4] - level_bounds[1]) * g_path_span_scale_005ec344;
    cell_count_024 = (short)(int)span_020 + 1;
    level_name = name;
}

/* Classify a waypoint from the path index cell beneath it.

   X and Z form the hash key. Entries with that key carry a one-based vertical
   cell in their low half; among candidates inside the service's vertical span,
   the closest height wins and its complete packed value is returned. */
// FUNCTION: WIZ8 0x00459c00
unsigned int W8PathingService::ClassifyWaypoint00459C00(
    const srVector3T<float>* position)
{
    int cell_x = (int)((position->x - level_bounds[0]) / grid_scale_01c);
    int cell_z = (int)((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int result = 0;

    if (key != 0) {
        W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
        W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
        unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
        int slot = static_cast<int*>(index->bucket_heads)[hash & (index->bucket_count - 1)];
        int height = (int)((position->y - level_bounds[1]) / span_020) + 1;
        int nearest = 0x0fffffff;

        while (slot != -1) {
            W8OctreeEntry* entry = &entries[slot];

            if (entry->key == key) {
                int delta = (entry->value & 0xffff) - height;

                if (delta < 0) {
                    delta = -delta;
                }
                if (delta < cell_count_024 && delta < nearest) {
                    nearest = delta;
                    result = entry->value;
                }
            }
            slot = entry->next_index;
        }
    }
    return result;
}

/* Test whether a position lies in the vertical neighborhood represented by its
   X/Z path-index cell, optionally snapping it onto that indexed cell.

   The accepted vertical window is twice the service's cell count in either
   direction. X and Z snap to the horizontal cell centers; Y snaps to the exact
   one-based height carried by the matching packed index value. */
// FUNCTION: WIZ8 0x00462e60
unsigned char W8PathingService::SnapWaypointPosition00462E60(
    srVector3T<float>* position,
    unsigned char snap_to_cell)
{
    int vertical_window = cell_count_024 * 2;
    unsigned int height =
        (unsigned int)(int)((position->y - level_bounds[1]) / span_020) + 1;
    unsigned int cell_x =
        (unsigned int)(int)((position->x - level_bounds[0]) / grid_scale_01c);
    unsigned int cell_z =
        (unsigned int)(int)((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int matched_height = height;
    unsigned char found = 0;
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = static_cast<int*>(index->bucket_heads)[hash & (index->bucket_count - 1)];

    while (slot != -1 && found == 0) {
        W8OctreeEntry* entry = &entries[slot];

        if (entry->key == key) {
            matched_height = entry->value & 0xffff;
            int delta = (int)(height - matched_height);

            if (-vertical_window < delta && delta < vertical_window) {
                found = 1;
                break;
            }
        }
        slot = entry->next_index;
    }

    if (snap_to_cell != 0 && found != 0) {
        position->y = (float)(matched_height - 1) * span_020 + level_bounds[1];
        position->x = ((float)cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z = ((float)cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }
    return found;
}

/* Sweep probes around the arc defined by a pair of waypoint positions.

   The accumulator starts perpendicular to the pair's horizontal direction.
   Every iteration probes that offset, advances by one grid-scale tangent step,
   and renormalizes to the pair's original radius. The dot product identifies
   when the sweep has passed its forward threshold and the walk stops after it
   subsequently crosses behind the starting direction. */
// FUNCTION: WIZ8 0x00462570
void W8PathingService::ProbeWaypointArc00462570(
    const srVector3T<float>* from,
    const srVector3T<float>* to)
{
    srVector3T<float> direction;
    srVector3T<float> arc;
    float radius;
    unsigned char passed_forward = 0;
    unsigned int iteration;

    direction.x = to->x - from->x;
    direction.y = to->y - from->y;
    direction.z = to->z - from->z;
    radius = (float)sqrt(
        direction.x * direction.x + direction.y * direction.y +
        direction.z * direction.z);
    arc.x = -direction.z;
    arc.y = 0.0f;
    arc.z = direction.x;

    for (iteration = 0; iteration < 50000; ++iteration) {
        srVector3T<float> probe;
        srVector3T<float> step;
        float length_squared;
        float scale;
        float dot;

        probe.x = from->x + arc.x;
        probe.y = from->y + arc.y;
        probe.z = from->z + arc.z;
        ProbeWaypointSegment00462750(from, &probe);

        step.x = arc.z;
        step.y = arc.y;
        step.z = -arc.x;
        length_squared = step.x * step.x + step.y * step.y + step.z * step.z;
        if ((double)length_squared != g_zero_005ebb40) {
            scale = grid_scale_01c / (float)sqrt(length_squared);
            step.x *= scale;
            step.y *= scale;
            step.z *= scale;
        }
        arc.x += step.x;
        arc.y += step.y;
        arc.z += step.z;

        length_squared = arc.x * arc.x + arc.y * arc.y + arc.z * arc.z;
        if ((double)length_squared != g_zero_005ebb40) {
            scale = radius / (float)sqrt(length_squared);
            arc.x *= scale;
            arc.y *= scale;
            arc.z *= scale;
        }

        dot = direction.x * arc.x + direction.y * arc.y + direction.z * arc.z;
        if (dot > g_float_005ec390) {
            passed_forward = 1;
        }
        else if (passed_forward == 0) {
            continue;
        }
        if (dot < g_float_005ebb34) {
            return;
        }
    }
}

/* Convert the signed steps on the walk's driving and secondary axes into the
   four horizontal direction codes consumed by the segment probe. */
// FUNCTION: WIZ8 0x0045aee0
void W8PathingService::GetPathGridStepDirections0045AEE0(
    const W8PathGridWalk* walk,
    int* directions)
{
    if (walk->major_axis_18 != 0) {
        if (walk->step_0c[1] < 1) {
            directions[0] = 4;
            if (walk->step_0c[0] > 0) {
                directions[1] = 2;
                return;
            }
        }
        else {
            directions[0] = 0;
            if (walk->step_0c[0] > 0) {
                directions[1] = 2;
                return;
            }
        }
        directions[1] = 6;
        return;
    }

    if (walk->step_0c[0] < 1) {
        directions[0] = 6;
    }
    else {
        directions[0] = 2;
    }
    if (walk->step_0c[1] > 0) {
        directions[1] = 0;
    }
    else {
        directions[1] = 4;
    }
}

/* Build the two-dimensional Bresenham record used to walk path-index cells.

   Coordinates are first converted to integer distances from the level origin.
   The larger absolute delta drives the walk; the start-cell remainder fixes
   how far each axis is from its next boundary and therefore the initial error. */
// FUNCTION: WIZ8 0x0045af60
void W8PathingService::BuildPathGridWalk0045AF60(
    const float* from,
    const float* to,
    const float* origin,
    W8PathGridWalk* walk)
{
    int cell_size = (int)grid_scale_01c;
    int coordinate[2];
    int destination[2];
    int step[2];
    int absolute_delta[2];
    float boundary_offset[2];
    float signed_delta[2];
    int major_axis = 0;
    int largest_delta = 0;
    int axis;

    for (axis = 0; axis < 2; ++axis) {
        coordinate[axis] = (int)(from[axis] - origin[axis]);
        destination[axis] = (int)(to[axis] - origin[axis]);

        int delta = destination[axis] - coordinate[axis];
        boundary_offset[axis] =
            (float)(coordinate[axis] % cell_size) / grid_scale_01c;
        signed_delta[axis] = (float)delta;

        if (delta < 0) {
            step[axis] = -1;
            delta = -delta;
        }
        else {
            step[axis] = 1;
            boundary_offset[axis] = g_float_005ebb38 - boundary_offset[axis];
        }
        if (largest_delta < delta) {
            largest_delta = delta;
            major_axis = axis;
        }
        absolute_delta[axis] = delta;
    }

    int minor_axis = (major_axis + 1) % 2;
    float ratio = signed_delta[minor_axis] / signed_delta[major_axis];
    int error_delta = (int)((ratio < 0.0f ? -ratio : ratio) * grid_scale_01c);
    int error = (int)(
        (float)cell_size * boundary_offset[minor_axis] -
        (float)error_delta * boundary_offset[major_axis]);
    int count;

    if (largest_delta % cell_size == 0) {
        count = largest_delta / cell_size;
    }
    else {
        count = largest_delta / cell_size + 1;
    }

    walk->major_axis_18 = major_axis;
    walk->minor_axis_1c = minor_axis;
    walk->cell_size_30 = cell_size;
    walk->count_24 = count;
    walk->cell_00[0] = destination[0] / cell_size;
    walk->step_0c[0] = step[0];
    walk->step_0c[1] = step[1];
    walk->error_28 = error_delta;
    walk->error_2c = error;
    walk->cell_00[1] = destination[1] / cell_size;
    walk->value_08 = 0;
    walk->value_14 = 0;
    walk->value_20 = 0;
    walk->value_34[0] = 0;
    walk->value_34[1] = 0;
    walk->value_34[2] = 0;
}

/* Walk every horizontal path cell crossed by a short waypoint segment.

   Each cell chooses the first vertically compatible path record. The secondary
   index carries the accumulated low-half cost and the most recent high-half
   step cost; when a bounded probe is active, the cheapest reached cell and its
   world-space center are retained on the service. Direction bits on the chosen
   path record can terminate the walk after the corresponding grid step. */
// FUNCTION: WIZ8 0x00462750
unsigned char W8PathingService::ProbeWaypointSegment00462750(
    const srVector3T<float>* from,
    const srVector3T<float>* to)
{
    float delta_x = to->x - from->x;
    float delta_y = to->y - from->y;
    float delta_z = to->z - from->z;
    float distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

    if (grid_scale_01c + grid_scale_01c > distance) {
        return 1;
    }

    int cell[2];
    float walk_from[2];
    float walk_to[2];
    float origin[2];
    W8PathGridWalk walk;
    int directions[2];

    cell[0] = (int)((from->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = (int)((from->z - level_bounds[2]) / grid_scale_01c);
    walk_from[0] = from->x;
    walk_from[1] = from->z;
    walk_to[0] = to->x;
    walk_to[1] = to->z;
    origin[0] = level_bounds[0];
    origin[1] = level_bounds[2];
    BuildPathGridWalk0045AF60(walk_from, walk_to, origin, &walk);
    GetPathGridStepDirections0045AEE0(&walk, directions);

    int error = walk.error_2c;
    unsigned char bounded_probe = flag_08c != 0 && probe_limit_088 != 0;
    unsigned int height =
        (unsigned int)(int)((from->y - level_bounds[1]) / span_020) + 1;
    unsigned char blocked = 0;
    int iteration = 0;

    while (iteration < walk.count_24 && blocked == 0) {
        unsigned int cell_key = cell[1] * 0x10000 + cell[0];
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        W8OctreeIndex* visited_index =
            static_cast<W8OctreeIndex*>(m_pIndex_074);
        W8OctreeEntry* visited_entries =
            static_cast<W8OctreeEntry*>(visited_index->entries);
        int slot = static_cast<int*>(visited_index->bucket_heads)[
            hash & (visited_index->bucket_count - 1)];
        unsigned int visited = 0;

        while (slot != -1) {
            W8OctreeEntry* entry = &visited_entries[slot];
            if (entry->key == cell_key) {
                visited = entry->value;
                break;
            }
            slot = entry->next_index;
        }

        if (bounded_probe != 0 && (visited & 0xffff) != 0xffff) {
            bounded_probe = 0;
        }

        unsigned int direction_mask = 0;
        unsigned int path_value = 0;
        unsigned char found = 0;

        if (iteration == 0 || (visited & 0xffff0000) != 0xffff0000 ||
            bounded_probe != 0) {
            W8OctreeIndex* path_index =
                static_cast<W8OctreeIndex*>(m_pIndex_064);
            W8OctreeEntry* path_entries =
                static_cast<W8OctreeEntry*>(path_index->entries);
            slot = static_cast<int*>(path_index->bucket_heads)[
                hash & (path_index->bucket_count - 1)];

            while (slot != -1) {
                W8OctreeEntry* entry = &path_entries[slot];

                if (entry->key == cell_key) {
                    path_value = entry->value;
                    int height_delta = (path_value & 0xffff) - height;

                    if ((path_value & 0x10000000) == 0 &&
                        -cell_count_024 < height_delta &&
                        height_delta < cell_count_024) {
                        if ((path_value & 0x01000000) != 0) {
                            direction_mask = path_value >> 16 & 0xff;
                        }
                        found = 1;
                        height = path_value & 0xffff;
                        break;
                    }
                }
                slot = entry->next_index;
            }

            if (found == 0) {
                blocked = 1;
            }
            else if (bounded_probe == 0 &&
                     ((probe_limit_088 == 0 && visited == 0) ||
                      (probe_limit_088 != 0 && (visited & 0xffff) != 0 &&
                       (visited & 0xffff0000) == 0))) {
                srVector3T<float> position;
                unsigned int initial_cost;

                position.x =
                    ((float)cell[0] + g_float_005ebc7c) * grid_scale_01c +
                    level_bounds[0];
                position.y =
                    (float)(height - 1) * span_020 + level_bounds[1];
                position.z =
                    ((float)cell[1] + g_float_005ebc7c) * grid_scale_01c +
                    level_bounds[2];

                delta_x = position.x - to->x;
                delta_y = position.y - to->y;
                delta_z = position.z - to->z;
                initial_cost = (unsigned int)(int)(
                    sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z) *
                    g_double_005ec3b0);

                if (probe_limit_088 == 0) {
                    if (visited_index->free_head == -1) {
                        GrowIndex00439290(visited_index);
                    }
                    int inserted = visited_index->free_head;
                    W8OctreeEntry* entries =
                        static_cast<W8OctreeEntry*>(visited_index->entries);
                    visited_index->free_head = entries[inserted].next_index;
                    entries[inserted].key = cell_key;
                    entries[inserted].value = initial_cost;
                    unsigned int bucket =
                        hash & (visited_index->bucket_count - 1);
                    entries[inserted].next_index =
                        static_cast<int*>(visited_index->bucket_heads)[bucket];
                    static_cast<int*>(visited_index->bucket_heads)[bucket] = inserted;
                }
                else {
                    unsigned int step_cost = (unsigned int)(int)(
                        (double)initial_cost * g_double_005ec3a8);
                    unsigned int total_cost = (visited & 0xffff) + step_cost;

                    if (total_cost < probe_limit_088) {
                        probe_limit_088 = total_cost;
                        probe_cell_key_078 = cell_key;
                        probe_position_07c = position;
                    }

                    int* bucket = static_cast<int*>(visited_index->bucket_heads) +
                        (hash & (visited_index->bucket_count - 1));
                    int removed = *bucket;
                    int previous = -1;
                    W8OctreeEntry* entries =
                        static_cast<W8OctreeEntry*>(visited_index->entries);

                    while (removed != -1) {
                        W8OctreeEntry* entry = &entries[removed];
                        if (entry->key == cell_key &&
                            (unsigned int)entry->value == visited) {
                            if (previous == -1) {
                                *bucket = entry->next_index;
                            }
                            else {
                                entries[previous].next_index = entry->next_index;
                            }
                            entry->next_index = visited_index->free_head;
                            visited_index->free_head = removed;
                            break;
                        }
                        previous = removed;
                        removed = entry->next_index;
                    }

                    if (visited_index->free_head == -1) {
                        GrowIndex00439290(visited_index);
                    }
                    int inserted = visited_index->free_head;
                    entries = static_cast<W8OctreeEntry*>(visited_index->entries);
                    visited_index->free_head = entries[inserted].next_index;
                    entries[inserted].key = cell_key;
                    entries[inserted].value = step_cost << 16 | visited;
                    unsigned int bucket_index =
                        hash & (visited_index->bucket_count - 1);
                    entries[inserted].next_index =
                        static_cast<int*>(visited_index->bucket_heads)[bucket_index];
                    static_cast<int*>(visited_index->bucket_heads)[bucket_index] = inserted;
                }
            }
        }
        else {
            blocked = 1;
        }

        unsigned int direction;
        if (error >= 0 || blocked != 0) {
            direction = directions[0];
            cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
            error -= walk.error_28;
        }
        else {
            direction = directions[1];
            --iteration;
            cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
            error += walk.cell_size_30;
        }
        if (direction_mask != 0 &&
            (direction_mask & 1 << (direction & 0x1f)) == 0) {
            blocked = 1;
        }
        ++iteration;
    }

    return blocked == 0;
}

/* Find the directions from one path cell that lead to vertically compatible
   neighboring cells. A source record carrying an explicit direction mask only
   permits those directions to be tested. As in retail, an entirely open set
   of eight neighbors is represented by zero rather than 0xff. */
// FUNCTION: WIZ8 0x004667a0
unsigned int W8PathingService::ComputeWaypointNeighborMask004667A0(
    const int* cell,
    unsigned int path_value)
{
    unsigned int source_directions = 0;
    if ((path_value & 0x01000000) != 0) {
        source_directions = path_value >> 16 & 0xff;
    }

    unsigned int result = 0;
    int direction;
    for (direction = 0; direction < 8; ++direction) {
        if ((path_value & 0x01000000) == 0 ||
            (source_directions & 1 << (direction & 0x1f)) != 0) {
            int neighbor[2];
            neighbor[0] = cell[0];
            neighbor[1] = cell[1];

            if (direction >= 1 && direction <= 3) {
                ++neighbor[0];
            }
            else if (direction > 4) {
                --neighbor[0];
            }
            if (direction < 2 || direction > 6) {
                ++neighbor[1];
            }
            else if (direction > 2 && direction < 6) {
                --neighbor[1];
            }

            unsigned int key = neighbor[1] * 0x10000 + neighbor[0];
            unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
            W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
            W8OctreeEntry* entries =
                static_cast<W8OctreeEntry*>(index->entries);
            int slot = static_cast<int*>(index->bucket_heads)[
                hash & (index->bucket_count - 1)];
            unsigned char found = 0;

            while (slot != -1) {
                W8OctreeEntry* entry = &entries[slot];
                if (entry->key == key) {
                    unsigned int candidate = entry->value;
                    int height_delta =
                        (path_value & 0xffff) - (candidate & 0xffff);
                    if ((candidate & 0x10000000) == 0 &&
                        -cell_count_024 < height_delta &&
                        height_delta < cell_count_024) {
                        found = 1;
                        break;
                    }
                }
                slot = entry->next_index;
            }
            if (found != 0) {
                result |= 1 << (direction & 0x1f);
            }
        }
    }

    if ((unsigned char)result == 0xff) {
        result = 0;
    }
    return result;
}

/* Test a waypoint span through the indexed path cells.

   The ordinary mode rejects endpoints outside the level and verifies the
   destination height. Adjustment mode instead snaps a failed destination to
   the last accepted cell; its alternate stepping mode combines simultaneous
   major/minor moves into the corresponding diagonal direction. Packed path
   records contribute their direction masks and two obstruction flag bits. */
// FUNCTION: WIZ8 0x0045a1b0
unsigned char W8PathingService::TestWaypointSpan0045A1B0(
    const srVector3T<float>* source,
    srVector3T<float>* destination,
    unsigned char adjust_destination,
    unsigned char diagonal_steps)
{
    unsigned char blocked = 0;

    if (adjust_destination == 0 &&
        (source->x < level_bounds[0] || source->y < level_bounds[1] ||
         source->z < level_bounds[2] || level_bounds[3] < source->x ||
         level_bounds[4] < source->y || level_bounds[5] < source->z ||
         destination->x < level_bounds[0] ||
         destination->y < level_bounds[1] ||
         destination->z < level_bounds[2] ||
         level_bounds[3] < destination->x ||
         level_bounds[4] < destination->y ||
         level_bounds[5] < destination->z)) {
        return 0;
    }

    flag_23c = 0;
    int cell[2];
    cell[0] = (int)((source->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = (int)((source->z - level_bounds[2]) / grid_scale_01c);
    unsigned int cell_key = cell[1] * 0x10000 + cell[0];
    int destination_x =
        (int)((destination->x - level_bounds[0]) / grid_scale_01c);
    int destination_z =
        (int)((destination->z - level_bounds[2]) / grid_scale_01c);
    unsigned int destination_key = destination_z * 0x10000 + destination_x;
    waypoint_neighbor_mask_0a0 = 0;

    W8OctreeIndex* path_index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(path_index->entries);

    if (cell_key == destination_key) {
        unsigned int height =
            (unsigned int)(int)((source->y - level_bounds[1]) / span_020) + 1;
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        int slot = static_cast<int*>(path_index->bucket_heads)[
            hash & (path_index->bucket_count - 1)];
        unsigned int source_value = 0;
        unsigned char found = 0;

        while (slot != -1) {
            W8OctreeEntry* entry = &entries[slot];
            if (entry->key == cell_key) {
                unsigned int value = entry->value;
                int difference = (value & 0xffff) - height;
                if ((value & 0x10000000) == 0 &&
                    -cell_count_024 < difference &&
                    difference < cell_count_024) {
                    source_value = value;
                    found = 1;
                    break;
                }
            }
            slot = entry->next_index;
        }
        if (found == 0) {
            return 0;
        }

        waypoint_neighbor_mask_0a0 =
            ComputeWaypointNeighborMask004667A0(cell, source_value);
        height = (unsigned int)(int)(
            (destination->y - level_bounds[1]) / span_020) + 1;
        slot = static_cast<int*>(path_index->bucket_heads)[
            hash & (path_index->bucket_count - 1)];
        found = 0;

        while (slot != -1) {
            W8OctreeEntry* entry = &entries[slot];
            if (entry->key == cell_key) {
                unsigned int value = entry->value;
                int difference = (value & 0xffff) - height;
                if ((value & 0x10000000) == 0 &&
                    -cell_count_024 < difference &&
                    difference < cell_count_024) {
                    found = value == source_value;
                    break;
                }
            }
            slot = entry->next_index;
        }
        return found;
    }

    float walk_source[2];
    float walk_destination[2];
    float origin[2];
    W8PathGridWalk walk;
    int directions[2];
    walk_source[0] = source->x;
    walk_source[1] = source->z;
    walk_destination[0] = destination->x;
    walk_destination[1] = destination->z;
    origin[0] = level_bounds[0];
    origin[1] = level_bounds[2];
    BuildPathGridWalk0045AF60(
        walk_source, walk_destination, origin, &walk);
    GetPathGridStepDirections0045AEE0(&walk, directions);

    int error = walk.error_2c;
    unsigned int height =
        (unsigned int)(int)((source->y - level_bounds[1]) / span_020) + 1;
    unsigned int previous_key = 0;
    unsigned int previous_value;
    int iteration = 0;

    while (iteration < walk.count_24 && blocked == 0) {
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        int slot = static_cast<int*>(path_index->bucket_heads)[
            hash & (path_index->bucket_count - 1)];
        unsigned int direction_mask = 0;
        unsigned int path_value;
        unsigned char found = 0;

        while (slot != -1) {
            W8OctreeEntry* entry = &entries[slot];
            if (entry->key == cell_key) {
                path_value = entry->value;
                int difference = (path_value & 0xffff) - height;

                if (-cell_count_024 < difference &&
                    difference < cell_count_024) {
                    if ((path_value & 0x10000000) == 0) {
                        if ((path_value & 0x01000000) != 0) {
                            direction_mask = path_value >> 16 & 0xff;
                        }
                        found = 1;
                        height = path_value & 0xffff;
                        previous_key = cell_key;
                        previous_value = path_value;
                        break;
                    }
                    flag_23c = 1;
                }
            }
            slot = entry->next_index;
        }

        if (found == 0) {
            unsigned int mask_key = previous_key;
            unsigned int mask_value = previous_value;
            if (previous_key == 0) {
                mask_key = cell_key;
                mask_value = path_value;
            }
            if (previous_key != 0 || cell_key != 0) {
                int mask_cell[2];
                mask_cell[0] = mask_key & 0xffff;
                mask_cell[1] = mask_key >> 16;
                waypoint_neighbor_mask_0a0 =
                    ComputeWaypointNeighborMask004667A0(mask_cell, mask_value);
            }
            blocked = 1;
        }

        if ((previous_value & 0x04000000) != 0) {
            flag_23c = 1;
        }

        unsigned int direction = directions[0];
        if (diagonal_steps == 0) {
            if (error >= 0 || blocked != 0) {
                cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
                error -= walk.error_28;
            }
            else {
                cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
                direction = directions[1];
                --iteration;
                error += walk.cell_size_30;
            }
        }
        else {
            if (error < 0 && blocked == 0) {
                cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
                error += walk.cell_size_30;
                if (cell[1] * 0x10000 + cell[0] == destination_key) {
                    --iteration;
                    direction = directions[1];
                    goto stepped;
                }
                if ((directions[0] == 0 && directions[1] == 6) ||
                    (directions[0] == 6 && directions[1] == 0)) {
                    direction = 7;
                }
                else {
                    direction = (directions[0] + directions[1]) / 2;
                }
            }
            cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
            error -= walk.error_28;
        }

stepped:
        if (cell_key == destination_key) {
            iteration = walk.count_24;
        }
        else if (direction_mask != 0 &&
                 (direction_mask & 1 << (direction & 0x1f)) == 0) {
            unsigned int mask_key = previous_key;
            unsigned int mask_value = previous_value;
            if (previous_key == 0) {
                mask_key = cell_key;
                mask_value = path_value;
            }
            if (previous_key != 0 || cell_key != 0) {
                int mask_cell[2];
                mask_cell[0] = mask_key & 0xffff;
                mask_cell[1] = mask_key >> 16;
                waypoint_neighbor_mask_0a0 =
                    ComputeWaypointNeighborMask004667A0(mask_cell, mask_value);
            }
            blocked = 1;
        }

        cell_key = cell[1] * 0x10000 + cell[0];
        ++iteration;
    }

    if (adjust_destination == 0) {
        if (blocked == 0) {
            int destination_height =
                (int)((destination->y - level_bounds[1]) / span_020) + 1;
            int difference = destination_height - height;
            if (difference < -cell_count_024 || cell_count_024 < difference) {
                blocked = 1;
            }
        }
    }
    else if (previous_key == 0) {
        *destination = *source;
    }
    else {
        if (blocked != 0) {
            destination->x =
                ((float)(previous_key & 0xffff) + g_float_005ebc7c) *
                    grid_scale_01c +
                level_bounds[0];
            destination->z =
                ((float)(previous_key >> 16) + g_float_005ebc7c) *
                    grid_scale_01c +
                level_bounds[2];
        }
        destination->y = (float)(height - 1) * span_020 + level_bounds[1];
    }

    return blocked == 0;
}

/* Compare clearance along the two compass rays bracketing a horizontal
   direction. The normalized Z component selects the pair; the sign of X
   selects which half of the compass owns the middle bands. */
// FUNCTION: WIZ8 0x0045aac0
float W8PathingService::CompareDirectionalClearance0045AAC0(
    const srVector3T<float>* position,
    const srVector3T<float>* direction,
    float distance)
{
    float normalized_x = direction->x;
    float normalized_z = direction->z;
    float length_squared =
        normalized_x * normalized_x + normalized_z * normalized_z;

    if ((double)length_squared != g_zero_005ebb40) {
        float scale = (float)(g_double_005ebc30 / sqrt(length_squared));
        normalized_x *= scale;
        normalized_z *= scale;
    }

    int first_direction;
    int second_direction;
    if (normalized_x <= g_float_005ebb34) {
        if (g_path_direction_threshold_3_005ec354 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        }
        else if (g_path_direction_threshold_2_005ec350 < normalized_z) {
            first_direction = 6;
            second_direction = 0;
        }
        else if (normalized_z <= g_path_direction_threshold_1_005ec34c) {
            if (normalized_z <= g_path_direction_threshold_0_005ec348) {
                first_direction = 3;
                second_direction = 5;
            }
            else {
                first_direction = 4;
                second_direction = 6;
            }
        }
        else {
            first_direction = 5;
            second_direction = 7;
        }
    }
    else {
        if (g_path_direction_threshold_3_005ec354 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        }
        else if (g_path_direction_threshold_2_005ec350 < normalized_z) {
            first_direction = 0;
            second_direction = 2;
        }
        else if (g_path_direction_threshold_1_005ec34c < normalized_z) {
            first_direction = 1;
            second_direction = 3;
        }
        else if (g_path_direction_threshold_0_005ec348 < normalized_z) {
            first_direction = 2;
            second_direction = 4;
        }
        else {
            first_direction = 3;
            second_direction = 5;
        }
    }

    int cell[2];
    cell[0] = (int)((position->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = (int)((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int height =
        (unsigned int)(int)((position->y - level_bounds[1]) / span_020) + 1;
    float first = MeasureDirectionalPath0045AC70(
        cell, first_direction, height, distance);
    float second = MeasureDirectionalPath0045AC70(
        cell, second_direction, height, distance);
    return second - first;
}

/* Measure how much of a requested run remains traversable in one compass
   direction. Cardinal runs use the retail diagonal-to-axis scale before cell
   stepping; every crossed cell must carry a vertically compatible record whose
   explicit direction mask, when present, permits the same direction. */
// FUNCTION: WIZ8 0x0045ac70
float W8PathingService::MeasureDirectionalPath0045AC70(
    const int* cell,
    int direction,
    unsigned int height,
    float distance)
{
    int step_x;
    int step_z;
    float remaining = distance;

    switch (direction) {
    case 0:
        step_x = 0;
        step_z = 1;
        remaining *= g_path_cardinal_scale_005ec358;
        break;
    case 1:
        step_x = 1;
        step_z = 1;
        break;
    case 2:
        step_x = 1;
        step_z = 0;
        remaining *= g_path_cardinal_scale_005ec358;
        break;
    case 3:
        step_x = 1;
        step_z = -1;
        break;
    case 4:
        step_x = 0;
        step_z = -1;
        remaining *= g_path_cardinal_scale_005ec358;
        break;
    case 5:
        step_x = -1;
        step_z = -1;
        break;
    case 6:
        step_x = -1;
        step_z = 0;
        remaining *= g_path_cardinal_scale_005ec358;
        break;
    default:
        step_x = -1;
        step_z = 1;
        break;
    }

    int cell_x = cell[0];
    int cell_z = cell[1];
    unsigned char stopped = 0;

    while (grid_scale_01c < remaining) {
        cell_x += step_x;
        cell_z += step_z;
        unsigned int key = cell_z * 0x10000 + cell_x;
        unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
        W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
        W8OctreeEntry* entries =
            static_cast<W8OctreeEntry*>(index->entries);
        int slot = static_cast<int*>(index->bucket_heads)[
            hash & (index->bucket_count - 1)];
        unsigned int path_value;
        unsigned char found = 0;

        while (slot != -1) {
            W8OctreeEntry* entry = &entries[slot];
            if (entry->key == key) {
                path_value = entry->value;
                int difference = (path_value & 0xffff) - height;
                if ((path_value & 0x10000000) == 0 &&
                    -cell_count_024 < difference &&
                    difference < cell_count_024) {
                    found = 1;
                    height = path_value & 0xffff;
                    break;
                }
            }
            slot = entry->next_index;
        }

        if (found != 0) {
            remaining -= grid_scale_01c;
        }
        if (found == 0 ||
            ((path_value & 0x01000000) != 0 &&
             (path_value & 1 << ((direction + 16) & 0x1f)) == 0)) {
            stopped = 1;
            break;
        }
    }

    if (stopped == 0) {
        return distance;
    }
    return distance - remaining;
}

/* Find a waypoint surface near a world position.

   Nearby kind-nine octree objects are ordered by integer three-dimensional
   distance. A very close horizontal match wins immediately; otherwise the
   first candidate connected by the ordinary span test is selected. Exhaustive
   mode retries the ordered candidates with paired arc probes, rebuilding the
   temporary visitation index for every attempt. */
// FUNCTION: WIZ8 0x0045b120
unsigned short W8PathingService::FindWaypoint0045B120(
    const srVector3T<float>* position,
    unsigned char exhaustive)
{
    srVector3T<float> query = *position;
    unsigned short result = 0;
    value_1d4 = 0;

    if (SnapWaypointPosition00462E60(&query, 0) == 0) {
        return 0;
    }

    srVector3T<float> lower;
    srVector3T<float> upper;
    lower.x = query.x - g_path_waypoint_query_horizontal_005ec360;
    lower.y = query.y - g_path_waypoint_query_vertical_005ec35c;
    lower.z = query.z - g_path_waypoint_query_horizontal_005ec360;
    upper.x = query.x + g_path_waypoint_query_horizontal_005ec360;
    upper.y = query.y + g_path_waypoint_query_vertical_005ec35c;
    upper.z = query.z + g_path_waypoint_query_horizontal_005ec360;

    int* candidates = 0;
    int count = g_octree_6598a4->QueryObjects0042F280(
        &candidates, &lower, &upper, 9, -1);
    if (count == 0) {
        return result;
    }
    if ((unsigned int)count >= 200) {
        srAssertFail(
            "s_ulCount<200", OCTPATH_CPP, 0xe0c, "Too many nodes in list");
    }

    unsigned int distances[199];
    int index;
    if (count > 1) {
        for (index = 0; index < count; ++index) {
            const srVector3T<float>* candidate =
                &m_pSurfaces_048[candidates[index]].position_04;
            float delta_x = query.x - candidate->x;
            float delta_y = query.y - candidate->y;
            float delta_z = query.z - candidate->z;
            distances[index] = (unsigned int)(int)sqrt(
                delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

            if ((double)distances[index] <
                    g_path_waypoint_exact_distance_005ebc64 &&
                sqrt(delta_x * delta_x + delta_z * delta_z) <
                    g_path_waypoint_snap_distance_005ec150) {
                result = (unsigned short)candidates[index];
            }
        }

        if (result == 0) {
            for (index = 1; index < count; ++index) {
                unsigned int distance = distances[index];
                int candidate = candidates[index];
                int insertion = index;

                while (insertion > 0 &&
                       distance < distances[insertion - 1]) {
                    distances[insertion] = distances[insertion - 1];
                    candidates[insertion] = candidates[insertion - 1];
                    --insertion;
                }
                distances[insertion] = distance;
                candidates[insertion] = candidate;
            }
        }
    }

    for (index = 0; index < count; ++index) {
        if (result != 0) {
            return result;
        }
        if (TestWaypointSpan0045A1B0(
                &query, &m_pSurfaces_048[candidates[index]].position_04, 0, 0) !=
            0) {
            result = (unsigned short)candidates[index];
        }
    }

    if (result == 0 && exhaustive != 0) {
        probe_position_07c.x = 0.0f;
        probe_position_07c.y = 0.0f;
        probe_position_07c.z = 0.0f;

        W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pIndex_074);
        if (visited->bucket_count != 0) {
            ::operator delete(visited->bucket_heads);
            ::operator delete(visited->entries);
        }
        visited->bucket_count = 0;
        visited->bucket_heads = 0;
        visited->entries = 0;
        visited->free_head = -1;
        GrowIndex00439290(visited);
        value_1d4 = 0;
        flag_08c = 0;

        for (index = 0; index < count; ++index) {
            if (value_1d4 != 0) {
                return result;
            }

            unsigned char saved_flag = flag_08c;
            visited = static_cast<W8OctreeIndex*>(m_pIndex_074);
            flag_08c = 0;
            if (visited->bucket_count != 0) {
                ::operator delete(visited->bucket_heads);
                ::operator delete(visited->entries);
            }
            visited->bucket_count = 0;
            visited->bucket_heads = 0;
            visited->entries = 0;
            visited->free_head = -1;
            GrowIndex00439290(visited);

            probe_cell_key_078 = 0;
            probe_limit_088 = 0;
            srVector3T<float>* candidate =
                &m_pSurfaces_048[candidates[index]].position_04;
            ProbeWaypointArc00462570(&query, candidate);
            flag_08c = saved_flag;
            probe_limit_088 = 0xffffffff;
            ProbeWaypointArc00462570(candidate, &query);
            if (probe_cell_key_078 != 0) {
                value_1d4 = (unsigned short)candidates[index];
            }
        }
    }

    return result;
}

/* Snap only the vertical component of a position to the first path-index
   record in the same horizontal cell and inside the service's vertical band.

   Unlike SnapWaypointPosition00462E60, this operation leaves X and Z exactly
   as supplied. The one-based height stored in the index is converted back to
   the level's world-space Y coordinate. */
// FUNCTION: WIZ8 0x0045b5a0
void W8PathingService::SnapPathHeight0045B5A0(
    srVector3T<float>* position)
{
    int cell_x = (int)((position->x - level_bounds[0]) / grid_scale_01c);
    int cell_z = (int)((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;

    if (key == 0) {
        return;
    }

    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = static_cast<int*>(index->bucket_heads)[
        hash & (index->bucket_count - 1)];
    int height =
        (int)((position->y - level_bounds[1]) / span_020) + 1;

    while (slot != -1) {
        W8OctreeEntry* entry = &entries[slot];
        if (entry->key == key) {
            unsigned int matched_height = entry->value & 0xffff;
            int difference = (int)matched_height - height;
            if (-cell_count_024 < difference &&
                difference < cell_count_024) {
                position->y =
                    (float)(matched_height - 1) * span_020 + level_bounds[1];
                return;
            }
        }
        slot = entry->next_index;
    }
}

/* Derive the path surface normal from the retail three-point construction.

   The middle point is height-snapped before receiving the same X offset as
   the second sample. This unusual order is intentional: it is the exact
   construction in the retail body, not a conventionalized terrain sampler. */
// FUNCTION: WIZ8 0x0045b730
void W8PathingService::GetPathSurfaceNormal0045B730(
    const srVector3T<float>* position,
    srVector3T<float>* normal)
{
    srVector3T<float> first = *position;
    srVector3T<float> middle = *position;
    srVector3T<float> second = *position;

    SnapPathHeight0045B5A0(&middle);
    second.x += grid_scale_01c;
    middle.x += grid_scale_01c;
    SnapPathHeight0045B5A0(&second);
    SnapPathHeight0045B5A0(&first);

    normal->x =
        (second.z - middle.z) * (first.y - middle.y) -
        (second.y - middle.y) * (first.z - middle.z);
    normal->y =
        (second.y - middle.y) * (first.x - middle.x) -
        (second.x - middle.x) * (first.y - middle.y);
    normal->z =
        (second.x - middle.x) * (first.z - middle.z) -
        (second.z - middle.z) * (first.x - middle.x);

    float length_squared =
        normal->x * normal->x + normal->y * normal->y + normal->z * normal->z;
    if (length_squared != (float)g_zero_005ebb40) {
        float scale = (float)(g_double_005ebc30 / sqrt(length_squared));
        normal->x *= scale;
        normal->y *= scale;
        normal->z *= scale;
    }
}

/* Activate the eligible trigger prop intersecting a navigator's next path
   segment.

   Ordinary movement supplies a box from the current position to twice the
   velocity. Edge mode instead resolves the attachment's current waypoint pair
   and accepts only an edge carrying both dynamic bits. Kind-eight octree hits
   are filtered to active props whose trigger owner permits this activation;
   when several remain, the owner nearest the segment midpoint wins. */
// FUNCTION: WIZ8 0x0045b880
void W8PathingService::ActivateMovementTrigger0045B880(
    W8NavigatorMovementState* movement,
    unsigned char use_path_edge)
{
    if ((movement->unknown_000 & 0x10000000) == 0) {
        return;
    }

    srVector3T<float> lower;
    srVector3T<float> upper;

    if (use_path_edge == 0) {
        if (movement->velocity_034.x == g_float_005ebb34 &&
            movement->velocity_034.y == g_float_005ebb34 &&
            movement->velocity_034.z == g_float_005ebb34) {
            return;
        }
        lower = movement->position_040;
        upper.x = movement->position_040.x + movement->velocity_034.x * 2.0f;
        upper.y = movement->position_040.y + movement->velocity_034.y * 2.0f;
        upper.z = movement->position_040.z + movement->velocity_034.z * 2.0f;
    }
    else {
        W8NavigatorAttachment* attachment = movement->attachment_0ac;

        /* Retail reaches the shared query with the local bounds untouched
           when this cursor is exhausted. Keep that source-level fallthrough;
           callers normally enter edge mode only while a pair remains. */
        if (attachment->value_04 < attachment->value_08) {
            unsigned short* pairs =
                static_cast<unsigned short*>(attachment->allocation_50);
            unsigned short source = pairs[attachment->value_04];
            unsigned short destination = pairs[attachment->value_04 + 1];
            W8PathSurface* source_surface = &m_pSurfaces_048[source];
            unsigned short edge_index = source_surface->first_edge_24;

            if (edge_index == 0) {
                return;
            }
            while (m_pEdges_04c[edge_index].destination_06 != destination) {
                edge_index = m_pEdges_04c[edge_index].next_0c;
                if (edge_index == 0) {
                    return;
                }
            }

            unsigned int flags = m_pEdges_04c[edge_index].flags_00;
            if ((flags & 0x10000000) == 0 ||
                (flags & 0x80000000) == 0) {
                return;
            }
            lower = source_surface->position_04;
            upper = m_pSurfaces_048[destination].position_04;
        }
    }

    if (upper.x < lower.x) {
        float temporary = lower.x;
        lower.x = upper.x;
        upper.x = temporary;
    }
    if (upper.y < lower.y) {
        float temporary = lower.y;
        lower.y = upper.y;
        upper.y = temporary;
    }
    if (upper.z < lower.z) {
        float temporary = lower.z;
        lower.z = upper.z;
        upper.z = temporary;
    }

    int* candidates = 0;
    int count = g_octree_6598a4->QueryObjects0042F280(
        &candidates, &lower, &upper, 8, -1);
    if (count <= 0) {
        return;
    }

    Trigger* selected = 0;
    if (count == 1) {
        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[0]);
        Trigger* trigger =
            reinterpret_cast<Trigger*>(prop->GetGDPropValue24());
        if (prop->GetSetting6C() == 0 || trigger == 0 ||
            (trigger->flags_0a0 & 0x100) == 0) {
            return;
        }
        selected = trigger;
    }
    else {
        srVector3T<float> midpoint;
        midpoint.x =
            (float)((upper.x - lower.x) * g_double_005ebe80) + lower.x;
        midpoint.y =
            (float)((upper.y - lower.y) * g_double_005ebe80) + lower.y;
        midpoint.z =
            (float)((upper.z - lower.z) * g_double_005ebe80) + lower.z;
        double nearest_distance = 1e32;

        for (int index = 0; index < count; ++index) {
            W8Prop* prop =
                *g_world->collidable_props->GetAt(candidates[index]);
            Trigger* trigger =
                reinterpret_cast<Trigger*>(prop->GetGDPropValue24());
            if (prop->GetSetting6C() != 0 && trigger != 0 &&
                (trigger->flags_0a0 & 0x100) != 0) {
                srVector3T<float> center;
                prop->GetCenterPosition(&center);
                float difference_x = center.x - midpoint.x;
                float difference_y = center.y - midpoint.y;
                float difference_z = center.z - midpoint.z;
                double distance =
                    difference_x * difference_x +
                    difference_y * difference_y +
                    difference_z * difference_z;
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    selected = trigger;
                }
            }
        }
    }

    if (selected != 0) {
        selected->Activate00444750();
    }
}

/* Drive the path editor's owned scene node from the service's mode flags.

   The ordinary mode draws one adjusted position or hides the existing node.
   Active path mode prepares the source/destination pair and rebuilds the
   visualization when the collector reports content. The alternate editor
   mode lazily creates and attaches its node before drawing the adjusted point.
   Visibility flag order follows the retail exits exactly. */
// FUNCTION: WIZ8 0x0045bc40
void W8PathingService::UpdatePathVisualization0045BC40(
    const srVector3T<float>* source,
    const srVector3T<float>* destination)
{
    W8World* world = GetWorld();
    srNode* node = reinterpret_cast<srNode*>(m_owned_054);

    if (flag_1c8 != 0) {
        srVector3T<float> adjusted = *source;
        srVector3T<float> endpoint = *destination;
        g_octree_6598a4->AdjustPosition00431DA0(&adjusted, 1);
        PreparePathVisualization0045E840(&adjusted, &endpoint);

        if (CollectPathVisualization0045D880(&adjusted) != 0) {
            if (m_owned_054 != 0) {
                BuildPathVisualization0045BE30();
                reinterpret_cast<srNode*>(m_owned_054)->clearFlag(
                    srNode::FLAG_POSITIONAL_0);
                return;
            }

            m_owned_054 = BuildPathVisualization0045BE30();
            node = reinterpret_cast<srNode*>(m_owned_054);
            if (node != 0) {
                node->setParent(world->dynamic_scene, 1);
                node->clearFlag(srNode::FLAG_POSITIONAL_0);
                return;
            }
            node->clearFlag(srNode::FLAG_POSITIONAL_0);
            return;
        }

        node = reinterpret_cast<srNode*>(m_owned_054);
        if (node != 0) {
            node->setFlag(srNode::FLAG_POSITIONAL_0);
            node->setFlag(srNode::FLAG_POSITIONAL_1);
        }
        return;
    }

    if (flag_1c9 == 0 && flag_1cb == 0) {
        DrawPathPosition0045C9A0(*source, 0);
        node = reinterpret_cast<srNode*>(m_owned_054);
        if (node != 0) {
            node->setFlag(srNode::FLAG_POSITIONAL_0);
            node->setFlag(srNode::FLAG_POSITIONAL_1);
        }
        return;
    }

    if (flag_1cb != 0) {
        if (m_owned_054 == 0) {
            EnsurePathVisualization0045D530();
            node = reinterpret_cast<srNode*>(m_owned_054);
            node->setParent(world->dynamic_scene, 1);
            node->setFlag(srNode::FLAG_POSITIONAL_1);
            if (m_owned_054 == 0) {
                node->clearFlag(srNode::FLAG_POSITIONAL_0);
                return;
            }
        }

        srVector3T<float> adjusted = *source;
        g_octree_6598a4->AdjustPosition00431DA0(&adjusted, 1);
        DrawPathPosition0045C9A0(adjusted, 1);
    }

    reinterpret_cast<srNode*>(m_owned_054)->clearFlag(
        srNode::FLAG_POSITIONAL_0);
}

/* Select the editor color for one waypoint. Disabled surfaces are black; the
   two current selection slots take yellow and either green or red; every other
   surface is blue. */
// FUNCTION: WIZ8 0x0045d490
void W8PathingService::GetWaypointVisualizationColor0045D490(
    unsigned short waypoint,
    srVector3T<float>* color)
{
    if ((m_pSurfaces_048[waypoint].flags_00 & 0x20) != 0) {
        color->x = 0.0f;
        color->y = 0.0f;
        color->z = 0.0f;
        return;
    }
    if (waypoint == value_1d4) {
        color->x = 1.0f;
        color->y = 1.0f;
        color->z = 0.0f;
        return;
    }
    if (waypoint != value_1d6) {
        color->x = 0.0f;
        color->y = 0.0f;
        color->z = 1.0f;
        return;
    }
    if (m_positional_1da[0] != 0) {
        color->x = 0.0f;
        color->y = 1.0f;
        color->z = 0.0f;
        return;
    }
    color->x = 1.0f;
    color->y = 0.0f;
    color->z = 0.0f;
}

/* Append one waypoint surface and keep every capacity-coupled side table sized
   to the same hundred-record block.

   Surface zero is reserved on the first allocation. Growth replaces the three
   BitArrays rather than preserving their bits, and recreates the shared
   unsigned-short scratch run. The new surface receives its index and position,
   is classified for the path-surface flag, and is registered in the octree's
   spatial object index as kind nine. */
// FUNCTION: WIZ8 0x0045ddb0
void W8PathingService::AddWaypoint0045DDB0(
    const srVector3T<float>* position)
{
    if (m_ulNumSurfaces % 100 == 0) {
        unsigned int capacity = (m_ulNumSurfaces / 100 + 1) * 100;
        W8PathSurface* new_surfaces =
            static_cast<W8PathSurface*>(malloc(capacity * sizeof(W8PathSurface)));

        if (new_surfaces == 0) {
            srAssertFail("pNewWayPoints", OCTPATH_CPP, 0x12ec, 0);
        }
        memset(new_surfaces, 0, capacity * sizeof(W8PathSurface));
        if (m_ulNumSurfaces == 0) {
            m_ulNumSurfaces = 1;
        }
        else {
            memcpy(
                new_surfaces, m_pSurfaces_048,
                m_ulNumSurfaces * sizeof(W8PathSurface));
            free(m_pSurfaces_048);
        }
        m_pSurfaces_048 = new_surfaces;

        if (m_owned_058 != 0) {
            m_owned_058->FreeIndex();
            ::operator delete(m_owned_058);
        }
        m_owned_058 = new BitArray(capacity);
        if (m_owned_05c != 0) {
            m_owned_05c->FreeIndex();
            ::operator delete(m_owned_05c);
        }
        m_owned_05c = new BitArray(capacity);
        if (m_owned_060 != 0) {
            m_owned_060->FreeIndex();
            ::operator delete(m_owned_060);
        }
        m_owned_060 = new BitArray(capacity);

        free(g_path_scratch_00659c64);
        g_path_scratch_00659c64 = malloc(capacity * sizeof(unsigned short));
    }

    W8PathSurface* surface = &m_pSurfaces_048[m_ulNumSurfaces];
    int point[2];

    surface->flags_00 = 0x2000;
    surface->index_02 = (unsigned short)m_ulNumSurfaces;
    surface->position_04 = *position;
    if ((ClassifyWaypoint00459C00(&surface->position_04) & 0x04000000) != 0) {
        surface->flags_00 |= 0x40;
    }
    g_octree_6598a4->WorldPositionToCell00431440(position, point);
    g_octree_6598a4->object_registry->UpdateObjectCell00436B90(
        9, (unsigned short)m_ulNumSurfaces + 1, point);
    ++m_ulNumSurfaces;
}

/* Unlink and clear one edge record.

   The owning surface or predecessor edge is redirected to the removed edge's
   successor. The retail scans stop after the first owner is found, then clear
   the packed record and increment the service's free-record count. */
// FUNCTION: WIZ8 0x0045e360
void W8PathingService::RemoveWaypointLink0045E360(
    unsigned short edge_index)
{
    if (m_ulNumSurfaces > 2) {
        unsigned char found = 0;
        unsigned int index;
        for (index = 1; index < m_ulNumSurfaces && found == 0; ++index) {
            if (m_pSurfaces_048[index].first_edge_24 == edge_index) {
                m_pSurfaces_048[index].first_edge_24 =
                    m_pEdges_04c[edge_index].next_0c;
                found = 1;
            }
        }

        for (index = 1; index < m_ulNumEdges && found == 0; ++index) {
            if (m_pEdges_04c[index].next_0c == edge_index) {
                m_pEdges_04c[index].next_0c =
                    m_pEdges_04c[edge_index].next_0c;
                found = 1;
            }
        }

        memset(&m_pEdges_04c[edge_index], 0, sizeof(W8PathEdge));
        ++m_positional_018;
        MarkRendererReady();
        flag_1cc = 1;
    }
}

/* Add one directed edge to the waypoint graph, or update the matching edge
   when the source already owns it.

   Edge zero is the list sentinel and storage grows in hundred-record blocks.
   The cached length is computed before growth, the new record is appended to
   the source surface's chain, and the geometry-derived flag follows the same
   endpoint/span tests as an updated edge. */
// FUNCTION: WIZ8 0x0045ec30
void W8PathingService::AddWaypointLink0045EC30(
    unsigned short source,
    unsigned short destination,
    unsigned int flags)
{
    W8PathSurface* source_surface;
    W8PathSurface* destination_surface;
    float delta_x;
    float delta_y;
    float delta_z;
    float distance;
    W8PathEdge* edge;

    if (source == 0 || destination == 0 || source == destination) {
        Function58AAD0(
            0xf, "Cannot Link: Tried to link WayPt %d to WayPt %d. ",
            source, destination);
        return;
    }

    source_surface = &m_pSurfaces_048[source];
    destination_surface = &m_pSurfaces_048[destination];
    if ((source_surface->position_04.x == g_float_005ebb34 &&
         source_surface->position_04.y == g_float_005ebb34 &&
         source_surface->position_04.z == g_float_005ebb34) ||
        (destination_surface->position_04.x == g_float_005ebb34 &&
         destination_surface->position_04.y == g_float_005ebb34 &&
         destination_surface->position_04.z == g_float_005ebb34)) {
        Function58AAD0(
            0xf, "Cannot Link: WayPt %d is at (0, 0, 0). ", source);
        return;
    }

    if (UpdateWaypointLink0045F200(source, destination, flags) != 0) {
        return;
    }

    delta_x = source_surface->position_04.x - destination_surface->position_04.x;
    delta_y = source_surface->position_04.y - destination_surface->position_04.y;
    delta_z = source_surface->position_04.z - destination_surface->position_04.z;
    distance = (float)sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

    if (m_ulNumEdges % 100 == 0) {
        unsigned int capacity = m_ulNumEdges / 100 + 1;
        W8PathEdge* new_edges = static_cast<W8PathEdge*>(malloc(capacity * 100 * sizeof(W8PathEdge)));

        if (new_edges == 0) {
            srAssertFail("pNewWayPtLinks", OCTPATH_CPP, 0x1526, 0);
        }
        memset(new_edges, 0, capacity * 100 * sizeof(W8PathEdge));
        if (m_ulNumEdges == 0) {
            m_ulNumEdges = 1;
        }
        else {
            memcpy(new_edges, m_pEdges_04c, m_ulNumEdges * sizeof(W8PathEdge));
            free(m_pEdges_04c);
        }
        m_pEdges_04c = new_edges;
    }

    edge = &m_pEdges_04c[m_ulNumEdges];
    edge->flags_00 = flags;
    edge->destination_06 = destination;
    edge->source_04 = source;
    edge->distance_08 = distance;
    edge->next_0c = 0;

    if (source_surface->first_edge_24 == 0) {
        source_surface->first_edge_24 = (unsigned short)m_ulNumEdges;
    }
    else {
        unsigned short previous = source_surface->first_edge_24;

        while (m_pEdges_04c[previous].next_0c != 0) {
            previous = m_pEdges_04c[previous].next_0c;
        }
        m_pEdges_04c[previous].next_0c = (unsigned short)m_ulNumEdges;
    }

    if ((source_surface->flags_00 & 0x40) != 0 ||
        (destination_surface->flags_00 & 0x40) != 0 ||
        (TestWaypointSpan0045A1B0(
             &source_surface->position_04,
             &destination_surface->position_04, 0, 0),
         flag_23c != 0)) {
        edge->flags_00 |= 0x20000000;
    }
    ++m_ulNumEdges;
}

/* Decide whether a new directed edge would duplicate the graph already leading
   from source toward destination.

   A direct edge is an immediate hit. Otherwise only nearer first-hop neighbors
   matter: their horizontal direction is normalized and compared with the
   destination direction. A neighbor that already links to the destination uses
   the tighter alignment threshold; every nearer neighbor also receives the
   looser threshold test. Vertical displacement participates in the distance
   ordering but not in either direction comparison. */
// FUNCTION: WIZ8 0x0045ef90
unsigned char W8PathingService::HasDirectionalWaypointLink0045EF90(
    unsigned short source,
    unsigned short destination)
{
    W8PathSurface* source_surface = &m_pSurfaces_048[source];
    const W8PathSurface* destination_surface = &m_pSurfaces_048[destination];
    srVector3T<float> destination_direction;
    float destination_distance;
    float horizontal_length_squared;
    float scale;
    unsigned short edge_index;

    destination_direction.x = destination_surface->position_04.x - source_surface->position_04.x;
    destination_direction.y = destination_surface->position_04.y - source_surface->position_04.y;
    destination_direction.z = destination_surface->position_04.z - source_surface->position_04.z;
    destination_distance = (float)sqrt(
        destination_direction.x * destination_direction.x +
        destination_direction.y * destination_direction.y +
        destination_direction.z * destination_direction.z);
    horizontal_length_squared =
        destination_direction.x * destination_direction.x +
        destination_direction.z * destination_direction.z;
    destination_direction.y = 0.0f;
    if ((double)horizontal_length_squared != g_zero_005ebb40) {
        scale = (float)(g_double_005ebc30 / sqrt(horizontal_length_squared));
        destination_direction.x *= scale;
        destination_direction.z *= scale;
    }

    edge_index = source_surface->first_edge_24;
    while (edge_index != 0) {
        W8PathEdge* edge = &m_pEdges_04c[edge_index];
        unsigned short neighbor_index = edge->destination_06;
        W8PathSurface* neighbor = &m_pSurfaces_048[neighbor_index];
        srVector3T<float> neighbor_direction;
        float neighbor_distance;

        if (neighbor_index == destination) {
            return 1;
        }

        neighbor_direction.x = neighbor->position_04.x - source_surface->position_04.x;
        neighbor_direction.y = neighbor->position_04.y - source_surface->position_04.y;
        neighbor_direction.z = neighbor->position_04.z - source_surface->position_04.z;
        neighbor_distance = (float)sqrt(
            neighbor_direction.x * neighbor_direction.x +
            neighbor_direction.y * neighbor_direction.y +
            neighbor_direction.z * neighbor_direction.z);
        if (neighbor_distance < destination_distance) {
            unsigned short second_edge_index;

            horizontal_length_squared =
                neighbor_direction.x * neighbor_direction.x +
                neighbor_direction.z * neighbor_direction.z;
            neighbor_direction.y = 0.0f;
            if ((double)horizontal_length_squared != g_zero_005ebb40) {
                scale = (float)(g_double_005ebc30 / sqrt(horizontal_length_squared));
                neighbor_direction.x *= scale;
                neighbor_direction.z *= scale;
            }

            second_edge_index = neighbor->first_edge_24;
            while (second_edge_index != 0) {
                if (m_pEdges_04c[second_edge_index].destination_06 == destination) {
                    break;
                }
                second_edge_index = m_pEdges_04c[second_edge_index].next_0c;
            }
            if (second_edge_index != 0 &&
                Function4218E0(neighbor_direction, destination_direction) >
                    g_float_005ec390) {
                return 1;
            }
            if (Function4218E0(neighbor_direction, destination_direction) >
                g_float_005ec38c) {
                return 1;
            }
        }
        edge_index = edge->next_0c;
    }
    return 0;
}

/* Update an already-linked directed edge.

   The source surface owns the chain. A match receives the caller's new flags;
   the geometry-derived flag is also forced when either endpoint is a registered
   path surface or when the service's span test leaves its shared mode enabled.
   The image applies that derived bit through the next-edge slot rather than the
   matched index, so this preserves that observable retail behavior. */
// FUNCTION: WIZ8 0x0045f200
unsigned char W8PathingService::UpdateWaypointLink0045F200(
    unsigned short source,
    unsigned short destination,
    unsigned int flags)
{
    unsigned short edge_index = m_pSurfaces_048[source].first_edge_24;

    while (edge_index != 0) {
        W8PathEdge* edge = &m_pEdges_04c[edge_index];

        if (edge->destination_06 == destination) {
            edge->flags_00 = flags;
            if ((m_pSurfaces_048[source].flags_00 & 0x40) != 0 ||
                (m_pSurfaces_048[destination].flags_00 & 0x40) != 0 ||
                (TestWaypointSpan0045A1B0(
                     &m_pSurfaces_048[source].position_04,
                     &m_pSurfaces_048[destination].position_04, 0, 0),
                 flag_23c != 0)) {
                m_pEdges_04c[m_ulNumEdges].flags_00 |= 0x20000000;
            }
            return 1;
        }
        edge_index = edge->next_0c;
    }
    return 0;
}

/* Edit the directed path edge joining a teleportal's two settled endpoints.

   An endpoint only counts as an existing teleportal waypoint when the lookup
   lands on a flagged surface within the shared snap distance. Missing ends are
   inserted with their respective inbound/outbound defaults. When both ends
   already existed, the first end's edge chain is searched so the dialog edits
   the current flags rather than starting from zero. The resulting edge is
   forced dynamic, its cached distance is invalidated, and both the renderer
   and path-edit state are marked dirty. */
// FUNCTION: WIZ8 0x0045f2d0
void W8PathingService::EditTeleportalLink(
    const srVector3T<float>* destination,
    const srVector3T<float>* source)
{
    char title[80];
    unsigned int link_flags[2];
    unsigned short destination_index;
    unsigned short source_index;
    unsigned short edge_index;
    unsigned char both_existing = 1;

    if (flag_1c8 == 0) {
        return;
    }

    destination_index = FindWaypoint0045B120(destination, 0);
    if ((m_pSurfaces_048[destination_index].flags_00 & 2) == 0 ||
        sqrt(
            (m_pSurfaces_048[destination_index].position_04.z - destination->z) *
                (m_pSurfaces_048[destination_index].position_04.z - destination->z) +
            (m_pSurfaces_048[destination_index].position_04.y - destination->y) *
                (m_pSurfaces_048[destination_index].position_04.y - destination->y) +
            (m_pSurfaces_048[destination_index].position_04.x - destination->x) *
                (m_pSurfaces_048[destination_index].position_04.x - destination->x)) >
            g_path_waypoint_snap_distance_005ec150) {
        destination_index = 0;
    }

    source_index = FindWaypoint0045B120(source, 0);
    if ((m_pSurfaces_048[source_index].flags_00 & 2) == 0 ||
        sqrt(
            (m_pSurfaces_048[source_index].position_04.z - source->z) *
                (m_pSurfaces_048[source_index].position_04.z - source->z) +
            (m_pSurfaces_048[source_index].position_04.y - source->y) *
                (m_pSurfaces_048[source_index].position_04.y - source->y) +
            (m_pSurfaces_048[source_index].position_04.x - source->x) *
                (m_pSurfaces_048[source_index].position_04.x - source->x)) >
            g_path_waypoint_snap_distance_005ec150) {
        source_index = 0;
    }

    if (destination_index == 0) {
        destination_index = (unsigned short)m_ulNumSurfaces;
        AddWaypoint0045DDB0(destination);
        m_pSurfaces_048[destination_index].flags_00 = 2;
        SetWaypointLinkFlags0045E030(destination_index, 6);
        both_existing = 0;
    }
    if (source_index == 0) {
        source_index = (unsigned short)m_ulNumSurfaces;
        AddWaypoint0045DDB0(source);
        m_pSurfaces_048[source_index].flags_00 = 2;
        SetWaypointLinkFlags0045E030(source_index, 5);
        both_existing = 0;
    }

    edge_index = 0;
    link_flags[0] = 0;
    link_flags[1] = 0;
    if (both_existing != 0) {
        edge_index = m_pSurfaces_048[destination_index].first_edge_24;
        while (edge_index != 0) {
            if (m_pEdges_04c[edge_index].destination_06 == source_index) {
                link_flags[0] = m_pEdges_04c[edge_index].flags_00;
                break;
            }
            edge_index = m_pEdges_04c[edge_index].next_0c;
        }
    }

    if (edge_index != 0) {
        sprintf(title, "EDIT FLAGS FOR EXISTING LINK BETWEEN TELEPORTAL WAYPOINTS: ");
    }
    else {
        sprintf(title, "EDIT FLAGS FOR NEW LINK BETWEEN TELEPORTAL WAYPOINTS: ");
    }
    EditWaypointLinkFlags0045F530(title, link_flags, 5);

    if (edge_index == 0) {
        edge_index = (unsigned short)m_ulNumEdges;
        AddWaypointLink0045EC30(destination_index, source_index, link_flags[0]);
    }
    else {
        m_pEdges_04c[edge_index].flags_00 = link_flags[0];
    }
    m_pEdges_04c[edge_index].flags_00 |= 0x01000000;
    m_pEdges_04c[edge_index].distance_08 = 0.0f;
    MarkRendererReady();
    flag_1cc = 1;
}

/* Look one named path up, and report the region and height range it spans.

   The table holds fixed 0x44-byte entries whose name is the entry itself and
   whose handle sits at +0x40. A match walks the path's two chained tables to
   take the extent of every node it touches: the region ids straight out of the
   low and high halves of each entry, and the height from the entry's low half
   scaled by the service's own span and lifted by the bounds floor. */
// FUNCTION: WIZ8 0x00457cf0
unsigned int W8PathingService::FindPathHandle(
    const unsigned char* path_name,
    unsigned short* path_bounds,
    float* path_range)
{
    const unsigned char* entry;
    unsigned int index;
    unsigned short value;
    float height;
    int outer;
    int inner;
    int outer_table;
    int inner_table;

    if (path_name == 0 || *path_name == 0 || m_pCondPaths == 0 || m_ulNumCondPaths == 0) {
        return 0;
    }
    index = 0;
    entry = m_pCondPaths;
    do {
        if (strcmp(reinterpret_cast<const char*>(path_name),
                   reinterpret_cast<const char*>(entry)) == 0) {
            path_bounds[2] = 0xffff;
            path_bounds[0] = 0xffff;
            path_bounds[3] = 0;
            path_bounds[1] = 0;
            path_range[0] = 1e+08f;
            path_range[2] = -1e+08f;
            outer = *reinterpret_cast<int*>(m_pCondPaths + index * 0x44 + 0x40) * 4;
            outer_table = reinterpret_cast<int>(m_pCondLookup);
            for (inner = *reinterpret_cast<int*>(outer_table + outer); inner != 0;
                 inner = *reinterpret_cast<int*>(inner + outer_table)) {
                inner = *reinterpret_cast<int*>(outer + outer_table) * 4;
                inner_table = reinterpret_cast<int>(m_pCondKeys);
                for (int node = *reinterpret_cast<int*>(inner_table + inner); node != 0;
                     node = *reinterpret_cast<int*>(node + inner_table)) {
                    value = *reinterpret_cast<unsigned short*>(inner + inner_table);
                    if (value < path_bounds[0]) {
                        path_bounds[0] = value;
                    }
                    if (path_bounds[1] < value) {
                        path_bounds[1] = value;
                    }
                    value = (unsigned short)
                        (*reinterpret_cast<unsigned int*>(inner + inner_table) >> 0x10);
                    if (value < path_bounds[2]) {
                        path_bounds[2] = value;
                    }
                    if (path_bounds[3] < value) {
                        path_bounds[3] = value;
                    }
                    height = (float)(*reinterpret_cast<unsigned int*>(
                                         inner + reinterpret_cast<int>(m_pCondValues)) & 0xffff) *
                            span_020 + level_bounds[1];
                    if (height < path_range[0]) {
                        path_range[0] = height;
                    }
                    if (path_range[2] < height) {
                        path_range[2] = height;
                    }
                    inner_table = reinterpret_cast<int>(m_pCondKeys);
                    inner += 4;
                }
                outer_table = reinterpret_cast<int>(m_pCondLookup);
                outer += 4;
            }
            return *reinterpret_cast<unsigned int*>(m_pCondPaths + index * 0x44 + 0x40);
        }
        ++index;
        entry += 0x44;
    } while (index < (unsigned int)m_ulNumCondPaths);
    return 0;
}


/* The state object's whole constructor: it defers to the one at 0x004CCCB0 and
   adds nothing of its own, which is why it has no members to clear here. */
// FUNCTION: WIZ8 0x004cae40
W8PathState004CAE40::W8PathState004CAE40()
{
    ConstructPathState004CCCB0(this);
}
