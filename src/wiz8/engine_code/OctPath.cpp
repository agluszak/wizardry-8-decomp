#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/regions.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"
#include "surrender/srNode.h"
#include "surrender/srModelInstance.h"
#include "FileMan.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GDFileIO.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/PolyPick.h"
#include "random.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GLOBAL: WIZ8 0x005ebc30
double g_double_005ebc30 = 1.0;
// GLOBAL: WIZ8 0x005ec020
const float g_float_005ec020 = 0.0f;
// GLOBAL: WIZ8 0x005ec368
double g_double_005ec368 = 25.00000037252903;
// GLOBAL: WIZ8 0x005ec378
double g_double_005ec378 = 4.0;
// GLOBAL: WIZ8 0x005ec38c
float g_float_005ec38c = 0.9847999811172485f;
// GLOBAL: WIZ8 0x005ec384
float g_float_005ec384 = 37500.0f;
// GLOBAL: WIZ8 0x005ec370
float g_float_005ec370 = 550.0f;
// GLOBAL: WIZ8 0x005ec394
float g_float_005ec394 = 1.2000000476837158f;
// GLOBAL: WIZ8 0x005ec39c
const float g_float_005ec39c = 250000.0f;
// GLOBAL: WIZ8 0x005ec3b8
float g_float_005ec3b8 = 1.5f;
// GLOBAL: WIZ8 0x005ec3bc
float g_float_005ec3bc = 0.5099999904632568f;
// GLOBAL: WIZ8 0x005ec3c0
float g_float_005ec3c0 = 1000000.0f;
// GLOBAL: WIZ8 0x005ec3c8
float g_float_005ec3c8 = -107374184.0f;
// GLOBAL: WIZ8 0x005ec3d0
float g_float_005ec3d0 = -107374184.0f;
// GLOBAL: WIZ8 0x005ed2e0
const double g_double_005ed2e0 = 0.2;
// GLOBAL: WIZ8 0x005ed2e8
const float g_float_005ed2e8 = 0.3333333f;
// GLOBAL: WIZ8 0x005ec2e8
const double g_double_005ec2e8 = -1.0;

// GLOBAL: WIZ8 0x00659c60
W8PathingService* g_pathing_00659c60;

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

// GLOBAL: WIZ8 0x0060827a
unsigned short g_path_reserve_0060827a = 2000;
// GLOBAL: WIZ8 0x005ec340
const float g_float_005ec340 = 1.25f;
// GLOBAL: WIZ8 0x005ec344
float g_path_span_scale_005ec344 = 1.5259254723787308e-05f;

// GLOBAL: WIZ8 0x00659c5c
unsigned char g_flag_00659c5c;
// GLOBAL: WIZ8 0x00659c64
void* g_path_scratch_00659c64;
// GLOBAL: WIZ8 0x005ec3a8
double g_double_005ec3a8 = 1.1;
// GLOBAL: WIZ8 0x005ec3a0
double g_double_005ec3a0 = 25000.0;
// GLOBAL: WIZ8 0x005ec3b0
double g_double_005ec3b0 = 0.1;
// GLOBAL: WIZ8 0x005ec348
float g_path_direction_threshold_0_005ec348 = -0.9239000082015991f;
// GLOBAL: WIZ8 0x005ec34c
float g_path_direction_threshold_1_005ec34c = -0.38269999623298645f;
// GLOBAL: WIZ8 0x005ec350
float g_path_direction_threshold_2_005ec350 = 0.38269999623298645f;
// GLOBAL: WIZ8 0x005ec354
float g_path_direction_threshold_3_005ec354 = 0.9239000082015991f;
// GLOBAL: WIZ8 0x005ec358
float g_path_cardinal_scale_005ec358 = 1.4149999618530273f;

// GLOBAL: WIZ8 0x005ec360
float g_float_005ec360 = 25000.0f;
// GLOBAL: WIZ8 0x00652dc4
srShader g_path_shader_00652dc4;
// GLOBAL: WIZ8 0x00652dc0
srTextureIFace* g_path_texture_00652dc0;
// GLOBAL: WIZ8 0x00652dbc
srMaterialIFace* g_path_material_00652dbc;
// GLOBAL: WIZ8 0x00659c6c
unsigned int g_path_visualization_cell_00659c6c;
// GLOBAL: WIZ8 0x005ec380
float g_path_search_visualization_limit_005ec380 = 15000.0f;

// GLOBAL: WIZ8 0x0060f9e8
float g_path_acceleration_factor_0060f9e8 = 3.0f;
// GLOBAL: WIZ8 0x0060f9ec
float g_path_angular_acceleration_factor_0060f9ec = 2.0f;
// GLOBAL: WIZ8 0x0060f9f0
float g_path_angular_deceleration_factor_0060f9f0 = 0.5f;
// GLOBAL: WIZ8 0x0060f9f4
float g_path_prediction_time_0060f9f4 = 1.0f;
// GLOBAL: WIZ8 0x0060f9f8
float g_path_approach_slow_time_0060f9f8 = 1.0f;
// GLOBAL: WIZ8 0x0060f9fc
float g_path_approach_run_time_0060f9fc = 8.0f;
// GLOBAL: WIZ8 0x0060fa00
float g_path_approach_run_rate_0060fa00 = 2.0f;
// GLOBAL: WIZ8 0x0060fa04
float g_path_lookahead_time_0060fa04 = 1.0f;
// GLOBAL: WIZ8 0x0060fa08
float g_path_group_repulsion_factor_0060fa08 = 1.0f;
// GLOBAL: WIZ8 0x0060fa0c
float g_path_party_boundary_radius_0060fa0c = 2.0f;
// GLOBAL: WIZ8 0x0060fa10
float g_path_obstacle_steering_factor_0060fa10 = 2.0f;
// GLOBAL: WIZ8 0x0060fa14
float g_path_obstacle_braking_factor_0060fa14 = 0.3f;

#define OCTPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPath.cpp"

static unsigned char LoadPathParameters004CCCB0();

/* The path-search heap specialization is emitted after OctPath.cpp's ordinary
   bodies. The generic definitions live once in stHeap.hpp. */
// TEMPLATE: WIZ8 0x004675B0
// stHeap<W8PathHeapEntry>::Insert004675B0

// TEMPLATE: WIZ8 0x00467910
// stHeap<W8PathHeapEntry>::SiftDown00467910

// TEMPLATE: WIZ8 0x00467990
// stHeap<W8PathHeapEntry>::SiftUp00467990

/* Advance the search queue and mark the node that was just expanded. The
   generic heap delete is visible here as the assertion and sift-down sequence
   in the retail body; an empty queue publishes node zero. */
// FUNCTION: WIZ8 0x004577F0
void W8PathHeapHandle::DeleteRoot004577F0(W8PathSearchNode* node)
{
    if (heap_00->size_0c == 0) {
        root_node_04 = 0;
    } else {
        root_node_04 = heap_00->Delete().node_00;
    }
    node->flags_00 |= 4;
}

/* Write the path hash serialization and its five conditional tables. Counts
   smaller than the two sentinel entries are normalized to an empty set before
   the header is emitted, exactly as the read side treats them. */
// FUNCTION: WIZ8 0x00458ad0
unsigned char W8PathingService::WritePathNodes00458AD0(unsigned int handle)
{
    unsigned int block[4];
    unsigned char success;

    if (size_004 != 0) {
        success = FileWrite(handle, path_nodes_044, size_004 << 3, 0);
        if (success == 0) {
            ReportBuildStatus00497690(7, "WritePathNodes: Couldn't write Path Hash array.\n");
            return 0;
        }
    }

    if (static_cast<unsigned int>(m_ulNumCondFrames) < 2 ||
        static_cast<unsigned int>(m_ulNumCondNodes) < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondFrames = 0;
        m_ulNumCondNodes = 0;
    }
    block[0] = m_ulNumCondPaths;
    block[1] = m_ulNumCondFrames;
    block[2] = m_ulNumCondNodes;
    block[3] = 0;
    success = FileWrite(handle, block, sizeof(block), 0);
    if (success == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8b2,
                     "WritePathNodes: Couldn't write Conditional Counts.\n");
    }

    if (static_cast<unsigned int>(m_ulNumCondFrames) > 1 &&
        static_cast<unsigned int>(m_ulNumCondNodes) > 1) {
        success = FileWrite(handle, m_pCondPaths, m_ulNumCondPaths * sizeof(GDPropCondPaths), 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8b7,
                         "WritePathNodes: Couldn't write Conditional Prop array.\n");
        }
        success = FileWrite(handle, m_pulCondLookup, m_ulNumCondFrames << 2, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8b9,
                         "WritePathNodes: Couldn't write Conditional Lookup array.\n");
        }
        success = FileWrite(handle, m_pusCondNodeFrames, m_ulNumCondFrames << 1, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8bb,
                         "WritePathNodes: Couldn't write Conditional Frame array.\n");
        }
        success = FileWrite(handle, m_pulCondNodeKeys, m_ulNumCondNodes << 2, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8bd,
                         "WritePathNodes: Couldn't write Conditional Key array.\n");
        }
        success = FileWrite(handle, m_pulCondNodeValues, m_ulNumCondNodes << 2, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8bf,
                         "WritePathNodes: Couldn't write Conditional Value array.\n");
        }
    }
    return 1;
}

/* Snapshot live path surfaces into the compact waypoint-file representation,
   clearing runtime-only disabled and edge bits before writing the .WPT file.
   The cd-rom sentinel disables this editor-side write path. */
// FUNCTION: WIZ8 0x00459400
unsigned char W8PathingService::SaveWaypointSnapshot00459400(unsigned char force)
{
    if (flag_1cc == 0 && force == 0) {
        return 0;
    }
    if (FileExists("cd-rom") != 0) {
        return 0;
    }

    BuildWaypointFileData0045E440();
    if (m_pFileWayPoints != 0) {
        free(m_pFileWayPoints);
    }
    m_pFileWayPoints =
        static_cast<W8FileWaypoint*>(malloc(m_ulNumWayPoints * sizeof(W8FileWaypoint)));
    if (m_pFileWayPoints == 0) {
        srAssertFail("m_pFileWayPoints", OCTPATH_CPP, 0x968, 0);
    }

    unsigned int index;
    for (index = 0; index < m_ulNumWayPoints; ++index) {
        m_pFileWayPoints[index].flags_00 = m_pSurfaces_048[index].flags_00 & 0xffdf;
        m_pFileWayPoints[index].first_edge_02 = m_pSurfaces_048[index].first_edge_24;
        m_pFileWayPoints[index].position_04 = m_pSurfaces_048[index].position_04;
    }
    for (index = 0; index < m_ulNumWayPtLinks; ++index) {
        m_pEdges_04c[index].flags_00 &= 0x7fffffff;
    }

    unsigned char result = WriteWaypointFile00459540();
    free(m_pFileWayPoints);
    m_pFileWayPoints = 0;
    return result;
}

/* Write the version-two waypoint snapshot. Retail combines the six write
   results with OR, so a partially successful sequence still reports success;
   that behavior is part of the recovered format contract. */
// FUNCTION: WIZ8 0x00459540
unsigned char W8PathingService::WriteWaypointFile00459540()
{
    unsigned int version = 2;
    unsigned char result = 0;
    char path[256];
    sprintf(path, "%s.WPT", level_name);

    if (m_ulNumWayPoints > 0xffff) {
        return 0;
    }
    unsigned int handle = FileOpen(path, 2, 0);
    if (handle == 0) {
        return 0;
    }
    if (m_ulNumWayPoints != 0 && m_pFileWayPoints != 0) {
        result = FileWrite(handle, &version, sizeof(version), 0) |
                 FileWrite(handle, &edge_node_count_008, sizeof(edge_node_count_008), 0) |
                 FileWrite(handle, &m_ulNumWayPoints, sizeof(m_ulNumWayPoints), 0) |
                 FileWrite(handle, &m_ulNumWayPtLinks, sizeof(m_ulNumWayPtLinks), 0) |
                 FileWrite(handle, m_pFileWayPoints, m_ulNumWayPoints * sizeof(W8FileWaypoint), 0) |
                 FileWrite(handle, m_pEdges_04c, m_ulNumWayPtLinks * sizeof(W8PathEdge), 0);
    }
    FileClose(handle);
    return result;
}

/* Read a versioned .WPT snapshot and rebuild its live graph representation.
   Version one stores the four persistent edge fields separately; later files
   contain the complete packed 0x0e-byte edge record. */
// FUNCTION: WIZ8 0x00459650
unsigned char W8PathingService::ReadWaypointFile00459650()
{
    unsigned int version = 2;
    unsigned char success = 0;
    char path[256];
    if (size_004 == 0)
        return 0;
    sprintf(path, "%s.WPT", level_name);
    unsigned int handle = FileOpen(path, 1, 0);
    if (handle == 0)
        return 0;

    success = FileRead(handle, &version, 4, 0) | FileRead(handle, &edge_node_count_008, 4, 0) |
              FileRead(handle, &m_ulNumWayPoints, 4, 0) |
              FileRead(handle, &m_ulNumWayPtLinks, 4, 0);
    if (success == 0) {
        FileClose(handle);
        return 0;
    }
    unsigned int surface_capacity = (m_ulNumWayPoints / 100 + 1) * 100;
    unsigned int edge_capacity = (m_ulNumWayPtLinks / 100 + 1) * 100;
    m_pFileWayPoints =
        static_cast<W8FileWaypoint*>(malloc(m_ulNumWayPoints * sizeof(W8FileWaypoint)));
    m_pSurfaces_048 = static_cast<W8PathSurface*>(malloc(surface_capacity * sizeof(W8PathSurface)));
    m_pEdges_04c = static_cast<W8PathEdge*>(malloc(edge_capacity * sizeof(W8PathEdge)));
    if (m_pSurfaces_048 != 0)
        memset(m_pSurfaces_048, 0, surface_capacity * sizeof(W8PathSurface));
    if (m_pEdges_04c != 0)
        memset(m_pEdges_04c, 0, edge_capacity * sizeof(W8PathEdge));
    if (m_pFileWayPoints == 0 || m_pSurfaces_048 == 0 || m_pEdges_04c == 0) {
        if (m_pFileWayPoints != 0)
            free(m_pFileWayPoints);
        if (m_pSurfaces_048 != 0)
            free(m_pSurfaces_048);
        if (m_pEdges_04c != 0)
            free(m_pEdges_04c);
        FileClose(handle);
        return 0;
    }

    success = FileRead(handle, m_pFileWayPoints, m_ulNumWayPoints * sizeof(W8FileWaypoint), 0);
    if (success != 0) {
        if (version == 1) {
            for (unsigned int edge = 0; edge < m_ulNumWayPtLinks; ++edge) {
                W8PathEdge* item = &m_pEdges_04c[edge];
                success &= FileRead(handle, &item->flags_00, 4, 0);
                success &= FileRead(handle, &item->destination_06, 2, 0);
                success &= FileRead(handle, &item->distance_08, 4, 0);
                success &= FileRead(handle, &item->next_0c, 2, 0);
                item->source_04 = 0;
            }
        } else {
            success &= FileRead(handle, m_pEdges_04c, m_ulNumWayPtLinks * sizeof(W8PathEdge), 0);
        }
    }
    if (success == 0) {
        free(m_pFileWayPoints);
        free(m_pSurfaces_048);
        free(m_pEdges_04c);
        FileClose(handle);
        return 0;
    }

    delete path_heap_06c;
    unsigned int heap_capacity = surface_capacity;
    if (heap_capacity <= g_path_reserve_0060827a)
        heap_capacity = g_path_reserve_0060827a;
    path_heap_06c = new W8PathHeapHandle;
    path_heap_06c->heap_00 = new W8PathHeap;
    path_heap_06c->heap_00->entries_00 = new W8PathHeapEntry[heap_capacity];
    if (path_heap_06c->heap_00->entries_00 == 0)
        srAssertFail("hlist", "..\\Engine Code\\Include\\stHeap.hpp", 0x79, 0);
    path_heap_06c->heap_00->external_storage_04 = 0;
    path_heap_06c->heap_00->capacity_08 = heap_capacity;
    path_heap_06c->heap_00->size_0c = 0;

    memset(m_pSurfaces_048, 0, m_ulNumWayPoints * sizeof(W8PathSurface));
    for (unsigned int surface = 1; surface < m_ulNumWayPoints; ++surface) {
        W8FileWaypoint* source = &m_pFileWayPoints[surface];
        W8PathSurface* destination = &m_pSurfaces_048[surface];
        destination->flags_00 = source->flags_00;
        destination->index_02 = static_cast<unsigned short>(surface);
        destination->first_edge_24 = source->first_edge_02;
        destination->position_04 = source->position_04;
        int point[3];
        g_octree_6598a4->WorldPositionToCell(&destination->position_04, point);
        g_octree_6598a4->object_registry->MoveObjectToCell(W8_OCTREE_KIND_WAYPOINT, surface + 1,
                                                           point);
        if ((destination->flags_00 & 0xf000) == 0)
            destination->flags_00 |= 0x2000;
    }
    visible_waypoints_058->SetSize(surface_capacity);
    rendered_waypoints_05c->SetSize(surface_capacity);
    collected_waypoints_060->SetSize(surface_capacity);
    FileClose(handle);
    g_path_scratch_00659c64 = malloc(surface_capacity * sizeof(unsigned short));

    if (version <= 2) {
        unsigned int surface;
        for (surface = 1; surface < m_ulNumWayPoints; ++surface) {
            if ((ClassifyWaypoint00459C00(&m_pSurfaces_048[surface].position_04) & 0x04000000) != 0)
                m_pSurfaces_048[surface].flags_00 |= 0x40;
        }
        for (surface = 1; surface < m_ulNumWayPoints; ++surface) {
            unsigned short edge_index = m_pSurfaces_048[surface].first_edge_24;
            while (edge_index != 0) {
                W8PathEdge* edge = &m_pEdges_04c[edge_index];
                edge->source_04 = static_cast<unsigned short>(surface);
                unsigned int destination_index = edge->destination_06;
                if ((m_pSurfaces_048[surface].flags_00 & 0x40) != 0 ||
                    (m_pSurfaces_048[destination_index].flags_00 & 0x40) != 0) {
                    edge->flags_00 |= 0x20000000;
                } else {
                    TestWaypointSpan0045A1B0(&m_pSurfaces_048[surface].position_04,
                                             &m_pSurfaces_048[destination_index].position_04, 0, 0);
                    if (flag_23c != 0)
                        edge->flags_00 |= 0x20000000;
                }
                edge_index = edge->next_0c;
            }
        }
    }
    flag_1cc = 1;
    return success;
}

/* Compact the editable waypoint graph. Invalid and dead-end surfaces are
   unregistered and discarded; every surviving surface and edge is packed
   toward its sentinel and all indices are rewritten through temporary maps. */
// FUNCTION: WIZ8 0x0045e440
void W8PathingService::BuildWaypointFileData0045E440()
{
    unsigned short next_surface = 1;
    unsigned short next_edge = 1;
    unsigned short* surface_map =
        static_cast<unsigned short*>(malloc((m_ulNumWayPoints + 1) * sizeof(unsigned short)));
    memset(surface_map, 0, (m_ulNumWayPoints + 1) * sizeof(unsigned short));
    unsigned short* edge_map =
        static_cast<unsigned short*>(malloc((m_ulNumWayPtLinks + 1) * sizeof(unsigned short)));
    memset(edge_map, 0, (m_ulNumWayPtLinks + 1) * sizeof(unsigned short));

    unsigned int old_surface;
    for (old_surface = 1; old_surface < m_ulNumWayPoints; ++old_surface) {
        W8PathSurface* surface = &m_pSurfaces_048[old_surface];
        if (surface->index_02 == 0 || surface->first_edge_24 == 0) {
            g_octree_6598a4->object_registry->UnregisterObject(W8_OCTREE_KIND_WAYPOINT,
                                                               old_surface + 1);
            if (surface->first_edge_24 == 0) {
                unsigned short removed = 0;
                unsigned int edge;
                for (edge = 1; edge < m_ulNumWayPtLinks; ++edge) {
                    if (m_pEdges_04c[edge].destination_06 == old_surface) {
                        RemoveWaypointLink0045E360(static_cast<unsigned short>(edge));
                        ++removed;
                    }
                }
                if (g_flag_689b32 != 0) {
                    const char* message = removed == 0 ? "Deleting Isolated WayPt at:  %1f, %1f"
                                                       : "Deleting Dead End WayPt at:  %1f, %1f";
                    FormatDebugMessage(0, message, surface->position_04.x, surface->position_04.y);
                }
            }
        } else {
            surface_map[old_surface] = next_surface;
            if (old_surface != next_surface) {
                m_pSurfaces_048[next_surface] = *surface;
                m_pSurfaces_048[next_surface].index_02 = next_surface;
                g_octree_6598a4->object_registry->UnregisterObject(W8_OCTREE_KIND_WAYPOINT,
                                                                   old_surface + 1);
                int point[3];
                g_octree_6598a4->WorldPositionToCell(&m_pSurfaces_048[next_surface].position_04,
                                                     point);
                g_octree_6598a4->object_registry->MoveObjectToCell(W8_OCTREE_KIND_WAYPOINT,
                                                                   next_surface + 1, point);
            }
            ++next_surface;
        }
    }

    unsigned int old_edge;
    for (old_edge = 1; old_edge < m_ulNumWayPtLinks; ++old_edge) {
        W8PathEdge* edge = &m_pEdges_04c[old_edge];
        if (edge->destination_06 != 0) {
            edge->destination_06 = surface_map[edge->destination_06];
            edge->source_04 = surface_map[edge->source_04];
            edge_map[old_edge] = next_edge;
            if (old_edge != next_edge) {
                m_pEdges_04c[next_edge] = *edge;
            }
            ++next_edge;
        }
    }

    memset(&m_pEdges_04c[next_edge], 0, (m_ulNumWayPtLinks - next_edge) * sizeof(W8PathEdge));
    memset(&m_pSurfaces_048[next_surface], 0,
           (m_ulNumWayPoints - next_surface) * sizeof(W8PathSurface));
    m_ulNumWayPoints = next_surface;
    m_ulNumWayPtLinks = next_edge;

    for (old_surface = 1; old_surface < m_ulNumWayPoints; ++old_surface) {
        m_pSurfaces_048[old_surface].first_edge_24 =
            edge_map[m_pSurfaces_048[old_surface].first_edge_24];
    }
    for (old_edge = 1; old_edge < m_ulNumWayPtLinks; ++old_edge) {
        m_pEdges_04c[old_edge].next_0c = edge_map[m_pEdges_04c[old_edge].next_0c];
    }
    value_1d8 = surface_map[value_1d8];
    free(surface_map);
    free(edge_map);
}

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
        m_pPathValues_064 = new W8HashTable<unsigned int, unsigned int>;
        m_pVisitedCells_074 = new W8OctreeIndex;
        buffer = static_cast<unsigned int*>(malloc(size_004 * 8));
        if (m_pPathValues_064 == 0 || buffer == 0) {
            strcpy(acMessage, "ReadPathNodes: Couldn't allocate path hash array.");
        } else {
            fSuccess = FileRead(handle, buffer, size_004 * 8, &uiRead);
            if (fSuccess == 0) {
                strcpy(acMessage, "ReadPathNodes: Couldn't read path hash array.");
                free(buffer);
            } else {
                scan = buffer;
                for (index = 0; index < static_cast<unsigned int>(size_004); ++index) {
                    m_pPathValues_064->Insert(&scan[0], &scan[1]);
                    scan += 2;
                }
                free(buffer);
            }
        }
    }
    fSuccess = FileRead(handle, block, 0x10, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8fa,
                     "ReadPathNodes: Couldn't write Conditional Counts.\n");
    }
    m_ulNumCondPaths = block[0];
    m_ulNumCondFrames = block[1];
    m_ulNumCondNodes = block[2];
    path_flags_000 = block[3];
    if (m_ulNumCondFrames < 2 || m_ulNumCondNodes < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondFrames = 0;
        m_ulNumCondNodes = 0;
        return fSuccess;
    }
    m_pCondPaths = static_cast<GDPropCondPaths*>(malloc(block[0] * sizeof(GDPropCondPaths)));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x903,
                     "ReadPathNodes: Couldn't allocate Conditional Prop array.\n");
    }
    m_pulCondLookup = static_cast<unsigned int*>(malloc(m_ulNumCondFrames << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x905,
                     "ReadPathNodes: Couldn't allocate Conditional Lookup array.\n");
    }
    m_pusCondNodeFrames = static_cast<unsigned short*>(malloc(m_ulNumCondFrames << 1));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x907,
                     "ReadPathNodes: Couldn't allocate Conditional Frame array.\n");
    }
    m_pulCondNodeKeys = static_cast<unsigned int*>(malloc(m_ulNumCondNodes << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x909,
                     "ReadPathNodes: Couldn't allocate Conditional Key array.\n");
    }
    m_pulCondNodeValues = static_cast<unsigned int*>(malloc(m_ulNumCondNodes << 2));
    if (m_pCondPaths == 0) {
        srAssertFail("m_pCondPaths", OCTPATH_CPP, 0x90b,
                     "ReadPathNodes: Couldn't allocate Conditional Value array.\n");
    }
    fSuccess = FileRead(handle, m_pCondPaths, m_ulNumCondPaths * sizeof(GDPropCondPaths), &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x90e,
                     "ReadPathNodes: Couldn't write Conditional Prop array.\n");
    }
    fSuccess = FileRead(handle, m_pulCondLookup, m_ulNumCondFrames << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x910,
                     "ReadPathNodes: Couldn't write Conditional Lookup array.\n");
    }
    fSuccess = FileRead(handle, m_pusCondNodeFrames, m_ulNumCondFrames << 1, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x912,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = FileRead(handle, m_pulCondNodeKeys, m_ulNumCondNodes << 2, &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x914,
                     "ReadPathNodes: Couldn't write Conditional Frame array.\n");
    }
    fSuccess = FileRead(handle, m_pulCondNodeValues, m_ulNumCondNodes << 2, &uiRead);
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
void W8PathingService::LinkSurfaces00460020(GDProp* prop)
{
    unsigned int index = 1;
    srVector2i point;
    W8PathSurface* surface;
    int converted;

    if (m_ulNumWayPoints <= index) {
        return;
    }
    do {
        surface = &m_pSurfaces_048[index];
        if ((surface->flags_00 & 0x40) != 0) {
            converted =
                static_cast<int>((surface->position_04.z - level_bounds[2]) / grid_scale_01c);
            point.x = static_cast<int>((surface->position_04.x - level_bounds[0]) / grid_scale_01c);
            point.y = converted;
            prop->RegisterPathSurface004B7730(index, &point);
        }
        ++index;
    } while (index < m_ulNumWayPoints);
}

/* The same for the edges, which are 0xe bytes apart, gated by a different flag,
   and which name two surfaces by index in their shorts at +4 and +6. Each of
   those surfaces contributes one converted point, so the builder receives the
   edge as a pair. */
// FUNCTION: WIZ8 0x004600b0
void W8PathingService::LinkEdges004600B0(GDProp* prop)
{
    unsigned int index = 1;
    srVector2i first;
    srVector2i second;
    W8PathEdge* edge;
    W8PathSurface* surface;
    int converted;

    if (m_ulNumWayPtLinks <= index) {
        return;
    }
    do {
        edge = &m_pEdges_04c[index];
        if ((edge->flags_00 & 0x20000000) != 0) {
            surface = &m_pSurfaces_048[edge->source_04];
            converted =
                static_cast<int>((surface->position_04.z - level_bounds[2]) / grid_scale_01c);
            first.x = static_cast<int>((surface->position_04.x - level_bounds[0]) / grid_scale_01c);
            first.y = converted;

            surface = &m_pSurfaces_048[edge->destination_06];
            converted =
                static_cast<int>((surface->position_04.z - level_bounds[2]) / grid_scale_01c);
            second.x =
                static_cast<int>((surface->position_04.x - level_bounds[0]) / grid_scale_01c);
            second.y = converted;
            prop->RegisterPathVertex004B7830(index, &first, &second);
        }
        ++index;
    } while (index < m_ulNumWayPtLinks);
}

/* Apply one GD prop frame to every conditional path cell in its serialized
   lookup run. Bit 0x02000000 is the frame-local state, while 0x10000000 is the
   disabled state published through the live path hash.

   A frame of -1 disables the complete run. When the requested frame exists,
   non-selected frames publish the inverse of their local state and the
   selected frame publishes the state itself. The serialized values for the
   non-selected frames are updated alongside the hash, exactly as retail does.

   Retail's missing-frame path performs its inverse-state work in two loops and
   deliberately carries the last value found by the first loop into the second.
   Once that carried value is disabled, later clear-state keys are skipped. */
