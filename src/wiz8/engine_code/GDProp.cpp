/* Engine Code\GDProp.cpp */

#include "wiz8/engine_code/GDProp.h"

#include "surrender/srHeap.h"

#include <stdlib.h>

class W8Pathing00457CF0 {
public:
    unsigned int FindPathHandle(
        const unsigned char* path_name,
        unsigned short* path_bounds,
        float* path_range);              /* 0x00457CF0 */
    void LinkPropSurfaces(GDProp* prop);  /* 0x00460020 */
    void LinkPropVertices(GDProp* prop);  /* 0x004600B0 */
};

struct W8EngineState006598A4 {
    unsigned char unknown_000[0x180];
    W8Pathing00457CF0* pathing_180;
};

extern W8EngineState006598A4* g_engine_state_6598a4;

// FUNCTION: WIZ8 0x004B6E00
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

    if (g_engine_state_6598a4 != 0 &&
        g_engine_state_6598a4->pathing_180 != 0) {
        m_path_handle_04 = g_engine_state_6598a4->pathing_180->FindPathHandle(
            path_name,
            &m_path_bound_4c,
            &m_path_range_28);
    }

    if (instance != 0) {
        if (m_path_handle_04 != 0 &&
            g_engine_state_6598a4->pathing_180 != 0) {
            g_engine_state_6598a4->pathing_180->LinkPropSurfaces(this);
            g_engine_state_6598a4->pathing_180->LinkPropVertices(this);
        }
        Initialize(instance, 1, prop_number, surface_flag, vertex_flag);
    }
}

// FUNCTION: WIZ8 0x004B6ED0
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
        PListDestroy(m_list_54);
    }
}
