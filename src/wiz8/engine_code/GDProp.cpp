#include "wiz8/local_code/ItemManager.h"
/* Engine Code\GDProp.cpp */

#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/float_constants.h"
#include "wiz8/layouts/world.h"
#include "wiz8/item_spawning.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "surrender/srHeap.h"
#include "surrender/srModelInstance.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
// GLOBAL: WIZ8 0x005ebccc
float g_float_005ebccc = 0.75f;

// FUNCTION: WIZ8 0x004b6e00
GDProp::GDProp(srModelInstance* instance, const char* path_name, unsigned short prop_number,
               unsigned char footstep_surface, unsigned char footstep_material)
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
    m_path_range_28.sentinel = -10000000.0f;
    m_path_bounds_4c.max_z = 0;
    m_path_bounds_4c.min_z = 0;
    m_path_bounds_4c.max_x = 0;
    m_path_bounds_4c.min_x = 0;

    if (g_octree_6598a4 != 0 && g_octree_6598a4->pathing_180 != 0) {
        m_path_handle_04 = g_octree_6598a4->pathing_180->FindPathHandle(
            path_name, &m_path_bounds_4c, &m_path_range_28);
    }

    if (instance != 0) {
        if (m_path_handle_04 != 0 && g_octree_6598a4->pathing_180 != 0) {
            g_octree_6598a4->pathing_180->LinkSurfaces00460020(this);
            g_octree_6598a4->pathing_180->LinkEdges004600B0(this);
        }
        Initialize(instance, 1, prop_number, footstep_surface, footstep_material);
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
        m_pGDSurfaces = static_cast<W8GDSurface*>(malloc(m_surface_count_14 * sizeof(W8GDSurface)));
        if (m_pGDSurfaces == 0) {
            srAssertFail("m_pGDSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0xa0,
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
            srAssertFail("m_pVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0xae,
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
void GDProp::Initialize(srModelInstance* instance, unsigned char attach, unsigned short prop_number,
                        unsigned char footstep_surface, unsigned char footstep_material)
{
    if (attach == 0) {
        m_flags_00 |= 4;
    } else {
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
        for (int vertex = 0; vertex < mesh->vertex_location_count_22c; ++vertex) {
            m_pVertices[vertex_base + vertex] =
                world_matrix.TransformPoint(source_vertices[vertex]);
        }
        vertex_base += mesh->vertex_location_count_22c;

        for (int surface_index = 0; surface_index < surface_total; ++surface_index) {
            W8GDSurface* surface = &m_pGDSurfaces[surface_index];
            BuildTrianglePlane00449A40(
                reinterpret_cast<srVector4T<float>*>(&surface->plane_24), /* reinterpret-ok:
                    the union's plane arm is a 4-float vector */

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
            surface->footstep_surface_3c = footstep_surface;
            surface->footstep_material_3d = footstep_material;
            surface->hit_plane_38 = 0;
            if ((mesh_flags & 1) != 0) {
                surface->flags_00 |= 0x8000;
            }

            if (g_float_005ebc7c <= surface->plane_24[1]) {
                surface->value_40 = 500.0f;
                surface->flags_00 |= 4;
                if (g_float_005ebccc < surface->plane_24[1]) {
                    surface->slope_48 = 1.0f;
                } else {
                    surface->slope_48 = surface->plane_24[1];
                }
            } else {
                surface->slope_48 = 0.0f;
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
        } else {
            pathing->SetConditionalPathFrame00457EA0(m_path_handle_04, (short)m_prop_number_02);
            if (m_waypoint_count_0a != 0) {
                pathing->CheckConditionalWayPtStatus004601B0(m_waypoint_count_0a, m_waypoints_10);
            }
            if (m_link_count_08 != 0) {
                pathing->CheckConditionalLinkStatus00460250(m_link_count_08, m_links_0c);
            }
        }
    }

    Trigger* owner = m_owner_24;
    if (owner != 0 && attach != 0) {
        W8TriggerActionData* action = owner->m_pActionData;
        if (action != 0 && action->type_004 == 10) {
            unsigned int flags = 0x08000000;
            if ((owner->value_368 != 0 && owner->state_370.state == 0) ||
                ((owner->flags_0a0 & 0x100) == 0 ||
                 (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 5) != 0)) {
                flags = 0x28000000;
            }
            if (pathing != 0) {
                pathing->UpdateConditionalPathFlags00465FB0(m_path_handle_04, m_prop_number_02,
                                                            flags);
            }
        }
    }

    if (m_list_54 != 0) {
        unsigned int count = PLLength(m_list_54);
        for (unsigned int index = 0; index < count; ++index) {
            W8WorldItem* item = static_cast<W8WorldItem*>(PLGet(m_list_54, (int)index));
            if (item != 0) {
                SetWorldItemFlag02(item, 1);
            }
        }
    }
}

/* Attach the product-side prop owner and immediately mirror its active state
   into both the local GD flags and the conditional path table. */
// FUNCTION: WIZ8 0x004b7470
void GDProp::BindTrigger(Trigger* owner)
{
    m_owner_24 = owner;
    W8TriggerActionData* action = owner->m_pActionData;
    if (action != 0 && action->type_004 == 10) {
        if (action != 0) {
            m_flags_00 |= 2;
            unsigned int path_flags = 0x08000000;
            if ((owner->value_368 == 0 || owner->state_370.state != 0) &&
                (owner->flags_0a0 & 0x100) != 0 &&
                (static_cast<W8DoorTriggerActionData*>(action)->flags_008 & 5) == 0) {
                m_flags_00 |= 8;
            } else {
                path_flags = 0x28000000;
                m_flags_00 &= 0xfff7;
            }

            W8PathingService* pathing = g_octree_6598a4->pathing_180;
            if (pathing != 0) {
                pathing->UpdateConditionalPathFlags00465FB0(m_path_handle_04, m_prop_number_02,
                                                            path_flags);
            }
        }
    }
}

/* Scan the transformed vertices for the axis-aligned bounds. The cached box
   feeds the path bookkeeping range/sentinel and is also copied out for the
   caller. */
// FUNCTION: WIZ8 0x004b7500
void GDProp::ComputeBounds004B7500(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    m_bound_max_40 = m_pVertices[0];
    m_bound_min_34 = m_pVertices[0];
    for (int vertex = 1; vertex < m_vertex_count_18; ++vertex) {
        for (int axis = 0; axis < 3; ++axis) {
            if ((&m_bound_min_34.x)[axis] > (&m_pVertices[vertex].x)[axis]) {
                (&m_bound_min_34.x)[axis] = (&m_pVertices[vertex].x)[axis];
            }
            if ((&m_bound_max_40.x)[axis] < (&m_pVertices[vertex].x)[axis]) {
                (&m_bound_max_40.x)[axis] = (&m_pVertices[vertex].x)[axis];
            }
        }
    }

    m_path_range_28.minimum = m_bound_min_34.y;
    m_path_range_28.maximum = m_bound_max_40.y;
    m_path_range_28.sentinel = (m_bound_min_34.y + m_bound_max_40.y) * g_float_005ebc7c;

    *minimum = m_bound_min_34;
    *maximum = m_bound_max_40;
}

/* The pathing record supplies inclusive unsigned coordinate bounds at
   +0x4c..+0x52. */
// FUNCTION: WIZ8 0x004B75F0
unsigned char GDProp::ContainsPathCoordinate004B75F0(unsigned short x, unsigned short y) const
{
    if (x >= m_path_bounds_4c.min_x && x <= m_path_bounds_4c.max_x && y >= m_path_bounds_4c.min_z &&
        y <= m_path_bounds_4c.max_z) {
        return 1;
    }
    return 0;
}

/* Box overlap test against every surface triangle: the query bounds are
   copied into a local two-vector box and each surface's three vertices are
   gathered into a triangle before the spatial triangle test runs. */
// FUNCTION: WIZ8 0x004b7620
char GDProp::BoundsOverlap004B7620(const srVector3T<float>* minimum,
                                   const srVector3T<float>* maximum)
{
    srVector3T<float> bounds[2];
    srVector3T<float> triangle[3];
    unsigned char hit = 0;

    bounds[0] = *minimum;
    bounds[1] = *maximum;
    for (int index = 0; index < m_surface_count_14 && hit == 0; ++index) {
        W8GDSurface* surface = &m_pGDSurfaces[index];
        triangle[0] = m_pVertices[surface->vertex_indices_18[0]];
        triangle[1] = m_pVertices[surface->vertex_indices_18[1]];
        triangle[2] = m_pVertices[surface->vertex_indices_18[2]];
        hit = TestSpatialTriangle0046CE60(bounds, triangle, surface->Normal());
    }
    return hit;
}

/* Record a waypoint index for a grid point that lies inside the path bounds.
   The waypoint list grows in blocks of ten shorts; the allocation message
   names CheckConditionalWayPt because the list is consumed by
   CheckConditionalWayPtStatus. */
// FUNCTION: WIZ8 0x004b7730
unsigned char GDProp::RegisterPathSurface004B7730(unsigned int index, const srVector2i* point)
{
    if (static_cast<unsigned short>(point->x) < m_path_bounds_4c.min_x ||
        static_cast<unsigned short>(point->x) > m_path_bounds_4c.max_x ||
        static_cast<unsigned short>(point->y) < m_path_bounds_4c.min_z ||
        static_cast<unsigned short>(point->y) > m_path_bounds_4c.max_z) {
        return 0;
    }

    if (m_waypoint_count_0a % 10 == 0) {
        unsigned short* pusNewList = static_cast<unsigned short*>(
            malloc((m_waypoint_count_0a + 10) * sizeof(unsigned short)));
        if (pusNewList == 0) {
            srAssertFail("pusNewList", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x242,
                         "CheckConditionalWayPt: Couldn't allocate new waypt list.");
        }
        memset(pusNewList, 0, (m_waypoint_count_0a + 10) * sizeof(unsigned short));
        if (m_waypoints_10 != 0) {
            memcpy(pusNewList, m_waypoints_10, m_waypoint_count_0a * sizeof(unsigned short));
            free(m_waypoints_10);
        }
        m_waypoints_10 = pusNewList;
    }
    m_waypoints_10[m_waypoint_count_0a] = static_cast<unsigned short>(index);
    ++m_waypoint_count_0a;
    return 1;
}

/* Record a link index when the segment between the two grid points touches
   the path bounds. The clip walks the segment's major axis: the crossed
   minor bound interpolates the other coordinate, which must land within one
   cell of the rectangle. The vertical branch re-tests its first crossing
   instead of interpolating the second — a faithful retail oddity. */
// FUNCTION: WIZ8 0x004b7830
unsigned char GDProp::RegisterPathVertex004B7830(unsigned int index, const srVector2i* point,
                                                 const srVector2i* second)
{
    unsigned short x0 = static_cast<unsigned short>(point->x);
    unsigned short y0 = static_cast<unsigned short>(point->y);
    unsigned short x1 = static_cast<unsigned short>(second->x);
    unsigned short y1 = static_cast<unsigned short>(second->y);
    float dx = static_cast<float>(x1 - x0);
    float dy = static_cast<float>(y1 - y0);
    bool found = false;

    if (fabs(dx) > fabs(dy)) {
        float slope = dy / dx;
        float cross = (m_path_bounds_4c.min_x - x0) * slope + y0;
        if (cross > m_path_bounds_4c.min_z - 1 && cross < m_path_bounds_4c.max_z + 1) {
            found = true;
        }
        cross = (m_path_bounds_4c.max_x - x0) * slope + y0;
        if (cross > m_path_bounds_4c.min_z - 1 && cross < m_path_bounds_4c.max_z + 1) {
            found = true;
        }
    } else {
        float slope = dx / dy;
        float cross = (m_path_bounds_4c.min_z - y0) * slope + x0;
        if (cross > m_path_bounds_4c.min_x - 1 && cross < m_path_bounds_4c.max_x + 1) {
            found = true;
        }
        if (cross > m_path_bounds_4c.min_x - 1 && cross < m_path_bounds_4c.max_x + 1) {
            found = true;
        }
    }
    if (found == 0) {
        return 0;
    }

    if (m_link_count_08 % 10 == 0) {
        unsigned short* pusNewList =
            static_cast<unsigned short*>(malloc((m_link_count_08 + 10) * sizeof(unsigned short)));
        if (pusNewList == 0) {
            srAssertFail("pusNewList", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x28d,
                         "CheckConditionalWayPt: Couldn't allocate new waypt list.");
        }
        memset(pusNewList, 0, (m_link_count_08 + 10) * sizeof(unsigned short));
        if (m_links_0c != 0) {
            memcpy(pusNewList, m_links_0c, m_link_count_08 * sizeof(unsigned short));
            free(m_links_0c);
        }
        m_links_0c = pusNewList;
    }
    m_links_0c[m_link_count_08] = static_cast<unsigned short>(index);
    ++m_link_count_08;
    return 1;
}

/* Register the item on the collidable prop's sector list, building the path
   representation and the list itself on first use. */
// FUNCTION: WIZ8 0x004b7ad0
void AddItemToSector(int sector, W8WorldItem* item)
{
    if (item != 0 && sector >= 0) {
        W8Prop* prop = *g_world->collidable_props->GetAt(sector);
        GDProp* gd = prop->m_gd_prop;
        if (gd == 0) {
            prop->BuildOrRefreshPathingRepresentation();
            gd = prop->m_gd_prop;
            if (gd == 0) {
                return;
            }
        }
        if (gd->m_list_54 == 0) {
            gd->m_list_54 = PLCreate();
        }
        if (gd->m_list_54 != 0 && PListIndexOf(gd->m_list_54, item) < 0) {
            PLAdoptAppend(gd->m_list_54, item);
        }
    }
}

/* Drop the item from the collidable prop's sector list when both exist. */
// FUNCTION: WIZ8 0x004b7b50
void RemoveItemFromSector(int sector, W8WorldItem* item)
{
    if (item != 0 && sector >= 0) {
        W8Prop* prop = *g_world->collidable_props->GetAt(sector);
        if (prop != 0 && prop->m_gd_prop != 0 && prop->m_gd_prop->m_list_54 != 0) {
            PListRemove(prop->m_gd_prop->m_list_54, item);
        }
    }
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

/* The array-element ctor for `new GDPreProp[n]`; the base default runs
   inlined, then the extra frame field is zeroed. */
// FUNCTION: WIZ8 0x004b7bc0
GDPreProp::GDPreProp()
{
    last_frame_58 = 0;
}

/* Rebuild the collision geometry for one animation frame. The frame index is
   clamped against every transform channel's path count and stored as the
   conditional frame number; the counts accumulated for allocation are then
   reset so each transform's fill re-counts the appended geometry. */
// FUNCTION: WIZ8 0x004b7c00
void GDProp::ApplyAnimFrame004B7C00(unsigned short frame, W8LevelFileAnimObj* anim)
{
    signed char index;

    for (index = 0; index < anim->num_transforms_5a; ++index) {
        W8LevelFileTransform* transform = &anim->pTransforms_5b[index];
        if (frame >= transform->pathAI_06.path_count_0a) {
            frame = transform->pathAI_06.path_count_0a - 1;
        }
    }
    m_prop_number_02 = frame;

    for (index = 0; index < anim->num_transforms_5a; ++index) {
        W8LevelFileMesh* mesh = &anim->pTransforms_5b[index].LODMesh_02.pFrames->mesh_01;
        if ((mesh->flags_0c & 1) != 0) {
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x355,
                         "Transform prop made collideable!!!");
            m_surface_count_14 += mesh->num_lods_42 * mesh->num_faces_08;
            m_vertex_count_18 += mesh->num_lods_42 * mesh->num_vertices_04;
        } else {
            m_surface_count_14 += mesh->num_faces_08;
            m_vertex_count_18 += mesh->num_vertices_04;
        }
    }

    m_pVertices = static_cast<srVector3T<float>*>(
        srHeap.allocate(m_vertex_count_18 * sizeof(srVector3T<float>)));
    m_pGDSurfaces = static_cast<W8GDSurface*>(malloc(m_surface_count_14 * sizeof(W8GDSurface)));
    if (m_pGDSurfaces == 0) {
        srAssertFail("m_pGDSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x364,
                     0);
    }
    memset(m_pGDSurfaces, 0, m_surface_count_14 * sizeof(W8GDSurface));
    m_vertex_count_18 = 0;
    m_surface_count_14 = 0;

    for (index = 0; index < anim->num_transforms_5a; ++index) {
        W8LevelFileTransform* transform = &anim->pTransforms_5b[index];
        W8LevelFileScaledPathNode node;
        if (transform->pathAI_06.scaled_01 == 2) {
            node = transform->pathAI_06.pScaledPaths[frame];
        } else {
            node.path = transform->pathAI_06.pPaths[frame];
            node.scale.x = node.scale.y = node.scale.z = 1.0f;
        }
        TransformMeshGeometry004B7E50(&node, &transform->LODMesh_02.pFrames->mesh_01);
    }
}

/* Append one transform channel's geometry under the path record's affine
   transform: position and scale convert through the world scale (LOD meshes
   carry their own compression factor), the axis/angle supplies the rotation,
   and every appended surface then receives its plane, dominant axis and slope
   classification. */
// FUNCTION: WIZ8 0x004b7e50
void GDProp::TransformMeshGeometry004B7E50(const W8LevelFileScaledPathNode* node,
                                           W8LevelFileMesh* mesh)
{
    int surface_base = m_surface_count_14;

    srVector3T<float> axis = node->path.axis_10;
    srMatrix3T<float> rotation;
    srMatrix4x3T<float> matrix;
    srVector3T<float> translation;
    srVector3T<float> scale;
    float factor;

    rotation.SetIdentity();
    if (node->path.angle_0c != g_zero_005ebb40) {
        rotation.RotateAroundAxis(sin(node->path.angle_0c), cos(node->path.angle_0c), axis);
    }
    translation.x = node->path.position_00.x * g_double_005ec150;
    translation.y = node->path.position_00.y * g_double_005ec150;
    translation.z = node->path.position_00.z * g_double_005ec150;

    if ((mesh->flags_0c & 1) != 0 && (mesh->flags_0c & 2) != 0) {
        factor = mesh->lod_scale_58 * g_world_scale_005ebc40;
    } else {
        factor = static_cast<float>(g_double_005ec150);
    }
    scale.x = node->scale.x * factor;
    scale.y = node->scale.y * factor;
    scale.z = node->scale.z * factor;

    matrix.SetRotation(rotation);
    matrix.SetTranslation(translation);
    matrix.Scale(scale);

    if ((mesh->flags_0c & 1) != 0) {
        for (int lod = 0; lod < mesh->num_lods_42; ++lod) {
            int vertex_base = m_vertex_count_18;
            if ((mesh->flags_0c & 2) != 0) {
                short* vertices = mesh->lod_shorts_44[lod];
                for (int vertex = 0; vertex < mesh->num_vertices_04; ++vertex) {
                    m_pVertices[m_vertex_count_18] = matrix.TransformPoint(
                        srVector3T<float>(static_cast<float>(vertices[vertex * 3]),
                                          static_cast<float>(vertices[vertex * 3 + 1]),
                                          static_cast<float>(vertices[vertex * 3 + 2])));
                    ++m_vertex_count_18;
                }
            } else {
                float* vertices = mesh->lods_48[lod];
                for (int vertex = 0; vertex < mesh->num_vertices_04; ++vertex) {
                    m_pVertices[m_vertex_count_18] = matrix.TransformPoint(srVector3T<float>(
                        vertices[vertex * 3], vertices[vertex * 3 + 1], vertices[vertex * 3 + 2]));
                    ++m_vertex_count_18;
                }
            }
            if ((mesh->flags_0c & 2) != 0) {
                W8LevelFileCompressedFace* faces = mesh->pstCompFaces;
                for (int face = 0; face < mesh->num_faces_08; ++face) {
                    W8GDSurface* surface = &m_pGDSurfaces[m_surface_count_14];
                    ++m_surface_count_14;
                    surface->vertex_indices_18[0] = faces[face].vertex_indices_00[0] + vertex_base;
                    surface->vertex_indices_18[1] = faces[face].vertex_indices_00[1] + vertex_base;
                    surface->vertex_indices_18[2] = faces[face].vertex_indices_00[2] + vertex_base;
                }
            } else {
                W8ReadMeshFace* faces = mesh->pstFaces;
                for (int face = 0; face < mesh->num_faces_08; ++face) {
                    W8GDSurface* surface = &m_pGDSurfaces[m_surface_count_14];
                    ++m_surface_count_14;
                    surface->vertex_indices_18[0] = faces[face].vertices[0] + vertex_base;
                    surface->vertex_indices_18[1] = faces[face].vertices[1] + vertex_base;
                    surface->vertex_indices_18[2] = faces[face].vertices[2] + vertex_base;
                }
            }
        }
    } else {
        int vertex_base = m_vertex_count_18;
        float* vertices = mesh->pstVertices;
        for (int vertex = 0; vertex < mesh->num_vertices_04; ++vertex) {
            m_pVertices[m_vertex_count_18] = matrix.TransformPoint(srVector3T<float>(
                vertices[vertex * 3], vertices[vertex * 3 + 1], vertices[vertex * 3 + 2]));
            ++m_vertex_count_18;
        }
        if ((mesh->flags_0c & 2) != 0) {
            W8LevelFileCompressedFace* faces = mesh->pstCompFaces;
            for (int face = 0; face < mesh->num_faces_08; ++face) {
                W8GDSurface* surface = &m_pGDSurfaces[m_surface_count_14];
                ++m_surface_count_14;
                surface->vertex_indices_18[0] = faces[face].vertex_indices_00[0] + vertex_base;
                surface->vertex_indices_18[1] = faces[face].vertex_indices_00[1] + vertex_base;
                surface->vertex_indices_18[2] = faces[face].vertex_indices_00[2] + vertex_base;
            }
        } else {
            W8ReadMeshFace* faces = mesh->pstFaces;
            for (int face = 0; face < mesh->num_faces_08; ++face) {
                W8GDSurface* surface = &m_pGDSurfaces[m_surface_count_14];
                ++m_surface_count_14;
                surface->vertex_indices_18[0] = faces[face].vertices[0] + vertex_base;
                surface->vertex_indices_18[1] = faces[face].vertices[1] + vertex_base;
                surface->vertex_indices_18[2] = faces[face].vertices[2] + vertex_base;
            }
        }
    }

    for (int index = surface_base; index < m_surface_count_14; ++index) {
        W8GDSurface* surface = &m_pGDSurfaces[index];
        BuildTrianglePlane00449A40(
            reinterpret_cast<srVector4T<float>*>(&surface->plane_24), /* reinterpret-ok:
                    the union's plane arm is a 4-float vector */
            &m_pVertices[surface->vertex_indices_18[0]],
            &m_pVertices[surface->vertex_indices_18[1]],
            &m_pVertices[surface->vertex_indices_18[2]]);

        int dominant_axis;
        float largest = g_float_005ebb34;
        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = static_cast<float>(fabs(surface->plane_24[axis]));
            if (largest < magnitude) {
                largest = magnitude;
                dominant_axis = axis;
            }
        }
        surface->flags_00 = dominant_axis + 0x800;
        surface->hit_plane_38 = 0;
        if (g_float_005ebc7c <= surface->plane_24[1]) {
            surface->value_40 = 500.0f;
            surface->flags_00 |= 4;
        } else {
            surface->slope_48 = 0.0f;
            surface->value_40 = 500.0f;
        }
    }
}

/* srMatrix4x3T<float>::SetTranslation/Scale emitted for this TU by
   TransformMeshGeometry004B7E50; the primary templates live in srMath.h. */
// TEMPLATE: WIZ8 0x004B8660
// srMatrix4x3T<float>::SetTranslation

// TEMPLATE: WIZ8 0x004B8680
// srMatrix4x3T<float>::Scale
