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
const float g_float_005ebccc = 0.75f;

// FUNCTION: WIZ8 0x004b6e00
GDProp::GDProp(srModelInstance* instance, const char* path_name, unsigned short prop_number,
               unsigned char footstep_surface, unsigned char footstep_material)
{
    m_flags = 0;
    m_path_handle = 0;
    m_prop_number = 0;
    m_vertex_count = 0;
    m_surface_count = 0;
    m_pGDSurfaces = 0;
    m_pVertices = 0;
    m_trigger = 0;
    m_path_edge_count = 0;
    m_path_waypoint_count = 0;
    m_path_edges = 0;
    m_path_waypoints = 0;
    m_supported_items = 0;
    m_path_range.sentinel = -10000000.0f;
    m_path_bounds.max_z = 0;
    m_path_bounds.min_z = 0;
    m_path_bounds.max_x = 0;
    m_path_bounds.min_x = 0;

    if (g_octree != 0 && g_octree->pathing != 0) {
        m_path_handle = g_octree->pathing->FindPathHandle(path_name, &m_path_bounds, &m_path_range);
    }

    if (instance != 0) {
        if (m_path_handle != 0 && g_octree->pathing != 0) {
            g_octree->pathing->LinkSurfaces(this);
            g_octree->pathing->LinkEdges(this);
        }
        Initialize(instance, true, prop_number, footstep_surface, footstep_material);
    }
}

// FUNCTION: WIZ8 0x004b6ed0
GDProp::~GDProp()
{
    if (m_pGDSurfaces != 0) {
        free(m_pGDSurfaces);
    }
    if (m_pVertices != 0) {
        delete[] m_pVertices;
    }
    if (m_path_edges != 0) {
        free(m_path_edges);
    }
    if (m_path_waypoints != 0) {
        free(m_path_waypoints);
    }
    if (m_supported_items != 0) {
        PLDestroy(m_supported_items);
    }
}

/* Resize the two geometry tables to the complete linked mesh. Surfaces use
   the CRT heap, while vertices retain SurRender's heap ownership. */
// FUNCTION: WIZ8 0x004b6f30
void GDProp::PrepareGeometry(srModelInstance* instance)
{
    int surface_count = 0;
    int vertex_count = 0;
    stMeshModel* mesh = static_cast<stMeshModel*>(instance->getModel());
    while (mesh != 0) {
        surface_count += mesh->polygon_count;
        vertex_count += mesh->vertex_location_count;
        mesh = mesh->next;
    }

    if (m_surface_count != surface_count) {
        m_surface_count = surface_count;
        if (m_pGDSurfaces != 0) {
            free(m_pGDSurfaces);
        }
        m_pGDSurfaces = static_cast<W8GDSurface*>(malloc(m_surface_count * sizeof(W8GDSurface)));
        if (m_pGDSurfaces == 0) {
            srAssertFail("m_pGDSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0xa0,
                         0);
        }
        memset(m_pGDSurfaces, 0, m_surface_count * sizeof(W8GDSurface));
    }

    if (m_vertex_count != vertex_count) {
        m_vertex_count = vertex_count;
        if (m_pVertices != 0) {
            delete[] m_pVertices;
        }
        m_pVertices = new srVector3T<float>[m_vertex_count];
        if (m_pVertices == 0) {
            srAssertFail("m_pVertices", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0xae,
                         0);
        }
        memset(m_pVertices, 0, m_vertex_count * sizeof(srVector3T<float>));
    }
}

/* Rebuild the prop's collision geometry from its current linked mesh and
   publish the corresponding conditional path state. Vertices are transformed
   into world space before each accumulated triangle receives its plane,
   dominant axis, slope classification and caller-provided material bytes. */