// FUNCTION: WIZ8 0x00457ea0
void W8PathingService::SetConditionalPathFrame00457EA0(unsigned int path_handle, short frame)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    unsigned int lookup_index = path_handle;
    unsigned char frame_missing = 1;

    while (m_pulCondLookup[lookup_index] != 0 && frame_missing != 0) {
        if (m_pusCondNodeFrames[lookup_index] == frame) {
            frame_missing = 0;
        }
        ++lookup_index;
    }

    if (frame == -1) {
        lookup_index = path_handle;
        while (m_pulCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int current_value =
                    FindConditionalPathValue00458970(key, m_pulCondNodeValues[key_index]);
                if ((current_value & 0x10000000) == 0) {
                    index->Remove(&key, &current_value);
                    current_value |= 0x10000000;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
            ++lookup_index;
        }
        return;
    }

    if (frame_missing != 0) {
        unsigned int current_value;
        lookup_index = path_handle;
        while (m_pulCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                current_value =
                    FindConditionalPathValue00458970(key, m_pulCondNodeValues[key_index]);
                if ((m_pulCondNodeValues[key_index] & 0x02000000) != 0 &&
                    (current_value & 0x10000000) != 0) {
                    index->Remove(&key, &current_value);
                    current_value &= 0xefffffff;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
            ++lookup_index;
        }

        lookup_index = path_handle;
        while (m_pulCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                if ((m_pulCondNodeValues[key_index] & 0x02000000) == 0 &&
                    (current_value & 0x10000000) == 0) {
                    unsigned int key = m_pulCondNodeKeys[key_index];
                    index->Remove(&key, &current_value);
                    current_value |= 0x10000000;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
            ++lookup_index;
        }
        return;
    }

    lookup_index = path_handle;
    while (m_pulCondLookup[lookup_index] != 0) {
        if (m_pusCondNodeFrames[lookup_index] != frame) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int current_value =
                    FindConditionalPathValue00458970(key, m_pulCondNodeValues[key_index]);
                if ((m_pulCondNodeValues[key_index] & 0x02000000) != 0) {
                    if ((current_value & 0x10000000) != 0) {
                        index->Remove(&key, &current_value);
                        m_pulCondNodeValues[key_index] &= 0xefffffff;
                        current_value &= 0xefffffff;
                        index->Insert(&key, &current_value);
                    }
                } else if ((current_value & 0x10000000) == 0) {
                    index->Remove(&key, &current_value);
                    m_pulCondNodeValues[key_index] |= 0x10000000;
                    current_value |= 0x10000000;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }

    lookup_index = path_handle;
    while (m_pulCondLookup[lookup_index] != 0) {
        if (m_pusCondNodeFrames[lookup_index] == frame) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int current_value =
                    FindConditionalPathValue00458970(key, m_pulCondNodeValues[key_index]);
                if ((m_pulCondNodeValues[key_index] & 0x02000000) != 0) {
                    if ((current_value & 0x10000000) == 0) {
                        index->Remove(&key, &current_value);
                        current_value |= 0x10000000;
                        index->Insert(&key, &current_value);
                    }
                } else if ((current_value & 0x10000000) != 0) {
                    index->Remove(&key, &current_value);
                    current_value &= 0xefffffff;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }
}

/* Find the value for one conditional path key whose persistent identity is
   the requested low word. Several state variants of the same path cell can
   occupy one hash bucket, so an ordinary key lookup is not sufficient. */
// FUNCTION: WIZ8 0x00458970
unsigned int W8PathingService::FindConditionalPathValue00458970(unsigned int key,
                                                                unsigned int value)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    int slot = index->FindNextEntry(&key, -1);
    unsigned int found = 0;
    unsigned char searching = 1;
    while (slot >= 0 && searching != 0) {
        unsigned int candidate = index->entries[slot].value;
        if (((candidate ^ value) & 0xffff) == 0) {
            searching = 0;
            found = candidate;
        }
        slot = index->FindNextEntry(&key, slot);
    }
    return found;
}

/* Refresh the disabled bit for a conditional list of waypoints. */
// FUNCTION: WIZ8 0x004601b0
void W8PathingService::CheckConditionalWayPtStatus004601B0(unsigned short count,
                                                           unsigned short* waypoints)
{
    while (count != 0) {
        unsigned short waypoint = *waypoints;
        if (waypoint >= static_cast<unsigned short>(m_ulNumWayPoints)) {
            srAssertFail("pusWayPts[i] < (UINT16)m_ulNumWayPoints", OCTPATH_CPP, 0x1a4e,
                         "Pathing::CheckConditionalWayPtStatus: WayPt Index out of range.");
        }

        W8PathSurface* surface = &m_pSurfaces_048[waypoint];
        if ((ClassifyWaypoint00459C00(&surface->position_04) & 0x10000000) == 0) {
            surface->flags_00 &= 0xffdf;
        } else {
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
void W8PathingService::CheckConditionalLinkStatus00460250(unsigned short count,
                                                          unsigned short* edges)
{
    while (count != 0) {
        unsigned short edge_index = *edges;
        if (edge_index >= static_cast<unsigned short>(m_ulNumWayPtLinks)) {
            srAssertFail("pusLinks[i] < (UINT16)m_ulNumWayPtLinks", OCTPATH_CPP, 0x1a6c,
                         "Pathing::CheckConditionalLinkStatus: Link Index out of range.");
        }

        W8PathEdge* edge = &m_pEdges_04c[edge_index];
        if ((edge->flags_00 & 0x20000000) != 0) {
            if ((m_pSurfaces_048[edge->source_04].flags_00 & 0x20) != 0 ||
                TestWaypointSpan0045A1B0(&m_pSurfaces_048[edge->source_04].position_04,
                                         &m_pSurfaces_048[edge->destination_06].position_04, 0,
                                         0) == 0) {
                edge->flags_00 |= 0x80000000;
            } else {
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
unsigned char W8PathingService::HandlePathEdgeTransition00460350(W8NavigatorMovementState* movement)
{
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    unsigned short cursor = attachment->value_04;
    unsigned int flags = 0;
    unsigned short destination;

    if (cursor < attachment->path_position_index_08) {
        unsigned short* pairs = attachment->path_values_50;
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
        ((flags & 0x10000000) == 0 || (movement->unknown_000 & 0x10000000) == 0)) {
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
    } else {
        attachment->flags_00 &= 0xfeffffff;
    }
    return 2;
}

/* Measure the route between two points for the noise line-of-sight query. A
   throwaway attachment is built over (from, to); a positive `range` budget is
   held in path_cost_limit_070 while the path builds, and both
   exits restore the 1.0e10f limit. On success `range` receives the
   measured path length and `hops` the count of hop segments whose link is
   gated by a door. */
// FUNCTION: WIZ8 0x004604B0
unsigned char W8PathingService::MeasureAttachmentPath004604B0(const srVector3T<float>* from,
                                                              srVector3T<float>* to, float* range,
                                                              int* hops)
{
    W8NavigatorAttachment attachment(from, to);
    if (*range > g_float_005ebb34) {
        path_cost_limit_070 = *range;
    }
    if (BuildAttachmentPath00460950(&attachment, 0) == 0) {
        path_cost_limit_070 = 1.0e10f;
        return 0;
    }

    unsigned short cursor = attachment.value_04;
    if ((attachment.flags_00 & 0x00080000) != 0) {
        --cursor;
    }
    attachment.unknown_06 = cursor;
    ++attachment.unknown_06;

    srVector3T<float> next;
    if (attachment.unknown_06 < attachment.path_position_index_08) {
        next = attachment.position_4c[attachment.unknown_06];
        TestWaypointSpan0045A1B0(from, &next, 0, 0);
    } else {
        next = attachment.position_1c;
    }

    *range = attachment.MeasurePathLength00456B00();
    *hops = 0;
    while (attachment.value_04 < attachment.path_position_index_08) {
        if (TestAttachmentHopDoor00460680(&attachment) != 0) {
            ++*hops;
        }
        ++attachment.value_04;
    }
    path_cost_limit_070 = 1.0e10f;
    return 1;
}

/* Whether the hop at the attachment's current index runs over a disabled
   conditional edge guarded by a closed door. The current path value pair names
   the two surfaces; when their link carries both the conditional and disabled
   bits, the props inside the segment's bounds box are queried and the one
   nearest the midpoint supplies the trigger. The hop counts when that
   trigger's action record is a type-10 door record whose flag bit is clear. */
// FUNCTION: WIZ8 0x00460680
unsigned char W8PathingService::TestAttachmentHopDoor00460680(W8NavigatorAttachment* attachment)
{
    unsigned short current = attachment->value_04;
    if (current < attachment->path_position_index_08) {
        unsigned short* pairs = attachment->path_values_50;
        unsigned short source = pairs[current];
        unsigned short destination = pairs[current + 1];
        W8PathSurface* source_surface = &m_pSurfaces_048[source];
        unsigned short edge_index = source_surface->first_edge_24;
        unsigned char found = 0;
        while (edge_index != 0) {
            if (found != 0) {
                break;
            }
            if (m_pEdges_04c[edge_index].destination_06 == destination) {
                found = 1;
            } else {
                edge_index = m_pEdges_04c[edge_index].next_0c;
            }
        }
        if (edge_index != 0 && (m_pEdges_04c[edge_index].flags_00 & 0x10000000) != 0 &&
            (m_pEdges_04c[edge_index].flags_00 & 0x80000000) != 0) {
            srVector3T<float> lower = source_surface->position_04;
            srVector3T<float> upper = source_surface->position_04;
            GrowBoundsByPoint(&m_pSurfaces_048[destination].position_04, &lower, &upper);
            unsigned long* candidates = 0;
            Trigger* selected = 0;
            int count =
                g_octree_6598a4->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
            if (count > 0) {
                if (count == 1) {
                    W8Prop* prop = *g_world->collidable_props->GetAt(candidates[0]);
                    selected = prop->GetGDPropValue24();
                    if (selected == 0) {
                        return 0;
                    }
                } else {
                    double nearest_distance = 1e32;
                    float half_y = (upper.y - lower.y) * g_double_005ebe80;
                    float half_z = (upper.z - lower.z) * g_double_005ebe80;
                    srVector3T<float> center;
                    center.x = (upper.x - lower.x) * g_double_005ebe80 + lower.x;
                    center.y = half_y + lower.y;
                    center.z = half_z + lower.z;
                    for (int index = 0; index < count; ++index) {
                        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
                        Trigger* trigger = prop->GetGDPropValue24();
                        if (trigger != 0) {
                            srVector3T<float> position;
                            prop->GetPosition0044E2C0(&position);
                            float distance = (position - center).LengthSquared();
                            if (distance < nearest_distance) {
                                nearest_distance = distance;
                                selected = trigger;
                            }
                        }
                    }
                }
                W8TriggerActionData* action_data = selected->m_pActionData;
                if (action_data == 0 || action_data->type_004 != 10) {
                    action_data = 0;
                }
                if ((action_data->flags_008 & 1) == 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* A* search over the waypoint graph from the attachment's start position to
   its end position. The shared bit arrays mark generated (visible) and closed
   (rendered) nodes; the path heap holds the open set ordered by the integer
   truncation of each node's estimated remaining cost. Edges are filtered by
   the surface/edge flags and the caller's `flags` masks. Returns the reached
   end waypoint index, or zero on failure. */
// FUNCTION: WIZ8 0x00460b80
unsigned int W8PathingService::FindPath00460B80(W8NavigatorAttachment* attachment,
                                                unsigned int flags)
{
    unsigned short usStartNode = FindWaypoint0045B120(&attachment->position_10, '\0');
    if (usStartNode == 0 && (usStartNode = value_1d4) == 0) {
        return 0;
    }
    unsigned int start = usStartNode;
    unsigned short usEndNode = FindWaypoint0045B120(&attachment->position_1c, '\0');
    if (m_ulNumWayPoints <= start) {
        srAssertFail("usStartNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1bdb,
                     "Starting index out of range");
    }
    unsigned int end = usEndNode;
    if (m_ulNumWayPoints <= end) {
        srAssertFail("usEndNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1bdc,
                     "Last index out of range");
    }
    if (usEndNode == 0) {
        return 0;
    }
    rendered_waypoints_05c->ClearAll();
    visible_waypoints_058->ClearAll();
    path_heap_06c->heap_00->size_0c = 0;
    visible_waypoints_058->Set(start);
    m_pSurfaces_048[start].parent_10 = 0;
    unsigned int usWayPt = start;
    if (usStartNode != usEndNode) {
        do {
            unsigned int current = usWayPt & 0xffff;
            if (m_ulNumWayPoints <= current) {
                srAssertFail("usWayPt < m_ulNumWayPoints", OCTPATH_CPP, 0x1bea,
                             "Waypoint index out of range (1)");
            }
            unsigned short usLink = m_pSurfaces_048[current].first_edge_24;
            unsigned int link = usLink;
            if (m_ulNumWayPtLinks <= link) {
                srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1bec,
                             "Link index out of range (1)");
            }
            bool settled = true;
            if (usLink != 0) {
                do {
                    if (m_ulNumWayPtLinks <= link) {
                        srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1bf0,
                                     "Link index out of range (2)");
                    }
                    unsigned int edge_flags = m_pEdges_04c[link].flags_00;
                    unsigned int usNextNode = m_pEdges_04c[link].destination_06;
                    if (((m_pSurfaces_048[usNextNode].flags_00 & 0x20) != 0) ||
                        ((edge_flags & 0x80000000) != 0 &&
                         ((edge_flags & 0x10000000) == 0 || (flags & 0x10000000) == 0)) ||
                        (flags != 0 &&
                         (((edge_flags & 0xffff) != 0xffff && (edge_flags & flags & 0xffff) == 0) ||
                          ((edge_flags & 0x70000) != 0x70000 &&
                           (edge_flags & flags & 0x70000) == 0) ||
                          ((edge_flags & 0x380000) != 0x380000 &&
                           (edge_flags & flags & 0x380000) == 0)))) {
                        usNextNode = 0;
                    }
                    unsigned short usNext = static_cast<unsigned short>(usNextNode);
                    unsigned short usCurrent = static_cast<unsigned short>(usWayPt);
                    if (m_ulNumWayPoints <= usNextNode) {
                        srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1bf5,
                                     "Waypoint index out of range (2)");
                    }
                    if ((usNext != 0) && (usNext != usCurrent) &&
                        (rendered_waypoints_05c->Test(usNextNode) == 0)) {
                        if (visible_waypoints_058->Set(usNextNode) == 0) {
                            m_pSurfaces_048[usNextNode].cost_1c =
                                m_pEdges_04c[link].distance_08 + m_pSurfaces_048[current].cost_1c;
                            W8PathSurface* surfaces = m_pSurfaces_048;
                            if (surfaces[usNextNode].cost_1c < path_cost_limit_070) {
                                float dx = surfaces[end].position_04.x -
                                           surfaces[usNextNode].position_04.x;
                                settled = false;
                                float dy = surfaces[end].position_04.y -
                                           surfaces[usNextNode].position_04.y;
                                float dz = surfaces[end].position_04.z -
                                           surfaces[usNextNode].position_04.z;
                                surfaces[usNextNode].heuristic_18 =
                                    sqrt(dx * dx + dy * dy + dz * dz) * g_float_005ec394;
                                surfaces[usNextNode].parent_10 = usCurrent;
                                surfaces = m_pSurfaces_048;
                                surfaces[current].remaining_cost_20 =
                                    surfaces[usNextNode].heuristic_18 * g_float_005ec394 +
                                    surfaces[usNextNode].cost_1c;
                                m_pSurfaces_048[usNextNode].flags_00 |= 0x10;
                                W8PathHeapHandle* handle = path_heap_06c;
                                W8PathHeapEntry entry;
                                entry.node_00 = usNextNode;
                                entry.priority_04 =
                                    static_cast<unsigned int>(surfaces[current].remaining_cost_20);
                                handle->heap_00->Insert004675B0(&entry);
                                handle->root_node_04 = handle->heap_00->entries_00[0].node_00;
                            }
                        } else {
                            float cost =
                                m_pEdges_04c[link].distance_08 + m_pSurfaces_048[current].cost_1c;
                            if ((cost + g_float_005ebc88 < m_pSurfaces_048[usNextNode].cost_1c) &&
                                (cost < path_cost_limit_070)) {
                                float reduction = m_pSurfaces_048[usNextNode].cost_1c - cost;
                                m_pSurfaces_048[usNextNode].parent_10 = usCurrent;
                                W8PathSurface* surfaces = m_pSurfaces_048;
                                settled = false;
                                surfaces[current].remaining_cost_20 =
                                    surfaces[usNextNode].heuristic_18 * g_float_005ec394 +
                                    surfaces[usNextNode].cost_1c;
                                if ((m_pSurfaces_048[current].flags_00 & 0x10) == 0) {
                                    surfaces = m_pSurfaces_048 + usNextNode;
                                    surfaces->flags_00 |= 0x10;
                                    W8PathHeapHandle* handle = path_heap_06c;
                                    W8PathHeapEntry entry;
                                    entry.node_00 = usNextNode;
                                    entry.priority_04 = static_cast<unsigned int>(
                                        m_pSurfaces_048[current].remaining_cost_20);
                                    handle->heap_00->Insert004675B0(&entry);
                                    handle->root_node_04 = handle->heap_00->entries_00[0].node_00;
                                }
                                ReduceWaypointCosts00462220(usNextNode, reduction);
                            }
                        }
                    }
                    usLink = m_pEdges_04c[link].next_0c;
                    link = usLink;
                    if (m_ulNumWayPtLinks <= link) {
                        srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1c22,
                                     "Link index out of range (3)");
                    }
                } while (usLink != 0);
            }
            if (settled) {
                W8PathHeapHandle* handle = path_heap_06c;
                W8PathHeap* heap = handle->heap_00;
                if (heap->size_0c == 0) {
                    handle->root_node_04 = 0;
                } else {
                    handle->root_node_04 = heap->Delete().node_00;
                }
                m_pSurfaces_048[current].flags_00 &= 0xffef;
                rendered_waypoints_05c->Set(current);
            }
            usWayPt = path_heap_06c->root_node_04;
            current = usWayPt & 0xffff;
            if (m_ulNumWayPoints <= current) {
                srAssertFail("usWayPt < m_ulNumWayPoints", OCTPATH_CPP, 0x1c2a,
                             "Waypoint index out of range (3)");
            }
            if (static_cast<short>(usWayPt) == 0) {
                return 0;
            }
        } while (static_cast<short>(usWayPt) != static_cast<short>(end));
    }
    m_pSurfaces_048[start].parent_10 = 0;
    return usWayPt & 0xffff;
}

/* Build the attachment's stored route for the path found between its start
   and end positions. The parent chain left by FindPath is reversed through
   the shared scratch array into position_4c/path_values_50, the end position
   is appended, and the final hop is trimmed back to the destination when the
   last waypoint already sees it. */
// FUNCTION: WIZ8 0x00460950
unsigned char W8PathingService::BuildAttachmentPath00460950(W8NavigatorAttachment* attachment,
                                                            unsigned int flags)
{
    if ((((rendered_waypoints_05c != 0) && (visible_waypoints_058 != 0)) && (path_heap_06c != 0)) &&
        (m_pSurfaces_048 != 0)) {
        rendered_waypoints_05c->ClearAll();
        visible_waypoints_058->ClearAll();
        path_heap_06c->heap_00->size_0c = 0;
        unsigned int node = FindPath00460B80(attachment, flags);
        if (static_cast<short>(node) != 0) {
            unsigned int count = 0;
            visible_waypoints_058->ClearAll();
            do {
                if (visible_waypoints_058->Set(node & 0xffff) != 0) {
                    break;
                }
                unsigned int index = count & 0xffff;
                if (m_ulNumWayPoints <= (count & 0xffff)) {
                    srAssertFail("i < m_ulNumWayPoints", OCTPATH_CPP, 0x1ba2, 0);
                    index = count;
                }
                static_cast<unsigned short*>(g_path_scratch_00659c64)[index] =
                    static_cast<unsigned short>(node);
                node = m_pSurfaces_048[node & 0xffff].parent_10;
                ++count;
            } while (node != 0);
            unsigned int remaining = count & 0xffff;
            static_cast<unsigned short*>(g_path_scratch_00659c64)[remaining] = 0;
            if (static_cast<short>(count) != 0) {
                do {
                    unsigned short surface_index =
                        static_cast<unsigned short*>(g_path_scratch_00659c64)[remaining - 1];
                    srVector3T<float>* position = &m_pSurfaces_048[surface_index].position_04;
                    if (static_cast<unsigned int>(attachment->capacity_0a) <=
                        static_cast<unsigned int>(attachment->path_position_index_08 + 1)) {
                        attachment->GrowPathStorage00456BD0();
                    }
                    srVector3T<float>* slot =
                        attachment->position_4c + attachment->path_position_index_08;
                    slot->x = position->x;
                    slot->y = position->y;
                    slot->z = position->z;
                    attachment->path_values_50[attachment->path_position_index_08] = surface_index;
                    attachment->path_position_index_08 = attachment->path_position_index_08 + 1;
                    --remaining;
                    attachment->flags_00 = attachment->flags_00 & 0xffbfffff;
                } while (remaining != 0);
            }
            srVector3T<float>* destination = &attachment->position_1c;
            srVector3T<float>* slot = attachment->position_4c + attachment->path_position_index_08;
            slot->x = destination->x;
            slot->y = attachment->position_1c.y;
            slot->z = attachment->position_1c.z;
            if (((attachment->value_04 < attachment->path_position_index_08) ||
                 ((attachment->flags_00 & 0x80000) != 0)) &&
                (TestWaypointSpan0045A1B0(attachment->position_4c +
                                              (attachment->path_position_index_08 - 2),
                                          destination, '\0', '\0') != '\0')) {
                attachment->path_position_index_08 = attachment->path_position_index_08 - 1;
                slot = attachment->position_4c + attachment->path_position_index_08;
                slot->x = destination->x;
                slot->y = attachment->position_1c.y;
                slot->z = attachment->position_1c.z;
            }
            attachment->flags_00 = attachment->flags_00 | 0x20000;
            return 1;
        }
    }
    return 0;
}

/* Build a randomized patrol route into the attachment. When the destination
   sits within `maximum` of the attachment's start position, RecursePatrolLinks
   walks the waypoint graph for a node whose accumulated link cost clears a
   randomized target distance; farther out it falls back to the straight
   attachment path. The fallback nodes tracked in patrol_node_1dc and
   probe_cell_key_078 supply the endpoint when no candidate qualifies.
   `velocity` is passed by callers but never read. */
// FUNCTION: WIZ8 0x00461960
unsigned char
W8PathingService::BuildPatrolPath00461960(W8NavigatorAttachment* attachment, unsigned int flags,
                                          const srVector3T<float>* destination, float minimum,
                                          const srVector3T<float>* velocity, float maximum)
{
    if (minimum >= maximum) {
        return 0;
    }
    patrol_max_1e4 = maximum;
    patrol_min_1e0 = minimum;
    patrol_cost_210 = 0.0f;
    patrol_start_1ec = attachment->position_10;
    patrol_destination_1f8 = *destination;
    path_flags_000 = flags;
    attachment->flags_00 = attachment->flags_00 | 0x200000;
    srVector3T<float> delta = patrol_start_1ec - patrol_destination_1f8;
    if (maximum < delta.Length()) {
        return BuildAttachmentPath00460950(attachment, flags);
    }
    float roll = Random(900) + g_octree_cell_scale_005ebcd0;
    value_1d4 = 0;
    patrol_distance_1e8 = roll * maximum * g_float_005ec128 + maximum;
    unsigned short usStartNode = FindWaypoint0045B120(&attachment->position_10, '\x01');
    if ((usStartNode == 0) && ((usStartNode = value_1d4) == 0)) {
        return 0;
    }
    rendered_waypoints_05c->ClearAll();
    visible_waypoints_058->ClearAll();
    path_heap_06c->heap_00->size_0c = 0;
    probe_cell_key_078 = 0;
    patrol_node_1dc = 0;
    unsigned int start = usStartNode;
    visible_waypoints_058->Set(start);
    W8PathSurface* surfaces = m_pSurfaces_048;
    probe_limit_088 = surfaces[start].positional_14;
    float dx = surfaces[start].position_04.x - attachment->position_10.x;
    float dy = surfaces[start].position_04.y - attachment->position_10.y;
    float dz = surfaces[start].position_04.z - attachment->position_10.z;
    m_pSurfaces_048[start].cost_1c = sqrt(dx * dx + dy * dy + dz * dz);
    m_pSurfaces_048[start].parent_10 = 0;
    unsigned int node = RecursePatrolLinks00461D10(usStartNode);
    if (static_cast<short>(node) == 0) {
        if (patrol_node_1dc == 0) {
            node = static_cast<unsigned short>(probe_cell_key_078);
        } else {
            node = patrol_node_1dc & 0xffff;
        }
        if (static_cast<short>(node) == 0) {
            return 0;
        }
    }
    attachment->position_1c = m_pSurfaces_048[node & 0xffff].position_04;
    unsigned int count = 0;
    unsigned int current = node;
    unsigned short previous = 0;
    do {
        if (static_cast<unsigned short>(current) == 0) {
            break;
        }
        unsigned int index = count & 0xffff;
        if (m_ulNumWayPoints <= (count & 0xffff)) {
            srAssertFail("i < m_ulNumWayPoints", OCTPATH_CPP, 0x1d87, 0);
            index = count;
        }
        previous = static_cast<unsigned short>(current);
        static_cast<unsigned short*>(g_path_scratch_00659c64)[index] = previous;
        current = m_pSurfaces_048[current & 0xffff].parent_10;
        count = index + 1;
    } while (previous != static_cast<unsigned short>(current));
    unsigned int remaining = count & 0xffff;
    static_cast<unsigned short*>(g_path_scratch_00659c64)[remaining] = 0;
    if (static_cast<short>(count) != 0) {
        do {
            unsigned short surface_index =
                static_cast<unsigned short*>(g_path_scratch_00659c64)[remaining - 1];
            srVector3T<float>* position = &m_pSurfaces_048[surface_index].position_04;
            if (static_cast<unsigned int>(attachment->capacity_0a) <=
                static_cast<unsigned int>(attachment->path_position_index_08 + 1)) {
                attachment->GrowPathStorage00456BD0();
            }
            srVector3T<float>* slot = attachment->position_4c + attachment->path_position_index_08;
            slot->x = position->x;
            slot->y = position->y;
            slot->z = position->z;
            attachment->path_values_50[attachment->path_position_index_08] = surface_index;
            attachment->path_position_index_08 = attachment->path_position_index_08 + 1;
            attachment->flags_00 = attachment->flags_00 & 0xffbfffff;
            --remaining;
        } while (remaining != 0);
    }
    if (1 < attachment->path_position_index_08) {
        attachment->path_position_index_08 = attachment->path_position_index_08 - 1;
        attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
    }
    if (value_1d4 != 0) {
        attachment->position_28 = probe_position_07c;
        attachment->flags_00 = attachment->flags_00 | 0x80000;
    }
    attachment->flags_00 = attachment->flags_00 | 0x20000;
    return 1;
}

/* Recursive depth-first patrol search from `waypoint`. Each pass collects the
   waypoint's admissible links (edge/surface flag filters matching FindPath),
   prices them by accumulated link cost, sorts them by the surface key, then
   accepts the first candidate whose cost clears the randomized
   patrol_distance - or the 250000 sanity bound - while the last-measured
   candidate distance still exceeds patrol_min. Otherwise it recurses into
   each candidate in sorted order. The argmin-key candidate is parked in
   patrol_node_1dc and the best-cost alternate in probe_cell_key_078 for the
   caller's fallback. The visited set is rendered_waypoints_05c. */
// FUNCTION: WIZ8 0x00461d10
unsigned short W8PathingService::RecursePatrolLinks00461D10(unsigned short waypoint)
{
    unsigned short links[20];
    unsigned long keys[20];
    float costs[20];
    unsigned int count = 0;
    float distance;

    unsigned short link = m_pSurfaces_048[waypoint].first_edge_24;
    if (link != 0) {
        do {
            if (m_ulNumWayPtLinks <= static_cast<unsigned int>(link)) {
                srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1dcf,
                             "Link index out of range");
            }
            unsigned int edge_flags = m_pEdges_04c[link].flags_00;
            unsigned short next = m_pEdges_04c[link].destination_06;
            if ((((edge_flags & 0x1000000) == 0) && (next != waypoint)) &&
                ((m_pSurfaces_048[next].flags_00 & 0x20) == 0) &&
                (((edge_flags & 0x80000000) == 0) ||
                 (((edge_flags & 0x10000000) != 0) && ((path_flags_000 & 0x10000000) != 0))) &&
                ((path_flags_000 == 0) || ((((edge_flags & 0xffff) == 0xffff) ||
                                            ((edge_flags & path_flags_000 & 0xffff) != 0)) &&
                                           (((edge_flags & 0x70000) == 0x70000) ||
                                            ((edge_flags & path_flags_000 & 0x70000) != 0)) &&
                                           (((edge_flags & 0x380000) == 0x380000) ||
                                            ((edge_flags & path_flags_000 & 0x380000) != 0)))) &&
                (next != 0)) {
                if (rendered_waypoints_05c->Test(next) == 0) {
                    if (m_ulNumWayPoints <= static_cast<unsigned int>(next)) {
                        srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1dd9,
                                     "Waypoint index out of range");
                    }
                    W8PathSurface* next_surface = &m_pSurfaces_048[next];
                    float dx = next_surface->position_04.x - patrol_destination_1f8.x;
                    float dy = next_surface->position_04.y - patrol_destination_1f8.y;
                    float dz = next_surface->position_04.z - patrol_destination_1f8.z;
                    distance = sqrt(dx * dx + dy * dy + dz * dz);
                    if (distance < patrol_max_1e4) {
                        next_surface->cost_1c =
                            m_pSurfaces_048[waypoint].cost_1c + m_pEdges_04c[link].distance_08;
                        costs[count] = next_surface->cost_1c;
                        next_surface->parent_10 = waypoint;
                        links[count] = link;
                        unsigned int key = next_surface->positional_14;
                        keys[count] = key;
                        ++count;
                        if (key < probe_limit_088) {
                            probe_limit_088 = key;
                            patrol_node_1dc = next;
                        } else {
                            if ((key != probe_limit_088) ||
                                (next_surface->cost_1c <= patrol_cost_210)) {
                                if ((patrol_node_1dc == 0) &&
                                    (patrol_cost_210 < next_surface->cost_1c)) {
                                    patrol_cost_210 = next_surface->cost_1c;
                                    probe_cell_key_078 = next;
                                }
                            } else {
                                patrol_cost_210 = next_surface->cost_1c;
                                patrol_node_1dc = next;
                                probe_cell_key_078 = next;
                            }
                        }
                        if (0x13 < count) {
                            srAssertFail("ulLinkNum < 20", OCTPATH_CPP, 0x1e00,
                                         "Too many links for waypoint in RecursePatrolLinks");
                        }
                    }
                }
            }
            link = m_pEdges_04c[link].next_0c;
        } while (link != 0);
        if (1 < count) {
            QuickSortByKey(links, keys, 0, static_cast<int>(count) - 1);
        }
    }
    rendered_waypoints_05c->Set(waypoint);
    if (count != 0) {
        unsigned int index = 0;
        do {
            unsigned short next = m_pEdges_04c[links[index]].destination_06;
            if (m_ulNumWayPoints <= static_cast<unsigned int>(next)) {
                srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1e0f,
                             "Waypoint index out of range (2)");
            }
            if (((patrol_distance_1e8 < costs[index]) || (g_float_005ec39c < costs[index])) &&
                (patrol_min_1e0 < distance)) {
                return next;
            }
            unsigned short found = RecursePatrolLinks00461D10(next);
            if (found != 0) {
                return found;
            }
            ++index;
        } while (index < count);
    }
    return 0;
}

/* Reduce both accumulated costs for one accepted waypoint and continue down
   the selected parent tree. A child participates only while it remains in the
   active waypoint bit set and names the current waypoint as its parent. The
   cost reaching zero is the retail recursion boundary. */
// FUNCTION: WIZ8 0x00462220
void W8PathingService::ReduceWaypointCosts00462220(unsigned int waypoint, float amount)
{
    W8PathSurface* surface = &m_pSurfaces_048[waypoint];
    surface->cost_1c -= amount;
    if (surface->cost_1c >= g_float_005ebb34) {
        surface->remaining_cost_20 -= amount;

        unsigned short edge_index = surface->first_edge_24;
        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short child = edge->destination_06;
            if (child != 0 && visible_waypoints_058->Test(child) != 0 &&
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
    } else if (direction > 7) {
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
unsigned char W8PathingService::AdvanceAttachmentWaypoint00462DE0(const srVector3T<float>* source,
                                                                  W8NavigatorAttachment* attachment)
{
    unsigned short cursor = attachment->value_04;
    if ((attachment->flags_00 & 0x00080000) != 0) {
        --cursor;
    }
    attachment->unknown_06 = cursor;
    ++attachment->unknown_06;

    if (attachment->unknown_06 < attachment->path_position_index_08) {
        srVector3T<float> destination = attachment->position_4c[attachment->unknown_06];
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
unsigned char W8PathingService::MatchesPathProbe00465970(unsigned int tag, const float* radius,
                                                         const srVector3T<float>* position)
{
    for (unsigned int index = 0; index < path_probe_count_0d4; ++index) {
        W8PathProbeVolume* probe = &path_probes_0d8[index];
        if (probe->tag_00 == tag) {
            if (radius == 0) {
                return 1;
            }

            srVector3T<float> delta = probe->center_0c - *position;
            float distance = delta.Length();
            if (distance < probe->outer_radius_04 + *radius && probe->inner_radius_08 < distance) {
                return 1;
            }
        }
    }
    return 0;
}

/* Reserve the next fixed-width planner node. Storage grows by fifty records,
   is cleared in full, and retains every node through the newly issued index. */
// FUNCTION: WIZ8 0x00465A00
unsigned short W8PathingService::AllocateSearchNode00465A00()
{
    unsigned int node_index = ++search_node_count_0cc;

    if (m_owned_0c8 != 0 && node_index < search_node_capacity_0d0) {
        return static_cast<unsigned short>(node_index);
    }

    search_node_capacity_0d0 += 50;
    W8PathSearchNode* new_nodes = new W8PathSearchNode[search_node_capacity_0d0];
    if (new_nodes == 0) {
        srAssertFail("pNewSearchNodes", OCTPATH_CPP, 0x2751, 0);
    }
    memset(new_nodes, 0, search_node_capacity_0d0 * sizeof(W8PathSearchNode));
    if (m_owned_0c8 != 0) {
        memcpy(new_nodes, m_owned_0c8, search_node_count_0cc * sizeof(W8PathSearchNode));
        delete[] m_owned_0c8;
    }
    m_owned_0c8 = new_nodes;
    return static_cast<unsigned short>(node_index);
}

/* Walk grid-sized steps from a position toward a search node. Every crossed
   cell must resolve through the visited-node index to a live, unblocked node
   whose recorded clearance exceeds the caller's limit. */
// FUNCTION: WIZ8 0x00465AF0
unsigned char W8PathingService::CanReachSearchNode00465AF0(const srVector3T<float>* position,
                                                           unsigned short target_node,
                                                           float clearance)
{
    W8PathSearchNode* target = &m_owned_0c8[target_node];
    srVector3T<float> delta = target->position_20 - *position;
    float scale;
    if (static_cast<float>(fabs(delta.x)) > static_cast<float>(fabs(delta.z))) {
        scale = static_cast<float>(fabs(grid_scale_01c / delta.x));
    } else {
        scale = static_cast<float>(fabs(grid_scale_01c / delta.z));
    }

    srVector3T<float> probe = *position;
    float step_x = delta.x * scale;
    float step_z = delta.z * scale;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
    unsigned char blocked = 0;

    while (1) {
        int cell_x = static_cast<int>((probe.x - level_bounds[0]) / grid_scale_01c);
        int cell_z = static_cast<int>((probe.z - level_bounds[2]) / grid_scale_01c);
        unsigned int key = cell_z * 0x10000 + cell_x;
        unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
        int slot = visited->bucket_heads[hash & (visited->bucket_count - 1)];
        unsigned int node_index = 0;
        W8OctreeEntry* entries = visited->entries;
        while (slot != -1) {
            if (entries[slot].key == key) {
                node_index = static_cast<unsigned int>(entries[slot].value);
                break;
            }
            slot = entries[slot].next_index;
        }

        if (static_cast<unsigned short>(node_index) == target_node || blocked != 0) {
            return blocked == 0;
        }
        if (static_cast<unsigned short>(node_index) == 0 ||
            (m_owned_0c8[node_index & 0xffff].flags_00 & 0x100) != 0 ||
            m_owned_0c8[node_index & 0xffff].clearance_18 <= clearance) {
            blocked = 1;
        } else {
            probe.x += step_x;
            probe.z += step_z;
        }
    }
}

/* Pull the last planned point onto the requested contact shell when it only
   overshoots that shell by less than half a path cell. The adjusted point is
   also made the attachment's current point and republished to the octree. */
// FUNCTION: WIZ8 0x00465D70
void W8PathingService::AdjustFinalPathEndpoint00465D70(W8NavigatorMovementState* movement,
                                                       float radius, float separation)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
    /* The decompiled body reads this storage only after the same short-circuit
   chain that clang's flow analysis cannot see through; retail leaves it
   uninitialised on the failed-read path. Suppress only this diagnostic. */
    int target_location = movement->value_010;
    if (target_location < 0 || flag_09c != 0) {
        return;
    }

    float target_radius;
    srVector3T<float> target_position;
    if (target_location <= 0) {
        target_radius = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
        target_position = g_startup_world_659c0c->GetPosition();
    } else {
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x27b0, OCTPATH_CPP, target_location, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info != 0 && info->monster != 0) {
            target_radius = info->monster->movement_0c0.alternate_radius_0b4;
            target_position = info->monster->GetPosition();
        }
    }

    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    srVector3T<float>* endpoint = &attachment->position_4c[attachment->path_position_index_08];
    srVector3T<float> direction = *endpoint - target_position;
    float excess = direction.Length() - (target_radius + radius);
    if (separation < excess && excess - separation < grid_scale_01c * g_float_005ebc7c) {
        direction.SetLength((target_radius + radius + separation) * g_path_endpoint_scale_005ec1a4);

        srVector3T<float> adjusted;
        adjusted = target_position + direction;
        *endpoint = adjusted;
        attachment->position_1c = *endpoint;

        if ((attachment->flags_00 & 0x08000000) == 0) {
            attachment->flags_00 |= 0x02000000;
            attachment->position_40 = adjusted;
            g_octree_6598a4->QueueOctreeKind130042E810(movement->location_id_004, &adjusted);
        }
    }
#pragma clang diagnostic pop
}

/* Select one conditional frame for a GD prop's path cells. Entries belonging
   to every other frame first lose both prop-state bits. Entries belonging to
   the selected frame then gain the caller's state bits. The hash table may
   contain several values for one key, so the low word from the serialized
   conditional value is the identity used to find the exact pairing. */
// FUNCTION: WIZ8 0x00465fb0
void W8PathingService::UpdateConditionalPathFlags00465FB0(unsigned int path_handle,
                                                          unsigned short frame, unsigned int flags)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    unsigned int lookup_index = path_handle;

    while (m_pulCondLookup[lookup_index] != 0) {
        if (m_pusCondNodeFrames[lookup_index] != frame) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int wanted_value = m_pulCondNodeValues[key_index];
                unsigned int current_value = 0;
                int slot = index->FindNextEntry(&key, -1);
                unsigned char searching = 1;
                while (slot >= 0 && searching != 0) {
                    unsigned int value = index->entries[slot].value;
                    if (((value ^ wanted_value) & 0xffff) == 0) {
                        searching = 0;
                        current_value = value;
                    }
                    slot = index->FindNextEntry(&key, slot);
                }

                if ((current_value & 0x08000000) != 0) {
                    index->Remove(&key, &current_value);
                    current_value &= 0xd7ffffff;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }

    lookup_index = path_handle;
    while (m_pulCondLookup[lookup_index] != 0) {
        if (m_pusCondNodeFrames[lookup_index] == frame) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int wanted_value = m_pulCondNodeValues[key_index];
                unsigned int current_value = 0;
                int slot = index->FindNextEntry(&key, -1);
                unsigned char searching = 1;
                while (slot >= 0 && searching != 0) {
                    unsigned int value = index->entries[slot].value;
                    if (((value ^ wanted_value) & 0xffff) == 0) {
                        searching = 0;
                        current_value = value;
                    }
                    slot = index->FindNextEntry(&key, slot);
                }

                if ((flags & current_value) == 0) {
                    index->Remove(&key, &current_value);
                    current_value |= flags;
                    index->Insert(&key, &current_value);
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }
}

/* Notify every collidable prop overlapping the search cell. The planner's
   single-result form stops after the first notification and returns that
   prop's collection index; the ordinary form visits the complete query. */
// FUNCTION: WIZ8 0x004663D0
int W8PathingService::ProcessSearchNodeProps004663D0(unsigned int node_index,
                                                     unsigned char first_only)
{
    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    float half_cell = grid_scale_01c * g_float_005ebc7c;

    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> lower_extent;
    srVector3T<float> upper_extent;
    float vertical_extent = grid_scale_01c + grid_scale_01c;
    lower_extent.Set(half_cell, 0.0f, half_cell);
    upper_extent.Set(half_cell, vertical_extent, half_cell);
    lower = node->position_20 - lower_extent;
    upper = node->position_20 + upper_extent;

    unsigned long* candidates = 0;
    unsigned int count =
        g_octree_6598a4->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
    for (unsigned int index = 0; index < count; ++index) {
        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
        prop->CanBeUsedFrom(node->cell_x_04, node->cell_z_06, 1);
        if (first_only != 0) {
            return candidates[index];
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
unsigned int W8PathingService::CollectPathProbes004656A0(W8NavigatorMovementState* movement,
                                                         float radius)
{
    path_probe_count_0d4 = 0;

    float extent = g_runtime_world_scale_6081e8 + radius;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> half_extent;
    half_extent.Set(extent, extent, extent);
    lower = movement->position_040 - half_extent;
    upper = movement->position_040 + half_extent;

    path_candidates_098 = 0;
    path_candidate_count_094 = g_octree_6598a4->QueryLocationsInBox(
        &path_candidates_098, &lower, &upper, movement->location_id_004);

    srVector3T<float> player_position = g_startup_world_659c0c->GetPosition();
    srVector3T<float> delta = player_position - movement->position_040;
    float distance = delta.Length();
    float player_radius = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
    float overlap_radius = radius;
    if (player_radius < radius) {
        overlap_radius = player_radius;
    }
    if (distance < (radius - overlap_radius * g_float_005ebc7c) + player_radius) {
        W8PathProbeVolume* probe = &path_probes_0d8[path_probe_count_0d4];
        probe->tag_00 = 0;
        probe->outer_radius_04 = player_radius;
        probe->center_0c = g_startup_world_659c0c->GetPosition();
        ++path_probe_count_0d4;
    }

    for (unsigned int index = 0; index < path_candidate_count_094 && path_probe_count_0d4 < 5;
         ++index) {
        int location_id = path_candidates_098[index];
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x26ae, OCTPATH_CPP, location_id, 0);
        if (monster_index != static_cast<unsigned int>(-1)) {
            monster_index = MonsterGetIndexByLocationID(0x26b1, OCTPATH_CPP, location_id, 1);
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (info != 0 && info->monster != 0 && info->monster->state_088 != 0) {
                W8Monster* monster = info->monster;
                srVector3T<float> monster_position = monster->GetPosition();
                srVector3T<float> monster_delta = monster_position - movement->position_040;
                distance = monster_delta.Length();
                float monster_radius = monster->movement_0c0.alternate_radius_0b4;
                overlap_radius = radius;
                if (monster_radius < radius) {
                    overlap_radius = monster_radius;
                }
                if (distance < (radius - overlap_radius * g_float_005ebc7c) + monster_radius) {
                    W8PathProbeVolume* probe = &path_probes_0d8[path_probe_count_0d4];
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

/* Build a bounded grid route from the navigator's current position to its
   active attachment target. The open-chain index owns one search node per
   cell, the fixed-capacity minimum heap chooses the next node to expand, and
   the selected parent chain is collapsed into the attachment's route array. */
// FUNCTION: WIZ8 0x00463460
unsigned short W8PathingService::PlanMovement00463460(W8NavigatorMovementState* movement,
                                                      float radius, float separation)
{
    float diagonal_step = grid_scale_01c * g_path_cardinal_scale_005ec358;
    unsigned short result = 0;
    unsigned char stop_search = 0;

    if (flag_0a4 == 0) {
        trace_offset_0ac.Set(0.0f, 500.0f, 0.0f);
        trace_mode_0b8 = 0;
        trace_height_offset_0bc = 500.0f;
    }

    search_node_count_0cc = 0;
    planner_location_090 = movement->location_id_004;
    memset(m_owned_0c8, 0, search_node_capacity_0d0 * sizeof(W8PathSearchNode));
    probe_cell_key_078 = 0;
    probe_limit_088 = static_cast<unsigned int>(-1);
    path_heap_06c->heap_00->size_0c = 0;

    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    attachment->flags_00 &= 0xfffffff0;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
    if (visited->bucket_count != 0) {
        delete[] visited->bucket_heads;
        delete[] visited->entries;
    }
    visited->bucket_count = 0;
    visited->bucket_heads = 0;
    visited->entries = 0;
    visited->free_head = -1;
    visited->Grow();
    attachment->flags_00 &= 0xfdffffff;

    unsigned char allow_dynamic = static_cast<unsigned char>(movement->unknown_000 >> 28 & 1);
    srVector3T<float> start = movement->position_040;
    srVector3T<float> target;
    if (flag_09c != 0) {
        target = movement->target_position_04c;
    } else if ((attachment->flags_00 & 0x00080000) != 0) {
        target = attachment->position_28;
    } else if (attachment->value_04 < attachment->path_position_index_08) {
        target = attachment->position_4c[attachment->value_04];
    } else {
        target = attachment->position_1c;
    }

    srVector3T<float> target_delta = target - start;
    float target_distance = target_delta.Length();
    float remaining_callback = movement->callback_threshold_058 - movement->callback_progress_05c;

    if ((attachment->flags_00 & 0x04000000) == 0) {
        float search_extent = target_distance;
        if (search_extent < remaining_callback) {
            search_extent = remaining_callback;
        }
        search_extent += g_runtime_world_scale_6081e8 + radius;
        srVector3T<float> lower;
        srVector3T<float> upper;
        srVector3T<float> half_extent;
        half_extent.Set(search_extent, search_extent, search_extent);
        lower = start - half_extent;
        upper = start + half_extent;
        CollectPathProbes004656A0(movement, radius);
        path_candidates_098 = 0;
        path_candidate_count_094 = g_octree_6598a4->QueryLocationsInBox(
            &path_candidates_098, &lower, &upper, movement->location_id_004);
    } else {
        path_probe_count_0d4 = 0;
        if (movement->value_010 < 1) {
            path_candidate_count_094 = 0;
        } else {
            path_candidate_count_094 = 1;
            // reinterpret-ok: single-slot candidate list overlays Navigator::value_010
            path_candidates_098 = reinterpret_cast<unsigned long*>(&movement->value_010);
        }
    }

    int root_x = static_cast<int>((start.x - level_bounds[0]) / grid_scale_01c);
    int root_z = static_cast<int>((start.z - level_bounds[2]) / grid_scale_01c);
    unsigned short root_index = AllocateSearchNode00465A00();
    W8PathSearchNode* root = &m_owned_0c8[root_index];
    root->flags_00 = 0;
    root->node_index_02 = root_index;
    root->cell_x_04 = static_cast<unsigned short>(root_x);
    root->cell_z_06 = static_cast<unsigned short>(root_z);
    root->path_height_08 =
        static_cast<unsigned short>(static_cast<int>((start.y - level_bounds[1]) / span_020) + 1);
    root->parent_node_0a = 0;
    root->base_score_0c = 0.0f;
    root->path_cost_10 = 0.0f;
    root->distance_14 = target_distance;
    root->position_20 = start;

    unsigned int root_key = root_z * 0x10000 + root_x;
    if (visited->free_head == -1) {
        visited->Grow();
    }
    int root_slot = visited->free_head;
    W8OctreeEntry* visited_entries = visited->entries;
    visited->free_head = visited_entries[root_slot].next_index;
    unsigned int root_hash = (root_key >> 10 ^ root_key) >> 10 ^ root_key;
    int* visited_buckets = visited->bucket_heads;
    visited_entries[root_slot].key = root_key;
    visited_entries[root_slot].value = root_index;
    visited_entries[root_slot].next_index =
        visited_buckets[root_hash & (visited->bucket_count - 1)];
    visited_buckets[root_hash & (visited->bucket_count - 1)] = root_slot;

    unsigned int root_height = root->path_height_08;
    float root_clearance = radius;
    float root_vertical = 0.0f;
    unsigned char root_dynamic = 0;
    ResolvePathCell004648D0(root_key, 1, &root_height, &root_clearance, &root_vertical,
                            &root_dynamic);
    UpdateSearchNodeScore00464FF0(root_index, &target, root_clearance, radius);
    unsigned int root_score = static_cast<unsigned int>(root->score_1c);
    if (root_score < probe_limit_088) {
        probe_limit_088 = root_score;
        probe_cell_key_078 = root_index;
    }
    probe_cell_key_078 = 0;

    W8PathHeap* heap = path_heap_06c->heap_00;
    W8PathHeapEntry entry;
    entry.node_00 = root_index;
    entry.priority_04 = static_cast<unsigned int>(root->score_1c);
    heap->Insert004675B0(&entry);
    path_heap_06c->root_node_04 = heap->entries_00[0].node_00;

    unsigned int best_node = path_heap_06c->root_node_04;
    while (best_node != 0 && stop_search == 0 && search_node_count_0cc < g_path_reserve_0060827a) {
        W8PathSearchNode* current = &m_owned_0c8[best_node];
        unsigned short current_x = current->cell_x_04;
        unsigned short current_z = current->cell_z_06;
        unsigned int current_height = current->path_height_08;

        for (int direction = 0; direction < 8; ++direction) {
            unsigned int neighbor_x = current_x;
            unsigned int neighbor_z = current_z;
            if (direction >= 1 && direction <= 3) {
                ++neighbor_x;
            } else if (direction > 4) {
                --neighbor_x;
            }
            if (direction < 2 || direction > 6) {
                ++neighbor_z;
            } else if (direction > 2 && direction < 6) {
                --neighbor_z;
            }

            unsigned int key = neighbor_z * 0x10000 + neighbor_x;
            float step = (direction & 1) == 0 ? grid_scale_01c : diagonal_step;
            float path_cost = current->path_cost_10 + step;
            float base_score = current->base_score_0c + step;

            visited_entries = visited->entries;
            visited_buckets = visited->bucket_heads;
            unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
            int slot = visited_buckets[hash & (visited->bucket_count - 1)];
            unsigned int existing_index = 0;
            while (slot != -1) {
                if (visited_entries[slot].key == key) {
                    existing_index = static_cast<unsigned int>(visited_entries[slot].value);
                    break;
                }
                slot = visited_entries[slot].next_index;
            }

            if (existing_index != 0) {
                W8PathSearchNode* existing = &m_owned_0c8[existing_index & 0xffff];
                if ((existing->flags_00 & 0x0100) != 0) {
                    continue;
                }
                int height_delta = static_cast<int>(current->path_height_08) -
                                   static_cast<int>(existing->path_height_08);
                if (height_delta < 0) {
                    height_delta = -height_delta;
                }
                base_score += height_delta * span_020 * g_float_005ec3b8;
                if (existing->base_score_0c <= base_score) {
                    continue;
                }
                existing->base_score_0c = base_score;
                existing->path_cost_10 = path_cost;
                existing->parent_node_0a = static_cast<unsigned short>(best_node);
                UpdateSearchNodeScore00464FF0(existing_index, &target, existing->clearance_18,
                                              radius);
                if ((existing->flags_00 & 0x0400) != 0) {
                    existing->flags_00 &= 0xfbff;
                    entry.node_00 = existing->node_index_02;
                    entry.priority_04 = static_cast<unsigned int>(existing->score_1c);
                    heap->Insert004675B0(&entry);
                    path_heap_06c->root_node_04 = heap->entries_00[0].node_00;
                }
                continue;
            }

            unsigned int height = current_height;
            float clearance = radius;
            float vertical = base_score;
            unsigned char dynamic = 0;
            if (ResolvePathCell004648D0(key, allow_dynamic, &height, &clearance, &vertical,
                                        &dynamic) == 0) {
                continue;
            }

            unsigned short node_index = AllocateSearchNode00465A00();
            if (visited->free_head == -1) {
                visited->Grow();
            }
            visited_entries = visited->entries;
            visited_buckets = visited->bucket_heads;
            int new_slot = visited->free_head;
            visited->free_head = visited_entries[new_slot].next_index;
            visited_entries[new_slot].key = key;
            visited_entries[new_slot].value = node_index;
            visited_entries[new_slot].next_index =
                visited_buckets[hash & (visited->bucket_count - 1)];
            visited_buckets[hash & (visited->bucket_count - 1)] = new_slot;

            W8PathSearchNode* node = &m_owned_0c8[node_index];
            node->flags_00 = dynamic != 0 ? 0x0800 : 0;
            node->node_index_02 = node_index;
            node->cell_x_04 = static_cast<unsigned short>(neighbor_x);
            node->cell_z_06 = static_cast<unsigned short>(neighbor_z);
            node->path_height_08 = static_cast<unsigned short>(height);
            node->parent_node_0a = static_cast<unsigned short>(best_node);
            node->base_score_0c = vertical;
            node->path_cost_10 = path_cost;
            node->clearance_18 = clearance;
            node->position_20.x =
                (node->cell_x_04 + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
            node->position_20.y = (node->path_height_08 - 1) * span_020 + level_bounds[1];
            node->position_20.z =
                (node->cell_z_06 + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];

            unsigned short collision =
                ResolveSearchNodeCollisions00465130(movement, node_index, radius, separation);
            UpdateSearchNodeScore00464FF0(node_index, &target, clearance, radius);
            if (collision == 1) {
                node->flags_00 |= 0x0100;
                continue;
            }
            if (collision == 3 && (current->flags_00 & 0x0100) == 0) {
                stop_search = 1;
                result = 1;
                probe_cell_key_078 = node_index;
                continue;
            }
            if (collision == 2) {
                node->flags_00 |= 0x0100;
            }

            srVector3T<float> node_delta = target - node->position_20;
            float distance = node_delta.Length();
            if (flag_09c == 0) {
                if (distance < diagonal_step || distance < separation) {
                    stop_search = 1;
                    result = 1;
                }
            } else if (separation < distance) {
                stop_search = 1;
                result = 1;
            }

            if (collision == 0) {
                unsigned int score = static_cast<unsigned int>(node->score_1c);
                if (score < probe_limit_088) {
                    probe_limit_088 = score;
                    probe_cell_key_078 = node_index;
                }
            }
            entry.node_00 = node->node_index_02;
            entry.priority_04 = static_cast<unsigned int>(node->score_1c);
            heap->Insert004675B0(&entry);
            path_heap_06c->root_node_04 = heap->entries_00[0].node_00;
        }

        if (heap->size_0c == 0) {
            path_heap_06c->root_node_04 = 0;
        } else {
            path_heap_06c->root_node_04 = heap->Delete().node_00;
        }
        current->flags_00 |= 4;
        best_node = path_heap_06c->root_node_04;
        if (best_node > search_node_count_0cc) {
            char message[80];
            sprintf(message, "A:  Invalid node index %d from Queue.", best_node);
            srAssertFail("(ulBestNode <= m_ulSearchNodesUsed)", OCTPATH_CPP, 0x22ad, message);
        }
    }

    if ((attachment->flags_00 & 0x00001000) != 0) {
        if (stop_search == 0) {
            result = 3;
        }
        attachment->flags_00 &= 0xfffffff0;
        attachment->flags_00 |= result;
        return result;
    }

    unsigned char direct_path = 0;
    unsigned short direct_visibility_node = 0;
    if (probe_cell_key_078 != 0) {
        unsigned short walk = static_cast<unsigned short>(probe_cell_key_078);
        unsigned short walk_parent = m_owned_0c8[walk].parent_node_0a;
        while (walk_parent != 0) {
            if (flag_09c == 0 && direct_visibility_node == 0) {
                srVector3T<float> trace_target = movement->target_position_04c;
                trace_target.y += trace_height_offset_0bc;
                float bearing =
                    NormalizeAngle(GetHeadingAngle(&m_owned_0c8[walk].position_20, &trace_target));
                float target_yaw = NormalizeAngle(trace_target_yaw_0c4);
                srMatrix3T<float> rotation;
                rotation.SetIdentity();
                float angle = bearing - target_yaw;
                if (angle != g_zero_005ebb40) {
                    rotation.RotateAboutY(sin(angle), cos(angle));
                }
                srVector3T<float> transformed = rotation.Transform(trace_offset_0ac);
                srVector3T<float> trace_source;
                trace_source = m_owned_0c8[walk].position_20 + transformed;
                short trace = g_octree_6598a4->TraceLineOfSight(&trace_source, &trace_target, 1, -3,
                                                                -3, 1, 0);
                if (trace != 0) {
                    m_owned_0c8[walk].flags_00 |= 4;
                } else {
                    direct_visibility_node = walk;
                }
            }
            walk = walk_parent;
            walk_parent = m_owned_0c8[walk].parent_node_0a;
        }
        if (flag_09c == 0 && direct_visibility_node == 0) {
            srVector3T<float> trace_target = movement->target_position_04c;
            trace_target.y += trace_height_offset_0bc;
            float bearing =
                NormalizeAngle(GetHeadingAngle(&m_owned_0c8[walk].position_20, &trace_target));
            float target_yaw = NormalizeAngle(trace_target_yaw_0c4);
            srMatrix3T<float> rotation;
            rotation.SetIdentity();
            float angle = bearing - target_yaw;
            if (angle != g_zero_005ebb40) {
                rotation.RotateAboutY(sin(angle), cos(angle));
            }
            srVector3T<float> transformed = rotation.Transform(trace_offset_0ac);
            srVector3T<float> trace_source;
            trace_source = m_owned_0c8[walk].position_20 + transformed;
            short trace =
                g_octree_6598a4->TraceLineOfSight(&trace_source, &trace_target, 1, -3, -3, 1, 0);
            if (trace == 0) {
                direct_path = 1;
            } else {
                m_owned_0c8[walk].flags_00 |= 4;
            }
        }
    }

    if ((probe_cell_key_078 == 0 && result == 0) || direct_path != 0) {
        attachment->flags_00 |= 0x02000000;
        attachment->position_40 = movement->position_040;
        attachment->position_4c[attachment->path_position_index_08] = movement->position_040;
        attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
        g_octree_6598a4->QueueOctreeKind130042E810(movement->location_id_004,
                                                   &movement->position_040);
        g_startup_world_659c0c->radius_084 =
            g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
        attachment->flags_00 &= 0xfffffff0;
        if (flag_1cb != 0 && m_pPathModelInstance != 0) {
            BuildSearchVisualization0045CFD0();
        }
        return 0;
    }

    if (search_node_count_0cc >= g_path_reserve_0060827a || best_node == 0) {
        result = 3;
    }

    unsigned short selected = static_cast<unsigned short>(probe_cell_key_078);
    m_owned_0c8[selected].flags_00 |= 2;
    unsigned short previous = selected;
    unsigned short parent = m_owned_0c8[selected].parent_node_0a;
    while (parent != 0) {
        W8PathSearchNode* parent_node = &m_owned_0c8[parent];
        parent_node->flags_00 |= 2;
        unsigned short next_parent = parent_node->parent_node_0a;
        parent_node->parent_node_0a = previous;
        previous = parent;
        parent = next_parent;
    }
    m_owned_0c8[selected].parent_node_0a = 0;

    if (flag_1cb != 0 && m_pPathModelInstance != 0) {
        BuildSearchVisualization0045CFD0();
    }

    unsigned short route_node = previous;
    attachment->path_position_index_08 = 1;
    unsigned int prop_count = 0;
    unsigned short anchor_node = previous;
    unsigned short route_parent = m_owned_0c8[route_node].parent_node_0a;
    while (route_parent != 0) {
        W8PathSearchNode* node = &m_owned_0c8[route_parent];
        if ((node->flags_00 & 0x0800) != 0) {
            if ((attachment->flags_00 & 0x08000000) == 0) {
                int prop = ProcessSearchNodeProps004663D0(route_parent, 0);
                if (prop != 0) {
                    attachment->path_values_50[prop_count++] = static_cast<unsigned short>(prop);
                }
            } else {
                ProcessSearchNodeProps004663D0(route_parent, 1);
            }
        }

        if (CanReachSearchNode00465AF0(&m_owned_0c8[anchor_node].position_20, route_parent,
                                       radius) == 0) {
            if (static_cast<unsigned int>(attachment->path_position_index_08) + 1 >=
                attachment->capacity_0a) {
                attachment->GrowPathStorage00456BD0();
            }
            attachment->position_4c[attachment->path_position_index_08] =
                m_owned_0c8[route_node].position_20;
            attachment->path_values_50[attachment->path_position_index_08] = 0;
            ++attachment->path_position_index_08;
            attachment->flags_00 &= 0xffbfffff;
            anchor_node = route_node;
        }

        unsigned short next = node->parent_node_0a;
        if (flag_0a4 != 0) {
            if ((node->flags_00 & 4) != 0) {
                node->flags_00 |= 8;
            } else if (TestSearchPositionVisibility00464CC0(&node->position_20, movement) == 0) {
                node->flags_00 |= 8;
            } else {
                next = 0;
                result = 1;
                probe_cell_key_078 = route_parent;
            }
        }

        route_node = route_parent;
        route_parent = next;
        if (direct_visibility_node != 0 && route_node == direct_visibility_node) {
            probe_cell_key_078 = route_node;
            break;
        }
        if (route_node == 0 || m_owned_0c8[route_node].path_cost_10 > remaining_callback) {
            continue;
        }
        probe_cell_key_078 = route_node;
        break;
    }

    if (static_cast<unsigned int>(attachment->path_position_index_08) + 1 >=
        attachment->capacity_0a) {
        attachment->GrowPathStorage00456BD0();
    }
    attachment->position_4c[attachment->path_position_index_08] =
        m_owned_0c8[anchor_node].position_20;
    attachment->path_values_50[attachment->path_position_index_08] = 0;
    ++attachment->path_position_index_08;
    attachment->flags_00 &= 0xffbfffff;
    attachment->path_values_50[prop_count] = 0;

    if (attachment->path_position_index_08 > 1) {
        --attachment->path_position_index_08;
        attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
    }
    if ((attachment->flags_00 & 0x08000000) == 0) {
        attachment->flags_00 |= 0x02000000;
        attachment->position_40 = m_owned_0c8[probe_cell_key_078].position_20;
        g_octree_6598a4->QueueOctreeKind130042E810(movement->location_id_004,
                                                   &m_owned_0c8[probe_cell_key_078].position_20);
    }
    if (movement->value_010 >= 0 && flag_09c == 0) {
        AdjustFinalPathEndpoint00465D70(movement, radius, separation);
    }
    attachment->flags_00 &= 0xfffffff0;
    attachment->flags_00 |= result;
    g_startup_world_659c0c->radius_084 = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
    return result;
}

/* Plan with an explicit target position. The service flag suppresses the core
   planner's ordinary post-search callback for exactly this nested call, while
   the planner's status is passed straight back to the navigator caller. */
// FUNCTION: WIZ8 0x00464ab0
unsigned short W8PathingService::PlanMovementToPosition00464AB0(W8NavigatorMovementState* movement,
                                                                const srVector3T<float>* target,
                                                                float radius, float separation)
{
    flag_09c = 1;
    movement->target_position_04c = *target;
    unsigned short result = PlanMovement00463460(movement, radius, separation);
    flag_09c = 0;
    return result;
}

/* Refresh one planner node's distance and accumulated score. Explicit-target
   mode scores from the shared ceiling; ordinary mode starts from the node's
   base score and adds a range penalty only when the adjusted gap is positive.
   Flag 0x2000 applies the final fixed penalty in either mode. */
// FUNCTION: WIZ8 0x00464ff0
float W8PathingService::UpdateSearchNodeScore00464FF0(unsigned int node_index,
                                                      const srVector3T<float>* position,
                                                      float minimum, float maximum)
{
    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    float distance = (*position - node->position_20).Length();
    node->distance_14 = distance;

    if (flag_09c == 0) {
        node->score_1c = distance * g_float_005ec3b8 + node->base_score_0c;
    } else {
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
    } else if (gap > g_float_005ebb34) {
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
    W8NavigatorMovementState* movement, unsigned int node_index, float radius, float separation)
{
    unsigned short result = 0;
    if (flag_09c != 0) {
        separation = 0.0f;
    }

    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    srVector3T<float> blocking_direction;
    srVector3T<float> player_position = g_startup_world_659c0c->GetPosition();
    srVector3T<float> player_delta = player_position - node->position_20;
    float distance = player_delta.Length();
    float threshold = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4 + radius;
    if (movement->value_010 == 0) {
        threshold += separation;
    }

    if (distance < threshold) {
        if (movement->value_010 != 0 || flag_09c != 0) {
            return 1;
        }
        blocking_direction.Set(player_delta.x, player_delta.y, player_delta.z);
        blocking_direction.Normalize();
        result = 3;
    }

    for (unsigned int candidate = 0; candidate < path_candidate_count_094; ++candidate) {
        int location_id = path_candidates_098[candidate];
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x2622, OCTPATH_CPP, location_id, 0);
        if (monster_index == static_cast<unsigned int>(-1)) {
            continue;
        }

        unsigned int probe_index;
        for (probe_index = 0; probe_index < path_probe_count_0d4; ++probe_index) {
            W8PathProbeVolume* probe = &path_probes_0d8[probe_index];
            if (probe->tag_00 == static_cast<unsigned int>(location_id)) {
                float probe_distance = (probe->center_0c - node->position_20).Length();
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

        monster_index = MonsterGetIndexByLocationID(0x262b, OCTPATH_CPP, location_id, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info == 0 || info->monster == 0 || info->monster->state_088 == 0) {
            continue;
        }

        W8Monster* monster = info->monster;
        srVector3T<float> monster_position = monster->GetPosition();
        srVector3T<float> delta = node->position_20 - monster_position;
        float distance = delta.Length();
        threshold = monster->movement_0c0.alternate_radius_0b4 + radius;
        if (location_id == movement->value_010 && flag_09c == 0) {
            threshold += separation;
        }

        if (distance < threshold) {
            if (location_id == movement->value_010 && flag_09c == 0) {
                blocking_direction.Set(delta.x, delta.y, delta.z);
                blocking_direction.Normalize();
                result = 3;
                continue;
            }

            unsigned char adjust = 0;
            if ((node->flags_00 & 0x0200) == 0 && threshold <= distance + g_float_005ec020) {
                if (result == 3) {
                    srVector3T<float> direction = delta;
                    direction.Normalize();
                    float dot = DotProduct(direction, blocking_direction);
                    if (dot <= g_float_005ec3d0 || g_float_005ec3c8 <= dot) {
                        adjust = 1;
                    }
                } else if (result != 1) {
                    adjust = 1;
                }
            }

            if (adjust != 0) {
                delta.SetLength(threshold - distance);
                node->position_20 += delta;
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

/* Test whether a candidate search position has the configured range and line
   of sight to the movement target. The trace origin is an offset rotated from
   the candidate-to-target bearing into the configured target yaw; the trace
   endpoint is the movement target with its configured vertical adjustment. */
// FUNCTION: WIZ8 0x00464cc0
unsigned char
W8PathingService::TestSearchPositionVisibility00464CC0(const srVector3T<float>* position,
                                                       W8NavigatorMovementState* movement)
{
    W8Monster* monster =
        GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id_004));
    float distance;
    if (trace_target_location_0c0 == -1) {
        distance = monster->GetPointDistanceToPlayer004C7D50(*position);
    } else {
        W8Monster* target = GetMonsterByLocationID(trace_target_location_0c0);
        distance = monster->GetPointDistanceToMonster004C7E80(target, *position);
    }
    if (trace_max_distance_0a8 < distance) {
        return 0;
    }

    srVector3T<float> movement_target = movement->target_position_04c;
    float bearing = NormalizeAngle(GetHeadingAngle(position, &movement_target));
    float target_yaw = NormalizeAngle(trace_target_yaw_0c4);

    srMatrix3T<float> rotation;
    rotation.SetIdentity();
    float angle = bearing - target_yaw;
    if (angle != g_zero_005ebb40) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }

    srVector3T<float> transformed = rotation.Transform(trace_offset_0ac);

    srVector3T<float> trace_source = *position + transformed;
    srVector3T<float> trace_target = movement->target_position_04c;
    trace_target.y += trace_height_offset_0bc;

    unsigned char range_mode = 0;
    if (CalcRangeDistance(W8_RANGE_TOUCH) < distance && trace_mode_0b8 == 1) {
        range_mode = 1;
    }
    short trace = g_octree_6598a4->TraceLineOfSight(&trace_source, &trace_target, 1,
                                                    movement->location_id_004,
                                                    trace_target_location_0c0, 1, range_mode);
    if (trace != 1 && (trace != -1 || TraceModeRejectsNoHit0051B3F0(trace_mode_0b8) != 0)) {
        return 1;
    }
    return 0;
}

/* Configure and run one movement search. An attachment already in path mode
   first gets a direct current-to-target segment; a nearby visible target can
   collapse that segment to the current position and finish without planning.
   The optional probe runs the same core planner under its two temporary flags
   before the ordinary authoritative call. */
// FUNCTION: WIZ8 0x00464b00
unsigned short W8PathingService::ConfigureMovementSearch00464B00(
    W8NavigatorMovementState* movement, int target_location, float radius, float separation,
    float maximum_distance, srVector3T<float> trace_offset, int requested_trace_mode,
    float target_height_offset, float target_yaw, unsigned char* probe_result)
{
    trace_max_distance_0a8 = maximum_distance;
    trace_offset_0ac = trace_offset;
    trace_mode_0b8 = requested_trace_mode;
    flag_0a4 = 1;
    trace_height_offset_0bc = target_height_offset;
    trace_target_yaw_0c4 = target_yaw;
    if (target_location == 0) {
        trace_target_location_0c0 = -1;
    } else {
        trace_target_location_0c0 = target_location;
    }

    g_octree_6598a4->AdjustPosition00431DA0(&movement->target_position_04c, 1);

    unsigned short result = 0;
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    if ((attachment->flags_00 & 0x00010000) != 0) {
        srVector3T<float> delta = movement->target_position_04c - movement->position_040;
        float horizontal_clearance = srVector2T<float>(delta.x, delta.z).Length() - radius;
        float target_radius;
        if (target_location == 0) {
            target_radius = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
        } else {
            W8Monster* target = GetMonsterByLocationID(target_location);
            target_radius = target->movement_0c0.alternate_radius_0b4;
        }

        if (horizontal_clearance - target_radius <= separation &&
            TestSearchPositionVisibility00464CC0(&movement->position_040, movement) != 0) {
            attachment->InitializeSegment004563E0(&movement->position_040, &movement->position_040);
            flag_0a4 = 0;
            if (probe_result != 0) {
                *probe_result = 0;
            }
            return 0;
        }

        attachment->InitializeSegment004563E0(&movement->position_040,
                                              &movement->target_position_04c);
        attachment->separation_54 = separation;
        if (probe_result != 0) {
            attachment->flags_00 |= 0x04001000;
            result = PlanMovement00463460(movement, radius, separation);
            attachment->flags_00 &= 0xfbffefff;
            *probe_result = result == 1;
            attachment->InitializeSegment004563E0(&movement->position_040,
                                                  &movement->target_position_04c);
        }
        result = PlanMovement00463460(movement, radius, separation);
    }

    flag_0a4 = 0;
    return result;
}

/* Resolve one packed path-cell entry from the open-chained index. Compatible
   height entries must be unblocked; dynamic entries additionally require the
   caller's permission and their own enabled bit. The selected packed value
   updates height, vertical offset, compass direction, and the dynamic byte. */
// FUNCTION: WIZ8 0x004648d0
unsigned char W8PathingService::ResolvePathCell004648D0(unsigned int key,
                                                        unsigned char allow_dynamic,
                                                        unsigned int* height, float* direction,
                                                        float* vertical, unsigned char* dynamic)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    int* buckets = index->bucket_heads;
    W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = buckets[hash & (index->bucket_count - 1)];

    while (slot != -1 && entries[slot].key != key) {
        slot = entries[slot].next_index;
    }
    while (slot != -1) {
        W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
        unsigned int value = static_cast<unsigned int>(entry->value);
        int difference = (value & 0xffff) - *height;
        if (-cell_count_024 < difference && difference < cell_count_024 &&
            (value & 0x20000000) == 0 &&
            ((value & 0x10000000) == 0 || (allow_dynamic != 0 && (value & 0x08000000) != 0))) {
            int magnitude = difference;
            if (magnitude < 0) {
                magnitude = -magnitude;
            }
            *vertical += magnitude * span_020 * g_float_005ec3b8;
            *height = value & 0xffff;
            if ((value & 0x01000000) == 0) {
                *direction = static_cast<float>(value >> 16 & 0xff);
            } else {
                *direction = 0.0f;
            }
            *direction = (*direction + g_float_005ebc7c) * g_world_scale_005ebc40;
            *dynamic = (value & 0x10000000) != 0;
            return 1;
        }

        slot = entry->next_index;
        while (slot != -1 && entries[slot].key != key) {
            slot = entries[slot].next_index;
        }
    }
    return 0;
}

/* Unit X/Z components of the eight path directions, indexed by direction bit:
   0 north, 1 north-west, 2 west, 3 south-west, 4 south, 5 south-east, 6 east,
   7 north-east. */
// GLOBAL: WIZ8 0x00608280
static float s_path_direction_x_00608280[8] = {0.0f, -0.70710677f, -1.0f, -0.70710677f,
                                               0.0f, 0.70710677f,  1.0f,  0.70710677f};
// GLOBAL: WIZ8 0x006082a0
static float s_path_direction_z_006082a0[8] = {-1.0f, -0.70710677f, 0.0f, 0.70710677f,
                                               1.0f,  0.70710677f,  0.0f, -0.70710677f};

/* Sum the blocked-direction unit vectors among the directions `delta` points
   toward; the normalized sum is the slide direction. `mask` uses the
   neighbor-mask encoding: a set bit is a walkable direction and an all-open
   set is zero, which reads as "no obstacle" here. */
// FUNCTION: WIZ8 0x004664f0
unsigned char W8PathingService::ComputeFreeDirection004664F0(unsigned int mask,
                                                             const srVector3T<float>* delta,
                                                             srVector3T<float>* direction)
{
    unsigned int wanted;
    unsigned int bit;
    float length_squared;

    if (mask == 0) {
        return '\0';
    }
    wanted = 0;
    if (g_zero_005ebb40 <= delta->x) {
        if (g_zero_005ebb40 < delta->x) {
            wanted = 0xe;
        }
    } else {
        wanted = 0xe0;
    }
    if (g_zero_005ebb40 <= delta->z) {
        if (g_zero_005ebb40 < delta->z) {
            wanted = wanted | 0x83;
        }
    } else {
        wanted = wanted | 0x38;
    }
    if ((~mask & wanted) == 0) {
        return '\0';
    }
    direction->x = g_float_005ebb34;
    direction->z = g_float_005ebb34;
    for (bit = 0; bit < 8; ++bit) {
        if ((~mask & wanted & 1 << (bit & 0x1f)) != 0) {
            direction->x = direction->x + s_path_direction_x_00608280[bit];
            direction->z = direction->z + s_path_direction_z_006082a0[bit];
        }
    }
    direction->y = 0.0f;
    length_squared = direction->x * direction->x + direction->z * direction->z;
    if (length_squared != g_zero_005ebb40) {
        direction->y = 0.0f;
        direction->x = direction->x * (g_double_005ebc30 / sqrt(length_squared));
        direction->z = direction->z * (g_double_005ebc30 / sqrt(length_squared));
    }
    return '\x01';
}

/* Resolve the path cell under `position` through the path-value hash, pick the
   entry whose height is within two cell spans, and compute the slide
   direction for `delta` from its neighbor mask. */
// FUNCTION: WIZ8 0x00466600
unsigned char W8PathingService::GetNeighborSlideDirection00466600(const srVector3T<float>* position,
                                                                  const srVector3T<float>* delta,
                                                                  srVector3T<float>* direction)
{
    W8HashTable<unsigned int, unsigned int>* index;
    int range;
    int height;
    int cell[2];
    unsigned int key;
    unsigned int value;
    int slot;
    char in_range;

    range = cell_count_024 * 2;
    in_range = '\0';
    height = static_cast<int>((position->y - level_bounds[1]) / span_020) + 1;
    cell[0] = static_cast<int>((position->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = static_cast<int>((position->z - level_bounds[2]) / grid_scale_01c);
    key = cell[1] * 0x10000 + cell[0];
    index = m_pPathValues_064;
    value = 0;
    slot = index->bucket_heads[((key >> 10 ^ key) >> 10 ^ key) & (index->bucket_count - 1)];
    if (slot != -1) {
        while (index->entries[slot].key != key) {
            slot = index->entries[slot].next_index;
            if (slot == -1) {
                return '\0';
            }
        }
        while (-1 < slot) {
            if (in_range != '\0') {
                unsigned int mask = ComputeWaypointNeighborMask004667A0(cell, value);
                return ComputeFreeDirection004664F0(mask, delta, direction);
            }
            value = index->entries[slot].value;
            int difference = height - static_cast<int>(value & 0xffff);
            if (-range < difference && difference < range) {
                in_range = '\x01';
            } else {
                slot = index->entries[slot].next_index;
                while (slot != -1 && index->entries[slot].key != key) {
                    slot = index->entries[slot].next_index;
                }
            }
        }
    }
    return '\0';
}

// FUNCTION: WIZ8 0x00466990
unsigned char W8PathingService::GetObstacleDirection00466990(const srVector3T<float>* delta,
                                                             srVector3T<float>* direction)
{
    return ComputeFreeDirection004664F0(waypoint_neighbor_mask_0a0, delta, direction);
}

/* One movement step for a navigator walking an attachment path. The 0x10000
   mode follows the recorded route directly; otherwise the steering context
   advances the position and the waypoint cursor, falls through to the
   edge-transition handler, and relinks to a linked navigator's route when
   the end is reached but still out of contact. Returns whether the step
   completed the path. */
// FUNCTION: WIZ8 0x004669b0
unsigned int W8PathingService::StepAlongPath004669B0(W8NavigatorMovementState* movement,
                                                     float radius, float separation)
{
    g_navigator_position_changed_659c11 = true;
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    unsigned int flags = attachment->flags_00;
    bool arrived;
    if ((flags & 0x10000) != 0) {
        arrived = true;
        if ((flags & 0x8000000) == 0) {
            srVector3T<float>* position = &movement->position_040;
            srVector3T<float> advanced = *position;
            char on_path = attachment->AdvanceAlongPathPositions00456830(
                g_game_time_accumulator_6598bc->GetValue28() * movement->movement_speed_064 *
                    movement->movement_scale_060 * g_rate_006068EC * g_world_scale_005ebc40,
                &advanced);
            arrived = on_path == '\0';
            if (arrived) {
                attachment->flags_00 = attachment->flags_00 & 0xfffffff0;
            }
            srVector3T<float> delta;
            delta.x = advanced.x - position->x;
            delta.y = advanced.y - position->y;
            delta.z = advanced.z - position->z;
            if (((delta.x != g_float_005ebb34) || (delta.y != g_float_005ebb34)) ||
                (delta.z != g_float_005ebb34)) {
                srVector3T<float> target;
                target.x = delta.x + advanced.x;
                *position = advanced;
                attachment->position_4c[0] = advanced;
                attachment->position_34 = attachment->position_4c[0];
                attachment->position_10 = attachment->position_4c[0];
                target.y = delta.y + advanced.y;
                target.z = delta.z + advanced.z;
                movement->target_position_04c = target;
                g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004, &advanced);
            }
        }
        return arrived;
    }
    arrived = false;
    if ((flags & 0x800000) == 0) {
        W8Monster* monster =
            GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id_004));
        W8Navigator* linked = monster->linked_navigator_05c;
        if (linked == 0) {
            linked_attachment_218 = 0;
        } else {
            linked_attachment_218 = linked->movement_0c0.attachment_0ac;
        }
    } else {
        linked_attachment_218 = 0;
    }
    if ((attachment->value_04 == 1) || (attachment->path_values_50[attachment->value_04] == 0)) {
        unsigned int steer_flags = ((attachment->flags_00 >> 0x17) & 0xffffff01);
        path_parameters_214->SteerFromPathStart004CCAD0(movement, steer_flags);
        unsigned int destination_flag = attachment->flags_00 & 0x80000;
        srVector3T<float>* waypoint;
        if (destination_flag == 0) {
            if (attachment->value_04 < attachment->path_position_index_08) {
                waypoint = attachment->position_4c + attachment->value_04;
            } else {
                waypoint = &attachment->position_1c;
            }
        } else {
            waypoint = &attachment->position_28;
        }
        srVector3T<float> target = *waypoint;
        float dx = target.x - movement->position_040.x;
        float dy = target.y - movement->position_040.y;
        float dz = target.z - movement->position_040.z;
        float distance = sqrt(dx * dx + dy * dy + dz * dz);
        if ((destination_flag != 0) &&
            (distance < static_cast<float>(g_monster_poster_max_distance_005ec3d8))) {
            attachment->flags_00 = attachment->flags_00 & 0xfff7ffff;
        }
        unsigned short cursor = attachment->value_04;
        if (((cursor == 1) && (1 < attachment->path_position_index_08)) &&
            (attachment->path_values_50[2] != 0)) {
            if (attachment->CheckPositionHopHeight00456CB0(&movement->position_040) != '\0') {
                ActivateMovementTrigger0045B880(movement, '\x01');
                attachment->value_04 = attachment->value_04 + 1;
            }
        } else if ((attachment->flags_00 & 0x80000) == 0) {
            if (cursor == attachment->path_position_index_08) {
                bool in_range;
                if (movement->value_010 < 0) {
                    float limit = g_startup_near_limit_005ec000;
                    if ((attachment->flags_00 & 0x100000) != 0) {
                        limit = g_float_005ebc64;
                    }
                    in_range = limit <= distance;
                } else if (attachment->separation_54 + separation <= distance) {
                    in_range =
                        static_cast<float>(g_monster_poster_max_distance_005ec3d8) <= distance;
                } else {
                    in_range = false;
                }
                if (in_range) {
                    ActivateMovementTrigger0045B880(movement, '\0');
                } else {
                    W8NavigatorAttachment* linked_attachment = linked_attachment_218;
                    if ((linked_attachment == 0) ||
                        (sqrt((attachment->position_1c.x - linked_attachment->position_1c.x) *
                                  (attachment->position_1c.x - linked_attachment->position_1c.x) +
                              (attachment->position_1c.y - linked_attachment->position_1c.y) *
                                  (attachment->position_1c.y - linked_attachment->position_1c.y) +
                              (attachment->position_1c.z - linked_attachment->position_1c.z) *
                                  (attachment->position_1c.z - linked_attachment->position_1c.z)) <
                         static_cast<float>(g_double_005ebc30))) {
                        arrived = true;
                    } else {
                        PrepareLinkedNavigator00466FB0(movement);
                    }
                }
            } else if (distance < static_cast<float>(g_monster_poster_max_distance_005ec3d8)) {
                attachment->value_04 = cursor + 1;
            }
        }
        ActivateMovementTrigger0045B880(movement, '\0');
        g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004, &movement->position_040);
        return arrived;
    }
    unsigned int steer_flags = ((attachment->flags_00 >> 0x17) & 0xffffff01);
    char stepped = path_parameters_214->SteerAlongPath004CCB60(movement, steer_flags);
    float distance = 0.0f;
    if (stepped == '\0') {
        srVector3T<float> target;
        if ((attachment->flags_00 & 0x80000) == 0) {
            if (attachment->value_04 < attachment->path_position_index_08) {
                srVector3T<float>* waypoint = attachment->position_4c + attachment->value_04;
                target.x = waypoint->x;
                target.y = waypoint->y;
                target.z = waypoint->z;
            } else {
                target.x = attachment->position_1c.x;
                target.y = attachment->position_1c.y;
                target.z = attachment->position_1c.z;
            }
        } else {
            target.x = attachment->position_28.x;
            target.y = attachment->position_28.y;
            target.z = attachment->position_28.z;
        }
        float dx = target.x - movement->position_040.x;
        float dy = target.y - movement->position_040.y;
        target.z = target.z - movement->position_040.z;
        distance = sqrt(dy * dy + target.z * target.z + dx * dx);
        float limit;
        if (attachment->value_04 == attachment->path_position_index_08) {
            if (-1 < movement->value_010) {
                if (distance < attachment->separation_54 + separation) {
                    goto transitioned;
                }
                limit = static_cast<float>(g_monster_poster_max_distance_005ec3d8);
            } else {
                limit = g_startup_near_limit_005ec000;
                if ((attachment->flags_00 & 0x100000) != 0) {
                    limit = g_float_005ebc64;
                }
            }
        } else {
            limit = static_cast<float>(g_monster_poster_max_distance_005ec3d8);
        }
        if (limit <= distance) {
            g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004,
                                                   &movement->position_040);
            return arrived;
        }
    }
transitioned: {
    unsigned char transition = HandlePathEdgeTransition00460350(movement);
    if (transition == '\x01') {
        if (attachment->value_04 != attachment->path_position_index_08) {
            ActivateMovementTrigger0045B880(movement, '\x01');
            m_pSurfaces_048[attachment->path_values_50[attachment->value_04]].positional_14 =
                static_cast<unsigned int>(distance);
            attachment->value_04 = attachment->value_04 + 1;
            g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004,
                                                   &movement->position_040);
            return arrived;
        }
        W8NavigatorAttachment* linked_attachment = linked_attachment_218;
        if ((linked_attachment != 0) &&
            (static_cast<float>(g_double_005ebc30) <=
             sqrt((attachment->position_1c.x - linked_attachment->position_1c.x) *
                      (attachment->position_1c.x - linked_attachment->position_1c.x) +
                  (attachment->position_1c.y - linked_attachment->position_1c.y) *
                      (attachment->position_1c.y - linked_attachment->position_1c.y) +
                  (attachment->position_1c.z - linked_attachment->position_1c.z) *
                      (attachment->position_1c.z - linked_attachment->position_1c.z)))) {
            PrepareLinkedNavigator00466FB0(movement);
            g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004,
                                                   &movement->position_040);
            return arrived;
        }
    } else if (transition != '\0') {
        g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004, &movement->position_040);
        return arrived;
    }
}
    arrived = true;
    g_octree_6598a4->UpdateMonsterLocation(movement->location_id_004, &movement->position_040);
    return arrived;
}

/* Rebuild the attachment route against the linked navigator's recorded path.
   The linked attachment is resolved through the moving monster's link (or
   cleared when the 0x800000 flag or a missing link says there is none), the
   route is rebuilt from the navigator's current position to the linked
   path's next live waypoint, and the linked route's remaining positions are
   appended verbatim. Returns the path-build result. */
// FUNCTION: WIZ8 0x00466fb0
unsigned char W8PathingService::PrepareLinkedNavigator00466FB0(W8NavigatorMovementState* movement)
{
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    if ((attachment->flags_00 & 0x800000) == 0) {
        W8Monster* monster =
            GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id_004));
        W8Navigator* linked = monster->linked_navigator_05c;
        if (linked == 0) {
            linked_attachment_218 = 0;
        } else {
            linked_attachment_218 = linked->movement_0c0.attachment_0ac;
        }
    } else {
        linked_attachment_218 = 0;
    }
    W8NavigatorAttachment* linked_attachment = linked_attachment_218;
    if (linked_attachment == 0) {
        return '\0';
    }
    unsigned short index = linked_attachment->value_04;
    if (index < linked_attachment->path_position_index_08) {
        do {
            if (linked_attachment->path_values_50[index] != 0) {
                break;
            }
            index = index + 1;
        } while (index < linked_attachment->path_position_index_08);
    }
    attachment->InitializeSegment004563E0(&movement->position_040,
                                          &linked_attachment->position_4c[index]);
    unsigned char built = BuildAttachmentPath00460950(attachment, movement->unknown_000);
    if (built != '\0') {
        index = index + 1;
        attachment->value_0c = attachment->path_position_index_08 - linked_attachment->value_04;
        linked_attachment = linked_attachment_218;
        if (index <= linked_attachment->path_position_index_08) {
            do {
                unsigned short surface = linked_attachment->path_values_50[index];
                float* source = &linked_attachment->position_4c[index].x;
                if (static_cast<unsigned int>(attachment->capacity_0a) <=
                    static_cast<unsigned int>(attachment->path_position_index_08 + 1)) {
                    attachment->GrowPathStorage00456BD0();
                }
                srVector3T<float>* slot =
                    attachment->position_4c + attachment->path_position_index_08;
                slot->x = source[0];
                slot->y = source[1];
                slot->z = source[2];
                attachment->path_values_50[attachment->path_position_index_08] = surface;
                attachment->path_position_index_08 = attachment->path_position_index_08 + 1;
                attachment->flags_00 = attachment->flags_00 & 0xffbfffff;
                linked_attachment = linked_attachment_218;
                index = index + 1;
            } while (index <= linked_attachment->path_position_index_08);
        }
    }
    return built;
}

/* Give everything the service owns back. The four malloc'd tables and the
   conditional path tables go back through free, the bit sets and the two
   hash indexes through their own teardown, and the global slot the
   constructor claimed is cleared last. */
// FUNCTION: WIZ8 0x00457b10
W8PathingService::~W8PathingService()
{
    if (path_nodes_044 != 0) {
        free(path_nodes_044);
    }
    if (m_pSurfaces_048 != 0) {
        free(m_pSurfaces_048);
    }
    if (m_pEdges_04c != 0) {
        free(m_pEdges_04c);
    }
    if (m_pFileWayPoints != 0) {
        free(m_pFileWayPoints);
    }
    if (m_pPathModelInstance != 0) {
        delete m_pPathModelInstance;
    }
    if (visible_waypoints_058 != 0) {
        delete visible_waypoints_058;
    }
    if (rendered_waypoints_05c != 0) {
        delete rendered_waypoints_05c;
    }
    if (collected_waypoints_060 != 0) {
        delete collected_waypoints_060;
    }
    delete m_pPathValues_064;
    delete m_pVisitedCells_074;
    delete path_heap_06c;
    if (m_owned_0c8 != 0) {
        delete[] m_owned_0c8;
    }
    if (path_parameters_214 != 0) {
        delete path_parameters_214;
    }
    if (g_path_scratch_00659c64 != 0) {
        free(g_path_scratch_00659c64);
    }
    g_path_scratch_00659c64 = 0;
    if (m_pCondPaths != 0) {
        free(m_pCondPaths);
    }
    if (m_pulCondLookup != 0) {
        free(m_pulCondLookup);
    }
    if (m_pusCondNodeFrames != 0) {
        free(m_pusCondNodeFrames);
    }
    if (m_pulCondNodeKeys != 0) {
        free(m_pulCondNodeKeys);
    }
    if (m_pulCondNodeValues != 0) {
        free(m_pulCondNodeValues);
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
    path_nodes_044 = 0;
    size_004 = 0;
    edge_node_count_008 = 0;
    m_ulNumWayPoints = 0;
    m_ulNumWayPtLinks = 0;
    m_positional_014 = 0;
    m_positional_018 = 0;
    m_pSurfaces_048 = 0;
    m_pEdges_04c = 0;
    m_pFileWayPoints = 0;
    m_pPathModelInstance = 0;
    visible_waypoints_058 = new BitArray(100);
    rendered_waypoints_05c = new BitArray(100);
    collected_waypoints_060 = new BitArray(100);
    m_pPathValues_064 = 0;
    m_pVisitedCells_074 = 0;
    level_name = 0;
    flag_08c = 0;
    path_heap_06c = 0;
    path_cost_limit_070 = 1.0e10f;
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
    m_owned_0c8 = new W8PathSearchNode[g_path_reserve_0060827a + 0x14];
    search_node_count_0cc = 0;
    search_node_capacity_0d0 = 0;
    flag_09c = 0;
    flag_0a4 = 0;
    trace_target_location_0c0 = 0;
    trace_max_distance_0a8 = 0;
    trace_offset_0ac.SetZero();
    trace_mode_0b8 = 0;
    trace_height_offset_0bc = 0;
    trace_target_yaw_0c4 = 0;
    path_parameters_214 = new W8PathParameters();
    linked_attachment_218 = 0;
    m_pCondPaths = 0;
    m_ulNumCondPaths = 0;
    m_ulNumCondFrames = 0;
    m_ulNumCondNodes = 0;
    m_pulCondLookup = 0;
    m_pusCondNodeFrames = 0;
    m_pulCondNodeKeys = 0;
    m_pulCondNodeValues = 0;
    g_pathing_00659c60 = this;
    g_runtime_world_scale_6081e8 = 500.0f;
}

/* Take the octree's own bounds and level name. The span is the vertical extent
   of that box scaled, and the cell count is that span plus one. */
// FUNCTION: WIZ8 0x00458a50
void W8PathingService::ConfigureForLevel(int size, float grid_scale, int path_clearance,
                                         const W8BoundingBox* bounds, const char* name)
{
    size_004 = size;
    grid_scale_01c = grid_scale;
    path_clearance_028 = path_clearance;
    level_bounds[0] = bounds->minimum.x;
    level_bounds[1] = bounds->minimum.y;
    level_bounds[2] = bounds->minimum.z;
    level_bounds[3] = bounds->maximum.x;
    level_bounds[4] = bounds->maximum.y;
    level_bounds[5] = bounds->maximum.z;
    span_020 = (level_bounds[4] - level_bounds[1]) * g_path_span_scale_005ec344;
    cell_count_024 = static_cast<short>(static_cast<int>(span_020)) + 1;
    level_name = name;
}

/* Classify a waypoint from the path index cell beneath it.

   X and Z form the hash key. Entries with that key carry a one-based vertical
   cell in their low half; among candidates inside the service's vertical span,
   the closest height wins and its complete packed value is returned. */
// FUNCTION: WIZ8 0x00459c00
unsigned int W8PathingService::ClassifyWaypoint00459C00(const srVector3T<float>* position)
{
    int cell_x = static_cast<int>((position->x - level_bounds[0]) / grid_scale_01c);
    int cell_z = static_cast<int>((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int result = 0;

    if (key != 0) {
        W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
        W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
        unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
        int slot = index->bucket_heads[hash & (index->bucket_count - 1)];
        int height = static_cast<int>((position->y - level_bounds[1]) / span_020) + 1;
        int nearest = 0x0fffffff;

        while (slot != -1) {
            W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];

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

// FUNCTION: WIZ8 0x00459d60
unsigned int W8PathingService::FindPathCell00459D60(srVector3T<float>* position, unsigned int* cell,
                                                    unsigned char adjust)
{
    int path_height = static_cast<int>((position->y - level_bounds[1]) / span_020) + 1;
    unsigned int source_x =
        static_cast<unsigned int>((position->x - level_bounds[0]) / grid_scale_01c);
    unsigned int source_z =
        static_cast<unsigned int>((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int selected_x = source_x;
    unsigned int selected_z = source_z;
    unsigned int selected_key = 0;
    unsigned int selected_height = 0;
    float closest_distance = 10000000.0f;
    unsigned int key = source_z * 0x10000 + source_x;
    int slot = m_pPathValues_064->FindNextEntry(&key, -1);

    while (slot != -1) {
        unsigned int value = m_pPathValues_064->entries[slot].value;
        unsigned int height = value & 0xffff;
        int difference = path_height - height;

        if ((value & 0x10000000) == 0 && -cell_count_024 < difference &&
            difference < cell_count_024) {
            selected_key = key;
            selected_height = height;
            break;
        }
        slot = m_pPathValues_064->FindNextEntry(&key, slot);
    }

    if (selected_key == 0) {
        int direction;

        for (direction = 0; direction < 8; ++direction) {
            unsigned int candidate_x;
            unsigned int candidate_z = source_z;

            if (direction < 1 || direction > 3) {
                candidate_x = source_x;
                if (direction > 4) {
                    --candidate_x;
                }
            } else {
                candidate_x = source_x + 1;
            }
            if (direction < 2 || direction > 6) {
                ++candidate_z;
            } else if (direction > 2 && direction < 6) {
                --candidate_z;
            }

            key = candidate_z * 0x10000 + candidate_x;
            slot = m_pPathValues_064->FindNextEntry(&key, -1);
            while (slot != -1) {
                unsigned int value = m_pPathValues_064->entries[slot].value;
                unsigned int height = value & 0xffff;
                int difference = path_height - height;

                if ((value & 0x10000000) == 0 && -cell_count_024 < difference &&
                    difference < cell_count_024) {
                    float x =
                        (static_cast<float>(candidate_x) + g_float_005ebc7c) * grid_scale_01c -
                        (position->x - level_bounds[0]);
                    float z =
                        (static_cast<float>(candidate_z) + g_float_005ebc7c) * grid_scale_01c -
                        (position->z - level_bounds[2]);
                    float distance = x * x + z * z;

                    if (distance < closest_distance) {
                        selected_key = key;
                        selected_height = height;
                        selected_x = candidate_x;
                        selected_z = candidate_z;
                        closest_distance = distance;
                    }
                    break;
                }
                slot = m_pPathValues_064->FindNextEntry(&key, slot);
            }
        }
    }

    if (adjust != 0 && selected_key != 0) {
        position->y = static_cast<float>(selected_height - 1) * span_020 + level_bounds[1];
        position->x =
            (static_cast<float>(selected_x) + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z =
            (static_cast<float>(selected_z) + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }
    if (cell != 0) {
        cell[0] = selected_x;
        cell[1] = selected_z;
    }
    return selected_key;
}

/* Test whether a position lies in the vertical neighborhood represented by its
   X/Z path-index cell, optionally snapping it onto that indexed cell.

   The accepted vertical window is twice the service's cell count in either
   direction. X and Z snap to the horizontal cell centers; Y snaps to the exact
   one-based height carried by the matching packed index value. */
// FUNCTION: WIZ8 0x00462e60
unsigned char W8PathingService::SnapWaypointPosition00462E60(srVector3T<float>* position,
                                                             unsigned char snap_to_cell)
{
    int vertical_window = cell_count_024 * 2;
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((position->y - level_bounds[1]) / span_020))) +
        1;
    unsigned int cell_x = static_cast<unsigned int>(
        static_cast<int>(((position->x - level_bounds[0]) / grid_scale_01c)));
    unsigned int cell_z = static_cast<unsigned int>(
        static_cast<int>(((position->z - level_bounds[2]) / grid_scale_01c)));
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int matched_height = height;
    unsigned char found = 0;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = index->bucket_heads[hash & (index->bucket_count - 1)];

    while (slot != -1 && found == 0) {
        W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];

        if (entry->key == key) {
            matched_height = entry->value & 0xffff;
            int delta = static_cast<int>(height - matched_height);

            if (-vertical_window < delta && delta < vertical_window) {
                found = 1;
                break;
            }
        }
        slot = entry->next_index;
    }

    if (snap_to_cell != 0 && found != 0) {
        position->y = (matched_height - 1) * span_020 + level_bounds[1];
        position->x = (cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z = (cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }
    return found;
}

/* Test the first static path-index value in the position's vertical
   neighborhood. The packed direction byte becomes a world-space clearance;
   the special direction flag selects the global fallback instead. A successful
   lookup may also move the position onto the indexed cell before testing that
   clearance. */
// FUNCTION: WIZ8 0x00463040
unsigned char W8PathingService::TestPathCellClearance00463040(srVector3T<float>* position,
                                                              float clearance,
                                                              unsigned char snap_to_cell)
{
    int vertical_window = cell_count_024 * 2;
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((position->y - level_bounds[1]) / span_020))) +
        1;
    unsigned int cell_x = static_cast<unsigned int>(
        static_cast<int>(((position->x - level_bounds[0]) / grid_scale_01c)));
    unsigned int cell_z = static_cast<unsigned int>(
        static_cast<int>(((position->z - level_bounds[2]) / grid_scale_01c)));
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int packed = 0;
    unsigned int matched_height = height;
    unsigned char found = 0;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = index->bucket_heads[hash & (index->bucket_count - 1)];

    while (slot != -1 && found == 0) {
        W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];

        if (entry->key == key) {
            packed = entry->value;
            matched_height = packed & 0xffff;
            int delta = static_cast<int>(height - matched_height);

            if ((packed & 0x10000000) == 0 && -vertical_window < delta && delta < vertical_window) {
                found = 1;
                break;
            }
        }
        slot = entry->next_index;
    }

    if (found == 0) {
        return 0;
    }
    if (snap_to_cell != 0) {
        position->y = (matched_height - 1) * span_020 + level_bounds[1];
        position->x = (cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z = (cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }

    float direction = g_float_005ebb34;
    if ((packed & 0x01000000) == 0) {
        direction = static_cast<float>((packed >> 16) & 0xff);
    }
    return clearance < direction * g_world_scale_005ebc40 + grid_scale_01c * g_float_005ebc7c;
}

/* Snap a position to the closest eligible indexed height no higher than its
   own. Directional entries are ignored unless the caller explicitly permits
   them; X and Z always move to the chosen cell's center. */
// FUNCTION: WIZ8 0x00463290
unsigned char W8PathingService::SnapToLowerPathCell00463290(srVector3T<float>* position,
                                                            unsigned char allow_directional)
{
    unsigned char found = 0;
    int nearest = 10000000;
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((position->y - level_bounds[1]) / span_020))) +
        1;
    unsigned int cell_x = static_cast<unsigned int>(
        static_cast<int>(((position->x - level_bounds[0]) / grid_scale_01c)));
    unsigned int cell_z = static_cast<unsigned int>(
        static_cast<int>(((position->z - level_bounds[2]) / grid_scale_01c)));
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int matched_height = 0;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = index->bucket_heads[hash & (index->bucket_count - 1)];

    while (slot != -1) {
        W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];

        if (entry->key == key && (allow_directional != 0 || (entry->value & 0x00ff0000) == 0)) {
            unsigned int candidate_height = entry->value & 0xffff;
            int delta = static_cast<int>(height - candidate_height);

            if (delta >= 0 && delta < nearest) {
                found = 1;
                nearest = delta;
                matched_height = candidate_height;
            }
        }
        slot = entry->next_index;
    }

    if (found != 0) {
        position->y = (matched_height - 1) * span_020 + level_bounds[1];
        position->x = (cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z = (cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }
    return found;
}

/* Search the short arc between an attachment's two endpoint positions. The
   temporary index is rebuilt before the paired directed probes, so both walks
   share only the path position they discover. */
// FUNCTION: WIZ8 0x00462360
unsigned char W8PathingService::ProbeAttachmentPath00462360(W8NavigatorAttachment* attachment)
{
    float distance = (attachment->position_10 - attachment->position_1c).Length();

    if (distance > g_double_005ec3a0) {
        return 0;
    }

    flag_08c = 0;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
    if (visited->bucket_count != 0) {
        delete[] visited->bucket_heads;
        delete[] visited->entries;
    }
    visited->bucket_count = 0;
    visited->bucket_heads = 0;
    visited->entries = 0;
    visited->free_head = -1;
    visited->Grow();

    probe_cell_key_078 = 0;
    probe_limit_088 = 0;
    ProbeWaypointArc00462570(&attachment->position_10, &attachment->position_1c);
    flag_08c = 0;
    probe_limit_088 = 0xffffffff;
    ProbeWaypointArc00462570(&attachment->position_1c, &attachment->position_10);

    if (probe_cell_key_078 == 0) {
        return 0;
    }
    attachment->position_28 = probe_position_07c;
    attachment->flags_00 |= 0x00080000;
    return 1;
}

/* Sweep probes around the arc defined by a pair of waypoint positions.

   The accumulator starts perpendicular to the pair's horizontal direction.
   Every iteration probes that offset, advances by one grid-scale tangent step,
   and renormalizes to the pair's original radius. The dot product identifies
   when the sweep has passed its forward threshold and the walk stops after it
   subsequently crosses behind the starting direction. */
// FUNCTION: WIZ8 0x00462570
void W8PathingService::ProbeWaypointArc00462570(const srVector3T<float>* from,
                                                const srVector3T<float>* to)
{
    srVector3T<float> direction;
    srVector3T<float> arc;
    float radius;
    unsigned char passed_forward = 0;
    unsigned int iteration;

    direction = *to - *from;
    radius = direction.Length();
    arc.x = -direction.z;
    arc.y = 0.0f;
    arc.z = direction.x;

    for (iteration = 0; iteration < 50000; ++iteration) {
        srVector3T<float> probe;
        srVector3T<float> step;
        float dot;

        probe = *from + arc;
        ProbeWaypointSegment00462750(from, &probe);

        step.x = arc.z;
        step.y = arc.y;
        step.z = -arc.x;
        step.SetLength(grid_scale_01c);
        arc += step;

        arc.SetLength(radius);

        dot = DotProduct(direction, arc);
        if (dot > g_float_005ec390) {
            passed_forward = 1;
        } else if (passed_forward == 0) {
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
void W8PathingService::GetPathGridStepDirections0045AEE0(const W8PathGridWalk* walk,
                                                         int* directions)
{
    if (walk->major_axis_18 != 0) {
        if (walk->step_0c[1] < 1) {
            directions[0] = 4;
            if (walk->step_0c[0] > 0) {
                directions[1] = 2;
                return;
            }
        } else {
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
    } else {
        directions[0] = 2;
    }
    if (walk->step_0c[1] > 0) {
        directions[1] = 0;
    } else {
        directions[1] = 4;
    }
}

/* Build the two-dimensional Bresenham record used to walk path-index cells.

   Coordinates are first converted to integer distances from the level origin.
   The larger absolute delta drives the walk; the start-cell remainder fixes
   how far each axis is from its next boundary and therefore the initial error. */
// FUNCTION: WIZ8 0x0045af60
void W8PathingService::BuildPathGridWalk0045AF60(const srVector2T<float>* from,
                                                 const srVector2T<float>* to,
                                                 const srVector2T<float>* origin,
                                                 W8PathGridWalk* walk)
{
    int cell_size = static_cast<int>(grid_scale_01c);
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
        coordinate[axis] = static_cast<int>((&from->x)[axis] - (&origin->x)[axis]);
        destination[axis] = static_cast<int>((&to->x)[axis] - (&origin->x)[axis]);

        int delta = destination[axis] - coordinate[axis];
        boundary_offset[axis] = coordinate[axis] % cell_size / grid_scale_01c;
        signed_delta[axis] = delta;

        if (delta < 0) {
            step[axis] = -1;
            delta = -delta;
        } else {
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
    int error_delta = static_cast<int>((ratio < 0.0f ? -ratio : ratio) * grid_scale_01c);
    int error = static_cast<int>(cell_size * boundary_offset[minor_axis] -
                                 error_delta * boundary_offset[major_axis]);
    int count;

    if (largest_delta % cell_size == 0) {
        count = largest_delta / cell_size;
    } else {
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
unsigned char W8PathingService::ProbeWaypointSegment00462750(const srVector3T<float>* from,
                                                             const srVector3T<float>* to)
{
    float distance = (*to - *from).Length();

    if (grid_scale_01c + grid_scale_01c > distance) {
        return 1;
    }

    int cell[2];
    srVector2T<float> walk_from;
    srVector2T<float> walk_to;
    srVector2T<float> origin;
    W8PathGridWalk walk;
    int directions[2];

    cell[0] = static_cast<int>((from->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = static_cast<int>((from->z - level_bounds[2]) / grid_scale_01c);
    walk_from.x = from->x;
    walk_from.y = from->z;
    walk_to.x = to->x;
    walk_to.y = to->z;
    origin.x = level_bounds[0];
    origin.y = level_bounds[2];
    BuildPathGridWalk0045AF60(&walk_from, &walk_to, &origin, &walk);
    GetPathGridStepDirections0045AEE0(&walk, directions);

    int error = walk.error_2c;
    unsigned char bounded_probe = flag_08c != 0 && probe_limit_088 != 0;
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((from->y - level_bounds[1]) / span_020))) + 1;
    unsigned char blocked = 0;
    int iteration = 0;

    while (iteration < walk.count_24 && blocked == 0) {
        unsigned int cell_key = cell[1] * 0x10000 + cell[0];
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        W8OctreeIndex* visited_index = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
        W8OctreeEntry* visited_entries = visited_index->entries;
        int slot = visited_index->bucket_heads[hash & (visited_index->bucket_count - 1)];
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

        if (iteration == 0 || (visited & 0xffff0000) != 0xffff0000 || bounded_probe != 0) {
            W8HashTable<unsigned int, unsigned int>* path_index = m_pPathValues_064;
            W8HashEntry<unsigned int, unsigned int>* path_entries = path_index->entries;
            slot = path_index->bucket_heads[hash & (path_index->bucket_count - 1)];

            while (slot != -1) {
                W8HashEntry<unsigned int, unsigned int>* entry = &path_entries[slot];

                if (entry->key == cell_key) {
                    path_value = entry->value;
                    int height_delta = (path_value & 0xffff) - height;

                    if ((path_value & 0x10000000) == 0 && -cell_count_024 < height_delta &&
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
            } else if (bounded_probe == 0 && ((probe_limit_088 == 0 && visited == 0) ||
                                              (probe_limit_088 != 0 && (visited & 0xffff) != 0 &&
                                               (visited & 0xffff0000) == 0))) {
                srVector3T<float> position;
                unsigned int initial_cost;

                position.x = (cell[0] + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
                position.y = (height - 1) * span_020 + level_bounds[1];
                position.z = (cell[1] + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];

                initial_cost = static_cast<unsigned int>(
                    static_cast<int>(((position - *to).Length() * g_double_005ec3b0)));

                if (probe_limit_088 == 0) {
                    if (visited_index->free_head == -1) {
                        visited_index->Grow();
                    }
                    int inserted = visited_index->free_head;
                    W8OctreeEntry* entries = visited_index->entries;
                    visited_index->free_head = entries[inserted].next_index;
                    entries[inserted].key = cell_key;
                    entries[inserted].value = initial_cost;
                    unsigned int bucket = hash & (visited_index->bucket_count - 1);
                    entries[inserted].next_index = visited_index->bucket_heads[bucket];
                    visited_index->bucket_heads[bucket] = inserted;
                } else {
                    unsigned int step_cost = static_cast<unsigned int>(
                        static_cast<int>(initial_cost * g_double_005ec3a8));
                    unsigned int total_cost = (visited & 0xffff) + step_cost;

                    if (total_cost < probe_limit_088) {
                        probe_limit_088 = total_cost;
                        probe_cell_key_078 = cell_key;
                        probe_position_07c = position;
                    }

                    int* bucket =
                        visited_index->bucket_heads + (hash & (visited_index->bucket_count - 1));
                    int removed = *bucket;
                    int previous = -1;
                    W8OctreeEntry* entries = visited_index->entries;

                    while (removed != -1) {
                        W8OctreeEntry* entry = &entries[removed];
                        if (entry->key == cell_key &&
                            static_cast<unsigned int>(entry->value) == visited) {
                            if (previous == -1) {
                                *bucket = entry->next_index;
                            } else {
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
                        visited_index->Grow();
                    }
                    int inserted = visited_index->free_head;
                    entries = visited_index->entries;
                    visited_index->free_head = entries[inserted].next_index;
                    entries[inserted].key = cell_key;
                    entries[inserted].value = step_cost << 16 | visited;
                    unsigned int bucket_index = hash & (visited_index->bucket_count - 1);
                    entries[inserted].next_index = visited_index->bucket_heads[bucket_index];
                    visited_index->bucket_heads[bucket_index] = inserted;
                }
            }
        } else {
            blocked = 1;
        }

        unsigned int direction;
        if (error >= 0 || blocked != 0) {
            direction = directions[0];
            cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
            error -= walk.error_28;
        } else {
            direction = directions[1];
            --iteration;
            cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
            error += walk.cell_size_30;
        }
        if (direction_mask != 0 && (direction_mask & 1 << (direction & 0x1f)) == 0) {
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
unsigned int W8PathingService::ComputeWaypointNeighborMask004667A0(const int* cell,
                                                                   unsigned int path_value)
{
    unsigned int source_directions = 0;
    if ((path_value & 0x01000000) != 0) {
        source_directions = path_value >> 16 & 0xff;
    }

    unsigned int result = 0;
    int direction;
    for (direction = 0; direction < 8; ++direction) {
        if ((path_value & 0x01000000) == 0 || (source_directions & 1 << (direction & 0x1f)) != 0) {
            int neighbor[2];
            neighbor[0] = cell[0];
            neighbor[1] = cell[1];

            if (direction >= 1 && direction <= 3) {
                ++neighbor[0];
            } else if (direction > 4) {
                --neighbor[0];
            }
            if (direction < 2 || direction > 6) {
                ++neighbor[1];
            } else if (direction > 2 && direction < 6) {
                --neighbor[1];
            }

            unsigned int key = neighbor[1] * 0x10000 + neighbor[0];
            unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
            W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
            W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
            int slot = index->bucket_heads[hash & (index->bucket_count - 1)];
            unsigned char found = 0;

            while (slot != -1) {
                W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
                if (entry->key == key) {
                    unsigned int candidate = entry->value;
                    int height_delta = (path_value & 0xffff) - (candidate & 0xffff);
                    if ((candidate & 0x10000000) == 0 && -cell_count_024 < height_delta &&
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

    if (static_cast<unsigned char>(result) == 0xff) {
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
unsigned char W8PathingService::TestWaypointSpan0045A1B0(const srVector3T<float>* source,
                                                         srVector3T<float>* destination,
                                                         unsigned char adjust_destination,
                                                         unsigned char diagonal_steps)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    unsigned char blocked = 0;

    if (adjust_destination == 0 &&
        (source->x < level_bounds[0] || source->y < level_bounds[1] ||
         source->z < level_bounds[2] || level_bounds[3] < source->x ||
         level_bounds[4] < source->y || level_bounds[5] < source->z ||
         destination->x < level_bounds[0] || destination->y < level_bounds[1] ||
         destination->z < level_bounds[2] || level_bounds[3] < destination->x ||
         level_bounds[4] < destination->y || level_bounds[5] < destination->z)) {
        return 0;
    }

    flag_23c = 0;
    int cell[2];
    cell[0] = static_cast<int>((source->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = static_cast<int>((source->z - level_bounds[2]) / grid_scale_01c);
    unsigned int cell_key = cell[1] * 0x10000 + cell[0];
    int destination_x = static_cast<int>((destination->x - level_bounds[0]) / grid_scale_01c);
    int destination_z = static_cast<int>((destination->z - level_bounds[2]) / grid_scale_01c);
    unsigned int destination_key = destination_z * 0x10000 + destination_x;
    waypoint_neighbor_mask_0a0 = 0;

    W8HashTable<unsigned int, unsigned int>* path_index = m_pPathValues_064;
    W8HashEntry<unsigned int, unsigned int>* entries = path_index->entries;

    if (cell_key == destination_key) {
        unsigned int height = static_cast<unsigned int>(
                                  static_cast<int>(((source->y - level_bounds[1]) / span_020))) +
                              1;
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        int slot = path_index->bucket_heads[hash & (path_index->bucket_count - 1)];
        unsigned int source_value = 0;
        unsigned char found = 0;

        while (slot != -1) {
            W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
            if (entry->key == cell_key) {
                unsigned int value = entry->value;
                int difference = (value & 0xffff) - height;
                if ((value & 0x10000000) == 0 && -cell_count_024 < difference &&
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

        waypoint_neighbor_mask_0a0 = ComputeWaypointNeighborMask004667A0(cell, source_value);
        height = static_cast<unsigned int>(
                     static_cast<int>(((destination->y - level_bounds[1]) / span_020))) +
                 1;
        slot = path_index->bucket_heads[hash & (path_index->bucket_count - 1)];
        found = 0;

        while (slot != -1) {
            W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
            if (entry->key == cell_key) {
                unsigned int value = entry->value;
                int difference = (value & 0xffff) - height;
                if ((value & 0x10000000) == 0 && -cell_count_024 < difference &&
                    difference < cell_count_024) {
                    found = value == source_value;
                    break;
                }
            }
            slot = entry->next_index;
        }
        return found;
    }

    srVector2T<float> walk_source;
    srVector2T<float> walk_destination;
    srVector2T<float> origin;
    W8PathGridWalk walk;
    int directions[2];
    walk_source.x = source->x;
    walk_source.y = source->z;
    walk_destination.x = destination->x;
    walk_destination.y = destination->z;
    origin.x = level_bounds[0];
    origin.y = level_bounds[2];
    BuildPathGridWalk0045AF60(&walk_source, &walk_destination, &origin, &walk);
    GetPathGridStepDirections0045AEE0(&walk, directions);

    int error = walk.error_2c;
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((source->y - level_bounds[1]) / span_020))) + 1;
    unsigned int previous_key = 0;
    unsigned int previous_value;
    int iteration = 0;

    while (iteration < walk.count_24 && blocked == 0) {
        unsigned int hash = (cell_key >> 10 ^ cell_key) >> 10 ^ cell_key;
        int slot = path_index->bucket_heads[hash & (path_index->bucket_count - 1)];
        unsigned int direction_mask = 0;
        unsigned int path_value;
        unsigned char found = 0;

        while (slot != -1) {
            W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
            if (entry->key == cell_key) {
                path_value = entry->value;
                int difference = (path_value & 0xffff) - height;

                if (-cell_count_024 < difference && difference < cell_count_024) {
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
            } else {
                cell[walk.minor_axis_1c] += walk.step_0c[walk.minor_axis_1c];
                direction = directions[1];
                --iteration;
                error += walk.cell_size_30;
            }
        } else {
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
                } else {
                    direction = (directions[0] + directions[1]) / 2;
                }
            }
            cell[walk.major_axis_18] += walk.step_0c[walk.major_axis_18];
            error -= walk.error_28;
        }

    stepped:
        if (cell_key == destination_key) {
            iteration = walk.count_24;
        } else if (direction_mask != 0 && (direction_mask & 1 << (direction & 0x1f)) == 0) {
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
                static_cast<int>((destination->y - level_bounds[1]) / span_020) + 1;
            int difference = destination_height - height;
            if (difference < -cell_count_024 || cell_count_024 < difference) {
                blocked = 1;
            }
        }
    } else if (previous_key == 0) {
        *destination = *source;
    } else {
        if (blocked != 0) {
            destination->x =
                ((previous_key & 0xffff) + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
            destination->z =
                ((previous_key >> 16) + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
        }
        destination->y = (height - 1) * span_020 + level_bounds[1];
    }

    return blocked == 0;
#pragma clang diagnostic pop
}

/* Compare clearance along the two compass rays bracketing a horizontal
   direction. The normalized Z component selects the pair; the sign of X
   selects which half of the compass owns the middle bands. */
// FUNCTION: WIZ8 0x0045aac0
float W8PathingService::CompareDirectionalClearance0045AAC0(const srVector3T<float>* position,
                                                            const srVector3T<float>* direction,
                                                            float distance)
{
    float normalized_x = direction->x;
    float normalized_z = direction->z;
    srVector2T<float> horizontal(normalized_x, normalized_z);
    horizontal.Normalize();
    normalized_x = horizontal.x;
    normalized_z = horizontal.y;

    int first_direction;
    int second_direction;
    if (normalized_x <= g_float_005ebb34) {
        if (g_path_direction_threshold_3_005ec354 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        } else if (g_path_direction_threshold_2_005ec350 < normalized_z) {
            first_direction = 6;
            second_direction = 0;
        } else if (normalized_z <= g_path_direction_threshold_1_005ec34c) {
            if (normalized_z <= g_path_direction_threshold_0_005ec348) {
                first_direction = 3;
                second_direction = 5;
            } else {
                first_direction = 4;
                second_direction = 6;
            }
        } else {
            first_direction = 5;
            second_direction = 7;
        }
    } else {
        if (g_path_direction_threshold_3_005ec354 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        } else if (g_path_direction_threshold_2_005ec350 < normalized_z) {
            first_direction = 0;
            second_direction = 2;
        } else if (g_path_direction_threshold_1_005ec34c < normalized_z) {
            first_direction = 1;
            second_direction = 3;
        } else if (g_path_direction_threshold_0_005ec348 < normalized_z) {
            first_direction = 2;
            second_direction = 4;
        } else {
            first_direction = 3;
            second_direction = 5;
        }
    }

    int cell[2];
    cell[0] = static_cast<int>((position->x - level_bounds[0]) / grid_scale_01c);
    cell[1] = static_cast<int>((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int height =
        static_cast<unsigned int>(static_cast<int>(((position->y - level_bounds[1]) / span_020))) +
        1;
    float first = MeasureDirectionalPath0045AC70(cell, first_direction, height, distance);
    float second = MeasureDirectionalPath0045AC70(cell, second_direction, height, distance);
    return second - first;
}

/* Measure how much of a requested run remains traversable in one compass
   direction. Cardinal runs use the retail diagonal-to-axis scale before cell
   stepping; every crossed cell must carry a vertically compatible record whose
   explicit direction mask, when present, permits the same direction. */
// FUNCTION: WIZ8 0x0045ac70
float W8PathingService::MeasureDirectionalPath0045AC70(const int* cell, int direction,
                                                       unsigned int height, float distance)
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
        W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
        W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
        int slot = index->bucket_heads[hash & (index->bucket_count - 1)];
        unsigned int path_value;
        unsigned char found = 0;

        while (slot != -1) {
            W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
            if (entry->key == key) {
                path_value = entry->value;
                int difference = (path_value & 0xffff) - height;
                if ((path_value & 0x10000000) == 0 && -cell_count_024 < difference &&
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
        if (found == 0 || ((path_value & 0x01000000) != 0 &&
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
unsigned short W8PathingService::FindWaypoint0045B120(const srVector3T<float>* position,
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
    srVector3T<float> half_extent;
    half_extent.Set(g_float_005ec360, g_float_005ec35c, g_float_005ec360);
    lower = query - half_extent;
    upper = query + half_extent;

    unsigned long* candidates = 0;
    int count =
        g_octree_6598a4->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);
    if (count == 0) {
        return result;
    }
    if (static_cast<unsigned int>(count) >= 200) {
        srAssertFail("s_ulCount<200", OCTPATH_CPP, 0xe0c, "Too many nodes in list");
    }

    unsigned int distances[199];
    int index;
    if (count > 1) {
        for (index = 0; index < count; ++index) {
            const srVector3T<float>* candidate = &m_pSurfaces_048[candidates[index]].position_04;
            srVector3T<float> delta = query - *candidate;
            distances[index] = static_cast<unsigned int>(static_cast<int>(delta.Length()));

            if (distances[index] < g_float_005ebc64 &&
                srVector2T<float>(delta.x, delta.z).Length() < g_double_005ec150) {
                result = static_cast<unsigned short>(candidates[index]);
            }
        }

        if (result == 0) {
            for (index = 1; index < count; ++index) {
                unsigned int distance = distances[index];
                int candidate = candidates[index];
                int insertion = index;

                while (insertion > 0 && distance < distances[insertion - 1]) {
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
        if (TestWaypointSpan0045A1B0(&query, &m_pSurfaces_048[candidates[index]].position_04, 0,
                                     0) != 0) {
            result = static_cast<unsigned short>(candidates[index]);
        }
    }

    if (result == 0 && exhaustive != 0) {
        probe_position_07c.SetZero();

        W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
        if (visited->bucket_count != 0) {
            delete[] visited->bucket_heads;
            delete[] visited->entries;
        }
        visited->bucket_count = 0;
        visited->bucket_heads = 0;
        visited->entries = 0;
        visited->free_head = -1;
        visited->Grow();
        value_1d4 = 0;
        flag_08c = 0;

        for (index = 0; index < count; ++index) {
            if (value_1d4 != 0) {
                return result;
            }

            unsigned char saved_flag = flag_08c;
            visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
            flag_08c = 0;
            if (visited->bucket_count != 0) {
                delete[] visited->bucket_heads;
                delete[] visited->entries;
            }
            visited->bucket_count = 0;
            visited->bucket_heads = 0;
            visited->entries = 0;
            visited->free_head = -1;
            visited->Grow();

            probe_cell_key_078 = 0;
            probe_limit_088 = 0;
            srVector3T<float>* candidate = &m_pSurfaces_048[candidates[index]].position_04;
            ProbeWaypointArc00462570(&query, candidate);
            flag_08c = saved_flag;
            probe_limit_088 = 0xffffffff;
            ProbeWaypointArc00462570(candidate, &query);
            if (probe_cell_key_078 != 0) {
                value_1d4 = static_cast<unsigned short>(candidates[index]);
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
void W8PathingService::SnapPathHeight0045B5A0(srVector3T<float>* position)
{
    int cell_x = static_cast<int>((position->x - level_bounds[0]) / grid_scale_01c);
    int cell_z = static_cast<int>((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;

    if (key == 0) {
        return;
    }

    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues_064;
    W8HashEntry<unsigned int, unsigned int>* entries = index->entries;
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = index->bucket_heads[hash & (index->bucket_count - 1)];
    int height = static_cast<int>((position->y - level_bounds[1]) / span_020) + 1;

    while (slot != -1) {
        W8HashEntry<unsigned int, unsigned int>* entry = &entries[slot];
        if (entry->key == key) {
            unsigned int matched_height = entry->value & 0xffff;
            int difference = static_cast<int>(matched_height) - height;
            if (-cell_count_024 < difference && difference < cell_count_024) {
                position->y = (matched_height - 1) * span_020 + level_bounds[1];
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
void W8PathingService::GetPathSurfaceNormal0045B730(const srVector3T<float>* position,
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

    *normal = CrossProduct(first - middle, second - middle);
    normal->Normalize();
}

/* Activate the eligible trigger prop intersecting a navigator's next path
   segment.

   Ordinary movement supplies a box from the current position to twice the
   velocity. Edge mode instead resolves the attachment's current waypoint pair
   and accepts only an edge carrying both dynamic bits. Kind-eight octree hits
   are filtered to active props whose trigger owner permits this activation;
   when several remain, the owner nearest the segment midpoint wins. */
// FUNCTION: WIZ8 0x0045b880
void W8PathingService::ActivateMovementTrigger0045B880(W8NavigatorMovementState* movement,
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
        upper = movement->position_040 + movement->velocity_034 * 2.0;
    } else {
        W8NavigatorAttachment* attachment = movement->attachment_0ac;

        /* Retail reaches the shared query with the local bounds untouched
           when this cursor is exhausted. Keep that source-level fallthrough;
           callers normally enter edge mode only while a pair remains. */
        if (attachment->value_04 < attachment->path_position_index_08) {
            unsigned short* pairs = attachment->path_values_50;
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
            if ((flags & 0x10000000) == 0 || (flags & 0x80000000) == 0) {
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

    unsigned long* candidates = 0;
    int count = g_octree_6598a4->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
    if (count <= 0) {
        return;
    }

    Trigger* selected = 0;
    if (count == 1) {
        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[0]);
        Trigger* trigger = prop->GetGDPropValue24();
        if (prop->GetSetting6C() == 0 || trigger == 0 || (trigger->flags_0a0 & 0x100) == 0) {
            return;
        }
        selected = trigger;
    } else {
        srVector3T<float> midpoint;
        midpoint = (upper + lower) * g_double_005ebe80;
        double nearest_distance = 1e32;

        for (int index = 0; index < count; ++index) {
            W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
            Trigger* trigger = prop->GetGDPropValue24();
            if (prop->GetSetting6C() != 0 && trigger != 0 && (trigger->flags_0a0 & 0x100) != 0) {
                srVector3T<float> center;
                prop->GetCenterPosition(&center);
                srVector3T<float> difference = center - midpoint;
                double distance = difference.LengthSquared();
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
void W8PathingService::UpdatePathVisualization0045BC40(const srVector3T<float>* source,
                                                       const srVector3T<float>* destination)
{
    W8World* world = GetWorld();
    srNode* node = m_pPathModelInstance;

    if (flag_1c8 != 0) {
        srVector3T<float> adjusted = *source;
        srVector3T<float> endpoint = *destination;
        g_octree_6598a4->AdjustPosition00431DA0(&adjusted, 1);
        PreparePathVisualization0045E840(&adjusted, &endpoint);

        if (CollectPathVisualization0045D880(&adjusted) != 0) {
            if (m_pPathModelInstance != 0) {
                BuildPathVisualization0045BE30();
                m_pPathModelInstance->clearFlag(srNode::FLAG_DISABLE);
                return;
            }

            m_pPathModelInstance = BuildPathVisualization0045BE30();
            node = m_pPathModelInstance;
            if (node != 0) {
                node->setParent(world->dynamic_scene, 1);
                node->clearFlag(srNode::FLAG_DISABLE);
                return;
            }
            node->clearFlag(srNode::FLAG_DISABLE);
            return;
        }

        node = m_pPathModelInstance;
        if (node != 0) {
            node->setFlag(srNode::FLAG_DISABLE);
            node->setFlag(srNode::FLAG_TERMINATE);
        }
        return;
    }

    if (flag_1c9 == 0 && flag_1cb == 0) {
        DrawPathPosition0045C9A0(*source, 0);
        node = m_pPathModelInstance;
        if (node != 0) {
            node->setFlag(srNode::FLAG_DISABLE);
            node->setFlag(srNode::FLAG_TERMINATE);
        }
        return;
    }

    if (flag_1cb != 0) {
        if (m_pPathModelInstance == 0) {
            EnsurePathVisualization0045D530();
            node = m_pPathModelInstance;
            node->setParent(world->dynamic_scene, 1);
            node->setFlag(srNode::FLAG_TERMINATE);
            if (m_pPathModelInstance == 0) {
                node->clearFlag(srNode::FLAG_DISABLE);
                return;
            }
        }

        srVector3T<float> adjusted = *source;
        g_octree_6598a4->AdjustPosition00431DA0(&adjusted, 1);
        DrawPathPosition0045C9A0(adjusted, 1);
    }

    m_pPathModelInstance->clearFlag(srNode::FLAG_DISABLE);
}

/* Populate the editor mesh from the currently visible waypoint set. Marker
   geometry occupies the first hundred six-polygon groups; directed links use
   the following groups and are emitted once when a visible reverse edge
   exists. */
// FUNCTION: WIZ8 0x0045BE30
stModelInstance* W8PathingService::BuildPathVisualization0045BE30()
{
    static srVector3T<float> marker_offsets[5] = {
        srVector3T<float>(0.0f, 0.5f, 0.0f), srVector3T<float>(-0.25f, 0.0f, -0.25f),
        srVector3T<float>(-0.25f, 0.0f, 0.25f), srVector3T<float>(0.25f, 0.0f, 0.25f),
        srVector3T<float>(0.25f, 0.0f, -0.25f)};
    static unsigned char marker_offsets_scaled = 0;
    int index;

    if (marker_offsets_scaled == 0) {
        for (index = 0; index < 5; ++index) {
            marker_offsets[index].x *= static_cast<float>(g_double_005ec150);
            marker_offsets[index].y *= static_cast<float>(g_double_005ec150);
            marker_offsets[index].z *= static_cast<float>(g_double_005ec150);
        }
        marker_offsets_scaled = 1;
    }
    if (m_pPathModelInstance == 0) {
        EnsurePathVisualization0045D530();
    }

    stMeshModel* model = static_cast<stMeshModel*>(m_pPathModelInstance->model());
    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    srVector3T<float>* vertices = model->getVertexLoc();
    srVector3i* polygons = model->getPolyVertex();
    int marker_count = 0;
    int link_count = 0;
    int next = visible_waypoints_058->NextSetBit(1);

    rendered_waypoints_05c->ClearAll();
    while (next != 0 && marker_count < 100) {
        unsigned short source_index = static_cast<unsigned short>(next - 1);
        W8PathSurface* source = &m_pSurfaces_048[source_index];
        srVector3T<float> marker_color;
        float marker_scale = (source->flags_00 >> 12) * static_cast<float>(g_double_005ec378);
        int marker_vertex = marker_count * 5;

        rendered_waypoints_05c->SetAndGrow(source_index);
        GetWaypointVisualizationColor0045D490(source_index, &marker_color);
        for (index = 0; index < 5; ++index) {
            vertices[marker_vertex + index].x =
                source->position_04.x + marker_offsets[index].x * marker_scale;
            vertices[marker_vertex + index].y =
                source->position_04.y + static_cast<float>(g_double_005ec150) +
                marker_offsets[index].y * (source->flags_00 >> 12) * 0.5f;
            vertices[marker_vertex + index].z =
                source->position_04.z + marker_offsets[index].z * marker_scale;
            colors[marker_vertex + index] = marker_color;
        }
        if ((source->flags_00 & 2) != 0) {
            colors[marker_vertex].x = colors[marker_vertex].x <= g_float_005ebb34 ? 1.0f : 0.0f;
            colors[marker_vertex].y = colors[marker_vertex].y <= g_float_005ebb34 ? 1.0f : 0.0f;
            colors[marker_vertex].z = colors[marker_vertex].z <= g_float_005ebb34 ? 1.0f : 0.0f;
        } else if ((source->flags_00 & 0x40) != 0) {
            colors[marker_vertex].SetZero();
        }

        unsigned short edge_index = source->first_edge_24;
        while (edge_index != 0 && link_count * 6 + 504 <= 0x30d1) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short destination_index = edge->destination_06;
            W8PathSurface* destination = &m_pSurfaces_048[destination_index];
            unsigned short reverse_index = destination->first_edge_24;
            unsigned char reverse_found = 0;

            while (reverse_index != 0 && reverse_found == 0) {
                if (m_pEdges_04c[reverse_index].destination_06 == source_index) {
                    reverse_found = 1;
                } else {
                    reverse_index = m_pEdges_04c[reverse_index].next_0c;
                }
            }

            if (!rendered_waypoints_05c->Test(destination_index) &&
                !visible_waypoints_058->Test(destination_index) && marker_count + 1 < 100) {
                srVector3T<float> destination_color;
                float destination_scale =
                    (destination->flags_00 >> 12) * static_cast<float>(g_double_005ec378);
                int destination_vertex = (marker_count + 1) * 5;

                GetWaypointVisualizationColor0045D490(destination_index, &destination_color);
                for (index = 0; index < 5; ++index) {
                    vertices[destination_vertex + index].x =
                        destination->position_04.x + marker_offsets[index].x * destination_scale;
                    vertices[destination_vertex + index].y =
                        destination->position_04.y + static_cast<float>(g_double_005ec150) +
                        marker_offsets[index].y * (destination->flags_00 >> 12) * 0.5f;
                    vertices[destination_vertex + index].z =
                        destination->position_04.z + marker_offsets[index].z * destination_scale;
                    colors[destination_vertex + index] = destination_color;
                }
                if ((source->flags_00 & 2) != 0) {
                    colors[destination_vertex].x =
                        colors[destination_vertex].x <= g_float_005ebb34 ? 1.0f : 0.0f;
                    colors[destination_vertex].y =
                        colors[destination_vertex].y <= g_float_005ebb34 ? 1.0f : 0.0f;
                    colors[destination_vertex].z =
                        colors[destination_vertex].z <= g_float_005ebb34 ? 1.0f : 0.0f;
                } else if ((source->flags_00 & 0x40) != 0) {
                    colors[destination_vertex].SetZero();
                }
                rendered_waypoints_05c->SetAndGrow(destination_index);
                ++marker_count;
            }

            if (source_index < destination_index ||
                !visible_waypoints_058->Test(destination_index) || reverse_found == 0) {
                int base_vertex = 500 + link_count * 6;
                int base_polygon = 600 + link_count * 6;
                srVector2T<float> perpendicular(
                    -(destination->position_04.z - source->position_04.z),
                    destination->position_04.x - source->position_04.x);
                perpendicular.SetLength(g_double_005ec368);
                float perpendicular_x = perpendicular.x;
                float perpendicular_z = perpendicular.y;

                polygons[base_polygon].x = base_vertex;
                polygons[base_polygon].y = base_vertex + 1;
                polygons[base_polygon].z = base_vertex + 3;
                polygons[base_polygon + 1].x = base_vertex + 1;
                polygons[base_polygon + 1].y = base_vertex + 2;
                polygons[base_polygon + 1].z = base_vertex + 4;
                polygons[base_polygon + 2].x = base_vertex + 2;
                polygons[base_polygon + 2].y = base_vertex;
                polygons[base_polygon + 2].z = base_vertex + 5;
                polygons[base_polygon + 3].x = base_vertex + 4;
                polygons[base_polygon + 3].y = base_vertex + 3;
                polygons[base_polygon + 3].z = base_vertex + 1;
                polygons[base_polygon + 4].x = base_vertex + 5;
                polygons[base_polygon + 4].y = base_vertex + 4;
                polygons[base_polygon + 4].z = base_vertex + 2;
                polygons[base_polygon + 5].x = base_vertex + 3;
                polygons[base_polygon + 5].y = base_vertex + 5;
                polygons[base_polygon + 5].z = base_vertex;

                vertices[base_vertex] = source->position_04;
                vertices[base_vertex].y += g_float_005ec370;
                vertices[base_vertex + 3] = destination->position_04;
                vertices[base_vertex + 3].y += g_float_005ec370;
                srVector3T<float> offset;
                offset.Set(perpendicular_x, g_world_scale_005ebc40, perpendicular_z);
                vertices[base_vertex + 1] = source->position_04 + offset;
                vertices[base_vertex + 2] = source->position_04 - offset;
                vertices[base_vertex + 4] = destination->position_04 + offset;
                vertices[base_vertex + 5] = destination->position_04 - offset;

                for (index = 0; index < 6; ++index) {
                    colors[base_vertex + index].SetZero();
                }
                if ((edge->flags_00 & 0x80000000) == 0) {
                    for (index = 0; index < 3; ++index) {
                        colors[base_vertex + index].z = 1.0f;
                        if ((edge->flags_00 & 0x20000000) != 0) {
                            colors[base_vertex + index].y = 1.0f;
                        }
                    }
                }
                if (reverse_found != 0 &&
                    (m_pEdges_04c[reverse_index].flags_00 & 0x80000000) == 0) {
                    for (index = 3; index < 6; ++index) {
                        colors[base_vertex + index].z = 1.0f;
                    }
                }
                if (reverse_found == 0 ||
                    (m_pEdges_04c[reverse_index].flags_00 & 0x20000000) != 0) {
                    for (index = 3; index < 6; ++index) {
                        colors[base_vertex + index].y = 1.0f;
                    }
                }
                ++link_count;
            }
            edge_index = edge->next_0c;
        }

        ++marker_count;
        next = visible_waypoints_058->NextSetBit(0);
    }

    unsigned long* active_polygons = model->getActivePolygonTable(1);
    unsigned long active_count = 0;
    for (index = 0; index < marker_count * 6; ++index) {
        active_polygons[active_count++] = index;
    }
    for (index = 0; index < link_count * 6; ++index) {
        active_polygons[active_count++] = 600 + index;
    }
    model->setActivePolygonCount(active_count);
    if ((model->control_state_390 & 1) == 0) {
        unsigned long state = model->control_state_390;
        model->control_state_390 = state | 9;
        model->reindexPolygons(0);
    }
    if ((model->control_state_390 & 2) == 0) {
        model->control_state_390 |= 10;
    }
    if ((model->control_state_390 & 4) == 0) {
        model->control_state_390 |= 12;
    }
    model->control_state_390 |= 8;
    model->flags_3a0 &= ~2U;
    return m_pPathModelInstance;
}

/* Rebuild the editor's bounded grid search when the cursor enters a new path
   cell. Each reachable vertical span is inserted once into the visited hash;
   the minimum heap expands the nearest pending position first, and the final
   node set is handed to the search-trace renderer. */
// FUNCTION: WIZ8 0x0045C9A0
void W8PathingService::DrawPathPosition0045C9A0(srVector3T<float> position, unsigned char mode)
{
    if (mode == 0 || g_flag_006081e4 == 0) {
        g_path_visualization_cell_00659c6c = 0;
        return;
    }

    int root_x = static_cast<int>((position.x - level_bounds[0]) / grid_scale_01c);
    int root_z = static_cast<int>((position.z - level_bounds[2]) / grid_scale_01c);
    unsigned int root_key = root_z * 0x10000 + root_x;
    if (root_key == g_path_visualization_cell_00659c6c) {
        return;
    }
    g_path_visualization_cell_00659c6c = root_key;

    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells_074);
    if (visited->bucket_count != 0) {
        delete[] visited->bucket_heads;
        delete[] visited->entries;
    }
    visited->bucket_count = 0;
    visited->bucket_heads = 0;
    visited->entries = 0;
    visited->free_head = -1;
    visited->Grow();

    search_node_count_0cc = 0;
    path_heap_06c->heap_00->size_0c = 0;
    unsigned short root_index = AllocateSearchNode00465A00();
    W8PathSearchNode* root = &m_owned_0c8[root_index];
    root->flags_00 = 0;
    root->node_index_02 = root_index;
    root->cell_x_04 = static_cast<unsigned short>(root_x);
    root->cell_z_06 = static_cast<unsigned short>(root_z);
    root->path_height_08 = static_cast<unsigned short>(
        static_cast<int>((position.y - level_bounds[1]) / span_020) + 1);
    root->parent_node_0a = 0;
    root->base_score_0c = 0.0f;
    root->position_20 = position;

    W8PathHeap* heap = path_heap_06c->heap_00;
    W8PathHeapEntry root_entry;
    root_entry.node_00 = root_index;
    root_entry.priority_04 = static_cast<unsigned int>(root->score_1c);
    if (heap->size_0c >= heap->capacity_08) {
        srAssertFail("heapsize < maxheapsize", "..\\Engine Code\\Include\\stHeap.hpp", 0xe1,
                     "stHeap overflow");
    }
    heap->entries_00[heap->size_0c] = root_entry;
    heap->SiftUp00467990(heap->size_0c);
    ++heap->size_0c;
    path_heap_06c->root_node_04 = heap->entries_00[0].node_00;

    unsigned int best_node = root_index;
    while (best_node != 0 && search_node_count_0cc < g_path_reserve_0060827a) {
        W8PathSearchNode* current = &m_owned_0c8[best_node];
        unsigned short current_x = current->cell_x_04;
        unsigned short current_z = current->cell_z_06;
        unsigned short current_height = current->path_height_08;

        for (int direction = 0; direction < 8; ++direction) {
            unsigned int neighbor_x;
            unsigned int neighbor_z;
            if (direction >= 1 && direction <= 3) {
                neighbor_x = current_x + 1;
            } else if (direction > 4) {
                neighbor_x = current_x - 1;
            } else {
                neighbor_x = current_x;
            }
            if (direction < 2 || direction > 6) {
                neighbor_z = current_z + 1;
            } else if (direction > 2 && direction < 6) {
                neighbor_z = current_z - 1;
            } else {
                neighbor_z = current_z;
            }

            unsigned int key = neighbor_z * 0x10000 + neighbor_x;
            unsigned int hash = ((key >> 10 ^ key) >> 10 ^ key);
            int slot = visited->bucket_heads[(visited->bucket_count - 1) & hash];
            while (slot != -1) {
                W8OctreeEntry* entry = &visited->entries[slot];
                if (entry->key == key) {
                    if (entry->value != 0) {
                        slot = -2;
                    }
                    break;
                }
                slot = entry->next_index;
            }
            if (slot == -2) {
                continue;
            }

            W8HashTable<unsigned int, unsigned int>* paths = m_pPathValues_064;
            int path_slot = paths->bucket_heads[(paths->bucket_count - 1) & hash];
            while (path_slot != -1) {
                W8HashEntry<unsigned int, unsigned int>* path_entry = &paths->entries[path_slot];
                if (path_entry->key == key) {
                    unsigned int path_value = path_entry->value;
                    int height_delta = static_cast<int>(path_value & 0xffff) - current_height;
                    if (height_delta > -static_cast<int>(cell_count_024) &&
                        height_delta < static_cast<int>(cell_count_024)) {
                        unsigned short node_index = AllocateSearchNode00465A00();
                        if (visited->free_head == -1) {
                            visited->Grow();
                        }
                        W8OctreeEntry* visited_entries = visited->entries;
                        int inserted = visited->free_head;
                        visited->free_head = visited_entries[inserted].next_index;
                        unsigned int bucket = (visited->bucket_count - 1) & hash;
                        visited_entries[inserted].key = key;
                        visited_entries[inserted].value = node_index;
                        visited_entries[inserted].next_index = visited->bucket_heads[bucket];
                        visited->bucket_heads[bucket] = inserted;

                        W8PathSearchNode* node = &m_owned_0c8[node_index];
                        node->flags_00 = 0;
                        if ((path_value & 0x10000000) != 0) {
                            node->flags_00 = 0x800;
                        }
                        if ((path_value & 0x04000000) != 0) {
                            node->flags_00 |= 0x1000;
                        }
                        node->node_index_02 = node_index;
                        node->cell_x_04 = static_cast<unsigned short>(neighbor_x);
                        node->cell_z_06 = static_cast<unsigned short>(neighbor_z);
                        node->path_height_08 = static_cast<unsigned short>(path_value & 0xffff);
                        node->parent_node_0a = static_cast<unsigned short>(best_node);
                        node->position_20.x =
                            (node->cell_x_04 + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
                        node->position_20.y =
                            (node->path_height_08 - 1) * span_020 + level_bounds[1];
                        node->position_20.z =
                            (node->cell_z_06 + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
                        node->score_1c = (node->position_20 - position).Length();
                        if (node->score_1c < g_path_search_visualization_limit_005ec380) {
                            W8PathHeapEntry pending;
                            pending.node_00 = node->node_index_02;
                            pending.priority_04 = static_cast<unsigned int>(node->score_1c);
                            heap->Insert004675B0(&pending);
                            path_heap_06c->root_node_04 = heap->entries_00[0].node_00;
                        } else {
                            --search_node_count_0cc;
                        }
                    }
                }
                path_slot = path_entry->next_index;
            }
        }

        path_heap_06c->DeleteRoot004577F0(current);
        best_node = path_heap_06c->root_node_04;
        if (best_node > search_node_count_0cc) {
            char message[80];
            sprintf(message, "A:  Invalid node index %d from Queue.", best_node);
            srAssertFail("(ulBestNode <= m_ulSearchNodesUsed)", OCTPATH_CPP, 0x1110, message);
        }
    }
    BuildSearchVisualization0045CFD0();
}

/* Draw the bounded path-search trace in the editor mesh. Search node zero is
   the root, so every later node contributes one raised square at its stored
   position. Its flags select the diagnostic color used for all five vertices
   in that marker. */
// FUNCTION: WIZ8 0x0045CFD0
void W8PathingService::BuildSearchVisualization0045CFD0()
{
    const srVector3T<float> marker_offsets[5] = {
        srVector3T<float>(0.0f, 125.0f, 0.0f), srVector3T<float>(-62.5f, 0.0f, -62.5f),
        srVector3T<float>(-62.5f, 0.0f, 62.5f), srVector3T<float>(62.5f, 0.0f, 62.5f),
        srVector3T<float>(62.5f, 0.0f, -62.5f)};
    stMeshModel* model = static_cast<stMeshModel*>(m_pPathModelInstance->model());
    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    srVector3T<float>* vertices = model->getVertexLoc();
    model->getActivePolygonTable(1);
    srVector3i* polygons = model->getPolyVertex();
    unsigned int node_count = search_node_count_0cc;

    if (node_count > 2000) {
        node_count = 2000;
    }
    for (unsigned int node_index = 1; node_index < node_count; ++node_index) {
        W8PathSearchNode* node = &m_owned_0c8[node_index];
        unsigned int vertex_index = 500 + (node_index - 1) * 5;
        unsigned int polygon_index = 600 + (node_index - 1) * 4;

        polygons[polygon_index].x = vertex_index;
        polygons[polygon_index].y = vertex_index + 1;
        polygons[polygon_index].z = vertex_index + 2;
        polygons[polygon_index + 1].x = vertex_index;
        polygons[polygon_index + 1].y = vertex_index + 2;
        polygons[polygon_index + 1].z = vertex_index + 3;
        polygons[polygon_index + 2].x = vertex_index;
        polygons[polygon_index + 2].y = vertex_index + 3;
        polygons[polygon_index + 2].z = vertex_index + 4;
        polygons[polygon_index + 3].x = vertex_index;
        polygons[polygon_index + 3].y = vertex_index + 4;
        polygons[polygon_index + 3].z = vertex_index + 1;

        srVector3T<float> color;
        if ((node->flags_00 & 0x800) != 0) {
            color = srVector3T<float>(0.0f, 0.0f, 0.0f);
        } else if ((node->flags_00 & 4) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 1.0f);
        } else if ((node->flags_00 & 2) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 1.0f);
        } else if ((node->flags_00 & 0x100) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 0.0f);
        } else if ((node->flags_00 & 0x2000) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 0.0f);
        } else if ((node->flags_00 & 0x8000) != 0) {
            color = srVector3T<float>(0.0f, 1.0f, 1.0f);
        } else if ((node->flags_00 & 0x1000) != 0) {
            color = srVector3T<float>(0.0f, 0.0f, 1.0f);
        } else {
            color = srVector3T<float>(0.0f, 1.0f, 0.0f);
        }

        for (int offset_index = 0; offset_index < 5; ++offset_index) {
            vertices[vertex_index + offset_index].x =
                node->position_20.x + marker_offsets[offset_index].x;
            vertices[vertex_index + offset_index].y =
                node->position_20.y + marker_offsets[offset_index].y;
            vertices[vertex_index + offset_index].z =
                node->position_20.z + marker_offsets[offset_index].z;
            colors[vertex_index + offset_index] = color;
        }
    }

    unsigned long* active_polygons = model->getActivePolygonTable(1);
    unsigned long active_count = node_count > 1 ? (node_count - 1) * 4 : 0;
    for (unsigned long index = 0; index < active_count; ++index) {
        active_polygons[index] = index + 600;
    }
    model->setActivePolygonCount(active_count);
    if ((model->control_state_390 & 1) == 0) {
        unsigned long state = model->control_state_390;
        model->control_state_390 = state | 9;
        model->reindexPolygons(0);
    }
    if ((model->control_state_390 & 2) == 0) {
        model->control_state_390 |= 10;
    }
    if ((model->control_state_390 & 4) == 0) {
        model->control_state_390 |= 12;
    }
    model->control_state_390 |= 8;
    model->flags_3a0 &= ~2U;
}

/* Select the editor color for one waypoint. Disabled surfaces are black; the
   two current selection slots take yellow and either green or red; every other
   surface is blue. */
// FUNCTION: WIZ8 0x0045d490
void W8PathingService::GetWaypointVisualizationColor0045D490(unsigned short waypoint,
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
    if (path_direction_valid_1da != 0) {
        color->x = 0.0f;
        color->y = 1.0f;
        color->z = 0.0f;
        return;
    }
    color->x = 1.0f;
    color->y = 0.0f;
    color->z = 0.0f;
}

/* Create the fixed-capacity editor mesh shared by waypoint and edge drawing.
   The first hundred six-triangle groups describe waypoint markers; the next
   two thousand describe edge segments. Every polygon and vertex receives the
   path editor's shared texture/material state before the concrete model
   instance takes ownership of the mesh. */
// FUNCTION: WIZ8 0x0045D530
stModelInstance* W8PathingService::EnsurePathVisualization0045D530()
{
    const int polygon_count = 0x3138;
    const int vertex_count = 0x30d4;
    stMeshModel* model = new stMeshModel(polygon_count, vertex_count);
    int index;

    if (model == 0) {
        srAssertFail("pstMeshModel", OCTPATH_CPP, 0x11e9,
                     "CreateWayPointMesh::Read -- Could not create pstMeshModel.");
    }
    model->autoRelease();
    model->flags_3a0 &= ~1U;
    model->setShader(g_path_shader_00652dc4, 0);
    model->setName("WayPoint Mesh");
    model->flag_3cc = 0;

    srVector3i* polygons = model->getPolyVertex();
    srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 1);
    srVector2T<float>* texture_coordinates = model->getVertexTexCoords(0, 0, 1);
    srPtr<srMaterialIFace>* materials =
        model->getVertexMaterial(0, static_cast<srMeshModel::e_side>(0), 1);
    unsigned long* shade_indices = model->getVertexShadeIndex(1);

    for (index = 0; index < polygon_count; ++index) {
        textures[index] = g_path_texture_00652dc0;
    }
    for (index = 0; index < vertex_count; ++index) {
        texture_coordinates[index].SetZero();
        materials[index] = g_path_material_00652dbc;
        shade_indices[index] = index;
    }

    int polygon_index = 0;
    int vertex_index = 1;
    do {
        polygons[polygon_index].x = vertex_index - 1;
        polygons[polygon_index].y = vertex_index;
        polygons[polygon_index].z = vertex_index + 1;
        polygons[polygon_index + 1].x = vertex_index - 1;
        polygons[polygon_index + 1].y = vertex_index + 1;
        polygons[polygon_index + 1].z = vertex_index + 2;
        polygons[polygon_index + 2].x = vertex_index - 1;
        polygons[polygon_index + 2].y = vertex_index + 2;
        polygons[polygon_index + 2].z = vertex_index + 3;
        polygons[polygon_index + 3].x = vertex_index - 1;
        polygons[polygon_index + 3].y = vertex_index + 3;
        polygons[polygon_index + 3].z = vertex_index;
        polygons[polygon_index + 4].x = vertex_index;
        polygons[polygon_index + 4].y = vertex_index + 3;
        polygons[polygon_index + 4].z = vertex_index + 2;
        polygons[polygon_index + 5].x = vertex_index;
        polygons[polygon_index + 5].y = vertex_index + 2;
        polygons[polygon_index + 5].z = vertex_index + 1;
        vertex_index += 5;
        polygon_index += 6;
    } while (vertex_index < 0x1f5);

    vertex_index = 0x1f8;
    do {
        polygons[polygon_index].x = vertex_index - 4;
        polygons[polygon_index].y = vertex_index - 3;
        polygons[polygon_index].z = vertex_index - 1;
        polygons[polygon_index + 1].x = vertex_index - 3;
        polygons[polygon_index + 1].y = vertex_index - 2;
        polygons[polygon_index + 1].z = vertex_index;
        polygons[polygon_index + 2].x = vertex_index - 2;
        polygons[polygon_index + 2].y = vertex_index - 4;
        polygons[polygon_index + 2].z = vertex_index + 1;
        polygons[polygon_index + 3].x = vertex_index;
        polygons[polygon_index + 3].y = vertex_index - 1;
        polygons[polygon_index + 3].z = vertex_index - 3;
        polygons[polygon_index + 4].x = vertex_index + 1;
        polygons[polygon_index + 4].y = vertex_index;
        polygons[polygon_index + 4].z = vertex_index - 2;
        polygons[polygon_index + 5].x = vertex_index - 1;
        polygons[polygon_index + 5].y = vertex_index + 1;
        polygons[polygon_index + 5].z = vertex_index - 4;
        vertex_index += 6;
        polygon_index += 6;
    } while (vertex_index < 0x30d8);

    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    for (index = 0; index < 5; ++index) {
        colors[index].x = 1.0f;
        colors[index].y = 0.0f;
        colors[index].z = 0.0f;
    }
    for (index = 10; index < 0x1f9; ++index) {
        colors[index].Set(0.0, 0.0, 1.0);
    }

    m_pPathModelInstance = CreateModelInstance0046F5C0(model);
    if (m_pPathModelInstance == 0) {
        srAssertFail("m_pPathModelInstance", OCTPATH_CPP, 0x1226,
                     "CreateWayPointMesh -- Could not create pstModelInstance.");
    }
    m_pPathModelInstance->setName("WayPoint Mesh");
    m_pPathModelInstance->setExclusionMask(3);
    m_pPathModelInstance->setFlag(srNode::FLAG_DISABLE);
    return m_pPathModelInstance;
}

/* Collect nearby waypoint surfaces and mark the subset directly visible from
   the editor position. The near query also admits every outgoing neighbor;
   the wider query contributes only its own surfaces. Candidates are deduped,
   sorted by integer distance, and span-tested nearest first. */
// FUNCTION: WIZ8 0x0045D880
short W8PathingService::CollectPathVisualization0045D880(const srVector3T<float>* position)
{
    unsigned short waypoints[500];
    unsigned long distances[500];
    unsigned int waypoint_count = 0;
    unsigned long* query_results = 0;
    int query_count;
    int index;

    if (value_1d4 == 0) {
        return 0;
    }

    visible_waypoints_058->ClearAll();
    collected_waypoints_060->ClearAll();
    if (value_1d6 != 0 && path_direction_valid_1da == 0) {
        visible_waypoints_058->Set(value_1d6);
        collected_waypoints_060->Set(value_1d6);
    }

    srVector3T<float> lower;
    srVector3T<float> upper;
    lower.x = position->x - g_float_005ec35c;
    lower.y = position->y - g_float_005ec2f8;
    lower.z = position->z - g_float_005ec35c;
    upper.x = position->x + g_float_005ec35c;
    upper.y = position->y + g_float_005ec2f8;
    upper.z = position->z + g_float_005ec35c;
    query_count =
        g_octree_6598a4->QueryObjects(&query_results, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = static_cast<unsigned short>(query_results[index]);
        unsigned short edge_index;

        if (!collected_waypoints_060->Set(waypoint)) {
            waypoints[waypoint_count++] = waypoint;
        }
        edge_index = m_pSurfaces_048[waypoint].first_edge_24;
        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short neighbor = edge->destination_06;

            if (!collected_waypoints_060->Set(neighbor)) {
                waypoints[waypoint_count++] = neighbor;
            }
            edge_index = edge->next_0c;
        }
    }

    query_results = 0;
    lower.x = position->x - g_float_005ec384;
    lower.y = position->y - g_float_005ec35c;
    lower.z = position->z - g_float_005ec384;
    upper.x = position->x + g_float_005ec384;
    upper.y = position->y + g_float_005ec35c;
    upper.z = position->z + g_float_005ec384;
    query_count =
        g_octree_6598a4->QueryObjects(&query_results, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = static_cast<unsigned short>(query_results[index]);
        if (!collected_waypoints_060->Set(waypoint)) {
            waypoints[waypoint_count++] = waypoint;
        }
    }

    if (waypoint_count > 1) {
        unsigned int sort_index;

        for (sort_index = 0; sort_index < waypoint_count; ++sort_index) {
            const srVector3T<float>* candidate =
                &m_pSurfaces_048[waypoints[sort_index]].position_04;
            distances[sort_index] =
                static_cast<unsigned int>(static_cast<int>((*position - *candidate).Length()));
        }

        if (waypoint_count <= 10) {
            for (sort_index = 1; sort_index < waypoint_count; ++sort_index) {
                unsigned short waypoint = waypoints[sort_index];
                unsigned int distance = distances[sort_index];
                unsigned int insertion = sort_index;

                while (insertion != 0 && distance < distances[insertion - 1]) {
                    distances[insertion] = distances[insertion - 1];
                    waypoints[insertion] = waypoints[insertion - 1];
                    --insertion;
                }
                distances[insertion] = distance;
                waypoints[insertion] = waypoint;
            }
        } else {
            QuickSortByKey(waypoints, distances, 0, static_cast<int>(waypoint_count) - 1);
        }
    }

    for (unsigned int visible_index = 0; visible_index < waypoint_count; ++visible_index) {
        unsigned short waypoint = waypoints[visible_index];
        if (TestWaypointSpan0045A1B0(position, &m_pSurfaces_048[waypoint].position_04, 0, 0) != 0) {
            visible_waypoints_058->Set(waypoint);
        }
    }
    return value_1d4;
}

/* Append one waypoint surface and keep every capacity-coupled side table sized
   to the same hundred-record block.

   Surface zero is reserved on the first allocation. Growth replaces the three
   BitArrays rather than preserving their bits, and recreates the shared
   unsigned-short scratch run. The new surface receives its index and position,
   is classified for the path-surface flag, and is registered in the octree's
   spatial object index as kind nine. */
// FUNCTION: WIZ8 0x0045ddb0
void W8PathingService::AddWaypoint0045DDB0(const srVector3T<float>* position)
{
    if (m_ulNumWayPoints % 100 == 0) {
        unsigned int capacity = (m_ulNumWayPoints / 100 + 1) * 100;
        W8PathSurface* new_surfaces =
            static_cast<W8PathSurface*>(malloc(capacity * sizeof(W8PathSurface)));

        if (new_surfaces == 0) {
            srAssertFail("pNewWayPoints", OCTPATH_CPP, 0x12ec, 0);
        }
        memset(new_surfaces, 0, capacity * sizeof(W8PathSurface));
        if (m_ulNumWayPoints == 0) {
            m_ulNumWayPoints = 1;
        } else {
            memcpy(new_surfaces, m_pSurfaces_048, m_ulNumWayPoints * sizeof(W8PathSurface));
            free(m_pSurfaces_048);
        }
        m_pSurfaces_048 = new_surfaces;

        if (visible_waypoints_058 != 0) {
            delete visible_waypoints_058;
        }
        visible_waypoints_058 = new BitArray(capacity);
        if (rendered_waypoints_05c != 0) {
            delete rendered_waypoints_05c;
        }
        rendered_waypoints_05c = new BitArray(capacity);
        if (collected_waypoints_060 != 0) {
            delete collected_waypoints_060;
        }
        collected_waypoints_060 = new BitArray(capacity);

        free(g_path_scratch_00659c64);
        g_path_scratch_00659c64 = malloc(capacity * sizeof(unsigned short));
    }

    W8PathSurface* surface = &m_pSurfaces_048[m_ulNumWayPoints];
    int point[3];

    surface->flags_00 = 0x2000;
    surface->index_02 = static_cast<unsigned short>(m_ulNumWayPoints);
    surface->position_04 = *position;
    if ((ClassifyWaypoint00459C00(&surface->position_04) & 0x04000000) != 0) {
        surface->flags_00 |= 0x40;
    }
    g_octree_6598a4->WorldPositionToCell(position, point);
    g_octree_6598a4->object_registry->MoveObjectToCell(
        W8_OCTREE_KIND_WAYPOINT, static_cast<unsigned short>(m_ulNumWayPoints) + 1, point);
    ++m_ulNumWayPoints;
}

/* Unlink and clear one edge record.

   The owning surface or predecessor edge is redirected to the removed edge's
   successor. The retail scans stop after the first owner is found, then clear
   the packed record and increment the service's free-record count. */
// FUNCTION: WIZ8 0x0045e360
void W8PathingService::RemoveWaypointLink0045E360(unsigned short edge_index)
{
    if (m_ulNumWayPoints > 2) {
        unsigned char found = 0;
        unsigned int index;
        for (index = 1; index < m_ulNumWayPoints && found == 0; ++index) {
            if (m_pSurfaces_048[index].first_edge_24 == edge_index) {
                m_pSurfaces_048[index].first_edge_24 = m_pEdges_04c[edge_index].next_0c;
                found = 1;
            }
        }

        for (index = 1; index < m_ulNumWayPtLinks && found == 0; ++index) {
            if (m_pEdges_04c[index].next_0c == edge_index) {
                m_pEdges_04c[index].next_0c = m_pEdges_04c[edge_index].next_0c;
                found = 1;
            }
        }

        memset(&m_pEdges_04c[edge_index], 0, sizeof(W8PathEdge));
        ++m_positional_018;
        MarkRendererReady();
        flag_1cc = 1;
    }
}

/* Choose the waypoint that best continues from the surface nearest source in
   the requested direction. Existing graph edges take precedence. When none
   are sufficiently aligned, probe twenty-five fixed steps forward and retain
   the best distinct surface found there. The second selection is marked valid
   only when it is an existing edge or the direct span test accepts it. */
// FUNCTION: WIZ8 0x0045e840
unsigned char W8PathingService::PreparePathVisualization0045E840(const srVector3T<float>* source,
                                                                 const srVector3T<float>* direction)
{
    float best_alignment = -1.0f;
    unsigned short best_waypoint = 0;
    unsigned short source_waypoint;
    W8PathSurface* source_surface;
    srVector3T<float> offset;

    value_1d6 = 0;
    path_direction_valid_1da = 0;
    source_waypoint = FindWaypoint0045B120(source, 0);
    source_surface = &m_pSurfaces_048[source_waypoint];

    offset = source_surface->position_04 - *source;
    if (offset.Length() <= g_double_005ec030) {
        unsigned short edge_index = source_surface->first_edge_24;

        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short neighbor_index = edge->destination_06;
            W8PathSurface* neighbor = &m_pSurfaces_048[neighbor_index];
            srVector3T<float> neighbor_direction;
            float alignment;

            neighbor_direction = neighbor->position_04 - source_surface->position_04;
            neighbor_direction.Normalize();
            alignment = DotProduct(neighbor_direction, *direction);
            if (alignment > best_alignment) {
                best_alignment = alignment;
                best_waypoint = neighbor_index;
            }
            edge_index = edge->next_0c;
        }

        if (best_alignment > g_float_005ec38c) {
            value_1d6 = best_waypoint;
            path_direction_valid_1da = 1;
            value_1d4 = source_waypoint;
            return 1;
        }

        best_alignment = -1.0f;
        best_waypoint = 0;
        float distance = 0.0f;
        int probe_count = 25;
        do {
            srVector3T<float> probe;
            unsigned short probe_waypoint;

            distance += g_float_005ebc64;
            probe = source_surface->position_04 + *direction * distance;
            probe_waypoint = FindWaypoint0045B120(&probe, 0);
            if (probe_waypoint != 0 && probe_waypoint != source_waypoint) {
                W8PathSurface* candidate = &m_pSurfaces_048[probe_waypoint];
                srVector3T<float> candidate_direction;
                float alignment;

                candidate_direction = candidate->position_04 - source_surface->position_04;
                candidate_direction.Normalize();
                alignment = DotProduct(candidate_direction, *direction);
                if (alignment > best_alignment) {
                    best_alignment = alignment;
                    best_waypoint = probe_waypoint;
                }
            }
            --probe_count;
        } while (probe_count != 0);

        if (best_alignment > g_float_005ec38c) {
            value_1d6 = best_waypoint;
            if (TestWaypointSpan0045A1B0(&source_surface->position_04,
                                         &m_pSurfaces_048[best_waypoint].position_04, 0, 0) != 0) {
                path_direction_valid_1da = 1;
            }
            value_1d4 = source_waypoint;
            return 1;
        }
    }

    value_1d4 = source_waypoint;
    return 0;
}

/* Add one directed edge to the waypoint graph, or update the matching edge
   when the source already owns it.

   Edge zero is the list sentinel and storage grows in hundred-record blocks.
   The cached length is computed before growth, the new record is appended to
   the source surface's chain, and the geometry-derived flag follows the same
   endpoint/span tests as an updated edge. */
// FUNCTION: WIZ8 0x0045ec30
void W8PathingService::AddWaypointLink0045EC30(unsigned short source, unsigned short destination,
                                               unsigned int flags)
{
    W8PathSurface* source_surface;
    W8PathSurface* destination_surface;
    float distance;
    W8PathEdge* edge;

    if (source == 0 || destination == 0 || source == destination) {
        WriteGameLog(0xf, L"Cannot Link: Tried to link WayPt %d to WayPt %d. ", source,
                     destination);
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
        WriteGameLog(0xf, L"Cannot Link: WayPt %d is at (0, 0, 0). ", source);
        return;
    }

    if (UpdateWaypointLink0045F200(source, destination, flags) != 0) {
        return;
    }

    distance = (source_surface->position_04 - destination_surface->position_04).Length();

    if (m_ulNumWayPtLinks % 100 == 0) {
        unsigned int capacity = m_ulNumWayPtLinks / 100 + 1;
        W8PathEdge* new_edges =
            static_cast<W8PathEdge*>(malloc(capacity * 100 * sizeof(W8PathEdge)));

        if (new_edges == 0) {
            srAssertFail("pNewWayPtLinks", OCTPATH_CPP, 0x1526, 0);
        }
        memset(new_edges, 0, capacity * 100 * sizeof(W8PathEdge));
        if (m_ulNumWayPtLinks == 0) {
            m_ulNumWayPtLinks = 1;
        } else {
            memcpy(new_edges, m_pEdges_04c, m_ulNumWayPtLinks * sizeof(W8PathEdge));
            free(m_pEdges_04c);
        }
        m_pEdges_04c = new_edges;
    }

    edge = &m_pEdges_04c[m_ulNumWayPtLinks];
    edge->flags_00 = flags;
    edge->destination_06 = destination;
    edge->source_04 = source;
    edge->distance_08 = distance;
    edge->next_0c = 0;

    if (source_surface->first_edge_24 == 0) {
        source_surface->first_edge_24 = static_cast<unsigned short>(m_ulNumWayPtLinks);
    } else {
        unsigned short previous = source_surface->first_edge_24;

        while (m_pEdges_04c[previous].next_0c != 0) {
            previous = m_pEdges_04c[previous].next_0c;
        }
        m_pEdges_04c[previous].next_0c = static_cast<unsigned short>(m_ulNumWayPtLinks);
    }

    if ((source_surface->flags_00 & 0x40) != 0 || (destination_surface->flags_00 & 0x40) != 0 ||
        (TestWaypointSpan0045A1B0(&source_surface->position_04, &destination_surface->position_04,
                                  0, 0),
         flag_23c != 0)) {
        edge->flags_00 |= 0x20000000;
    }
    ++m_ulNumWayPtLinks;
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
unsigned char W8PathingService::HasDirectionalWaypointLink0045EF90(unsigned short source,
                                                                   unsigned short destination)
{
    W8PathSurface* source_surface = &m_pSurfaces_048[source];
    const W8PathSurface* destination_surface = &m_pSurfaces_048[destination];
    srVector3T<float> destination_direction;
    float destination_distance;
    unsigned short edge_index;

    destination_direction = destination_surface->position_04 - source_surface->position_04;
    destination_distance = destination_direction.Length();
    srVector2T<float> destination_horizontal(destination_direction.x, destination_direction.z);
    destination_horizontal.Normalize();
    destination_direction.x = destination_horizontal.x;
    destination_direction.y = 0.0f;
    destination_direction.z = destination_horizontal.y;

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

        neighbor_direction = neighbor->position_04 - source_surface->position_04;
        neighbor_distance = neighbor_direction.Length();
        if (neighbor_distance < destination_distance) {
            unsigned short second_edge_index;
            srVector2T<float> neighbor_horizontal(neighbor_direction.x, neighbor_direction.z);

            neighbor_horizontal.Normalize();
            neighbor_direction.x = neighbor_horizontal.x;
            neighbor_direction.y = 0.0f;
            neighbor_direction.z = neighbor_horizontal.y;

            second_edge_index = neighbor->first_edge_24;
            while (second_edge_index != 0) {
                if (m_pEdges_04c[second_edge_index].destination_06 == destination) {
                    break;
                }
                second_edge_index = m_pEdges_04c[second_edge_index].next_0c;
            }
            if (second_edge_index != 0 &&
                DotProduct(neighbor_direction, destination_direction) > g_float_005ec390) {
                return 1;
            }
            if (DotProduct(neighbor_direction, destination_direction) > g_float_005ec38c) {
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
unsigned char W8PathingService::UpdateWaypointLink0045F200(unsigned short source,
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
                (TestWaypointSpan0045A1B0(&m_pSurfaces_048[source].position_04,
                                          &m_pSurfaces_048[destination].position_04, 0, 0),
                 flag_23c != 0)) {
                m_pEdges_04c[m_ulNumWayPtLinks].flags_00 |= 0x20000000;
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
void W8PathingService::EditTeleportalLink(const srVector3T<float>* destination,
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
        (m_pSurfaces_048[destination_index].position_04 - *destination).Length() >
            g_double_005ec150) {
        destination_index = 0;
    }

    source_index = FindWaypoint0045B120(source, 0);
    if ((m_pSurfaces_048[source_index].flags_00 & 2) == 0 ||
        (m_pSurfaces_048[source_index].position_04 - *source).Length() > g_double_005ec150) {
        source_index = 0;
    }

    if (destination_index == 0) {
        destination_index = static_cast<unsigned short>(m_ulNumWayPoints);
        AddWaypoint0045DDB0(destination);
        m_pSurfaces_048[destination_index].flags_00 = 2;
        SetWaypointLinkFlags0045E030(destination_index, 6);
        both_existing = 0;
    }
    if (source_index == 0) {
        source_index = static_cast<unsigned short>(m_ulNumWayPoints);
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
    } else {
        sprintf(title, "EDIT FLAGS FOR NEW LINK BETWEEN TELEPORTAL WAYPOINTS: ");
    }
    EditWaypointLinkFlags0045F530(title, link_flags, 5);

    if (edge_index == 0) {
        edge_index = static_cast<unsigned short>(m_ulNumWayPtLinks);
        AddWaypointLink0045EC30(destination_index, source_index, link_flags[0]);
    } else {
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
unsigned int W8PathingService::FindPathHandle(const char* path_name, W8PathGridBounds* path_bounds,
                                              W8PathVerticalRange* path_range)
{
    GDPropCondPaths* path;
    unsigned int index;
    unsigned int lookup_index;
    unsigned short value;
    float height;

    if (path_name == 0 || *path_name == 0 || m_pCondPaths == 0 || m_ulNumCondPaths == 0) {
        return 0;
    }
    index = 0;
    path = m_pCondPaths;
    do {
        if (strcmp(path_name, path->name) == 0) {
            path_bounds->min_z = 0xffff;
            path_bounds->min_x = 0xffff;
            path_bounds->max_z = 0;
            path_bounds->max_x = 0;
            path_range->minimum = 1e+08f;
            path_range->maximum = -1e+08f;
            lookup_index = path->lookup_index;
            while (m_pulCondLookup[lookup_index] != 0) {
                unsigned int key_index = m_pulCondLookup[lookup_index];
                while (m_pulCondNodeKeys[key_index] != 0) {
                    value = static_cast<unsigned short>(m_pulCondNodeKeys[key_index]);
                    if (value < path_bounds->min_x) {
                        path_bounds->min_x = value;
                    }
                    if (path_bounds->max_x < value) {
                        path_bounds->max_x = value;
                    }
                    value = static_cast<unsigned short>(m_pulCondNodeKeys[key_index] >> 0x10);
                    if (value < path_bounds->min_z) {
                        path_bounds->min_z = value;
                    }
                    if (path_bounds->max_z < value) {
                        path_bounds->max_z = value;
                    }
                    height = (m_pulCondNodeValues[key_index] & 0xffff) * span_020 + level_bounds[1];
                    if (height < path_range->minimum) {
                        path_range->minimum = height;
                    }
                    if (path_range->maximum < height) {
                        path_range->maximum = height;
                    }
                    ++key_index;
                }
                ++lookup_index;
            }
            return path->lookup_index;
        }
        ++index;
        ++path;
    } while (index < static_cast<unsigned int>(m_ulNumCondPaths));
    return 0;
}

struct W8PathParameter {
    const char* name;
    float* value;
};

static W8PathParameter g_path_parameters[] = {
    {"ACCELERATION_FACTOR", &g_path_acceleration_factor_0060f9e8},
    {"ANGULAR_ACCEL_FACTOR", &g_path_angular_acceleration_factor_0060f9ec},
    {"ANGULAR_DECEL_FACTOR", &g_path_angular_deceleration_factor_0060f9f0},
    {"PREDICTION_TIME", &g_path_prediction_time_0060f9f4},
    {"APPROACH_SLOW_TIME", &g_path_approach_slow_time_0060f9f8},
    {"APPROACH_RUN_TIME", &g_path_approach_run_time_0060f9fc},
    {"APPROACH_RUN_RATE", &g_path_approach_run_rate_0060fa00},
    {"PATH_PREDICTION_TIME", &g_path_lookahead_time_0060fa04},
    {"GROUP_REPULSION_FACTOR", &g_path_group_repulsion_factor_0060fa08},
    {"PARTY_BOUNDARY_RADIUS", &g_path_party_boundary_radius_0060fa0c},
    {"OBSTACLE_STEER_FACTOR", &g_path_obstacle_steering_factor_0060fa10},
    {"OBSTACLE_BRAKE_FACTOR", &g_path_obstacle_braking_factor_0060fa14},
    {0, 0}};

// FUNCTION: WIZ8 0x004cae40
W8PathParameters::W8PathParameters()
{
    LoadPathParameters004CCCB0();
}

// FUNCTION: WIZ8 0x004cae50
void W8PathParameters::InitializeSteeringContext004CAE50(W8NavigatorMovementState* movement)
{
    W8Navigator* linked;

    movement_00 = movement;
    monster_54 = GetMonsterByLocationID(movement->location_id_004);
    radius_44 = monster_54->radius_084;
    velocity_length_10 = movement->velocity_034.Length();
    linked = monster_54->linked_navigator_05c;
    if (linked == 0) {
        speed_limit_08 = movement->movement_scale_060 * g_world_scale_005ebc40;
    } else {
        speed_limit_08 = linked->movement_0c0.movement_scale_060 * g_world_scale_005ebc40;
    }
    acceleration_0c = g_path_acceleration_factor_0060f9e8 * speed_limit_08;
    if (movement->velocity_034.x == g_float_005ebb34 &&
        movement->velocity_034.y == g_float_005ebb34 &&
        movement->velocity_034.z == g_float_005ebb34) {
        direction_20.Set(0.0, 0.0, 1.0);
        direction_20.RotateAboutY(sin(movement->yaw), cos(movement->yaw));
    } else {
        direction_20 = movement->velocity_034;
        direction_20.Normalize();
    }
    perpendicular_2c.Set(direction_20.z, 0.0, -direction_20.x);
    force_38.SetZero();
    blocked_49 = 0;
    nearby_queried_48 = 0;
}

// FUNCTION: WIZ8 0x004cafc0
unsigned char W8PathParameters::QueryNearbyNavigators004CAFC0()
{
    srVector3T<float> lower;
    srVector3T<float> upper;
    float extent;

    if (nearby_queried_48 != 0) {
        return nearby_count_4c != 0;
    }
    extent = radius_44 * g_float_005ebc28;
    nearby_queried_48 = 1;
    lower.Set(movement_00->position_040.x - extent, movement_00->position_040.y - extent,
              movement_00->position_040.z - extent);
    upper.Set(extent + movement_00->position_040.x, extent + movement_00->position_040.y,
              extent + movement_00->position_040.z);
    nearby_locations_50 = 0;
    nearby_count_4c = g_octree_6598a4->QueryLocationsInBox(&nearby_locations_50, &lower, &upper,
                                                           movement_00->location_id_004);
    return nearby_count_4c != 0;
}

// FUNCTION: WIZ8 0x004cb090
void W8PathParameters::IntegrateSteering004CB090()
{
    float step;
    srVector3T<float> velocity;
    srVector3T<float> position;
    srVector3T<float> delta;
    srVector3T<float> slide;
    float length_squared;
    float scale;
    unsigned char snapped;
    char direction;

    step = g_rate_006068EC * g_game_time_accumulator_6598bc->GetValue28();
    if (blocked_49 == 0) {
        force_38.y = 0.0f;
        if (speed_limit_08 <= g_float_005ebb34) {
            velocity.SetZero();
            velocity_length_10 = 0.0f;
            movement_00->target_yaw =
                NormalizeAngle(static_cast<float>(atan2(force_38.x, force_38.z)));
        } else {
            if (acceleration_0c < force_38.Length() &&
                (length_squared = force_38.LengthSquared(), length_squared != g_zero_005ebb40)) {
                scale = acceleration_0c / sqrt(length_squared);
                force_38 *= scale;
            }
            velocity = movement_00->velocity_034 + force_38 * step;
            velocity_length_10 = velocity.Length();
            if (speed_limit_08 < velocity_length_10) {
                length_squared = velocity.LengthSquared();
                if (length_squared != g_zero_005ebb40) {
                    scale = speed_limit_08 / sqrt(length_squared);
                    velocity *= scale;
                }
                velocity_length_10 = speed_limit_08;
            }
            movement_00->target_yaw =
                NormalizeAngle(static_cast<float>(atan2(velocity.x, velocity.z)));
        }
        if (movement_00->target_yaw != movement_00->yaw) {
            UpdateYawSteering004CB520(step, 1);
            movement_00->yaw = movement_00->target_yaw;
            velocity.Set(0.0, 0.0, velocity_length_10);
            velocity.RotateAboutY(sin(movement_00->target_yaw), cos(movement_00->target_yaw));
        }
        position = movement_00->position_040 + velocity * step;
        delta = position;
        snapped = g_octree_6598a4->pathing_180->SnapWaypointPosition00462E60(&position, '\0');
        if (snapped == '\0') {
            delta = position - movement_00->position_040;
            direction = g_octree_6598a4->pathing_180->GetNeighborSlideDirection00466600(
                &movement_00->position_040, &delta, &slide);
            if (direction == '\0') {
                blocked_49 = 1;
            } else {
                scale = DotProduct(slide, delta);
                slide += delta * scale;
                position = movement_00->position_040 + slide;
                snapped =
                    g_octree_6598a4->pathing_180->SnapWaypointPosition00462E60(&position, '\0');
                if (snapped == '\0') {
                    movement_00->target_yaw =
                        NormalizeAngle(static_cast<float>(atan2(delta.x, delta.z)));
                    position = movement_00->position_040;
                    UpdateYawSteering004CB520(step, 0);
                    movement_00->yaw = movement_00->target_yaw;
                }
            }
        }
        if (blocked_49 == 0) {
            movement_00->velocity_034 = velocity;
            movement_00->position_040 = position;
            return;
        }
    }
    velocity.SetZero();
    position = movement_00->position_040;
    g_octree_6598a4->pathing_180->FindPathCell00459D60(&position, 0, '\x01');
    movement_00->velocity_034 = velocity;
    movement_00->position_040 = position;
}

// FUNCTION: WIZ8 0x004cb520
void W8PathParameters::UpdateYawSteering004CB520(float time_step, char use_turn_rate)
{
    float remaining;
    float rate;
    float direction;
    float angular_velocity;

    remaining = NormalizeAngle(movement_00->yaw - movement_00->target_yaw);
    rate = NormalizeAngle(movement_00->target_yaw - movement_00->yaw);
    direction = g_negative_one_005ebc38;
    if (rate < remaining) {
        remaining = rate;
        direction = g_float_005ebb38;
    }
    if (use_turn_rate == '\0') {
        rate = movement_00->turn_rate_068;
        angular_velocity = 0.0f;
    } else {
        angular_velocity = g_path_angular_acceleration_factor_0060f9ec *
                               movement_00->turn_rate_068 * direction * time_step +
                           movement_00->unknown_01c;
        rate = static_cast<float>(fabs(angular_velocity));
    }
    if (remaining <= rate * time_step) {
        movement_00->unknown_01c = g_path_angular_deceleration_factor_0060f9f0 * angular_velocity;
        return;
    }
    movement_00->target_yaw = NormalizeAngle(rate * time_step * direction + movement_00->yaw);
    movement_00->unknown_01c = angular_velocity;
}

// FUNCTION: WIZ8 0x004cb620
unsigned char W8PathParameters::PredictNavigatorCollision004CB620()
{
    float lookahead;
    float nearest;
    float lateral;
    bool found;
    unsigned int index;
    W8Monster* monster;
    W8Navigator* navigator;
    srVector3T<float> position;
    srVector3T<float> other_velocity;
    srVector3T<float> relative;
    srVector3T<float> delta;
    float approach;
    float combined;
    float steer;
    float brake;
    float scale;

    found = false;
    if (monster_54->state_088 == '\0') {
        return '\0';
    }
    if (velocity_length_10 == g_zero_005ebb40) {
        return '\0';
    }
    lookahead = g_path_prediction_time_0060f9f4 * velocity_length_10 + radius_44;
    nearest = lookahead + g_camera_snap_epsilon_005ebc2c;
    if (QueryNearbyNavigators004CAFC0() != '\0') {
        for (index = 0; index < nearby_count_4c; ++index) {
            monster = GetMonsterByLocationID(nearby_locations_50[index]);
            if (monster == 0) {
                navigator = 0;
            } else {
                navigator = monster;
            }
            if (navigator->IsLinkedToNavigator00452E10(monster_54) != '\0') {
                continue;
            }
            navigator = monster;
            position = navigator->GetPosition();
            delta = position - movement_00->position_040;
            if (g_float_005ebb34 < DotProduct(delta, direction_20)) {
                navigator->GetVelocity(&other_velocity);
                relative = movement_00->velocity_034 - other_velocity;
                relative.Normalize();
                approach = DotProduct(delta, relative);
                approach = approach - (monster->radius_084 * approach) / delta.Length();
                if (g_float_005ebb34 < approach && approach < nearest) {
                    lateral = relative.z * delta.x + -relative.x * delta.z;
                    combined = monster->radius_084 + radius_44;
                    if (static_cast<float>(fabs(lateral)) < combined) {
                        scale = lateral / combined;
                        found = true;
                        nearest = approach;
                    }
                }
            }
        }
    }
    if (movement_00->unknown_076[0] != '\0') {
        position = g_startup_world_659c0c->GetPosition();
        delta = position - movement_00->position_040;
        combined = g_path_party_boundary_radius_0060fa0c * g_world_scale_005ebc40;
        approach = radius_44 * g_float_005ebc28 + combined;
        if (approach * approach > delta.LengthSquared()) {
            approach = DotProduct(delta, direction_20);
            if (approach > g_float_005ebb34) {
                relative = movement_00->velocity_034 - g_level_data_00652dac->camera_forward_4c;
                relative.Normalize();
                approach = DotProduct(delta, relative);
                approach = approach - (combined * approach) / delta.Length();
                if (approach > g_float_005ebb34 && approach < nearest) {
                    lateral = delta.x * relative.z + delta.z * -relative.x;
                    combined = combined + radius_44;
                    if (static_cast<float>(fabs(lateral)) < combined) {
                        scale = lateral / combined;
                        found = true;
                        nearest = approach;
                    }
                }
            }
        }
    }
    if (found == '\0') {
        return '\0';
    }
    if (scale >= g_float_005ebb34) {
        steer = scale - g_float_005ebb38;
    } else {
        steer = scale + g_float_005ebb38;
    }
    steer = (g_float_005ebb38 - nearest / lookahead) * acceleration_0c * steer;
    brake = g_path_obstacle_braking_factor_0060fa14 * steer;
    steer = steer * g_path_obstacle_steering_factor_0060fa10;
    force_38 += perpendicular_2c * steer;
    force_38 += direction_20 * -brake;
    return found;
}

// FUNCTION: WIZ8 0x004cbb70
unsigned char W8PathParameters::HandleObstacleAhead004CBB70()
{
    srVector3T<float> escape;
    srVector3T<float> waypoint;
    srVector3T<float> ahead;
    float reach;
    float distance;
    float clearance;
    float steer;
    float brake;
    float side;
    float scale;

    if (velocity_length_10 == g_float_005ebb34) {
        if (g_octree_6598a4->pathing_180->GetNeighborSlideDirection00466600(
                &movement_00->position_040, &direction_20, &escape) != '\0') {
            movement_00->attachment_0ac->GetNextPosition00456660(&waypoint);
            waypoint -= movement_00->position_040;
            waypoint.y = 0.0f;
            if ((escape.z * waypoint.x + waypoint.z * -escape.x) *
                    (escape.z * direction_20.x + -escape.x * direction_20.z) <
                g_zero_005ebb40) {
                speed_limit_08 = 0.0f;
                scale = waypoint.x * waypoint.x + waypoint.z * waypoint.z;
                if (scale != g_zero_005ebb40) {
                    scale = acceleration_0c / sqrt(scale);
                    waypoint.x = waypoint.x * scale;
                    waypoint.z = waypoint.z * scale;
                }
                force_38 += waypoint;
                return 1;
            }
        }
    } else {
        reach = g_path_prediction_time_0060f9f4 * velocity_length_10 + radius_44;
        ahead = movement_00->position_040 + direction_20 * reach;
        if (g_octree_6598a4->pathing_180->TestWaypointSpan0045A1B0(
                &movement_00->position_040, &ahead, '\x01', '\x01') == '\0') {
            ahead = ahead - movement_00->position_040;
            distance = ahead.Length();
            if (distance <= reach) {
                movement_00->attachment_0ac->GetNextPosition00456660(&waypoint);
                waypoint -= movement_00->position_040;
                waypoint.y = 0.0f;
                waypoint.SetLength(1.0);
                if (g_zero_005ebb40 <= DotProduct(waypoint, direction_20)) {
                    steer = (g_float_005ebb38 - distance / reach) * acceleration_0c;
                    brake = g_path_obstacle_braking_factor_0060fa14 * steer;
                    steer = g_path_obstacle_steering_factor_0060fa10 * steer;
                    if (g_octree_6598a4->pathing_180->GetObstacleDirection00466990(
                            &direction_20, &escape) != '\0') {
                        side = DotProduct(escape, perpendicular_2c);
                        if (g_double_005ed2e0 <= fabs(side)) {
                            if (side < g_float_005ebb34) {
                                steer = -steer;
                            }
                        } else {
                            escape.Normalize();
                            if (DotProduct(escape, perpendicular_2c) < g_float_005ebb34) {
                                steer = -steer;
                            }
                        }
                        force_38 += perpendicular_2c * steer;
                        force_38 += direction_20 * -brake;
                        return 1;
                    }
                    blocked_49 = 1;
                    return 1;
                }
                steer = acceleration_0c;
                speed_limit_08 = 0.0f;
                force_38 += waypoint * steer;
                return 1;
            }
        } else if (g_world_scale_005ebc40 < radius_44) {
            clearance = g_octree_6598a4->pathing_180->CompareDirectionalClearance0045AAC0(
                &movement_00->position_040, &direction_20, radius_44);
            if (g_float_005ebb38 < static_cast<float>(fabs(clearance))) {
                force_38 += perpendicular_2c * clearance;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004cc1a0
void W8PathParameters::AccumulateSeekForce004CC1A0()
{
    srVector3T<float> desired;
    srVector3T<float> seek;
    float scale;

    if (velocity_length_10 == g_float_005ebb34) {
        desired = target_14 - movement_00->position_040;
        desired.y = 0.0f;
        scale = desired.x * desired.x + desired.z * desired.z;
        if (scale != g_zero_005ebb40) {
            scale = static_cast<float>(g_double_005ebc30) / sqrt(scale);
            desired *= scale;
        }
        if (DotProduct(desired, direction_20) < g_zero_005ebb40) {
            speed_limit_08 = 0.0f;
            force_38 += desired * acceleration_0c;
            return;
        }
    }
    desired = target_14 - movement_00->position_040;
    desired.y = 0.0f;
    scale = desired.x * desired.x + desired.z * desired.z;
    if (scale != g_zero_005ebb40) {
        scale = speed_limit_08 / sqrt(scale);
        desired *= scale;
    }
    seek.Set(desired.x - movement_00->velocity_034.x, 0.0 - movement_00->velocity_034.y,
             desired.z - movement_00->velocity_034.z);
    seek = seek * g_path_acceleration_factor_0060f9e8;
    if (acceleration_0c < seek.Length()) {
        seek.SetLength(acceleration_0c);
    }
    force_38 += seek;
}

// FUNCTION: WIZ8 0x004cc420
void W8PathParameters::SeekWithApproachSpeed004CC420()
{
    float distance;
    float scale;

    distance = (target_14 - movement_00->position_040).Length();
    scale = distance / (g_path_approach_slow_time_0060f9f8 * speed_limit_08);
    if (g_float_005ebb38 < scale) {
        distance = distance / (g_path_approach_run_time_0060f9fc * speed_limit_08);
        if (g_float_005ebb38 < distance) {
            distance = g_float_005ebb38;
        }
        scale =
            (g_path_approach_run_rate_0060fa00 - g_float_005ebb38) * distance + g_float_005ebb38;
    }
    speed_limit_08 = scale * speed_limit_08;
    AccumulateSeekForce004CC1A0();
}

// FUNCTION: WIZ8 0x004cc4c0
void W8PathParameters::AccumulateGroupRepulsion004CC4C0()
{
    unsigned int index;
    W8Monster* monster;
    W8Navigator* navigator;
    srVector3T<float> position;
    srVector3T<float> delta;
    float distance;
    float combined;
    float falloff;
    float scale;

    if (QueryNearbyNavigators004CAFC0() != '\0') {
        for (index = 0; index < nearby_count_4c; ++index) {
            monster = GetMonsterByLocationID(nearby_locations_50[index]);
            if (monster == 0) {
                navigator = 0;
            } else {
                navigator = monster;
            }
            if (navigator->IsLinkedToNavigator00452E10(monster_54) == '\0') {
                continue;
            }
            position = monster->GetPosition();
            delta = position - movement_00->position_040;
            distance = delta.Length();
            combined = (radius_44 + radius_44 + monster->radius_084) * g_float_005ed2e8;
            if (distance < combined * g_float_005ec52c) {
                falloff = g_float_005ebb38;
                if (combined < distance) {
                    falloff = combined / distance;
                }
                delta = delta * static_cast<float>(g_double_005ec2e8);
                scale = delta.LengthSquared();
                if (scale != g_zero_005ebb40) {
                    scale = (g_path_group_repulsion_factor_0060fa08 * acceleration_0c * falloff *
                             falloff) /
                            sqrt(scale);
                    delta = delta * scale;
                }
                force_38 += delta;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004cc680
unsigned char W8PathParameters::SteerAroundLeader004CC680(char allow_path_fallback)
{
    W8Navigator* leader;
    srVector3T<float> heading;
    srVector3T<float> delta;
    srVector3T<float> offset;
    float leader_radius;
    float ahead;
    float lateral;
    float distance;
    double side;

    leader = monster_54->linked_navigator_05c;
    leader_radius = leader->radius_084;
    if (leader->movement_0c0.velocity_034.x == g_float_005ebb34 &&
        leader->movement_0c0.velocity_034.y == g_float_005ebb34 &&
        leader->movement_0c0.velocity_034.z == g_float_005ebb34) {
        heading.Set(0.0, 0.0, 1.0);
        heading.RotateAboutY(sin(leader->movement_0c0.yaw), cos(leader->movement_0c0.yaw));
    } else {
        heading = leader->movement_0c0.velocity_034;
        heading.Normalize();
    }
    if (leader->movement_0c0.velocity_034.x != g_float_005ebb34 ||
        leader->movement_0c0.velocity_034.y != g_float_005ebb34 ||
        leader->movement_0c0.velocity_034.z != g_float_005ebb34) {
        delta.x = movement_00->position_040.x - leader->movement_0c0.position_040.x;
        delta.z = movement_00->position_040.z - leader->movement_0c0.position_040.z;
        ahead = delta.x * heading.x + delta.z * heading.z +
                (movement_00->position_040.y - leader->movement_0c0.position_040.y) * heading.y;
        if (g_float_005ebb34 < ahead &&
            ahead < leader->movement_0c0.velocity_034.Length() * g_path_prediction_time_0060f9f4) {
            lateral = heading.z * delta.x - heading.x * delta.z;
            if (fabs(lateral) < static_cast<double>(leader_radius) + radius_44) {
                side = g_double_005ebc30;
                if (lateral < g_zero_005ebb40) {
                    side = g_double_005ec2e8;
                }
                offset.Set(heading.z * side * g_double_005ec150, 0.0,
                           -heading.x * side * g_double_005ec150);
                target_14 = movement_00->position_040 + offset;
                speed_limit_08 = g_path_approach_run_rate_0060fa00 * speed_limit_08;
                AccumulateSeekForce004CC1A0();
                return 1;
            }
        }
    }
    if (allow_path_fallback != '\0' &&
        static_cast<int>(movement_00->attachment_0ac->value_04) <
            static_cast<int>(leader->movement_0c0.attachment_0ac->value_04) +
                static_cast<int>(movement_00->attachment_0ac->value_0c)) {
        delta = leader->movement_0c0.position_040 - movement_00->position_040;
        distance = delta.Length() - leader_radius;
        if (g_path_approach_slow_time_0060f9f8 * speed_limit_08 < distance) {
            distance = distance / (g_path_approach_run_time_0060f9fc * speed_limit_08);
            if (g_float_005ebb38 < distance) {
                distance = g_float_005ebb38;
            }
            speed_limit_08 = ((g_path_approach_run_rate_0060fa00 - g_float_005ebb38) * distance +
                              g_float_005ebb38) *
                             speed_limit_08;
        }
        return 0;
    }
    offset = heading * (leader_radius * static_cast<float>(g_double_005ec2e8));
    target_14 = leader->movement_0c0.position_040 + offset;
    SeekWithApproachSpeed004CC420();
    return 1;
}

// FUNCTION: WIZ8 0x004ccad0
void W8PathParameters::SteerFromPathStart004CCAD0(W8NavigatorMovementState* movement,
                                                  char alternate)
{
    InitializeSteeringContext004CAE50(movement);
    movement->attachment_0ac->flags_00 = movement->attachment_0ac->flags_00 & 0xffefffff;
    if (HandleObstacleAhead004CBB70() == '\0') {
        if (PredictNavigatorCollision004CB620() != '\0') {
            movement->attachment_0ac->flags_00 = movement->attachment_0ac->flags_00 | 0x100000;
        } else if (alternate == '\0' && SteerAroundLeader004CC680(1) != '\0') {
            AccumulateGroupRepulsion004CC4C0();
            IntegrateSteering004CB090();
            return;
        } else {
            movement->attachment_0ac->GetNextPosition00456660(&target_14);
            AccumulateSeekForce004CC1A0();
        }
    }
    if (alternate == '\0') {
        AccumulateGroupRepulsion004CC4C0();
    }
    IntegrateSteering004CB090();
}

// FUNCTION: WIZ8 0x004ccb60
unsigned char W8PathParameters::SteerAlongPath004CCB60(W8NavigatorMovementState* movement,
                                                       char alternate)
{
    srVector3T<float> ahead;
    unsigned char advanced;
    float reach;

    advanced = 0;
    InitializeSteeringContext004CAE50(movement);
    movement->attachment_0ac->flags_00 = movement->attachment_0ac->flags_00 & 0xffefffff;
    if (HandleObstacleAhead004CBB70() == '\0') {
        if (g_float_005ebb34 < velocity_length_10) {
            reach = g_path_prediction_time_0060f9f4 * velocity_length_10;
            ahead = movement_00->position_040 + direction_20 * reach;
            if (movement->attachment_0ac->CheckPredictedHopHeight00456DD0(&ahead) == '\0') {
                target_14 = movement_00->position_040;
                advanced = movement->attachment_0ac->AdvancePositionTowardWaypoint00456F60(
                    &target_14, reach);
                AccumulateSeekForce004CC1A0();
                goto steered;
            }
        }
        if (PredictNavigatorCollision004CB620() != '\0') {
            movement->attachment_0ac->flags_00 = movement->attachment_0ac->flags_00 | 0x100000;
            goto steered;
        }
        if (alternate == '\0' && SteerAroundLeader004CC680(1) != '\0') {
            AccumulateGroupRepulsion004CC4C0();
            IntegrateSteering004CB090();
            return 0;
        }
        movement->attachment_0ac->GetNextPosition00456660(&target_14);
        AccumulateSeekForce004CC1A0();
    }
steered:
    if (alternate == '\0') {
        AccumulateGroupRepulsion004CC4C0();
    }
    IntegrateSteering004CB090();
    return advanced;
}

// FUNCTION: WIZ8 0x004cccb0
unsigned char LoadPathParameters004CCCB0()
{
    int handle;
    int index;
    unsigned char more = 1;
    char line[128];

    handle = FileOpen("Data\\Monsters\\pathparms.txt", 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    for (;;) {
        do {
            if (more == 0) {
                FileClose(handle);
                return 1;
            }
            ReadTextLine004CEE40(handle, line, sizeof(line), &more);
        } while (line[0] == '#');
        for (index = 0; g_path_parameters[index].name != 0; ++index) {
            if (strncmp(line, g_path_parameters[index].name,
                        strlen(g_path_parameters[index].name)) == 0) {
                sscanf(line, "%*s %f", g_path_parameters[index].value);
                break;
            }
        }
    }
}
