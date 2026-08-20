/* Engine Code\GDProp.cpp */

#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/float_constants.h"
#include "wiz8/item_spawning.h"
#include "wiz8/mesh_model.h"

#include "surrender/srHeap.h"
#include "surrender/srModelInstance.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern float g_float_005ebccc;
extern void BuildTrianglePlane00449A40(
    float* plane,
    const srVector3T<float>* first,
    const srVector3T<float>* second,
    const srVector3T<float>* third);
extern void SetWorldItemFlag02(W8WorldItem* item, char enabled);

// FUNCTION: WIZ8 0x004b6e00
GDProp::GDProp(
    srModelInstance* instance,
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
    m_owner_24 = 0;
    m_link_count_08 = 0;
    m_waypoint_count_0a = 0;
    m_links_0c = 0;
    m_waypoints_10 = 0;
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
    if (m_links_0c != 0) {
        free(m_links_0c);
    }
    if (m_waypoints_10 != 0) {
        free(m_waypoints_10);
    }
    if (m_list_54 != 0) {
        PLDestroy(m_list_54);
    }
}

/* Resize the two geometry tables to the complete linked mesh. Surfaces use
   the CRT heap, while vertices retain SurRender's heap ownership. */
// FUNCTION: WIZ8 0x004b6f30
void GDProp::PrepareGeometry004B6F30(srModelInstance* instance)
{
    int surface_count = 0;
    int vertex_count = 0;
    stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
    while (mesh != 0) {
        surface_count += mesh->polygon_count_230;
        vertex_count += mesh->vertex_location_count_22c;
        mesh = mesh->next;
    }

    if (m_surface_count_14 != surface_count) {
        m_surface_count_14 = surface_count;
        if (m_pGDSurfaces != 0) {
            free(m_pGDSurfaces);
        }
        m_pGDSurfaces = static_cast<W8GDSurface*>(
            malloc(m_surface_count_14 * sizeof(W8GDSurface)));
        if (m_pGDSurfaces == 0) {
            srAssertFail(
                "m_pGDSurfaces",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp",
                0xa0,
                0);
        }
        memset(m_pGDSurfaces, 0, m_surface_count_14 * sizeof(W8GDSurface));
    }

    if (m_vertex_count_18 != vertex_count) {
        m_vertex_count_18 = vertex_count;
        if (m_pVertices != 0) {
            srHeap.free(m_pVertices);
        }
        m_pVertices = static_cast<srVector3T<float>*>(
            srHeap.allocate(m_vertex_count_18 * sizeof(srVector3T<float>)));
        if (m_pVertices == 0) {
            srAssertFail(
                "m_pVertices",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp",
                0xae,
                0);
        }
        memset(m_pVertices, 0, m_vertex_count_18 * sizeof(srVector3T<float>));
    }
}

/* Rebuild the prop's collision geometry from its current linked mesh and
   publish the corresponding conditional path state. Vertices are transformed
   into world space before each accumulated triangle receives its plane,
   dominant axis, slope classification and caller-provided material bytes. */