// FUNCTION: WIZ8 0x004b7060
void GDProp::Initialize(srModelInstance* instance, bool attach, unsigned short prop_number,
                        unsigned char footstep_surface, unsigned char footstep_material)
{
    if (!attach) {
        m_flags |= W8_GD_PROP_UNATTACHED;
    } else {
        m_flags &= 0xffffu & ~W8_GD_PROP_UNATTACHED;
        m_prop_number = prop_number;
    }

    PrepareGeometry(instance);
    srMatrix4T<float> world_matrix;
    instance->getWorldSpaceMatrix(world_matrix);

    int vertex_base = 0;
    int surface_total = 0;
    stMeshModel* mesh = static_cast<stMeshModel*>(instance->getModel());
    while (mesh != 0) {
        unsigned int mesh_flags = mesh->flags;
        srVector3i* polygon_vertices = mesh->getPolyVertex();
        for (int polygon = 0; polygon < mesh->polygon_count; ++polygon) {
            W8GDSurface* surface = &m_pGDSurfaces[surface_total + polygon];
            surface->vertex_indices[0] = polygon_vertices[polygon].x + vertex_base;
            surface->vertex_indices[1] = polygon_vertices[polygon].y + vertex_base;
            surface->vertex_indices[2] = polygon_vertices[polygon].z + vertex_base;
        }
        surface_total += mesh->polygon_count;

        srVector3T<float>* source_vertices = mesh->getVertexLoc();
        for (int vertex = 0; vertex < mesh->vertex_location_count; ++vertex) {
            m_pVertices[vertex_base + vertex] =
                world_matrix.TransformPoint(source_vertices[vertex]);
        }
        vertex_base += mesh->vertex_location_count;

        for (int surface_index = 0; surface_index < surface_total; ++surface_index) {
            W8GDSurface* surface = &m_pGDSurfaces[surface_index];
            BuildTrianglePlane(&surface->plane, &m_pVertices[surface->vertex_indices[0]],
                               &m_pVertices[surface->vertex_indices[1]],
                               &m_pVertices[surface->vertex_indices[2]]);

            int dominant_axis;
            float largest = 0.0f;
            for (int axis = 0; axis < 3; ++axis) {
                float magnitude = static_cast<float>(fabs((&surface->plane.normal.x)[axis]));
                if (largest < magnitude) {
                    largest = magnitude;
                    dominant_axis = axis;
                }
            }
            surface->flags = dominant_axis + W8_GD_SURFACE_PROP_GEOMETRY;
            surface->footstep_surface = footstep_surface;
            surface->footstep_material = footstep_material;
            surface->hit_plane = 0;
            if ((mesh_flags & W8_MESH_SORTED_RENDERING) != 0) {
                surface->flags |= W8_GD_SURFACE_SKIP_FILTERED_TRACE;
            }

            if (g_float_005ebc7c <= surface->plane.normal.y) {
                surface->contact_margin = 500.0f;
                surface->flags |= W8_GD_SURFACE_WALKABLE;
                if (g_float_005ebccc < surface->plane.normal.y) {
                    surface->slope = 1.0f;
                } else {
                    surface->slope = surface->plane.normal.y;
                }
            } else {
                surface->slope = 0.0f;
                surface->contact_margin = 500.0f;
            }
        }
        mesh = mesh->next;
    }

    W8PathingService* pathing = g_octree->pathing;
    if (m_path_handle != 0 && pathing != 0) {
        if (!attach) {
            if (m_prop_number != 0xffff) {
                m_prop_number = 0xffff;
                pathing->SetConditionalPathFrame(m_path_handle, 0xffff);
            }
        } else {
            pathing->SetConditionalPathFrame(m_path_handle, m_prop_number);
            if (m_path_waypoint_count != 0) {
                pathing->CheckConditionalWayPtStatus(m_path_waypoint_count, m_path_waypoints);
            }
            if (m_path_edge_count != 0) {
                pathing->CheckConditionalLinkStatus(m_path_edge_count, m_path_edges);
            }
        }
    }

    Trigger* owner = m_trigger;
    if (owner != 0 && attach) {
        W8TriggerActionData* action = owner->m_pActionData;
        if (action != 0 && action->type == W8_TRIGGER_PAYLOAD_DOOR) {
            unsigned int flags = W8_PATH_CELL_DOOR;
            if ((owner->lock_state.lock_type != 0 &&
                 owner->lock_state.device_state.completed == 0) ||
                ((owner->flags & W8_TRIGGER_ENABLED) == 0 ||
                 (static_cast<W8DoorTriggerActionData*>(action)->door_flags &
                  (W8_DOOR_OPEN | W8_DOOR_KEY_REQUIRED)) != 0)) {
                flags = W8_PATH_CELL_DOOR | W8_PATH_CELL_BLOCKED;
            }
            if (pathing != 0) {
                pathing->UpdateConditionalPathFlags(m_path_handle, m_prop_number, flags);
            }
        }
    }

    if (m_supported_items != 0) {
        unsigned int count = PLLength(m_supported_items);
        for (unsigned int index = 0; index < count; ++index) {
            W8WorldItem* item =
                static_cast<W8WorldItem*>(PLGet(m_supported_items, static_cast<int>(index)));
            if (item != 0) {
                SetWorldItemFalling(item, true);
            }
        }
    }
}

