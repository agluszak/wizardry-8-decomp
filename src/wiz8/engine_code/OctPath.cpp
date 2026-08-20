#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file.h"

#include "surrender/srNode.h"
#include "surrender/srModelInstance.h"
#include "FileMan.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Engine Code\OctPath.cpp. The unit is named by its own assertions, which
   place every one of these bodies in OctPath.cpp rather than in Octree.cpp
   where the octree's own loader lives. */

extern unsigned short g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
extern W8PathingService* g_pathing_00659c60;
extern W8Navigator* g_startup_world_659c0c;
extern unsigned char g_flag_00659c5c;
extern unsigned char g_flag_00689b32;
extern const float g_world_scale_005ebc40;
extern void ConstructPathState004CCCB0(void* state);
extern void* CreateOctPathIndex();
extern void* g_path_scratch_00659c64;
extern void RegisterPathSurface004B7730(unsigned int index, const int* point);
extern void RegisterPathVertex004B7830(
    unsigned int index, const int* point, const int* second);
extern const double g_path_waypoint_snap_distance_005ec150;
extern double g_double_005ec3a8;
extern double g_double_005ec3a0;
extern double g_double_005ec3b0;
extern float g_path_direction_threshold_0_005ec348;
extern float g_path_direction_threshold_1_005ec34c;
extern float g_path_direction_threshold_2_005ec350;
extern float g_path_direction_threshold_3_005ec354;
extern float g_path_cardinal_scale_005ec358;
extern float g_path_waypoint_query_vertical_005ec35c;
extern float g_path_waypoint_query_horizontal_005ec360;
extern float g_path_waypoint_exact_distance_005ebc64;
extern float g_float_005ebb34;
extern double g_double_005ebe80;
extern void Function58AAD0(int channel, const char* format, ...);
extern float Function4BE420(
    const srVector3T<float>* source,
    const srVector3T<float>* target);
extern float CalcRangeDistance(int range_category);
extern unsigned char Function51B3F0(int mode);
extern void Function497690(int channel, const char* message);
extern stModelInstance005EC7D0* CreateModelInstance0046F5C0(
    stMeshModel* model);
extern srShader g_path_shader_00652dc4;
extern srTextureIFace* g_path_texture_00652dc0;
extern srMaterialIFace* g_path_material_00652dbc;
extern void SortPathCandidates004677A0(
    unsigned short* waypoints,
    unsigned int* distances,
    int first,
    int last);
extern unsigned char g_flag_006081e4;
extern unsigned int g_path_visualization_cell_00659c6c;
extern float g_path_search_visualization_limit_005ec380;
extern float g_path_endpoint_scale_005ec1a4;