// FUNCTION: WIZ8 0x004b7060
void GDProp::Initialize(
    srModelInstance* instance,
    unsigned char attach,
    unsigned short prop_number,
    unsigned char surface_flag,
    unsigned char vertex_flag)
{
    if (attach == 0) {
        m_flags_00 |= 4;
    }
    else {
        m_flags_00 &= 0xfffb;
        m_prop_number_02 = prop_number;
    }

    PrepareGeometry004B6F30(instance);
    srMatrix4T<float> world_matrix;
    instance->getWorldSpaceMatrix(world_matrix);

    int vertex_base = 0;
    int surface_total = 0;
    stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
    while (mesh != 0) {
        unsigned int mesh_flags = mesh->flags_3a0;
        srVector3i* polygon_vertices = mesh->getPolyVertex();
        for (int polygon = 0; polygon < mesh->polygon_count_230; ++polygon) {
            W8GDSurface* surface = &m_pGDSurfaces[surface_total + polygon];
            surface->vertex_indices_18[0] = polygon_vertices[polygon].x + vertex_base;
            surface->vertex_indices_18[1] = polygon_vertices[polygon].y + vertex_base;
            surface->vertex_indices_18[2] = polygon_vertices[polygon].z + vertex_base;
        }
        surface_total += mesh->polygon_count_230;

        srVector3T<float>* source_vertices = mesh->getVertexLoc();
        float* matrix = &world_matrix.vectors[0].x;
        for (int vertex = 0; vertex < mesh->vertex_location_count_22c; ++vertex) {
            srVector3T<float>* destination = &m_pVertices[vertex_base + vertex];
            srVector3T<float>* source = &source_vertices[vertex];
            destination->x =
                source->x * matrix[0] + source->y * matrix[1] +
                source->z * matrix[2] + matrix[3];
            destination->y =
                source->x * matrix[4] + source->y * matrix[5] +
                source->z * matrix[6] + matrix[7];
            destination->z =
                source->x * matrix[8] + source->y * matrix[9] +
                source->z * matrix[10] + matrix[11];
        }
        vertex_base += mesh->vertex_location_count_22c;

        for (int surface_index = 0; surface_index < surface_total; ++surface_index) {
            W8GDSurface* surface = &m_pGDSurfaces[surface_index];
            BuildTrianglePlane00449A40(
                surface->plane_24,
                &m_pVertices[surface->vertex_indices_18[0]],
                &m_pVertices[surface->vertex_indices_18[1]],
                &m_pVertices[surface->vertex_indices_18[2]]);

            int dominant_axis;
            float largest = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                float magnitude = (float)fabs(surface->plane_24[axis]);
                if (largest < magnitude) {
                    largest = magnitude;
                    dominant_axis = axis;
                }
            }
            surface->flags_00 = dominant_axis + 0x800;
            surface->surface_flag_3c = surface_flag;
            surface->vertex_flag_3d = vertex_flag;
            surface->value_38 = 0;
            if ((mesh_flags & 1) != 0) {
                surface->flags_00 |= 0x8000;
            }

            if (g_float_005ebc7c <= surface->plane_24[1]) {
                surface->value_40 = 500.0f;
                surface->flags_00 |= 4;
                if (g_float_005ebccc < surface->plane_24[1]) {
                    surface->value_48 = 1.0f;
                }
                else {
                    surface->value_48 = surface->plane_24[1];
                }
            }
            else {
                surface->value_48 = 0.0f;
                surface->value_40 = 500.0f;
            }
        }
        mesh = mesh->next;
    }

    W8PathingService* pathing = g_octree_6598a4->pathing_180;
    if (m_path_handle_04 != 0 && pathing != 0) {
        if (attach == 0) {
            if (m_prop_number_02 != 0xffff) {
                m_prop_number_02 = 0xffff;
                pathing->SetConditionalPathFrame00457EA0(m_path_handle_04, -1);
            }
        }
        else {
            pathing->SetConditionalPathFrame00457EA0(
                m_path_handle_04, (short)m_prop_number_02);
            if (m_waypoint_count_0a != 0) {
                pathing->CheckConditionalWaypointStatus004601B0(
                    m_waypoint_count_0a, m_waypoints_10);
            }
            if (m_link_count_08 != 0) {
                pathing->CheckConditionalLinkStatus00460250(
                    m_link_count_08, m_links_0c);
            }
        }
    }

    unsigned char* owner = static_cast<unsigned char*>(m_owner_24);
    if (owner != 0 && attach != 0) {
        unsigned char* state = *reinterpret_cast<unsigned char**>(owner + 0x234);
        if (state != 0 && state[4] == 10) {
            unsigned int flags = 0x08000000;
            if ((*reinterpret_cast<int*>(owner + 0x368) != 0 &&
                 owner[0x370] == 0) ||
                ((*reinterpret_cast<unsigned int*>(owner + 0xa0) & 0x100) == 0 ||
                 (state[8] & 5) != 0)) {
                flags = 0x28000000;
            }
            if (pathing != 0) {
                pathing->UpdateConditionalPathFlags00465FB0(
                    m_path_handle_04, m_prop_number_02, flags);
            }
        }
    }

    if (m_list_54 != 0) {
        unsigned int count = PLLength(m_list_54);
        for (unsigned int index = 0; index < count; ++index) {
            W8WorldItem* item = static_cast<W8WorldItem*>(
                PLGet(m_list_54, (int)index));
            if (item != 0) {
                SetWorldItemFlag02(item, 1);
            }
        }
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