/* Attach the product-side prop owner and immediately mirror its active state
   into both the local GD flags and the conditional path table. */
// FUNCTION: WIZ8 0x004b7470
void GDProp::BindTrigger(Trigger* owner)
{
    m_trigger = owner;
    W8TriggerActionData* action = owner->m_pActionData;
    if (action != 0 && action->type == W8_TRIGGER_PAYLOAD_DOOR) {
        if (action != 0) {
            m_flags |= W8_GD_PROP_DOOR;
            unsigned int path_flags = W8_PATH_CELL_DOOR;
            if ((owner->lock_state.lock_type == 0 ||
                 owner->lock_state.device_state.completed != 0) &&
                (owner->flags & W8_TRIGGER_ENABLED) != 0 &&
                (static_cast<W8DoorTriggerActionData*>(action)->door_flags &
                 (W8_DOOR_OPEN | W8_DOOR_KEY_REQUIRED)) == 0) {
                m_flags |= W8_GD_PROP_DOOR_USABLE;
            } else {
                path_flags = W8_PATH_CELL_DOOR | W8_PATH_CELL_BLOCKED;
                m_flags &= 0xffffu & ~W8_GD_PROP_DOOR_USABLE;
            }

            W8PathingService* pathing = g_octree->pathing;
            if (pathing != 0) {
                pathing->UpdateConditionalPathFlags(m_path_handle, m_prop_number, path_flags);
            }
        }
    }
}

/* Scan the transformed vertices for the axis-aligned bounds. The cached box
   feeds the path bookkeeping range/sentinel and is also copied out for the
   caller. */
// FUNCTION: WIZ8 0x004b7500
void GDProp::ComputeBounds(srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    m_bound_max = m_pVertices[0];
    m_bound_min = m_pVertices[0];
    for (int vertex = 1; vertex < m_vertex_count; ++vertex) {
        for (int axis = 0; axis < 3; ++axis) {
            if ((&m_bound_min.x)[axis] > (&m_pVertices[vertex].x)[axis]) {
                (&m_bound_min.x)[axis] = (&m_pVertices[vertex].x)[axis];
            }
            if ((&m_bound_max.x)[axis] < (&m_pVertices[vertex].x)[axis]) {
                (&m_bound_max.x)[axis] = (&m_pVertices[vertex].x)[axis];
            }
        }
    }

    m_path_range.minimum = m_bound_min.y;
    m_path_range.maximum = m_bound_max.y;
    m_path_range.sentinel = (m_bound_min.y + m_bound_max.y) * g_float_005ebc7c;

    *minimum = m_bound_min;
    *maximum = m_bound_max;
}

/* The pathing record supplies inclusive unsigned coordinate bounds at
   +0x4c..+0x52. */
// FUNCTION: WIZ8 0x004B75F0
bool GDProp::ContainsPathCoordinate(unsigned short x, unsigned short y) const
{
    if (x >= m_path_bounds.min_x && x <= m_path_bounds.max_x && y >= m_path_bounds.min_z &&
        y <= m_path_bounds.max_z) {
        return true;
    }
    return false;
}

/* Box overlap test against every surface triangle: the query bounds are
   copied into a local two-vector box and each surface's three vertices are
   gathered into a triangle before the spatial triangle test runs. */
// FUNCTION: WIZ8 0x004b7620
char GDProp::BoundsOverlap(const srVector3T<float>* minimum, const srVector3T<float>* maximum)
{
    srVector3T<float> bounds[2];
    srVector3T<float> triangle[3];
    unsigned char hit = 0;

    bounds[0] = *minimum;
    bounds[1] = *maximum;
    for (int index = 0; index < m_surface_count && hit == 0; ++index) {
        W8GDSurface* surface = &m_pGDSurfaces[index];
        triangle[0] = m_pVertices[surface->vertex_indices[0]];
        triangle[1] = m_pVertices[surface->vertex_indices[1]];
        triangle[2] = m_pVertices[surface->vertex_indices[2]];
        hit = TestSpatialTriangle(bounds, triangle, surface->Normal());
    }
    return hit;
}

/* Record a waypoint index for a grid point that lies inside the path bounds.
   The waypoint list grows in blocks of ten shorts; the allocation message
   names CheckConditionalWayPt because the list is consumed by
   CheckConditionalWayPtStatus. */
