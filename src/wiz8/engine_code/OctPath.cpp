#include "wiz8/sgp_text.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/OctMeshModel.h"
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
#include "surrender/srScene.h"
#include "surrender/srModelInstance.h"
#include "FileMan.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/GDFileIO.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/GameTimeAccumulator.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/PolyPick.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/environment_colour.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/MonsterAI.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Sight.h"
#include "random.h"

#include "Font.h"
#include "input.h"
#include "sgp.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GLOBAL: WIZ8 0x005ebc30
const double g_double_005ebc30 = 1.0;
// GLOBAL: WIZ8 0x005ec020
const float g_float_005ec020 = 0.0f;
// GLOBAL: WIZ8 0x005ec368
const double g_double_005ec368 = 25.00000037252903;
// GLOBAL: WIZ8 0x005ec378
const double g_double_005ec378 = 4.0;
// GLOBAL: WIZ8 0x005ec38c
const float g_float_005ec38c = 0.9847999811172485f;
// GLOBAL: WIZ8 0x005ec384
const float g_float_005ec384 = 37500.0f;
// GLOBAL: WIZ8 0x005ec388
const float g_float_005ec388 = 100000.0f;
// GLOBAL: WIZ8 0x005ec370
const float g_float_005ec370 = 550.0f;
// GLOBAL: WIZ8 0x005ec394
const float g_float_005ec394 = 1.2000000476837158f;
// GLOBAL: WIZ8 0x005ec398
const float g_float_005ec398 = 1001.0f;
// GLOBAL: WIZ8 0x005ec39c
const float g_float_005ec39c = 250000.0f;
// GLOBAL: WIZ8 0x005ec3b8
const float g_float_005ec3b8 = 1.5f;
// GLOBAL: WIZ8 0x005ec3bc
const float g_float_005ec3bc = 0.5099999904632568f;
// GLOBAL: WIZ8 0x005ec3c0
const float g_float_005ec3c0 = 1000000.0f;
// GLOBAL: WIZ8 0x005ec3c8
const float g_float_005ec3c8 = -107374184.0f;
// GLOBAL: WIZ8 0x005ec3d0
const float g_float_005ec3d0 = -107374184.0f;
// GLOBAL: WIZ8 0x005ed2e0
const double g_double_005ed2e0 = 0.2;
// GLOBAL: WIZ8 0x005ed2e8
const float g_float_005ed2e8 = 0.3333333f;
// GLOBAL: WIZ8 0x005ec2e8
const double g_double_005ec2e8 = -1.0;

// GLOBAL: WIZ8 0x00659c60
W8PathingService* g_pathing;

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

// GLOBAL: WIZ8 0x0060827a
unsigned short g_path_reserve = 2000;
// GLOBAL: WIZ8 0x005ec340
const float g_fast_magic_recovery_scale = 1.25f;
// GLOBAL: WIZ8 0x005ec344
const float g_path_span_scale = 1.5259254723787308e-05f;

// GLOBAL: WIZ8 0x00659c5c
static bool g_flag_00659c5c;
// GLOBAL: WIZ8 0x00659c64
static unsigned short* g_path_scratch;
// GLOBAL: WIZ8 0x005ec3a8
const double g_double_005ec3a8 = 1.1;
// GLOBAL: WIZ8 0x005ec3a0
const double g_double_005ec3a0 = 25000.0;
// GLOBAL: WIZ8 0x005ec3b0
const double g_double_005ec3b0 = 0.1;
// GLOBAL: WIZ8 0x005ec348
const float g_path_direction_threshold_0 = -0.9239000082015991f;
// GLOBAL: WIZ8 0x005ec34c
const float g_path_direction_threshold_1 = -0.38269999623298645f;
// GLOBAL: WIZ8 0x005ec350
const float g_path_direction_threshold_2 = 0.38269999623298645f;
// GLOBAL: WIZ8 0x005ec354
const float g_path_direction_threshold_3 = 0.9239000082015991f;
// GLOBAL: WIZ8 0x005ec358
const float g_path_cardinal_scale = 1.4149999618530273f;

// GLOBAL: WIZ8 0x005ec360
const float g_float_005ec360 = 25000.0f;
// GLOBAL: WIZ8 0x00659c6c
static unsigned int g_path_visualization_cell;
// GLOBAL: WIZ8 0x005ec380
const float g_path_search_visualization_limit = 15000.0f;

// GLOBAL: WIZ8 0x0060f9e8
static float g_path_acceleration_factor = 3.0f;
// GLOBAL: WIZ8 0x0060f9ec
static float g_path_angular_acceleration_factor = 2.0f;
// GLOBAL: WIZ8 0x0060f9f0
static float g_path_angular_deceleration_factor = 0.5f;
// GLOBAL: WIZ8 0x0060f9f4
static float g_path_prediction_time = 1.0f;
// GLOBAL: WIZ8 0x0060f9f8
static float g_path_approach_slow_time = 1.0f;
// GLOBAL: WIZ8 0x0060f9fc
static float g_path_approach_run_time = 8.0f;
// GLOBAL: WIZ8 0x0060fa00
static float g_path_approach_run_rate = 2.0f;
// GLOBAL: WIZ8 0x0060fa04
static float g_path_lookahead_time = 1.0f;
// GLOBAL: WIZ8 0x0060fa08
static float g_path_group_repulsion_factor = 1.0f;
// GLOBAL: WIZ8 0x0060fa0c
static float g_path_party_boundary_radius = 2.0f;
// GLOBAL: WIZ8 0x0060fa10
static float g_path_obstacle_steering_factor = 2.0f;
// GLOBAL: WIZ8 0x0060fa14
static float g_path_obstacle_braking_factor = 0.3f;

#define OCTPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPath.cpp"

unsigned char LoadPathParameters();

/* The path-search heap specialization is emitted after OctPath.cpp's ordinary
   bodies. The generic definitions live once in stHeap.hpp. */

/* Advance the search queue and mark the node that was just expanded. The
   generic heap delete is visible here as the assertion and sift-down sequence
   in the retail body; an empty queue publishes node zero. */
// FUNCTION: WIZ8 0x004577F0
void W8PathHeapHandle::DeleteRoot(W8PathSearchNode* node)
{
    if (heap->size == 0) {
        root_node = 0;
    } else {
        root_node = heap->Delete().node;
    }
    node->flags |= 4;
}

/* Write the path hash serialization and its five conditional tables. Counts
   smaller than the two sentinel entries are normalized to an empty set before
   the header is emitted, exactly as the read side treats them. */
// FUNCTION: WIZ8 0x00458ad0
unsigned char W8PathingService::WritePathNodes(unsigned int handle)
{
    W8ConditionalPathHeader header;
    unsigned char success;

    if (path_node_count != 0) {
        success = FileWrite(handle, file_path_nodes, path_node_count << 3, 0);
        if (success == 0) {
            ReportBuildStatus(7, "WritePathNodes: Couldn't write Path Hash array.\n");
            return 0;
        }
    }

    if (static_cast<unsigned int>(m_ulNumCondFrames) < 2 ||
        static_cast<unsigned int>(m_ulNumCondNodes) < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondFrames = 0;
        m_ulNumCondNodes = 0;
    }
    header.path_count = m_ulNumCondPaths;
    header.frame_count = m_ulNumCondFrames;
    header.node_count = m_ulNumCondNodes;
    header.flags = 0;
    success = FileWrite(handle, &header, sizeof(header), 0);
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
unsigned char W8PathingService::SaveWaypointSnapshot(bool force)
{
    if (waypoints_dirty == 0 && force == 0) {
        return 0;
    }
    if (FileExists("cd.rom") != 0) {
        return 0;
    }

    BuildWaypointFileData();
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
        m_pFileWayPoints[index].flags = m_pSurfaces[index].flags & 0xffdf;
        m_pFileWayPoints[index].first_edge = m_pSurfaces[index].first_edge;
        m_pFileWayPoints[index].position = m_pSurfaces[index].position;
    }
    for (index = 0; index < m_ulNumWayPtLinks; ++index) {
        m_pEdges[index].flags &= 0x7fffffff;
    }

    unsigned char result = WriteWaypointFile();
    free(m_pFileWayPoints);
    m_pFileWayPoints = 0;
    return result;
}

/* Write the version-two waypoint snapshot. Retail combines the six write
   results with OR, so a partially successful sequence still reports success;
   that behavior is part of the recovered format contract. */
// FUNCTION: WIZ8 0x00459540
unsigned char W8PathingService::WriteWaypointFile()
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
        result = FileWrite(handle, &version, sizeof(version), 0);
        result |= FileWrite(handle, &edge_node_count, sizeof(edge_node_count), 0);
        result |= FileWrite(handle, &m_ulNumWayPoints, sizeof(m_ulNumWayPoints), 0);
        result |= FileWrite(handle, &m_ulNumWayPtLinks, sizeof(m_ulNumWayPtLinks), 0);
        result |= FileWrite(handle, m_pFileWayPoints, m_ulNumWayPoints * sizeof(W8FileWaypoint), 0);
        result |= FileWrite(handle, m_pEdges, m_ulNumWayPtLinks * sizeof(W8PathEdge), 0);
    }
    FileClose(handle);
    return result;
}

/* Read a versioned .WPT snapshot and rebuild its live graph representation.
   Version one stores the four persistent edge fields separately; later files
   contain the complete packed 0x0e-byte edge record. */
// FUNCTION: WIZ8 0x00459650
unsigned char W8PathingService::ReadWaypointFile()
{
    unsigned int version = 2;
    unsigned char success = 0;
    char path[256];
    if (path_node_count == 0)
        return 0;
    sprintf(path, "%s.WPT", level_name);
    unsigned int handle = FileOpen(path, 1, 0);
    if (handle == 0)
        return 0;

    success = FileRead(handle, &version, 4, 0);
    success |= FileRead(handle, &edge_node_count, 4, 0);
    success |= FileRead(handle, &m_ulNumWayPoints, 4, 0);
    success |= FileRead(handle, &m_ulNumWayPtLinks, 4, 0);
    if (success == 0) {
        FileClose(handle);
        return 0;
    }
    unsigned int surface_capacity = (m_ulNumWayPoints / 100 + 1) * 100;
    unsigned int edge_capacity = (m_ulNumWayPtLinks / 100 + 1) * 100;
    m_pFileWayPoints =
        static_cast<W8FileWaypoint*>(malloc(m_ulNumWayPoints * sizeof(W8FileWaypoint)));
    m_pSurfaces = static_cast<W8PathSurface*>(malloc(surface_capacity * sizeof(W8PathSurface)));
    m_pEdges = static_cast<W8PathEdge*>(malloc(edge_capacity * sizeof(W8PathEdge)));
    if (m_pSurfaces != 0)
        memset(m_pSurfaces, 0, surface_capacity * sizeof(W8PathSurface));
    if (m_pEdges != 0)
        memset(m_pEdges, 0, edge_capacity * sizeof(W8PathEdge));
    if (m_pFileWayPoints == 0 || m_pSurfaces == 0 || m_pEdges == 0) {
        if (m_pFileWayPoints != 0)
            free(m_pFileWayPoints);
        if (m_pSurfaces != 0)
            free(m_pSurfaces);
        if (m_pEdges != 0)
            free(m_pEdges);
        FileClose(handle);
        return 0;
    }

    success = FileRead(handle, m_pFileWayPoints, m_ulNumWayPoints * sizeof(W8FileWaypoint), 0);
    if (success != 0) {
        if (version == 1) {
            for (unsigned int edge = 0; edge < m_ulNumWayPtLinks; ++edge) {
                W8PathEdge* item = &m_pEdges[edge];
                success &= FileRead(handle, &item->flags, 4, 0);
                success &= FileRead(handle, &item->destination, 2, 0);
                success &= FileRead(handle, &item->distance, 4, 0);
                success &= FileRead(handle, &item->next, 2, 0);
                item->source = 0;
            }
        } else {
            success &= FileRead(handle, m_pEdges, m_ulNumWayPtLinks * sizeof(W8PathEdge), 0);
        }
    }
    if (success == 0) {
        free(m_pFileWayPoints);
        free(m_pSurfaces);
        free(m_pEdges);
        FileClose(handle);
        return 0;
    }

    delete path_heap;
    unsigned int heap_capacity = surface_capacity;
    if (heap_capacity <= g_path_reserve)
        heap_capacity = g_path_reserve;
    path_heap = new W8PathHeapHandle;
    path_heap->heap = new W8PathHeap;
    path_heap->heap->entries = new W8PathHeapEntry[heap_capacity];
    if (path_heap->heap->entries == 0)
        srAssertFail("hlist", "..\\Engine Code\\Include\\stHeap.hpp", 0x79, 0);
    path_heap->heap->external_storage = 0;
    path_heap->heap->capacity = heap_capacity;
    path_heap->heap->size = 0;

    memset(m_pSurfaces, 0, m_ulNumWayPoints * sizeof(W8PathSurface));
    for (unsigned int surface = 1; surface < m_ulNumWayPoints; ++surface) {
        W8FileWaypoint* source = &m_pFileWayPoints[surface];
        W8PathSurface* destination = &m_pSurfaces[surface];
        destination->flags = source->flags;
        destination->index = static_cast<unsigned short>(surface);
        destination->first_edge = source->first_edge;
        destination->position = source->position;
        srVector3T<int> point;
        g_octree->WorldPositionToCell(&destination->position, &point);
        g_octree->object_registry->MoveObjectToCell(W8_OCTREE_KIND_WAYPOINT, surface + 1, &point);
        if ((destination->flags & 0xf000) == 0)
            destination->flags |= 0x2000;
    }
    m_visible_waypoints->SetSize(surface_capacity);
    m_rendered_waypoints->SetSize(surface_capacity);
    m_collected_waypoints->SetSize(surface_capacity);
    FileClose(handle);
    g_path_scratch =
        static_cast<unsigned short*>(malloc(surface_capacity * sizeof(unsigned short)));

    if (version <= 2) {
        unsigned int surface;
        for (surface = 1; surface < m_ulNumWayPoints; ++surface) {
            if ((ClassifyWaypoint(&m_pSurfaces[surface].position) & 0x04000000) != 0)
                m_pSurfaces[surface].flags |= 0x40;
        }
        for (surface = 1; surface < m_ulNumWayPoints; ++surface) {
            unsigned short edge_index = m_pSurfaces[surface].first_edge;
            while (edge_index != 0) {
                W8PathEdge* edge = &m_pEdges[edge_index];
                edge->source = static_cast<unsigned short>(surface);
                unsigned int destination_index = edge->destination;
                if ((m_pSurfaces[surface].flags & 0x40) != 0 ||
                    (m_pSurfaces[destination_index].flags & 0x40) != 0) {
                    edge->flags |= 0x20000000;
                } else {
                    TestWaypointSpan(&m_pSurfaces[surface].position,
                                     &m_pSurfaces[destination_index].position, 0, 0);
                    if (span_blocked != 0)
                        edge->flags |= 0x20000000;
                }
                edge_index = edge->next;
            }
        }
    }
    waypoints_dirty = 1;
    return success;
}

/* Compact the editable waypoint graph. Invalid and dead-end surfaces are
   unregistered and discarded; every surviving surface and edge is packed
   toward its sentinel and all indices are rewritten through temporary maps. */
// FUNCTION: WIZ8 0x0045e440
void W8PathingService::BuildWaypointFileData()
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
        W8PathSurface* surface = &m_pSurfaces[old_surface];
        if (surface->index == 0 || surface->first_edge == 0) {
            g_octree->object_registry->UnregisterObject(W8_OCTREE_KIND_WAYPOINT, old_surface + 1);
            if (surface->first_edge == 0) {
                unsigned short removed = 0;
                unsigned int edge;
                for (edge = 1; edge < m_ulNumWayPtLinks; ++edge) {
                    if (m_pEdges[edge].destination == old_surface) {
                        /* Retail passes the discarded surface, not this incoming edge. */
                        RemoveWaypointLink(static_cast<unsigned short>(old_surface));
                        ++removed;
                    }
                }
                if (g_dev_mode != 0) {
                    if (removed == 0) {
                        FormatDebugMessage(0, "Deleting Isolated WayPt at (%.1f, %.1f, %.1f)",
                                           surface->position.x, surface->position.y,
                                           surface->position.z);
                    } else {
                        FormatDebugMessage(0, "Deleting Dead End WayPt at (%.1f, %.1f, %.1f)",
                                           surface->position.x, surface->position.y,
                                           surface->position.z);
                    }
                }
            }
        } else {
            surface_map[old_surface] = next_surface;
            if (old_surface != next_surface) {
                m_pSurfaces[next_surface] = *surface;
                m_pSurfaces[next_surface].index = next_surface;
                g_octree->object_registry->UnregisterObject(W8_OCTREE_KIND_WAYPOINT,
                                                            old_surface + 1);
                srVector3T<int> point;
                g_octree->WorldPositionToCell(&m_pSurfaces[next_surface].position, &point);
                g_octree->object_registry->MoveObjectToCell(W8_OCTREE_KIND_WAYPOINT,
                                                            next_surface + 1, &point);
            }
            ++next_surface;
        }
    }

    unsigned int old_edge;
    for (old_edge = 1; old_edge < m_ulNumWayPtLinks; ++old_edge) {
        W8PathEdge* edge = &m_pEdges[old_edge];
        if (edge->destination != 0) {
            edge->destination = surface_map[edge->destination];
            edge->source = surface_map[edge->source];
            edge_map[old_edge] = next_edge;
            if (old_edge != next_edge) {
                m_pEdges[next_edge] = *edge;
            }
            ++next_edge;
        }
    }

    memset(&m_pEdges[next_edge], 0, (m_ulNumWayPtLinks - next_edge) * sizeof(W8PathEdge));
    memset(&m_pSurfaces[next_surface], 0,
           (m_ulNumWayPoints - next_surface) * sizeof(W8PathSurface));
    m_ulNumWayPoints = next_surface;
    m_ulNumWayPtLinks = next_edge;

    for (old_surface = 1; old_surface < m_ulNumWayPoints; ++old_surface) {
        m_pSurfaces[old_surface].first_edge =
            edge_map[m_pSurfaces[old_surface].first_edge];
    }
    for (old_edge = 1; old_edge < m_ulNumWayPtLinks; ++old_edge) {
        m_pEdges[old_edge].next = edge_map[m_pEdges[old_edge].next];
    }
    saved_surface = surface_map[saved_surface];
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
unsigned char W8PathingService::ReadPathNodes(int handle)
{
    char acMessage[256];
    W8ConditionalPathHeader header;
    unsigned int uiRead;
    W8FilePathNode* buffer;
    W8FilePathNode* scan;
    unsigned char fSuccess = 0;
    unsigned int index;

    if (path_node_count != 0) {
        m_pPathValues = new W8HashTable<unsigned int, unsigned int>;
        m_pVisitedCells = new W8OctreeIndex;
        buffer = static_cast<W8FilePathNode*>(malloc(path_node_count * sizeof(W8FilePathNode)));
        if (m_pPathValues == 0 || buffer == 0) {
            strcpy(acMessage, "ReadPathNodes: Couldn't allocate path hash array.");
        } else {
            fSuccess = FileRead(handle, buffer, path_node_count * sizeof(W8FilePathNode), &uiRead);
            if (fSuccess == 0) {
                strcpy(acMessage, "ReadPathNodes: Couldn't read path hash array.");
                free(buffer);
            } else {
                scan = buffer;
                for (index = 0; index < static_cast<unsigned int>(path_node_count); ++index) {
                    m_pPathValues->Insert(&scan->cell, &scan->level_flags);
                    ++scan;
                }
                free(buffer);
            }
        }
    }
    fSuccess = FileRead(handle, &header, sizeof(header), &uiRead);
    if (fSuccess == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8fa,
                     "ReadPathNodes: Couldn't write Conditional Counts.\n");
    }
    m_ulNumCondPaths = header.path_count;
    m_ulNumCondFrames = header.frame_count;
    m_ulNumCondNodes = header.node_count;
    path_flags0 = header.flags;
    if (m_ulNumCondFrames < 2 || m_ulNumCondNodes < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondFrames = 0;
        m_ulNumCondNodes = 0;
        return fSuccess;
    }
    m_pCondPaths =
        static_cast<GDPropCondPaths*>(malloc(header.path_count * sizeof(GDPropCondPaths)));
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

/* The path-linking callers share this conversion. Retail computes Z before
   testing the optional destination, then writes X and Z together. */
static void PositionToPathPoint(const W8PathingService* pathing, const srVector3T<float>* position,
                                srVector2i* point)
{
    int converted =
        static_cast<int>((position->z - pathing->level_bounds.minimum.z) / pathing->grid_scale);
    if (point != 0) {
        point->x = static_cast<int>((position->x - pathing->level_bounds.minimum.x) /
                                    pathing->grid_scale);
        point->y = converted;
    }
}

/* Retail also packs the converted coordinates into a path-cell key before
   conditionally returning the individual coordinates to its caller. */
static unsigned int PositionToPathKey(const W8PathingService* pathing,
                                      const srVector3T<float>* position, srVector2i* cell)
{
    int x =
        static_cast<int>((position->x - pathing->level_bounds.minimum.x) / pathing->grid_scale);
    int z =
        static_cast<int>((position->z - pathing->level_bounds.minimum.z) / pathing->grid_scale);
    unsigned int key = static_cast<unsigned int>(z) * 0x10000 + static_cast<unsigned int>(x);
    if (cell != 0) {
        cell->x = x;
        cell->y = z;
    }
    return key;
}

/* Offer every flagged surface to the path builder.

   Surfaces are 0x28 bytes apart and the walk starts at index one, so entry zero
   is never a real surface. The point handed on is the surface's own position
   converted to the graph's integer grid - x from the bounds floor, z from the
   third bound - and the two conversions happen in the order the point's fields
   do not. */
// FUNCTION: WIZ8 0x00460020
void W8PathingService::LinkSurfaces(GDProp* prop)
{
    unsigned int index = 1;
    srVector2i point;
    W8PathSurface* surface;

    if (m_ulNumWayPoints <= index) {
        return;
    }
    do {
        surface = &m_pSurfaces[index];
        if ((surface->flags & 0x40) != 0) {
            PositionToPathPoint(this, &surface->position, &point);
            prop->RegisterPathSurface(index, &point);
        }
        ++index;
    } while (index < m_ulNumWayPoints);
}

/* The same for the edges, which are 0xe bytes apart, gated by a different flag,
   and which name two surfaces by index in their shorts at +4 and +6. Each of
   those surfaces contributes one converted point, so the builder receives the
   edge as a pair. */
