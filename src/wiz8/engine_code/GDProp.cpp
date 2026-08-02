/* Engine Code\GDProp.cpp */

#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/Octree.h"

#include "surrender/srHeap.h"

#include <stdlib.h>

// FUNCTION: WIZ8 0x004b6e00
GDProp::GDProp(
    srNode* instance,
    const unsigned char* path_name,
    unsigned short prop_number,
    unsigned char surface_flag,
    unsigned char vertex_flag)
{
    m_flags_00 = 0;
    m_path_handle_04 = 0;
    m_prop_number_02 = 0;
    m_vertex_count_18 = 0;
    m_surface_count_14 = 0;
    m_pGDSurfaces = 0;
    m_pVertices = 0;
    m_instance_24 = 0;
    m_surface_index_08 = 0;
    m_vertex_index_0a = 0;
    m_allocation_0c = 0;
    m_allocation_10 = 0;
    m_list_54 = 0;
    m_path_sentinel_2c = -10000000.0f;
    m_path_bound_52 = 0;
    m_path_bound_50 = 0;
    m_path_bound_4e = 0;
    m_path_bound_4c = 0;

    if (g_octree_6598a4 != 0 &&
        g_octree_6598a4->pathing_180 != 0) {
        m_path_handle_04 = g_octree_6598a4->pathing_180->FindPathHandle(
            path_name,
            &m_path_bound_4c,
            &m_path_range_28);
    }

    if (instance != 0) {
        if (m_path_handle_04 != 0 &&
            g_octree_6598a4->pathing_180 != 0) {
            g_octree_6598a4->pathing_180->LinkSurfaces00460020();
            g_octree_6598a4->pathing_180->LinkEdges004600B0();
        }
        Initialize(instance, 1, prop_number, surface_flag, vertex_flag);
    }
}

// FUNCTION: WIZ8 0x004b6ed0
GDProp::~GDProp()
{
    if (m_pGDSurfaces != 0) {
        free(m_pGDSurfaces);
    }
    if (m_pVertices != 0) {
        srHeap.free(m_pVertices);
    }
    if (m_allocation_0c != 0) {
        free(m_allocation_0c);
    }
    if (m_allocation_10 != 0) {
        free(m_allocation_10);
    }
    if (m_list_54 != 0) {
        PLDestroy(m_list_54);
    }
}

/* The pathing record supplies inclusive unsigned coordinate bounds at
   +0x4c..+0x52. */
// FUNCTION: WIZ8 0x004B75F0
unsigned char GDProp::ContainsPathCoordinate004B75F0(
    unsigned short x, unsigned short y) const
{
    if (x >= m_path_bound_4c && x <= m_path_bound_4e &&
        y >= m_path_bound_50 && y <= m_path_bound_52) {
        return 1;
    }
    return 0;
}

/* Whether the optional owned list currently contains an entry. */
// FUNCTION: WIZ8 0x004B7BA0
unsigned char GDProp::HasListEntries004B7BA0()
{
    if (m_list_54 != 0 && (int)PLLength(m_list_54) > 0) {
        return 1;
    }
    return 0;
}