// FUNCTION: WIZ8 0x004b7730
unsigned char GDProp::RegisterPathSurface(unsigned int index, const srVector2i* point)
{
    if (static_cast<unsigned short>(point->x) < m_path_bounds.min_x ||
        static_cast<unsigned short>(point->x) > m_path_bounds.max_x ||
        static_cast<unsigned short>(point->y) < m_path_bounds.min_z ||
        static_cast<unsigned short>(point->y) > m_path_bounds.max_z) {
        return 0;
    }

    if (m_path_waypoint_count % 10 == 0) {
        unsigned short* pusNewList = static_cast<unsigned short*>(
            malloc((m_path_waypoint_count + 10) * sizeof(unsigned short)));
        if (pusNewList == 0) {
            srAssertFail("pusNewList", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x242,
                         "CheckConditionalWayPt: Couldn't allocate new waypt list.");
        }
        memset(pusNewList, 0, (m_path_waypoint_count + 10) * sizeof(unsigned short));
        if (m_path_waypoints != 0) {
            memcpy(pusNewList, m_path_waypoints, m_path_waypoint_count * sizeof(unsigned short));
            free(m_path_waypoints);
        }
        m_path_waypoints = pusNewList;
    }
    m_path_waypoints[m_path_waypoint_count] = static_cast<unsigned short>(index);
    ++m_path_waypoint_count;
    return 1;
}

/* Record a link index when the segment between the two grid points touches
   the path bounds. The clip walks the segment's major axis: the crossed
   minor bound interpolates the other coordinate, which must land within one
   cell of the rectangle. The vertical branch re-tests its first crossing
   instead of interpolating the second — a faithful retail oddity. */
// FUNCTION: WIZ8 0x004b7830
unsigned char GDProp::RegisterPathVertex(unsigned int index, const srVector2i* point,
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
        float cross = (m_path_bounds.min_x - x0) * slope + y0;
        if (cross > m_path_bounds.min_z - 1 && cross < m_path_bounds.max_z + 1) {
            found = true;
        }
        cross = (m_path_bounds.max_x - x0) * slope + y0;
        if (cross > m_path_bounds.min_z - 1 && cross < m_path_bounds.max_z + 1) {
            found = true;
        }
    } else {
        float slope = dx / dy;
        float cross = (m_path_bounds.min_z - y0) * slope + x0;
        if (cross > m_path_bounds.min_x - 1 && cross < m_path_bounds.max_x + 1) {
            found = true;
        }
        if (cross > m_path_bounds.min_x - 1 && cross < m_path_bounds.max_x + 1) {
            found = true;
        }
    }
    if (!found) {
        return 0;
    }

    if (m_path_edge_count % 10 == 0) {
        unsigned short* pusNewList =
            static_cast<unsigned short*>(malloc((m_path_edge_count + 10) * sizeof(unsigned short)));
        if (pusNewList == 0) {
            srAssertFail("pusNewList", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x28d,
                         "CheckConditionalWayPt: Couldn't allocate new waypt list.");
        }
        memset(pusNewList, 0, (m_path_edge_count + 10) * sizeof(unsigned short));
        if (m_path_edges != 0) {
            memcpy(pusNewList, m_path_edges, m_path_edge_count * sizeof(unsigned short));
            free(m_path_edges);
        }
        m_path_edges = pusNewList;
    }
    m_path_edges[m_path_edge_count] = static_cast<unsigned short>(index);
    ++m_path_edge_count;
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
        if (gd->m_supported_items == 0) {
            gd->m_supported_items = PLCreate();
        }
        if (gd->m_supported_items != 0 && PListIndexOf(gd->m_supported_items, item) < 0) {
            PLAdoptAppend(gd->m_supported_items, item);
        }
    }
}

/* Drop the item from the collidable prop's sector list when both exist. */
// FUNCTION: WIZ8 0x004b7b50
void RemoveItemFromSector(int sector, W8WorldItem* item)
{
    if (item != 0 && sector >= 0) {
        W8Prop* prop = *g_world->collidable_props->GetAt(sector);
        if (prop != 0 && prop->m_gd_prop != 0 && prop->m_gd_prop->m_supported_items != 0) {
            PListRemove(prop->m_gd_prop->m_supported_items, item);
        }
    }
}

/* Whether the optional owned list currently contains an entry. */
// FUNCTION: WIZ8 0x004B7BA0
bool GDProp::HasSupportedItems()
{
    if (m_supported_items != 0 && static_cast<int>(PLLength(m_supported_items)) > 0) {
        return true;
    }
    return false;
}