// FUNCTION: WIZ8 0x004600b0
void W8PathingService::LinkEdges(GDProp* prop)
{
    unsigned int index = 1;
    srVector2i first;
    srVector2i second;
    W8PathEdge* edge;
    W8PathSurface* surface;

    if (m_ulNumWayPtLinks <= index) {
        return;
    }
    do {
        edge = &m_pEdges[index];
        if ((edge->flags & 0x20000000) != 0) {
            surface = &m_pSurfaces[edge->source];
            PositionToPathPoint(this, &surface->position, &first);

            surface = &m_pSurfaces[edge->destination];
            PositionToPathPoint(this, &surface->position, &second);
            prop->RegisterPathVertex(index, &first, &second);
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
void W8PathingService::SetConditionalPathFrame(unsigned int path_handle, unsigned short frame)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    unsigned int lookup_index = path_handle;
    bool frame_missing = 1;

    while (m_pulCondLookup[lookup_index] != 0 && frame_missing != 0) {
        if (m_pusCondNodeFrames[lookup_index] == frame) {
            frame_missing = 0;
        }
        ++lookup_index;
    }

    if (frame == 0xffff) {
        lookup_index = path_handle;
        while (m_pulCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int current_value =
                    FindConditionalPathValue(key, m_pulCondNodeValues[key_index]);
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
                current_value = FindConditionalPathValue(key, m_pulCondNodeValues[key_index]);
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
                    FindConditionalPathValue(key, m_pulCondNodeValues[key_index]);
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
                    FindConditionalPathValue(key, m_pulCondNodeValues[key_index]);
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
unsigned int W8PathingService::FindConditionalPathValue(unsigned int key, unsigned int value)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);
    unsigned int found = 0;
    bool searching = true;
    while (slot >= 0 && searching != 0) {
        unsigned int candidate = index->entries[slot].value;
        if (((candidate ^ value) & 0xffff) == 0) {
            searching = false;
            found = candidate;
        }
        slot = index->FindNextEntry(&key, slot);
    }
    return found;
}

/* Refresh the disabled bit for a conditional list of waypoints. */
// FUNCTION: WIZ8 0x004601b0
void W8PathingService::CheckConditionalWayPtStatus(unsigned short count, unsigned short* waypoints)
{
    while (count != 0) {
        unsigned short waypoint = *waypoints;
        if (waypoint >= static_cast<unsigned short>(m_ulNumWayPoints)) {
            srAssertFail("pusWayPts[i] < (UINT16)m_ulNumWayPoints", OCTPATH_CPP, 0x1a4e,
                         "Pathing::CheckConditionalWayPtStatus: WayPt Index out of range.");
        }

        W8PathSurface* surface = &m_pSurfaces[waypoint];
        if ((ClassifyWaypoint(&surface->position) & 0x10000000) == 0) {
            surface->flags &= 0xffdf;
        } else {
            surface->flags |= 0x20;
        }
        ++waypoints;
        --count;
    }
}

/* Refresh the disabled bit for conditional path edges. Only edges carrying
   the conditional-span flag participate. A disabled source always disables
   its edge; otherwise the current span test decides the bit. */
// FUNCTION: WIZ8 0x00460250
void W8PathingService::CheckConditionalLinkStatus(unsigned short count, unsigned short* edges)
{
    while (count != 0) {
        unsigned short edge_index = *edges;
        if (edge_index >= static_cast<unsigned short>(m_ulNumWayPtLinks)) {
            srAssertFail("pusLinks[i] < (UINT16)m_ulNumWayPtLinks", OCTPATH_CPP, 0x1a6c,
                         "Pathing::CheckConditionalLinkStatus: Link Index out of range.");
        }

        W8PathEdge* edge = &m_pEdges[edge_index];
        if ((edge->flags & 0x20000000) != 0) {
            if ((m_pSurfaces[edge->source].flags & 0x20) != 0 ||
                TestWaypointSpan(&m_pSurfaces[edge->source].position,
                                 &m_pSurfaces[edge->destination].position, 0, 0) == 0) {
                edge->flags |= 0x80000000;
            } else {
                edge->flags &= 0x7fffffff;
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
unsigned char W8PathingService::HandlePathEdgeTransition(W8NavigatorMovementState* movement)
{
    W8NavigatorAttachment* attachment = movement->attachment;
    unsigned short cursor = attachment->path_cursor;
    unsigned int flags = 0;
    unsigned short destination;

    if (cursor < attachment->path_position_index) {
        unsigned short* pairs = attachment->path_values;
        unsigned short source = pairs[cursor];
        destination = pairs[cursor + 1];
        unsigned short edge_index = m_pSurfaces[source].first_edge;

        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges[edge_index];
            if (edge->destination == destination) {
                flags = edge->flags;
                break;
            }
            edge_index = edge->next;
        }
    }

    if ((flags & 0x80000000) != 0 &&
        ((flags & 0x10000000) == 0 || (movement->movement_flags & 0x10000000) == 0)) {
        return 0;
    }
    if ((flags & 0x01000000) == 0) {
        return 1;
    }

    attachment->path_cursor += 2;
    movement->position = m_pSurfaces[destination].position;
    attachment->position4 = m_pSurfaces[destination].position;
    if ((attachment->flags & 0x01000000) == 0) {
        attachment->flags |= 0x01000000;
    } else {
        attachment->flags &= 0xfeffffff;
    }
    return 2;
}

/* Measure the route between two points for the noise line-of-sight query. A
   throwaway attachment is built over (from, to); a positive `range` budget is
   held in m_path_cost_limit while the path builds, and both
   exits restore the 1.0e10f limit. On success `range` receives the
   measured path length and `hops` the count of hop segments whose link is
   gated by a door. */
// FUNCTION: WIZ8 0x004604B0
unsigned char W8PathingService::MeasureAttachmentPath(const srVector3T<float>* from,
                                                      srVector3T<float>* to, float* range,
                                                      int* hops)
{
    W8NavigatorAttachment attachment(from, to);
    if (*range > g_float_zero) {
        m_path_cost_limit = *range;
    }
    if (BuildAttachmentPath(&attachment, 0) == 0) {
        m_path_cost_limit = 1.0e10f;
        return 0;
    }

    unsigned short cursor = attachment.path_cursor;
    if ((attachment.flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
        --cursor;
    }
    attachment.position_cursor = cursor;
    ++attachment.position_cursor;

    srVector3T<float> next;
    if (attachment.position_cursor < attachment.path_position_index) {
        next = attachment.position7[attachment.position_cursor];
        TestWaypointSpan(from, &next, 0, 0);
    } else {
        next = attachment.position1;
    }

    *range = attachment.MeasurePathLength();
    *hops = 0;
    while (attachment.path_cursor < attachment.path_position_index) {
        if (TestAttachmentHopDoor(&attachment) != 0) {
            ++*hops;
        }
        ++attachment.path_cursor;
    }
    m_path_cost_limit = 1.0e10f;
    return 1;
}

/* Whether the hop at the attachment's current index runs over a disabled
   conditional edge guarded by a closed door. The current path value pair names
   the two surfaces; when their link carries both the conditional and disabled
   bits, the props inside the segment's bounds box are queried and the one
   nearest the midpoint supplies the trigger. The hop counts when that
   trigger's action record is a type-10 door record whose flag bit is clear. */
// FUNCTION: WIZ8 0x00460680
unsigned char W8PathingService::TestAttachmentHopDoor(W8NavigatorAttachment* attachment)
{
    unsigned short current = attachment->path_cursor;
    if (current < attachment->path_position_index) {
        unsigned short* pairs = attachment->path_values;
        unsigned short source = pairs[current];
        unsigned short destination = pairs[current + 1];
        W8PathSurface* source_surface = &m_pSurfaces[source];
        unsigned short edge_index = source_surface->first_edge;
        bool found = false;
        while (edge_index != 0) {
            if (found != 0) {
                break;
            }
            if (m_pEdges[edge_index].destination == destination) {
                found = 1;
            } else {
                edge_index = m_pEdges[edge_index].next;
            }
        }
        if (edge_index != 0 && (m_pEdges[edge_index].flags & 0x10000000) != 0 &&
            (m_pEdges[edge_index].flags & 0x80000000) != 0) {
            srVector3T<float> lower = source_surface->position;
            srVector3T<float> upper = source_surface->position;
            GrowBoundsByPoint(&m_pSurfaces[destination].position, &lower, &upper);
            unsigned long* candidates = 0;
            Trigger* selected = 0;
            int count =
                g_octree->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
            if (count > 0) {
                if (count == 1) {
                    W8Prop* prop = *g_world->collidable_props->GetAt(candidates[0]);
                    selected = prop->GetGDPropOwnerTrigger();
                    if (selected == 0) {
                        return 0;
                    }
                } else {
                    double nearest_distance = 1e32;
                    srVector3T<float> center = (upper - lower) * g_double_005ebe80 + lower;
                    for (int index = 0; index < count; ++index) {
                        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
                        Trigger* trigger = prop->GetGDPropOwnerTrigger();
                        if (trigger != 0) {
                            srVector3T<float> position;
                            prop->GetPosition(&position);
                            float distance = (position - center).LengthSquared();
                            if (distance < nearest_distance) {
                                nearest_distance = distance;
                                selected = trigger;
                            }
                        }
                    }
                }
                W8TriggerActionData* action_data = selected->m_pActionData;
                if (action_data == 0 || action_data->type != 10) {
                    action_data = 0;
                }
                if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 1) == 0) {
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
unsigned short W8PathingService::FindPath(W8NavigatorAttachment* attachment, unsigned int flags)
{
    unsigned short usStartNode = FindWaypoint(&attachment->position0, 0);
    if (usStartNode == 0 && (usStartNode = start_waypoint) == 0) {
        return 0;
    }
    unsigned int start = usStartNode;
    unsigned short usEndNode = FindWaypoint(&attachment->position1, 0);
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
    m_rendered_waypoints->ClearAll();
    m_visible_waypoints->ClearAll();
    path_heap->heap->size = 0;
    m_visible_waypoints->Set(start);
    m_pSurfaces[start].parent = 0;
    unsigned short usWayPt = usStartNode;
    if (usStartNode != usEndNode) {
        do {
            unsigned int current = usWayPt;
            if (m_ulNumWayPoints <= current) {
                srAssertFail("usWayPt < m_ulNumWayPoints", OCTPATH_CPP, 0x1bea,
                             "Waypoint index out of range (1)");
            }
            unsigned short usLink = m_pSurfaces[current].first_edge;
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
                    unsigned int edge_flags = m_pEdges[link].flags;
                    unsigned int usNextNode = m_pEdges[link].destination;
                    if (((m_pSurfaces[usNextNode].flags & 0x20) != 0) ||
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
                    unsigned short usCurrent = usWayPt;
                    if (m_ulNumWayPoints <= usNextNode) {
                        srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1bf5,
                                     "Waypoint index out of range (2)");
                    }
                    if ((usNext != 0) && (usNext != usCurrent) &&
                        (m_rendered_waypoints->Test(usNextNode) == 0)) {
                        if (m_visible_waypoints->Set(usNextNode) == 0) {
                            m_pSurfaces[usNextNode].cost =
                                m_pEdges[link].distance + m_pSurfaces[current].cost;
                            W8PathSurface* surfaces = m_pSurfaces;
                            if (surfaces[usNextNode].cost < m_path_cost_limit) {
                                float dx = surfaces[end].position.x -
                                           surfaces[usNextNode].position.x;
                                settled = false;
                                float dy = surfaces[end].position.y -
                                           surfaces[usNextNode].position.y;
                                float dz = surfaces[end].position.z -
                                           surfaces[usNextNode].position.z;
                                surfaces[usNextNode].heuristic =
                                    sqrt(dx * dx + dy * dy + dz * dz) * g_float_005ec394;
                                surfaces[usNextNode].parent = usCurrent;
                                surfaces = m_pSurfaces;
                                surfaces[current].remaining_cost =
                                    surfaces[usNextNode].heuristic * g_float_005ec394 +
                                    surfaces[usNextNode].cost;
                                m_pSurfaces[usNextNode].flags |= 0x10;
                                W8PathHeapHandle* handle = path_heap;
                                W8PathHeapEntry entry;
                                entry.node = usNextNode;
                                entry.priority =
                                    static_cast<unsigned int>(surfaces[current].remaining_cost);
                                handle->heap->Insert(&entry);
                                handle->root_node = handle->heap->entries[0].node;
                            }
                        } else {
                            float cost =
                                m_pEdges[link].distance + m_pSurfaces[current].cost;
                            if ((cost + g_float_005ebc88 < m_pSurfaces[usNextNode].cost) &&
                                (cost < m_path_cost_limit)) {
                                float reduction = m_pSurfaces[usNextNode].cost - cost;
                                m_pSurfaces[usNextNode].parent = usCurrent;
                                W8PathSurface* surfaces = m_pSurfaces;
                                settled = false;
                                surfaces[current].remaining_cost =
                                    surfaces[usNextNode].heuristic * g_float_005ec394 +
                                    surfaces[usNextNode].cost;
                                if ((m_pSurfaces[current].flags & 0x10) == 0) {
                                    surfaces = m_pSurfaces + usNextNode;
                                    surfaces->flags |= 0x10;
                                    W8PathHeapHandle* handle = path_heap;
                                    W8PathHeapEntry entry;
                                    entry.node = usNextNode;
                                    entry.priority = static_cast<unsigned int>(
                                        m_pSurfaces[current].remaining_cost);
                                    handle->heap->Insert(&entry);
                                    handle->root_node = handle->heap->entries[0].node;
                                }
                                ReduceWaypointCosts(usNextNode, reduction);
                            }
                        }
                    }
                    usLink = m_pEdges[link].next;
                    link = usLink;
                    if (m_ulNumWayPtLinks <= link) {
                        srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1c22,
                                     "Link index out of range (3)");
                    }
                } while (usLink != 0);
            }
            if (settled) {
                W8PathHeapHandle* handle = path_heap;
                W8PathHeap* heap = handle->heap;
                if (heap->size == 0) {
                    handle->root_node = 0;
                } else {
                    handle->root_node = heap->Delete().node;
                }
                m_pSurfaces[current].flags &= 0xffef;
                m_rendered_waypoints->Set(current);
            }
            usWayPt = path_heap->root_node;
            current = usWayPt;
            if (m_ulNumWayPoints <= current) {
                srAssertFail("usWayPt < m_ulNumWayPoints", OCTPATH_CPP, 0x1c2a,
                             "Waypoint index out of range (3)");
            }
            if (usWayPt == 0) {
                return 0;
            }
        } while (usWayPt != usEndNode);
    }
    m_pSurfaces[start].parent = 0;
    return usWayPt;
}

/* Build the attachment's stored route for the path found between its start
   and end positions. The parent chain left by FindPath is reversed through
   the shared scratch array into position7/path_values, the end position
   is appended, and the final hop is trimmed back to the destination when the
   last waypoint already sees it. */
// FUNCTION: WIZ8 0x00460950
unsigned char W8PathingService::BuildAttachmentPath(W8NavigatorAttachment* attachment,
                                                    unsigned int flags)
{
    if ((((m_rendered_waypoints != 0) && (m_visible_waypoints != 0)) && (path_heap != 0)) &&
        (m_pSurfaces != 0)) {
        m_rendered_waypoints->ClearAll();
        m_visible_waypoints->ClearAll();
        path_heap->heap->size = 0;
        unsigned short node = FindPath(attachment, flags);
        if (node != 0) {
            unsigned short count = 0;
            m_visible_waypoints->ClearAll();
            do {
                if (m_visible_waypoints->Set(node) != 0) {
                    break;
                }
                if (m_ulNumWayPoints <= count) {
                    srAssertFail("i < m_ulNumWayPoints", OCTPATH_CPP, 0x1ba2, 0);
                }
                g_path_scratch[count] = node;
                node = m_pSurfaces[node].parent;
                ++count;
            } while (node != 0);
            unsigned int remaining = count;
            g_path_scratch[remaining] = 0;
            if (count != 0) {
                do {
                    unsigned short surface_index = g_path_scratch[remaining - 1];
                    srVector3T<float>* position = &m_pSurfaces[surface_index].position;
                    attachment->AppendPathPosition(position, surface_index);
                    --remaining;
                } while (remaining != 0);
            }
            srVector3T<float>* destination = &attachment->position1;
            srVector3T<float>* slot = attachment->position7 + attachment->path_position_index;
            *slot = *destination;
            if (((attachment->path_cursor < attachment->path_position_index) ||
                 ((attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0)) &&
                (TestWaypointSpan(attachment->position7 +
                                      (attachment->path_position_index - 2),
                                  destination, 0, 0) != 0)) {
                --attachment->path_position_index;
                slot = attachment->position7 + attachment->path_position_index;
                *slot = *destination;
            }
            attachment->flags |= 0x20000;
            return 1;
        }
    }
    return 0;
}

/* Link the attachment's recorded route to `target`: seeds the search state
   from the attachment's start waypoint, runs the recursive link search, and
   when that fails falls back to the A* FindPath when the straight-line cost
   still exceeds `separation`. The reached chain is walked back through
   parent into the scratch array, reversed into the attachment's position
   and value arrays, trimmed at the `separation` sphere around the target, and
   the last stored position becomes position1. */
// FUNCTION: WIZ8 0x004612a0
unsigned char W8PathingService::LinkAttachmentTarget(W8NavigatorAttachment* attachment,
                                                     unsigned int flags,
                                                     const srVector3T<float>* target,
                                                     float separation)
{
    unsigned short node;
    unsigned short count;
    unsigned short current;
    unsigned int index;
    unsigned short start;

    m_patrol_min = separation;
    m_patrol_distance = separation + g_float_005ec0a8;
    m_patrol_start = *target;
    path_flags0 = flags;
    start_waypoint = 0;
    start = FindWaypoint(&attachment->position0, 1);
    if ((start == 0) && ((start = start_waypoint) == 0)) {
        return 0;
    }
    m_rendered_waypoints->ClearAll();
    m_visible_waypoints->ClearAll();
    path_heap->heap->size = 0;
    index = start;
    m_visible_waypoints->Set(index);
    m_probe_cell_key = index;
    m_patrol_cost = (*target - attachment->position0).Length();
    m_pSurfaces[index].parent = 0;
    m_pSurfaces[index].cost = m_patrol_cost;
    node = RecurseTargetLinks(start);
    if (node == 0) {
        if (m_patrol_cost > separation) {
            attachment->position1 = m_pSurfaces[m_probe_cell_key].position;
            node = FindPath(attachment, flags);
        }
        if (node == 0) {
            return 0;
        }
    }
    attachment->position1 = m_pSurfaces[node].position;
    count = 0;
    m_visible_waypoints->ClearAll();
    current = node;
    while (current != 0) {
        if (m_visible_waypoints->Set(current) != 0) {
            break;
        }
        if (m_ulNumWayPoints <= count) {
            srAssertFail("i < m_ulNumWayPoints", OCTPATH_CPP, 0x1c97, 0);
        }
        g_path_scratch[count] = current;
        current = m_pSurfaces[current].parent;
        ++count;
    }
    g_path_scratch[count] = 0;
    if (count != 0) {
        do {
            unsigned short surface_index = g_path_scratch[count - 1];
            srVector3T<float>* position = &m_pSurfaces[surface_index].position;
            attachment->AppendPathPosition(position, surface_index);
            --count;
        } while (count != 0);
    }
    attachment->TruncatePathAtRadius(target, separation);
    if (1 < attachment->path_position_index) {
        --attachment->path_position_index;
        attachment->position1 = attachment->position7[attachment->path_position_index];
    }
    if (start_waypoint != 0) {
        attachment->start_waypoint = m_probe_position;
        attachment->flags |= W8_NAV_ATTACHMENT_START_WAYPOINT;
    }
    attachment->flags |= 0x20000;
    return 1;
}

/* Recursive depth-first link search from `waypoint`. Collects the waypoint's
   admissible links (the same edge/surface flag filters as FindPath), extends
   each destination's accumulated link cost, keys the candidates by their
   distance to the stored target, sorts them, and either returns the first
   candidate priced beyond m_patrol_distance or recurses into each in sorted
   order. The farthest-measured candidate is parked in m_probe_cell_key for
   the fallback path in LinkAttachmentTarget. */
// FUNCTION: WIZ8 0x004615d0
unsigned short W8PathingService::RecurseTargetLinks(unsigned short waypoint)
{
    unsigned short edge;
    unsigned short destination;
    unsigned short candidates[20];
    unsigned long keys[20];
    float distances[20];
    unsigned int count = 0;
    unsigned int index;
    unsigned int link_flags;
    float distance;
    W8PathEdge* link;

    edge = m_pSurfaces[waypoint].first_edge;
    if (edge != 0) {
        unsigned short* slot = candidates;
        do {
            link = m_pEdges + edge;
            link_flags = link->flags;
            if ((link_flags & 0x1000000) == 0) {
                destination = link->destination;
                if ((((m_pSurfaces[destination].flags & 0x20) == 0) &&
                     (((link_flags & 0x80000000) == 0) ||
                      (((link_flags & 0x10000000) != 0 &&
                        ((path_flags0 & 0x10000000) != 0))))) &&
                    ((path_flags0 == 0) ||
                     ((((link_flags & 0xffff) == 0xffff ||
                        ((link_flags & path_flags0 & 0xffff) != 0)) &&
                       (((link_flags & 0x70000) == 0x70000 ||
                         ((link_flags & path_flags0 & 0x70000) != 0)))) &&
                      (((link_flags & 0x380000) == 0x380000 ||
                        ((link_flags & path_flags0 & 0x380000) != 0))))) &&
                    (destination != 0)) {
                    if (m_rendered_waypoints->Test(destination) == 0) {
                        m_pSurfaces[destination].cost =
                            link->distance + m_pSurfaces[waypoint].cost;
                        m_pSurfaces[destination].parent = waypoint;
                        srVector3T<float> to_target =
                            m_pSurfaces[destination].position - m_patrol_start;
                        distance = to_target.Length();
                        distances[count] = distance;
                        if (m_patrol_cost < distance) {
                            m_patrol_cost = distance;
                            m_probe_cell_key = destination;
                        }
                        srVector3T<float> link_direction =
                            m_pSurfaces[destination].position -
                            m_pSurfaces[waypoint].position;
                        link_direction.Normalize();
                        *slot = edge;
                        keys[count] = static_cast<unsigned long>(
                            g_float_005ec398 -
                            DotProduct(link_direction, to_target / distance) * g_float_005ebc64);
                        ++count;
                        ++slot;
                    }
                }
            }
            edge = link->next;
        } while (edge != 0);
    }
    QuickSortByKey(candidates, keys, 0, static_cast<int>(count) - 1);
    m_rendered_waypoints->Set(waypoint);
    if (count != 0) {
        for (index = 0; index < count; ++index) {
            destination = m_pEdges[candidates[index]].destination;
            if (m_patrol_distance < distances[index]) {
                return destination;
            }
            destination = RecurseTargetLinks(destination);
            if (destination != 0) {
                return destination;
            }
        }
    }
    return 0;
}

/* Build a randomized patrol route into the attachment. When the destination
   sits within `maximum` of the attachment's start position, RecursePatrolLinks
   walks the waypoint graph for a node whose accumulated link cost clears a
   randomized target distance; farther out it falls back to the straight
   attachment path. The fallback nodes tracked in m_patrol_node and
   m_probe_cell_key supply the endpoint when no candidate qualifies.
   `velocity` is passed by callers but never read. */
// FUNCTION: WIZ8 0x00461960
unsigned char W8PathingService::BuildPatrolPath(W8NavigatorAttachment* attachment,
                                                unsigned int flags,
                                                const srVector3T<float>* destination, float minimum,
                                                const srVector3T<float>* velocity, float maximum)
{
    if (minimum >= maximum) {
        return 0;
    }
    m_patrol_max = maximum;
    m_patrol_min = minimum;
    m_patrol_cost = 0.0f;
    m_patrol_start = attachment->position0;
    m_patrol_destination = *destination;
    path_flags0 = flags;
    attachment->flags |= 0x200000;
    srVector3T<float> delta = m_patrol_start - m_patrol_destination;
    if (maximum < delta.Length()) {
        return BuildAttachmentPath(attachment, flags);
    }
    float roll = Random(900) + g_octree_cell_scale;
    start_waypoint = 0;
    m_patrol_distance = roll * maximum * g_float_005ec128 + maximum;
    unsigned short usStartNode = FindWaypoint(&attachment->position0, 1);
    if ((usStartNode == 0) && ((usStartNode = start_waypoint) == 0)) {
        return 0;
    }
    m_rendered_waypoints->ClearAll();
    m_visible_waypoints->ClearAll();
    path_heap->heap->size = 0;
    m_probe_cell_key = 0;
    m_patrol_node = 0;
    unsigned int start = usStartNode;
    m_visible_waypoints->Set(start);
    W8PathSurface* surfaces = m_pSurfaces;
    m_probe_limit = surfaces[start].visit_stamp;
    m_pSurfaces[start].cost = (surfaces[start].position - attachment->position0).Length();
    m_pSurfaces[start].parent = 0;
    unsigned int node = RecursePatrolLinks(usStartNode);
    if (static_cast<short>(node) == 0) {
        if (m_patrol_node == 0) {
            node = static_cast<unsigned short>(m_probe_cell_key);
        } else {
            node = m_patrol_node & 0xffff;
        }
        if (static_cast<short>(node) == 0) {
            return 0;
        }
    }
    attachment->position1 = m_pSurfaces[node & 0xffff].position;
    unsigned short count = 0;
    unsigned int current = node;
    unsigned short previous = 0;
    do {
        if (static_cast<unsigned short>(current) == 0) {
            break;
        }
        if (m_ulNumWayPoints <= count) {
            srAssertFail("i < m_ulNumWayPoints", OCTPATH_CPP, 0x1d87, 0);
        }
        previous = static_cast<unsigned short>(current);
        g_path_scratch[count] = previous;
        current = m_pSurfaces[current & 0xffff].parent;
        ++count;
    } while (previous != static_cast<unsigned short>(current));
    unsigned int remaining = count;
    g_path_scratch[remaining] = 0;
    if (count != 0) {
        do {
            unsigned short surface_index = g_path_scratch[remaining - 1];
            srVector3T<float>* position = &m_pSurfaces[surface_index].position;
            attachment->AppendPathPosition(position, surface_index);
            --remaining;
        } while (remaining != 0);
    }
    if (1 < attachment->path_position_index) {
        --attachment->path_position_index;
        attachment->position1 = attachment->position7[attachment->path_position_index];
    }
    if (start_waypoint != 0) {
        attachment->start_waypoint = m_probe_position;
        attachment->flags |= W8_NAV_ATTACHMENT_START_WAYPOINT;
    }
    attachment->flags |= 0x20000;
    return 1;
}

/* Recursive depth-first patrol search from `waypoint`. Each pass collects the
   waypoint's admissible links (edge/surface flag filters matching FindPath),
   prices them by accumulated link cost, sorts them by the surface key, then
   accepts the first candidate whose cost clears the randomized
   patrol_distance - or the 250000 sanity bound - while the last-measured
   candidate distance still exceeds patrol_min. Otherwise it recurses into
   each candidate in sorted order. The argmin-key candidate is parked in
   m_patrol_node and the best-cost alternate in m_probe_cell_key for the
   caller's fallback. The visited set is m_rendered_waypoints. */
// FUNCTION: WIZ8 0x00461d10
unsigned short W8PathingService::RecursePatrolLinks(unsigned short waypoint)
{
    unsigned short links[20];
    unsigned long keys[20];
    float costs[20];
    unsigned int count = 0;
    float distance;

    unsigned short link = m_pSurfaces[waypoint].first_edge;
    if (link != 0) {
        do {
            if (m_ulNumWayPtLinks <= static_cast<unsigned int>(link)) {
                srAssertFail("usLink < m_ulNumWayPtLinks", OCTPATH_CPP, 0x1dcf,
                             "Link index out of range");
            }
            unsigned int edge_flags = m_pEdges[link].flags;
            unsigned short next = m_pEdges[link].destination;
            if ((((edge_flags & 0x1000000) == 0) && (next != waypoint)) &&
                ((m_pSurfaces[next].flags & 0x20) == 0) &&
                (((edge_flags & 0x80000000) == 0) ||
                 (((edge_flags & 0x10000000) != 0) && ((path_flags0 & 0x10000000) != 0))) &&
                ((path_flags0 == 0) || ((((edge_flags & 0xffff) == 0xffff) ||
                                            ((edge_flags & path_flags0 & 0xffff) != 0)) &&
                                           (((edge_flags & 0x70000) == 0x70000) ||
                                            ((edge_flags & path_flags0 & 0x70000) != 0)) &&
                                           (((edge_flags & 0x380000) == 0x380000) ||
                                            ((edge_flags & path_flags0 & 0x380000) != 0)))) &&
                (next != 0)) {
                if (m_rendered_waypoints->Test(next) == 0) {
                    if (m_ulNumWayPoints <= static_cast<unsigned int>(next)) {
                        srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1dd9,
                                     "Waypoint index out of range");
                    }
                    W8PathSurface* next_surface = &m_pSurfaces[next];
                    distance = (next_surface->position - m_patrol_destination).Length();
                    if (distance < m_patrol_max) {
                        next_surface->cost =
                            m_pSurfaces[waypoint].cost + m_pEdges[link].distance;
                        costs[count] = next_surface->cost;
                        next_surface->parent = waypoint;
                        links[count] = link;
                        unsigned int key = next_surface->visit_stamp;
                        keys[count] = key;
                        ++count;
                        if (key < m_probe_limit) {
                            m_probe_limit = key;
                            m_patrol_node = next;
                        } else {
                            if ((key != m_probe_limit) ||
                                (next_surface->cost <= m_patrol_cost)) {
                                if ((m_patrol_node == 0) &&
                                    (m_patrol_cost < next_surface->cost)) {
                                    m_patrol_cost = next_surface->cost;
                                    m_probe_cell_key = next;
                                }
                            } else {
                                m_patrol_cost = next_surface->cost;
                                m_patrol_node = next;
                                m_probe_cell_key = next;
                            }
                        }
                        if (0x13 < count) {
                            srAssertFail("ulLinkNum < 20", OCTPATH_CPP, 0x1e00,
                                         "Too many links for waypoint in RecursePatrolLinks");
                        }
                    }
                }
            }
            link = m_pEdges[link].next;
        } while (link != 0);
        if (1 < count) {
            QuickSortByKey(links, keys, 0, static_cast<int>(count) - 1);
        }
    }
    m_rendered_waypoints->Set(waypoint);
    if (count != 0) {
        unsigned int index = 0;
        do {
            unsigned short next = m_pEdges[links[index]].destination;
            if (m_ulNumWayPoints <= static_cast<unsigned int>(next)) {
                srAssertFail("usNextNode < m_ulNumWayPoints", OCTPATH_CPP, 0x1e0f,
                             "Waypoint index out of range (2)");
            }
            if (((m_patrol_distance < costs[index]) || (g_float_005ec39c < costs[index])) &&
                (m_patrol_min < distance)) {
                return next;
            }
            unsigned short found = RecursePatrolLinks(next);
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
void W8PathingService::ReduceWaypointCosts(unsigned int waypoint, float amount)
{
    W8PathSurface* surface = &m_pSurfaces[waypoint];
    surface->cost -= amount;
    if (surface->cost >= g_float_zero) {
        surface->remaining_cost -= amount;

        unsigned short edge_index = surface->first_edge;
        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges[edge_index];
            unsigned short child = edge->destination;
            if (child != 0 && m_visible_waypoints->Test(child) != 0 &&
                m_pSurfaces[child].parent == waypoint) {
                ReduceWaypointCosts(child, amount);
            }
            edge_index = edge->next;
        }
    }
}

/* Move an integer path cell one compass step. Directions immediately outside
   the eight-value range wrap once; values still outside it leave the cell
   untouched. The jump-table order is north through north-west. */
// FUNCTION: WIZ8 0x004622d0
void __stdcall StepPathCell(int* x, int* z, int direction)
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
   W8_NAV_ATTACHMENT_START_WAYPOINT makes the first probe repeat the current path
   index. */
// FUNCTION: WIZ8 0x00462de0
unsigned char W8PathingService::AdvanceAttachmentWaypoint(const srVector3T<float>* source,
                                                          W8NavigatorAttachment* attachment)
{
    unsigned short cursor = attachment->path_cursor;
    if ((attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
        --cursor;
    }
    attachment->position_cursor = cursor;
    ++attachment->position_cursor;

    if (attachment->position_cursor < attachment->path_position_index) {
        srVector3T<float> destination = attachment->position7[attachment->position_cursor];
        if (TestWaypointSpan(source, &destination, 0, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Match a tag against the path probes collected for the current search. A
   tag-only query succeeds immediately. A spatial query must lie strictly
   farther from the probe center than the search origin was, while remaining
   strictly inside its collision radius plus the caller's own radius. */
// FUNCTION: WIZ8 0x00465970
unsigned char W8PathingService::MatchesPathProbe(unsigned int tag, const float* radius,
                                                 const srVector3T<float>* position)
{
    for (unsigned int index = 0; index < m_path_probe_count; ++index) {
        W8PathProbeVolume* probe = &m_path_probes[index];
        if (probe->tag == tag) {
            if (radius == 0) {
                return 1;
            }

            srVector3T<float> delta = probe->center - *position;
            float distance = delta.Length();
            if (distance < probe->outer_radius + *radius && probe->inner_radius < distance) {
                return 1;
            }
        }
    }
    return 0;
}

/* Reserve the next fixed-width planner node. Storage grows by fifty records,
   is cleared in full, and retains every node through the newly issued index. */
// FUNCTION: WIZ8 0x00465A00
unsigned short W8PathingService::AllocateSearchNode()
{
    unsigned int node_index = ++m_search_node_count;

    if (m_search_nodes != 0 && node_index < m_search_node_capacity) {
        return static_cast<unsigned short>(node_index);
    }

    m_search_node_capacity += 50;
    W8PathSearchNode* new_nodes = new W8PathSearchNode[m_search_node_capacity];
    if (new_nodes == 0) {
        srAssertFail("pNewSearchNodes", OCTPATH_CPP, 0x2751, 0);
    }
    memset(new_nodes, 0, m_search_node_capacity * sizeof(W8PathSearchNode));
    if (m_search_nodes != 0) {
        memcpy(new_nodes, m_search_nodes, m_search_node_count * sizeof(W8PathSearchNode));
        delete[] m_search_nodes;
    }
    m_search_nodes = new_nodes;
    return static_cast<unsigned short>(node_index);
}

/* Walk grid-sized steps from a position toward a search node. Every crossed
   cell must resolve through the visited-node index to a live, unblocked node
   whose recorded clearance exceeds the caller's limit. */
// FUNCTION: WIZ8 0x00465AF0
unsigned char W8PathingService::CanReachSearchNode(const srVector3T<float>* position,
                                                   unsigned short target_node, float clearance)
{
    W8PathSearchNode* target = &m_search_nodes[target_node];
    srVector3T<float> delta = target->position - *position;
    float scale;
    if (static_cast<float>(fabs(delta.x)) > static_cast<float>(fabs(delta.z))) {
        scale = static_cast<float>(fabs(grid_scale / delta.x));
    } else {
        scale = static_cast<float>(fabs(grid_scale / delta.z));
    }

    srVector3T<float> probe = *position;
    float step_x = delta.x * scale;
    float step_z = delta.z * scale;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
    bool blocked = false;

    while (1) {
        unsigned int key = PositionToPathKey(this, &probe, 0);
        unsigned int node_index = visited->Lookup(&key);

        if (static_cast<unsigned short>(node_index) == target_node || blocked != 0) {
            return blocked == 0;
        }
        if (static_cast<unsigned short>(node_index) == 0 ||
            (m_search_nodes[node_index & 0xffff].flags & 0x100) != 0 ||
            m_search_nodes[node_index & 0xffff].clearance <= clearance) {
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                                   \
    "-Wsometimes-uninitialized" // uninit-ok: retail uses an unset target radius/position when monster info or its model is absent; this can alter the published endpoint.
#pragma clang diagnostic ignored                                                                   \
    "-Wuninitialized" // uninit-ok: retail uses an unset target radius/position when monster info or its model is absent; this can alter the published endpoint.
// FUNCTION: WIZ8 0x00465D70
void W8PathingService::AdjustFinalPathEndpoint(W8NavigatorMovementState* movement, float radius,
                                               float separation)
{
    int target_location = movement->target_location_id;
    if (target_location < 0 || explicit_target != 0) {
        return;
    }

    /* Retail leaves the target radius and position unset when the monster
       info or model is absent. */
    float target_radius;
    srVector3T<float> target_position;
    if (target_location <= 0) {
        target_radius = g_startup_world->movement.alternate_radius;
        target_position = g_startup_world->GetPosition();
    } else {
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x27b0, OCTPATH_CPP, target_location, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info != 0 && info->p3D != 0) {
            target_radius = info->p3D->movement.alternate_radius;
            target_position = info->p3D->GetPosition();
        }
    }

    W8NavigatorAttachment* attachment = movement->attachment;
    srVector3T<float>* endpoint = &attachment->position7[attachment->path_position_index];
    srVector3T<float> direction = *endpoint - target_position;
    float excess = direction.Length() - (target_radius + radius);
    if (separation < excess && excess - separation < grid_scale * g_float_005ebc7c) {
        direction.SetLength((target_radius + radius + separation) * g_path_endpoint_scale);

        srVector3T<float> adjusted;
        adjusted = target_position + direction;
        *endpoint = adjusted;
        attachment->position1 = *endpoint;

        if ((attachment->flags & 0x08000000) == 0) {
            attachment->flags |= W8_NAV_ATTACHMENT_POSITION_RECORDED;
            attachment->position6 = adjusted;
            g_octree->QueueOctreeKind13(movement->location_id, &adjusted);
        }
    }
}
#pragma clang diagnostic pop

/* Select one conditional frame for a GD prop's path cells. Entries belonging
   to every other frame first lose both prop-state bits. Entries belonging to
   the selected frame then gain the caller's state bits. The hash table may
   contain several values for one key, so the low word from the serialized
   conditional value is the identity used to find the exact pairing. */
// FUNCTION: WIZ8 0x00465fb0
void W8PathingService::UpdateConditionalPathFlags(unsigned int path_handle, unsigned short frame,
                                                  unsigned int flags)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    unsigned int lookup_index = path_handle;

    while (m_pulCondLookup[lookup_index] != 0) {
        if (m_pusCondNodeFrames[lookup_index] != frame) {
            unsigned int key_index = m_pulCondLookup[lookup_index];
            while (m_pulCondNodeKeys[key_index] != 0) {
                unsigned int key = m_pulCondNodeKeys[key_index];
                unsigned int wanted_value = m_pulCondNodeValues[key_index];
                unsigned int current_value = FindConditionalPathValue(key, wanted_value);

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
                unsigned int current_value = FindConditionalPathValue(key, wanted_value);

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
int W8PathingService::ProcessSearchNodeProps(unsigned short node_index, bool first_only)
{
    W8PathSearchNode* node = &m_search_nodes[node_index];
    float half_cell = grid_scale * g_float_005ebc7c;

    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> lower_extent;
    srVector3T<float> upper_extent;
    float vertical_extent = grid_scale + grid_scale;
    lower_extent.Set(half_cell, 0.0f, half_cell);
    upper_extent.Set(half_cell, vertical_extent, half_cell);
    lower = node->position - lower_extent;
    upper = node->position + upper_extent;

    unsigned long* candidates = 0;
    unsigned int count =
        g_octree->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
    for (unsigned int index = 0; index < count; ++index) {
        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
        prop->CanBeUsedFrom(node->cell_x, node->cell_z, 1);
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
unsigned int W8PathingService::CollectPathProbes(W8NavigatorMovementState* movement, float radius)
{
    m_path_probe_count = 0;

    float extent = g_runtime_world_scale + radius;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> half_extent;
    half_extent.Set(extent, extent, extent);
    lower = movement->position - half_extent;
    upper = movement->position + half_extent;

    m_path_candidates = 0;
    m_path_candidate_count = g_octree->QueryLocationsInBox(&m_path_candidates, &lower, &upper,
                                                             movement->location_id);

    srVector3T<float> player_position = g_startup_world->GetPosition();
    srVector3T<float> delta = player_position - movement->position;
    float distance = delta.Length();
    float player_radius = g_startup_world->movement.alternate_radius;
    float overlap_radius = radius;
    if (player_radius < radius) {
        overlap_radius = player_radius;
    }
    if (distance < (radius - overlap_radius * g_float_005ebc7c) + player_radius) {
        W8PathProbeVolume* probe = &m_path_probes[m_path_probe_count];
        probe->tag = 0;
        probe->outer_radius = player_radius;
        probe->center = g_startup_world->GetPosition();
        ++m_path_probe_count;
    }

    for (unsigned int index = 0; index < m_path_candidate_count && m_path_probe_count < 5;
         ++index) {
        int location_id = m_path_candidates[index];
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x26ae, OCTPATH_CPP, location_id, 0);
        if (monster_index != static_cast<unsigned int>(-1)) {
            monster_index = MonsterGetIndexByLocationID(0x26b1, OCTPATH_CPP, location_id, 1);
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (info != 0 && info->p3D != 0 && info->p3D->active != 0) {
                W8Monster* monster = info->p3D;
                srVector3T<float> monster_position = monster->GetPosition();
                srVector3T<float> monster_delta = monster_position - movement->position;
                distance = monster_delta.Length();
                float monster_radius = monster->movement.alternate_radius;
                overlap_radius = radius;
                if (monster_radius < radius) {
                    overlap_radius = monster_radius;
                }
                if (distance < (radius - overlap_radius * g_float_005ebc7c) + monster_radius) {
                    W8PathProbeVolume* probe = &m_path_probes[m_path_probe_count];
                    probe->tag = location_id;
                    probe->outer_radius = monster_radius;
                    probe->inner_radius = distance;
                    probe->center = monster->GetPosition();
                    ++m_path_probe_count;
                }
            }
        }
    }
    return m_path_probe_count;
}

/* Build a bounded grid route from the navigator's current position to its
   active attachment target. The open-chain index owns one search node per
   cell, the fixed-capacity minimum heap chooses the next node to expand, and
   the selected parent chain is collapsed into the attachment's route array. */
srVector3T<float> W8PathingService::GetSearchTraceOffset(float bearing)
{
    float target_yaw = NormalizeAngle(m_trace_target_yaw);
    srMatrix3T<float> rotation;
    rotation.SetIdentity();
    float angle = bearing - target_yaw;
    if (angle != g_double_zero) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }
    return rotation.Transform(trace_offset);
}

// FUNCTION: WIZ8 0x00463460
unsigned short W8PathingService::PlanMovement(W8NavigatorMovementState* movement, float radius,
                                              float separation)
{
    float diagonal_step = grid_scale * g_path_cardinal_scale;
    unsigned short result = 0;
    bool stop_search = 0;

    if (m_trace_configured == 0) {
        trace_offset.Set(0.0f, 500.0f, 0.0f);
        m_trace_mode = 0;
        trace_height_offset = 500.0f;
    }

    m_search_node_count = 0;
    planner_location = movement->location_id;
    memset(m_search_nodes, 0, m_search_node_capacity * sizeof(W8PathSearchNode));
    m_probe_cell_key = 0;
    m_probe_limit = static_cast<unsigned int>(-1);
    path_heap->heap->size = 0;

    W8NavigatorAttachment* attachment = movement->attachment;
    attachment->flags &= ~W8_NAV_ATTACHMENT_RESULT_MASK;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
    visited->Clear();
    attachment->flags &= ~W8_NAV_ATTACHMENT_POSITION_RECORDED;

    unsigned char allow_dynamic = static_cast<unsigned char>(movement->movement_flags >> 28 & 1);
    srVector3T<float> start = movement->position;
    srVector3T<float> target;
    if (explicit_target != 0) {
        target = movement->target_position;
    } else if ((attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
        target = attachment->start_waypoint;
    } else if (attachment->path_cursor < attachment->path_position_index) {
        target = attachment->position7[attachment->path_cursor];
    } else {
        target = attachment->position1;
    }

    srVector3T<float> target_delta = target - start;
    float target_distance = target_delta.Length();
    float remaining_callback = movement->callback_threshold - movement->callback_progress;

    if ((attachment->flags & 0x04000000) == 0) {
        float search_extent = target_distance;
        if (search_extent < remaining_callback) {
            search_extent = remaining_callback;
        }
        search_extent += g_runtime_world_scale + radius;
        srVector3T<float> lower;
        srVector3T<float> upper;
        srVector3T<float> half_extent;
        half_extent.Set(search_extent, search_extent, search_extent);
        lower = start - half_extent;
        upper = start + half_extent;
        CollectPathProbes(movement, radius);
        m_path_candidates = 0;
        m_path_candidate_count = g_octree->QueryLocationsInBox(&m_path_candidates, &lower,
                                                                 &upper, movement->location_id);
    } else {
        m_path_probe_count = 0;
        if (movement->target_location_id < 1) {
            m_path_candidate_count = 0;
        } else {
            m_path_candidate_count = 1;
            m_path_candidates = movement->TargetLocationAsCandidate();
        }
    }

    srVector2i root_cell;
    unsigned int root_key = PositionToPathKey(this, &start, &root_cell);
    unsigned short root_index = AllocateSearchNode();
    W8PathSearchNode* root = &m_search_nodes[root_index];
    root->flags = 0;
    root->node_index = root_index;
    root->cell_x = static_cast<unsigned short>(root_cell.x);
    root->cell_z = static_cast<unsigned short>(root_cell.y);
    root->path_height = static_cast<unsigned short>(
        static_cast<int>((start.y - level_bounds.minimum.y) / span) + 1);
    root->parent_node = 0;
    root->base_score = 0.0f;
    root->path_cost = 0.0f;
    root->distance = target_distance;
    root->position = start;

    int root_value = root_index;
    visited->Insert(&root_key, &root_value);

    unsigned int root_height = root->path_height;
    float root_clearance = radius;
    float root_vertical = 0.0f;
    unsigned char root_dynamic = 0;
    ResolvePathCell(root_key, 1, &root_height, &root_clearance, &root_vertical, &root_dynamic);
    UpdateSearchNodeScore(root_index, &target, root_clearance, radius);
    unsigned int root_score = static_cast<unsigned int>(root->score);
    if (root_score < m_probe_limit) {
        m_probe_limit = root_score;
        m_probe_cell_key = root_index;
    }
    m_probe_cell_key = 0;

    W8PathHeap* heap = path_heap->heap;
    W8PathHeapEntry entry;
    entry.node = root_index;
    entry.priority = static_cast<unsigned int>(root->score);
    heap->Insert(&entry);
    path_heap->root_node = heap->entries[0].node;

    unsigned int best_node = path_heap->root_node;
    while (best_node != 0 && stop_search == 0 && m_search_node_count < g_path_reserve) {
        W8PathSearchNode* current = &m_search_nodes[best_node];
        unsigned short current_x = current->cell_x;
        unsigned short current_z = current->cell_z;
        unsigned int current_height = current->path_height;

        for (int direction = 0; direction < 8; ++direction) {
            current = &m_search_nodes[best_node];
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
            float step = (direction & 1) == 0 ? grid_scale : diagonal_step;
            float path_cost = current->path_cost + step;
            float base_score = current->base_score + step;

            unsigned int existing_index = visited->Lookup(&key);

            if (existing_index != 0) {
                W8PathSearchNode* existing = &m_search_nodes[existing_index & 0xffff];
                if ((existing->flags & 0x0100) != 0) {
                    continue;
                }
                int height_delta = static_cast<int>(current->path_height) -
                                   static_cast<int>(existing->path_height);
                if (height_delta < 0) {
                    height_delta = -height_delta;
                }
                base_score += height_delta * span * g_float_005ec3b8;
                if (existing->base_score <= base_score) {
                    continue;
                }
                existing->base_score = base_score;
                existing->path_cost = path_cost;
                existing->parent_node = static_cast<unsigned short>(best_node);
                UpdateSearchNodeScore(existing_index, &target, existing->clearance, radius);
                if ((existing->flags & 0x0400) != 0) {
                    existing->flags &= 0xfbff;
                    entry.node = existing->node_index;
                    entry.priority = static_cast<unsigned int>(existing->score);
                    heap->Insert(&entry);
                    path_heap->root_node = heap->entries[0].node;
                }
                continue;
            }

            unsigned int height = current_height;
            float clearance = radius;
            float vertical = base_score;
            unsigned char dynamic = 0;
            if (ResolvePathCell(key, allow_dynamic, &height, &clearance, &vertical, &dynamic) ==
                0) {
                continue;
            }

            unsigned short node_index = AllocateSearchNode();
            int node_value = node_index;
            visited->Insert(&key, &node_value);

            W8PathSearchNode* node = &m_search_nodes[node_index];
            node->flags = dynamic != 0 ? 0x0800 : 0;
            node->node_index = node_index;
            node->cell_x = static_cast<unsigned short>(neighbor_x);
            node->cell_z = static_cast<unsigned short>(neighbor_z);
            node->path_height = static_cast<unsigned short>(height);
            node->parent_node = static_cast<unsigned short>(best_node);
            node->base_score = vertical;
            node->path_cost = path_cost;
            node->clearance = clearance;
            node->position.Set(
                (node->cell_x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                (node->path_height - 1) * span + level_bounds.minimum.y,
                (node->cell_z + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);

            unsigned short collision =
                ResolveSearchNodeCollisions(movement, node_index, radius, separation);
            UpdateSearchNodeScore(node_index, &target, clearance, radius);
            if (collision == 1) {
                node->flags |= 0x0100;
                continue;
            }
            if (collision == 3 && (m_search_nodes[best_node].flags & 0x0100) == 0) {
                stop_search = 1;
                result = 1;
                m_probe_cell_key = node_index;
                continue;
            }
            if (collision == 2) {
                node->flags |= 0x0100;
            }

            srVector3T<float> node_delta = target - node->position;
            float distance = node_delta.Length();
            if (explicit_target == 0) {
                if (distance < diagonal_step || distance < separation) {
                    stop_search = 1;
                    result = 1;
                }
            } else if (separation < distance) {
                stop_search = 1;
                result = 1;
            }

            if (collision == 0) {
                unsigned int score = static_cast<unsigned int>(node->score);
                if (score < m_probe_limit) {
                    m_probe_limit = score;
                    m_probe_cell_key = node_index;
                }
            }
            entry.node = node->node_index;
            entry.priority = static_cast<unsigned int>(node->score);
            heap->Insert(&entry);
            path_heap->root_node = heap->entries[0].node;
        }

        if (heap->size == 0) {
            path_heap->root_node = 0;
        } else {
            path_heap->root_node = heap->Delete().node;
        }
        m_search_nodes[best_node].flags |= 4;
        best_node = path_heap->root_node;
        if (best_node > m_search_node_count) {
            char message[80];
            sprintf(message, "A*, Invalid node index %d from Queue", best_node);
            srAssertFail("(ulBestNode <= m_ulSearchNodesUsed)", OCTPATH_CPP, 0x22ad, message);
        }
    }

    if ((attachment->flags & 0x00001000) != 0) {
        if (stop_search == 0) {
            result = 3;
        }
        attachment->flags &= ~W8_NAV_ATTACHMENT_RESULT_MASK;
        attachment->flags |= result;
        return result;
    }

    bool direct_path = 0;
    unsigned short direct_visibility_node = 0;
    if (m_probe_cell_key != 0) {
        unsigned short walk = static_cast<unsigned short>(m_probe_cell_key);
        unsigned short walk_parent = m_search_nodes[walk].parent_node;
        while (walk_parent != 0) {
            if (explicit_target == 0 && direct_visibility_node == 0) {
                srVector3T<float> trace_target = movement->target_position;
                trace_target.y += trace_height_offset;
                float bearing =
                    NormalizeAngle(GetHeadingAngle(&m_search_nodes[walk].position, &trace_target));
                srVector3T<float> transformed = GetSearchTraceOffset(bearing);
                srVector3T<float> trace_source;
                trace_source = m_search_nodes[walk].position + transformed;
                short trace =
                    g_octree->TraceLineOfSight(&trace_source, &trace_target, 1, -3, -3, 1, 0);
                if (trace != 0) {
                    m_search_nodes[walk].flags |= 4;
                } else {
                    direct_visibility_node = walk;
                }
            }
            walk = walk_parent;
            walk_parent = m_search_nodes[walk].parent_node;
        }
        if (explicit_target == 0 && direct_visibility_node == 0) {
            srVector3T<float> trace_target = movement->target_position;
            trace_target.y += trace_height_offset;
            float bearing =
                NormalizeAngle(GetHeadingAngle(&m_search_nodes[walk].position, &trace_target));
            srVector3T<float> transformed = GetSearchTraceOffset(bearing);
            srVector3T<float> trace_source;
            trace_source = m_search_nodes[walk].position + transformed;
            short trace = g_octree->TraceLineOfSight(&trace_source, &trace_target, 1, -3, -3, 1, 0);
            if (trace == 0) {
                direct_path = 1;
            } else {
                m_search_nodes[walk].flags |= 4;
            }
        }
    }

    if ((m_probe_cell_key == 0 && result == 0) || direct_path != 0) {
        attachment->flags |= W8_NAV_ATTACHMENT_POSITION_RECORDED;
        attachment->position6 = movement->position;
        attachment->position7[attachment->path_position_index] = movement->position;
        attachment->position1 = attachment->position7[attachment->path_position_index];
        g_octree->QueueOctreeKind13(movement->location_id, &movement->position);
        g_startup_world->radius = g_startup_world->movement.alternate_radius;
        attachment->flags &= ~W8_NAV_ATTACHMENT_RESULT_MASK;
        if (search_visualization != 0 && m_pPathModelInstance != 0) {
            BuildSearchVisualization();
        }
        return 0;
    }

    if (m_search_node_count >= g_path_reserve || best_node == 0) {
        result = 3;
    }

    unsigned short selected = static_cast<unsigned short>(m_probe_cell_key);
    m_search_nodes[selected].flags |= 2;
    unsigned short previous = selected;
    unsigned short parent = m_search_nodes[selected].parent_node;
    while (parent != 0) {
        W8PathSearchNode* parent_node = &m_search_nodes[parent];
        parent_node->flags |= 2;
        unsigned short next_parent = parent_node->parent_node;
        parent_node->parent_node = previous;
        previous = parent;
        parent = next_parent;
    }
    m_search_nodes[selected].parent_node = 0;

    if (search_visualization != 0 && m_pPathModelInstance != 0) {
        BuildSearchVisualization();
    }

    unsigned short route_node = previous;
    attachment->path_position_index = 1;
    unsigned int prop_count = 0;
    unsigned short anchor_node = previous;
    unsigned short route_parent = m_search_nodes[route_node].parent_node;
    while (route_parent != 0) {
        W8PathSearchNode* node = &m_search_nodes[route_parent];
        if ((node->flags & 0x0800) != 0) {
            if ((attachment->flags & 0x08000000) == 0) {
                int prop = ProcessSearchNodeProps(route_parent, 0);
                if (prop != 0) {
                    attachment->path_values[prop_count++] = static_cast<unsigned short>(prop);
                }
            } else {
                ProcessSearchNodeProps(route_parent, 1);
            }
        }

        if (CanReachSearchNode(&m_search_nodes[anchor_node].position, route_parent, radius) == 0) {
            attachment->AppendPathPosition(&m_search_nodes[route_node].position, 0);
            anchor_node = route_node;
        }

        unsigned short next = node->parent_node;
        if (m_trace_configured != 0) {
            if ((node->flags & 4) != 0) {
                node->flags |= 8;
            } else if (TestSearchPositionVisibility(&node->position, movement) == 0) {
                node->flags |= 8;
            } else {
                next = 0;
                result = 1;
                m_probe_cell_key = route_parent;
            }
        }

        route_node = route_parent;
        route_parent = next;
        if (direct_visibility_node != 0 && route_node == direct_visibility_node) {
            m_probe_cell_key = route_node;
            break;
        }
        if (route_node == 0 || m_search_nodes[route_node].path_cost > remaining_callback) {
            continue;
        }
        m_probe_cell_key = route_node;
        break;
    }

    attachment->AppendPathPosition(&m_search_nodes[anchor_node].position, 0);
    attachment->path_values[prop_count] = 0;

    if (attachment->path_position_index > 1) {
        --attachment->path_position_index;
        attachment->position1 = attachment->position7[attachment->path_position_index];
    }
    if ((attachment->flags & 0x08000000) == 0) {
        attachment->flags |= W8_NAV_ATTACHMENT_POSITION_RECORDED;
        attachment->position6 = m_search_nodes[m_probe_cell_key].position;
        g_octree->QueueOctreeKind13(movement->location_id,
                                    &m_search_nodes[m_probe_cell_key].position);
    }
    if (movement->target_location_id >= 0 && explicit_target == 0) {
        AdjustFinalPathEndpoint(movement, radius, separation);
    }
    attachment->flags &= ~W8_NAV_ATTACHMENT_RESULT_MASK;
    attachment->flags |= result;
    g_startup_world->radius = g_startup_world->movement.alternate_radius;
    return result;
}

/* Plan with an explicit target position. The service flag suppresses the core
   planner's ordinary post-search callback for exactly this nested call, while
   the planner's status is passed straight back to the navigator caller. */
// FUNCTION: WIZ8 0x00464ab0
unsigned short W8PathingService::PlanMovementToPosition(W8NavigatorMovementState* movement,
                                                        const srVector3T<float>* target,
                                                        float radius, float separation)
{
    explicit_target = 1;
    movement->target_position = *target;
    unsigned short result = PlanMovement(movement, radius, separation);
    explicit_target = 0;
    return result;
}

/* Refresh one planner node's distance and accumulated score. Explicit-target
   mode scores from the shared ceiling; ordinary mode starts from the node's
   base score and adds a range penalty only when the adjusted gap is positive.
   Flag 0x2000 applies the final fixed penalty in either mode. */
// FUNCTION: WIZ8 0x00464ff0
float W8PathingService::UpdateSearchNodeScore(unsigned short node_index,
                                              const srVector3T<float>* position, float minimum,
                                              float maximum)
{
    W8PathSearchNode* node = &m_search_nodes[node_index];
    float distance = (*position - node->position).Length();
    node->distance = distance;

    if (explicit_target == 0) {
        node->score = distance * g_float_005ec3b8 + node->base_score;
    } else {
        node->score = g_float_005ec3c0 - distance;
    }

    float gap = maximum - minimum;
    if (explicit_target == 0) {
        float adjusted_gap = gap;
        if (distance <= gap) {
            adjusted_gap = (gap - distance) * g_float_005ec390;
        }
        if (adjusted_gap > g_float_zero) {
            node->score += gap * g_float_005ec3bc;
        }
    } else if (gap > g_float_zero) {
        node->score += g_float_005ec3c0;
    }

    if ((node->flags & 0x2000) != 0) {
        node->score += g_float_005ec3c0;
    }
    return node->score;
}

/* Resolve dynamic navigator overlap for one candidate search node.

   The player is tag zero; a target navigator receives the caller's separation
   allowance, while every other live monster uses only the two radii. Probe
   volumes can mark the node as hard-blocked before the live object lookup.
   Shallow overlaps move the node outward and set flag 0x200; deeper or
   directionally conflicting overlaps return the retail collision state. */
// FUNCTION: WIZ8 0x00465130
unsigned short W8PathingService::ResolveSearchNodeCollisions(W8NavigatorMovementState* movement,
                                                             unsigned short node_index,
                                                             float radius, float separation)
{
    unsigned short result = 0;
    if (explicit_target != 0) {
        separation = 0.0f;
    }

    W8PathSearchNode* node = &m_search_nodes[node_index];
    srVector3T<float> blocking_direction;
    srVector3T<float> player_position = g_startup_world->GetPosition();
    srVector3T<float> player_delta = player_position - node->position;
    float distance = player_delta.Length();
    float threshold = g_startup_world->movement.alternate_radius + radius;
    if (movement->target_location_id == 0) {
        threshold += separation;
    }

    if (distance < threshold) {
        if (movement->target_location_id != 0 || explicit_target != 0) {
            return 1;
        }
        blocking_direction.Set(player_delta.x, player_delta.y, player_delta.z);
        blocking_direction.Normalize();
        result = 3;
    }

    for (unsigned int candidate = 0; candidate < m_path_candidate_count; ++candidate) {
        int location_id = m_path_candidates[candidate];
        unsigned int monster_index =
            MonsterGetIndexByLocationID(0x2622, OCTPATH_CPP, location_id, 0);
        if (monster_index == static_cast<unsigned int>(-1)) {
            continue;
        }

        unsigned int probe_index;
        for (probe_index = 0; probe_index < m_path_probe_count; ++probe_index) {
            W8PathProbeVolume* probe = &m_path_probes[probe_index];
            if (probe->tag == static_cast<unsigned int>(location_id)) {
                float probe_distance = (probe->center - node->position).Length();
                if (probe_distance < radius + probe->outer_radius &&
                    probe->inner_radius < probe_distance) {
                    node->flags |= 0x2000;
                    break;
                }
            }
        }
        if (probe_index < m_path_probe_count) {
            continue;
        }

        monster_index = MonsterGetIndexByLocationID(0x262b, OCTPATH_CPP, location_id, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info == 0 || info->p3D == 0 || info->p3D->active == 0) {
            continue;
        }

        W8Monster* monster = info->p3D;
        srVector3T<float> monster_position = monster->GetPosition();
        srVector3T<float> delta = node->position - monster_position;
        float distance = delta.Length();
        threshold = monster->movement.alternate_radius + radius;
        if (location_id == movement->target_location_id && explicit_target == 0) {
            threshold += separation;
        }

        if (distance < threshold) {
            if (location_id == movement->target_location_id && explicit_target == 0) {
                blocking_direction.Set(delta.x, delta.y, delta.z);
                blocking_direction.Normalize();
                result = 3;
                continue;
            }

            bool adjust = 0;
            if ((node->flags & 0x0200) == 0 && threshold <= distance + g_float_005ec020) {
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
                node->position += delta;
                node->flags |= 0x0200;
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
unsigned char W8PathingService::TestSearchPositionVisibility(const srVector3T<float>* position,
                                                             W8NavigatorMovementState* movement)
{
    W8Monster* monster =
        GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id));
    float distance;
    if (m_trace_target_location == -1) {
        distance = monster->GetPointDistanceToPlayer(*position);
    } else {
        W8Monster* target = GetMonsterByLocationID(m_trace_target_location);
        distance = monster->GetPointDistanceToMonster(target, *position);
    }
    if (m_trace_max_distance < distance) {
        return 0;
    }

    srVector3T<float> movement_target = movement->target_position;
    float bearing = NormalizeAngle(GetHeadingAngle(position, &movement_target));
    srVector3T<float> transformed = GetSearchTraceOffset(bearing);

    srVector3T<float> trace_source = *position + transformed;
    srVector3T<float> trace_target = movement->target_position;
    trace_target.y += trace_height_offset;

    unsigned char range_mode = 0;
    if (CalcRangeDistance(W8_RANGE_TOUCH) < distance && m_trace_mode == 1) {
        range_mode = 1;
    }
    short trace =
        g_octree->TraceLineOfSight(&trace_source, &trace_target, true, movement->location_id,
                                   m_trace_target_location, true, range_mode);
    if (trace != 1 && (trace != -1 || TraceModeRejectsNoHit(m_trace_mode) != 0)) {
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
unsigned short W8PathingService::ConfigureMovementSearch(
    W8NavigatorMovementState* movement, int target_location, float radius, float separation,
    float maximum_distance, srVector3T<float> trace_offset, int requested_trace_mode,
    float target_height_offset, float target_yaw, unsigned char* probe_result)
{
    m_trace_max_distance = maximum_distance;
    this->trace_offset = trace_offset;
    m_trace_mode = requested_trace_mode;
    m_trace_configured = 1;
    trace_height_offset = target_height_offset;
    m_trace_target_yaw = target_yaw;
    if (target_location == 0) {
        m_trace_target_location = -1;
    } else {
        m_trace_target_location = target_location;
    }

    g_octree->AdjustPosition(&movement->target_position, 1);

    unsigned short result = 0;
    W8NavigatorAttachment* attachment = movement->attachment;
    if ((attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) != 0) {
        srVector3T<float> delta = movement->target_position - movement->position;
        float horizontal_clearance = delta.xz().Length() - radius;
        float target_radius;
        if (target_location == 0) {
            target_radius = g_startup_world->movement.alternate_radius;
        } else {
            W8Monster* target = GetMonsterByLocationID(target_location);
            target_radius = target->movement.alternate_radius;
        }

        if (horizontal_clearance - target_radius <= separation &&
            TestSearchPositionVisibility(&movement->position, movement) != 0) {
            attachment->InitializeSegment(&movement->position, &movement->position);
            m_trace_configured = 0;
            if (probe_result != 0) {
                *probe_result = 0;
            }
            return 0;
        }

        attachment->InitializeSegment(&movement->position, &movement->target_position);
        attachment->separation = separation;
        if (probe_result != 0) {
            attachment->flags |= 0x04001000;
            result = PlanMovement(movement, radius, separation);
            attachment->flags &= 0xfbffefff;
            *probe_result = result == 1;
            attachment->InitializeSegment(&movement->position, &movement->target_position);
        }
        result = PlanMovement(movement, radius, separation);
    }

    m_trace_configured = 0;
    return result;
}

/* Resolve one packed path-cell entry from the open-chained index. Compatible
   height entries must be unblocked; dynamic entries additionally require the
   caller's permission and their own enabled bit. The selected packed value
   updates height, vertical offset, compass direction, and the dynamic byte. */
// FUNCTION: WIZ8 0x004648d0
unsigned char W8PathingService::ResolvePathCell(unsigned int key, unsigned char allow_dynamic,
                                                unsigned int* height, float* direction,
                                                float* vertical, unsigned char* dynamic)
{
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);
    bool found = false;

    while (slot >= 0 && !found) {
        unsigned int value = index->entries[slot].value;
        int difference = (value & 0xffff) - *height;
        if (-cell_count < difference && difference < cell_count &&
            (value & 0x20000000) == 0 &&
            ((value & 0x10000000) == 0 || (allow_dynamic != 0 && (value & 0x08000000) != 0))) {
            int magnitude = difference;
            if (magnitude < 0) {
                magnitude = -magnitude;
            }
            *vertical += magnitude * span * g_float_005ec3b8;
            *height = value & 0xffff;
            if ((value & 0x01000000) == 0) {
                *direction = static_cast<float>(value >> 16 & 0xff);
            } else {
                *direction = 0.0f;
            }
            *direction = (*direction + g_float_005ebc7c) * g_world_scale;
            *dynamic = (value & 0x10000000) != 0;
            found = true;
        }

        slot = index->FindNextEntry(&key, slot);
    }
    return found;
}

/* Unit X/Z components of the eight path directions, indexed by direction bit:
   0 north, 1 north-west, 2 west, 3 south-west, 4 south, 5 south-east, 6 east,
   7 north-east. */
// GLOBAL: WIZ8 0x00608280
static float s_path_direction_x[8] = {0.0f, -0.7071070075035095f, -1.0f, -0.7071070075035095f,
                                      0.0f, 0.7071070075035095f,  1.0f,  0.7071070075035095f};
// GLOBAL: WIZ8 0x006082a0
static float s_path_direction_z[8] = {
    -1.0f, -0.7071070075035095f, 0.0f, 0.7071070075035095f,
    1.0f,  0.7071070075035095f,  0.0f, -0.7071070075035095f};

/* Sum the blocked-direction unit vectors among the directions `delta` points
   toward; the normalized sum is the slide direction. `mask` uses the
   neighbor-mask encoding: a set bit is a walkable direction and an all-open
   set is zero, which reads as "no obstacle" here. */
// FUNCTION: WIZ8 0x004664f0
unsigned char W8PathingService::ComputeFreeDirection(unsigned int mask,
                                                     const srVector3T<float>* delta,
                                                     srVector3T<float>* direction)
{
    unsigned int wanted;
    unsigned int bit;
    float length_squared;

    if (mask == 0) {
        return 0;
    }
    wanted = 0;
    if (g_double_zero <= delta->x) {
        if (g_double_zero < delta->x) {
            wanted = 0xe;
        }
    } else {
        wanted = 0xe0;
    }
    if (g_double_zero <= delta->z) {
        if (g_double_zero < delta->z) {
            wanted |= 0x83;
        }
    } else {
        wanted |= 0x38;
    }
    if ((~mask & wanted) == 0) {
        return 0;
    }
    direction->x = g_float_zero;
    direction->z = g_float_zero;
    for (bit = 0; bit < 8; ++bit) {
        if ((~mask & wanted & 1 << (bit & 0x1f)) != 0) {
            direction->x += s_path_direction_x[bit];
            direction->z += s_path_direction_z[bit];
        }
    }
    direction->y = 0.0f;
    length_squared = direction->x * direction->x + direction->z * direction->z;
    if (length_squared != g_double_zero) {
        direction->y = 0.0f;
        direction->x = direction->x * (g_double_005ebc30 / sqrt(length_squared));
        direction->z = direction->z * (g_double_005ebc30 / sqrt(length_squared));
    }
    return 1;
}

/* Resolve the path cell under `position` through the path-value hash, pick the
   entry whose height is within two cell spans, and compute the slide
   direction for `delta` from its neighbor mask. */
// FUNCTION: WIZ8 0x00466600
unsigned char W8PathingService::GetNeighborSlideDirection(const srVector3T<float>* position,
                                                          const srVector3T<float>* delta,
                                                          srVector3T<float>* direction)
{
    W8HashTable<unsigned int, unsigned int>* index;
    int range;
    int height;
    srVector2i cell;
    unsigned int key;
    unsigned int value;
    int slot;
    bool in_range;

    range = cell_count * 2;
    in_range = 0;
    height = static_cast<int>((position->y - level_bounds.minimum.y) / span) + 1;
    key = PositionToPathKey(this, position, &cell);
    index = m_pPathValues;
    value = 0;
    slot = index->FindNextEntry(&key, -1);
    while (slot >= 0) {
        if (in_range != 0) {
            unsigned int mask = ComputeWaypointNeighborMask(&cell, value);
            return ComputeFreeDirection(mask, delta, direction);
        }
        value = index->entries[slot].value;
        int difference = height - static_cast<int>(value & 0xffff);
        if (-range < difference && difference < range) {
            in_range = 1;
        } else {
            slot = index->FindNextEntry(&key, slot);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00466990
unsigned char W8PathingService::GetObstacleDirection(const srVector3T<float>* delta,
                                                     srVector3T<float>* direction)
{
    return ComputeFreeDirection(m_waypoint_neighbor_mask, delta, direction);
}

/* One movement step for a navigator walking an attachment path. The 0x10000
   mode follows the recorded route directly; otherwise the steering context
   advances the position and the waypoint cursor, falls through to the
   edge-transition handler, and relinks to a linked navigator's route when
   the end is reached but still out of contact. Returns whether the step
   completed the path. */
// FUNCTION: WIZ8 0x004669b0
unsigned int W8PathingService::StepAlongPath(W8NavigatorMovementState* movement, float radius,
                                             float separation)
{
    g_navigator_position_changed = true;
    W8NavigatorAttachment* attachment = movement->attachment;
    unsigned int flags = attachment->flags;
    bool arrived;
    if ((flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) != 0) {
        arrived = true;
        if ((flags & 0x8000000) == 0) {
            srVector3T<float>* position = &movement->position;
            srVector3T<float> advanced = *position;
            char on_path = attachment->AdvanceAlongPathPositions(
                g_game_time_accumulator->GetFrameDelta() * movement->movement_speed *
                    movement->movement_scale * g_rate * g_world_scale,
                &advanced);
            arrived = on_path == 0;
            if (arrived) {
                attachment->flags &= ~W8_NAV_ATTACHMENT_RESULT_MASK;
            }
            srVector3T<float> delta = advanced - *position;
            if (((delta.x != g_float_zero) || (delta.y != g_float_zero)) ||
                (delta.z != g_float_zero)) {
                srVector3T<float> target = delta + advanced;
                *position = advanced;
                attachment->position7[0] = advanced;
                attachment->position4 = attachment->position7[0];
                attachment->position0 = attachment->position7[0];
                movement->target_position = target;
                g_octree->UpdateMonsterLocation(movement->location_id, &advanced);
            }
        }
        return arrived;
    }
    arrived = false;
    if ((flags & 0x800000) == 0) {
        W8Monster* monster =
            GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id));
        W8Navigator* linked = monster->linked_navigator;
        if (linked == 0) {
            m_linked_attachment = 0;
        } else {
            m_linked_attachment = linked->movement.attachment;
        }
    } else {
        m_linked_attachment = 0;
    }
    if ((attachment->path_cursor == 1) ||
        (attachment->path_values[attachment->path_cursor] == 0)) {
        unsigned int steer_flags = ((attachment->flags >> 0x17) & 0xffffff01);
        m_path_parameters->SteerFromPathStart(movement, steer_flags);
        unsigned int destination_flag = attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT;
        srVector3T<float>* waypoint;
        if (destination_flag == 0) {
            if (attachment->path_cursor < attachment->path_position_index) {
                waypoint = attachment->position7 + attachment->path_cursor;
            } else {
                waypoint = &attachment->position1;
            }
        } else {
            waypoint = &attachment->start_waypoint;
        }
        srVector3T<float> target = *waypoint;
        float distance = (target - movement->position).Length();
        if ((destination_flag != 0) &&
            (distance < static_cast<float>(g_monster_poster_max_distance))) {
            attachment->flags &= ~W8_NAV_ATTACHMENT_START_WAYPOINT;
        }
        unsigned short cursor = attachment->path_cursor;
        if (((cursor == 1) && (1 < attachment->path_position_index)) &&
            (attachment->path_values[2] != 0)) {
            if (attachment->CheckPositionHopHeight(&movement->position) != 0) {
                ActivateMovementTrigger(movement, 1);
                ++attachment->path_cursor;
            }
        } else if ((attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) == 0) {
            if (cursor == attachment->path_position_index) {
                bool in_range;
                if (movement->target_location_id < 0) {
                    float limit = g_startup_near_limit;
                    if ((attachment->flags & 0x100000) != 0) {
                        limit = g_float_005ebc64;
                    }
                    in_range = limit <= distance;
                } else if (attachment->separation + separation <= distance) {
                    in_range = static_cast<float>(g_monster_poster_max_distance) <= distance;
                } else {
                    in_range = false;
                }
                if (in_range) {
                    ActivateMovementTrigger(movement, 0);
                } else {
                    W8NavigatorAttachment* linked_attachment = m_linked_attachment;
                    if ((linked_attachment == 0) ||
                        (sqrt((attachment->position1.x - linked_attachment->position1.x) *
                                  (attachment->position1.x - linked_attachment->position1.x) +
                              (attachment->position1.y - linked_attachment->position1.y) *
                                  (attachment->position1.y - linked_attachment->position1.y) +
                              (attachment->position1.z - linked_attachment->position1.z) *
                                  (attachment->position1.z - linked_attachment->position1.z)) <
                         static_cast<float>(g_double_005ebc30))) {
                        arrived = true;
                    } else {
                        PrepareLinkedNavigator(movement);
                    }
                }
            } else if (distance < static_cast<float>(g_monster_poster_max_distance)) {
                attachment->path_cursor = cursor + 1;
            }
        }
        ActivateMovementTrigger(movement, 0);
        g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
        return arrived;
    }
    unsigned int steer_flags = ((attachment->flags >> 0x17) & 0xffffff01);
    char stepped = m_path_parameters->SteerAlongPath(movement, steer_flags);
    float distance = 0.0f;
    if (stepped == 0) {
        srVector3T<float> target;
        if ((attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) == 0) {
            if (attachment->path_cursor < attachment->path_position_index) {
                srVector3T<float>* waypoint = attachment->position7 + attachment->path_cursor;
                target = *waypoint;
            } else {
                target = attachment->position1;
            }
        } else {
            target = attachment->start_waypoint;
        }
        float dx = target.x - movement->position.x;
        float dy = target.y - movement->position.y;
        target.z -= movement->position.z;
        distance = sqrt(dy * dy + target.z * target.z + dx * dx);
        float limit;
        if (attachment->path_cursor == attachment->path_position_index) {
            if (-1 < movement->target_location_id) {
                if (distance < attachment->separation + separation) {
                    goto transitioned;
                }
                limit = static_cast<float>(g_monster_poster_max_distance);
            } else {
                limit = g_startup_near_limit;
                if ((attachment->flags & 0x100000) != 0) {
                    limit = g_float_005ebc64;
                }
            }
        } else {
            limit = static_cast<float>(g_monster_poster_max_distance);
        }
        if (limit <= distance) {
            g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
            return arrived;
        }
    }
transitioned: {
    unsigned char transition = HandlePathEdgeTransition(movement);
    if (transition == 1) {
        if (attachment->path_cursor != attachment->path_position_index) {
            ActivateMovementTrigger(movement, 1);
            m_pSurfaces[attachment->path_values[attachment->path_cursor]].visit_stamp =
                static_cast<unsigned int>(g_game_time_accumulator->GetElapsed());
            ++attachment->path_cursor;
            g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
            return arrived;
        }
        W8NavigatorAttachment* linked_attachment = m_linked_attachment;
        if ((linked_attachment != 0) &&
            (static_cast<float>(g_double_005ebc30) <=
             sqrt((attachment->position1.x - linked_attachment->position1.x) *
                      (attachment->position1.x - linked_attachment->position1.x) +
                  (attachment->position1.y - linked_attachment->position1.y) *
                      (attachment->position1.y - linked_attachment->position1.y) +
                  (attachment->position1.z - linked_attachment->position1.z) *
                      (attachment->position1.z - linked_attachment->position1.z)))) {
            PrepareLinkedNavigator(movement);
            g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
            return arrived;
        }
    } else if (transition != 0) {
        g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
        return arrived;
    }
}
    arrived = true;
    g_octree->UpdateMonsterLocation(movement->location_id, &movement->position);
    return arrived;
}

/* Rebuild the attachment route against the linked navigator's recorded path.
   The linked attachment is resolved through the moving monster's link (or
   cleared when the 0x800000 flag or a missing link says there is none), the
   route is rebuilt from the navigator's current position to the linked
   path's next live waypoint, and the linked route's remaining positions are
   appended verbatim. Returns the path-build result. */
// FUNCTION: WIZ8 0x00466fb0
unsigned char W8PathingService::PrepareLinkedNavigator(W8NavigatorMovementState* movement)
{
    W8NavigatorAttachment* attachment = movement->attachment;
    if ((attachment->flags & 0x800000) == 0) {
        W8Monster* monster =
            GetMonsterByLocationID(static_cast<unsigned int>(movement->location_id));
        W8Navigator* linked = monster->linked_navigator;
        if (linked == 0) {
            m_linked_attachment = 0;
        } else {
            m_linked_attachment = linked->movement.attachment;
        }
    } else {
        m_linked_attachment = 0;
    }
    W8NavigatorAttachment* linked_attachment = m_linked_attachment;
    if (linked_attachment == 0) {
        return 0;
    }
    unsigned short index = linked_attachment->path_cursor;
    if (index < linked_attachment->path_position_index) {
        do {
            if (linked_attachment->path_values[index] != 0) {
                break;
            }
            ++index;
        } while (index < linked_attachment->path_position_index);
    }
    attachment->InitializeSegment(&movement->position, &linked_attachment->position7[index]);
    unsigned char built = BuildAttachmentPath(attachment, movement->movement_flags);
    if (built != 0) {
        ++index;
        attachment->follow_offset =
            attachment->path_position_index - linked_attachment->path_cursor;
        linked_attachment = m_linked_attachment;
        if (index <= linked_attachment->path_position_index) {
            do {
                unsigned short surface = linked_attachment->path_values[index];
                srVector3T<float>* source = &linked_attachment->position7[index];
                attachment->AppendPathPosition(source, surface);
                linked_attachment = m_linked_attachment;
                ++index;
            } while (index <= linked_attachment->path_position_index);
        }
    }
    return built;
}

/* One movement step for a monster walking an attachment path. The scaled
   frame time is spent in 5.0-unit slices: each slice advances the position
   along the recorded route, stamps every waypoint the advance crossed with
   the accumulator's current time, and re-aims yaw at the step direction.
   Hostile groups run sight and RTAI between slices and bail when combat
   starts. `radius`/`separation` are passed by callers but never read.
   Returns whether the step completed the path. */
// FUNCTION: WIZ8 0x00467150
unsigned int W8PathingService::StepMonsterAlongPath(W8NavigatorMovementState* movement,
                                                    float radius, float separation)
{
    g_navigator_position_changed = true;
    unsigned char arrived = 0;
    W8NavigatorAttachment* attachment = movement->attachment;
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(0x2a95, OCTPATH_CPP, movement->location_id, 1));
    W8MonsterGroup* group = GetMonsterGroupByListIndex(
        GetMonsterGroupIndexByID(0x2a96, OCTPATH_CPP, monster_info->monster_group_id, 1));
    monster_info->p3D->group_linked = 1;
    float remaining = g_rate * g_game_time_accumulator->GetFrameDelta();
    do {
        if (remaining <= g_float_zero) {
            break;
        }
        float step;
        if (remaining <= g_float_005ebc28) {
            step = remaining;
        } else {
            step = 5.0f;
        }
        unsigned short previous = attachment->path_cursor;
        srVector3T<float> direction;
        arrived = attachment->AdvancePositionWithDirection(
            &movement->position, step * movement->movement_scale * g_world_scale,
            &direction);
        unsigned short reached = attachment->path_cursor;
        if (previous < reached) {
            do {
                attachment->path_cursor = previous;
                m_pSurfaces[attachment->path_values[attachment->path_cursor]]
                    .visit_stamp =
                    static_cast<unsigned int>(g_game_time_accumulator->GetElapsed());
                ++previous;
            } while (previous < reached);
            attachment->path_cursor = reached;
        }
        float angle = NormalizeAngle(static_cast<float>(atan2(direction.x, direction.z)));
        movement->target_yaw = angle;
        movement->yaw = angle;
        if (group->ubDisposition == W8_DISPOSITION_HOSTILE) {
            UpdateMonsterSight(monster_info, 1, 0);
            DoMonsterRTAI(monster_info, 1);
            if (monster_info->fInCombat != 0) {
                break;
            }
        }
        remaining -= step;
    } while (arrived == 0);
    return arrived != 0;
}

/* Give everything the service owns back. The four malloc'd tables and the
   conditional path tables go back through free, the bit sets and the two
   hash indexes through their own teardown, and the global slot the
   constructor claimed is cleared last. */
// FUNCTION: WIZ8 0x00457b10
W8PathingService::~W8PathingService()
{
    if (file_path_nodes != 0) {
        free(file_path_nodes);
    }
    if (m_pSurfaces != 0) {
        free(m_pSurfaces);
    }
    if (m_pEdges != 0) {
        free(m_pEdges);
    }
    if (m_pFileWayPoints != 0) {
        free(m_pFileWayPoints);
    }
    if (m_pPathModelInstance != 0) {
        delete m_pPathModelInstance;
    }
    if (m_visible_waypoints != 0) {
        delete m_visible_waypoints;
    }
    if (m_rendered_waypoints != 0) {
        delete m_rendered_waypoints;
    }
    if (m_collected_waypoints != 0) {
        delete m_collected_waypoints;
    }
    delete m_pPathValues;
    delete m_pVisitedCells;
    delete path_heap;
    if (m_search_nodes != 0) {
        delete[] m_search_nodes;
    }
    if (m_path_parameters != 0) {
        delete m_path_parameters;
    }
    if (g_path_scratch != 0) {
        free(g_path_scratch);
    }
    g_path_scratch = 0;
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
    g_pathing = 0;
}

/* Build the pathing service.

   Everything starts cleared except three hundred-bit sets, a reserve table
   sized from the shared bound, and one state object. The service registers
   itself in the global slot as it is built, which is what lets the rest of the
   engine reach it without the octree handing it over. */
// FUNCTION: WIZ8 0x004578e0
W8PathingService::W8PathingService()
{
    grid_scale = 0;
    span = 0;
    level_bounds.minimum.SetZero();
    level_bounds.maximum.SetZero();
    file_path_nodes = 0;
    path_node_count = 0;
    edge_node_count = 0;
    m_ulNumWayPoints = 0;
    m_ulNumWayPtLinks = 0;
    m_unknown_014 = 0;
    m_removed_edge_count = 0;
    m_pSurfaces = 0;
    m_pEdges = 0;
    m_pFileWayPoints = 0;
    m_pPathModelInstance = 0;
    m_visible_waypoints = new BitArray(100);
    m_rendered_waypoints = new BitArray(100);
    m_collected_waypoints = new BitArray(100);
    m_pPathValues = 0;
    m_pVisitedCells = 0;
    level_name = 0;
    m_probe_bounded = 0;
    path_heap = 0;
    m_path_cost_limit = 1.0e10f;
    m_waypoint_editing = false;
    flag1 = false;
    flag2 = false;
    search_visualization = 0;
    waypoints_dirty = 0;
    path_flags1 = 4;
    link_flags = 0;
    start_waypoint = 0;
    destination_waypoint = 0;
    saved_surface = 0;
    m_search_nodes = new W8PathSearchNode[g_path_reserve + 0x14];
    m_search_node_count = 0;
    m_search_node_capacity = 0;
    explicit_target = 0;
    m_trace_configured = 0;
    m_trace_target_location = 0;
    m_trace_max_distance = 0;
    trace_offset.SetZero();
    m_trace_mode = 0;
    trace_height_offset = 0;
    m_trace_target_yaw = 0;
    m_path_parameters = new W8PathParameters();
    m_pCondPaths = 0;
    m_ulNumCondPaths = 0;
    m_ulNumCondFrames = 0;
    m_ulNumCondNodes = 0;
    m_pulCondLookup = 0;
    m_pusCondNodeFrames = 0;
    m_pulCondNodeKeys = 0;
    m_pulCondNodeValues = 0;
    g_pathing = this;
    g_runtime_world_scale = 500.0f;
}

/* Take the octree's own minimum/maximum bounds pair and level name. The span is the vertical extent
   of that box scaled, and the cell count is that span plus one. */
// FUNCTION: WIZ8 0x00458a50
void W8PathingService::ConfigureForLevel(int size, float grid_scale, int path_clearance,
                                         const srVector3T<float>* bounds, const char* name)
{
    path_node_count = size;
    this->grid_scale = grid_scale;
    this->path_clearance = path_clearance;
    level_bounds.minimum = bounds[0];
    level_bounds.maximum = bounds[1];
    span = (level_bounds.maximum.y - level_bounds.minimum.y) * g_path_span_scale;
    cell_count = static_cast<short>(static_cast<int>(span)) + 1;
    level_name = name;
}

/* Classify a waypoint from the path index cell beneath it.

   X and Z form the hash key. Entries with that key carry a one-based vertical
   cell in their low half; among candidates inside the service's vertical span,
   the closest height wins and its complete packed value is returned. */
// FUNCTION: WIZ8 0x00459c00
unsigned int W8PathingService::ClassifyWaypoint(const srVector3T<float>* position)
{
    unsigned int key = PositionToPathKey(this, position, 0);
    unsigned int result = 0;

    if (key != 0) {
        W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
        int slot = index->FindNextEntry(&key, -1);
        int height = static_cast<int>((position->y - level_bounds.minimum.y) / span) + 1;
        int nearest = 0x0fffffff;

        while (slot >= 0) {
            unsigned int value = index->entries[slot].value;
            int delta = (value & 0xffff) - height;

            if (delta < 0) {
                delta = -delta;
            }
            if (delta < cell_count && delta < nearest) {
                nearest = delta;
                result = value;
            }
            slot = index->FindNextEntry(&key, slot);
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00459d60
unsigned int W8PathingService::FindPathCell(srVector3T<float>* position,
                                            srVector2T<unsigned int>* cell, bool adjust)
{
    int path_height = static_cast<int>((position->y - level_bounds.minimum.y) / span) + 1;
    srVector2i source_cell;
    unsigned int key = PositionToPathKey(this, position, &source_cell);
    unsigned int source_x = source_cell.x;
    unsigned int source_z = source_cell.y;
    unsigned int selected_x = source_x;
    unsigned int selected_z = source_z;
    unsigned int selected_key = 0;
    /* A compatible neighbor marks success before the nearest-distance test;
       retail can leave this height unset when that test rejects every match. */
    unsigned int selected_height;
    bool found = false;
    float closest_distance = 10000000.0f;
    int slot = m_pPathValues->FindNextEntry(&key, -1);

    while (slot != -1) {
        unsigned int value = m_pPathValues->entries[slot].value;
        unsigned int height = value & 0xffff;
        int difference = path_height - height;

        if ((value & 0x10000000) == 0 && -cell_count < difference &&
            difference < cell_count) {
            selected_key = key;
            selected_height = height;
            found = true;
            break;
        }
        slot = m_pPathValues->FindNextEntry(&key, slot);
    }

    if (!found) {
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
            slot = m_pPathValues->FindNextEntry(&key, -1);
            while (slot != -1) {
                unsigned int value = m_pPathValues->entries[slot].value;
                unsigned int height = value & 0xffff;
                int difference = path_height - height;

                if ((value & 0x10000000) == 0 && -cell_count < difference &&
                    difference < cell_count) {
                    found = true;
                    float x = (candidate_x + g_float_005ebc7c) * grid_scale -
                              (position->x - level_bounds.minimum.x);
                    float z = (candidate_z + g_float_005ebc7c) * grid_scale -
                              (position->z - level_bounds.minimum.z);
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
                slot = m_pPathValues->FindNextEntry(&key, slot);
            }
        }
    }

    if (adjust != 0 && found) {
        position->Set((selected_x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                      (selected_height - 1) * span + level_bounds.minimum.y,
                      (selected_z + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);
    }
    if (cell != 0) {
        cell->x = selected_x;
        cell->y = selected_z;
    }
    return selected_key;
}

/* Test whether a position lies in the vertical neighborhood represented by its
   X/Z path-index cell, optionally snapping it onto that indexed cell.

   The accepted vertical window is twice the service's cell count in either
   direction. X and Z snap to the horizontal cell centers; Y snaps to the exact
   one-based height carried by the matching packed index value. */
// FUNCTION: WIZ8 0x00462e60
unsigned char W8PathingService::SnapWaypointPosition(srVector3T<float>* position,
                                                     bool snap_to_cell)
{
    int vertical_window = cell_count * 2;
    unsigned int height = static_cast<unsigned int>(static_cast<int>(
                              ((position->y - level_bounds.minimum.y) / span))) +
                          1;
    srVector2i cell;
    unsigned int key = PositionToPathKey(this, position, &cell);
    unsigned int matched_height = height;
    bool found = false;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);

    while (slot != -1 && found == 0) {
        matched_height = index->entries[slot].value & 0xffff;
        int delta = static_cast<int>(height - matched_height);

        if (-vertical_window < delta && delta < vertical_window) {
            found = 1;
            break;
        }
        slot = index->FindNextEntry(&key, slot);
    }

    if (snap_to_cell != 0 && found != 0) {
        position->Set((cell.x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                      (matched_height - 1) * span + level_bounds.minimum.y,
                      (cell.y + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);
    }
    return found;
}

/* Test the first static path-index value in the position's vertical
   neighborhood. The packed direction byte becomes a world-space clearance;
   the special direction flag selects the global fallback instead. A successful
   lookup may also move the position onto the indexed cell before testing that
   clearance. */
// FUNCTION: WIZ8 0x00463040
unsigned char W8PathingService::TestPathCellClearance(srVector3T<float>* position, float clearance,
                                                      bool snap_to_cell)
{
    int vertical_window = cell_count * 2;
    unsigned int height = static_cast<unsigned int>(static_cast<int>(
                              ((position->y - level_bounds.minimum.y) / span))) +
                          1;
    srVector2i cell;
    unsigned int key = PositionToPathKey(this, position, &cell);
    unsigned int packed = 0;
    unsigned int matched_height = height;
    bool found = false;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);

    while (slot != -1 && found == 0) {
        packed = index->entries[slot].value;
        matched_height = packed & 0xffff;
        int delta = static_cast<int>(height - matched_height);

        if ((packed & 0x10000000) == 0 && -vertical_window < delta && delta < vertical_window) {
            found = 1;
            break;
        }
        slot = index->FindNextEntry(&key, slot);
    }

    if (found == 0) {
        return 0;
    }
    if (snap_to_cell != 0) {
        position->Set((cell.x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                      (matched_height - 1) * span + level_bounds.minimum.y,
                      (cell.y + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);
    }

    float direction = g_float_zero;
    if ((packed & 0x01000000) == 0) {
        direction = static_cast<float>((packed >> 16) & 0xff);
    }
    return clearance < direction * g_world_scale + grid_scale * g_float_005ebc7c;
}

/* Snap a position to the closest eligible indexed height no higher than its
   own. Directional entries are ignored unless the caller explicitly permits
   them; X and Z always move to the chosen cell's center. */
// FUNCTION: WIZ8 0x00463290
unsigned char W8PathingService::SnapToLowerPathCell(srVector3T<float>* position,
                                                    bool allow_directional)
{
    bool found = false;
    int nearest = 10000000;
    unsigned int height = static_cast<unsigned int>(static_cast<int>(
                              ((position->y - level_bounds.minimum.y) / span))) +
                          1;
    srVector2i cell;
    unsigned int key = PositionToPathKey(this, position, &cell);
    unsigned int matched_height = 0;
    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);

    while (slot != -1) {
        W8HashEntry<unsigned int, unsigned int>* entry = &index->entries[slot];

        if (allow_directional != 0 || (entry->value & 0x00ff0000) == 0) {
            unsigned int candidate_height = entry->value & 0xffff;
            int delta = static_cast<int>(height - candidate_height);

            if (delta >= 0 && delta < nearest) {
                found = 1;
                nearest = delta;
                matched_height = candidate_height;
            }
        }
        slot = index->FindNextEntry(&key, slot);
    }

    if (found != 0) {
        position->Set((cell.x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                      (matched_height - 1) * span + level_bounds.minimum.y,
                      (cell.y + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);
    }
    return found;
}

/* Search the short arc between an attachment's two endpoint positions. The
   temporary index is rebuilt before the paired directed probes, so both walks
   share only the path position they discover. */
// FUNCTION: WIZ8 0x00462360
unsigned char W8PathingService::ProbeAttachmentPath(W8NavigatorAttachment* attachment)
{
    float distance = (attachment->position0 - attachment->position1).Length();

    if (distance > g_double_005ec3a0) {
        return 0;
    }

    m_probe_bounded = 0;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
    visited->Clear();

    m_probe_cell_key = 0;
    m_probe_limit = 0;
    ProbeWaypointArc(&attachment->position0, &attachment->position1);
    m_probe_bounded = 0;
    m_probe_limit = 0xffffffff;
    ProbeWaypointArc(&attachment->position1, &attachment->position0);

    if (m_probe_cell_key == 0) {
        return 0;
    }
    attachment->start_waypoint = m_probe_position;
    attachment->flags |= W8_NAV_ATTACHMENT_START_WAYPOINT;
    return 1;
}

/* Sweep probes around the arc defined by a pair of waypoint positions.

   The accumulator starts perpendicular to the pair's horizontal direction.
   Every iteration probes that offset, advances by one grid-scale tangent step,
   and renormalizes to the pair's original radius. The dot product identifies
   when the sweep has passed its forward threshold and the walk stops after it
   subsequently crosses behind the starting direction. */
// FUNCTION: WIZ8 0x00462570
void W8PathingService::ProbeWaypointArc(const srVector3T<float>* from, const srVector3T<float>* to)
{
    srVector3T<float> direction;
    srVector3T<float> arc;
    float radius;
    bool passed_forward = 0;
    unsigned int iteration;

    direction = *to - *from;
    radius = direction.Length();
    arc.Set(-direction.z, 0.0f, direction.x);

    for (iteration = 0; iteration < 50000; ++iteration) {
        srVector3T<float> probe;
        srVector3T<float> step;
        float dot;

        probe = *from + arc;
        ProbeWaypointSegment(from, &probe);

        step.Set(arc.z, arc.y, -arc.x);
        step.SetLength(grid_scale);
        arc += step;

        arc.SetLength(radius);

        dot = DotProduct(direction, arc);
        if (dot > g_float_005ec390) {
            passed_forward = 1;
        } else if (passed_forward == 0) {
            continue;
        }
        if (dot < g_float_zero) {
            return;
        }
    }
}

/* Convert the signed steps on the walk's driving and secondary axes into the
   four horizontal direction codes consumed by the segment probe. */
// FUNCTION: WIZ8 0x0045aee0
void W8PathingService::GetPathGridStepDirections(const W8PathGridWalk* walk, int* directions)
{
    if (walk->major_axis != 0) {
        if (walk->step.y < 1) {
            directions[0] = 4;
            if (walk->step.x > 0) {
                directions[1] = 2;
                return;
            }
        } else {
            directions[0] = 0;
            if (walk->step.x > 0) {
                directions[1] = 2;
                return;
            }
        }
        directions[1] = 6;
        return;
    }

    if (walk->step.x < 1) {
        directions[0] = 6;
    } else {
        directions[0] = 2;
    }
    if (walk->step.y > 0) {
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
void W8PathingService::BuildPathGridWalk(const srVector2T<float>* from, const srVector2T<float>* to,
                                         const srVector2T<float>* origin, W8PathGridWalk* walk)
{
    int cell_size = static_cast<int>(grid_scale);
    srVector2i coordinate;
    srVector2i destination;
    srVector2i step;
    srVector2i absolute_delta;
    srVector2T<float> boundary_offset;
    srVector2T<float> signed_delta;
    int major_axis = 0;
    int largest_delta = 0;
    int axis;

    for (axis = 0; axis < 2; ++axis) {
        (&coordinate.x)[axis] = static_cast<int>((&from->x)[axis] - (&origin->x)[axis]);
        (&destination.x)[axis] = static_cast<int>((&to->x)[axis] - (&origin->x)[axis]);

        int delta = (&destination.x)[axis] - (&coordinate.x)[axis];
        (&boundary_offset.x)[axis] = (&coordinate.x)[axis] % cell_size / grid_scale;
        (&signed_delta.x)[axis] = delta;

        if (delta < 0) {
            (&step.x)[axis] = -1;
            delta = -delta;
        } else {
            (&step.x)[axis] = 1;
            (&boundary_offset.x)[axis] = g_float_one - (&boundary_offset.x)[axis];
        }
        if (largest_delta < delta) {
            largest_delta = delta;
            major_axis = axis;
        }
        (&absolute_delta.x)[axis] = delta;
    }

    int minor_axis = (major_axis + 1) % 2;
    float ratio = (&signed_delta.x)[minor_axis] / (&signed_delta.x)[major_axis];
    int error_delta = static_cast<int>((ratio < 0.0f ? -ratio : ratio) * grid_scale);
    int error = static_cast<int>(cell_size * (&boundary_offset.x)[minor_axis] -
                                 error_delta * (&boundary_offset.x)[major_axis]);
    int count;

    if (largest_delta % cell_size == 0) {
        count = largest_delta / cell_size;
    } else {
        count = largest_delta / cell_size + 1;
    }

    walk->major_axis = major_axis;
    walk->minor_axis0 = minor_axis;
    walk->error_reset0 = cell_size;
    walk->count = count;
    walk->cell.Set(destination.x / cell_size, destination.y / cell_size, 0);
    walk->step.Set(step.x, step.y, 0);
    walk->error_delta0 = error_delta;
    walk->error0 = error;
    walk->minor_axis1 = 0;
    walk->error_delta1 = 0;
    walk->error1 = 0;
    walk->error_reset1 = 0;
}

/* Walk every horizontal path cell crossed by a short waypoint segment.

   Each cell chooses the first vertically compatible path record. The secondary
   index carries the accumulated low-half cost and the most recent high-half
   step cost; when a bounded probe is active, the cheapest reached cell and its
   world-space center are retained on the service. Direction bits on the chosen
   path record can terminate the walk after the corresponding grid step. */
// FUNCTION: WIZ8 0x00462750
unsigned char W8PathingService::ProbeWaypointSegment(const srVector3T<float>* from,
                                                     const srVector3T<float>* to)
{
    float distance = (*to - *from).Length();

    if (grid_scale + grid_scale > distance) {
        return 1;
    }

    srVector2i cell;
    srVector2T<float> walk_from;
    srVector2T<float> walk_to;
    srVector2T<float> origin;
    W8PathGridWalk walk;
    int directions[2];

    PositionToPathKey(this, from, &cell);
    walk_from = from->xz();
    walk_to = to->xz();
    origin = level_bounds.minimum.xz();
    BuildPathGridWalk(&walk_from, &walk_to, &origin, &walk);
    GetPathGridStepDirections(&walk, directions);

    int error = walk.error0;
    bool bounded_probe = m_probe_bounded != 0 && m_probe_limit != 0;
    unsigned int height = static_cast<unsigned int>(
                              static_cast<int>(((from->y - level_bounds.minimum.y) / span))) +
                          1;
    bool blocked = false;
    int iteration = 0;

    while (iteration < walk.count && blocked == 0) {
        unsigned int cell_key = cell.y * 0x10000 + cell.x;
        W8OctreeIndex* visited_index = static_cast<W8OctreeIndex*>(m_pVisitedCells);
        unsigned int visited = visited_index->Lookup(&cell_key);

        if (bounded_probe != 0 && (visited & 0xffff) != 0xffff) {
            bounded_probe = 0;
        }

        unsigned int direction_mask = 0;
        unsigned int path_value = 0;
        bool found = false;

        if (iteration == 0 || (visited & 0xffff0000) != 0xffff0000 || bounded_probe != 0) {
            W8HashTable<unsigned int, unsigned int>* path_index = m_pPathValues;
            int slot = path_index->FindNextEntry(&cell_key, -1);

            while (slot >= 0 && found == 0) {
                path_value = path_index->entries[slot].value;
                int height_delta = (path_value & 0xffff) - height;

                if ((path_value & 0x10000000) == 0 && -cell_count < height_delta &&
                    height_delta < cell_count) {
                    if ((path_value & 0x01000000) != 0) {
                        direction_mask = path_value >> 16 & 0xff;
                    }
                    found = 1;
                    height = path_value & 0xffff;
                }
                slot = path_index->FindNextEntry(&cell_key, slot);
            }

            if (found == 0) {
                blocked = 1;
            } else if (bounded_probe == 0 && ((m_probe_limit == 0 && visited == 0) ||
                                              (m_probe_limit != 0 && (visited & 0xffff) != 0 &&
                                               (visited & 0xffff0000) == 0))) {
                srVector3T<float> position;
                unsigned int initial_cost;

                position.Set((cell.x + g_float_005ebc7c) * grid_scale + level_bounds.minimum.x,
                             (height - 1) * span + level_bounds.minimum.y,
                             (cell.y + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z);

                initial_cost = static_cast<unsigned int>(
                    static_cast<int>(((position - *to).Length() * g_double_005ec3b0)));

                if (m_probe_limit == 0) {
                    int stored_cost = initial_cost;
                    visited_index->Insert(&cell_key, &stored_cost);
                } else {
                    unsigned int step_cost = static_cast<unsigned int>(
                        static_cast<int>(initial_cost * g_double_005ec3a8));
                    unsigned int total_cost = (visited & 0xffff) + step_cost;

                    if (total_cost < m_probe_limit) {
                        m_probe_limit = total_cost;
                        m_probe_cell_key = cell_key;
                        m_probe_position = position;
                    }

                    visited_index->Remove(&cell_key);
                    int stored_cost = step_cost << 16 | visited;
                    visited_index->Insert(&cell_key, &stored_cost);
                }
            }
        } else {
            blocked = 1;
        }

        unsigned int direction;
        if (error >= 0 || blocked != 0) {
            direction = directions[0];
            (&cell.x)[walk.major_axis] += (&walk.step.x)[walk.major_axis];
            error -= walk.error_delta0;
        } else {
            direction = directions[1];
            --iteration;
            (&cell.x)[walk.minor_axis0] += (&walk.step.x)[walk.minor_axis0];
            error += walk.error_reset0;
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
unsigned int W8PathingService::ComputeWaypointNeighborMask(const srVector2i* cell,
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
            srVector2i neighbor;
            neighbor.x = cell->x;
            neighbor.y = cell->y;

            if (direction >= 1 && direction <= 3) {
                ++neighbor.x;
            } else if (direction > 4) {
                --neighbor.x;
            }
            if (direction < 2 || direction > 6) {
                ++neighbor.y;
            } else if (direction > 2 && direction < 6) {
                --neighbor.y;
            }

            unsigned int key = neighbor.y * 0x10000 + neighbor.x;
            W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
            int slot = index->FindNextEntry(&key, -1);
            bool found = false;

            while (slot != -1) {
                unsigned int candidate = index->entries[slot].value;
                int height_delta = (path_value & 0xffff) - (candidate & 0xffff);
                if ((candidate & 0x10000000) == 0 && -cell_count < height_delta &&
                    height_delta < cell_count) {
                    found = 1;
                    break;
                }
                slot = index->FindNextEntry(&key, slot);
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
unsigned char W8PathingService::TestWaypointSpan(const srVector3T<float>* source,
                                                 srVector3T<float>* destination,
                                                 bool adjust_destination,
                                                 bool diagonal_steps)
{
    bool blocked = false;

    if (adjust_destination == 0 &&
        (source->x < level_bounds.minimum.x || source->y < level_bounds.minimum.y ||
         source->z < level_bounds.minimum.z || level_bounds.maximum.x < source->x ||
         level_bounds.maximum.y < source->y || level_bounds.maximum.z < source->z ||
         destination->x < level_bounds.minimum.x || destination->y < level_bounds.minimum.y ||
         destination->z < level_bounds.minimum.z || level_bounds.maximum.x < destination->x ||
         level_bounds.maximum.y < destination->y || level_bounds.maximum.z < destination->z)) {
        return 0;
    }

    span_blocked = 0;
    srVector2i cell;
    unsigned int cell_key = PositionToPathKey(this, source, &cell);
    unsigned int destination_key = PositionToPathKey(this, destination, 0);
    m_waypoint_neighbor_mask = 0;

    W8HashTable<unsigned int, unsigned int>* path_index = m_pPathValues;

    if (cell_key == destination_key) {
        unsigned int height = static_cast<unsigned int>(static_cast<int>(
                                  ((source->y - level_bounds.minimum.y) / span))) +
                              1;
        int slot = path_index->FindNextEntry(&cell_key, -1);
        unsigned int source_value = 0;
        bool found = false;

        while (slot >= 0 && found == 0) {
            unsigned int value = path_index->entries[slot].value;
            int difference = (value & 0xffff) - height;
            if ((value & 0x10000000) == 0 && -cell_count < difference &&
                difference < cell_count) {
                source_value = value;
                found = 1;
            }
            slot = path_index->FindNextEntry(&cell_key, slot);
        }
        if (found == 0) {
            return 0;
        }

        m_waypoint_neighbor_mask = ComputeWaypointNeighborMask(&cell, source_value);
        height = static_cast<unsigned int>(
                     static_cast<int>(((destination->y - level_bounds.minimum.y) / span))) +
                 1;
        slot = path_index->FindNextEntry(&cell_key, -1);
        found = 0;
        unsigned int destination_value;

        while (slot >= 0 && found == 0) {
            unsigned int value = path_index->entries[slot].value;
            int difference = (value & 0xffff) - height;
            if ((value & 0x10000000) == 0 && -cell_count < difference &&
                difference < cell_count) {
                destination_value = value;
                found = 1;
            }
            slot = path_index->FindNextEntry(&cell_key, slot);
        }
        return found != 0 && destination_value == source_value;
    }

    srVector2T<float> walk_source;
    srVector2T<float> walk_destination;
    srVector2T<float> origin;
    W8PathGridWalk walk;
    int directions[2];
    walk_source = source->xz();
    walk_destination = destination->xz();
    origin = level_bounds.minimum.xz();
    BuildPathGridWalk(&walk_source, &walk_destination, &origin, &walk);
    GetPathGridStepDirections(&walk, directions);

    int error = walk.error0;
    unsigned int height = static_cast<unsigned int>(
                              static_cast<int>(((source->y - level_bounds.minimum.y) / span))) +
                          1;
    unsigned int previous_key = 0;
    unsigned int previous_value;
    int iteration = 0;

    while (iteration < walk.count && blocked == 0) {
        int slot = path_index->FindNextEntry(&cell_key, -1);
        unsigned int direction_mask = 0;
        unsigned int path_value;
        bool found = false;

        while (slot >= 0 && found == 0) {
            path_value = path_index->entries[slot].value;
            int difference = (path_value & 0xffff) - height;

            if (-cell_count < difference && difference < cell_count) {
                if ((path_value & 0x10000000) == 0) {
                    if ((path_value & 0x01000000) != 0) {
                        direction_mask = path_value >> 16 & 0xff;
                    }
                    found = 1;
                    height = path_value & 0xffff;
                    previous_key = cell_key;
                    previous_value = path_value;
                } else {
                    span_blocked = 1;
                }
            }
            slot = path_index->FindNextEntry(&cell_key, slot);
        }

        if (found == 0) {
            unsigned int mask_key = previous_key;
            unsigned int mask_value = previous_value;
            if (previous_key == 0) {
                mask_key = cell_key;
                mask_value = path_value;
            }
            if (previous_key != 0 || cell_key != 0) {
                srVector2i mask_cell;
                mask_cell.x = mask_key & 0xffff;
                mask_cell.y = mask_key >> 16;
                m_waypoint_neighbor_mask = ComputeWaypointNeighborMask(&mask_cell, mask_value);
            }
            blocked = 1;
        }

        if ((previous_value & 0x04000000) != 0) {
            span_blocked = 1;
        }

        unsigned int direction = directions[0];
        if (diagonal_steps == 0) {
            if (error >= 0 || blocked != 0) {
                (&cell.x)[walk.major_axis] += (&walk.step.x)[walk.major_axis];
                error -= walk.error_delta0;
            } else {
                (&cell.x)[walk.minor_axis0] += (&walk.step.x)[walk.minor_axis0];
                direction = directions[1];
                --iteration;
                error += walk.error_reset0;
            }
        } else if (error < 0 && blocked == 0) {
            (&cell.x)[walk.minor_axis0] += (&walk.step.x)[walk.minor_axis0];
            error += walk.error_reset0;
            if (static_cast<unsigned int>(cell.y * 0x10000 + cell.x) == destination_key) {
                --iteration;
                direction = directions[1];
            } else {
                if ((directions[0] == 0 && directions[1] == 6) ||
                    (directions[0] == 6 && directions[1] == 0)) {
                    direction = 7;
                } else {
                    direction = (directions[0] + directions[1]) / 2;
                }
                (&cell.x)[walk.major_axis] += (&walk.step.x)[walk.major_axis];
                error -= walk.error_delta0;
            }
        } else {
            (&cell.x)[walk.major_axis] += (&walk.step.x)[walk.major_axis];
            error -= walk.error_delta0;
        }

        if (cell_key == destination_key) {
            iteration = walk.count;
        } else if (direction_mask != 0 && (direction_mask & 1 << (direction & 0x1f)) == 0) {
            unsigned int mask_key = previous_key;
            unsigned int mask_value = previous_value;
            if (previous_key == 0) {
                mask_key = cell_key;
                mask_value = path_value;
            }
            if (previous_key != 0 || cell_key != 0) {
                srVector2i mask_cell;
                mask_cell.x = mask_key & 0xffff;
                mask_cell.y = mask_key >> 16;
                m_waypoint_neighbor_mask = ComputeWaypointNeighborMask(&mask_cell, mask_value);
            }
            blocked = 1;
        }

        cell_key = cell.y * 0x10000 + cell.x;
        ++iteration;
    }

    if (adjust_destination == 0) {
        if (blocked == 0) {
            int destination_height =
                static_cast<int>((destination->y - level_bounds.minimum.y) / span) + 1;
            int difference = destination_height - height;
            if (difference < -cell_count || cell_count < difference) {
                blocked = 1;
            }
        }
    } else if (previous_key == 0) {
        *destination = *source;
    } else {
        if (blocked != 0) {
            destination->x = ((previous_key & 0xffff) + g_float_005ebc7c) * grid_scale +
                             level_bounds.minimum.x;
            destination->z =
                ((previous_key >> 16) + g_float_005ebc7c) * grid_scale + level_bounds.minimum.z;
        }
        destination->y = (height - 1) * span + level_bounds.minimum.y;
    }

    return blocked == 0;
}

/* Compare clearance along the two compass rays bracketing a horizontal
   direction. The normalized Z component selects the pair; the sign of X
   selects which half of the compass owns the middle bands. */
// FUNCTION: WIZ8 0x0045aac0
float W8PathingService::CompareDirectionalClearance(const srVector3T<float>* position,
                                                    const srVector3T<float>* direction,
                                                    float distance)
{
    srVector2T<float> horizontal = direction->xz();
    horizontal.Normalize();
    float normalized_x = horizontal.x;
    float normalized_z = horizontal.y;

    int first_direction;
    int second_direction;
    if (normalized_x <= g_float_zero) {
        if (g_path_direction_threshold_3 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        } else if (g_path_direction_threshold_2 < normalized_z) {
            first_direction = 6;
            second_direction = 0;
        } else if (normalized_z <= g_path_direction_threshold_1) {
            if (normalized_z <= g_path_direction_threshold_0) {
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
        if (g_path_direction_threshold_3 < normalized_z) {
            first_direction = 7;
            second_direction = 1;
        } else if (g_path_direction_threshold_2 < normalized_z) {
            first_direction = 0;
            second_direction = 2;
        } else if (g_path_direction_threshold_1 < normalized_z) {
            first_direction = 1;
            second_direction = 3;
        } else if (g_path_direction_threshold_0 < normalized_z) {
            first_direction = 2;
            second_direction = 4;
        } else {
            first_direction = 3;
            second_direction = 5;
        }
    }

    srVector2i cell;
    PositionToPathKey(this, position, &cell);
    unsigned int height = static_cast<unsigned int>(static_cast<int>(
                              ((position->y - level_bounds.minimum.y) / span))) +
                          1;
    float first = MeasureDirectionalPath(&cell, first_direction, height, distance);
    float second = MeasureDirectionalPath(&cell, second_direction, height, distance);
    return second - first;
}

/* Measure how much of a requested run remains traversable in one compass
   direction. Cardinal runs use the retail diagonal-to-axis scale before cell
   stepping; every crossed cell must carry a vertically compatible record whose
   explicit direction mask, when present, permits the same direction. */
// FUNCTION: WIZ8 0x0045ac70
float W8PathingService::MeasureDirectionalPath(const srVector2i* cell, int direction,
                                               unsigned int height, float distance)
{
    int step_x;
    int step_z;
    float remaining = distance;

    switch (direction) {
    case 0:
        step_x = 0;
        step_z = 1;
        remaining *= g_path_cardinal_scale;
        break;
    case 1:
        step_x = 1;
        step_z = 1;
        break;
    case 2:
        step_x = 1;
        step_z = 0;
        remaining *= g_path_cardinal_scale;
        break;
    case 3:
        step_x = 1;
        step_z = -1;
        break;
    case 4:
        step_x = 0;
        step_z = -1;
        remaining *= g_path_cardinal_scale;
        break;
    case 5:
        step_x = -1;
        step_z = -1;
        break;
    case 6:
        step_x = -1;
        step_z = 0;
        remaining *= g_path_cardinal_scale;
        break;
    case 7:
        step_x = -1;
        step_z = 1;
        break;
    }

    int cell_x = cell->x;
    int cell_z = cell->y;
    bool stopped = false;

    while (grid_scale < remaining) {
        cell_x += step_x;
        cell_z += step_z;
        unsigned int key = cell_z * 0x10000 + cell_x;
        W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
        int slot = index->FindNextEntry(&key, -1);
        unsigned int path_value;
        bool found = false;

        while (slot != -1) {
            path_value = index->entries[slot].value;
            int difference = (path_value & 0xffff) - height;
            if ((path_value & 0x10000000) == 0 && -cell_count < difference &&
                difference < cell_count) {
                found = 1;
                height = path_value & 0xffff;
                break;
            }
            slot = index->FindNextEntry(&key, slot);
        }

        if (found != 0) {
            remaining -= grid_scale;
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
unsigned short W8PathingService::FindWaypoint(const srVector3T<float>* position,
                                              bool exhaustive)
{
    srVector3T<float> query = *position;
    unsigned short result = 0;
    start_waypoint = 0;

    if (SnapWaypointPosition(&query, 0) == 0) {
        return 0;
    }

    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> half_extent;
    half_extent.Set(g_float_005ec360, g_float_005ec35c, g_float_005ec360);
    lower = query - half_extent;
    upper = query + half_extent;

    unsigned long* candidates = 0;
    int count = g_octree->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);
    if (count == 0) {
        return result;
    }
    if (static_cast<unsigned int>(count) >= 200) {
        srAssertFail("ulCount<200", OCTPATH_CPP, 0xe0c, "Too many nodes in list");
    }

    unsigned int distances[199];
    int index;
    if (count > 1) {
        for (index = 0; index < count; ++index) {
            const srVector3T<float>* candidate = &m_pSurfaces[candidates[index]].position;
            srVector3T<float> delta = query - *candidate;
            distances[index] = static_cast<unsigned int>(static_cast<int>(delta.Length()));

            if (distances[index] < g_float_005ebc64 && delta.xz().Length() < g_double_005ec150) {
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
        if (TestWaypointSpan(&query, &m_pSurfaces[candidates[index]].position, 0, 0) != 0) {
            result = static_cast<unsigned short>(candidates[index]);
        }
    }

    if (result == 0 && exhaustive != 0) {
        m_probe_position.SetZero();

        W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
        visited->Clear();
        start_waypoint = 0;
        m_probe_bounded = 0;

        for (index = 0; index < count; ++index) {
            if (start_waypoint != 0) {
                return result;
            }

            bool saved_flag = m_probe_bounded != 0;
            visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
            m_probe_bounded = 0;
            visited->Clear();

            m_probe_cell_key = 0;
            m_probe_limit = 0;
            srVector3T<float>* candidate = &m_pSurfaces[candidates[index]].position;
            ProbeWaypointArc(&query, candidate);
            m_probe_bounded = saved_flag;
            m_probe_limit = 0xffffffff;
            ProbeWaypointArc(candidate, &query);
            if (m_probe_cell_key != 0) {
                start_waypoint = static_cast<unsigned short>(candidates[index]);
            }
        }
    }

    return result;
}

/* Snap only the vertical component of a position to the first path-index
   record in the same horizontal cell and inside the service's vertical band.

   Unlike SnapWaypointPosition, this operation leaves X and Z exactly
   as supplied. The one-based height stored in the index is converted back to
   the level's world-space Y coordinate. */
// FUNCTION: WIZ8 0x0045b5a0
void W8PathingService::SnapPathHeight(srVector3T<float>* position)
{
    unsigned int key = PositionToPathKey(this, position, 0);

    if (key == 0) {
        return;
    }

    W8HashTable<unsigned int, unsigned int>* index = m_pPathValues;
    int slot = index->FindNextEntry(&key, -1);
    int height = static_cast<int>((position->y - level_bounds.minimum.y) / span) + 1;
    unsigned int matched_height;
    bool found = false;

    while (slot != -1 && !found) {
        unsigned int candidate_height = index->entries[slot].value & 0xffff;
        int difference = static_cast<int>(candidate_height) - height;
        if (-cell_count < difference && difference < cell_count) {
            matched_height = candidate_height;
            found = true;
        }
        slot = index->FindNextEntry(&key, slot);
    }
    if (found) {
        position->y = (matched_height - 1) * span + level_bounds.minimum.y;
    }
}

/* Build the path surface normal from height-snapped samples at the current
   point, one X cell over, and one Z cell over. The Z-edge crossed with the
   X-edge gives the upward normal on flat ground, matching the retail order. */
// FUNCTION: WIZ8 0x0045b730
void W8PathingService::GetPathSurfaceNormal(const srVector3T<float>* position,
                                            srVector3T<float>* normal)
{
    srVector3T<float> origin = *position;
    srVector3T<float> x_sample = *position;
    srVector3T<float> z_sample = *position;

    SnapPathHeight(&origin);
    x_sample.x += grid_scale;
    SnapPathHeight(&x_sample);
    z_sample.z += grid_scale;
    SnapPathHeight(&z_sample);

    *normal = CrossProduct(z_sample - origin, x_sample - origin);
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
void W8PathingService::ActivateMovementTrigger(W8NavigatorMovementState* movement,
                                               bool use_path_edge)
{
    if ((movement->movement_flags & 0x10000000) == 0) {
        return;
    }

    srVector3T<float> lower;
    srVector3T<float> upper;

    if (use_path_edge == 0) {
        if (IsZeroVector(&movement->velocity) != 0) {
            return;
        }
        lower = movement->position;
        upper = movement->position + movement->velocity * 2.0;
    } else {
        W8NavigatorAttachment* attachment = movement->attachment;

        /* Retail reaches the shared query with the local bounds untouched
           when this cursor is exhausted. Keep that source-level fallthrough;
           callers normally enter edge mode only while a pair remains. */
        if (attachment->path_cursor < attachment->path_position_index) {
            unsigned short* pairs = attachment->path_values;
            unsigned short source = pairs[attachment->path_cursor];
            unsigned short destination = pairs[attachment->path_cursor + 1];
            W8PathSurface* source_surface = &m_pSurfaces[source];
            unsigned short edge_index = source_surface->first_edge;

            if (edge_index == 0) {
                return;
            }
            while (m_pEdges[edge_index].destination != destination) {
                edge_index = m_pEdges[edge_index].next;
                if (edge_index == 0) {
                    return;
                }
            }

            unsigned int flags = m_pEdges[edge_index].flags;
            if ((flags & 0x10000000) == 0 || (flags & 0x80000000) == 0) {
                return;
            }
            lower = source_surface->position;
            upper = m_pSurfaces[destination].position;
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
    int count = g_octree->QueryObjects(&candidates, &lower, &upper, W8_OCTREE_KIND_PROP, -1);
    if (count <= 0) {
        return;
    }

    Trigger* selected = 0;
    if (count == 1) {
        W8Prop* prop = *g_world->collidable_props->GetAt(candidates[0]);
        Trigger* trigger = prop->GetGDPropOwnerTrigger();
        if (prop->GetSetting6C() == 0 || trigger == 0 ||
            (trigger->flags & W8_TRIGGER_ENABLED) == 0) {
            return;
        }
        selected = trigger;
    } else {
        srVector3T<float> midpoint;
        midpoint = (upper - lower) * g_double_005ebe80 + lower;
        double nearest_distance = 1e32;

        for (int index = 0; index < count; ++index) {
            W8Prop* prop = *g_world->collidable_props->GetAt(candidates[index]);
            Trigger* trigger = prop->GetGDPropOwnerTrigger();
            if (prop->GetSetting6C() != 0 && trigger != 0 &&
                (trigger->flags & W8_TRIGGER_ENABLED) != 0) {
                srVector3T<float> center;
                prop->GetPosition(&center);
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
        selected->Activate();
    }
}

/* Drive the path editor's owned scene node from the service's mode flags.

   The ordinary mode draws one adjusted position or hides the existing node.
   Active path mode prepares the source/destination pair and rebuilds the
   visualization when the collector reports content. The alternate editor
   mode lazily creates and attaches its node before drawing the adjusted point.
   Visibility flag order follows the retail exits exactly. */
// FUNCTION: WIZ8 0x0045bc40
void W8PathingService::UpdatePathVisualization(const srVector3T<float>* source,
                                               const srVector3T<float>* destination)
{
    W8World* world = GetWorld();
    srNode* node = m_pPathModelInstance;

    if (m_waypoint_editing != 0) {
        srVector3T<float> adjusted = *source;
        srVector3T<float> endpoint = *destination;
        g_octree->AdjustPosition(&adjusted, 1);
        PreparePathVisualization(&adjusted, &endpoint);

        if (CollectPathVisualization(&adjusted) != 0) {
            if (m_pPathModelInstance != 0) {
                BuildPathVisualization();
                m_pPathModelInstance->clearFlag(srNode::FLAG_DISABLE);
                return;
            }

            m_pPathModelInstance = BuildPathVisualization();
            node = m_pPathModelInstance;
            if (node != 0) {
                node->setParent(world->static_scene, 1);
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

    if (flag1 == 0 && search_visualization == 0) {
        DrawPathPosition(*source, 0);
        node = m_pPathModelInstance;
        if (node != 0) {
            node->setFlag(srNode::FLAG_DISABLE);
            node->setFlag(srNode::FLAG_TERMINATE);
        }
        return;
    }

    if (search_visualization != 0) {
        if (m_pPathModelInstance == 0) {
            EnsurePathVisualization();
            node = m_pPathModelInstance;
            node->setParent(world->static_scene, 1);
            node->setFlag(srNode::FLAG_TERMINATE);
            if (m_pPathModelInstance == 0) {
                node->clearFlag(srNode::FLAG_DISABLE);
                return;
            }
        }

        srVector3T<float> adjusted = *source;
        g_octree->AdjustPosition(&adjusted, 1);
        DrawPathPosition(adjusted, 1);
    }

    m_pPathModelInstance->clearFlag(srNode::FLAG_DISABLE);
}

/* Populate the editor mesh from the currently visible waypoint set. Marker
   geometry occupies the first hundred six-polygon groups; directed links use
   the following groups and are emitted once when a visible reverse edge
   exists. */
// FUNCTION: WIZ8 0x0045BE30
stModelInstance* W8PathingService::BuildPathVisualization()
{
    static srVector3T<float> marker_offsets[5];
    int index;

    if (m_pPathModelInstance == 0) {
        EnsurePathVisualization();
    }
    marker_offsets[0].Set(0.0, 0.5, 0.0);
    marker_offsets[1].Set(-0.25, 0.0, -0.25);
    marker_offsets[2].Set(-0.25, 0.0, 0.25);
    marker_offsets[3].Set(0.25, 0.0, 0.25);
    marker_offsets[4].Set(0.25, 0.0, -0.25);
    for (index = 0; index < 5; ++index) {
        marker_offsets[index] *= g_double_005ec150;
    }

    stMeshModel* model = static_cast<stMeshModel*>(m_pPathModelInstance->getModel());
    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    srVector3T<float>* vertices = model->getVertexLoc();
    srVector3i* polygons = model->getPolyVertex();
    int marker_count = 0;
    int link_count = 0;
    m_rendered_waypoints->ClearAll();
    unsigned short next = m_visible_waypoints->NextSetBit(1);
    while (next != 0 && marker_count < 99) {
        unsigned short source_index = static_cast<unsigned short>(next - 1);
        W8PathSurface* source = &m_pSurfaces[source_index];
        srVector3T<float> marker_color;
        srVector3T<float> marker_scale((source->flags >> 12) * g_double_005ec378,
                                       (source->flags >> 12) * 0.5,
                                       (source->flags >> 12) * g_double_005ec378);
        int marker_vertex = marker_count * 5;

        m_rendered_waypoints->SetAndGrow(source_index);
        GetWaypointVisualizationColor(source_index, &marker_color);
        for (index = 0; index < 5; ++index) {
            srVector3T<float> offset = marker_offsets[index];
            offset *= marker_scale;
            offset.y += g_double_005ec150;
            vertices[marker_vertex + index] = offset + source->position;
            colors[marker_vertex + index] = marker_color;
        }
        if ((source->flags & 2) != 0) {
            colors[marker_vertex].x = colors[marker_vertex].x <= g_float_zero ? 1.0f : 0.0f;
            colors[marker_vertex].y = colors[marker_vertex].y <= g_float_zero ? 1.0f : 0.0f;
            colors[marker_vertex].z = colors[marker_vertex].z <= g_float_zero ? 1.0f : 0.0f;
        } else if ((source->flags & 0x40) != 0) {
            colors[marker_vertex].SetZero();
        }

        unsigned short edge_index = source->first_edge;
        while (edge_index != 0 && link_count * 6 + 504 <= 0x30d1) {
            W8PathEdge* edge = &m_pEdges[edge_index];
            unsigned short destination_index = edge->destination;
            W8PathSurface* destination = &m_pSurfaces[destination_index];
            unsigned short reverse_index = destination->first_edge;
            bool reverse_found = false;

            while (reverse_index != 0 && reverse_found == 0) {
                if (m_pEdges[reverse_index].destination == source_index) {
                    reverse_found = true;
                } else {
                    reverse_index = m_pEdges[reverse_index].next;
                }
            }

            if (!m_rendered_waypoints->Test(destination_index) &&
                !m_visible_waypoints->Test(destination_index)) {
                srVector3T<float> destination_color;
                srVector3T<float> destination_scale(
                    (destination->flags >> 12) * g_double_005ec378,
                    (destination->flags >> 12) * 0.5,
                    (destination->flags >> 12) * g_double_005ec378);
                int destination_vertex = (marker_count + 1) * 5;

                GetWaypointVisualizationColor(destination_index, &destination_color);
                for (index = 0; index < 5; ++index) {
                    srVector3T<float> offset = marker_offsets[index];
                    offset *= destination_scale;
                    offset.y += g_double_005ec150;
                    vertices[destination_vertex + index] = offset + destination->position;
                    colors[destination_vertex + index] = destination_color;
                }
                if ((source->flags & 2) != 0) {
                    colors[destination_vertex].x =
                        colors[destination_vertex].x <= g_float_zero ? 1.0f : 0.0f;
                    colors[destination_vertex].y =
                        colors[destination_vertex].y <= g_float_zero ? 1.0f : 0.0f;
                    colors[destination_vertex].z =
                        colors[destination_vertex].z <= g_float_zero ? 1.0f : 0.0f;
                } else if ((source->flags & 0x40) != 0) {
                    colors[destination_vertex].SetZero();
                }
                m_rendered_waypoints->SetAndGrow(destination_index);
                ++marker_count;
            }

            if (source_index < destination_index ||
                !m_visible_waypoints->Test(destination_index) || reverse_found == 0) {
                int base_vertex = 500 + link_count * 6;
                int base_polygon = 600 + link_count * 6;
                srVector2T<float> perpendicular(
                    -(destination->position.z - source->position.z),
                    destination->position.x - source->position.x);
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

                vertices[base_vertex] = source->position;
                vertices[base_vertex].y += g_float_005ec370;
                vertices[base_vertex + 3] = destination->position;
                vertices[base_vertex + 3].y += g_float_005ec370;
                srVector3T<float> offset;
                offset.Set(perpendicular_x, g_world_scale, perpendicular_z);
                vertices[base_vertex + 1] = source->position + offset;
                vertices[base_vertex + 2] = source->position - offset;
                vertices[base_vertex + 4] = destination->position + offset;
                vertices[base_vertex + 5] = destination->position - offset;

                for (index = 0; index < 6; ++index) {
                    colors[base_vertex + index].SetZero();
                }
                if ((edge->flags & 0x80000000) == 0) {
                    for (index = 0; index < 3; ++index) {
                        colors[base_vertex + index].z = 1.0f;
                        if ((edge->flags & 0x20000000) != 0) {
                            colors[base_vertex + index].y = 1.0f;
                        }
                    }
                }
                if (reverse_found != 0 &&
                    (m_pEdges[reverse_index].flags & 0x80000000) == 0) {
                    for (index = 3; index < 6; ++index) {
                        colors[base_vertex + index].z = 1.0f;
                    }
                }
                if (reverse_found == 0 ||
                    (m_pEdges[reverse_index].flags & 0x20000000) != 0) {
                    for (index = 3; index < 6; ++index) {
                        colors[base_vertex + index].y = 1.0f;
                    }
                }
                ++link_count;
            }
            edge_index = edge->next;
        }

        ++marker_count;
        next = m_visible_waypoints->NextSetBit(0);
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
    model->setDirtyAll();
    model->flags &= ~W8_MESH_VERTEX_LIGHTING_DIRTY;
    return m_pPathModelInstance;
}

/* Rebuild the editor's bounded grid search when the cursor enters a new path
   cell. Each reachable vertical span is inserted once into the visited hash;
   the minimum heap expands the nearest pending position first, and the final
   node set is handed to the search-trace renderer. */
// FUNCTION: WIZ8 0x0045C9A0
void W8PathingService::DrawPathPosition(srVector3T<float> position, unsigned char mode)
{
    if (mode == 0 || g_combat_inactive == 0) {
        g_path_visualization_cell = 0;
        return;
    }

    srVector2i root_cell;
    unsigned int root_key = PositionToPathKey(this, &position, &root_cell);
    if (root_key == g_path_visualization_cell) {
        return;
    }
    g_path_visualization_cell = root_key;

    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pVisitedCells);
    visited->Clear();

    m_search_node_count = 0;
    path_heap->heap->size = 0;
    unsigned short root_index = AllocateSearchNode();
    W8PathSearchNode* root = &m_search_nodes[root_index];
    root->flags = 0;
    root->node_index = root_index;
    root->cell_x = static_cast<unsigned short>(root_cell.x);
    root->cell_z = static_cast<unsigned short>(root_cell.y);
    root->path_height = static_cast<unsigned short>(
        static_cast<int>((position.y - level_bounds.minimum.y) / span) + 1);
    root->parent_node = 0;
    root->base_score = 0.0f;
    root->position = position;

    W8PathHeap* heap = path_heap->heap;
    W8PathHeapEntry root_entry;
    root_entry.node = root_index;
    root_entry.priority = static_cast<unsigned int>(root->score);
    heap->Insert(&root_entry);
    path_heap->root_node = heap->entries[0].node;

    unsigned int best_node = root_index;
    while (best_node != 0 && m_search_node_count < g_path_reserve) {
        W8PathSearchNode* current = &m_search_nodes[best_node];
        unsigned short current_x = current->cell_x;
        unsigned short current_z = current->cell_z;
        unsigned short current_height = current->path_height;

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
            if (visited->Lookup(&key) != 0) {
                continue;
            }

            W8HashTable<unsigned int, unsigned int>* paths = m_pPathValues;
            int path_slot = paths->FindNextEntry(&key, -1);
            while (path_slot >= 0) {
                unsigned int path_value = paths->entries[path_slot].value;
                int height_delta = static_cast<int>(path_value & 0xffff) - current_height;
                if (height_delta > -static_cast<int>(cell_count) &&
                    height_delta < static_cast<int>(cell_count)) {
                    unsigned short node_index = AllocateSearchNode();
                    int node_value = node_index;
                    visited->Insert(&key, &node_value);

                    W8PathSearchNode* node = &m_search_nodes[node_index];
                    node->flags = 0;
                    if ((path_value & 0x10000000) != 0) {
                        node->flags = 0x800;
                    }
                    if ((path_value & 0x04000000) != 0) {
                        node->flags |= 0x1000;
                    }
                    node->node_index = node_index;
                    node->cell_x = static_cast<unsigned short>(neighbor_x);
                    node->cell_z = static_cast<unsigned short>(neighbor_z);
                    node->path_height = static_cast<unsigned short>(path_value & 0xffff);
                    node->parent_node = static_cast<unsigned short>(best_node);
                    node->position.Set((node->cell_x + g_float_005ebc7c) * grid_scale +
                                              level_bounds.minimum.x,
                                          (node->path_height - 1) * span +
                                              level_bounds.minimum.y,
                                          (node->cell_z + g_float_005ebc7c) * grid_scale +
                                              level_bounds.minimum.z);
                    node->score = (node->position - position).Length();
                    if (node->score < g_path_search_visualization_limit) {
                        W8PathHeapEntry pending;
                        pending.node = node->node_index;
                        pending.priority = static_cast<unsigned int>(node->score);
                        heap->Insert(&pending);
                        path_heap->root_node = heap->entries[0].node;
                    } else {
                        --m_search_node_count;
                    }
                }
                path_slot = paths->FindNextEntry(&key, path_slot);
            }
        }

        path_heap->DeleteRoot(current);
        best_node = path_heap->root_node;
        if (best_node > m_search_node_count) {
            char message[80];
            sprintf(message, "A*, Invalid node index %d from Queue", best_node);
            srAssertFail("(ulBestNode <= m_ulSearchNodesUsed)", OCTPATH_CPP, 0x1110, message);
        }
    }
    BuildSearchVisualization();
}

/* Draw the bounded path-search trace in the editor mesh. Search node zero is
   the root, so every later node contributes one raised square at its stored
   position. Its flags select the diagnostic color used for all five vertices
   in that marker. */
// FUNCTION: WIZ8 0x0045CFD0
void W8PathingService::BuildSearchVisualization()
{
    const srVector3T<float> marker_offsets[5] = {
        srVector3T<float>(0.0f, 125.0f, 0.0f), srVector3T<float>(-62.5f, 0.0f, -62.5f),
        srVector3T<float>(-62.5f, 0.0f, 62.5f), srVector3T<float>(62.5f, 0.0f, 62.5f),
        srVector3T<float>(62.5f, 0.0f, -62.5f)};
    stMeshModel* model = static_cast<stMeshModel*>(m_pPathModelInstance->getModel());
    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    srVector3T<float>* vertices = model->getVertexLoc();
    model->getActivePolygonTable(1);
    srVector3i* polygons = model->getPolyVertex();
    /* 0x0045D3BD tests this bound signed even though m_search_node_count is
       stored unsigned. */
    int node_count = m_search_node_count;

    if (node_count > 2000) {
        node_count = 2000;
    }
    /* 0x0045D3AD decrements the bound and 0x0045D3BF tests it signed. */
    for (int node_index = 1; node_index < node_count; ++node_index) {
        W8PathSearchNode* node = &m_search_nodes[node_index];
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
        if ((node->flags & 0x800) != 0) {
            color = srVector3T<float>(0.0f, 0.0f, 0.0f);
        } else if ((node->flags & 4) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 1.0f);
        } else if ((node->flags & 2) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 1.0f);
        } else if ((node->flags & 0x100) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 0.0f);
        } else if ((node->flags & 0x2000) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 0.0f);
        } else if ((node->flags & 0x8000) != 0) {
            color = srVector3T<float>(0.0f, 1.0f, 1.0f);
        } else if ((node->flags & 0x1000) != 0) {
            color = srVector3T<float>(0.0f, 0.0f, 1.0f);
        } else {
            color = srVector3T<float>(0.0f, 1.0f, 0.0f);
        }

        for (int offset_index = 0; offset_index < 5; ++offset_index) {
            vertices[vertex_index + offset_index] =
                marker_offsets[offset_index] + node->position;
            colors[vertex_index + offset_index] = color;
        }
    }

    unsigned long* active_polygons = model->getActivePolygonTable(1);
    /* 0x0045D3BF tests this bound with JLE after computing it as
       4 * (node_count - 1), so it is a signed count and not an unsigned one. */
    long active_count = node_count > 1 ? (node_count - 1) * 4 : 0;
    for (long index = 0; index < active_count; ++index) {
        active_polygons[index] = index + 600;
    }
    model->setActivePolygonCount(active_count);
    model->setDirtyAll();
    model->flags &= ~W8_MESH_VERTEX_LIGHTING_DIRTY;
}

/* Select the editor color for one waypoint. Disabled surfaces are black; the
   two current selection slots take yellow and either green or red; every other
   surface is blue. */
// FUNCTION: WIZ8 0x0045d490
void W8PathingService::GetWaypointVisualizationColor(unsigned short waypoint,
                                                     srVector3T<float>* color)
{
    if ((m_pSurfaces[waypoint].flags & 0x20) != 0) {
        color->SetZero();
        return;
    }
    if (waypoint == start_waypoint) {
        color->x = 1.0f;
        color->y = 1.0f;
        color->z = 0.0f;
        return;
    }
    if (waypoint != destination_waypoint) {
        color->x = 0.0f;
        color->y = 0.0f;
        color->z = 1.0f;
        return;
    }
    if (path_direction_valid != 0) {
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
stModelInstance* W8PathingService::EnsurePathVisualization()
{
    const int polygon_count = 0x3138;
    const int vertex_count = 0x30d4;
    stMeshModel* model = new stMeshModel(polygon_count, vertex_count);
    int index;

    if (model == 0) {
        srAssertFail("pstMeshModel", OCTPATH_CPP, 0x11e9,
                     "CreateWayPointMesh::Read -- Could not create pstMeshModel.\n");
    }
    model->autoRelease();
    model->flags &= ~1U;
    model->setShader(*g_oct_mesh_default_shader, 0);
    model->setName("WayPoint Mesh");
    model->duplicate_on_reuse = 0;

    srVector3i* polygons = model->getPolyVertex();
    srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 1);
    srVector2T<float>* texture_coordinates = model->getVertexTexCoords(0, 0, 1);
    srPtr<srMaterialIFace>* materials = model->getVertexMaterial(0, srMeshModel::SIDE_FRONT, 1);
    unsigned long* shade_indices = model->getVertexShadeIndex(1);

    for (index = 0; index < polygon_count; ++index) {
        textures[index] = g_oct_mesh_default_texture;
    }
    for (index = 0; index < vertex_count; ++index) {
        texture_coordinates[index].SetZero();
        materials[index] = g_oct_mesh_default_material;
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
        colors[index].Set(1.0f, 0.0f, 0.0f);
    }
    for (index = 5; index < 500; ++index) {
        colors[index].Set(0.0, 0.0, 1.0);
    }

    m_pPathModelInstance = CreateModelInstance(model);
    if (m_pPathModelInstance == 0) {
        srAssertFail("m_pPathModelInstance", OCTPATH_CPP, 0x1226,
                     "CreateWayPointMesh -- Could not create pstModelInstance.\n");
    }
    m_pPathModelInstance->setName("WayPoint Mesh");
    m_pPathModelInstance->setExclusionMask(3);
    m_pPathModelInstance->setFlag(srNode::FLAG_TERMINATE);
    return m_pPathModelInstance;
}

/* Collect nearby waypoint surfaces and mark the subset directly visible from
   the editor position. The near query also admits every outgoing neighbor;
   the wider query contributes only its own surfaces. Candidates are deduped,
   sorted by integer distance, and span-tested nearest first. */
// FUNCTION: WIZ8 0x0045D880
short W8PathingService::CollectPathVisualization(const srVector3T<float>* position)
{
    unsigned short waypoints[500];
    unsigned long distances[500];
    unsigned short waypoint_count = 0;
    unsigned long* query_results = 0;
    int query_count;
    int index;

    if (start_waypoint == 0) {
        return 0;
    }

    m_visible_waypoints->ClearAll();
    m_collected_waypoints->ClearAll();
    if (destination_waypoint != 0 && path_direction_valid == 0) {
        m_visible_waypoints->Set(destination_waypoint);
        m_collected_waypoints->Set(destination_waypoint);
    }

    srVector3T<float> lower;
    srVector3T<float> upper;
    lower.Set(position->x - g_float_005ec35c, position->y - g_float_005ec2f8,
              position->z - g_float_005ec35c);
    upper.Set(position->x + g_float_005ec35c, position->y + g_float_005ec2f8,
              position->z + g_float_005ec35c);
    query_count =
        g_octree->QueryObjects(&query_results, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = static_cast<unsigned short>(query_results[index]);
        unsigned short edge_index;

        if (!m_collected_waypoints->Set(waypoint)) {
            waypoints[waypoint_count++] = waypoint;
        }
        edge_index = m_pSurfaces[waypoint].first_edge;
        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges[edge_index];
            unsigned short neighbor = edge->destination;

            if (!m_collected_waypoints->Set(neighbor)) {
                waypoints[waypoint_count++] = neighbor;
            }
            edge_index = edge->next;
        }
    }

    query_results = 0;
    lower.Set(position->x - g_float_005ec384, position->y - g_float_005ec35c,
              position->z - g_float_005ec384);
    upper.Set(position->x + g_float_005ec384, position->y + g_float_005ec35c,
              position->z + g_float_005ec384);
    query_count =
        g_octree->QueryObjects(&query_results, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = static_cast<unsigned short>(query_results[index]);
        if (!m_collected_waypoints->Set(waypoint)) {
            waypoints[waypoint_count++] = waypoint;
        }
    }

    if (waypoint_count > 1) {
        unsigned int sort_index;

        for (sort_index = 0; sort_index < waypoint_count; ++sort_index) {
            const srVector3T<float>* candidate =
                &m_pSurfaces[waypoints[sort_index]].position;
            distances[sort_index] =
                static_cast<unsigned int>(static_cast<int>((*position - *candidate).Length()));
        }

        QuickSortByKey(waypoints, distances, 0, static_cast<int>(waypoint_count) - 1);
    }

    for (unsigned int visible_index = 0; visible_index < waypoint_count; ++visible_index) {
        unsigned short waypoint = waypoints[visible_index];
        if (TestWaypointSpan(position, &m_pSurfaces[waypoint].position, 0, 0) != 0) {
            m_visible_waypoints->Set(waypoint);
        }
    }
    return start_waypoint;
}

/* Append one waypoint surface and keep every capacity-coupled side table sized
   to the same hundred-record block.

   Surface zero is reserved on the first allocation. Growth replaces the three
   BitArrays rather than preserving their bits, and recreates the shared
   unsigned-short scratch run. The new surface receives its index and position,
   is classified for the path-surface flag, and is registered in the octree's
   spatial object index as kind nine. */
// FUNCTION: WIZ8 0x0045ddb0
void W8PathingService::AddWaypoint(const srVector3T<float>* position)
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
            memcpy(new_surfaces, m_pSurfaces, m_ulNumWayPoints * sizeof(W8PathSurface));
            free(m_pSurfaces);
        }
        m_pSurfaces = new_surfaces;

        if (m_visible_waypoints != 0) {
            delete m_visible_waypoints;
        }
        m_visible_waypoints = new BitArray(capacity);
        if (m_rendered_waypoints != 0) {
            delete m_rendered_waypoints;
        }
        m_rendered_waypoints = new BitArray(capacity);
        if (m_collected_waypoints != 0) {
            delete m_collected_waypoints;
        }
        m_collected_waypoints = new BitArray(capacity);

        free(g_path_scratch);
        g_path_scratch = static_cast<unsigned short*>(malloc(capacity * sizeof(unsigned short)));
    }

    W8PathSurface* surface = &m_pSurfaces[m_ulNumWayPoints];
    srVector3T<int> point;

    surface->flags = 0x2000;
    surface->index = static_cast<unsigned short>(m_ulNumWayPoints);
    surface->position = *position;
    if ((ClassifyWaypoint(&surface->position) & 0x04000000) != 0) {
        surface->flags |= 0x40;
    }
    g_octree->WorldPositionToCell(position, &point);
    g_octree->object_registry->MoveObjectToCell(
        W8_OCTREE_KIND_WAYPOINT, static_cast<unsigned short>(m_ulNumWayPoints + 1), &point);
    ++m_ulNumWayPoints;
}

/* Interactive link builder for one waypoint: asks the editor for a direction
   mask and link flags, queries the waypoint octree kind inside a 100000/50000
   box around the waypoint, span-tests each candidate, sorts them nearest-first
   by truncated distance, then adds the links the accepted direction mask
   allows. The first unreachable candidate ends the walk (they sort last). */
// FUNCTION: WIZ8 0x0045e030
void W8PathingService::SetWaypointLinkFlags(unsigned short waypoint, unsigned int direction)
{
    unsigned long* objects = 0;
    unsigned int link_flags[2];
    unsigned int index;
    unsigned int count;
    unsigned int accepted;
    unsigned long* distances;
    W8PathSurface* surface;
    W8PathSurface* candidate;
    srVector3T<float> lower;
    srVector3T<float> upper;
    float distance;

    if (1 < m_ulNumWayPoints) {
        link_flags[0] = 0;
        link_flags[1] = 0;
        accepted = EditWaypointLinkFlags("SET FLAGS FOR ALL LINKS FOR THIS WAYPOINT", link_flags,
                                         direction);
        if (accepted != 0) {
            surface = m_pSurfaces + waypoint;
            lower.Set(surface->position.x - g_float_005ec388,
                      surface->position.y - g_float_005ec260,
                      surface->position.z - g_float_005ec388);
            upper.Set(surface->position.x + g_float_005ec388,
                      surface->position.y + g_float_005ec260,
                      surface->position.z + g_float_005ec388);
            count =
                g_octree->QueryObjects(&objects, &lower, &upper, W8_OCTREE_KIND_WAYPOINT, waypoint);
            if (count != 0) {
                distances = static_cast<unsigned long*>(malloc(count * sizeof(unsigned long)));
                for (index = 0; index < count; ++index) {
                    candidate = m_pSurfaces + objects[index];
                    if ((candidate->flags & 2) == 0) {
                        distance = (candidate->position - surface->position).Length();
                        if (TestWaypointSpan(&surface->position, &candidate->position, 0,
                                             0) != 0) {
                            distances[index] = static_cast<unsigned long>(distance);
                        } else {
                            distances[index] = 0xffffffff;
                        }
                    } else {
                        distances[index] = 0xffffffff;
                    }
                }
                QuickSortByKey(objects, distances, 0, static_cast<int>(count) - 1);
                for (index = 0; index < count; ++index) {
                    if (distances[index] == 0xffffffff) {
                        return;
                    }
                    if (HasDirectionalWaypointLink(
                            waypoint, static_cast<unsigned short>(objects[index])) == 0) {
                        if ((accepted & 1) != 0) {
                            AddWaypointLink(waypoint, static_cast<unsigned short>(objects[index]),
                                            link_flags[0]);
                        }
                        if ((accepted & 2) != 0) {
                            AddWaypointLink(static_cast<unsigned short>(objects[index]), waypoint,
                                            link_flags[1]);
                        }
                    }
                }
            }
        }
    }
}

/* Unlink and clear one edge record.

   The owning surface or predecessor edge is redirected to the removed edge's
   successor. The retail scans stop after the first owner is found, then clear
   the packed record and increment the service's free-record count. */
// FUNCTION: WIZ8 0x0045e360
void W8PathingService::RemoveWaypointLink(unsigned short edge_index)
{
    if (m_ulNumWayPoints > 2) {
        bool found = false;
        unsigned int index;
        for (index = 1; index < m_ulNumWayPoints && found == 0; ++index) {
            if (m_pSurfaces[index].first_edge == edge_index) {
                m_pSurfaces[index].first_edge = m_pEdges[edge_index].next;
                found = 1;
            }
        }

        for (index = 1; index < m_ulNumWayPtLinks && found == 0; ++index) {
            if (m_pEdges[index].next == edge_index) {
                m_pEdges[index].next = m_pEdges[edge_index].next;
                found = 1;
            }
        }

        memset(&m_pEdges[edge_index], 0, sizeof(W8PathEdge));
        ++m_removed_edge_count;
        MarkRendererReady();
        waypoints_dirty = 1;
    }
}

/* Choose the waypoint that best continues from the surface nearest source in
   the requested direction. Existing graph edges take precedence. When none
   are sufficiently aligned, probe twenty-five fixed steps forward and retain
   the best distinct surface found there. The second selection is marked valid
   only when it is an existing edge or the direct span test accepts it. */
// FUNCTION: WIZ8 0x0045e840
unsigned char W8PathingService::PreparePathVisualization(const srVector3T<float>* source,
                                                         const srVector3T<float>* direction)
{
    float best_alignment = -1.0f;
    unsigned short best_waypoint = 0;
    unsigned short source_waypoint;
    W8PathSurface* source_surface;
    srVector3T<float> offset;

    destination_waypoint = 0;
    path_direction_valid = 0;
    source_waypoint = FindWaypoint(source, 0);
    source_surface = &m_pSurfaces[source_waypoint];

    offset = source_surface->position - *source;
    if (offset.Length() <= g_double_005ec030) {
        unsigned short edge_index = source_surface->first_edge;

        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges[edge_index];
            unsigned short neighbor_index = edge->destination;
            W8PathSurface* neighbor = &m_pSurfaces[neighbor_index];
            srVector3T<float> neighbor_direction;
            float alignment;

            neighbor_direction = neighbor->position - source_surface->position;
            neighbor_direction.Normalize();
            alignment = DotProduct(neighbor_direction, *direction);
            if (alignment > best_alignment) {
                best_alignment = alignment;
                best_waypoint = neighbor_index;
            }
            edge_index = edge->next;
        }

        if (best_alignment > g_float_005ec38c) {
            destination_waypoint = best_waypoint;
            path_direction_valid = 1;
            start_waypoint = source_waypoint;
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
            probe = source_surface->position + *direction * distance;
            probe_waypoint = FindWaypoint(&probe, 0);
            if (probe_waypoint != 0 && probe_waypoint != source_waypoint) {
                W8PathSurface* candidate = &m_pSurfaces[probe_waypoint];
                srVector3T<float> candidate_direction;
                float alignment;

                candidate_direction = candidate->position - source_surface->position;
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
            destination_waypoint = best_waypoint;
            if (TestWaypointSpan(&source_surface->position,
                                 &m_pSurfaces[best_waypoint].position, 0, 0) != 0) {
                path_direction_valid = 1;
            }
            start_waypoint = source_waypoint;
            return 1;
        }
    }

    start_waypoint = source_waypoint;
    return 0;
}

/* Add one directed edge to the waypoint graph, or update the matching edge
   when the source already owns it.

   Edge zero is the list sentinel and storage grows in hundred-record blocks.
   The cached length is computed before growth, the new record is appended to
   the source surface's chain, and the geometry-derived flag follows the same
   endpoint/span tests as an updated edge. */
// FUNCTION: WIZ8 0x0045ec30
void W8PathingService::AddWaypointLink(unsigned short source, unsigned short destination,
                                       unsigned int flags)
{
    W8PathSurface* source_surface;
    W8PathSurface* destination_surface;
    float distance;
    W8PathEdge* edge;

    if (source == 0 || destination == 0 || source == destination) {
        ShowNoticef(0xf, L"Cannot Link: Tried to link WayPt %d to WayPt %d. ", source, destination);
        return;
    }

    source_surface = &m_pSurfaces[source];
    destination_surface = &m_pSurfaces[destination];
    if ((IsZeroVector(&source_surface->position) != 0) ||
        (IsZeroVector(&destination_surface->position) != 0)) {
        ShowNoticef(0xf, L"Cannot Link: WayPt %d is at (0, 0, 0). ", source);
        return;
    }

    if (UpdateWaypointLink(source, destination, flags) != 0) {
        return;
    }

    distance = (source_surface->position - destination_surface->position).Length();

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
            memcpy(new_edges, m_pEdges, m_ulNumWayPtLinks * sizeof(W8PathEdge));
            free(m_pEdges);
        }
        m_pEdges = new_edges;
    }

    edge = &m_pEdges[m_ulNumWayPtLinks];
    edge->flags = flags;
    edge->destination = destination;
    edge->source = source;
    edge->distance = distance;
    edge->next = 0;

    if (source_surface->first_edge == 0) {
        source_surface->first_edge = static_cast<unsigned short>(m_ulNumWayPtLinks);
    } else {
        unsigned short previous = source_surface->first_edge;

        while (m_pEdges[previous].next != 0) {
            previous = m_pEdges[previous].next;
        }
        m_pEdges[previous].next = static_cast<unsigned short>(m_ulNumWayPtLinks);
    }

    if ((source_surface->flags & 0x40) != 0 || (destination_surface->flags & 0x40) != 0 ||
        (TestWaypointSpan(&source_surface->position, &destination_surface->position, 0, 0),
         span_blocked != 0)) {
        edge->flags |= 0x20000000;
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
unsigned char W8PathingService::HasDirectionalWaypointLink(unsigned short source,
                                                           unsigned short destination)
{
    W8PathSurface* source_surface = &m_pSurfaces[source];
    const W8PathSurface* destination_surface = &m_pSurfaces[destination];
    srVector3T<float> destination_direction;
    float destination_distance;
    unsigned short edge_index;

    destination_direction = destination_surface->position - source_surface->position;
    destination_distance = destination_direction.Length();
    srVector2T<float> destination_horizontal = destination_direction.xz();
    destination_horizontal.Normalize();
    destination_direction.Set(destination_horizontal.x, 0.0f, destination_horizontal.y);

    edge_index = source_surface->first_edge;
    while (edge_index != 0) {
        W8PathEdge* edge = &m_pEdges[edge_index];
        unsigned short neighbor_index = edge->destination;
        W8PathSurface* neighbor = &m_pSurfaces[neighbor_index];
        srVector3T<float> neighbor_direction;
        float neighbor_distance;

        if (neighbor_index == destination) {
            return 1;
        }

        neighbor_direction = neighbor->position - source_surface->position;
        neighbor_distance = neighbor_direction.Length();
        if (neighbor_distance < destination_distance) {
            unsigned short second_edge_index;
            srVector2T<float> neighbor_horizontal = neighbor_direction.xz();

            neighbor_horizontal.Normalize();
            neighbor_direction.Set(neighbor_horizontal.x, 0.0f, neighbor_horizontal.y);

            second_edge_index = neighbor->first_edge;
            while (second_edge_index != 0) {
                if (m_pEdges[second_edge_index].destination == destination) {
                    break;
                }
                second_edge_index = m_pEdges[second_edge_index].next;
            }
            if (second_edge_index != 0 &&
                DotProduct(neighbor_direction, destination_direction) > g_float_005ec390) {
                return 1;
            }
            if (DotProduct(neighbor_direction, destination_direction) > g_float_005ec38c) {
                return 1;
            }
        }
        edge_index = edge->next;
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
unsigned char W8PathingService::UpdateWaypointLink(unsigned short source,
                                                   unsigned short destination, unsigned int flags)
{
    unsigned short edge_index = m_pSurfaces[source].first_edge;

    while (edge_index != 0) {
        W8PathEdge* edge = &m_pEdges[edge_index];

        if (edge->destination == destination) {
            edge->flags = flags;
            if ((m_pSurfaces[source].flags & 0x40) != 0 ||
                (m_pSurfaces[destination].flags & 0x40) != 0 ||
                (TestWaypointSpan(&m_pSurfaces[source].position,
                                  &m_pSurfaces[destination].position, 0, 0),
                 span_blocked != 0)) {
                m_pEdges[m_ulNumWayPtLinks].flags |= 0x20000000;
            }
            return 1;
        }
        edge_index = edge->next;
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
    bool both_existing = true;

    if (m_waypoint_editing == 0) {
        return;
    }

    destination_index = FindWaypoint(destination, 0);
    if ((m_pSurfaces[destination_index].flags & 2) == 0 ||
        (m_pSurfaces[destination_index].position - *destination).Length() >
            g_double_005ec150) {
        destination_index = 0;
    }

    source_index = FindWaypoint(source, 0);
    if ((m_pSurfaces[source_index].flags & 2) == 0 ||
        (m_pSurfaces[source_index].position - *source).Length() > g_double_005ec150) {
        source_index = 0;
    }

    if (destination_index == 0) {
        destination_index = static_cast<unsigned short>(m_ulNumWayPoints);
        AddWaypoint(destination);
        m_pSurfaces[destination_index].flags = 2;
        SetWaypointLinkFlags(destination_index, 6);
        both_existing = false;
    }
    if (source_index == 0) {
        source_index = static_cast<unsigned short>(m_ulNumWayPoints);
        AddWaypoint(source);
        m_pSurfaces[source_index].flags = 2;
        SetWaypointLinkFlags(source_index, 5);
        both_existing = false;
    }

    edge_index = 0;
    link_flags[0] = 0;
    link_flags[1] = 0;
    if (both_existing != 0) {
        edge_index = m_pSurfaces[destination_index].first_edge;
        while (edge_index != 0) {
            if (m_pEdges[edge_index].destination == source_index) {
                link_flags[0] = m_pEdges[edge_index].flags;
                break;
            }
            edge_index = m_pEdges[edge_index].next;
        }
    }

    if (edge_index != 0) {
        sprintf(title, "EDIT FLAGS FOR EXISTING LINK BETWEEN TELEPORTAL WAYPOINTS: ");
    } else {
        sprintf(title, "EDIT FLAGS FOR NEW LINK BETWEEN TELEPORTAL WAYPOINTS: ");
    }
    EditWaypointLinkFlags(title, link_flags, 5);

    if (edge_index == 0) {
        edge_index = static_cast<unsigned short>(m_ulNumWayPtLinks);
        AddWaypointLink(destination_index, source_index, link_flags[0]);
    } else {
        m_pEdges[edge_index].flags = link_flags[0];
    }
    m_pEdges[edge_index].flags |= 0x01000000;
    m_pEdges[edge_index].distance = 0.0f;
    MarkRendererReady();
    waypoints_dirty = 1;
}

/* Modal waypoint-link flag editor: renders the link's direction, movement,
   size, nav-group, collision and door bits over nine heap-allocated text/wide
   line buffers and services a keyboard loop. 'C' and 'T' toggle the collide
   and through-door bits, 'D'/'G'/'M'/'S' arm a pending second-key prompt for
   the direction/group/movement/size bits, ENTER commits and ESC cancels. Only
   the pending-'D' direction keys actually change state; the 'G' second key
   computes a bit the retail code drops, and the 'M'/'S' second keys are dead.
   Returns the accepted direction mask, zero on cancel. */
// FUNCTION: WIZ8 0x0045f530
unsigned int W8PathingService::EditWaypointLinkFlags(const char* title, unsigned int* flags,
                                                     unsigned int direction)
{
    EnvironmentColour colour_saved;
    EnvironmentColour colour_backup;
    InputAtom atom;
    MSG message;
    char* lines[9];
    unsigned short* wide[9];
    char groups[32];
    unsigned int current;
    unsigned int link_flags;
    unsigned int index;
    unsigned int line;
    int letter;
    int key;
    char pending = 0;
    short length;

    colour_saved.SetZero();
    colour_backup.SetZero();
    GetWorldColour(&colour_backup);
    PublishLightDirection(&colour_saved);
    SetFont(g_smfnt_font);
    SetRGBFontShadow(0, 0, 0);
    SetFontObjectPalette16BPP(g_smfnt_font, g_font_state_palettes[5]);
    if (direction == 0 || (path_flags1 & 2) != 0) {
        direction = 3;
    }
    if (flags[0] == 0) {
        flags[0] = 0x3fffff;
    }
    if (flags[1] == 0) {
        flags[1] = 0x3fffff;
    }
    link_flags = (((direction & 3) == 2) ? flags[1] : flags[0]) & 0x12000000;
    for (index = 0; index < 9; ++index) {
        lines[index] = new char[0x100];
        memset(lines[index], 0, 0x100 * sizeof(char));
        wide[index] = new unsigned short[0x100];
        memset(wide[index], 0, 0x100 * sizeof(unsigned short));
        groups[index] = 0;
    }
    if (title != 0) {
        strcpy(lines[0], title);
    } else {
        sprintf(lines[0], "EDIT NEW WAYPOINT LINK FLAGS:");
    }
    sprintf(lines[8], "ESC to Cancel, ENTER to Accept");
    for (;;) {
        current = ((direction & 1) != 0) ? flags[0] : flags[1];
        sprintf(lines[2], " (D)irections: ");
        if ((direction & 3) == 3) {
            strcat(lines[2], "2 way link             ");
        } else if ((direction & 3) == 1) {
            strcat(lines[2], "1 way link outward     ");
        } else {
            strcat(lines[2], "1 way link inward      ");
        }
        if ((direction & 4) != 0) {
            strcat(lines[2], "(FIXED)");
        }
        sprintf(lines[3], " (M)ovement: ");
        if ((current & 0x70000) == 0x70000) {
            strcat(lines[3], "All types              ");
        } else {
            if ((current & 0x10000) != 0) {
                strcat(lines[3], "Walk ");
            }
            if ((current & 0x20000) != 0) {
                strcat(lines[3], "Fly ");
            }
            if ((current & 0x40000) != 0) {
                strcat(lines[3], "Swim ");
            }
        }
        sprintf(lines[4], " (S)ize of Monster: ");
        if ((current & 0x380000) == 0x380000) {
            strcat(lines[4], "All sizes");
        } else {
            if ((current & 0x80000) != 0) {
                strcat(lines[4], "Tiny ");
            }
            if ((current & 0x100000) != 0) {
                strcat(lines[4], "Small ");
            }
            if ((current & 0x200000) != 0) {
                strcat(lines[4], "Medium ");
            }
            if ((current & 0x400000) != 0) {
                strcat(lines[4], "Large ");
            }
            if ((current & 0x800000) != 0) {
                strcat(lines[4], "Huge ");
            }
        }
        sprintf(lines[5], " (G)roup: ");
        if ((current & 0xffff) == 0xffff) {
            strcat(lines[5], "All Nav Groups");
        } else {
            sprintf(groups, "[                   ] (A to P)");
            letter = 0;
            for (index = 0; index < 16; ++index) {
                if ((index % 4) == 0) {
                    ++letter;
                }
                if ((current & (1 << index)) != 0) {
                    groups[letter] = 'A' + index;
                }
                ++letter;
            }
            strcat(lines[5], groups);
        }
        sprintf(lines[6], " (C)ollide with Geometry: ");
        strcat(lines[6], (current & 0x2000000) != 0 ? "No" : "Yes");
        sprintf(lines[7], " (T)hrough Door: ");
        strcat(lines[7], (current & 0x10000000) != 0 ? "Yes" : "No");
        ClearSurfaceRect(0x1e, 0x64, 0x262, 0xcc);
        for (line = 0; line < 9; ++line) {
            for (length = 0; lines[line][length] != '\0' && length < 0x50; ++length) {
                wide[line][length] = static_cast<short>(lines[line][length]);
            }
            while (length < 0x50) {
                wide[line][length] = 0x20;
                ++length;
            }
            wide[line][length] = 0;
            gprintfDirty(0x1f, 0x65 + line * 0xd, Wiz8ToSgpWideText(g_format_s), wide[line]);
        }
        InvalidateRegion(0x1e, 0x64, 0x262, 0xcc, 4);
        if (DequeueEvent(&atom) == 0) {
            do {
                RenderFrame();
                RenderFrame();
                WaitMessage();
                if (PeekMessageA(&message, (HWND)0, 0, 0, 0) != 0 &&
                    GetMessageA(&message, (HWND)0, 0, 0) != 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
            } while (DequeueEvent(&atom) == 0);
        }
        if (atom.usEvent != 1) {
            continue;
        }
        if (atom.usParam == 0x1b) {
            if (pending == 0) {
                direction = 0;
                break;
            }
            sprintf(lines[1], "                                             ");
            pending = 0;
            continue;
        }
        if (atom.usParam == 0x0d) {
            if (pending == 0) {
                break;
            }
            pending = 0;
            continue;
        }
        if (pending != 0) {
            key = toupper(atom.usParam);
            switch (pending) {
            case 'D':
                if (key == 'B') {
                    direction |= 3;
                } else if (key == 'O') {
                    direction = direction & ~2;
                } else if (key == 'I') {
                    direction = direction & ~1;
                }
                break;
            case 'M':
                break;
            case 'S':
                if (0 <= key - 'A' && key - 'A' <= 0x13) {
                    switch (key) {
                    case 'A':
                    case 'H':
                    case 'K':
                    case 'L':
                    case 'R':
                    case 'S':
                    case 'T':
                        break;
                    case 'B':
                    case 'C':
                    case 'D':
                    case 'E':
                    case 'F':
                    case 'G':
                    case 'I':
                    case 'J':
                    case 'M':
                    case 'N':
                    case 'O':
                    case 'P':
                    case 'Q':
                        break;
                    }
                }
                break;
            case 'G':
                if ('A' <= key && key <= 'P') {
                    /* Retail computes 1 << (pending - 'A') here and drops the
                       result; nothing consumes it. */
                }
                break;
            case 'C':
                if (key != 'Y') {
                    pending = 0;
                }
                break;
            }
        } else {
            switch (toupper(atom.usParam)) {
            case 'D':
                if ((path_flags1 & 2) != 0) {
                    sprintf(lines[1], " Can only set flags in both directions in this link mode");
                } else if ((direction & 4) == 0) {
                    sprintf(lines[1], " (B)oth, (O)utward, or (I)nward");
                    pending = 'D';
                }
                break;
            case 'M':
                sprintf(lines[1], " Toggle (W)alking, (F)lying, or (S)wimming");
                pending = 'M';
                break;
            case 'S':
                sprintf(lines[1], " (A)ll (T)iny (S)mall (M)edium (L)arge (H)uge");
                pending = 'S';
                break;
            case 'G':
                sprintf(lines[1], " Key Toggle: Cap is On, Lowercase is Off");
                pending = 'G';
                break;
            case 'C':
                link_flags ^= 0x2000000;
                break;
            case 'T':
                link_flags ^= 0x10000000;
                break;
            default:
                lines[1][0] = 0;
                break;
            }
        }
    }
    ClearSurfaceRect(0x1e, 0x64, 0x262, 0xd9);
    for (line = 0; line < 9; ++line) {
        for (length = 0; length < 0x50; ++length) {
            wide[line][length] = 0x20;
        }
        wide[line][0x50] = 0;
        gprintfDirty(0x1f, 0x65 + line * 0xd, Wiz8ToSgpWideText(g_format_s), wide[line]);
    }
    InvalidateRegion(0x1e, 0x64, 0x262, 0xd9, 4);
    RenderFrame();
    RenderFrame();
    for (index = 0; index < 9; ++index) {
        delete[] lines[index];
        delete[] wide[index];
    }
    PublishLightDirection(&colour_backup);
    link_flags = (current & 0xedffffff) | link_flags;
    direction &= 3;
    if ((direction & 1) != 0) {
        flags[0] = link_flags;
    }
    if ((direction & 2) != 0) {
        flags[1] = link_flags;
    }
    path_flags1 |= 0x100;
    this->link_flags = link_flags;
    return direction;
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
                    height = (m_pulCondNodeValues[key_index] & 0xffff) * span +
                             level_bounds.minimum.y;
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

// GLOBAL: WIZ8 0x0060FA18
static W8PathParameter g_path_parameters[] = {
    {"ACCELERATION_FACTOR", &g_path_acceleration_factor},
    {"ANGULAR_ACCEL_FACTOR", &g_path_angular_acceleration_factor},
    {"ANGULAR_DECEL_FACTOR", &g_path_angular_deceleration_factor},
    {"PREDICTION_TIME", &g_path_prediction_time},
    {"APPROACH_SLOW_TIME", &g_path_approach_slow_time},
    {"APPROACH_RUN_TIME", &g_path_approach_run_time},
    {"APPROACH_RUN_RATE", &g_path_approach_run_rate},
    {"PATH_PREDICTION_TIME", &g_path_lookahead_time},
    {"GROUP_REPULSION_FACTOR", &g_path_group_repulsion_factor},
    {"PARTY_BOUNDARY_RADIUS", &g_path_party_boundary_radius},
    {"OBSTACLE_STEER_FACTOR", &g_path_obstacle_steering_factor},
    {"OBSTACLE_BRAKE_FACTOR", &g_path_obstacle_braking_factor},
    {"MIN_ANIMATION_RATE", &g_navigator_minimum_speed},
    {"FLY_SWIM_MIN_ANIM_RATE", &g_navigator_minimum_speed_mode23},
    {0, 0}};

// FUNCTION: WIZ8 0x004cae40
W8PathParameters::W8PathParameters()
{
    LoadPathParameters();
}

// FUNCTION: WIZ8 0x004cae50
void W8PathParameters::InitializeSteeringContext(W8NavigatorMovementState* movement)
{
    W8Navigator* linked;

    this->movement = movement;
    monster = GetMonsterByLocationID(movement->location_id);
    radius = monster->radius;
    velocity_length = movement->velocity.Length();
    linked = monster->linked_navigator;
    if (linked == 0) {
        speed_limit = movement->movement_scale * g_world_scale;
    } else {
        speed_limit = linked->movement.movement_scale * g_world_scale;
    }
    acceleration = g_path_acceleration_factor * speed_limit;
    if (IsZeroVector(&movement->velocity) != 0) {
        direction.Set(0.0, 0.0, 1.0);
        direction.RotateAboutY(sin(movement->yaw), cos(movement->yaw));
    } else {
        direction = movement->velocity;
        direction.Normalize();
    }
    perpendicular.Set(direction.z, 0.0, -direction.x);
    force.SetZero();
    blocked = false;
    nearby_queried = 0;
}

// FUNCTION: WIZ8 0x004cafc0
unsigned char W8PathParameters::QueryNearbyNavigators()
{
    srVector3T<float> lower;
    srVector3T<float> upper;
    float extent;

    if (nearby_queried != 0) {
        return nearby_count != 0;
    }
    extent = radius * g_float_005ebc28;
    nearby_queried = 1;
    lower.Set(movement->position.x - extent, movement->position.y - extent,
              movement->position.z - extent);
    upper.Set(extent + movement->position.x, extent + movement->position.y,
              extent + movement->position.z);
    nearby_locations = 0;
    nearby_count = g_octree->QueryLocationsInBox(&nearby_locations, &lower, &upper,
                                                    movement->location_id);
    return nearby_count != 0;
}

// FUNCTION: WIZ8 0x004cb090
void W8PathParameters::IntegrateSteering()
{
    float step;
    srVector3T<float> velocity;
    srVector3T<float> position;
    srVector3T<float> delta;
    srVector3T<float> slide;
    float scale;
    unsigned char snapped;
    char direction;

    step = g_rate * g_game_time_accumulator->GetFrameDelta();
    if (blocked == 0) {
        force.y = 0.0f;
        if (speed_limit <= g_float_zero) {
            velocity.SetZero();
            velocity_length = 0.0f;
            movement->target_yaw =
                NormalizeAngle(static_cast<float>(atan2(force.x, force.z)));
        } else {
            if (acceleration < force.Length()) {
                force.SetLength(acceleration);
            }
            velocity = movement->velocity + force * step;
            velocity_length = velocity.Length();
            if (speed_limit < velocity_length) {
                velocity.SetLength(speed_limit);
                velocity_length = speed_limit;
            }
            movement->target_yaw =
                NormalizeAngle(static_cast<float>(atan2(velocity.x, velocity.z)));
        }
        if (movement->target_yaw != movement->yaw) {
            UpdateYawSteering(step, 1);
            movement->yaw = movement->target_yaw;
            velocity.Set(0.0, 0.0, velocity_length);
            velocity.RotateAboutY(sin(movement->target_yaw), cos(movement->target_yaw));
        }
        position = movement->position + velocity * step;
        delta = position;
        snapped = g_octree->pathing->SnapWaypointPosition(&position, 0);
        if (snapped == 0) {
            delta = position - movement->position;
            slide = delta;
            direction = g_octree->pathing->GetNeighborSlideDirection(&movement->position,
                                                                         &delta, &slide);
            if (direction == 0) {
                blocked = 1;
            } else {
                scale = DotProduct(slide, delta);
                delta += slide * scale;
                position = movement->position + delta;
                snapped = g_octree->pathing->SnapWaypointPosition(&position, 0);
                if (snapped == 0) {
                    movement->target_yaw =
                        NormalizeAngle(static_cast<float>(atan2(slide.x, slide.z)));
                    position = movement->position;
                    UpdateYawSteering(step, 0);
                    movement->yaw = movement->target_yaw;
                }
            }
        }
        if (blocked == 0) {
            movement->velocity = velocity;
            movement->position = position;
            return;
        }
    }
    velocity.SetZero();
    position = movement->position;
    g_octree->pathing->FindPathCell(&position, 0, 1);
    movement->velocity = velocity;
    movement->position = position;
}

// FUNCTION: WIZ8 0x004cb520
void W8PathParameters::UpdateYawSteering(float time_step, bool use_turn_rate)
{
    float remaining;
    float rate;
    float direction;
    float angular_velocity;

    remaining = NormalizeAngle(movement->yaw - movement->target_yaw);
    rate = NormalizeAngle(movement->target_yaw - movement->yaw);
    direction = g_negative_one;
    if (rate < remaining) {
        remaining = rate;
        direction = g_float_one;
    }
    if (use_turn_rate == 0) {
        rate = movement->turn_rate;
        angular_velocity = 0.0f;
    } else {
        angular_velocity = g_path_angular_acceleration_factor * movement->turn_rate *
                               direction * time_step +
                           movement->yaw_velocity;
        rate = static_cast<float>(fabs(angular_velocity));
    }
    if (remaining <= rate * time_step) {
        movement->yaw_velocity = g_path_angular_deceleration_factor * angular_velocity;
        return;
    }
    movement->target_yaw = NormalizeAngle(rate * time_step * direction + movement->yaw);
    movement->yaw_velocity = angular_velocity;
}

// FUNCTION: WIZ8 0x004cb620
unsigned char W8PathParameters::PredictNavigatorCollision()
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
    if (this->monster->active == 0) {
        return 0;
    }
    if (velocity_length == g_double_zero) {
        return 0;
    }
    lookahead = g_path_prediction_time * velocity_length + radius;
    nearest = lookahead + g_camera_snap_epsilon;
    if (QueryNearbyNavigators() != 0) {
        for (index = 0; index < nearby_count; ++index) {
            monster = GetMonsterByLocationID(nearby_locations[index]);
            if (monster == 0) {
                navigator = 0;
            } else {
                navigator = monster;
            }
            if (this->monster->IsLinkedToNavigator(navigator) != 0) {
                continue;
            }
            navigator = monster;
            position = navigator->GetPosition();
            delta = position - movement->position;
            if (g_float_zero < DotProduct(delta, direction)) {
                navigator->GetVelocity(&other_velocity);
                relative = movement->velocity - other_velocity;
                relative.Normalize();
                approach = DotProduct(delta, relative);
                approach = approach - (monster->radius * approach) / delta.Length();
                if (g_float_zero < approach && approach < nearest) {
                    lateral = relative.z * delta.x + -relative.x * delta.z;
                    combined = monster->radius + radius;
                    if (static_cast<float>(fabs(lateral)) < combined) {
                        scale = lateral / combined;
                        found = true;
                        nearest = approach;
                    }
                }
            }
        }
    }
    if (movement->boundary_enabled != 0) {
        position = g_startup_world->GetPosition();
        delta = position - movement->position;
        combined = g_path_party_boundary_radius * g_world_scale;
        approach = radius * g_float_005ebc28 + combined;
        if (approach * approach > delta.LengthSquared()) {
            approach = DotProduct(delta, direction);
            if (approach > g_float_zero) {
                relative = movement->velocity - g_level_data->camera_forward;
                relative.Normalize();
                approach = DotProduct(delta, relative);
                approach = approach - (combined * approach) / delta.Length();
                if (approach > g_float_zero && approach < nearest) {
                    lateral = delta.x * relative.z + delta.z * -relative.x;
                    combined += radius;
                    if (static_cast<float>(fabs(lateral)) < combined) {
                        scale = lateral / combined;
                        found = true;
                        nearest = approach;
                    }
                }
            }
        }
    }
    if (found == 0) {
        return 0;
    }
    if (scale >= g_float_zero) {
        steer = scale - g_float_one;
    } else {
        steer = scale + g_float_one;
    }
    steer = (g_float_one - nearest / lookahead) * acceleration * steer;
    brake = g_path_obstacle_braking_factor * steer;
    steer *= g_path_obstacle_steering_factor;
    force += perpendicular * steer;
    force += direction * -brake;
    return found;
}

// FUNCTION: WIZ8 0x004cbb70
unsigned char W8PathParameters::HandleObstacleAhead()
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

    if (velocity_length == g_float_zero) {
        if (g_octree->pathing->GetNeighborSlideDirection(&movement->position,
                                                             &direction, &escape) != 0) {
            movement->attachment->GetNextPosition(&waypoint);
            waypoint -= movement->position;
            waypoint.y = 0.0f;
            if ((escape.z * waypoint.x + waypoint.z * -escape.x) *
                    (escape.z * direction.x + -escape.x * direction.z) <
                g_double_zero) {
                speed_limit = 0.0f;
                waypoint.SetLength(acceleration);
                force += waypoint;
                return 1;
            }
        }
    } else {
        reach = g_path_prediction_time * velocity_length + radius;
        ahead = movement->position + direction * reach;
        if (g_octree->pathing->TestWaypointSpan(&movement->position, &ahead, 1, 1) ==
            0) {
            ahead -= movement->position;
            distance = ahead.Length();
            if (distance <= reach) {
                movement->attachment->GetNextPosition(&waypoint);
                waypoint -= movement->position;
                waypoint.y = 0.0f;
                waypoint.SetLength(1.0);
                if (g_double_zero <= DotProduct(waypoint, direction)) {
                    steer = (g_float_one - distance / reach) * acceleration;
                    brake = g_path_obstacle_braking_factor * steer;
                    steer = g_path_obstacle_steering_factor * steer;
                    if (g_octree->pathing->GetObstacleDirection(&direction, &escape) != 0) {
                        side = DotProduct(escape, perpendicular);
                        if (g_double_005ed2e0 <= fabs(side)) {
                            if (side < g_float_zero) {
                                steer = -steer;
                            }
                        } else {
                            escape.Normalize();
                            if (DotProduct(escape, perpendicular) < g_float_zero) {
                                steer = -steer;
                            }
                        }
                        force += perpendicular * steer;
                        force += direction * -brake;
                        return 1;
                    }
                    blocked = true;
                    return 1;
                }
                steer = acceleration;
                speed_limit = 0.0f;
                force += waypoint * steer;
                return 1;
            }
        } else if (g_world_scale < radius) {
            clearance = g_octree->pathing->CompareDirectionalClearance(
                &movement->position, &direction, radius);
            if (g_float_one < static_cast<float>(fabs(clearance))) {
                force += perpendicular * clearance;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004cc1a0
void W8PathParameters::AccumulateSeekForce()
{
    srVector3T<float> desired;
    srVector3T<float> seek;

    if (velocity_length == g_float_zero) {
        desired = target - movement->position;
        desired.y = 0.0f;
        desired.Normalize();
        if (DotProduct(desired, direction) < g_double_zero) {
            speed_limit = 0.0f;
            force += desired * acceleration;
            return;
        }
    }
    desired = target - movement->position;
    desired.y = 0.0f;
    desired.SetLength(speed_limit);
    seek = desired - movement->velocity;
    seek *= g_path_acceleration_factor;
    if (acceleration < seek.Length()) {
        seek.SetLength(acceleration);
    }
    force += seek;
}

// FUNCTION: WIZ8 0x004cc420
void W8PathParameters::SeekWithApproachSpeed()
{
    float distance;
    float scale;

    distance = (target - movement->position).Length();
    scale = distance / (g_path_approach_slow_time * speed_limit);
    if (g_float_one < scale) {
        distance /= g_path_approach_run_time * speed_limit;
        if (g_float_one < distance) {
            distance = g_float_one;
        }
        scale = (g_path_approach_run_rate - g_float_one) * distance + g_float_one;
    }
    speed_limit = scale * speed_limit;
    AccumulateSeekForce();
}

// FUNCTION: WIZ8 0x004cc4c0
void W8PathParameters::AccumulateGroupRepulsion()
{
    unsigned int index;
    W8Monster* monster;
    W8Navigator* navigator;
    srVector3T<float> position;
    srVector3T<float> delta;
    float distance;
    float combined;
    float falloff;

    if (QueryNearbyNavigators() != 0) {
        for (index = 0; index < nearby_count; ++index) {
            monster = GetMonsterByLocationID(nearby_locations[index]);
            if (monster == 0) {
                navigator = 0;
            } else {
                navigator = monster;
            }
            if (this->monster->IsLinkedToNavigator(navigator) == 0) {
                continue;
            }
            position = monster->GetPosition();
            delta = position - movement->position;
            distance = delta.Length();
            combined = (radius + radius + monster->radius) * g_float_005ed2e8;
            if (distance < combined * g_float_005ec52c) {
                falloff = g_float_one;
                if (combined < distance) {
                    falloff = combined / distance;
                }
                delta = delta * static_cast<float>(g_double_005ec2e8);
                if (delta.LengthSquared() != g_double_zero) {
                    delta.SetLength(g_path_group_repulsion_factor * acceleration * falloff *
                                    falloff);
                }
                force += delta;
            }
        }
    }
}

// FUNCTION: WIZ8 0x004cc680
unsigned char W8PathParameters::SteerAroundLeader(bool allow_path_fallback)
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

    leader = monster->linked_navigator;
    leader_radius = leader->radius;
    if (IsZeroVector(&leader->movement.velocity) != 0) {
        heading.Set(0.0, 0.0, 1.0);
        heading.RotateAboutY(sin(leader->movement.yaw), cos(leader->movement.yaw));
    } else {
        heading = leader->movement.velocity;
        heading.Normalize();
    }
    if (leader->movement.velocity.x != g_float_zero ||
        leader->movement.velocity.y != g_float_zero ||
        leader->movement.velocity.z != g_float_zero) {
        delta.x = movement->position.x - leader->movement.position.x;
        delta.z = movement->position.z - leader->movement.position.z;
        ahead = delta.x * heading.x + delta.z * heading.z +
                (movement->position.y - leader->movement.position.y) * heading.y;
        if (g_float_zero < ahead &&
            ahead < leader->movement.velocity.Length() * g_path_prediction_time) {
            lateral = heading.z * delta.x - heading.x * delta.z;
            if (fabs(lateral) < static_cast<double>(leader_radius) + radius) {
                side = g_double_005ebc30;
                if (lateral < g_double_zero) {
                    side = g_double_005ec2e8;
                }
                offset.Set(heading.z * side * g_double_005ec150, 0.0,
                           -heading.x * side * g_double_005ec150);
                target = movement->position + offset;
                speed_limit = g_path_approach_run_rate * speed_limit;
                AccumulateSeekForce();
                return 1;
            }
        }
    }
    if (allow_path_fallback != 0 &&
        static_cast<int>(movement->attachment->path_cursor) <
            static_cast<int>(leader->movement.attachment->path_cursor) +
                static_cast<int>(movement->attachment->follow_offset)) {
        delta = leader->movement.position - movement->position;
        distance = delta.Length() - leader_radius;
        if (g_path_approach_slow_time * speed_limit < distance) {
            distance /= g_path_approach_run_time * speed_limit;
            if (g_float_one < distance) {
                distance = g_float_one;
            }
            speed_limit =
                ((g_path_approach_run_rate - g_float_one) * distance + g_float_one) *
                speed_limit;
        }
        return 0;
    }
    offset = heading * (leader_radius * static_cast<float>(g_double_005ec2e8));
    target = leader->movement.position + offset;
    SeekWithApproachSpeed();
    return 1;
}

// FUNCTION: WIZ8 0x004ccad0
void W8PathParameters::SteerFromPathStart(W8NavigatorMovementState* movement, char alternate)
{
    InitializeSteeringContext(movement);
    movement->attachment->flags &= 0xffefffff;
    if (HandleObstacleAhead() == 0) {
        if (PredictNavigatorCollision() != 0) {
            movement->attachment->flags |= 0x100000;
        } else if (alternate == 0 && SteerAroundLeader(1) != 0) {
            AccumulateGroupRepulsion();
            IntegrateSteering();
            return;
        } else {
            movement->attachment->GetNextPosition(&target);
            AccumulateSeekForce();
        }
    }
    if (alternate == 0) {
        AccumulateGroupRepulsion();
    }
    IntegrateSteering();
}

// FUNCTION: WIZ8 0x004ccb60
unsigned char W8PathParameters::SteerAlongPath(W8NavigatorMovementState* movement, char alternate)
{
    srVector3T<float> ahead;
    unsigned char advanced;
    float reach;

    advanced = 0;
    InitializeSteeringContext(movement);
    movement->attachment->flags &= 0xffefffff;
    if (HandleObstacleAhead() == 0) {
        if (g_float_zero < velocity_length) {
            reach = g_path_prediction_time * velocity_length;
            ahead = this->movement->position + direction * reach;
            if (movement->attachment->CheckPredictedHopHeight(&ahead) == 0) {
                target = this->movement->position;
                advanced =
                    movement->attachment->AdvancePositionTowardWaypoint(&target, reach);
                AccumulateSeekForce();
                goto steered;
            }
        }
        if (PredictNavigatorCollision() != 0) {
            movement->attachment->flags |= 0x100000;
            goto steered;
        }
        if (alternate == 0 && SteerAroundLeader(1) != 0) {
            AccumulateGroupRepulsion();
            IntegrateSteering();
            return 0;
        }
        movement->attachment->GetNextPosition(&target);
        AccumulateSeekForce();
    }
steered:
    if (alternate == 0) {
        AccumulateGroupRepulsion();
    }
    IntegrateSteering();
    return advanced;
}

// FUNCTION: WIZ8 0x004cccb0
unsigned char LoadPathParameters()
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
            ReadTextLine(handle, line, sizeof(line), &more);
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

/* Second emission of the float RotateAboutY; the primary template lives in
   srMath.h and surrender_math.cpp holds the 0x438F90 copy. */