#define OCTPATH_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctPath.cpp"

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
    }
    else {
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
            Function497690(7, "WritePathNodes: Couldn't write Path Hash array.\n");
            return 0;
        }
    }

    if ((unsigned int)m_ulNumCondLookup < 2 ||
        (unsigned int)m_ulNumCondKeys < 2) {
        m_ulNumCondPaths = 0;
        m_ulNumCondLookup = 0;
        m_ulNumCondKeys = 0;
    }
    block[0] = m_ulNumCondPaths;
    block[1] = m_ulNumCondLookup;
    block[2] = m_ulNumCondKeys;
    block[3] = 0;
    success = FileWrite(handle, block, sizeof(block), 0);
    if (success == 0) {
        srAssertFail("fSuccess", OCTPATH_CPP, 0x8b2,
                     "WritePathNodes: Couldn't write Conditional Counts.\n");
    }

    if ((unsigned int)m_ulNumCondLookup > 1 &&
        (unsigned int)m_ulNumCondKeys > 1) {
        success = FileWrite(
            handle, m_pCondPaths, m_ulNumCondPaths * 0x44, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8b7,
                         "WritePathNodes: Couldn't write Conditional Prop array.\n");
        }
        success = FileWrite(
            handle, m_pCondLookup, m_ulNumCondLookup << 2, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8b9,
                         "WritePathNodes: Couldn't write Conditional Lookup array.\n");
        }
        success = FileWrite(
            handle, m_pCondFrames, m_ulNumCondLookup << 1, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8bb,
                         "WritePathNodes: Couldn't write Conditional Frame array.\n");
        }
        success = FileWrite(
            handle, m_pCondKeys, m_ulNumCondKeys << 2, 0);
        if (success == 0) {
            srAssertFail("fSuccess", OCTPATH_CPP, 0x8bd,
                         "WritePathNodes: Couldn't write Conditional Key array.\n");
        }
        success = FileWrite(
            handle, m_pCondValues, m_ulNumCondKeys << 2, 0);
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
unsigned char W8PathingService::SaveWaypointSnapshot00459400(
    unsigned char force)
{
    if (flag_1cc == 0 && force == 0) {
        return 0;
    }
    if (FileExists("cd-rom") != 0) {
        return 0;
    }

    BuildWaypointFileData0045E440();
    if (file_waypoints_050 != 0) {
        free(file_waypoints_050);
    }
    file_waypoints_050 = static_cast<W8FileWaypoint*>(
        malloc(m_ulNumSurfaces * sizeof(W8FileWaypoint)));
    if (file_waypoints_050 == 0) {
        srAssertFail("m_pFileWayPoints", OCTPATH_CPP, 0x968, 0);
    }

    unsigned int index;
    for (index = 0; index < m_ulNumSurfaces; ++index) {
        file_waypoints_050[index].flags_00 =
            m_pSurfaces_048[index].flags_00 & 0xffdf;
        file_waypoints_050[index].first_edge_02 =
            m_pSurfaces_048[index].first_edge_24;
        file_waypoints_050[index].position_04 =
            m_pSurfaces_048[index].position_04;
    }
    for (index = 0; index < m_ulNumEdges; ++index) {
        m_pEdges_04c[index].flags_00 &= 0x7fffffff;
    }

    unsigned char result = WriteWaypointFile00459540();
    free(file_waypoints_050);
    file_waypoints_050 = 0;
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

    if (m_ulNumSurfaces > 0xffff) {
        return 0;
    }
    unsigned int handle = FileOpen(path, 2, 0);
    if (handle == 0) {
        return 0;
    }
    if (m_ulNumSurfaces != 0 && file_waypoints_050 != 0) {
        result =
            FileWrite(handle, &version, sizeof(version), 0) |
            FileWrite(handle, &m_positional_008, sizeof(m_positional_008), 0) |
            FileWrite(handle, &m_ulNumSurfaces, sizeof(m_ulNumSurfaces), 0) |
            FileWrite(handle, &m_ulNumEdges, sizeof(m_ulNumEdges), 0) |
            FileWrite(handle, file_waypoints_050,
                      m_ulNumSurfaces * sizeof(W8FileWaypoint), 0) |
            FileWrite(handle, m_pEdges_04c,
                      m_ulNumEdges * sizeof(W8PathEdge), 0);
    }
    CloseVirtualFile(handle);
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
    if (size_004 == 0) return 0;
    sprintf(path, "%s.WPT", level_name);
    unsigned int handle = FileOpen(path, 1, 0);
    if (handle == 0) return 0;

    success = ReadVirtualFile(handle, &version, 4, 0) |
              ReadVirtualFile(handle, &m_positional_008, 4, 0) |
              ReadVirtualFile(handle, &m_ulNumSurfaces, 4, 0) |
              ReadVirtualFile(handle, &m_ulNumEdges, 4, 0);
    if (success == 0) {
        CloseVirtualFile(handle);
        return 0;
    }
    unsigned int surface_capacity = (m_ulNumSurfaces / 100 + 1) * 100;
    unsigned int edge_capacity = (m_ulNumEdges / 100 + 1) * 100;
    file_waypoints_050 = static_cast<W8FileWaypoint*>(
        malloc(m_ulNumSurfaces * sizeof(W8FileWaypoint)));
    m_pSurfaces_048 = static_cast<W8PathSurface*>(
        malloc(surface_capacity * sizeof(W8PathSurface)));
    m_pEdges_04c = static_cast<W8PathEdge*>(
        malloc(edge_capacity * sizeof(W8PathEdge)));
    if (m_pSurfaces_048 != 0)
        memset(m_pSurfaces_048, 0, surface_capacity * sizeof(W8PathSurface));
    if (m_pEdges_04c != 0)
        memset(m_pEdges_04c, 0, edge_capacity * sizeof(W8PathEdge));
    if (file_waypoints_050 == 0 || m_pSurfaces_048 == 0 || m_pEdges_04c == 0) {
        if (file_waypoints_050 != 0) free(file_waypoints_050);
        if (m_pSurfaces_048 != 0) free(m_pSurfaces_048);
        if (m_pEdges_04c != 0) free(m_pEdges_04c);
        CloseVirtualFile(handle);
        return 0;
    }

    success = ReadVirtualFile(handle, file_waypoints_050,
        m_ulNumSurfaces * sizeof(W8FileWaypoint), 0);
    if (success != 0) {
        if (version == 1) {
            for (unsigned int edge = 0; edge < m_ulNumEdges; ++edge) {
                W8PathEdge* item = &m_pEdges_04c[edge];
                success &= ReadVirtualFile(handle, &item->flags_00, 4, 0);
                success &= ReadVirtualFile(handle, &item->destination_06, 2, 0);
                success &= ReadVirtualFile(handle, &item->distance_08, 4, 0);
                success &= ReadVirtualFile(handle, &item->next_0c, 2, 0);
                item->source_04 = 0;
            }
        } else {
            success &= ReadVirtualFile(handle, m_pEdges_04c,
                m_ulNumEdges * sizeof(W8PathEdge), 0);
        }
    }
    if (success == 0) {
        free(file_waypoints_050);
        free(m_pSurfaces_048);
        free(m_pEdges_04c);
        CloseVirtualFile(handle);
        return 0;
    }

    if (path_heap_06c != 0) {
        if (path_heap_06c->heap_00 != 0) {
            if (path_heap_06c->heap_00->external_storage_04 == 0)
                ::operator delete(path_heap_06c->heap_00->entries_00);
            ::operator delete(path_heap_06c->heap_00);
        }
        ::operator delete(path_heap_06c);
    }
    unsigned int heap_capacity = surface_capacity;
    if (heap_capacity <= g_path_reserve_0060827a)
        heap_capacity = g_path_reserve_0060827a;
    path_heap_06c = static_cast<W8PathHeapHandle*>(::operator new(8));
    path_heap_06c->heap_00 = static_cast<W8PathHeap*>(::operator new(0x10));
    path_heap_06c->heap_00->entries_00 = static_cast<W8PathHeapEntry*>(
        ::operator new(heap_capacity * sizeof(W8PathHeapEntry)));
    if (path_heap_06c->heap_00->entries_00 == 0)
        srAssertFail("hlist", "..\\Engine Code\\Include\\stHeap.hpp", 0x79, 0);
    path_heap_06c->heap_00->external_storage_04 = 0;
    path_heap_06c->heap_00->capacity_08 = heap_capacity;
    path_heap_06c->heap_00->size_0c = 0;

    memset(m_pSurfaces_048, 0, m_ulNumSurfaces * sizeof(W8PathSurface));
    for (unsigned int surface = 1; surface < m_ulNumSurfaces; ++surface) {
        W8FileWaypoint* source = &file_waypoints_050[surface];
        W8PathSurface* destination = &m_pSurfaces_048[surface];
        destination->flags_00 = source->flags_00;
        destination->index_02 = (unsigned short)surface;
        destination->first_edge_24 = source->first_edge_02;
        destination->position_04 = source->position_04;
        int point[2];
        g_octree_6598a4->WorldPositionToCell00431440(&destination->position_04, point);
        g_octree_6598a4->object_registry->UpdateObjectCell00436B90(9, surface + 1, point);
        if ((destination->flags_00 & 0xf000) == 0)
            destination->flags_00 |= 0x2000;
    }
    visible_waypoints_058->SetSize(surface_capacity);
    rendered_waypoints_05c->SetSize(surface_capacity);
    collected_waypoints_060->SetSize(surface_capacity);
    CloseVirtualFile(handle);
    g_path_scratch_00659c64 = malloc(surface_capacity * sizeof(unsigned short));

    if (version <= 2) {
        unsigned int surface;
        for (surface = 1; surface < m_ulNumSurfaces; ++surface) {
            if ((ClassifyWaypoint00459C00(&m_pSurfaces_048[surface].position_04) &
                 0x04000000) != 0)
                m_pSurfaces_048[surface].flags_00 |= 0x40;
        }
        for (surface = 1; surface < m_ulNumSurfaces; ++surface) {
            unsigned short edge_index = m_pSurfaces_048[surface].first_edge_24;
            while (edge_index != 0) {
                W8PathEdge* edge = &m_pEdges_04c[edge_index];
                edge->source_04 = (unsigned short)surface;
                unsigned int destination_index = edge->destination_06;
                if ((m_pSurfaces_048[surface].flags_00 & 0x40) != 0 ||
                    (m_pSurfaces_048[destination_index].flags_00 & 0x40) != 0) {
                    edge->flags_00 |= 0x20000000;
                } else {
                    TestWaypointSpan0045A1B0(
                        &m_pSurfaces_048[surface].position_04,
                        &m_pSurfaces_048[destination_index].position_04, 0, 0);
                    if (flag_23c != 0) edge->flags_00 |= 0x20000000;
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
    unsigned short* surface_map = static_cast<unsigned short*>(
        malloc((m_ulNumSurfaces + 1) * sizeof(unsigned short)));
    memset(surface_map, 0, (m_ulNumSurfaces + 1) * sizeof(unsigned short));
    unsigned short* edge_map = static_cast<unsigned short*>(
        malloc((m_ulNumEdges + 1) * sizeof(unsigned short)));
    memset(edge_map, 0, (m_ulNumEdges + 1) * sizeof(unsigned short));

    unsigned int old_surface;
    for (old_surface = 1; old_surface < m_ulNumSurfaces; ++old_surface) {
        W8PathSurface* surface = &m_pSurfaces_048[old_surface];
        if (surface->index_02 == 0 || surface->first_edge_24 == 0) {
            g_octree_6598a4->object_registry->RemoveObjectCell00436DC0(
                9, old_surface + 1);
            if (surface->first_edge_24 == 0) {
                unsigned short removed = 0;
                unsigned int edge;
                for (edge = 1; edge < m_ulNumEdges; ++edge) {
                    if (m_pEdges_04c[edge].destination_06 == old_surface) {
                        RemoveWaypointLink0045E360((unsigned short)edge);
                        ++removed;
                    }
                }
                if (g_flag_00689b32 != 0) {
                    const char* message = removed == 0
                        ? "Deleting Isolated WayPt at:  %1f, %1f"
                        : "Deleting Dead End WayPt at:  %1f, %1f";
                    FormatDebugMessage(
                        0, message, (double)surface->position_04.x,
                        (double)surface->position_04.y);
                }
            }
        }
        else {
            surface_map[old_surface] = next_surface;
            if (old_surface != next_surface) {
                m_pSurfaces_048[next_surface] = *surface;
                m_pSurfaces_048[next_surface].index_02 = next_surface;
                g_octree_6598a4->object_registry->RemoveObjectCell00436DC0(
                    9, old_surface + 1);
                int point[2];
                g_octree_6598a4->WorldPositionToCell00431440(
                    &m_pSurfaces_048[next_surface].position_04, point);
                g_octree_6598a4->object_registry->UpdateObjectCell00436B90(
                    9, next_surface + 1, point);
            }
            ++next_surface;
        }
    }

    unsigned int old_edge;
    for (old_edge = 1; old_edge < m_ulNumEdges; ++old_edge) {
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

    memset(&m_pEdges_04c[next_edge], 0,
           (m_ulNumEdges - next_edge) * sizeof(W8PathEdge));
    memset(&m_pSurfaces_048[next_surface], 0,
           (m_ulNumSurfaces - next_surface) * sizeof(W8PathSurface));
    m_ulNumSurfaces = next_surface;
    m_ulNumEdges = next_edge;

    for (old_surface = 1; old_surface < m_ulNumSurfaces; ++old_surface) {
        m_pSurfaces_048[old_surface].first_edge_24 =
            edge_map[m_pSurfaces_048[old_surface].first_edge_24];
    }
    for (old_edge = 1; old_edge < m_ulNumEdges; ++old_edge) {
        m_pEdges_04c[old_edge].next_0c =
            edge_map[m_pEdges_04c[old_edge].next_0c];
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
void W8PathingService::SetConditionalPathFrame00457EA0(
    unsigned int path_handle,
    short frame)
{
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    unsigned int lookup_index = path_handle;
    unsigned char frame_missing = 1;

    while (m_pCondLookup[lookup_index] != 0 && frame_missing != 0) {
        if (m_pCondFrames[lookup_index] == frame) {
            frame_missing = 0;
        }
        ++lookup_index;
    }

    if (frame == -1) {
        lookup_index = path_handle;
        while (m_pCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                unsigned int current_value = FindConditionalPathValue00458970(
                    key, m_pCondValues[key_index]);
                if ((current_value & 0x10000000) == 0) {
                    RemoveEntry00438C90(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                    current_value |= 0x10000000;
                    InsertEntry0055DBB0(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
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
        while (m_pCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                current_value = FindConditionalPathValue00458970(
                    key, m_pCondValues[key_index]);
                if ((m_pCondValues[key_index] & 0x02000000) != 0 &&
                    (current_value & 0x10000000) != 0) {
                    RemoveEntry00438C90(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                    current_value &= 0xefffffff;
                    InsertEntry0055DBB0(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                }
                ++key_index;
            }
            ++lookup_index;
        }

        lookup_index = path_handle;
        while (m_pCondLookup[lookup_index] != 0) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                if ((m_pCondValues[key_index] & 0x02000000) == 0 &&
                    (current_value & 0x10000000) == 0) {
                    unsigned int key = m_pCondKeys[key_index];
                    RemoveEntry00438C90(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                    current_value |= 0x10000000;
                    InsertEntry0055DBB0(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                }
                ++key_index;
            }
            ++lookup_index;
        }
        return;
    }

    lookup_index = path_handle;
    while (m_pCondLookup[lookup_index] != 0) {
        if (m_pCondFrames[lookup_index] != frame) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                unsigned int current_value = FindConditionalPathValue00458970(
                    key, m_pCondValues[key_index]);
                if ((m_pCondValues[key_index] & 0x02000000) != 0) {
                    if ((current_value & 0x10000000) != 0) {
                        RemoveEntry00438C90(
                            index, &key,
                            reinterpret_cast<const int*>(&current_value));
                        m_pCondValues[key_index] &= 0xefffffff;
                        current_value &= 0xefffffff;
                        InsertEntry0055DBB0(
                            index, &key,
                            reinterpret_cast<const int*>(&current_value));
                    }
                }
                else if ((current_value & 0x10000000) == 0) {
                    RemoveEntry00438C90(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                    m_pCondValues[key_index] |= 0x10000000;
                    current_value |= 0x10000000;
                    InsertEntry0055DBB0(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }

    lookup_index = path_handle;
    while (m_pCondLookup[lookup_index] != 0) {
        if (m_pCondFrames[lookup_index] == frame) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                unsigned int current_value = FindConditionalPathValue00458970(
                    key, m_pCondValues[key_index]);
                if ((m_pCondValues[key_index] & 0x02000000) != 0) {
                    if ((current_value & 0x10000000) == 0) {
                        RemoveEntry00438C90(
                            index, &key,
                            reinterpret_cast<const int*>(&current_value));
                        current_value |= 0x10000000;
                        InsertEntry0055DBB0(
                            index, &key,
                            reinterpret_cast<const int*>(&current_value));
                    }
                }
                else if ((current_value & 0x10000000) != 0) {
                    RemoveEntry00438C90(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
                    current_value &= 0xefffffff;
                    InsertEntry0055DBB0(
                        index, &key,
                        reinterpret_cast<const int*>(&current_value));
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
unsigned int W8PathingService::FindConditionalPathValue00458970(
    unsigned int key,
    unsigned int value)
{
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    int slot = index->FindNextEntry00438D50(&key, -1);
    unsigned int found = 0;
    unsigned char searching = 1;
    while (slot >= 0 && searching != 0) {
        unsigned int candidate = static_cast<W8OctreeEntry*>(
            index->entries)[slot].value;
        if (((candidate ^ value) & 0xffff) == 0) {
            searching = 0;
            found = candidate;
        }
        slot = index->FindNextEntry00438D50(&key, slot);
    }
    return found;
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

    if (cursor < attachment->path_position_index_08) {
        unsigned short* pairs =
            attachment->path_values_50;
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

    if (attachment->unknown_06 < attachment->path_position_index_08) {
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

/* Reserve the next fixed-width planner node. Storage grows by fifty records,
   is cleared in full, and retains every node through the newly issued index. */
// FUNCTION: WIZ8 0x00465A00
unsigned short W8PathingService::AllocateSearchNode00465A00()
{
    unsigned int node_index = ++search_node_count_0cc;

    if (m_owned_0c8 != 0 && node_index < search_node_capacity_0d0) {
        return (unsigned short)node_index;
    }

    search_node_capacity_0d0 += 50;
    W8PathSearchNode* new_nodes = static_cast<W8PathSearchNode*>(
        ::operator new(search_node_capacity_0d0 * sizeof(W8PathSearchNode)));
    if (new_nodes == 0) {
        srAssertFail("pNewSearchNodes", OCTPATH_CPP, 0x2751, 0);
    }
    memset(
        new_nodes, 0,
        search_node_capacity_0d0 * sizeof(W8PathSearchNode));
    if (m_owned_0c8 != 0) {
        memcpy(
            new_nodes, m_owned_0c8,
            search_node_count_0cc * sizeof(W8PathSearchNode));
        ::operator delete(m_owned_0c8);
    }
    m_owned_0c8 = new_nodes;
    return (unsigned short)node_index;
}

/* Walk grid-sized steps from a position toward a search node. Every crossed
   cell must resolve through the visited-node index to a live, unblocked node
   whose recorded clearance exceeds the caller's limit. */
// FUNCTION: WIZ8 0x00465AF0
unsigned char W8PathingService::CanReachSearchNode00465AF0(
    const srVector3T<float>* position,
    unsigned short target_node,
    float clearance)
{
    W8PathSearchNode* target = &m_owned_0c8[target_node];
    float delta_x = target->position_20.x - position->x;
    float delta_z = target->position_20.z - position->z;
    float scale;
    if ((float)fabs(delta_x) > (float)fabs(delta_z)) {
        scale = (float)fabs(grid_scale_01c / delta_x);
    }
    else {
        scale = (float)fabs(grid_scale_01c / delta_z);
    }

    srVector3T<float> probe = *position;
    float step_x = delta_x * scale;
    float step_z = delta_z * scale;
    W8OctreeIndex* visited = static_cast<W8OctreeIndex*>(m_pIndex_074);
    unsigned char blocked = 0;

    while (1) {
        int cell_x = (int)((probe.x - level_bounds[0]) / grid_scale_01c);
        int cell_z = (int)((probe.z - level_bounds[2]) / grid_scale_01c);
        unsigned int key = cell_z * 0x10000 + cell_x;
        unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
        int slot = static_cast<int*>(visited->bucket_heads)[
            hash & (visited->bucket_count - 1)];
        unsigned int node_index = 0;
        W8OctreeEntry* entries =
            static_cast<W8OctreeEntry*>(visited->entries);
        while (slot != -1) {
            if (entries[slot].key == key) {
                node_index = (unsigned int)entries[slot].value;
                break;
            }
            slot = entries[slot].next_index;
        }

        if ((unsigned short)node_index == target_node || blocked != 0) {
            return blocked == 0;
        }
        if ((unsigned short)node_index == 0 ||
            (m_owned_0c8[node_index & 0xffff].flags_00 & 0x100) != 0 ||
            m_owned_0c8[node_index & 0xffff].clearance_18 <= clearance) {
            blocked = 1;
        }
        else {
            probe.x += step_x;
            probe.z += step_z;
        }
    }
}

/* Pull the last planned point onto the requested contact shell when it only
   overshoots that shell by less than half a path cell. The adjusted point is
   also made the attachment's current point and republished to the octree. */
// FUNCTION: WIZ8 0x00465D70
void W8PathingService::AdjustFinalPathEndpoint00465D70(
    W8NavigatorMovementState* movement,
    float radius,
    float separation)
{
    int target_location = movement->value_010;
    if (target_location < 0 || flag_09c != 0) {
        return;
    }

    float target_radius;
    srVector3T<float> target_position;
    if (target_location <= 0) {
        target_radius = g_startup_world_659c0c->
            fields.movement_0c0.alternate_radius_0b4;
        target_position = g_startup_world_659c0c->GetPosition();
    }
    else {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x27b0, OCTPATH_CPP, target_location, 1);
        W8MonsterInfo* info =
            MonsterGetScriptPartByLocationIndex(monster_index);
        if (info != 0 && info->monster != 0) {
            target_radius = info->monster->
                fields.movement_0c0.alternate_radius_0b4;
            target_position = info->monster->GetPosition();
        }
    }

    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    srVector3T<float>* endpoint =
        &attachment->position_4c[attachment->path_position_index_08];
    srVector3T<float> direction;
    direction.x = endpoint->x - target_position.x;
    direction.y = endpoint->y - target_position.y;
    direction.z = endpoint->z - target_position.z;
    float length_squared =
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z;
    float excess =
        (float)sqrt(length_squared) - (target_radius + radius);
    if (separation < excess &&
        excess - separation < grid_scale_01c * g_float_005ebc7c) {
        if ((double)length_squared != g_zero_005ebb40) {
            float scale =
                (target_radius + radius + separation) *
                g_path_endpoint_scale_005ec1a4 /
                (float)sqrt(length_squared);
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
        }

        srVector3T<float> adjusted;
        adjusted.x = target_position.x + direction.x;
        adjusted.y = target_position.y + direction.y;
        adjusted.z = target_position.z + direction.z;
        *endpoint = adjusted;
        attachment->position_1c = *endpoint;

        if ((attachment->flags_00 & 0x08000000) == 0) {
            attachment->flags_00 |= 0x02000000;
            attachment->position_40 = adjusted;
            g_octree_6598a4->QueueOctreeKind130042E810(
                movement->location_id_004, &adjusted);
        }
    }
}

/* Select one conditional frame for a GD prop's path cells. Entries belonging
   to every other frame first lose both prop-state bits. Entries belonging to
   the selected frame then gain the caller's state bits. The hash table may
   contain several values for one key, so the low word from the serialized
   conditional value is the identity used to find the exact pairing. */
// FUNCTION: WIZ8 0x00465fb0
void W8PathingService::UpdateConditionalPathFlags00465FB0(
    unsigned int path_handle,
    unsigned short frame,
    unsigned int flags)
{
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    unsigned int lookup_index = path_handle;

    while (m_pCondLookup[lookup_index] != 0) {
        if ((unsigned short)m_pCondFrames[lookup_index] != frame) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                unsigned int wanted_value = m_pCondValues[key_index];
                unsigned int current_value = 0;
                int slot = index->FindNextEntry00438D50(&key, -1);
                unsigned char searching = 1;
                while (slot >= 0 && searching != 0) {
                    unsigned int value = static_cast<W8OctreeEntry*>(
                        index->entries)[slot].value;
                    if (((value ^ wanted_value) & 0xffff) == 0) {
                        searching = 0;
                        current_value = value;
                    }
                    slot = index->FindNextEntry00438D50(&key, slot);
                }

                if ((current_value & 0x08000000) != 0) {
                    RemoveEntry00438C90(
                        index, &key, reinterpret_cast<const int*>(&current_value));
                    current_value &= 0xd7ffffff;
                    InsertEntry0055DBB0(
                        index, &key, reinterpret_cast<const int*>(&current_value));
                }
                ++key_index;
            }
        }
        ++lookup_index;
    }

    lookup_index = path_handle;
    while (m_pCondLookup[lookup_index] != 0) {
        if ((unsigned short)m_pCondFrames[lookup_index] == frame) {
            unsigned int key_index = m_pCondLookup[lookup_index];
            while (m_pCondKeys[key_index] != 0) {
                unsigned int key = m_pCondKeys[key_index];
                unsigned int wanted_value = m_pCondValues[key_index];
                unsigned int current_value = 0;
                int slot = index->FindNextEntry00438D50(&key, -1);
                unsigned char searching = 1;
                while (slot >= 0 && searching != 0) {
                    unsigned int value = static_cast<W8OctreeEntry*>(
                        index->entries)[slot].value;
                    if (((value ^ wanted_value) & 0xffff) == 0) {
                        searching = 0;
                        current_value = value;
                    }
                    slot = index->FindNextEntry00438D50(&key, slot);
                }

                if ((flags & current_value) == 0) {
                    RemoveEntry00438C90(
                        index, &key, reinterpret_cast<const int*>(&current_value));
                    current_value |= flags;
                    InsertEntry0055DBB0(
                        index, &key, reinterpret_cast<const int*>(&current_value));
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
int W8PathingService::ProcessSearchNodeProps004663D0(
    unsigned int node_index, unsigned char first_only)
{
    W8PathSearchNode* node = &m_owned_0c8[node_index & 0xffff];
    float half_cell = grid_scale_01c * g_float_005ebc7c;

    srVector3T<float> lower;
    lower.x = node->position_20.x - half_cell;
    lower.y = node->position_20.y;
    lower.z = node->position_20.z - half_cell;

    srVector3T<float> upper;
    upper.x = node->position_20.x + half_cell;
    upper.y = node->position_20.y + grid_scale_01c + grid_scale_01c;
    upper.z = node->position_20.z + half_cell;

    int* candidates = 0;
    unsigned int count = g_octree_6598a4->QueryObjects0042F280(
        &candidates, &lower, &upper, 8, -1);
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

/* Build a bounded grid route from the navigator's current position to its
   active attachment target. The open-chain index owns one search node per
   cell, the fixed-capacity minimum heap chooses the next node to expand, and
   the selected parent chain is collapsed into the attachment's route array. */
// FUNCTION: WIZ8 0x00463460
unsigned short W8PathingService::PlanMovement00463460(
    W8NavigatorMovementState* movement,
    float radius,
    float separation)
{
    float diagonal_step = grid_scale_01c * g_path_cardinal_scale_005ec358;
    unsigned short result = 0;
    unsigned char stop_search = 0;

    if (flag_0a4 == 0) {
        trace_offset_0ac.x = 0.0f;
        trace_offset_0ac.y = 500.0f;
        trace_offset_0ac.z = 0.0f;
        trace_mode_0b8 = 0;
        trace_height_offset_0bc = 500.0f;
    }

    search_node_count_0cc = 0;
    planner_location_090 = movement->location_id_004;
    memset(
        m_owned_0c8, 0,
        search_node_capacity_0d0 * sizeof(W8PathSearchNode));
    probe_cell_key_078 = 0;
    probe_limit_088 = (unsigned int)-1;
    path_heap_06c->heap_00->size_0c = 0;

    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    attachment->flags_00 &= 0xfffffff0;
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
    attachment->flags_00 &= 0xfdffffff;

    unsigned char allow_dynamic =
        (unsigned char)(movement->unknown_000 >> 28 & 1);
    srVector3T<float> start = movement->position_040;
    srVector3T<float> target;
    if (flag_09c != 0) {
        target = movement->target_position_04c;
    }
    else if ((attachment->flags_00 & 0x00080000) != 0) {
        target = attachment->position_28;
    }
    else if (attachment->value_04 < attachment->path_position_index_08) {
        target = attachment->position_4c[attachment->value_04];
    }
    else {
        target = attachment->position_1c;
    }

    float target_dx = target.x - start.x;
    float target_dy = target.y - start.y;
    float target_dz = target.z - start.z;
    float target_distance = (float)sqrt(
        target_dx * target_dx + target_dy * target_dy +
        target_dz * target_dz);
    float remaining_callback =
        movement->callback_threshold_058 - movement->callback_progress_05c;

    if ((attachment->flags_00 & 0x04000000) == 0) {
        float search_extent = target_distance;
        if (search_extent < remaining_callback) {
            search_extent = remaining_callback;
        }
        search_extent += g_path_limit_006081e8 + radius;
        srVector3T<float> lower;
        srVector3T<float> upper;
        lower.x = start.x - search_extent;
        lower.y = start.y - search_extent;
        lower.z = start.z - search_extent;
        upper.x = start.x + search_extent;
        upper.y = start.y + search_extent;
        upper.z = start.z + search_extent;
        CollectPathProbes004656A0(movement, radius);
        path_candidates_098 = 0;
        path_candidate_count_094 = OctreeTraverseKind12(
            &path_candidates_098, &lower, &upper,
            movement->location_id_004);
    }
    else {
        path_probe_count_0d4 = 0;
        if (movement->value_010 < 1) {
            path_candidate_count_094 = 0;
        }
        else {
            path_candidate_count_094 = 1;
            path_candidates_098 = &movement->value_010;
        }
    }

    int root_x = (int)((start.x - level_bounds[0]) / grid_scale_01c);
    int root_z = (int)((start.z - level_bounds[2]) / grid_scale_01c);
    unsigned short root_index = AllocateSearchNode00465A00();
    W8PathSearchNode* root = &m_owned_0c8[root_index];
    root->flags_00 = 0;
    root->node_index_02 = root_index;
    root->cell_x_04 = (unsigned short)root_x;
    root->cell_z_06 = (unsigned short)root_z;
    root->path_height_08 = (unsigned short)(
        (int)((start.y - level_bounds[1]) / span_020) + 1);
    root->parent_node_0a = 0;
    root->base_score_0c = 0.0f;
    root->path_cost_10 = 0.0f;
    root->distance_14 = target_distance;
    root->position_20 = start;

    unsigned int root_key = root_z * 0x10000 + root_x;
    if (visited->free_head == -1) {
        GrowIndex00439290(visited);
    }
    int root_slot = visited->free_head;
    W8OctreeEntry* visited_entries =
        static_cast<W8OctreeEntry*>(visited->entries);
    visited->free_head = visited_entries[root_slot].next_index;
    unsigned int root_hash =
        (root_key >> 10 ^ root_key) >> 10 ^ root_key;
    int* visited_buckets = static_cast<int*>(visited->bucket_heads);
    visited_entries[root_slot].key = root_key;
    visited_entries[root_slot].value = root_index;
    visited_entries[root_slot].next_index =
        visited_buckets[root_hash & (visited->bucket_count - 1)];
    visited_buckets[root_hash & (visited->bucket_count - 1)] = root_slot;

    unsigned int root_height = root->path_height_08;
    float root_clearance = radius;
    float root_vertical = 0.0f;
    unsigned char root_dynamic = 0;
    ResolvePathCell004648D0(
        root_key, 1, &root_height, &root_clearance,
        &root_vertical, &root_dynamic);
    UpdateSearchNodeScore00464FF0(
        root_index, &target, root_clearance, radius);
    unsigned int root_score = (unsigned int)root->score_1c;
    if (root_score < probe_limit_088) {
        probe_limit_088 = root_score;
        probe_cell_key_078 = root_index;
    }
    probe_cell_key_078 = 0;

    W8PathHeap* heap = path_heap_06c->heap_00;
    W8PathHeapEntry entry;
    entry.node_00 = root_index;
    entry.priority_04 = (unsigned int)root->score_1c;
    heap->Insert004675B0(&entry);
    path_heap_06c->root_node_04 = heap->entries_00[0].node_00;

    unsigned int best_node = path_heap_06c->root_node_04;
    while (best_node != 0 && stop_search == 0 &&
           search_node_count_0cc < g_path_reserve_0060827a) {
        W8PathSearchNode* current = &m_owned_0c8[best_node];
        unsigned short current_x = current->cell_x_04;
        unsigned short current_z = current->cell_z_06;
        unsigned int current_height = current->path_height_08;

        for (int direction = 0; direction < 8; ++direction) {
            unsigned int neighbor_x = current_x;
            unsigned int neighbor_z = current_z;
            if (direction >= 1 && direction <= 3) {
                ++neighbor_x;
            }
            else if (direction > 4) {
                --neighbor_x;
            }
            if (direction < 2 || direction > 6) {
                ++neighbor_z;
            }
            else if (direction > 2 && direction < 6) {
                --neighbor_z;
            }

            unsigned int key = neighbor_z * 0x10000 + neighbor_x;
            float step = (direction & 1) == 0
                ? grid_scale_01c : diagonal_step;
            float path_cost = current->path_cost_10 + step;
            float base_score = current->base_score_0c + step;

            visited_entries =
                static_cast<W8OctreeEntry*>(visited->entries);
            visited_buckets = static_cast<int*>(visited->bucket_heads);
            unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
            int slot = visited_buckets[hash & (visited->bucket_count - 1)];
            unsigned int existing_index = 0;
            while (slot != -1) {
                if (visited_entries[slot].key == key) {
                    existing_index =
                        (unsigned int)visited_entries[slot].value;
                    break;
                }
                slot = visited_entries[slot].next_index;
            }

            if (existing_index != 0) {
                W8PathSearchNode* existing =
                    &m_owned_0c8[existing_index & 0xffff];
                if ((existing->flags_00 & 0x0100) != 0) {
                    continue;
                }
                int height_delta =
                    (int)current->path_height_08 -
                    (int)existing->path_height_08;
                if (height_delta < 0) {
                    height_delta = -height_delta;
                }
                base_score +=
                    (float)height_delta * span_020 * g_float_005ec3b8;
                if (existing->base_score_0c <= base_score) {
                    continue;
                }
                existing->base_score_0c = base_score;
                existing->path_cost_10 = path_cost;
                existing->parent_node_0a = (unsigned short)best_node;
                UpdateSearchNodeScore00464FF0(
                    existing_index, &target,
                    existing->clearance_18, radius);
                if ((existing->flags_00 & 0x0400) != 0) {
                    existing->flags_00 &= 0xfbff;
                    entry.node_00 = existing->node_index_02;
                    entry.priority_04 = (unsigned int)existing->score_1c;
                    heap->Insert004675B0(&entry);
                    path_heap_06c->root_node_04 =
                        heap->entries_00[0].node_00;
                }
                continue;
            }

            unsigned int height = current_height;
            float clearance = radius;
            float vertical = base_score;
            unsigned char dynamic = 0;
            if (ResolvePathCell004648D0(
                    key, allow_dynamic, &height, &clearance,
                    &vertical, &dynamic) == 0) {
                continue;
            }

            unsigned short node_index = AllocateSearchNode00465A00();
            if (visited->free_head == -1) {
                GrowIndex00439290(visited);
            }
            visited_entries =
                static_cast<W8OctreeEntry*>(visited->entries);
            visited_buckets = static_cast<int*>(visited->bucket_heads);
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
            node->cell_x_04 = (unsigned short)neighbor_x;
            node->cell_z_06 = (unsigned short)neighbor_z;
            node->path_height_08 = (unsigned short)height;
            node->parent_node_0a = (unsigned short)best_node;
            node->base_score_0c = vertical;
            node->path_cost_10 = path_cost;
            node->clearance_18 = clearance;
            node->position_20.x =
                ((float)node->cell_x_04 + g_float_005ebc7c) *
                    grid_scale_01c + level_bounds[0];
            node->position_20.y =
                (float)(node->path_height_08 - 1) * span_020 +
                level_bounds[1];
            node->position_20.z =
                ((float)node->cell_z_06 + g_float_005ebc7c) *
                    grid_scale_01c + level_bounds[2];

            unsigned short collision = ResolveSearchNodeCollisions00465130(
                movement, node_index, radius, separation);
            UpdateSearchNodeScore00464FF0(
                node_index, &target, clearance, radius);
            if (collision == 1) {
                node->flags_00 |= 0x0100;
                continue;
            }
            if (collision == 3 &&
                (current->flags_00 & 0x0100) == 0) {
                stop_search = 1;
                result = 1;
                probe_cell_key_078 = node_index;
                continue;
            }
            if (collision == 2) {
                node->flags_00 |= 0x0100;
            }

            float node_dx = target.x - node->position_20.x;
            float node_dy = target.y - node->position_20.y;
            float node_dz = target.z - node->position_20.z;
            float distance = (float)sqrt(
                node_dx * node_dx + node_dy * node_dy +
                node_dz * node_dz);
            if (flag_09c == 0) {
                if (distance < diagonal_step || distance < separation) {
                    stop_search = 1;
                    result = 1;
                }
            }
            else if (separation < distance) {
                stop_search = 1;
                result = 1;
            }

            if (collision == 0) {
                unsigned int score = (unsigned int)node->score_1c;
                if (score < probe_limit_088) {
                    probe_limit_088 = score;
                    probe_cell_key_078 = node_index;
                }
            }
            entry.node_00 = node->node_index_02;
            entry.priority_04 = (unsigned int)node->score_1c;
            heap->Insert004675B0(&entry);
            path_heap_06c->root_node_04 = heap->entries_00[0].node_00;
        }

        if (heap->size_0c == 0) {
            path_heap_06c->root_node_04 = 0;
        }
        else {
            path_heap_06c->root_node_04 = heap->Delete().node_00;
        }
        current->flags_00 |= 4;
        best_node = path_heap_06c->root_node_04;
        if (best_node > search_node_count_0cc) {
            char message[80];
            sprintf(
                message, "A:  Invalid node index %d from Queue.",
                best_node);
            srAssertFail(
                "(ulBestNode <= m_ulSearchNodesUsed)",
                OCTPATH_CPP, 0x22ad, message);
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
        unsigned short walk = (unsigned short)probe_cell_key_078;
        unsigned short walk_parent = m_owned_0c8[walk].parent_node_0a;
        while (walk_parent != 0) {
            if (flag_09c == 0 && direct_visibility_node == 0) {
                srVector3T<float> trace_target = movement->target_position_04c;
                trace_target.y += trace_height_offset_0bc;
                float bearing = NormalizeAngle(Function4BE420(
                    &m_owned_0c8[walk].position_20, &trace_target));
                float target_yaw = NormalizeAngle(trace_target_yaw_0c4);
                srMatrix3T<float> rotation;
                rotation.SetIdentity00467310();
                float angle = bearing - target_yaw;
                if ((double)angle != g_zero_005ebb40) {
                    rotation.method_00438F90(sin(angle), cos(angle));
                }
                srVector3T<float> transformed;
                transformed.x = Function4218E0(
                    rotation.vectors[0], trace_offset_0ac);
                transformed.y = Function4218E0(
                    rotation.vectors[1], trace_offset_0ac);
                transformed.z = Function4218E0(
                    rotation.vectors[2], trace_offset_0ac);
                srVector3T<float> trace_source;
                trace_source.x = m_owned_0c8[walk].position_20.x + transformed.x;
                trace_source.y = m_owned_0c8[walk].position_20.y + transformed.y;
                trace_source.z = m_owned_0c8[walk].position_20.z + transformed.z;
                short trace = g_octree_6598a4->TraceLineOfSight(
                    &trace_source, &trace_target, 1, -3, -3, 1, 0);
                if (trace != 0) {
                    m_owned_0c8[walk].flags_00 |= 4;
                }
                else {
                    direct_visibility_node = walk;
                }
            }
            walk = walk_parent;
            walk_parent = m_owned_0c8[walk].parent_node_0a;
        }
        if (flag_09c == 0 && direct_visibility_node == 0) {
            srVector3T<float> trace_target = movement->target_position_04c;
            trace_target.y += trace_height_offset_0bc;
            float bearing = NormalizeAngle(Function4BE420(
                &m_owned_0c8[walk].position_20, &trace_target));
            float target_yaw = NormalizeAngle(trace_target_yaw_0c4);
            srMatrix3T<float> rotation;
            rotation.SetIdentity00467310();
            float angle = bearing - target_yaw;
            if ((double)angle != g_zero_005ebb40) {
                rotation.method_00438F90(sin(angle), cos(angle));
            }
            srVector3T<float> transformed;
            transformed.x = Function4218E0(
                rotation.vectors[0], trace_offset_0ac);
            transformed.y = Function4218E0(
                rotation.vectors[1], trace_offset_0ac);
            transformed.z = Function4218E0(
                rotation.vectors[2], trace_offset_0ac);
            srVector3T<float> trace_source;
            trace_source.x = m_owned_0c8[walk].position_20.x + transformed.x;
            trace_source.y = m_owned_0c8[walk].position_20.y + transformed.y;
            trace_source.z = m_owned_0c8[walk].position_20.z + transformed.z;
            short trace = g_octree_6598a4->TraceLineOfSight(
                &trace_source, &trace_target, 1, -3, -3, 1, 0);
            if (trace == 0) {
                direct_path = 1;
            }
            else {
                m_owned_0c8[walk].flags_00 |= 4;
            }
        }
    }

    if ((probe_cell_key_078 == 0 && result == 0) || direct_path != 0) {
        attachment->flags_00 |= 0x02000000;
        attachment->position_40 = movement->position_040;
        attachment->position_4c[attachment->path_position_index_08] =
            movement->position_040;
        attachment->position_1c =
            attachment->position_4c[attachment->path_position_index_08];
        g_octree_6598a4->QueueOctreeKind130042E810(
            movement->location_id_004, &movement->position_040);
        g_startup_world_659c0c->fields.radius_084 =
            g_startup_world_659c0c->
                fields.movement_0c0.alternate_radius_0b4;
        attachment->flags_00 &= 0xfffffff0;
        if (flag_1cb != 0 && m_owned_054 != 0) {
            BuildSearchVisualization0045CFD0();
        }
        return 0;
    }

    if (search_node_count_0cc >= g_path_reserve_0060827a ||
        best_node == 0) {
        result = 3;
    }

    unsigned short selected = (unsigned short)probe_cell_key_078;
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

    if (flag_1cb != 0 && m_owned_054 != 0) {
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
                    attachment->path_values_50[prop_count++] =
                        (unsigned short)prop;
                }
            }
            else {
                ProcessSearchNodeProps004663D0(route_parent, 1);
            }
        }

        if (CanReachSearchNode00465AF0(
                &m_owned_0c8[anchor_node].position_20,
                route_parent, radius) == 0) {
            if ((unsigned int)attachment->path_position_index_08 + 1 >=
                attachment->capacity_0a) {
                attachment->GrowPathStorage00456BD0();
            }
            attachment->position_4c[
                attachment->path_position_index_08] =
                m_owned_0c8[route_node].position_20;
            attachment->path_values_50[
                attachment->path_position_index_08] = 0;
            ++attachment->path_position_index_08;
            attachment->flags_00 &= 0xffbfffff;
            anchor_node = route_node;
        }

        unsigned short next = node->parent_node_0a;
        if (flag_0a4 != 0) {
            if ((node->flags_00 & 4) != 0) {
                node->flags_00 |= 8;
            }
            else if (TestSearchPositionVisibility00464CC0(
                         &node->position_20, movement) == 0) {
                node->flags_00 |= 8;
            }
            else {
                next = 0;
                result = 1;
                probe_cell_key_078 = route_parent;
            }
        }

        route_node = route_parent;
        route_parent = next;
        if (direct_visibility_node != 0 &&
            route_node == direct_visibility_node) {
            probe_cell_key_078 = route_node;
            break;
        }
        if (route_node == 0 ||
            m_owned_0c8[route_node].path_cost_10 > remaining_callback) {
            continue;
        }
        probe_cell_key_078 = route_node;
        break;
    }

    if ((unsigned int)attachment->path_position_index_08 + 1 >=
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
        attachment->position_1c =
            attachment->position_4c[attachment->path_position_index_08];
    }
    if ((attachment->flags_00 & 0x08000000) == 0) {
        attachment->flags_00 |= 0x02000000;
        attachment->position_40 =
            m_owned_0c8[probe_cell_key_078].position_20;
        g_octree_6598a4->QueueOctreeKind130042E810(
            movement->location_id_004,
            &m_owned_0c8[probe_cell_key_078].position_20);
    }
    if (movement->value_010 >= 0 && flag_09c == 0) {
        AdjustFinalPathEndpoint00465D70(movement, radius, separation);
    }
    attachment->flags_00 &= 0xfffffff0;
    attachment->flags_00 |= result;
    g_startup_world_659c0c->fields.radius_084 =
        g_startup_world_659c0c->fields.movement_0c0.alternate_radius_0b4;
    return result;
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

/* Test whether a candidate search position has the configured range and line
   of sight to the movement target. The trace origin is an offset rotated from
   the candidate-to-target bearing into the configured target yaw; the trace
   endpoint is the movement target with its configured vertical adjustment. */
// FUNCTION: WIZ8 0x00464cc0
unsigned char W8PathingService::TestSearchPositionVisibility00464CC0(
    const srVector3T<float>* position,
    W8NavigatorMovementState* movement)
{
    W8Monster* monster =
        GetMonsterByLocationID((unsigned int)movement->location_id_004);
    float distance;
    if (trace_target_location_0c0 == -1) {
        distance = monster->GetPointDistanceToPlayer004C7D50(
            position->x, position->y, position->z);
    }
    else {
        W8Monster* target = GetMonsterByLocationID(trace_target_location_0c0);
        distance = monster->GetPointDistanceToMonster004C7E80(
            target, position->x, position->y, position->z);
    }
    if (trace_max_distance_0a8 < distance) {
        return 0;
    }

    srVector3T<float> movement_target = movement->target_position_04c;
    float bearing = NormalizeAngle(Function4BE420(position, &movement_target));
    float target_yaw = NormalizeAngle(trace_target_yaw_0c4);

    srMatrix3T<float> rotation;
    rotation.vectors[0].method_00421680(1.0f, 0.0f, 0.0f);
    rotation.vectors[1].method_00421680(0.0f, 1.0f, 0.0f);
    rotation.vectors[2].method_00421680(0.0f, 0.0f, 1.0f);
    float angle = bearing - target_yaw;
    if ((double)angle != g_zero_005ebb40) {
        rotation.method_00438F90(sin(angle), cos(angle));
    }

    srVector3T<float> transformed;
    transformed.x = Function4218E0(rotation.vectors[0], trace_offset_0ac);
    transformed.y = Function4218E0(rotation.vectors[1], trace_offset_0ac);
    transformed.z = Function4218E0(rotation.vectors[2], trace_offset_0ac);

    srVector3T<float> trace_source;
    trace_source.x = position->x + transformed.x;
    trace_source.y = position->y + transformed.y;
    trace_source.z = position->z + transformed.z;
    srVector3T<float> trace_target = movement->target_position_04c;
    trace_target.y += trace_height_offset_0bc;

    unsigned char range_mode = 0;
    if (CalcRangeDistance(0) < distance && trace_mode_0b8 == 1) {
        range_mode = 1;
    }
    short trace = g_octree_6598a4->TraceLineOfSight(
        &trace_source,
        &trace_target,
        1,
        movement->location_id_004,
        trace_target_location_0c0,
        1,
        range_mode);
    if (trace != 1 &&
        (trace != -1 || Function51B3F0(trace_mode_0b8) != 0)) {
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
    W8NavigatorMovementState* movement,
    int target_location,
    float radius,
    float separation,
    float maximum_distance,
    float offset_x,
    float offset_y,
    float offset_z,
    int requested_trace_mode,
    float target_height_offset,
    float target_yaw,
    unsigned char* probe_result)
{
    trace_max_distance_0a8 = maximum_distance;
    trace_offset_0ac.x = offset_x;
    trace_offset_0ac.y = offset_y;
    trace_offset_0ac.z = offset_z;
    trace_mode_0b8 = requested_trace_mode;
    flag_0a4 = 1;
    trace_height_offset_0bc = target_height_offset;
    trace_target_yaw_0c4 = target_yaw;
    if (target_location == 0) {
        trace_target_location_0c0 = -1;
    }
    else {
        trace_target_location_0c0 = target_location;
    }

    g_octree_6598a4->AdjustPosition00431DA0(
        &movement->target_position_04c, 1);

    unsigned short result = 0;
    W8NavigatorAttachment* attachment = movement->attachment_0ac;
    if ((attachment->flags_00 & 0x00010000) != 0) {
        float delta_x =
            movement->target_position_04c.x - movement->position_040.x;
        float delta_z =
            movement->target_position_04c.z - movement->position_040.z;
        float horizontal_clearance =
            (float)sqrt(delta_x * delta_x + delta_z * delta_z) - radius;
        float target_radius;
        if (target_location == 0) {
            target_radius = g_startup_world_659c0c->
                fields.movement_0c0.alternate_radius_0b4;
        }
        else {
            W8Monster* target = GetMonsterByLocationID(target_location);
            target_radius = target->fields.movement_0c0.alternate_radius_0b4;
        }

        if (horizontal_clearance - target_radius <= separation &&
            TestSearchPositionVisibility00464CC0(
                &movement->position_040, movement) != 0) {
            attachment->InitializeSegment004563E0(
                &movement->position_040, &movement->position_040);
            flag_0a4 = 0;
            if (probe_result != 0) {
                *probe_result = 0;
            }
            return 0;
        }

        attachment->InitializeSegment004563E0(
            &movement->position_040, &movement->target_position_04c);
        attachment->separation_54 = separation;
        if (probe_result != 0) {
            attachment->flags_00 |= 0x04001000;
            result = PlanMovement00463460(movement, radius, separation);
            attachment->flags_00 &= 0xfbffefff;
            *probe_result = result == 1;
            attachment->InitializeSegment004563E0(
                &movement->position_040, &movement->target_position_04c);
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
unsigned char W8PathingService::ResolvePathCell004648D0(
    unsigned int key,
    unsigned char allow_dynamic,
    unsigned int* height,
    float* direction,
    float* vertical,
    unsigned char* dynamic)
{
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    int* buckets = static_cast<int*>(index->bucket_heads);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = buckets[hash & (index->bucket_count - 1)];

    while (slot != -1 && entries[slot].key != key) {
        slot = entries[slot].next_index;
    }
    while (slot != -1) {
        W8OctreeEntry* entry = &entries[slot];
        unsigned int value = (unsigned int)entry->value;
        int difference = (value & 0xffff) - *height;
        if (-cell_count_024 < difference && difference < cell_count_024 &&
            (value & 0x20000000) == 0 &&
            ((value & 0x10000000) == 0 ||
             (allow_dynamic != 0 && (value & 0x08000000) != 0))) {
            int magnitude = difference;
            if (magnitude < 0) {
                magnitude = -magnitude;
            }
            *vertical +=
                (float)magnitude * span_020 * g_float_005ec3b8;
            *height = value & 0xffff;
            if ((value & 0x01000000) == 0) {
                *direction = (float)(value >> 16 & 0xff);
            }
            else {
                *direction = 0.0f;
            }
            *direction =
                (*direction + g_float_005ebc7c) * g_world_scale_005ebc40;
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

    if (path_nodes_044 != 0) {
        free(path_nodes_044);
    }
    if (m_pSurfaces_048 != 0) {
        free(m_pSurfaces_048);
    }
    if (m_pEdges_04c != 0) {
        free(m_pEdges_04c);
    }
    if (file_waypoints_050 != 0) {
        free(file_waypoints_050);
    }
    if (m_owned_054 != 0) {
        delete m_owned_054;
    }
    if (visible_waypoints_058 != 0) {
        visible_waypoints_058->FreeIndex();
        ::operator delete(visible_waypoints_058);
    }
    if (rendered_waypoints_05c != 0) {
        rendered_waypoints_05c->FreeIndex();
        ::operator delete(rendered_waypoints_05c);
    }
    if (collected_waypoints_060 != 0) {
        collected_waypoints_060->FreeIndex();
        ::operator delete(collected_waypoints_060);
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
    index = reinterpret_cast<void**>(path_heap_06c);
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
    path_nodes_044 = 0;
    size_004 = 0;
    m_positional_008 = 0;
    m_ulNumSurfaces = 0;
    m_ulNumEdges = 0;
    m_positional_014 = 0;
    m_positional_018 = 0;
    m_pSurfaces_048 = 0;
    m_pEdges_04c = 0;
    file_waypoints_050 = 0;
    m_owned_054 = 0;
    visible_waypoints_058 = new BitArray(100);
    rendered_waypoints_05c = new BitArray(100);
    collected_waypoints_060 = new BitArray(100);
    m_pIndex_064 = 0;
    m_pIndex_074 = 0;
    level_name = 0;
    flag_08c = 0;
    path_heap_06c = 0;
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
    search_node_count_0cc = 0;
    search_node_capacity_0d0 = 0;
    flag_09c = 0;
    flag_0a4 = 0;
    trace_target_location_0c0 = 0;
    trace_max_distance_0a8 = 0;
    trace_offset_0ac.x = 0;
    trace_offset_0ac.y = 0;
    trace_offset_0ac.z = 0;
    trace_mode_0b8 = 0;
    trace_height_offset_0bc = 0;
    trace_target_yaw_0c4 = 0;
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

/* Test the first static path-index value in the position's vertical
   neighborhood. The packed direction byte becomes a world-space clearance;
   the special direction flag selects the global fallback instead. A successful
   lookup may also move the position onto the indexed cell before testing that
   clearance. */
// FUNCTION: WIZ8 0x00463040
unsigned char W8PathingService::TestPathCellClearance00463040(
    srVector3T<float>* position,
    float clearance,
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
    unsigned int packed = 0;
    unsigned int matched_height = height;
    unsigned char found = 0;
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = static_cast<int*>(index->bucket_heads)[hash & (index->bucket_count - 1)];

    while (slot != -1 && found == 0) {
        W8OctreeEntry* entry = &entries[slot];

        if (entry->key == key) {
            packed = entry->value;
            matched_height = packed & 0xffff;
            int delta = (int)(height - matched_height);

            if ((packed & 0x10000000) == 0 &&
                -vertical_window < delta && delta < vertical_window) {
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
        position->y = (float)(matched_height - 1) * span_020 + level_bounds[1];
        position->x =
            ((float)cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z =
            ((float)cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }

    float direction = g_float_005ebb34;
    if ((packed & 0x01000000) == 0) {
        direction = (float)((packed >> 16) & 0xff);
    }
    return clearance <
        direction * g_world_scale_005ebc40 + grid_scale_01c * g_float_005ebc7c;
}

/* Snap a position to the closest eligible indexed height no higher than its
   own. Directional entries are ignored unless the caller explicitly permits
   them; X and Z always move to the chosen cell's center. */
// FUNCTION: WIZ8 0x00463290
unsigned char W8PathingService::SnapToLowerPathCell00463290(
    srVector3T<float>* position,
    unsigned char allow_directional)
{
    unsigned char found = 0;
    int nearest = 10000000;
    unsigned int height =
        (unsigned int)(int)((position->y - level_bounds[1]) / span_020) + 1;
    unsigned int cell_x =
        (unsigned int)(int)((position->x - level_bounds[0]) / grid_scale_01c);
    unsigned int cell_z =
        (unsigned int)(int)((position->z - level_bounds[2]) / grid_scale_01c);
    unsigned int key = cell_z * 0x10000 + cell_x;
    unsigned int matched_height = 0;
    W8OctreeIndex* index = static_cast<W8OctreeIndex*>(m_pIndex_064);
    W8OctreeEntry* entries = static_cast<W8OctreeEntry*>(index->entries);
    unsigned int hash = (key >> 10 ^ key) >> 10 ^ key;
    int slot = static_cast<int*>(index->bucket_heads)[hash & (index->bucket_count - 1)];

    while (slot != -1) {
        W8OctreeEntry* entry = &entries[slot];

        if (entry->key == key &&
            (allow_directional != 0 || (entry->value & 0x00ff0000) == 0)) {
            unsigned int candidate_height = entry->value & 0xffff;
            int delta = (int)(height - candidate_height);

            if (delta >= 0 && delta < nearest) {
                found = 1;
                nearest = delta;
                matched_height = candidate_height;
            }
        }
        slot = entry->next_index;
    }

    if (found != 0) {
        position->y = (float)(matched_height - 1) * span_020 + level_bounds[1];
        position->x =
            ((float)cell_x + g_float_005ebc7c) * grid_scale_01c + level_bounds[0];
        position->z =
            ((float)cell_z + g_float_005ebc7c) * grid_scale_01c + level_bounds[2];
    }
    return found;
}

/* Search the short arc between an attachment's two endpoint positions. The
   temporary index is rebuilt before the paired directed probes, so both walks
   share only the path position they discover. */
// FUNCTION: WIZ8 0x00462360
unsigned char W8PathingService::ProbeAttachmentPath00462360(
    W8NavigatorAttachment* attachment)
{
    float delta_x = attachment->position_10.x - attachment->position_1c.x;
    float delta_y = attachment->position_10.y - attachment->position_1c.y;
    float delta_z = attachment->position_10.z - attachment->position_1c.z;
    float distance =
        (float)sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

    if ((double)distance > g_double_005ec3a0) {
        return 0;
    }

    flag_08c = 0;
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

    probe_cell_key_078 = 0;
    probe_limit_088 = 0;
    ProbeWaypointArc00462570(
        &attachment->position_10, &attachment->position_1c);
    flag_08c = 0;
    probe_limit_088 = 0xffffffff;
    ProbeWaypointArc00462570(
        &attachment->position_1c, &attachment->position_10);

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

            if ((float)distances[index] <
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
        if (attachment->value_04 < attachment->path_position_index_08) {
            unsigned short* pairs =
                attachment->path_values_50;
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

/* Populate the editor mesh from the currently visible waypoint set. Marker
   geometry occupies the first hundred six-polygon groups; directed links use
   the following groups and are emitted once when a visible reverse edge
   exists. */
// FUNCTION: WIZ8 0x0045BE30
stModelInstance005EC7D0* W8PathingService::BuildPathVisualization0045BE30()
{
    static srVector3T<float> marker_offsets[5] = {
        srVector3T<float>(0.0f, 0.5f, 0.0f),
        srVector3T<float>(-0.25f, 0.0f, -0.25f),
        srVector3T<float>(-0.25f, 0.0f, 0.25f),
        srVector3T<float>(0.25f, 0.0f, 0.25f),
        srVector3T<float>(0.25f, 0.0f, -0.25f)
    };
    static unsigned char marker_offsets_scaled = 0;
    int index;

    if (marker_offsets_scaled == 0) {
        for (index = 0; index < 5; ++index) {
            marker_offsets[index].x *=
                (float)g_path_waypoint_snap_distance_005ec150;
            marker_offsets[index].y *=
                (float)g_path_waypoint_snap_distance_005ec150;
            marker_offsets[index].z *=
                (float)g_path_waypoint_snap_distance_005ec150;
        }
        marker_offsets_scaled = 1;
    }
    if (m_owned_054 == 0) {
        EnsurePathVisualization0045D530();
    }

    stMeshModel* model = static_cast<stMeshModel*>(m_owned_054->model());
    srVector3T<float>* colors = model->getVertexDIG(0, 1);
    srVector3T<float>* vertices = model->getVertexLoc();
    srVector3i* polygons = model->getPolyVertex();
    int marker_count = 0;
    int link_count = 0;
    int next = visible_waypoints_058->NextSetBit(1);

    rendered_waypoints_05c->ClearAll();
    while (next != 0 && marker_count < 100) {
        unsigned short source_index = (unsigned short)(next - 1);
        W8PathSurface* source = &m_pSurfaces_048[source_index];
        srVector3T<float> marker_color;
        float marker_scale =
            (float)(source->flags_00 >> 12) * (float)g_double_005ec378;
        int marker_vertex = marker_count * 5;

        rendered_waypoints_05c->SetAndGrow(source_index);
        GetWaypointVisualizationColor0045D490(source_index, &marker_color);
        for (index = 0; index < 5; ++index) {
            vertices[marker_vertex + index].x =
                source->position_04.x + marker_offsets[index].x * marker_scale;
            vertices[marker_vertex + index].y =
                source->position_04.y +
                (float)g_path_waypoint_snap_distance_005ec150 +
                marker_offsets[index].y *
                    (float)(source->flags_00 >> 12) * 0.5f;
            vertices[marker_vertex + index].z =
                source->position_04.z + marker_offsets[index].z * marker_scale;
            colors[marker_vertex + index] = marker_color;
        }
        if ((source->flags_00 & 2) != 0) {
            colors[marker_vertex].x =
                colors[marker_vertex].x <= g_float_005ebb34 ? 1.0f : 0.0f;
            colors[marker_vertex].y =
                colors[marker_vertex].y <= g_float_005ebb34 ? 1.0f : 0.0f;
            colors[marker_vertex].z =
                colors[marker_vertex].z <= g_float_005ebb34 ? 1.0f : 0.0f;
        }
        else if ((source->flags_00 & 0x40) != 0) {
            colors[marker_vertex].x = 0.0f;
            colors[marker_vertex].y = 0.0f;
            colors[marker_vertex].z = 0.0f;
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
                }
                else {
                    reverse_index = m_pEdges_04c[reverse_index].next_0c;
                }
            }

            if (!rendered_waypoints_05c->Test(destination_index) &&
                !visible_waypoints_058->Test(destination_index) &&
                marker_count + 1 < 100) {
                srVector3T<float> destination_color;
                float destination_scale =
                    (float)(destination->flags_00 >> 12) *
                    (float)g_double_005ec378;
                int destination_vertex = (marker_count + 1) * 5;

                GetWaypointVisualizationColor0045D490(
                    destination_index, &destination_color);
                for (index = 0; index < 5; ++index) {
                    vertices[destination_vertex + index].x =
                        destination->position_04.x +
                        marker_offsets[index].x * destination_scale;
                    vertices[destination_vertex + index].y =
                        destination->position_04.y +
                        (float)g_path_waypoint_snap_distance_005ec150 +
                        marker_offsets[index].y *
                            (float)(destination->flags_00 >> 12) * 0.5f;
                    vertices[destination_vertex + index].z =
                        destination->position_04.z +
                        marker_offsets[index].z * destination_scale;
                    colors[destination_vertex + index] = destination_color;
                }
                if ((source->flags_00 & 2) != 0) {
                    colors[destination_vertex].x =
                        colors[destination_vertex].x <= g_float_005ebb34
                            ? 1.0f : 0.0f;
                    colors[destination_vertex].y =
                        colors[destination_vertex].y <= g_float_005ebb34
                            ? 1.0f : 0.0f;
                    colors[destination_vertex].z =
                        colors[destination_vertex].z <= g_float_005ebb34
                            ? 1.0f : 0.0f;
                }
                else if ((source->flags_00 & 0x40) != 0) {
                    colors[destination_vertex].x = 0.0f;
                    colors[destination_vertex].y = 0.0f;
                    colors[destination_vertex].z = 0.0f;
                }
                rendered_waypoints_05c->SetAndGrow(destination_index);
                ++marker_count;
            }

            if (source_index < destination_index ||
                !visible_waypoints_058->Test(destination_index) ||
                reverse_found == 0) {
                int base_vertex = 500 + link_count * 6;
                int base_polygon = 600 + link_count * 6;
                float perpendicular_x =
                    -(destination->position_04.z - source->position_04.z);
                float perpendicular_z =
                    destination->position_04.x - source->position_04.x;
                float length_squared =
                    perpendicular_x * perpendicular_x +
                    perpendicular_z * perpendicular_z;

                if ((double)length_squared != g_zero_005ebb40) {
                    float scale = (float)(
                        g_double_005ec368 / sqrt(length_squared));
                    perpendicular_x *= scale;
                    perpendicular_z *= scale;
                }

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
                vertices[base_vertex + 1].x =
                    source->position_04.x + perpendicular_x;
                vertices[base_vertex + 1].y =
                    source->position_04.y + g_world_scale_005ebc40;
                vertices[base_vertex + 1].z =
                    source->position_04.z + perpendicular_z;
                vertices[base_vertex + 2].x =
                    source->position_04.x - perpendicular_x;
                vertices[base_vertex + 2].y =
                    source->position_04.y + g_world_scale_005ebc40;
                vertices[base_vertex + 2].z =
                    source->position_04.z - perpendicular_z;
                vertices[base_vertex + 4].x =
                    destination->position_04.x + perpendicular_x;
                vertices[base_vertex + 4].y =
                    destination->position_04.y + g_world_scale_005ebc40;
                vertices[base_vertex + 4].z =
                    destination->position_04.z + perpendicular_z;
                vertices[base_vertex + 5].x =
                    destination->position_04.x - perpendicular_x;
                vertices[base_vertex + 5].y =
                    destination->position_04.y + g_world_scale_005ebc40;
                vertices[base_vertex + 5].z =
                    destination->position_04.z - perpendicular_z;

                for (index = 0; index < 6; ++index) {
                    colors[base_vertex + index].x = 0.0f;
                    colors[base_vertex + index].y = 0.0f;
                    colors[base_vertex + index].z = 0.0f;
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
    return m_owned_054;
}

/* Rebuild the editor's bounded grid search when the cursor enters a new path
   cell. Each reachable vertical span is inserted once into the visited hash;
   the minimum heap expands the nearest pending position first, and the final
   node set is handed to the search-trace renderer. */
// FUNCTION: WIZ8 0x0045C9A0
void W8PathingService::DrawPathPosition0045C9A0(
    srVector3T<float> position, unsigned char mode)
{
    if (mode == 0 || g_flag_006081e4 == 0) {
        g_path_visualization_cell_00659c6c = 0;
        return;
    }

    int root_x = (int)((position.x - level_bounds[0]) / grid_scale_01c);
    int root_z = (int)((position.z - level_bounds[2]) / grid_scale_01c);
    unsigned int root_key = root_z * 0x10000 + root_x;
    if (root_key == g_path_visualization_cell_00659c6c) {
        return;
    }
    g_path_visualization_cell_00659c6c = root_key;

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

    search_node_count_0cc = 0;
    path_heap_06c->heap_00->size_0c = 0;
    unsigned short root_index = AllocateSearchNode00465A00();
    W8PathSearchNode* root = &m_owned_0c8[root_index];
    root->flags_00 = 0;
    root->node_index_02 = root_index;
    root->cell_x_04 = (unsigned short)root_x;
    root->cell_z_06 = (unsigned short)root_z;
    root->path_height_08 = (unsigned short)(
        (int)((position.y - level_bounds[1]) / span_020) + 1);
    root->parent_node_0a = 0;
    root->base_score_0c = 0.0f;
    root->position_20 = position;

    W8PathHeap* heap = path_heap_06c->heap_00;
    W8PathHeapEntry root_entry;
    root_entry.node_00 = root_index;
    root_entry.priority_04 = (unsigned int)root->score_1c;
    if (heap->size_0c >= heap->capacity_08) {
        srAssertFail(
            "heapsize < maxheapsize",
            "..\\Engine Code\\Include\\stHeap.hpp",
            0xe1,
            "stHeap overflow");
    }
    heap->entries_00[heap->size_0c] = root_entry;
    heap->SiftUp00467990(heap->size_0c);
    ++heap->size_0c;
    path_heap_06c->root_node_04 = heap->entries_00[0].node_00;

    unsigned int best_node = root_index;
    while (best_node != 0 &&
           search_node_count_0cc < g_path_reserve_0060827a) {
        W8PathSearchNode* current = &m_owned_0c8[best_node];
        unsigned short current_x = current->cell_x_04;
        unsigned short current_z = current->cell_z_06;
        unsigned short current_height = current->path_height_08;

        for (int direction = 0; direction < 8; ++direction) {
            unsigned int neighbor_x;
            unsigned int neighbor_z;
            if (direction >= 1 && direction <= 3) {
                neighbor_x = current_x + 1;
            }
            else if (direction > 4) {
                neighbor_x = current_x - 1;
            }
            else {
                neighbor_x = current_x;
            }
            if (direction < 2 || direction > 6) {
                neighbor_z = current_z + 1;
            }
            else if (direction > 2 && direction < 6) {
                neighbor_z = current_z - 1;
            }
            else {
                neighbor_z = current_z;
            }

            unsigned int key = neighbor_z * 0x10000 + neighbor_x;
            unsigned int hash = ((key >> 10 ^ key) >> 10 ^ key);
            int slot = static_cast<int*>(visited->bucket_heads)[
                (visited->bucket_count - 1) & hash];
            while (slot != -1) {
                W8OctreeEntry* entry =
                    static_cast<W8OctreeEntry*>(visited->entries) + slot;
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

            W8OctreeIndex* paths = static_cast<W8OctreeIndex*>(m_pIndex_064);
            int path_slot = static_cast<int*>(paths->bucket_heads)[
                (paths->bucket_count - 1) & hash];
            while (path_slot != -1) {
                W8OctreeEntry* path_entry =
                    static_cast<W8OctreeEntry*>(paths->entries) + path_slot;
                if (path_entry->key == key) {
                    unsigned int path_value = (unsigned int)path_entry->value;
                    int height_delta =
                        (int)(path_value & 0xffff) - current_height;
                    if (height_delta > -(int)cell_count_024 &&
                        height_delta < (int)cell_count_024) {
                        unsigned short node_index =
                            AllocateSearchNode00465A00();
                        if (visited->free_head == -1) {
                            GrowIndex00439290(visited);
                        }
                        W8OctreeEntry* visited_entries =
                            static_cast<W8OctreeEntry*>(visited->entries);
                        int inserted = visited->free_head;
                        visited->free_head =
                            visited_entries[inserted].next_index;
                        unsigned int bucket =
                            (visited->bucket_count - 1) & hash;
                        visited_entries[inserted].key = key;
                        visited_entries[inserted].value = node_index;
                        visited_entries[inserted].next_index =
                            static_cast<int*>(visited->bucket_heads)[bucket];
                        static_cast<int*>(visited->bucket_heads)[bucket] =
                            inserted;

                        W8PathSearchNode* node = &m_owned_0c8[node_index];
                        node->flags_00 = 0;
                        if ((path_value & 0x10000000) != 0) {
                            node->flags_00 = 0x800;
                        }
                        if ((path_value & 0x04000000) != 0) {
                            node->flags_00 |= 0x1000;
                        }
                        node->node_index_02 = node_index;
                        node->cell_x_04 = (unsigned short)neighbor_x;
                        node->cell_z_06 = (unsigned short)neighbor_z;
                        node->path_height_08 =
                            (unsigned short)(path_value & 0xffff);
                        node->parent_node_0a = (unsigned short)best_node;
                        node->position_20.x =
                            ((float)node->cell_x_04 + g_float_005ebc7c) *
                                grid_scale_01c +
                            level_bounds[0];
                        node->position_20.y =
                            (float)(node->path_height_08 - 1) * span_020 +
                            level_bounds[1];
                        node->position_20.z =
                            ((float)node->cell_z_06 + g_float_005ebc7c) *
                                grid_scale_01c +
                            level_bounds[2];
                        float delta_x = node->position_20.x - position.x;
                        float delta_y = node->position_20.y - position.y;
                        float delta_z = node->position_20.z - position.z;
                        node->score_1c = (float)sqrt(
                            delta_x * delta_x + delta_y * delta_y +
                            delta_z * delta_z);
                        if (node->score_1c <
                            g_path_search_visualization_limit_005ec380) {
                            W8PathHeapEntry pending;
                            pending.node_00 = node->node_index_02;
                            pending.priority_04 =
                                (unsigned int)node->score_1c;
                            heap->Insert004675B0(&pending);
                            path_heap_06c->root_node_04 =
                                heap->entries_00[0].node_00;
                        }
                        else {
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
            sprintf(
                message,
                "A:  Invalid node index %d from Queue.",
                best_node);
            srAssertFail(
                "(ulBestNode <= m_ulSearchNodesUsed)",
                OCTPATH_CPP,
                0x1110,
                message);
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
        srVector3T<float>(0.0f, 125.0f, 0.0f),
        srVector3T<float>(-62.5f, 0.0f, -62.5f),
        srVector3T<float>(-62.5f, 0.0f, 62.5f),
        srVector3T<float>(62.5f, 0.0f, 62.5f),
        srVector3T<float>(62.5f, 0.0f, -62.5f)
    };
    stMeshModel* model = static_cast<stMeshModel*>(m_owned_054->model());
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
        }
        else if ((node->flags_00 & 4) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 1.0f);
        }
        else if ((node->flags_00 & 2) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 1.0f);
        }
        else if ((node->flags_00 & 0x100) != 0) {
            color = srVector3T<float>(1.0f, 0.0f, 0.0f);
        }
        else if ((node->flags_00 & 0x2000) != 0) {
            color = srVector3T<float>(1.0f, 1.0f, 0.0f);
        }
        else if ((node->flags_00 & 0x8000) != 0) {
            color = srVector3T<float>(0.0f, 1.0f, 1.0f);
        }
        else if ((node->flags_00 & 0x1000) != 0) {
            color = srVector3T<float>(0.0f, 0.0f, 1.0f);
        }
        else {
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
stModelInstance005EC7D0* W8PathingService::EnsurePathVisualization0045D530()
{
    const int polygon_count = 0x3138;
    const int vertex_count = 0x30d4;
    stMeshModel* model = new stMeshModel(polygon_count, vertex_count);
    int index;

    if (model == 0) {
        srAssertFail(
            "pstMeshModel", OCTPATH_CPP, 0x11e9,
            "CreateWayPointMesh::Read -- Could not create pstMeshModel.");
    }
    model->autoRelease();
    model->flags_3a0 &= ~1U;
    model->setShader(g_path_shader_00652dc4, 0);
    model->setName("WayPoint Mesh");
    model->flag_3cc = 0;

    srVector3i* polygons = model->getPolyVertex();
    srPtr<srTextureIFace>* textures = model->getPolyTexture(0, 0, 1);
    srVector2T<float>* texture_coordinates =
        model->getVertexTexCoords(0, 0, 1);
    srPtr<srMaterialIFace>* materials = model->getVertexMaterial(
        0, static_cast<srMeshModel::e_side>(0), 1);
    unsigned long* shade_indices = model->getVertexShadeIndex(1);

    for (index = 0; index < polygon_count; ++index) {
        textures[index] = g_path_texture_00652dc0;
    }
    for (index = 0; index < vertex_count; ++index) {
        texture_coordinates[index].x = 0.0f;
        texture_coordinates[index].y = 0.0f;
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
        colors[index].x = 0.0f;
        colors[index].y = 0.0f;
        colors[index].z = 1.0f;
    }

    m_owned_054 = CreateModelInstance0046F5C0(model);
    if (m_owned_054 == 0) {
        srAssertFail(
            "m_pPathModelInstance", OCTPATH_CPP, 0x1226,
            "CreateWayPointMesh -- Could not create pstModelInstance.");
    }
    m_owned_054->setName("WayPoint Mesh");
    m_owned_054->setExclusionMask(3);
    m_owned_054->setFlag(srNode::FLAG_POSITIONAL_0);
    return m_owned_054;
}

/* Collect nearby waypoint surfaces and mark the subset directly visible from
   the editor position. The near query also admits every outgoing neighbor;
   the wider query contributes only its own surfaces. Candidates are deduped,
   sorted by integer distance, and span-tested nearest first. */
// FUNCTION: WIZ8 0x0045D880
short W8PathingService::CollectPathVisualization0045D880(
    const srVector3T<float>* position)
{
    unsigned short waypoints[500];
    unsigned int distances[500];
    unsigned int waypoint_count = 0;
    int* query_results = 0;
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
    lower.x = position->x - g_path_waypoint_query_vertical_005ec35c;
    lower.y = position->y - g_float_005ec2f8;
    lower.z = position->z - g_path_waypoint_query_vertical_005ec35c;
    upper.x = position->x + g_path_waypoint_query_vertical_005ec35c;
    upper.y = position->y + g_float_005ec2f8;
    upper.z = position->z + g_path_waypoint_query_vertical_005ec35c;
    query_count = g_octree_6598a4->QueryObjects0042F280(
        &query_results, &lower, &upper, 9, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = (unsigned short)query_results[index];
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
    lower.y = position->y - g_path_waypoint_query_vertical_005ec35c;
    lower.z = position->z - g_float_005ec384;
    upper.x = position->x + g_float_005ec384;
    upper.y = position->y + g_path_waypoint_query_vertical_005ec35c;
    upper.z = position->z + g_float_005ec384;
    query_count = g_octree_6598a4->QueryObjects0042F280(
        &query_results, &lower, &upper, 9, -1);

    for (index = 0; index < query_count; ++index) {
        unsigned short waypoint = (unsigned short)query_results[index];
        if (!collected_waypoints_060->Set(waypoint)) {
            waypoints[waypoint_count++] = waypoint;
        }
    }

    if (waypoint_count > 1) {
        unsigned int sort_index;

        for (sort_index = 0; sort_index < waypoint_count; ++sort_index) {
            const srVector3T<float>* candidate =
                &m_pSurfaces_048[waypoints[sort_index]].position_04;
            float delta_x = position->x - candidate->x;
            float delta_y = position->y - candidate->y;
            float delta_z = position->z - candidate->z;
            distances[sort_index] = (unsigned int)(int)sqrt(
                delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
        }

        if (waypoint_count <= 10) {
            for (sort_index = 1; sort_index < waypoint_count; ++sort_index) {
                unsigned short waypoint = waypoints[sort_index];
                unsigned int distance = distances[sort_index];
                unsigned int insertion = sort_index;

                while (insertion != 0 &&
                       distance < distances[insertion - 1]) {
                    distances[insertion] = distances[insertion - 1];
                    waypoints[insertion] = waypoints[insertion - 1];
                    --insertion;
                }
                distances[insertion] = distance;
                waypoints[insertion] = waypoint;
            }
        }
        else {
            SortPathCandidates004677A0(
                waypoints, distances, 0, (int)waypoint_count - 1);
        }
    }

    for (unsigned int visible_index = 0;
         visible_index < waypoint_count;
         ++visible_index) {
        unsigned short waypoint = waypoints[visible_index];
        if (TestWaypointSpan0045A1B0(
                position, &m_pSurfaces_048[waypoint].position_04, 0, 0) != 0) {
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

        if (visible_waypoints_058 != 0) {
            visible_waypoints_058->FreeIndex();
            ::operator delete(visible_waypoints_058);
        }
        visible_waypoints_058 = new BitArray(capacity);
        if (rendered_waypoints_05c != 0) {
            rendered_waypoints_05c->FreeIndex();
            ::operator delete(rendered_waypoints_05c);
        }
        rendered_waypoints_05c = new BitArray(capacity);
        if (collected_waypoints_060 != 0) {
            collected_waypoints_060->FreeIndex();
            ::operator delete(collected_waypoints_060);
        }
        collected_waypoints_060 = new BitArray(capacity);

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

/* Choose the waypoint that best continues from the surface nearest source in
   the requested direction. Existing graph edges take precedence. When none
   are sufficiently aligned, probe twenty-five fixed steps forward and retain
   the best distinct surface found there. The second selection is marked valid
   only when it is an existing edge or the direct span test accepts it. */
// FUNCTION: WIZ8 0x0045e840
unsigned char W8PathingService::PreparePathVisualization0045E840(
    const srVector3T<float>* source,
    const srVector3T<float>* direction)
{
    float best_alignment = -1.0f;
    unsigned short best_waypoint = 0;
    unsigned short source_waypoint;
    W8PathSurface* source_surface;
    srVector3T<float> offset;
    float length_squared;
    float scale;

    value_1d6 = 0;
    path_direction_valid_1da = 0;
    source_waypoint = FindWaypoint0045B120(source, 0);
    source_surface = &m_pSurfaces_048[source_waypoint];

    offset.x = source_surface->position_04.x - source->x;
    offset.y = source_surface->position_04.y - source->y;
    offset.z = source_surface->position_04.z - source->z;
    if ((double)(float)sqrt(
            offset.x * offset.x + offset.y * offset.y + offset.z * offset.z) <=
        g_double_005ec030) {
        unsigned short edge_index = source_surface->first_edge_24;

        while (edge_index != 0) {
            W8PathEdge* edge = &m_pEdges_04c[edge_index];
            unsigned short neighbor_index = edge->destination_06;
            W8PathSurface* neighbor = &m_pSurfaces_048[neighbor_index];
            srVector3T<float> neighbor_direction;
            float alignment;

            neighbor_direction.x =
                neighbor->position_04.x - source_surface->position_04.x;
            neighbor_direction.y =
                neighbor->position_04.y - source_surface->position_04.y;
            neighbor_direction.z =
                neighbor->position_04.z - source_surface->position_04.z;
            length_squared =
                neighbor_direction.x * neighbor_direction.x +
                neighbor_direction.y * neighbor_direction.y +
                neighbor_direction.z * neighbor_direction.z;
            if ((double)length_squared != g_zero_005ebb40) {
                scale = (float)(g_double_005ebc30 / sqrt(length_squared));
                neighbor_direction.x *= scale;
                neighbor_direction.y *= scale;
                neighbor_direction.z *= scale;
            }
            alignment =
                neighbor_direction.x * direction->x +
                neighbor_direction.y * direction->y +
                neighbor_direction.z * direction->z;
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

            distance += g_path_waypoint_exact_distance_005ebc64;
            probe.x = source_surface->position_04.x + direction->x * distance;
            probe.y = source_surface->position_04.y + direction->y * distance;
            probe.z = source_surface->position_04.z + direction->z * distance;
            probe_waypoint = FindWaypoint0045B120(&probe, 0);
            if (probe_waypoint != 0 && probe_waypoint != source_waypoint) {
                W8PathSurface* candidate = &m_pSurfaces_048[probe_waypoint];
                srVector3T<float> candidate_direction;
                float alignment;

                candidate_direction.x =
                    candidate->position_04.x - source_surface->position_04.x;
                candidate_direction.y =
                    candidate->position_04.y - source_surface->position_04.y;
                candidate_direction.z =
                    candidate->position_04.z - source_surface->position_04.z;
                length_squared =
                    candidate_direction.x * candidate_direction.x +
                    candidate_direction.y * candidate_direction.y +
                    candidate_direction.z * candidate_direction.z;
                if ((double)length_squared != g_zero_005ebb40) {
                    scale =
                        (float)(g_double_005ebc30 / sqrt(length_squared));
                    candidate_direction.x *= scale;
                    candidate_direction.y *= scale;
                    candidate_direction.z *= scale;
                }
                alignment =
                    candidate_direction.x * direction->x +
                    candidate_direction.y * direction->y +
                    candidate_direction.z * direction->z;
                if (alignment > best_alignment) {
                    best_alignment = alignment;
                    best_waypoint = probe_waypoint;
                }
            }
            --probe_count;
        } while (probe_count != 0);

        if (best_alignment > g_float_005ec38c) {
            value_1d6 = best_waypoint;
            if (TestWaypointSpan0045A1B0(
                    &source_surface->position_04,
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