/* The array-element ctor for `new GDPreProp[n]`; the base default runs
   inlined, then the extra frame field is zeroed. */
// FUNCTION: WIZ8 0x004b7bc0
GDPreProp::GDPreProp()
{
    last_frame = 0;
}

/* Rebuild the collision geometry for one animation frame. The frame index is
   clamped against every transform channel's path count and stored as the
   conditional frame number; the counts accumulated for allocation are then
   reset so each transform's fill re-counts the appended geometry. */
// FUNCTION: WIZ8 0x004b7c00
void GDProp::ApplyAnimFrame(unsigned short frame, W8LevelFileAnimObj* anim)
{
    signed char index;

    for (index = 0; index < anim->num_transforms; ++index) {
        W8LevelFileTransform* transform = &anim->pTransforms[index];
        if (frame >= transform->pathAI.path_count) {
            frame = transform->pathAI.path_count - 1;
        }
    }
    m_prop_number = frame;

    for (index = 0; index < anim->num_transforms; ++index) {
        W8LevelFileMesh* mesh = &anim->pTransforms[index].LODMesh.pFrames->mesh;
        if ((mesh->flags & W8_LEVEL_MESH_LOD_VERTICES) != 0) {
            srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x355,
                         "Transform prop made collideable!!!");
            m_surface_count += mesh->num_lods * mesh->num_faces;
            m_vertex_count += mesh->num_lods * mesh->num_vertices;
        } else {
            m_surface_count += mesh->num_faces;
            m_vertex_count += mesh->num_vertices;
        }
    }

    m_pVertices = new srVector3T<float>[m_vertex_count];
    m_pGDSurfaces = static_cast<W8GDSurface*>(malloc(m_surface_count * sizeof(W8GDSurface)));
    if (m_pGDSurfaces == 0) {
        srAssertFail("m_pGDSurfaces", "C:\\Projects\\Wizardry 8\\Engine Code\\GDProp.cpp", 0x364,
                     0);
    }
    memset(m_pGDSurfaces, 0, m_surface_count * sizeof(W8GDSurface));
    m_vertex_count = 0;
    m_surface_count = 0;

    for (index = 0; index < anim->num_transforms; ++index) {
        W8LevelFileTransform* transform = &anim->pTransforms[index];
        W8LevelFileScaledPathNode node;
        if (transform->pathAI.scaled == 2) {
            node = transform->pathAI.pScaledPaths[frame];
        } else {
            node.path = transform->pathAI.pPaths[frame];
            node.scale.x = node.scale.y = node.scale.z = 1.0f;
        }
        TransformMeshGeometry(&node, &transform->LODMesh.pFrames->mesh);
    }
}

/* Append one transform channel's geometry under the path record's affine
   transform: position and scale convert through the world scale (LOD meshes
   carry their own compression factor), the axis/angle supplies the rotation,
   and every appended surface then receives its plane, dominant axis and slope
   classification. */
// FUNCTION: WIZ8 0x004b7e50
void GDProp::TransformMeshGeometry(const W8LevelFileScaledPathNode* node, W8LevelFileMesh* mesh)
{
    int surface_base = m_surface_count;

    srVector3T<float> axis = node->path.axis;
    srMatrix3T<float> rotation;
    srMatrix4x3T<float> matrix;
    srVector3T<float> translation;
    srVector3T<float> scale;
    float factor;

    rotation.SetIdentity();
    if (node->path.angle != g_double_zero) {
        rotation.RotateAroundAxis(sin(node->path.angle), cos(node->path.angle), axis);
    }
    translation.Set(node->path.position.x * g_double_005ec150,
                    node->path.position.y * g_double_005ec150,
                    node->path.position.z * g_double_005ec150);

    if ((mesh->flags & W8_LEVEL_MESH_LOD_VERTICES) != 0 &&
        (mesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) != 0) {
        factor = mesh->lod_scale * g_world_scale;
    } else {
        factor = static_cast<float>(g_double_005ec150);
    }
    scale = node->scale * factor;

    matrix.SetRotation(rotation);
    matrix.SetTranslation(translation);
    matrix.Scale(scale);

    if ((mesh->flags & W8_LEVEL_MESH_LOD_VERTICES) != 0) {
        for (int lod = 0; lod < mesh->num_lods; ++lod) {
            int vertex_base = m_vertex_count;
            if ((mesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) != 0) {
                short* vertices = mesh->lod_shorts[lod];
                for (int vertex = 0; vertex < mesh->num_vertices; ++vertex) {
                    m_pVertices[m_vertex_count] = matrix.TransformPoint(
                        srVector3T<float>(static_cast<float>(vertices[vertex * 3]),
                                          static_cast<float>(vertices[vertex * 3 + 1]),
                                          static_cast<float>(vertices[vertex * 3 + 2])));
                    ++m_vertex_count;
                }
            } else {
                const srVector3T<float>* vertices = mesh->lods[lod];
                for (int vertex = 0; vertex < mesh->num_vertices; ++vertex) {
                    m_pVertices[m_vertex_count] = matrix.TransformPoint(vertices[vertex]);
                    ++m_vertex_count;
                }
            }
            /* Retail tests the short-LOD bit here for the face representation. */
            if ((mesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) != 0) {
                W8LevelFileCompressedFace* faces = mesh->pstCompFaces;
                for (int face = 0; face < mesh->num_faces; ++face) {
                    W8GDSurface* surface = &m_pGDSurfaces[m_surface_count];
                    ++m_surface_count;
                    surface->vertex_indices[0] = faces[face].vertex_indices[0] + vertex_base;
                    surface->vertex_indices[1] = faces[face].vertex_indices[1] + vertex_base;
                    surface->vertex_indices[2] = faces[face].vertex_indices[2] + vertex_base;
                }
            } else {
                W8ReadMeshFace* faces = mesh->pstFaces;
                for (int face = 0; face < mesh->num_faces; ++face) {
                    W8GDSurface* surface = &m_pGDSurfaces[m_surface_count];
                    ++m_surface_count;
                    surface->vertex_indices[0] = faces[face].vertices[0] + vertex_base;
                    surface->vertex_indices[1] = faces[face].vertices[1] + vertex_base;
                    surface->vertex_indices[2] = faces[face].vertices[2] + vertex_base;
                }
            }
        }
    } else {
        int vertex_base = m_vertex_count;
        const srVector3T<float>* vertices = mesh->pstVertices;
        for (int vertex = 0; vertex < mesh->num_vertices; ++vertex) {
            m_pVertices[m_vertex_count] = matrix.TransformPoint(vertices[vertex]);
            ++m_vertex_count;
        }
        if ((mesh->flags & W8_LEVEL_MESH_SHORT_LOD_VERTICES) != 0) {
            W8LevelFileCompressedFace* faces = mesh->pstCompFaces;
            for (int face = 0; face < mesh->num_faces; ++face) {
                W8GDSurface* surface = &m_pGDSurfaces[m_surface_count];
                ++m_surface_count;
                surface->vertex_indices[0] = faces[face].vertex_indices[0] + vertex_base;
                surface->vertex_indices[1] = faces[face].vertex_indices[1] + vertex_base;
                surface->vertex_indices[2] = faces[face].vertex_indices[2] + vertex_base;
            }
        } else {
            W8ReadMeshFace* faces = mesh->pstFaces;
            for (int face = 0; face < mesh->num_faces; ++face) {
                W8GDSurface* surface = &m_pGDSurfaces[m_surface_count];
                ++m_surface_count;
                surface->vertex_indices[0] = faces[face].vertices[0] + vertex_base;
                surface->vertex_indices[1] = faces[face].vertices[1] + vertex_base;
                surface->vertex_indices[2] = faces[face].vertices[2] + vertex_base;
            }
        }
    }

    for (int index = surface_base; index < m_surface_count; ++index) {
        W8GDSurface* surface = &m_pGDSurfaces[index];
        BuildTrianglePlane(&surface->plane, &m_pVertices[surface->vertex_indices[0]],
                           &m_pVertices[surface->vertex_indices[1]],
                           &m_pVertices[surface->vertex_indices[2]]);

        int dominant_axis;
        float largest = g_float_zero;
        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = static_cast<float>(fabs((&surface->plane.normal.x)[axis]));
            if (largest < magnitude) {
                largest = magnitude;
                dominant_axis = axis;
            }
        }
        surface->flags = dominant_axis + W8_GD_SURFACE_PROP_GEOMETRY;
        surface->hit_plane = 0;
        if (g_float_005ebc7c <= surface->plane.normal.y) {
            surface->contact_margin = 500.0f;
            surface->flags |= W8_GD_SURFACE_WALKABLE;
        } else {
            surface->slope = 0.0f;
            surface->contact_margin = 500.0f;
        }
    }
}

/* srMatrix4x3T<float>::SetTranslation/Scale emitted for this TU by
   TransformMeshGeometry; the primary templates live in srMath.h. */
