#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <sys/stat.h>

#include "surrender/srCamera.h"
#include "surrender/srHeap.h"
#include "surrender/srMath.h"
#include "surrender/srScene.h"
#include "surrender/srShader.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/OctMeshModel.h"
#include "wiz8/location_variables.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/GameTimeAccumulator.h"
#include "wiz8/startup_world.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_code/CombatRange.h"
#include "random.h"
#include "wiz8/fonts.h"
#include "wiz8/monster_generators.h"
#include "wiz8/utility.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/local_screens/MainGameScreen.h"
#include "input.h"

#include <math.h>
#include <time.h>
#include "surrender/srModelInstance.h"

#define OCTREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp"

// FUNCTION: WIZ8 0x00433a70
void W8Octree::GetPathSurfaceNormal(const srVector3T<float>* position, srVector3T<float>* normal)
{
    if (pathing != 0) {
        pathing->GetPathSurfaceNormal(position, normal);
        return;
    }
    normal->Set(0.0f, 1.0f, 0.0f);
}

// FUNCTION: WIZ8 0x00434170
void W8Octree::UpdatePathVisualization()
{
    if (pathing != 0) {
        srVector3T<float> cursor;
        GetWorldCursorPosition(&cursor);
        /* Retail reads the y component twice (proven in disassembly: both
           fcomp loads use the y slot) and never tests x. */
        if (cursor.y != g_float_zero || cursor.y != g_float_zero || cursor.z != g_float_zero) {
            srVector3T<float> point = cursor;
            pathing->UpdatePathVisualization(&point, &view.camera_dof);
            return;
        }
        pathing->UpdatePathVisualization(&view.camera_location, &view.camera_dof);
    }
}

// GLOBAL: WIZ8 0x00659760
static int g_octree_query_slot;
// GLOBAL: WIZ8 0x00659764
static unsigned short g_octree_query_kind;
// GLOBAL: WIZ8 0x00659766
static unsigned short g_octree_query_id;

// GLOBAL: WIZ8 0x00659770
unsigned int* g_octree_storage_;

// GLOBAL: WIZ8 0x00659774
static unsigned int g_octree_query_cell;

// GLOBAL: WIZ8 0x00659778
static GETFILESTRUCT g_octree_file_search;

// GLOBAL: WIZ8 0x006598b2
static unsigned char g_octree_file_search_active;

// GLOBAL: WIZ8 0x00606810
static char g_octree_point_extension[] = ".pts";

// GLOBAL: WIZ8 0x006068a0
static char g_octree_file_search_wildcard[] = "*";

// GLOBAL: WIZ8 0x00659890
unsigned long* g_octree_state;
// GLOBAL: WIZ8 0x00659894
stModelInstance* g_octree_trace_node;
// GLOBAL: WIZ8 0x00659898
bool g_octree_update_suspended;
// GLOBAL: WIZ8 0x00659899
bool g_octree_trace_enabled;

/* Build a packed four-byte colour from four components and answer its
   address. The receiver is the output slot. */
/* Draw the probe box through the world camera. */

// GLOBAL: WIZ8 0x006598a4
W8Octree* g_octree;

// GLOBAL: WIZ8 0x006068a4
static char g_region_link_extension[] = ".rlk";

// GLOBAL: WIZ8 0x006598a8
bool g_octree_disabled;
// GLOBAL: WIZ8 0x006598b0
static unsigned short g_octree_region_debug_last;

/* Renderer switches toggled across a region-link build: while sampling,
   meshes render without baked vertex lighting or textures and with front
   faces culled.  The inverted-depth switch is read by the surface-capture
   path (RenderWorldToSurface and the tri-mesh renderer) but no Wiz8 code
   ever writes it. */
// GLOBAL: WIZ8 0x0065a0ec
bool g_render_unlit;
// GLOBAL: WIZ8 0x0065a0ed
bool g_render_cull_front;
// GLOBAL: WIZ8 0x0065a0ee
bool g_inverted_depth_render;
// GLOBAL: WIZ8 0x0065a146
bool g_render_untextured;

// FUNCTION: WIZ8 0x0042bc00
void NoOct(void)
{
    g_octree_disabled = true;
}

// GLOBAL: WIZ8 0x005ec02c
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;

/* Noise falloff on the trace resolver's sphere-distance penalty: the farther
   the probe is along the ray, the more the candidate's effective distance is
   discounted. Shared with CreateTraceModel. */
// GLOBAL: WIZ8 0x005ebc78
const float g_float_fifteen_hundredths = 0.15000000596046448f;
/* Vertical snap ceiling for navigator placement: the source may rise or fall
   at most this many units before a candidate is rejected outright. */
// GLOBAL: WIZ8 0x005ec038
const double g_navigator_vertical_snap_limit = 5000.0;
/* Fixed camera tilt (15 degrees below horizontal) the region-link projector
   applies to every sampled direction. */
// GLOBAL: WIZ8 0x005ec008
const double g_region_link_camera_tilt = -0.26179999113082886;
/* Circle-coverage bound just under 2*pi: when samples*fov still falls short,
   one more direction is added. */
// GLOBAL: WIZ8 0x005ec010
const float g_region_link_circle_coverage = 6.282185077667236f;
/* Jitter scale applied to the Random(1000) roll for scatter-ring candidates
   past the first; 0.0004 * 1000 spans 0.4 units. */
// GLOBAL: WIZ8 0x005ec044
const float g_scatter_outer_ring_jitter_scale = 0.00040000001899898052f;
/* Multiplier on the placement radius that gives the monster-proximity query
   box its extent. */
// GLOBAL: WIZ8 0x005ec048
const float g_monster_proximity_radius_scale = 15.0f;
/* Jitter scale applied to the Random(1000) roll for ring candidates past the
   first; 0.0002 * 1000 spans 0.2 units. */
// GLOBAL: WIZ8 0x005ec050
const float g_scatter_inner_ring_jitter_scale = 0.00020000000949949026f;

// FUNCTION: WIZ8 0x0042f7e0
void W8Octree::UpdateCameraVisibility()
{
    W8World* world = GetWorld();
    if (g_octree_update_suspended || m_visibility_suspended) {
        return;
    }

    view.far_clip = static_cast<float>(WorldGetFarClip(world));
    world->camera->getLocation(view.camera_location);
    view.horizontal_fov = static_cast<float>(world->camera->getHorizontalFOV());
    view.vertical_fov = static_cast<float>(world->camera->getVerticalFOV());

    srMatrix3T<float> rotation;
    world->camera->getRotation(rotation);
    view.rotation_column0.Set(rotation.vectors[0].x, rotation.vectors[1].x, rotation.vectors[2].x);
    view.rotation_column1.Set(rotation.vectors[0].y, rotation.vectors[1].y, rotation.vectors[2].y);

    srVector3T<double> dof = world->camera->getWorldSpaceDOF();
    view.camera_dof = dof;
    view.horizontal_fov_cosine = static_cast<float>(cos(view.horizontal_fov));
    view.vertical_fov_cosine = static_cast<float>(cos(view.vertical_fov));
    m_visited_polygon_bits->ClearAll();
    m_projected_regions->ClearAll();
    UpdateVisibility();
    if (g_octree_trace_enabled) {
        UpdateWorldTrace();
    }
}

/* Rebuild the sector-to-mesh visibility state, then publish this frame's
   visible regions to the meshes, props and particles.

   The reset pass only runs after a level load or a resumed update. It walks
   the meshes, records each one's sector mapping, drops the visible flags, and
   hides every prop and particle before clearing the previous frame's sets.

   The frame pass clears the new sets, projects the camera through the octree,
   publishes the union of the previous and current region sets, and finally
   diffs the two frames so only the meshes, props and particles that changed
   state are touched. */
// FUNCTION: WIZ8 0x004304a0
void W8Octree::UpdateVisibility()
{
    unsigned short mesh_index;
    unsigned int index;
    int bit;

    if (m_reset_visibility) {
        m_reset_visibility = false;
        for (mesh_index = 0; mesh_index < m_meshCount; ++mesh_index) {
            if (g_world->psrMeshes[mesh_index] != 0) {
                m_pSubmeshes
                    [static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index])->mesh_index + 1]
                        .mesh = mesh_index;
                m_pSubmeshes[mesh_index + 1].flags &= 0xffffffc7;
                stModelInstance* mesh =
                    static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index]);
                if (mesh != 0) {
                    mesh->setFlag(srNode::FLAG_DISABLE);
                    mesh->setFlag(srNode::FLAG_TERMINATE);
                }
            }
        }
        for (index = 0; index < m_usNumPropsLoaded; ++index) {
            m_papProps[index]->SetActivationState(0);
        }
        if (m_pusMeshProps != 0) {
            for (index = 0; m_pusMeshProps[index] != 0; ++index) {
                m_papProps[m_pusMeshProps[index] - 1]->SetActivationState(1);
            }
        }
        for (index = 0; index < m_usNumParticlesLoaded; ++index) {
            m_papParticles[index]->SetTraversalEnabled(false);
        }
        if (m_pusMeshParticles != 0) {
            for (index = 0; m_pusMeshParticles[index] != 0; ++index) {
                m_papParticles[m_pusMeshParticles[index] - 1]->SetTraversalEnabled(true);
            }
        }
        ValidateRegionMeshLinks();
        m_previous_regions->ClearAll();
        m_accumulated_regions->ClearAll();
        if (m_ulNumParticles != 0) {
            m_visible_particles->ClearAll();
        }
        if (m_ulNumProps != 0) {
            m_visible_props->ClearAll();
        }
    }

    m_current_regions->ClearAll();
    m_projected_regions->ClearAll();
    if (m_ulNumParticles != 0) {
        m_particles_to_disable->ClearAll();
        m_linked_particles->ClearAll();
    }
    if (m_ulNumProps != 0) {
        m_linked_props->ClearAll();
        m_props_to_disable->ClearAll();
    }
    m_projected_regions_valid = false;
    m_location_region_matched = 0;
    CollectVisibleRegions(&view.camera_location, &view.visible_cells, 0, 1);
    CollectVisibleCells();
    if (pathing != 0) {
        srVector3T<float> dof;
        GetWorldCursorPosition(&dof);
        /* Same proven retail quirk as UpdatePathVisualization: y is tested
           twice and x is never examined. */
        if (dof.y != g_float_zero || dof.y != g_float_zero || dof.z != g_float_zero) {
            srVector3T<float> probe = dof;
            pathing->UpdatePathVisualization(&probe, &view.camera_dof);
        } else {
            pathing->UpdatePathVisualization(&view.camera_location, &view.camera_dof);
        }
    }
    if (m_projected_regions_valid) {
        m_current_regions->IntersectWith(*m_projected_regions);
    }
    m_accumulated_regions->UnionWith(*m_current_regions);
    bit = m_previous_regions->NextSetBit(true);
    while (bit != 0) {
        --bit;
        if (!m_current_regions->Test(bit) && bit != 0) {
            W8OctSubmesh* submesh = &m_pSubmeshes[bit];
            submesh->flags &= 0xfffffff7;
            stModelInstance* mesh =
                static_cast<stModelInstance*>(g_world->psrMeshes[submesh->mesh]);
            if (mesh != 0) {
                mesh->setFlag(srNode::FLAG_DISABLE);
                mesh->setFlag(srNode::FLAG_TERMINATE);
            }
        }
        bit = m_previous_regions->NextSetBit(false);
    }
    bit = m_current_regions->NextSetBit(true);
    while (bit != 0) {
        --bit;
        if (!m_previous_regions->Test(bit) && bit != 0) {
            W8OctSubmesh* submesh = &m_pSubmeshes[bit];
            int mesh_index = submesh->mesh;
            submesh->flags |= 0x28;
            stModelInstance* mesh = static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index]);
            if (mesh != 0) {
                mesh->clearFlag(srNode::FLAG_DISABLE);
                mesh->clearFlag(srNode::FLAG_TERMINATE);
            }
        }
        MarkMeshLinksVisible(bit);
        bit = m_current_regions->NextSetBit(false);
    }
    if (m_ulNumParticles != 0) {
        for (index = 0; m_pusMeshParticles[index] != 0; ++index) {
            m_linked_particles->Set(m_pusMeshParticles[index] - 1);
        }
        m_particles_to_disable->SetToComplementOf(*m_linked_particles);
        m_particles_to_disable->IntersectWith(*m_visible_particles);
        bit = m_linked_particles->NextSetBit(true);
        while (bit != 0) {
            --bit;
            if (!m_visible_particles->Test(bit)) {
                m_papParticles[bit]->SetTraversalEnabled(true);
            }
            bit = m_linked_particles->NextSetBit(false);
        }
        bit = m_particles_to_disable->NextSetBit(true);
        while (bit != 0) {
            m_papParticles[bit - 1]->SetTraversalEnabled(false);
            bit = m_particles_to_disable->NextSetBit(false);
        }
        m_visible_particles->CopyFrom(*m_linked_particles);
    }
    if (m_ulNumProps != 0) {
        for (index = 0; m_pusMeshProps[index] != 0; ++index) {
            m_linked_props->Set(m_pusMeshProps[index] - 1);
        }
        m_props_to_disable->SetToComplementOf(*m_linked_props);
        m_props_to_disable->IntersectWith(*m_visible_props);
        bit = m_linked_props->NextSetBit(true);
        while (bit != 0) {
            --bit;
            if (!m_visible_props->Test(bit)) {
                m_papProps[bit]->SetActivationState(1);
            }
            bit = m_linked_props->NextSetBit(false);
        }
        bit = m_props_to_disable->NextSetBit(true);
        while (bit != 0) {
            m_papProps[bit - 1]->SetActivationState(0);
            bit = m_props_to_disable->NextSetBit(false);
        }
        m_visible_props->CopyFrom(*m_linked_props);
    }
    m_previous_regions->CopyFrom(*m_current_regions);
}

/* Collect every model instance whose bounds come within `radius` of `point`.

   The region-grid cells under the sphere (one cell of slack on each side)
   contribute their leaf regions; every pre-generated region whose frustum the
   sphere reaches contributes its bit. Each marked region then offers up its
   mesh instance when the instance's bounding box is inside the radius, and -
   while the flag byte of `flags` is set - each linked prop's instance when the
   prop's bounding sphere, shifted to the prop position and fattened by
   `radius`, contains the point. `only_accumulated` restricts the marking to
   regions the camera pass already accumulated.

   The prop path toggles m_papProps[bit - 2] but positions m_papProps[bit - 1]:
   the animation and the sphere test land one prop apart. That is what retail
   does (0x0042FD40 reads [array + bit*4 - 8], 0x0042FD8C reads
   [array + bit*4 - 4]), so it stays. */
// FUNCTION: WIZ8 0x0042f9a0
int W8Octree::CollectModelsNearPoint(W8GrowableVector<stModelInstance*>* out,
                                     const srVector3T<float>* point, float radius,
                                     unsigned int flags, bool only_accumulated)
{
    unsigned int level_mask = m_region_mask;
    unsigned int limit = m_spatial.m_region_cells_per_axis - 1;

    srVector3T<int> first;
    srVector3T<unsigned int> last;
    int axis;
    for (axis = 0; axis < 3; ++axis) {
        (&first.x)[axis] =
            static_cast<int>(((&point->x)[axis] - radius - (&m_spatial.m_minimum.x)[axis]) /
                             m_spatial.m_region_grid_cell) -
            1;
        if ((&first.x)[axis] < 0) {
            (&first.x)[axis] = 0;
        }
        (&last.x)[axis] = static_cast<unsigned int>(
                              ((&point->x)[axis] + radius - (&m_spatial.m_minimum.x)[axis]) /
                              m_spatial.m_region_grid_cell) +
                          1;
        if (limit < (&last.x)[axis]) {
            (&last.x)[axis] = limit;
        }
    }

    m_current_regions->ClearAll();
    /* last and cell are unsigned, and the region loops compare them as such:
       0x0042FB3C, 0x0042FB51 and 0x0042FB5E are all jbe, not jle. first stays
       signed because it is the one clamped against zero above. */
    srVector3T<unsigned int> cell;
    for (cell.x = static_cast<unsigned int>(first.x); cell.x <= last.x; ++cell.x) {
        for (cell.y = static_cast<unsigned int>(first.y); cell.y <= last.y; ++cell.y) {
            for (cell.z = static_cast<unsigned int>(first.z); cell.z <= last.z; ++cell.z) {
                unsigned int masked_cell[4];
                masked_cell[0] = level_mask;
                masked_cell[1] = cell.x;
                masked_cell[2] = cell.y;
                masked_cell[3] = cell.z;
                int node = DescendByMask(masked_cell);
                if (node != 0) {
                    unsigned int region = m_branches[node].region;
                    if (region != 0) {
                        m_current_regions->Set(region);
                    }
                }
            }
        }
    }

    unsigned int region;
    for (region = 1; region < m_spatial.m_region_count; ++region) {
        if (SphereInsideFrustum(point, radius, m_spatial.m_region_volumes[region].m_planes) &&
            m_spatial.m_region_volumes[region].m_region_bit != 0) {
            m_current_regions->Set(m_spatial.m_region_volumes[region].m_region_bit);
        }
    }

    if (m_ulNumProps == 0) {
        flags &= 0xffffff00;
    } else if ((flags & 0xff) != 0) {
        m_linked_props->ClearAll();
    }
    if (only_accumulated) {
        m_current_regions->IntersectWith(*m_accumulated_regions);
    }

    int bit = m_current_regions->NextSetBit(true);
    while (bit != 0) {
        unsigned int submesh = bit - 1;
        if (submesh != 0) {
            stModelInstance* instance =
                static_cast<stModelInstance*>(g_world->psrMeshes[m_pSubmeshes[submesh].mesh]);
            if (instance != 0) {
                srModel* model = instance->getModel();
                if (model != 0) {
                    W8BoundingBox bounds;
                    model->getBoundingBox(bounds.minimum, bounds.maximum);
                    if (SphereNearBounds(point, radius, &bounds)) {
                        out->Add(instance);
                    }
                }
            }
            if ((flags & 0xff) != 0) {
                MarkMeshLinksVisible(submesh);
            }
        }
        bit = m_current_regions->NextSetBit(false);
    }

    if ((flags & 0xff) != 0) {
        bit = m_linked_props->NextSetBit(true);
        while (bit != 0) {
            srModelInstance* instance = m_papProps[bit - 2]->ToggleRepAnimationDefault();
            if (instance != 0) {
                srModel* model = instance->getModel();
                if (model != 0) {
                    srVector3T<float> center;
                    float sphere_radius;
                    model->getBoundingSphere(center, sphere_radius);
                    srVector3T<float> prop_position;
                    m_papProps[bit - 1]->GetPosition(&prop_position);
                    center += prop_position;
                    sphere_radius += radius;
                    if ((center.x - point->x) * (center.x - point->x) +
                            (center.y - point->y) * (center.y - point->y) +
                            (center.z - point->z) * (center.z - point->z) <
                        sphere_radius * sphere_radius) {
                        out->Add(static_cast<stModelInstance*>(instance));
                    }
                }
            }
            bit = m_linked_props->NextSetBit(false);
        }
    }
    return out->count;
}

/* Add every particle and prop linked to one mesh to the live visibility sets.

   The lookup gives a 1-based start into a packed link table; each run is
   terminated by a zero. A linked entry whose slot is empty is skipped. */
// FUNCTION: WIZ8 0x00430a70
void W8Octree::MarkMeshLinksVisible(unsigned int mesh)
{
    if (mesh > m_meshCount) {
        return;
    }
    if (m_ulNumParticles != 0) {
        unsigned short link = m_pusMeshParticleLookup[mesh];
        if (link != 0 && m_pusMeshParticles[link] != 0) {
            do {
                if (m_usMeshParticlesLen < link) {
                    srAssertFail("usProp<=m_usMeshParticlesLen", OCTREE_CPP, 0xa07,
                                 "Particle lookup index out of range");
                }
                unsigned short particle = m_pusMeshParticles[link];
                ++link;
                if (particle <= m_usNumParticlesLoaded && m_papParticles[particle] != 0) {
                    m_linked_particles->Set(particle - 1);
                }
            } while (m_pusMeshParticles[link] != 0);
        }
    }
    if (m_ulNumProps != 0) {
        unsigned short link = m_pusMeshPropLookup[mesh];
        if (link != 0 && m_pusMeshProps[link] != 0) {
            do {
                if (m_usMeshPropsLen < link) {
                    srAssertFail("usProp<=m_usMeshPropsLen", OCTREE_CPP, 0xa1b,
                                 "Prop lookup index out of range");
                }
                unsigned short prop = m_pusMeshProps[link];
                ++link;
                if (prop <= m_usNumPropsLoaded && m_papProps[prop] != 0) {
                    m_linked_props->Set(prop - 1);
                }
            } while (m_pusMeshProps[link] != 0);
        }
    }
}

/* Expand the camera box over the spatial levels and collect the regions the
   camera can occupy.

   The camera cell is quantized twice, once against the node extent and once
   against the leaf cell size. The walk keeps the camera inside an expanding
   box while the node extent shrinks per level; the deepest level looks the
   camera cell up in the region-link table and adds every linked region to the
   projected set. The leaf the walk ends on contributes its region list through
   ProjectLinkedRegionsForLocation. */
// FUNCTION: WIZ8 0x00431050
short W8Octree::ProjectLinkedRegionsForLocation(srVector3T<float>* location,
                                                unsigned short* region_list)
{
    if (!m_region_links_ready) {
        return 0;
    }

    short match_count = 0;
    char expected_regions[256];
    expected_regions[0] = '\0';
    char region_label[128];

    if (region_list != 0 && region_list[0] != 0) {
        for (unsigned short* cursor = region_list; *cursor != 0; ++cursor) {
            unsigned short region_index = *cursor;
            sprintf(region_label, "%d  ", region_index);
            strcat(expected_regions, region_label);

            W8OctRegionVolume* volume = &m_spatial.m_region_volumes[region_index];
            if (!PointInsideFrustum(location, volume->m_planes)) {
                continue;
            }
            if (match_count == 0) {
                m_projected_regions->ClearAll();
                m_projected_regions_valid = false;
            }
            ++match_count;
            unsigned int key = region_index;
            for (int slot = m_pRegionLinks->FindNextEntry(&key, -1); slot != -1;
                 slot = m_pRegionLinks->FindNextEntry(&key, slot)) {
                m_projected_regions->Set(m_pRegionLinks->entries[slot].value);
                m_projected_regions_valid = true;
            }
        }
        if (match_count != 0) {
            m_location_region_matched = 1;
            return match_count;
        }
    }

    char point_region_label[128];
    strcpy(point_region_label, expected_regions);

    if (m_spatial.m_region_count < 2) {
        return 0;
    }

    unsigned short reported_region = 0;
    for (unsigned short region_index = 1; region_index < m_spatial.m_region_count; ++region_index) {
        if (match_count != 0) {
            break;
        }
        W8OctRegionVolume* volume = &m_spatial.m_region_volumes[region_index];
        if (!PointInsideFrustum(location, volume->m_planes)) {
            continue;
        }
        if (reported_region == 0 && region_index != g_octree_region_debug_last) {
            sprintf(expected_regions, "Point in region %d, expected regions: %s\n", region_index,
                    point_region_label);
            NoOp();
            g_octree_region_debug_last = region_index;
            reported_region = region_index;
        }
        m_projected_regions->ClearAll();
        m_projected_regions_valid = false;
        match_count = 1;
        unsigned int key = region_index;
        for (int slot = m_pRegionLinks->FindNextEntry(&key, -1); slot != -1;
             slot = m_pRegionLinks->FindNextEntry(&key, slot)) {
            m_projected_regions->Set(m_pRegionLinks->entries[slot].value);
            m_projected_regions_valid = true;
        }
    }

    if (match_count == 0) {
        return 0;
    }
    m_location_region_matched = 1;
    return match_count;
}

// FUNCTION: WIZ8 0x00430BF0
unsigned int W8Octree::GetSectorForPosition(const srVector3T<float>* position)
{
    srVector3T<float> point = *position;
    for (unsigned int region = 1; region < m_spatial.m_region_count; ++region) {
        if (PointInsideFrustum(&point, m_spatial.m_region_volumes[region].m_planes)) {
            return m_spatial.m_region_volumes[region].m_region_bit;
        }
    }

    unsigned int cell[4];
    cell[0] = m_region_mask;
    for (int axis = 0; axis < 3; ++axis) {
        int coordinate = static_cast<int>(((&position->x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                                          m_spatial.m_region_grid_cell);
        if (coordinate < 0 || coordinate >= m_spatial.m_region_cells_per_axis) {
            return 0;
        }
        cell[axis + 1] = coordinate;
    }

    int node = DescendByMask(cell);
    if (node == 0) {
        return 0;
    }
    return m_branches[node].region;
}

// FUNCTION: WIZ8 0x00430d50
unsigned char W8Octree::CollectVisibleRegions(srVector3T<float>* location, srVector3T<int>* cells,
                                              srVector3T<float>* depth, unsigned char mode)
{
    srVector3T<int> link_cell;
    srVector3T<float> box_min;
    srVector3T<float> box_max;
    bool inside = true;

    for (int axis = 0; axis < 3; ++axis) {
        (&box_min.x)[axis] = (&m_spatial.m_minimum.x)[axis];
        (&box_max.x)[axis] = (&m_spatial.m_maximum.x)[axis];
        (&cells->x)[axis] = static_cast<int>(
            (((&location->x)[axis] - (&box_min.x)[axis]) / m_spatial.m_node_extent));
        (&link_cell.x)[axis] = static_cast<int>(
            (((&location->x)[axis] - (&box_min.x)[axis]) / m_spatial.m_region_grid_cell));
        if ((&location->x)[axis] < (&box_min.x)[axis]) {
            --(&cells->x)[axis];
            inside = false;
        }
        if ((&location->x)[axis] > (&box_max.x)[axis]) {
            inside = false;
        }
        if (depth != 0) {
            (&depth->x)[axis] =
                ((&location->x)[axis] -
                 ((&cells->x)[axis] * m_spatial.m_node_extent + (&box_min.x)[axis])) /
                    m_spatial.m_node_extent -
                g_float_half;
        }
    }
    if (!inside) {
        return 0;
    }
    if (mode == 0) {
        return 1;
    }
    float span = m_spatial.m_extent * g_float_half;
    int node = 1;
    for (short level = 0; level < m_spatial.m_depth; ++level) {
        unsigned int child_index = 0;
        for (int axis = 0; axis < 3; ++axis) {
            float edge = span + (&box_min.x)[axis];
            if ((&location->x)[axis] < edge) {
                (&box_max.x)[axis] = edge;
            } else {
                (&box_min.x)[axis] = edge;
                child_index |= 1 << (2 - axis);
            }
        }
        if (level == m_spatial.m_leaf_level) {
            unsigned int key =
                ((m_region_mask * 0x100 + link_cell.x) * 0x100 + link_cell.y) * 0x100 + link_cell.z;
            int slot = m_pRegionLinks->FindNextEntry(&key, -1);
            while (slot != -1) {
                m_projected_regions->Set(m_pRegionLinks->entries[slot].value);
                slot = m_pRegionLinks->FindNextEntry(&key, slot);
                m_projected_regions_valid = true;
            }
        }
        if (node != 0) {
            node = static_cast<int>(m_branches[node].children[child_index]);
        }
        span *= g_float_half;
    }
    if (node != 0) {
        unsigned long region_offset = m_leaves[node].region_offset;
        if (region_offset != 0) {
            ProjectLinkedRegionsForLocation(location, m_region_index_stream + region_offset);
            return 1;
        }
    }
    ProjectLinkedRegionsForLocation(location, 0);
    return 1;
}

/* Project each candidate region volume against the camera frustum and add the
   visible ones to the current region set. The first corner is tested against
   the far-clip sphere before the eight remaining corners are projected. */
// FUNCTION: WIZ8 0x004301c0
void W8Octree::MarkVisibleRegions()
{
    float radius = view.far_clip;
    float radius_squared = radius * radius;

    for (int index = 1; index < m_spatial.m_region_count; ++index) {
        W8OctRegionVolume* volume = &m_spatial.m_region_volumes[index];

        if (m_projected_regions_valid && !m_projected_regions->Test(volume->m_region_bit)) {
            continue;
        }
        srVector3T<float> delta = view.camera_location - volume->m_points[0];

        if (delta.LengthSquared() >= radius_squared) {
            continue;
        }
        bool visible = PointInsideFrustum(&volume->m_points[0], view.frustum_planes);

        for (int point = 1; !visible && point < 9; ++point) {
            visible = PointInsideFrustum(&volume->m_points[point], view.frustum_planes);
        }
        if (visible && volume->m_region_bit != 0) {
            m_current_regions->Set(volume->m_region_bit);
        }
    }
}

/* Build the four side frustum planes from the camera basis and far clip, and
   accumulate the two far-plane offsets the region projection reads. */
// FUNCTION: WIZ8 0x004302e0
void W8Octree::BuildFrustumPlanes()
{
    float fov = view.horizontal_fov * g_float_half;
    float extent = m_spatial.m_max_region_radius;
    float far_clip = view.far_clip;
    float backoff = extent / sin(fov);
    float ratio = extent / cos(fov);
    double tangent = tan(fov);
    double tangent_vertical = tan(view.vertical_fov * g_float_half);
    srVector3T<float> corners[8];

    view.frustum_planes[4].w = 0.0f;
    view.frustum_planes[5].w = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        float dof = (&view.camera_dof.x)[axis];
        float column1 = (&view.rotation_column0.x)[axis];
        float column2 = (&view.rotation_column1.x)[axis];
        float w = far_clip * dof;
        float a = (tangent * far_clip + ratio) * column1;
        float b = (tangent_vertical * far_clip + ratio) * column2;

        (&corners[0].x)[axis] = (&view.camera_location.x)[axis] - backoff * dof;
        (&corners[1].x)[axis] = a;
        (&corners[2].x)[axis] = b;
        (&corners[3].x)[axis] = w;
        (&corners[4].x)[axis] = (w - a) + b;
        (&corners[5].x)[axis] = (w + b) + a;
        (&corners[6].x)[axis] = (w - a) - b;
        (&corners[7].x)[axis] = (w + a) - b;
        (&corners[4].x)[axis] += (&view.camera_location.x)[axis];
        (&corners[5].x)[axis] += (&view.camera_location.x)[axis];
        (&corners[6].x)[axis] += (&view.camera_location.x)[axis];
        (&corners[7].x)[axis] += (&view.camera_location.x)[axis];
        (&view.frustum_planes[4].normal.x)[axis] = dof;
        view.frustum_planes[4].w -= dof * (&corners[0].x)[axis];
        (&view.frustum_planes[5].normal.x)[axis] = -dof;
        view.frustum_planes[5].w -= -dof * (&corners[4].x)[axis];
    }
    BuildPlaneFromPoints(&view.frustum_planes[0], &corners[0], &corners[5], &corners[4]);
    BuildPlaneFromPoints(&view.frustum_planes[1], &corners[0], &corners[4], &corners[6]);
    BuildPlaneFromPoints(&view.frustum_planes[2], &corners[0], &corners[7], &corners[5]);
    BuildPlaneFromPoints(&view.frustum_planes[3], &corners[0], &corners[6], &corners[7]);
    for (int index = 0; index < 6; ++index) {
        view.frustum_planes[index].w += m_spatial.m_region_grid_cell;
    }
}

/* Collect the cells around the camera into the current region set.

   Two setup helpers refresh the frame state. The leaf cell radius comes from
   the far clip over the cell size, and the camera cell is quantized against
   the same size. Every cell within that radius of the camera cell that stays
   inside the spatial extent is considered: the near cells descend the branch
   array directly, while the far cells filter through
   PointInsideFrustum first. Either way the reached node contributes
   the region stored at its branch head. */
// FUNCTION: WIZ8 0x0042fe90
void W8Octree::CollectVisibleCells()
{
    BuildFrustumPlanes();
    MarkVisibleRegions();
    short radius =
        static_cast<short>((static_cast<int>((view.far_clip / m_spatial.m_region_grid_cell)) + 1));
    srVector3T<short> center;

    for (int axis = 0; axis < 3; ++axis) {
        (&center.x)[axis] = static_cast<short>(
            static_cast<int>((((&view.camera_location.x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                              m_spatial.m_region_grid_cell)));
    }
    unsigned int region_base = m_region_mask;
    for (short x = -radius; x <= radius; ++x) {
        short cell_x = center.x + x;
        if (cell_x < 0 || cell_x >= m_spatial.m_region_cells_per_axis) {
            continue;
        }
        for (short y = -radius; y <= radius; ++y) {
            short cell_y = center.y + y;
            if (cell_y < 0 || cell_y >= m_spatial.m_region_cells_per_axis) {
                continue;
            }
            for (short z = -radius; z <= radius; ++z) {
                short cell_z = center.z + z;
                if (cell_z < 0 || cell_z >= m_spatial.m_region_cells_per_axis) {
                    continue;
                }
                if (abs(x) >= 2 || abs(y) >= 2 || abs(z) >= 2) {
                    float offset = m_spatial.m_region_grid_cell * g_float_half;
                    srVector3T<float> point;
                    point.x =
                        cell_x * m_spatial.m_region_grid_cell + offset + m_spatial.m_minimum.x;
                    point.y =
                        cell_y * m_spatial.m_region_grid_cell + m_spatial.m_minimum.y + offset;
                    point.z =
                        cell_z * m_spatial.m_region_grid_cell + m_spatial.m_minimum.z + offset;
                    if (!PointInsideFrustum(&point, view.frustum_planes)) {
                        continue;
                    }
                }
                unsigned int cell[4];
                cell[0] = region_base;
                cell[1] = cell_x;
                cell[2] = cell_y;
                cell[3] = cell_z;
                int node = DescendByMask(cell);
                if (node != 0) {
                    unsigned short region = m_branches[node].region;
                    if (region != 0) {
                        m_current_regions->Set(region);
                    }
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004329a0
static unsigned char FindNextLevelFile(char* name)
{
    if (name == 0) {
        g_octree_file_search_active = 0;
        return 0;
    }

    if (g_octree_file_search_active == 0) {
        char pattern[256];
        char extension[52];

        strcpy(pattern, name);
        char* extension_start = strrchr(pattern, '.');
        extension[0] = '\0';
        if (extension_start != 0) {
            strcpy(extension, extension_start);
            *extension_start = '\0';
        }
        strcat(pattern, g_octree_file_search_wildcard);
        strcat(pattern, extension);
        g_octree_file_search_active = GetFileFirst(pattern, &g_octree_file_search);
    } else {
        g_octree_file_search_active = GetFileNext(&g_octree_file_search);
    }
    if (g_octree_file_search_active == 0) {
        GetFileClose(&g_octree_file_search);
        return 0;
    }

    char* separator = strrchr(name, '\\');
    if (separator != 0) {
        separator[1] = '\0';
    }
    strcat(name, g_octree_file_search.zFileName);
    return g_octree_file_search_active;
}

// FUNCTION: WIZ8 0x00432b80
bool W8Octree::LoadPointFiles(const char* level_name)
{
    char name[256];
    strcpy(name, level_name);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, g_octree_point_extension);

    FindNextLevelFile(0);
    bool first = true;
    unsigned char read_ok = 0;
    while (FindNextLevelFile(name) != 0) {
        if (first) {
            first = false;
        } else {
            m_points_dirty = true;
        }
        int file = FileOpen(name, 1, 0);
        if (file == 0) {
            return false;
        }
        if (FileRead(file, &m_point_count, 4, 0) == 0) {
            FileClose(file);
            return false;
        }
        m_sample_points = new srVector3T<float>[m_point_count + 1];
        if (m_sample_points == 0) {
            FileClose(file);
            return false;
        }
        read_ok = FileRead(file, m_sample_points, m_point_count * sizeof(srVector3T<float>), 0);
        FileClose(file);
    }
    if (read_ok != 0) {
        return read_ok;
    }
    delete[] m_sample_points;
    m_point_count = 0;
    return false;
}

/* Write the octree's point array to a companion file.

   The level path supplies the base name and its existing extension is
   replaced with the point-file extension. A read-only file is made writable
   first. The count precedes the records, and the result reports either
   write. */
// FUNCTION: WIZ8 0x00432d60
BOOLEAN W8Octree::SavePoints(char* path)
{
    char name[256];
    unsigned char result = 0;

    strcpy(name, path);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, g_octree_point_extension);
    if (FileExists(name) != 0) {
        if (_access(name, 2) != 0) {
            _chmod(name, 0x180);
        }
    }
    int file = FileOpen(name, 2, 0);
    if (file != 0) {
        if (m_point_count != 0 && m_sample_points != 0) {
            unsigned char wrote_count = FileWrite(file, &m_point_count, 4, 0);
            unsigned char wrote_points =
                FileWrite(file, m_sample_points, m_point_count * sizeof(srVector3T<float>), 0);
            result = wrote_count | wrote_points;
            FileClose(file);
        }
    }
    return result;
}

// FUNCTION: WIZ8 0x00432e90
bool W8Octree::ReadRegionLinkFile(const char* level_name)
{
    unsigned int count = 0;
    unsigned int* keys = 0;
    unsigned short* values = 0;
    int file = 0;
    bool result = false;
    char name[256];

    strcpy(name, level_name);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, g_region_link_extension);
    if (FileExists(name) == 0) {
        return false;
    }
    file = FileOpen(name, 1, 0);
    if (file == 0) {
        return false;
    }
    if (FileRead(file, &count, 4, 0) == 0) {
        return false;
    }

    keys = static_cast<unsigned int*>(malloc(count * sizeof(unsigned int)));
    values = static_cast<unsigned short*>(malloc(count * 2));
    if (keys == 0 || values == 0) {
        FileClose(file);
        free(keys);
        free(values);
        return false;
    }
    if (FileRead(file, keys, count * 4, 0) == 0 || FileRead(file, values, count * 2, 0) == 0) {
        FileClose(file);
        free(keys);
        free(values);
        return false;
    }
    if (m_pRegionLinks == 0) {
        m_pRegionLinks = new W8HashTable<unsigned int, unsigned short>;
    }
    for (unsigned int index = 0; index < count; ++index) {
        m_pRegionLinks->Remove(&keys[index], &values[index]);
        m_pRegionLinks->Insert(&keys[index], &values[index]);
    }
    result = true;
    FileClose(file);
    free(keys);
    free(values);
    if (result) {
        m_region_links_ready = true;
    }
    return result;
}

/* The region volume containing `point` as its 1-based index into the spatial
   array, else the packed auto-region key - the region-grid level in the top
   byte over the point's cell coordinates. Region zero is never tested: it is
   the no-region sentinel. */
// FUNCTION: WIZ8 0x00432720
unsigned int W8Octree::RegionKeyForPoint(const srVector3T<float>* point)
{
    unsigned int region = 0;
    if (1 < m_spatial.m_region_count) {
        unsigned int index = 1;
        do {
            if (region != 0) {
                return region;
            }
            if (m_spatial.m_region_volumes[index].ContainsPoint(point)) {
                region = index;
            }
            ++index;
        } while (index < m_spatial.m_region_count);
        if (region != 0) {
            return region;
        }
    }
    srVector3T<int> cell;
    for (int axis = 0; axis < 3; ++axis) {
        (&cell.x)[axis] = static_cast<int>(((&point->x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                                           m_spatial.m_region_grid_cell);
    }
    return (cell.x * 0x100 + cell.y) * 0x100 + cell.z + m_region_mask * 0x1000000;
}

/* Record `region_key` against every mesh marked in m_projected_regions:
   unseen (region, mesh) pairs are inserted into m_pRegionLinks and the
   link list is logged once per call. */
// FUNCTION: WIZ8 0x004327f0
void W8Octree::RecordRegionMeshLinks(unsigned int region_key)
{
    bool reported = false;
    if (region_key == 0) {
        return;
    }
    int bit = m_projected_regions->NextSetBit(true);
    if (bit == 0) {
        return;
    }
    char text[256];
    do {
        unsigned short mesh = static_cast<unsigned short>(bit - 1);
        int slot = m_pRegionLinks->FindNextEntry(&region_key, -1);
        while (slot != -1 && m_pRegionLinks->entries[slot].value != mesh) {
            slot = m_pRegionLinks->FindNextEntry(&region_key, slot);
        }
        if (slot == -1) {
            if (!reported) {
                if (region_key < m_spatial.m_region_count) {
                    sprintf(text, "Pre-generated Region %d linked to meshes: ", region_key);
                } else {
                    sprintf(text, "Auto Region (%d, %d, %d) linked to meshes: ",
                            (region_key >> 0x10) & 0xff, (region_key >> 8) & 0xff,
                            region_key & 0xff);
                }
                NoOp();
                reported = true;
            }
            sprintf(text, "%d, ", mesh);
            NoOp();
            m_pRegionLinks->Insert(&region_key, &mesh);
        }
        bit = m_projected_regions->NextSetBit(false);
    } while (bit != 0);
    if (reported) {
        NoOp();
    }
}

/* Sample `point` by rotating the camera through enough evenly spaced
   directions to cover the circle, collecting each direction's visible cells
   and render-probing their meshes. Cells whose mesh (or its first-child
   chain when `descend` is set) still draws more than nine faces are marked
   into the projected region set; `region_key` nonzero selects the region
   directly and zero resolves it through RegionKeyForPoint. `clear_sets`
   empties the projected and previous sets first. Returns the region key when
   at least one cell linked, zero otherwise. */
// FUNCTION: WIZ8 0x00431e10
unsigned int W8Octree::SampleRegionLinks(const srVector3T<float>* point, bool descend,
                                         bool clear_sets, unsigned int region_key)
{
    bool linked = false;
    W8World* world = GetWorld();
    W8Vector<stModelInstance*> meshes(5);
    W8GrowableVector<int> cells;
    view.camera_location = *point;
    view.far_clip = m_spatial.m_extent;
    char text[104];
    sprintf(text, "Sample point: %f, %f, %f, Links Found: ", point->x, point->y, point->z);
    NoOp();
    srVector3T<double> location;
    location.SetFromFloat(point);
    world->camera->setLocation(location);
    view.horizontal_fov = static_cast<float>(world->camera->getHorizontalFOV());
    view.vertical_fov = static_cast<float>(world->camera->getVerticalFOV());
    if (region_key == 0) {
        region_key = RegionKeyForPoint(point);
    }
    unsigned int key = region_key;
    if (clear_sets) {
        m_projected_regions->ClearAll();
        m_previous_regions->ClearAll();
    }
    m_projected_regions_valid = false;
    short samples = static_cast<short>(
        static_cast<int>(g_camera_angle_period / view.horizontal_fov + g_camera_snap_epsilon));
    if (samples * view.horizontal_fov < g_region_link_circle_coverage) {
        ++samples;
    }
    if (samples > 0) {
        double cos_tilt = cos(g_region_link_camera_tilt);
        double sin_tilt = sin(g_region_link_camera_tilt);
        for (int direction = 0; direction < samples; ++direction) {
            float angle = direction * view.horizontal_fov;
            srMatrix3T<float> frame;
            frame.SetIdentity();
            if (angle != g_double_zero) {
                frame.RotateAboutY(sin(angle), cos(angle));
            }
            frame.RotateAboutX(sin_tilt, cos_tilt);
            world->camera->setRotation(frame);
            view.rotation_column0.x = frame.vectors[0].x;
            srVector3T<float> unit;
            unit.Set(1.0, 0.0, 0.0);
            view.rotation_column0.y = DotProduct(frame.vectors[1], unit);
            view.rotation_column0.z = DotProduct(frame.vectors[2], unit);
            float right_y = frame.vectors[0].y;
            unit.Set(0.0, 1.0, 0.0);
            view.rotation_column1.x = right_y;
            view.rotation_column1.y = DotProduct(frame.vectors[1], unit);
            view.rotation_column1.z = DotProduct(frame.vectors[2], unit);
            srVector3T<double> dof = world->camera->getWorldSpaceDOF();
            view.camera_dof = dof;
            m_current_regions->ClearAll();
            CollectVisibleCells();
            int bit = m_current_regions->NextSetBit(true);
            bool all_known = true;
            if (bit != 0) {
                do {
                    unsigned int cell = static_cast<unsigned int>(bit - 1);
                    meshes.Add(
                        static_cast<stModelInstance*>(g_world->psrMeshes[m_pSubmeshes[cell].mesh]));
                    if (!m_projected_regions->Test(cell)) {
                        int slot = m_pRegionLinks->FindNextEntry(&key, -1);
                        while (slot != -1 && m_pRegionLinks->entries[slot].value !=
                                                 static_cast<unsigned short>(cell)) {
                            slot = m_pRegionLinks->FindNextEntry(&key, slot);
                        }
                        if (slot == -1) {
                            if (cell != 0) {
                                m_pSubmeshes[cell].flags |= 0x28;
                                srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh];
                                if (node != 0) {
                                    node->clearFlag(srNode::FLAG_DISABLE);
                                    if (descend) {
                                        node->clearFlag(srNode::FLAG_TERMINATE);
                                    }
                                }
                            }
                            cells.Add(static_cast<int>(cell));
                            all_known = false;
                        } else {
                            goto known;
                        }
                    } else {
                    known:
                        if (cell != 0) {
                            m_pSubmeshes[cell].flags &= 0xfffffff7;
                            srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh];
                            if (node != 0) {
                                node->setFlag(srNode::FLAG_DISABLE);
                                node->setFlag(srNode::FLAG_TERMINATE);
                            }
                        }
                        cells.Add(0);
                    }
                    bit = m_current_regions->NextSetBit(false);
                } while (bit != 0);
                if (!all_known) {
                    BeginRenderProbe();
                    for (int i = 0; i < meshes.GetCount(); ++i) {
                        stModelInstance* mesh = *meshes.GetAt(i);
                        if (mesh != 0) {
                            unsigned int faces = MeasureNodeRender(mesh);
                            all_known = 9 < faces;
                            srNode* child = mesh->first_child_;
                            while (child != 0 && descend) {
                                faces = MeasureNodeRender(child);
                                if (9 < faces) {
                                    all_known = true;
                                }
                                child = child->first_child_;
                            }
                            if (!all_known) {
                                unsigned int candidate = *cells.GetAt(i);
                                if (candidate != 0) {
                                    m_pSubmeshes[candidate].flags &= 0xfffffff7;
                                    srNode* node = g_world->psrMeshes[m_pSubmeshes[candidate].mesh];
                                    if (node != 0) {
                                        node->setFlag(srNode::FLAG_DISABLE);
                                        node->setFlag(srNode::FLAG_TERMINATE);
                                    }
                                }
                                cells.SetAt(i, 0);
                            }
                        }
                    }
                    for (int index = 0; index < meshes.GetCount(); ++index) {
                        stModelInstance* mesh = *meshes.GetAt(index);
                        if (mesh != 0) {
                            unsigned int cell = static_cast<unsigned int>(*cells.GetAt(index));
                            if (cell != 0) {
                                unsigned int faces = MeasureNodeRender(mesh);
                                if (9 < faces) {
                                    sprintf(text, "%d, ", cell);
                                    NoOp();
                                    m_projected_regions->Set(cell);
                                    linked = true;
                                }
                                srNode* child = mesh->first_child_;
                                while (child != 0 && descend) {
                                    faces = MeasureNodeRender(child);
                                    if (9 < faces) {
                                        m_projected_regions->Set(cell);
                                        linked = true;
                                    }
                                    child = child->first_child_;
                                }
                                m_pSubmeshes[cell].flags &= 0xfffffff7;
                                srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh];
                                if (node != 0) {
                                    node->setFlag(srNode::FLAG_DISABLE);
                                    node->setFlag(srNode::FLAG_TERMINATE);
                                }
                            }
                        }
                    }
                    EndRenderProbe();
                }
            }
        }
        region_key = key;
    }
    NoOp();
    if (!linked) {
        region_key = 0;
    }
    return region_key;
}

/* Rebuild the region-link table. With `rebuild_all` clear the x/z sample
   grid steps three region cells and checkerboards the z start, then the
   stored point list is re-sampled; with it set every cell is walked and the
   .pts point file and stored list are discarded. Either path then samples
   every camera path node. The old link table is swapped for a scratch table
   during the build so a failure or Esc abort can restore it; the scratch
   becomes the live table once the links save. */
// FUNCTION: WIZ8 0x004314c0
void W8Octree::BuildRegionLinks(bool rebuild_all)
{
    W8World* world = GetWorld();
    bool aborted = false;
    unsigned int last_key = 0;
    time_t started = time(0);
    W8HashTable<unsigned int, unsigned short>* links =
        new W8HashTable<unsigned int, unsigned short>;
    unsigned short z_step = rebuild_all ? 1 : 3;
    float stride = z_step * m_region_cell;
    unsigned short rows =
        static_cast<unsigned short>(static_cast<int>(m_spatial.m_extent / stride));
    for (unsigned int prop = 0; prop < m_usNumPropsLoaded; ++prop) {
        m_papProps[prop]->SetActivationState(0);
    }
    for (unsigned int particle = 0; particle < m_usNumParticlesLoaded; ++particle) {
        m_papParticles[particle]->SetTraversalEnabled(false);
    }
    SetViewportMode(0);
    SetWorldScaledViewport(0, 0, 0x140, 0xf0);
    srVector3T<float> saved_location;
    srMatrix3T<float> saved_rotation;
    world->camera->getLocation(saved_location);
    world->camera->getRotation(saved_rotation);
    g_light_update_flags &= ~1u;
    g_render_untextured = true;
    SetCameraLightMode(2);
    g_render_unlit = true;
    g_render_cull_front = true;
    double saved_width = world->camera->getHorizontalFOV();
    double saved_height = world->camera->getVerticalFOV();
    world->camera->setViewPlane(2.094395102, 2.094395102);
    DisableAllRenderOptions();
    for (unsigned int mesh = 0; mesh < m_meshCount; ++mesh) {
        srNode* node = g_world->psrMeshes[mesh];
        if (node != 0) {
            node->setFlag(srNode::FLAG_DISABLE);
            node->setFlag(srNode::FLAG_TERMINATE);
        }
    }
    links->Clear();
    W8HashTable<unsigned int, unsigned short>* saved_links = m_pRegionLinks;
    m_pRegionLinks = links;
    if (rows != 0) {
        for (unsigned short x_index = 0; x_index < rows; ++x_index) {
            if (aborted) {
                break;
            }
            srVector3T<float> point;
            point.x = m_region_cell * g_float_half + x_index * stride + m_spatial.m_minimum.x;
            unsigned short z_index = !rebuild_all && (x_index & 1) != 0 ? z_step : 0;
            if (!(m_spatial.m_working_minimum.x < point.x &&
                  point.x < m_spatial.m_working_maximum.x)) {
                continue;
            }
            for (; z_index < rows; z_index += z_step) {
                if (aborted) {
                    break;
                }
                point.z = m_region_cell * g_float_half + z_index * stride + m_spatial.m_minimum.z;
                if (!(m_spatial.m_working_minimum.z < point.z &&
                      point.z < m_spatial.m_working_maximum.z)) {
                    continue;
                }
                MSG message;
                if (PeekMessageA(&message, 0, 0, 0, 0) != 0 &&
                    GetMessageA(&message, 0, 0, 0) != 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                    InputAtom input;
                    if (DequeueEvent(&input) != 0 && input.usEvent == KEY_DOWN) {
                        if (input.usParam == VK_RETURN) {
                            x_index += 2;
                            z_index = 0;
                        } else if (input.usParam == VK_ESCAPE) {
                            aborted = true;
                            if (g_build_level_links) {
                                g_build_level_links = false;
                            }
                            continue;
                        }
                    }
                }
                point.y = m_spatial.m_minimum.y + m_spatial.m_extent;
                while (pathing->SnapToLowerPathCell(&point, true)) {
                    point.y += g_default_world_height;
                    unsigned int key = RegionKeyForPoint(&point);
                    if (key != last_key) {
                        if (last_key != 0) {
                            RecordRegionMeshLinks(last_key);
                        }
                        m_projected_regions->ClearAll();
                        m_previous_regions->ClearAll();
                        last_key = key;
                    }
                    SampleRegionLinks(&point, false, false, key);
                    point.y -= g_default_world_height + g_startup_near_limit;
                }
            }
        }
    }
    g_render_untextured = false;
    EnableAllRenderOptions();
    W8HashTable<unsigned int, unsigned short>* stale;
    if (!rebuild_all) {
        if (!aborted) {
            for (unsigned int point_index = 0; point_index < m_point_count; ++point_index) {
                unsigned int key = SampleRegionLinks(&m_sample_points[point_index], true, true, 0);
                RecordRegionMeshLinks(key);
            }
        }
    } else {
        char point_path[256];
        strcpy(point_path, m_owned_0c0);
        char* extension = strrchr(point_path, '.');
        if (extension != 0) {
            *extension = '\0';
        }
        strcat(point_path, g_octree_point_extension);
        if (FileExists(point_path)) {
            if (_access(point_path, 2) != 0) {
                _chmod(point_path, 0x180);
            }
            DeleteFileA(point_path);
        }
        delete[] m_sample_points;
        m_sample_points = 0;
        m_point_count = 0;
    }
    world->camera->setViewPlane(saved_width, saved_height);
    unsigned int camera_count = PLLength(world->plsCameras);
    if (!aborted && world->plsCameras != 0 && camera_count != 0) {
        for (int camera_index = 0; camera_index < static_cast<int>(camera_count); ++camera_index) {
            W8CameraPath* entry = GetWorldCameraPath(world, camera_index);
            if (entry != 0 && entry->path != 0) {
                for (int node_index = 0; node_index < entry->path->nodes->GetCount();
                     ++node_index) {
                    unsigned int key =
                        SampleRegionLinks(*entry->path->nodes->GetAt(node_index), true, true, 0);
                    RecordRegionMeshLinks(key);
                }
            }
        }
    }
    srVector3T<double> restore_location;
    restore_location.SetFromFloat(&saved_location);
    world->camera->setLocation(restore_location);
    world->camera->setRotation(saved_rotation);
    g_light_update_flags |= 1u;
    SetCameraLightMode(3);
    g_render_unlit = false;
    g_render_cull_front = false;
    SetWorldScaledViewport(0, 0, 0x280, 0x1e0);
    m_region_links_dirty = true;
    m_region_links_ready = true;
    m_reset_visibility = true;
    unsigned int elapsed = static_cast<unsigned int>(static_cast<int>(difftime(time(0), started)));
    unsigned int minutes = elapsed / 60;
    unsigned int hours = 0;
    if (minutes > 0x3b) {
        hours = minutes / 60;
        minutes %= 60;
    }
    if (aborted) {
        stale = m_pRegionLinks;
        m_pRegionLinks = saved_links;
        CreateMessageBox(FormatWideString(L"  Linking Aborted!  "), g_small_font, 1, true, false,
                         0);
    } else {
        SaveRegionLinks(m_owned_0c0);
        if (hours == 0) {
            CreateMessageBox(
                FormatWideString(L"  Linking Time: %d Min, %d Sec  ", minutes, elapsed % 60),
                g_small_font, 1, true, false, 0);
        } else {
            CreateMessageBox(FormatWideString(L"  Linking Time: %d Hours, %d Min, %d Sec  ", hours,
                                              minutes, elapsed % 60),
                             g_small_font, 1, true, false, 0);
        }
        stale = saved_links;
    }
    delete stale;
    MarkRendererReady();
}

/* Write the octree's region-link table to the .rlk companion file.

   The collected keys are the region ids from one up to the spatial region
   count, followed by the cell keys of the region grid offset by the link id.
   Each key's table values are appended in chain order; the count is written
   first, then the keys and their short values. */
// FUNCTION: WIZ8 0x004331f0
BOOLEAN W8Octree::SaveRegionLinks(char* path)
{
    unsigned char result = 1;
    unsigned int* keys = 0;
    unsigned short* values = 0;
    int file = 0;

    if (m_pRegionLinks == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    unsigned int capacity = m_pRegionLinks->bucket_count;
    if (capacity == 0) {
        return 0;
    }
    char name[256];
    strcpy(name, path);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, g_region_link_extension);
    if (FileExists(name) != 0) {
        if (_access(name, 2) != 0) {
            _chmod(name, 0x180);
        }
    }
    file = FileOpen(name, 2, 0);
    if (file == 0) {
        goto cleanup;
    }
    keys = static_cast<unsigned int*>(malloc(capacity * sizeof(unsigned int)));
    values = static_cast<unsigned short*>(malloc(capacity * 2));
    if (keys == 0 || values == 0) {
        result = 0;
        goto cleanup;
    }
    {
        unsigned int count = 0;
        for (unsigned int key = 1; key < m_spatial.m_region_count; ++key) {
            for (int slot = m_pRegionLinks->FindNextEntry(&key, -1); slot != -1;
                 slot = m_pRegionLinks->FindNextEntry(&key, slot)) {
                keys[count] = key;
                values[count] = m_pRegionLinks->entries[slot].value;
                ++count;
            }
        }
        unsigned int extent = 1;
        for (unsigned short level = 0; level < m_spatial.m_leaf_level; ++level) {
            extent *= 2;
        }
        unsigned int base = m_region_mask * 0x1000000;
        for (unsigned int x = 0; x < extent; ++x) {
            for (unsigned int y = 0; y < extent; ++y) {
                for (unsigned int z = 0; z < extent; ++z) {
                    unsigned int key = (x << 16) + (y << 8) + z + base;
                    for (int slot = m_pRegionLinks->FindNextEntry(&key, -1); slot != -1;
                         slot = m_pRegionLinks->FindNextEntry(&key, slot)) {
                        keys[count] = key;
                        values[count] = m_pRegionLinks->entries[slot].value;
                        ++count;
                    }
                }
            }
        }
        if (FileWrite(file, &count, 4, 0) == 0) {
            return 0;
        }
        unsigned char wrote_keys = FileWrite(file, keys, count * 4, 0);
        unsigned char wrote_values = FileWrite(file, values, count * 2, 0);
        result = wrote_keys | wrote_values;
    }
cleanup:
    FileClose(file);
    if (keys != 0) {
        free(keys);
    }
    if (values != 0) {
        free(values);
    }
    return result;
}

/* Draw the octree cell the camera currently occupies as a wire box. The cell
   is derived on each axis by quantizing the camera position relative to the
   spatial minimum, and the box spans one cell from there. */
// FUNCTION: WIZ8 0x00433eb0
unsigned char W8Octree::UpdateWorldTrace()
{
    srVector3T<float> camera;
    srVector3T<int> cell;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    unsigned long color;

    GetCameraPosition(&camera);
    WorldPositionToCell(&camera, &cell);
    minimum.Set(cell.x * m_spatial.m_node_extent + m_spatial.m_minimum.x,
                cell.y * m_spatial.m_node_extent + m_spatial.m_minimum.y,
                cell.z * m_spatial.m_node_extent + m_spatial.m_minimum.z);
    maximum.Set(minimum.x + m_spatial.m_node_extent, minimum.y + m_spatial.m_node_extent,
                minimum.z + m_spatial.m_node_extent);
    unsigned long* packed = PackColourToLong(&color, 0.0, 1.0, 0.0, 0.0);
    DrawWorldBox(g_world, minimum, maximum, *packed);
    return 1;
}

// GLOBAL: WIZ8 0x005ebf60
const double g_color_byte_scale = 255.0;

/* Pack normalized alpha/red/green/blue components using the current x87
   rounding mode, then retain each result's low byte. */
// FUNCTION: WIZ8 0x00433fb0
unsigned long* __fastcall PackColourToLong(unsigned long* color, double alpha, double red,
                                           double green, double blue)
{
    unsigned char* bytes =
        reinterpret_cast<unsigned char*>(color); // reinterpret-ok: packed colour storage
    bytes[3] = static_cast<unsigned char>(srFloatToInt(alpha * g_color_byte_scale));
    bytes[2] = static_cast<unsigned char>(srFloatToInt(red * g_color_byte_scale));
    bytes[1] = static_cast<unsigned char>(srFloatToInt(green * g_color_byte_scale));
    bytes[0] = static_cast<unsigned char>(srFloatToInt(blue * g_color_byte_scale));
    return color;
}

/* Validate the current octree's region-to-mesh links against a scratch copy
   of the spatial state. The scratch copy is flattened to the leaf level with
   all region bounds enabled, and every reported bad link is posted as one
   notice. */
// FUNCTION: WIZ8 0x00433ab0
unsigned char W8Octree::ValidateRegionMeshLinks()
{
    W8OctSpatialState spatial(&m_spatial);
    spatial.m_depth = 0;
    spatial.m_node_index = 1;
    spatial.m_level_kind = 1;
    int bad_links = CountBadRegionMeshLinks(&spatial);
    if (bad_links != 0) {
        CreateMessageBox(FormatWideString(L" %d Bad Region-Mesh Links!", bad_links), g_small_font,
                         1, true, false, 0);
        return 0;
    }
    return 1;
}

/* Recount a node's bad region links.  Leaf nodes union the bounding boxes of
   the linked mesh model's chain and flag the link when the union is disjoint
   from the cell; branch nodes subdivide the cell among the eight children and
   accumulate their counts.  Depths past the record limit count as one bad
   link. */
// FUNCTION: WIZ8 0x00433B90
int W8Octree::CountBadRegionMeshLinks(W8OctSpatialState* spatial)
{
    W8OctSpatialState local(spatial);
    int bad_links = 0;
    if (spatial->m_depth < 0x10) {
        if (spatial->m_depth == m_spatial.m_leaf_level) {
            unsigned short link = m_branches[spatial->m_node_index].region;
            if (link != 0) {
                stModelInstance* mesh =
                    static_cast<stModelInstance*>(g_world->psrMeshes[m_pSubmeshes[link].mesh]);
                if (mesh != 0) {
                    stMeshModel* model = static_cast<stMeshModel*>(mesh->getModel());
                    srVector3T<float> minimum;
                    srVector3T<float> maximum;
                    minimum.x = minimum.y = minimum.z = 1.0e7f;
                    maximum.x = maximum.y = maximum.z = -1.0e7f;
                    for (; model != 0; model = model->next) {
                        srVector3T<float> box_minimum;
                        srVector3T<float> box_maximum;
                        model->getBoundingBox(box_minimum, box_maximum);
                        if (box_minimum.x < minimum.x) {
                            minimum.x = box_minimum.x;
                        }
                        if (maximum.x < box_maximum.x) {
                            maximum.x = box_maximum.x;
                        }
                        if (box_minimum.y < minimum.y) {
                            minimum.y = box_minimum.y;
                        }
                        if (maximum.y < box_maximum.y) {
                            maximum.y = box_maximum.y;
                        }
                        if (box_minimum.z < minimum.z) {
                            minimum.z = box_minimum.z;
                        }
                        if (maximum.z < box_maximum.z) {
                            maximum.z = box_maximum.z;
                        }
                    }
                    if (maximum.x < spatial->m_minimum.x || spatial->m_maximum.x < minimum.x ||
                        maximum.y < spatial->m_minimum.y || spatial->m_maximum.y < minimum.y ||
                        maximum.z < spatial->m_minimum.z || spatial->m_maximum.z < minimum.z) {
                        bad_links = 1;
                    }
                }
            }
        } else {
            short child = 0;
            for (int x = 0; x < 2; ++x) {
                for (int y = 0; y < 2; ++y) {
                    for (int z = 0; z < 2; ++z, ++child) {
                        local.m_node_index = m_branches[spatial->m_node_index].children[child];
                        if (local.m_node_index != 0) {
                            local.m_minimum.x = x * local.m_extent + spatial->m_minimum.x;
                            local.m_maximum.x = local.m_minimum.x + local.m_extent;
                            local.m_minimum.y = y * local.m_extent + spatial->m_minimum.y;
                            local.m_maximum.y = local.m_minimum.y + local.m_extent;
                            local.m_minimum.z = z * local.m_extent + spatial->m_minimum.z;
                            local.m_maximum.z = local.m_minimum.z + local.m_extent;
                            bad_links += CountBadRegionMeshLinks(&local);
                        }
                    }
                }
            }
        }
    } else {
        bad_links = 1;
    }
    return bad_links;
}

/* Flip or clear the octree update suspension.

   A null world only clears the retained render node and the flag. A real world
   toggles the flag: resuming forces the next update to rebuild all visibility
   state, while suspending drops every mesh from view, clears the visited
   region bytes, and lazily attaches the render node to the static scene. */
// FUNCTION: WIZ8 0x00434020
void W8Octree::ToggleUpdateSuspension(W8World* world)
{
    if (world == 0) {
        g_octree_trace_node = 0;
        g_octree_update_suspended = false;
        return;
    }
    g_octree_update_suspended = (!g_octree_update_suspended);
    if (!g_octree_update_suspended) {
        g_octree_trace_node->setFlag(srNode::FLAG_DISABLE);
        g_octree_trace_node->setFlag(srNode::FLAG_TERMINATE);
        m_reset_visibility = true;
        MarkRendererReady();
        return;
    }
    for (unsigned short mesh_index = 0; mesh_index < m_meshCount; ++mesh_index) {
        m_pSubmeshes[static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index])->mesh_index + 1]
            .mesh = mesh_index;
        m_pSubmeshes[mesh_index + 1].flags &= 0xffffffc7;
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])->setFlag(srNode::FLAG_DISABLE);
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])
            ->setFlag(srNode::FLAG_TERMINATE);
    }
    memset(m_pfRegsVisited, 0, m_spatial.m_region_id_bound + 1);
    if (g_octree_trace_node == 0) {
        g_octree_trace_node = g_octree_game_data->CreateTraceModel();
        g_octree_trace_node->setParent(world->static_scene, 1);
        SetModelInstanceChainExclusionMask(g_octree_trace_node, 2);
    }
    g_octree_trace_node->clearFlag(srNode::FLAG_DISABLE);
    g_octree_trace_node->clearFlag(srNode::FLAG_TERMINATE);
}

// FUNCTION: WIZ8 0x00434220
unsigned char W8Octree::TestNoiseLineOfSight(const srVector3T<float>* from, srVector3T<float>* to,
                                             float* range, int* hops)
{
    return pathing->MeasureAttachmentPath(from, to, range, hops);
}

// FUNCTION: WIZ8 0x00434250
unsigned char W8Octree::PrepareNavigatorTarget(W8NavigatorMovementState* movement, float radius,
                                               float separation)
{
    unsigned char result = 0;
    bool hit = false;
    if (movement->target_position.y > m_spatial.m_clipped_maximum.y) {
        movement->target_position.y = m_spatial.m_clipped_maximum.y;
    }
    srVector3T<float> probe = movement->target_position;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit) {
        movement->target_position.y = probe.y;
    }
    if (pathing == 0) {
        return 1;
    }
    srVector3T<float> delta = movement->target_position - movement->position;
    delta.y = 0.0f;
    if (delta.xz().Length() < NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE) {
        return 0;
    }
    if ((movement->attachment->flags & W8_NAV_ATTACHMENT_FOLLOW_PATH) == 0) {
        srVector3T<float> target = movement->target_position;
        if (pathing->FindPathCell(&target, 0, true) != 0) {
            if (!pathing->TestWaypointSpan(&movement->position, &target, false, false)) {
                movement->attachment->InitializeSegment(&movement->position, &target);
                movement->attachment->separation = separation;
                result =
                    pathing->BuildAttachmentPath(movement->attachment, movement->navigation_filter);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment;
                    attachment->path_positions[attachment->path_position_index] =
                        movement->target_position;
                    attachment->path_destination =
                        attachment->path_positions[attachment->path_position_index];
                    pathing->AdvanceAttachmentWaypoint(&movement->position, attachment);
                    movement->attachment->GetNextPosition(&movement->target_position);
                    return result;
                }
                result = pathing->ProbeAttachmentPath(movement->attachment);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment;
                    attachment->path_positions[attachment->path_position_index] =
                        movement->target_position;
                    attachment->path_destination =
                        attachment->path_positions[attachment->path_position_index];
                    return result;
                }
            } else {
                movement->attachment->InitializeSegment(&movement->position,
                                                        &movement->target_position);
                result = 1;
            }
        }
        return result;
    }
    delta = movement->target_position - movement->position;
    float length = delta.Length();
    float gap = length - separation;
    if (gap < g_float_zero) {
        movement->attachment->InitializeSegment(&movement->position, &movement->position);
        return 1;
    }
    if (gap < g_world_scale) {
        delta.SetLength(gap * g_float_one_and_one_hundredth);
        delta += movement->position;
        movement->attachment->InitializeSegment(&movement->position, &delta);
        return 1;
    }
    movement->attachment->InitializeSegment(&movement->position, &movement->target_position);
    movement->attachment->separation = separation;
    return pathing->PlanMovement(movement, radius, separation) != 0;
}

// FUNCTION: WIZ8 0x004347d0
bool __stdcall IsNavigatorAtTarget(W8NavigatorMovementState* movement)
{
    srVector3T<float> target;
    if (movement->attachment != 0) {
        W8NavigatorAttachment* attachment = movement->attachment;
        if (attachment->path_cursor < attachment->path_position_index ||
            (attachment->flags & W8_NAV_ATTACHMENT_START_WAYPOINT) != 0) {
            return false;
        }
        attachment->GetNextPosition(&target);
    } else {
        target = movement->target_position;
    }
    if (movement->movement_scale * g_world_scale < (target - movement->position).Length()) {
        return false;
    }
    return true;
}

// FUNCTION: WIZ8 0x00434880
unsigned char W8Octree::PrepareNavigatorPatrol(W8NavigatorMovementState* movement, float minimum,
                                               float maximum)
{
    unsigned char result = 0;
    if (pathing != 0) {
        srVector3T<float> velocity = movement->velocity;
        movement->attachment->InitializeSegment(&movement->position, &movement->target_position);
        result = pathing->BuildPatrolPath(movement->attachment, movement->navigation_filter,
                                          &movement->target_position, minimum, &velocity, maximum);
        if (result == 0) {
            return 0;
        }
        double step = movement->movement_scale * g_world_scale;
        movement->attachment->GetNextPosition(&movement->target_position);
        srVector3T<float> delta;
        delta = movement->target_position - movement->position;
        if (step < delta.Length()) {
            delta.SetLength(step);
        }
        movement->target_position = delta + movement->position;
    }
    return result;
}

// FUNCTION: WIZ8 0x00434a00
unsigned char W8Octree::LinkNavigatorTarget(W8NavigatorMovementState* movement,
                                            const srVector3T<float>* target, float separation)
{
    if (pathing != 0) {
        return pathing->LinkAttachmentTarget(movement->attachment, movement->navigation_filter,
                                             target, separation);
    }
    return 0;
}

/* The cell-walk probes and the trace helpers the two line-of-sight bodies use.
   None of their bodies are recovered, so they keep address-qualified names. */

// GLOBAL: WIZ8 0x005ebcd0
const float g_octree_cell_scale = 100.0f;
/* 0x00659888 accumulates every byte the loader reads, and 0x00652DB0 caches the
   game-data block LoadWorld hands back through its out parameter. */

// GLOBAL: WIZ8 0x00659888
unsigned long g_octree_bytes_read;

/* Follow one child bit per axis and level through the compact 9-word branch
   records.  Zero is the missing-child sentinel; live leaves start at one. */
// FUNCTION: WIZ8 0x00433660
unsigned long W8Octree::FindLeaf(const srVector3T<int>* point)
{
    int level = m_spatial.m_depth;
    int mask = 1 << m_spatial.m_depth;
    unsigned long node = 1;

    do {
        if (level < 1) {
            break;
        }
        mask /= 2;
        int child = 0;
        if ((point->x & mask) != 0) {
            child = 4;
        }
        if ((point->y & mask) != 0) {
            child += 2;
        }
        if ((point->z & mask) != 0) {
            child += 1;
        }
        node = m_branches[node].children[child];
        --level;
    } while (node != 0);
    if (m_leaf_count < node) {
        return 0;
    }
    return node;
}

/* Descend the branch tree while `masked_cell`'s leading mask word keeps the
   current level bit set, choosing the octant from the three coordinate words.
   The region validators compare the returned node's region field. */
// FUNCTION: WIZ8 0x004336d0
int W8Octree::DescendByMask(const unsigned int* masked_cell)
{
    int node = 1;
    int bit = 1 << m_spatial.m_depth;
    while (bit != 0 && node != 0) {
        if ((masked_cell[0] & bit) != 0) {
            int octant = 0;
            if ((bit & masked_cell[1]) != 0) {
                octant = 4;
            }
            if ((masked_cell[2] & bit) != 0) {
                octant += 2;
            }
            if ((masked_cell[3] & bit) != 0) {
                octant += 1;
            }
            node = m_branches[node].children[octant];
        }
        bit /= 2;
    }
    return node;
}

/* Bounds-checked cell -> leaf index shared by the cell probes: the direct
   leaf grid when one exists, else the same masked branch descent. */
// FUNCTION: WIZ8 0x00433730
unsigned int W8Octree::LeafIndexForCell(const srVector3T<int>* cell)
{
    if (cell->x < 0 || static_cast<int>(m_leaf_grid_dimensions.x) <= cell->x || cell->y < 0 ||
        static_cast<int>(m_leaf_grid_dimensions.y) <= cell->y || cell->z < 0 ||
        static_cast<int>(m_leaf_grid_dimensions.z) <= cell->z) {
        return 0;
    }
    if (m_leaf_lookup != 0) {
        return m_leaf_lookup[m_spatial.m_leaf_grid_stride_x * cell->x + cell->z +
                             m_spatial.m_leaf_grid_stride_y * cell->y];
    }
    return FindLeaf(cell);
}

/* Settle a point onto the geometry below it: seed a downward trace from the
   point raised by `limit` and march the cell column downward, optionally
   testing the cell's props first (a prop hit is remembered in current_prop;
   a level-surface hit clears it) and always testing the level surfaces.
   The point's y drops to the contact on a hit and keeps its input value on a
   miss; `out_hit` receives the outcome byte when given. */
// FUNCTION: WIZ8 0x00433820
float W8Octree::SettleToGround(srVector3T<float>* position, bool* out_hit, char test_props,
                               float limit)
{
    W8OctreeTrace trace;
    srVector3T<int> cell;
    srVector3T<float> start;
    srVector3T<float> end;
    bool prop_hit = false;
    bool hit = false;

    m_gd_result_count = 0;
    if (test_props != 0) {
        current_prop = -1;
    }
    m_visited_object_bits->ClearAll();
    if (m_spatial.m_clipped_maximum.y < position->y) {
        position->y = m_spatial.m_clipped_maximum.y;
    }
    end = *position;
    start = *position;
    start.y = position->y + limit;
    WorldPositionToCell(&start, &cell);
    end.y = ((cell.y - 1)) * m_spatial.m_node_extent + m_spatial.m_minimum.y;
    trace.Reseed(&start, &end);
    if (-1 < cell.y) {
        do {
            if (hit) {
                goto done;
            }
            if (test_props != 0) {
                g_octree_state = m_aulGDObjs;
                m_gd_result_count = 0;
                unsigned long before = m_gd_result_count;
                CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                if (m_gd_result_count != before) {
                    int prop = g_octree_game_data->TestPropSurfaces(m_gd_result_count, m_aulGDObjs,
                                                                    &trace, 0, 0);
                    current_prop = prop;
                    if (prop >= 0) {
                        prop_hit = true;
                    }
                }
            }
            g_octree_game_data->trace_flag4_gate = true;
            if (ProbeCellForTrace(&cell) == 0) {
                goto descend;
            }
            hit = g_octree_game_data->TestTraceResult(m_gd_result_count, m_aulGDObjs, &trace, 0, 0);
            if (!hit) {
            descend:
                if (prop_hit) {
                    hit = true;
                } else {
                    float level = static_cast<float>(cell.y);
                    --cell.y;
                    start.y = level * m_spatial.m_node_extent + m_spatial.m_minimum.y;
                    end.y -= m_spatial.m_node_extent;
                    trace.Reseed(&start, &end);
                }
            } else if (prop_hit) {
                current_prop = -1;
            }
            g_octree_game_data->trace_flag4_gate = false;
        } while (-1 < cell.y);
        if (hit) {
        done:
            position->y = trace.end.y;
            goto out;
        }
    }
    trace.end.y = position->y;
out:
    if (out_hit != 0) {
        *out_hit = hit;
    }
    return trace.end.y;
}

/* Snap a point onto the surface below: clamp it to the octree's clipped
   ceiling, probe the ground one world-scale unit lower and keep the settled
   height on a hit. The path builders and navigator placement use it to drop
   points onto terrain. */
// FUNCTION: WIZ8 0x00431d20
bool W8Octree::SnapToGround(srVector3T<float>* position, char mode)
{
    bool hit = false;
    if (position->y > m_spatial.m_clipped_maximum.y) {
        position->y = m_spatial.m_clipped_maximum.y;
    }
    srVector3T<float> probe = *position;
    probe.y = position->y - g_world_scale;
    SettleToGround(&probe, &hit, mode, 500.0f);
    if (hit) {
        position->y = probe.y;
    }
    return hit;
}

/* Whether one point can see another, and where the line stops if it cannot.

   Both of these walk the same cell line. A line inside one or two cells probes
   those directly; anything longer builds a walk and steps it, advancing the
   driving axis every iteration and each minor axis whenever its accumulator
   goes negative - and probing after every one of those advances, so a blocker
   in a diagonally-crossed cell is not stepped over. The walk stops at the first
   blocker.

   HasLineOfSight answers the question and lets the caller fall back to a prop
   trace; TraceLineOfSight additionally reports where the line was stopped and
   distinguishes a world hit from a prop hit by the sign of its answer. */
// FUNCTION: WIZ8 0x00434b60
bool W8Octree::HasLineOfSight(const srVector3T<float>* from, srVector3T<float>* to,
                              bool allow_fallback)
{
    W8OctreeWalk walk;
    srVector3T<int> cell;
    srVector3T<int> end_cell;
    srVector3T<int> step;
    unsigned char blocked = 0;
    int span;
    int error_0;
    int error_1;
    int minor_1;
    int count;
    int index;

    W8OctreeTrace trace(from, to);

    m_gd_result_count = 0;
    m_visited_polygon_bits->ClearAll();
    m_current_regions->ClearAll();
    cell.x = static_cast<int>(((from->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent));
    end_cell.x = static_cast<int>(((to->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent));
    cell.y = static_cast<int>(((from->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent));
    end_cell.y = static_cast<int>(((to->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent));
    cell.z = static_cast<int>(((from->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent));
    end_cell.z = static_cast<int>(((to->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent));
    span = abs(cell.z - end_cell.z) + abs(cell.y - end_cell.y) + abs(cell.x - end_cell.x);

    if (span < 2) {
        ProbeCellForBlockers(&cell);
        blocked = TestProbeResult(&trace);
        if (blocked == 0 && span != 0) {
            ProbeCellForBlockers(&end_cell);
            blocked = TestProbeResult(&trace);
        }
    } else {
        BuildCellWalk(from, to, &walk);
        cell = walk.cell;
        minor_1 = walk.minor_axis1;
        step = walk.step;
        count = walk.count;
        index = 0;
        error_1 = walk.error1;
        error_0 = walk.error0;
        if (walk.count > 0) {
            do {
                if (blocked != 0) {
                    break;
                }
                if (ProbeCellForBlockers(&cell) != 0) {
                    blocked = TestProbeResult(&trace);
                }
                if (error_0 < error_1) {
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset0;
                        (&cell.x)[walk.minor_axis0] += (&step.x)[walk.minor_axis0];
                        if (ProbeCellForBlockers(&cell) != 0) {
                            blocked = TestProbeResult(&trace);
                        }
                        if (error_1 < 0 && blocked == 0) {
                            (&cell.x)[minor_1] += (&step.x)[minor_1];
                            error_1 += walk.error_reset1;
                            if (ProbeCellForBlockers(&cell) != 0) {
                                blocked = TestProbeResult(&trace);
                            }
                        }
                    }
                } else if (error_1 < 0 && blocked == 0) {
                    (&cell.x)[minor_1] += (&step.x)[minor_1];
                    error_1 += walk.error_reset1;
                    if (ProbeCellForBlockers(&cell) != 0) {
                        blocked = TestProbeResult(&trace);
                    }
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset0;
                        (&cell.x)[walk.minor_axis0] += (&step.x)[walk.minor_axis0];
                        if (ProbeCellForBlockers(&cell) != 0) {
                            blocked = TestProbeResult(&trace);
                        }
                    }
                }
                (&cell.x)[walk.major_axis] += (&step.x)[walk.major_axis];
                error_1 -= walk.error_delta1;
                error_0 -= walk.error_delta0;
                ++index;
            } while (index < count);
        }
    }
    if (blocked != 0) {
        *to = trace.end;
    } else if (allow_fallback && TraceAgainstProps(from, to, 1, 1) != 0) {
        blocked = 1;
    }
    return blocked == 0;
}

// FUNCTION: WIZ8 0x00434f20
short W8Octree::TraceLineOfSight(const srVector3T<float>* from, srVector3T<float>* to,
                                 bool trace_world, int from_location_id, int to_location_id,
                                 bool visit_octree, int trace_mode)
{
    W8OctreeWalk walk;
    srVector3T<int> cell;
    srVector3T<int> end_cell;
    srVector3T<int> step;

    bool blocked = false;
    bool previous = false;
    int span;
    int error_0;
    int error_1;
    int minor_0;
    int minor_1;
    int major;
    int index;
    int count;
    int result;
    int hit_location;

    W8OctreeTrace trace(from, to);
    result = 0;

    if (visit_octree) {
        m_gd_result_count = 0;
        m_visited_object_bits->ClearAll();
        cell.x = static_cast<int>(((from->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent));
        end_cell.x = static_cast<int>(((to->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent));
        cell.y = static_cast<int>(((from->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent));
        end_cell.y = static_cast<int>(((to->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent));
        cell.z = static_cast<int>(((from->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent));
        end_cell.z = static_cast<int>(((to->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent));
        span = abs(cell.z - end_cell.z) + abs(cell.y - end_cell.y) + abs(cell.x - end_cell.x);

        if (span < 2) {
            ProbeCellForTrace(&cell);
            blocked = g_octree_game_data->TestTraceResult(m_gd_result_count, m_aulGDObjs, &trace,
                                                          m_trace_skip_flag, 0);

            if (!blocked && span != 0) {
                ProbeCellForTrace(&end_cell);
                blocked = g_octree_game_data->TestTraceResult(m_gd_result_count, m_aulGDObjs,
                                                              &trace, m_trace_skip_flag, 0);
            }
        } else {
            BuildCellWalk(from, to, &walk);
            cell = walk.cell;
            minor_0 = walk.minor_axis0;
            major = walk.major_axis;
            minor_1 = walk.minor_axis1;
            step = walk.step;
            count = walk.count;
            index = 0;
            error_1 = walk.error1;
            error_0 = walk.error0;
            previous = false;
            if (walk.count > 0) {
                do {
                    blocked = previous;
                    if (blocked) {
                        break;
                    }
                    if (ProbeCellForTrace(&cell) != 0) {
                        blocked = g_octree_game_data->TestTraceResult(
                            m_gd_result_count, m_aulGDObjs, &trace, m_trace_skip_flag, 0);
                    }
                    if (error_0 < error_1) {
                        if (error_0 < 0 && !blocked) {
                            (&cell.x)[minor_0] += (&step.x)[minor_0];
                            error_0 += walk.error_reset0;
                            if (ProbeCellForTrace(&cell) != 0) {
                                blocked = g_octree_game_data->TestTraceResult(
                                    m_gd_result_count, m_aulGDObjs, &trace, m_trace_skip_flag, 0);
                            }
                            if (error_1 < 0 && !blocked) {
                                (&cell.x)[minor_1] += (&step.x)[minor_1];
                                error_1 += walk.error_reset1;
                                if (ProbeCellForTrace(&cell) != 0) {
                                    blocked = g_octree_game_data->TestTraceResult(
                                        m_gd_result_count, m_aulGDObjs, &trace, m_trace_skip_flag,
                                        0);
                                }
                            }
                        }
                    } else if (error_1 < 0 && !blocked) {
                        (&cell.x)[minor_1] += (&step.x)[minor_1];
                        error_1 += walk.error_reset1;
                        if (ProbeCellForTrace(&cell) != 0) {
                            blocked = g_octree_game_data->TestTraceResult(
                                m_gd_result_count, m_aulGDObjs, &trace, m_trace_skip_flag, 0);
                        }
                        if (error_0 < 0 && !blocked) {
                            (&cell.x)[minor_0] += (&step.x)[minor_0];
                            error_0 += walk.error_reset0;
                            if (ProbeCellForTrace(&cell) != 0) {
                                blocked = g_octree_game_data->TestTraceResult(
                                    m_gd_result_count, m_aulGDObjs, &trace, m_trace_skip_flag, 0);
                            }
                        }
                    }
                    (&cell.x)[major] += (&step.x)[major];
                    error_1 -= walk.error_delta1;
                    error_0 -= walk.error_delta0;
                    ++index;
                    previous = blocked;
                } while (index < count);
            }
        }
        if (!trace_world || TraceAgainstProps(from, &trace.end, 0, 0) == 0) {
            if (!blocked) {
                goto resolve;
            }
        } else {
            blocked = true;
        }
        result = 1;
        if (blocked) {
            *to = trace.end;
            return 1;
        }
    }
resolve:
    if (from_location_id > -3) {
        hit_location = to_location_id;
        if (ResolveTraceHit(&trace.start, &trace.end, from_location_id, &hit_location,
                            to_location_id, 0, trace_mode) != 0) {
            *to = trace.end;
            return -1;
        }
    }
    return static_cast<short>(result);
}

/* Nearest ray-vs-sphere hit across the kind-12 objects in the segment box,
   then against the camera sphere. `hit_location` carries the target's
   location id in (a null or negative in-value skips the to-exclusion and the
   probe set) and receives the winning id, 0 for the camera, or -1 on a miss.
   `excluded`/`location` skip the two endpoint objects; `flags` masks each
   monster's navigator trace_mask; `noise_adjust` applies the range-scaled
   noise penalty. The winning offset is the last colliding candidate's, not
   necessarily the nearest id's - the retail quirk is preserved. */
// FUNCTION: WIZ8 0x004353f0
char W8Octree::ResolveTraceHit(const srVector3T<float>* from, srVector3T<float>* to, int excluded,
                               int* hit_location, int location, unsigned int flags,
                               char noise_adjust)
{
    unsigned int index = 0;
    unsigned long* ids = 0;
    unsigned int best_index = 0;
    double best = -1.0;
    float segment_length = 0.0f;
    float inflate;
    int target;
    W8Navigator* navigator;
    float radius;
    unsigned int probe_set = 0;
    srVector3T<float> low;
    srVector3T<float> high;
    srVector3T<float> offset;
    srVector3T<float> direction;
    srVector3T<float> center;
    srVector3T<float> camera;

    if (noise_adjust != 0) {
        segment_length = static_cast<float>(sqrt((to->x - from->x) * (to->x - from->x) +
                                                 (to->y - from->y) * (to->y - from->y) +
                                                 (to->z - from->z) * (to->z - from->z)));
    }
    inflate = g_position_height_epsilon;
    if (g_runtime_world_scale > inflate) {
        inflate = g_runtime_world_scale;
    }
    if (hit_location == 0) {
        target = -3;
    } else {
        target = *hit_location;
    }
    high = *from;
    low = *from;
    if (low.x > to->x) {
        low.x = to->x;
    } else {
        high.x = to->x;
    }
    if (low.y > to->y) {
        low.y = to->y;
    } else {
        high.y = to->y;
    }
    if (low.z > to->z) {
        low.z = to->z;
    } else {
        high.z = to->z;
    }
    low.x -= inflate;
    low.y -= inflate;
    low.z -= inflate;
    high.x += inflate;
    high.y += inflate;
    high.z += inflate;
    if (target == -1) {
        radius = g_startup_world->movement.alternate_radius;
        navigator = g_startup_world;
    } else {
        if (target < 1) {
            goto no_probes;
        }
        unsigned int monster_index = MonsterGetIndexByLocationID(0x1836, OCTREE_CPP, target, true);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info == 0 || info->p3D == 0) {
            goto no_probes;
        }
        navigator = info->p3D;
        radius = navigator->movement.alternate_radius;
    }
    if (navigator != 0 && pathing != 0) {
        probe_set = pathing->CollectPathProbes(&navigator->movement, radius);
    }
no_probes:;
    unsigned int count = static_cast<unsigned int>(
        g_octree->QueryObjects(&ids, &low, &high, W8_OCTREE_KIND_LOCATION, -1)); /* c-style-cast-ok:
            the shared query count field is stored unsigned */
    if (count != 0) {
        do {
            int id = ids[index];
            if (((excluded < 0) || (excluded != id)) && ((target < 0) || (location != id)) &&
                (probe_set == 0 || !pathing->MatchesPathProbe(id, 0, 0))) {
                unsigned int monster_index =
                    MonsterGetIndexByLocationID(0x1851, OCTREE_CPP, id, true);
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
                if (info != 0) {
                    W8Monster* monster = info->p3D;
                    if (monster != 0 && monster->active && (monster->trace_mask & flags) == 0) {
                        center.x = monster->movement.position.x;
                        center.z = monster->movement.position.z;
                        center.y = monster->movement.position.y + monster->movement.height_offset;
                        float distance = PointToSegmentDistance(&center, from, to, true, 0);
                        if (noise_adjust != 0) {
                            distance =
                                distance - (static_cast<float>(
                                                sqrt((center.x - from->x) * (center.x - from->x) +
                                                     (center.y - from->y) * (center.y - from->y) +
                                                     (center.z - from->z) * (center.z - from->z))) /
                                                segment_length * g_float_fifteen_hundredths +
                                            g_float_one_tenth) *
                                               g_world_scale;
                            if (distance < g_float_zero) {
                                distance = g_float_zero;
                            }
                        }
                        float monster_radius = monster->radius;
                        if (distance < monster_radius) {
                            direction = *to - *from;
                            double length2 = static_cast<double>((direction.LengthSquared()));
                            offset = direction;
                            if (length2 != g_double_zero) {
                                offset *= (sqrt(length2) - sqrt(monster_radius * monster_radius -
                                                                distance * distance)) /
                                          sqrt(length2);
                            }
                            if (0.0 <= best) {
                                float span = offset.x * offset.x;
                                if (sqrt(offset.z * offset.z + offset.y * offset.y + span) < best) {
                                    best = sqrt(offset.y * offset.y + offset.z * offset.z + span);
                                    best_index = index;
                                }
                            } else {
                                best = sqrt(offset.LengthSquared());
                                best_index = index;
                            }
                        }
                    }
                }
            }
            ++index;
        } while (index < count);
        /* Retail verified: `offset` holds the last in-radius collider's
           offset, not necessarily the best one's — best/best_index update
           only on a shorter hit while `to` is written from the current
           `offset`, so the two can describe different monsters. */
        if (0.0 <= best) {
            *to = offset + *from;
            if (hit_location != 0) {
                *hit_location = ids[best_index];
            }
            return 1;
        }
    }
    if (excluded != -1 && location != -1) {
        GetCameraPosition(&camera);
        center = camera;
        float distance = PointToSegmentDistance(&center, from, to, true, 0);
        if (noise_adjust != 0) {
            distance =
                distance - (static_cast<float>(sqrt((center.y - from->y) * (center.y - from->y) +
                                                    (center.z - from->z) * (center.z - from->z) +
                                                    (center.x - from->x) * (center.x - from->x))) /
                                segment_length * g_float_fifteen_hundredths +
                            g_float_one_tenth) *
                               g_world_scale;
            if (distance < g_float_zero) {
                distance = g_float_zero;
            }
        }
        float camera_radius = g_startup_world->radius * g_camera_sphere_radius_scale;
        if (distance < camera_radius) {
            offset = *to - *from;
            double length2 = static_cast<double>(
                (offset.y * offset.y + offset.z * offset.z + offset.x * offset.x));
            if (length2 != g_double_zero) {
                offset *=
                    (sqrt(length2) - sqrt(camera_radius * camera_radius - distance * distance)) /
                    sqrt(length2);
            }
            *to = offset + *from;
            if (hit_location != 0) {
                *hit_location = 0;
            }
            return 1;
        }
    }
    /* Retail verified at 0x00435AEF: unlike the guarded writes above, the
       miss path stores through `hit_location` unconditionally, so a null
       out-pointer crashes here exactly as in retail. */
    *hit_location = -1;
    return 0;
}

/* Second Remove emission (0x00438C90 above is the other), serving the
   keyboard/automap tables' callers. */

// FUNCTION: WIZ8 0x00436840
W8OctreeObjectRegistry::W8OctreeObjectRegistry()
    : by_cell(new W8OctreeIndex), by_object(new W8OctreeIndex)
{
}

// FUNCTION: WIZ8 0x00436b20
W8OctreeObjectRegistry::~W8OctreeObjectRegistry()
{
    delete by_cell;
    delete by_object;
}

// FUNCTION: WIZ8 0x00436b90
unsigned char W8OctreeObjectRegistry::MoveObjectToCell(W8OctreeObjectKind kind, int id,
                                                       const srVector3T<int>* point)
{
    unsigned int object_key = PackOctreeObjectKey(kind, id);
    unsigned int cell_key = PackOctreeCellKey(point->x, point->y, point->z);
    int object_value = static_cast<int>(object_key);
    int cell_value = static_cast<int>(cell_key);

    by_object->Remove(&object_key, &cell_value);
    by_object->Insert(&object_key, &cell_value);
    by_cell->Remove(&cell_key, &object_value);
    by_cell->Insert(&cell_key, &object_value);
    return 1;
}

// FUNCTION: WIZ8 0x00436dc0
unsigned char W8OctreeObjectRegistry::UnregisterObject(W8OctreeObjectKind kind, int id)
{
    unsigned int object_key = PackOctreeObjectKey(kind, id);
    int object_value = static_cast<int>(object_key);
    bool removed = false;

    int slot = by_object->FindNextEntry(&object_key, -1);
    while (slot != -1) {
        unsigned int cell_key = static_cast<unsigned int>(by_object->entries[slot].value);
        by_object->RemoveAt(slot);
        by_cell->Remove(&cell_key, &object_value);
        removed = true;
        // Retail traverses from the removed slot after its link became a free-list link.
        slot = by_object->FindNextEntry(&object_key, slot);
    }
    return removed;
}

/* Record that one object now occupies one cell.

   The object's key is its kind in the high half and its id in the low half.
   When it is already registered somewhere, the cell it was in is compared
   against the one being queued using the degenerate retail coordinate test
   below; otherwise the old pairing comes out of both indexes first. A new
   location registration additionally maintains a pairing under the navigator
   kind.

   The two indexes are the same pair AddCollidablePropBounds keeps: one keyed by
   cell, one keyed by object. */
// FUNCTION: WIZ8 0x00437000
unsigned char W8OctreeObjectRegistry::RegisterObjectCell(W8OctreeObjectKind kind, int id,
                                                         const srVector3T<int>* point)
{
    unsigned int object_key = PackOctreeObjectKey(kind & 0xffff, id);
    int occupied = by_object->Lookup(&object_key);

    if (occupied != 0) {
        /* Retail only recognizes a repeat of (0,0,z): x is not decoded,
           and the old y byte is shifted too far. Preserve that test. */
        if (point->x == 0 && (((occupied - 1) & 0xff00) << 8) == point->y &&
            OctreeCellKeyZ(occupied) == point->z) {
            return 1;
        }
        unsigned int occupied_key = static_cast<unsigned int>(occupied);
        int object_value = static_cast<int>(object_key);
        by_object->Remove(&object_key);
        by_cell->Remove(&occupied_key, &object_value);
    } else if (static_cast<short>(kind) == W8_OCTREE_KIND_LOCATION) {
        unsigned int cell_key = PackOctreeCellKey(point->x, point->y, point->z);
        int cell_value = static_cast<int>(cell_key);
        unsigned int tagged_key = PackOctreeObjectKey(W8_OCTREE_KIND_NAVIGATOR, id);
        int tagged_value = static_cast<int>(tagged_key);
        by_object->Remove(&tagged_key, &cell_value);
        by_object->Insert(&tagged_key, &cell_value);
        by_cell->Remove(&cell_key, &tagged_value);
        by_cell->Insert(&cell_key, &tagged_value);
    }

    unsigned int cell_key = PackOctreeCellKey(point->x, point->y, point->z);
    int cell_value = static_cast<int>(cell_key);
    int object_value = static_cast<int>(object_key);
    by_object->Remove(&object_key, &cell_value);
    by_object->Insert(&object_key, &cell_value);
    by_cell->Remove(&cell_key, &object_value);
    by_cell->Insert(&cell_key, &object_value);
    return 1;
}

/* Register one collidable prop against every octree cell its bounding box
   touches.

   Two indexes are kept in step: one keyed by cell so a cell can name its props,
   one keyed by prop so a prop can name its cells. Each cell first drops any
   matching pairing in both directions before the new one goes in. The cell key
   packs x, y and z into one dword a byte apart, and the prop key carries its id
   in the low half with a tag above it. */
// FUNCTION: WIZ8 0x0042eab0
void W8Octree::AddCollidablePropBounds(int index, const W8BoundingBox* bounds)
{
    srVector3T<int> minimum;
    srVector3T<int> maximum;
    W8OctreeIndex* by_prop;
    W8OctreeIndex* by_cell;
    unsigned int prop_key;
    unsigned int cell_key;
    int axis;
    int x;
    int y;
    int z;

    for (axis = 0; axis < 3; ++axis) {
        (&minimum.x)[axis] =
            static_cast<int>(((&bounds->minimum.x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                             m_spatial.m_node_extent);
    }
    for (axis = 0; axis < 3; ++axis) {
        (&maximum.x)[axis] =
            static_cast<int>(((&bounds->maximum.x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                             m_spatial.m_node_extent);
    }

    prop_key = PackOctreeObjectKey(W8_OCTREE_KIND_PROP, index + 1);
    for (x = minimum.x; x <= maximum.x; ++x) {
        for (y = minimum.y; y <= maximum.y; ++y) {
            for (z = minimum.z; z <= maximum.z; ++z) {
                cell_key = PackOctreeCellKey(x, y, z);
                int cell_value = static_cast<int>(cell_key);
                int prop_value = static_cast<int>(prop_key);
                by_prop = object_registry->by_object;
                by_prop->Remove(&prop_key, &cell_value);
                by_prop->Insert(&prop_key, &cell_value);
                by_cell = object_registry->by_cell;
                by_cell->Remove(&cell_key, &prop_value);
                by_cell->Insert(&cell_key, &prop_value);
            }
        }
    }
}

/* Build the cell walk between two world points.

   The three axis deltas are taken in cells; the longest of them drives, and the
   other two each get an error triple seeded from where inside its cell the line
   starts. The step count covers the driving axis plus the partial cell at each
   end, which is why the remainder decides between one and two extra. */
// FUNCTION: WIZ8 0x004362d0
void W8Octree::BuildCellWalk(const srVector3T<float>* from, const srVector3T<float>* to,
                             W8OctreeWalk* walk)
{
    srVector3T<int> from_cell;
    srVector3T<int> to_cell;
    srVector3T<float> fraction;
    srVector3T<float> delta;
    srVector3T<int> step;
    srVector3T<int> extent;
    float cell_size;
    int cell;
    int axis;
    int longest = 0;
    int major = 0;
    int minor_0;
    int minor_1;
    int span;

    cell_size = m_spatial.m_node_extent * g_octree_cell_scale;
    cell = static_cast<int>(cell_size);
    from_cell.x = static_cast<int>(((from->x - m_spatial.m_minimum.x) * g_octree_cell_scale));
    to_cell.x = static_cast<int>(((to->x - m_spatial.m_minimum.x) * g_octree_cell_scale));
    from_cell.y = static_cast<int>(((from->y - m_spatial.m_minimum.y) * g_octree_cell_scale));
    to_cell.y = static_cast<int>(((to->y - m_spatial.m_minimum.y) * g_octree_cell_scale));
    from_cell.z = static_cast<int>(((from->z - m_spatial.m_minimum.z) * g_octree_cell_scale));
    to_cell.z = static_cast<int>(((to->z - m_spatial.m_minimum.z) * g_octree_cell_scale));

    for (axis = 0; axis < 3; ++axis) {
        span = (&to_cell.x)[axis] - (&from_cell.x)[axis];
        (&fraction.x)[axis] = (((&from_cell.x)[axis] % cell)) / cell_size;
        (&delta.x)[axis] = static_cast<float>(span);
        if (span < 0) {
            (&step.x)[axis] = -1;
            span = -span;
        } else {
            (&step.x)[axis] = 1;
            (&fraction.x)[axis] = 1.0f - (&fraction.x)[axis];
        }
        if (longest < span) {
            major = axis;
            longest = span;
        }
        (&extent.x)[axis] = span;
    }

    minor_0 = (major + 1) % 3;
    minor_1 = (major + 2) % 3;
    walk->error_delta0 = static_cast<int>(
        (static_cast<float>(fabs((&delta.x)[minor_0] / (&delta.x)[major])) * cell_size));
    walk->error0 = static_cast<int>(
        (cell * (&fraction.x)[minor_0] - walk->error_delta0 * (&fraction.x)[major]));
    walk->error_delta1 = static_cast<int>(
        (static_cast<float>(fabs((&delta.x)[minor_1] / (&delta.x)[major])) * cell_size));
    walk->error1 = static_cast<int>(
        (cell * (&fraction.x)[minor_1] - walk->error_delta1 * (&fraction.x)[major]));
    walk->count = longest % cell == 0 ? longest / cell + 1 : longest / cell + 2;

    walk->minor_axis0 = minor_0;
    walk->error_reset0 = cell;
    walk->error_reset1 = cell;
    walk->minor_axis1 = minor_1;
    walk->major_axis = major;
    walk->cell.x = from_cell.x / cell;
    walk->cell.y = from_cell.y / cell;
    walk->cell.z = from_cell.z / cell;
    walk->step = step;
}

// FUNCTION: WIZ8 0x00436280
void W8Octree::BuildCellWalk(srVector3T<float> from, srVector3T<float> to, W8OctreeWalk* walk)
{
    BuildCellWalk(&from, &to, walk);
}

/* Walk the segment's cells collecting every kind-8 (collidable prop) registry
   entry into the shared buffer, then hand the accumulated id list to the
   GameData prop trace. The returned index lands in current_prop and, when
   non-negative, `to` is rewritten with the record's contact point. */
// FUNCTION: WIZ8 0x00436510
int W8Octree::TraceAgainstProps(const srVector3T<float>* from, srVector3T<float>* to, int skip_flag,
                                int gate)
{
    W8OctreeWalk walk;
    srVector3T<int> cell;
    srVector3T<int> end_cell;
    srVector3T<int> step;
    int span;
    int error_0;
    int error_1;
    int minor_1;

    W8OctreeTrace trace(from, to);
    g_octree_state = m_aulGDObjs;
    m_gd_result_count = 0;
    m_visited_object_bits->ClearAll();
    cell.x = static_cast<int>((from->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent);
    end_cell.x = static_cast<int>((to->x - m_spatial.m_minimum.x) / m_spatial.m_node_extent);
    cell.y = static_cast<int>((from->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent);
    end_cell.y = static_cast<int>((to->y - m_spatial.m_minimum.y) / m_spatial.m_node_extent);
    cell.z = static_cast<int>((from->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent);
    end_cell.z = static_cast<int>((to->z - m_spatial.m_minimum.z) / m_spatial.m_node_extent);
    span = abs(cell.z - end_cell.z) + abs(cell.y - end_cell.y) + abs(cell.x - end_cell.x);
    if (span < 2) {
        g_octree_state = m_aulGDObjs;
        CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
        if (span != 0) {
            g_octree_state = m_aulGDObjs;
            CollectObjectsInCell(&end_cell, W8_OCTREE_KIND_PROP);
        }
    } else {
        BuildCellWalk(from, to, &walk);
        cell = walk.cell;
        minor_1 = walk.minor_axis1;
        step = walk.step;
        if (walk.count > 0) {
            int major_step = (&step.x)[walk.major_axis];
            int* major_cell = &cell.x + walk.major_axis;
            int count = walk.count;
            error_1 = walk.error1;
            error_0 = walk.error0;
            do {
                g_octree_state = m_aulGDObjs;
                CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                if (error_0 < error_1) {
                    if (error_0 < 0) {
                        error_0 += walk.error_reset0;
                        (&cell.x)[walk.minor_axis0] += (&step.x)[walk.minor_axis0];
                        g_octree_state = m_aulGDObjs;
                        CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                        if (error_1 < 0) {
                            (&cell.x)[minor_1] += (&step.x)[minor_1];
                            error_1 += walk.error_reset1;
                            g_octree_state = m_aulGDObjs;
                            CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                        }
                    }
                } else if (error_1 < 0) {
                    (&cell.x)[minor_1] += (&step.x)[minor_1];
                    g_octree_state = m_aulGDObjs;
                    error_1 += walk.error_reset1;
                    CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                    if (error_0 < 0) {
                        (&cell.x)[walk.minor_axis0] += (&step.x)[walk.minor_axis0];
                        error_0 += walk.error_reset0;
                        g_octree_state = m_aulGDObjs;
                        CollectObjectsInCell(&cell, W8_OCTREE_KIND_PROP);
                    }
                }
                *major_cell += major_step;
                error_0 -= walk.error_delta0;
                error_1 -= walk.error_delta1;
                --count;
            } while (count != 0);
        }
    }
    current_prop = g_octree_game_data->TestPropSurfaces(m_gd_result_count, m_aulGDObjs, &trace,
                                                        skip_flag, gate);
    if (current_prop < 0) {
        return 0;
    }
    *to = trace.end;
    return current_prop + 1;
}

/* The cell probes share one lookup: bounds-check the cell against the grid,
   resolve its leaf index through the direct leaf grid when one exists else by
   descending the branch tree one octant per level, then drain the chosen leaf
   stream into m_aulGDObjs through the m_visited_object_bits dedupe set. ProbeCellForTrace
   reads the leaf's gd_polygon_offset into the m_gd_surface_index_stream surface-id stream;
   ProbeCellForBlockers reads polygon_offset into the m_polygon_index_stream index stream
   and maps each entry through m_aulPolyLookup to a (mesh<<16)|polygon key, marking
   the mesh in m_current_regions. The Append variant skips the result-count
   reset so successive cells accumulate. */
/* Cell probes share LeafIndexForCell; their retail bodies expand this lookup. */
// FUNCTION: WIZ8 0x00435b00
int W8Octree::ProbeCellForTrace(const srVector3T<int>* cell)
{
    unsigned int leaf_index = LeafIndexForCell(cell);
    m_gd_result_count = 0;
    if (leaf_index != 0 && m_leaves[leaf_index].gd_polygon_offset != 0) {
        const unsigned long* stream =
            m_gd_surface_index_stream + m_leaves[leaf_index].gd_polygon_offset;
        int remaining = *stream;
        while (remaining != 0) {
            ++stream;
            if (9999 < m_gd_result_count) {
                break;
            }
            if (!m_visited_object_bits->Set(*stream)) {
                m_aulGDObjs[m_gd_result_count] = *stream;
                ++m_gd_result_count;
            }
            --remaining;
        }
    }
    return m_gd_result_count;
}

// FUNCTION: WIZ8 0x00435c40
int W8Octree::ProbeCellForBlockers(const srVector3T<int>* cell)
{
    unsigned int leaf_index = LeafIndexForCell(cell);
    m_gd_result_count = 0;
    if (leaf_index != 0 && m_leaves[leaf_index].polygon_offset != 0) {
        const unsigned long* stream = m_polygon_index_stream + m_leaves[leaf_index].polygon_offset;
        AppendBlockerStream(stream);
    }
    return m_gd_result_count;
}

void W8Octree::AppendBlockerStream(const unsigned long* stream)
{
    for (int remaining = *stream; remaining != 0; --remaining) {
        ++stream;
        if (!m_visited_polygon_bits->Set(*stream)) {
            if (9999 < m_gd_result_count) {
                break;
            }
            unsigned int key = m_aulPolyLookup[*stream];
            m_aulGDObjs[m_gd_result_count] = key;
            ++m_gd_result_count;
            m_current_regions->Set(key >> 0x10);
        }
    }
}

// FUNCTION: WIZ8 0x00435da0
int W8Octree::ProbeCellForBlockersAppend(const srVector3T<int>* cell)
{
    unsigned int leaf_index = LeafIndexForCell(cell);
    if (leaf_index != 0 && m_leaves[leaf_index].polygon_offset != 0) {
        const unsigned long* stream = m_polygon_index_stream + m_leaves[leaf_index].polygon_offset;
        AppendBlockerStream(stream);
    }
    return m_gd_result_count;
}

/* March the buffered (mesh<<16)|polygon keys, ray-test each live mesh
   instance's triangle against the record and keep the nearest contact: the
   record's end returns the hit position and hit_limit the distance. */
// FUNCTION: WIZ8 0x00435f00
unsigned char W8Octree::TestProbeResult(W8OctreeTrace* trace)
{
    bool hit = false;
    srVector3T<float> contact;

    for (unsigned int index = 0; index < m_gd_result_count; ++index) {
        unsigned int mesh_index = m_aulGDObjs[index] >> 0x10;
        if (mesh_index < m_meshCount && g_world->psrMeshes[mesh_index] != 0 &&
            !m_pAlphaBits->Test(mesh_index)) {
            stMeshModel* model =
                static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->getModel());
            const srVector4T<float>* planes = model->getPolyEq();
            unsigned int polygon = m_aulGDObjs[index] & 0xffff;
            const srVector4T<float>* plane = planes + polygon;
            float t;
            float distance;
            srVector3T<float> point;
            if (plane->x * trace->step.x + trace->step.y * plane->y + trace->step.z * plane->z <=
                    g_float_zero &&
                (distance = plane->y * trace->start.y + plane->x * trace->start.x +
                            plane->z * trace->start.z + plane->w,
                 distance <= trace->hit_limit) &&
                g_float_zero < distance) {
                if (g_float_one <= distance) {
                    float back = plane->x * trace->end.x + trace->end.y * plane->y +
                                 trace->end.z * plane->z + plane->w;
                    if (g_float_one <= back) {
                        continue;
                    }
                    back = -back;
                    if (g_float_one <= back || trace->hit_limit < trace->length) {
                        t = (distance / (back + distance)) * trace->length;
                        point.x = trace->step.x * t;
                        point.y = trace->step.y * t;
                        point.x += trace->start.x;
                        point.y += trace->start.y;
                        point.z = t * trace->step.z + trace->start.z;
                    } else {
                        point = trace->end;
                        t = trace->length;
                    }
                } else {
                    point = trace->start;
                    t = 0.0f;
                }
                float abs_x = fabsf(plane->x);
                float abs_y = fabsf(plane->y);
                float widest = abs_x;
                int axis = 0;
                if (abs_x < abs_y) {
                    widest = abs_y;
                    axis = 1;
                }
                if (widest < fabsf(plane->z)) {
                    axis = 2;
                }
                const srVector3i* poly_vertex = model->getPolyVertex() + polygon;
                const srVector3T<float>* vertices = model->getVertexLoc();
                srVector3T<float> triangle[3];
                triangle[0] = vertices[poly_vertex->x];
                triangle[1] = vertices[poly_vertex->y];
                triangle[2] = vertices[poly_vertex->z];
                if (PointInsideTriangle(triangle, axis, &point) && t < trace->hit_limit) {
                    trace->hit_limit = t;
                    hit = true;
                    contact = point;
                }
            }
        }
    }
    if (hit) {
        trace->end = contact;
    }
    return hit;
}

/* Collect the (mesh<<16)|polygon keys whose triangles touch the box
   (x±radius, y-height..y, z±radius) around `center`: every cell in the box is
   probed through ProbeCellForBlockersAppend, each key is then dropped when the
   mesh is gone, alpha-flagged or facing away, and when its triangle fails the
   spatial bounds test. The survivors are compacted, sorted for ordered
   consumption and zero-terminated; the shared buffer is returned. */
// FUNCTION: WIZ8 0x00438780
unsigned long* W8Octree::CollectPolygonsNearPoint(srVector3T<float>* center, float radius,
                                                  float height)
{
    m_gd_result_count = 0;
    m_visited_polygon_bits->ClearAll();
    m_current_regions->ClearAll();

    srVector3T<float> bounds[2];
    bounds[0].x = center->x - radius;
    bounds[0].z = center->z - radius;
    bounds[1].y = center->y;
    bounds[0].y = center->y - height;
    bounds[1].x = center->x + radius;
    bounds[1].z = center->z + radius;

    srVector3T<int> start;
    srVector3T<int> end;
    int axis;
    for (axis = 0; axis < 3; ++axis) {
        (&start.x)[axis] = static_cast<int>(
            ((&bounds[0].x)[axis] - (&m_spatial.m_minimum.x)[axis]) / m_spatial.m_node_extent);
        (&end.x)[axis] = static_cast<int>(((&bounds[1].x)[axis] - (&m_spatial.m_minimum.x)[axis]) /
                                          m_spatial.m_node_extent);
    }
    srVector3T<int> cell;
    for (cell.x = start.x; cell.x <= end.x; ++cell.x) {
        for (cell.y = start.y; cell.y <= end.y; ++cell.y) {
            for (cell.z = start.z; cell.z <= end.z; ++cell.z) {
                ProbeCellForBlockersAppend(&cell);
            }
        }
    }

    unsigned int index;
    for (index = 0; index < m_gd_result_count; ++index) {
        unsigned int mesh_index = m_aulGDObjs[index] >> 0x10;
        stMeshModel* model = 0;
        const srVector4T<float>* plane = 0;
        if (mesh_index < m_meshCount && g_world->psrMeshes[mesh_index] != 0 &&
            !m_pAlphaBits->Test(mesh_index)) {
            model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->getModel());
            unsigned int polygon = m_aulGDObjs[index] & 0xffff;
            plane = model->getPolyEq() + polygon;
            if (plane->y < g_float_half) {
                m_aulGDObjs[index] = 0;
            }
        } else {
            m_aulGDObjs[index] = 0;
        }
        if (m_aulGDObjs[index] != 0) {
            srVector3T<float> normal;
            normal = plane->xyz();
            const srVector3i* poly_vertex = model->getPolyVertex() + (m_aulGDObjs[index] & 0xffff);
            const srVector3T<float>* vertices = model->getVertexLoc();
            srVector3T<float> triangle[3];
            triangle[0] = vertices[poly_vertex->x];
            triangle[1] = vertices[poly_vertex->y];
            triangle[2] = vertices[poly_vertex->z];
            if (TestSpatialTriangle(bounds, triangle, &normal) == 0) {
                m_aulGDObjs[index] = 0;
            }
        }
    }

    unsigned int count = 0;
    for (index = 0; index < m_gd_result_count; ++index) {
        unsigned int key = m_aulGDObjs[index];
        if (key != 0) {
            m_aulGDObjs[count] = key;
            ++count;
        }
    }
    m_aulGDObjs[count] = 0;
    if (count != 0) {
        QuickSort(m_aulGDObjs, 0, count - 1);
    }
    return m_aulGDObjs;
}

/* A location the octree cannot resolve, or one whose submesh has no live model
   instance, clears the monster's cached mesh rather than leaving a stale one. */
// FUNCTION: WIZ8 0x0042e540
void W8Octree::UpdateMonsterLocation(unsigned short location_id, const srVector3T<float>* position)
{
    int queue_id = location_id + 1;
    unsigned int monster_list_index;
    W8MonsterInfo* info;
    W8Monster* monster;
    int sector;
    srModelInstance* mesh;
    srVector3T<int> point;

    if (location_id == 0) {
        return;
    }
    monster_list_index = MonsterGetIndexByLocationID(0x4c4, OCTREE_CPP, location_id, true);
    info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    if (info != 0 && info->p3D != 0) {
        monster = info->p3D;
        sector = GetSectorForPosition(position);
        if (sector == 0 || (mesh = g_world->psrMeshes[m_pSubmeshes[sector].mesh]) == 0) {
            monster->sector_mesh = 0;
        } else {
            monster->sector_mesh = mesh;
        }
    }
    WorldPositionToCell(position, &point);
    object_registry->RegisterObjectCell(W8_OCTREE_KIND_LOCATION, queue_id, &point);
}

/* Store `path` with its extension stripped into m_owned_0c0; the sibling data
   files (.PNT/.LVL/.WGD/.RLK) are then derived from the stem. The constructor
   inlines the same sequence; this out-of-line emission serves the
   preprocessed-file builder. */
// FUNCTION: WIZ8 0x0042cf90
bool W8Octree::SetPathStem(const char* path)
{
    size_t name_length = strlen(path);
    if (name_length == 0) {
        return false;
    }
    if (m_owned_0c0 != 0) {
        free(m_owned_0c0);
    }
    m_owned_0c0 = static_cast<char*>(malloc(name_length + 1));
    if (m_owned_0c0 == 0) {
        return false;
    }
    strcpy(m_owned_0c0, path);
    char* extension = strrchr(m_owned_0c0, '.');
    if (extension != 0 && extension - m_owned_0c0 > static_cast<int>(name_length) - 6) {
        *extension = '\0';
    }
    return true;
}

/* Read one .oct file into a fresh octree.

   The original name is the image's own: fourteen assertions in this body spell
   it ReadOctFile. The shape is one block repeated for every table the file
   carries - allocate, read, accumulate the byte count, and on either failure
   copy a message into the local buffer and stop advancing. fSuccess threading
   is what makes the failures cascade rather than each one returning; the flag
   at +0x000 bit 31 is the load error the caller tests.

   A null path builds an empty octree, and a level whose preprocessed files
   cannot be built runs on the LVL file alone with the same error bit set. */
// FUNCTION: WIZ8 0x0042bc10
W8Octree::W8Octree(const char* path, W8GameData** game_data)
{
    W8OctFileHeader header;
    char acMessage[256];
    int hOctFile;
    unsigned int uiRead;
    int uiTerminator;
    unsigned char fSuccess;
    unsigned char fLoaded;
    unsigned int index;
    unsigned int limit;
    W8GameData* pGameData = 0;
    int asset_status;

    Reset();
    if (path == 0) {
        Initialize(0);
        return;
    }
    asset_status = CheckLevelAssetSet(path);
    if (asset_status < 0) {
        goto failed;
    }
    if (asset_status > 0 && BuildPreprocessedFiles(path) == 0) {
        g_octree = 0;
        m_spatial.flags |= 0x80000000;
        ReportStartupMessage("Cannot find or build current preprocessed files.");
        ReportStartupMessage("Attempting to run with LVL file only -- SOME FEATURES DISABLED.");
        ReportStartupMessage(0);
        return;
    }
    hOctFile = FileOpen(const_cast<char*>(path), 1, 0);
    if (hOctFile == 0) {
        srAssertFail("hOctFile", OCTREE_CPP, 0xa3, "ReadOctFile: Couldn't open octree file.");
    }
    fSuccess = FileRead(hOctFile, &header, sizeof(header), &uiRead);
    g_octree_bytes_read += uiRead;
    fLoaded = 0;
    if (fSuccess != 0) {
        fSuccess = FileRead(hOctFile, &uiTerminator, 4, &uiRead);
        if (fSuccess == 0 || uiTerminator != -1) {
            srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0xac,
                         "ReadOctFile: Header of Oct file longer than expected.");
        }
        fLoaded = 0;
        if (fSuccess != 0) {
            Initialize(&header);
            SetPathStem(path);

            m_branches = static_cast<W8OctPreTreeBranch*>(
                malloc((header.m_branch_count + 2) * sizeof(W8OctPreTreeBranch)));
            if (m_branches == 0) {
                fSuccess = 0;
                strcpy(acMessage, "ReadOctFile: Couldn't allocate octree nodes.");
            } else {
                fSuccess = FileRead(hOctFile, m_branches,
                                    header.m_branch_count * sizeof(W8OctPreTreeBranch), &uiRead);
                if (fSuccess == 0) {
                    strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
                }
            }
            g_octree_bytes_read += uiRead;
            fLoaded = 0;
            if (fSuccess != 0) {
                m_leaves = static_cast<W8OctPreTreeLeaf*>(
                    malloc((header.m_leaf_count + 2) * sizeof(W8OctPreTreeLeaf)));
                if (m_leaves == 0) {
                    fSuccess = 0;
                    strcpy(acMessage, "ReadOctFile: Couldn't allocate octree leaves.");
                } else {
                    fSuccess = FileRead(hOctFile, m_leaves,
                                        header.m_leaf_count * sizeof(W8OctPreTreeLeaf), &uiRead);
                    if (fSuccess == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read octree leaves.");
                    }
                }
                g_octree_bytes_read += uiRead;
                fLoaded = 0;
                if (fSuccess != 0) {
                    m_polygon_index_stream = static_cast<unsigned long*>(
                        malloc(header.m_leaf_polygon_stream_len * 4 + 8));
                    if (m_polygon_index_stream == 0) {
                        fLoaded = 0;
                        strcpy(acMessage,
                               "ReadOctFile: Couldn't allocate polygon index list for leaves.");
                    } else {
                        fLoaded = FileRead(hOctFile, m_polygon_index_stream,
                                           header.m_leaf_polygon_stream_len * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage,
                                   "ReadOctFile: Couldn't read polygon index list for leaves.");
                        }
                    }
                    g_octree_bytes_read += uiRead;
                }
            }
        }
    }

    limit = m_leaf_grid_dimensions.x * m_leaf_grid_dimensions.y * m_leaf_grid_dimensions.z;
    if (fLoaded != 0 && limit < 250000) {
        m_leaf_lookup = static_cast<unsigned long*>(malloc(limit * sizeof(unsigned long)));
        if (m_leaf_lookup == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Leaf grid.");
            goto finish;
        }
        fLoaded = FileRead(hOctFile, m_leaf_lookup,
                           m_leaf_grid_dimensions.x * m_leaf_grid_dimensions.y *
                               m_leaf_grid_dimensions.z * 4,
                           &uiRead);
        if (fLoaded == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't read leaf grid.");
        }
        g_octree_bytes_read += uiRead;
    } else {
        m_leaf_lookup = 0;
    }

    fSuccess = 0;
    if (fLoaded != 0) {
        m_aulPolyLookup = static_cast<unsigned long*>(malloc(header.m_polygon_count * 4 + 8));
        if (m_aulPolyLookup == 0) {
            fSuccess = 0;
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Poly Lookup table.");
        } else {
            fLoaded = FileRead(hOctFile, m_aulPolyLookup, header.m_polygon_count * 4, &uiRead);
            if (fLoaded == 0) {
                strcpy(acMessage, "ReadOctFile: Couldn't read Poly Lookup table.");
            }
            g_octree_bytes_read += uiRead;
            fSuccess = 0;
            if (fLoaded != 0) {
                if (header.m_region_list_len != 0) {
                    m_region_index_stream =
                        static_cast<unsigned short*>(malloc(header.m_region_list_len * 2 + 4));
                    if (m_region_index_stream == 0) {
                        fSuccess = 0;
                        strcpy(acMessage, "ReadOctFile: Couldn't allocate region list.");
                        goto finish;
                    }
                    fLoaded = FileRead(hOctFile, m_region_index_stream,
                                       header.m_region_list_len * 2, &uiRead);
                    if (fLoaded == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read region list.");
                    }
                    g_octree_bytes_read += uiRead;
                }
                fSuccess = 0;
                if (fLoaded != 0) {
                    if (header.m_gd_surface_stream_len != 0) {
                        m_gd_surface_index_stream = static_cast<unsigned long*>(
                            malloc(header.m_gd_surface_stream_len * 4 + 8));
                        if (m_gd_surface_index_stream == 0) {
                            fSuccess = 0;
                            strcpy(acMessage, "ReadOctFile: Couldn't allocate GD Poly list.");
                            goto finish;
                        }
                        fLoaded = FileRead(hOctFile, m_gd_surface_index_stream,
                                           header.m_gd_surface_stream_len * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage, "ReadOctFile: Couldn't read GD Poly list.");
                        }
                        g_octree_bytes_read += uiRead;
                    }
                    fSuccess = 0;
                    if (fLoaded != 0) {
                        if (header.m_trigger_count != 0) {
                            m_trigger_indices = static_cast<unsigned short*>(
                                malloc(header.m_trigger_count * 2 + 4));
                            if (m_trigger_indices == 0) {
                                fSuccess = 0;
                                strcpy(acMessage, "ReadOctFile: Couldn't allocate Trigger list.");
                                goto finish;
                            }
                            fLoaded = FileRead(hOctFile, m_trigger_indices,
                                               header.m_trigger_count * 2, &uiRead);
                            if (fLoaded == 0) {
                                strcpy(acMessage, "ReadOctFile: Couldn't read Trigger list.");
                            }
                            g_octree_bytes_read += uiRead;
                        }
                        fSuccess = 0;
                        if (fLoaded != 0) {
                            if (header.m_region_count > 1) {
                                m_spatial.m_region_volumes = static_cast<W8OctRegionVolume*>(malloc(
                                    (header.m_region_count + 2) * sizeof(W8OctRegionVolume)));
                                if (m_spatial.m_region_volumes == 0) {
                                    fSuccess = 0;
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't allocate region array.");
                                    goto finish;
                                }
                                fLoaded = FileRead(
                                    hOctFile, m_spatial.m_region_volumes,
                                    header.m_region_count * sizeof(W8OctRegionVolume), &uiRead);
                                if (fLoaded == 0) {
                                    strcpy(acMessage, "ReadOctFile: Couldn't read region array.");
                                }
                                g_octree_bytes_read += uiRead;
                            }
                            fSuccess = 0;
                            if (fLoaded != 0) {
                                fLoaded = FileRead(hOctFile, &uiTerminator, 4, &uiRead);
                                if (fLoaded == 0 || uiTerminator != -1) {
                                    srAssertFail("fSuccess && (uiTerminator==0xffffffff)",
                                                 OCTREE_CPP, 0x15f,
                                                 "ReadOctFile: PreRegions longer than expected.");
                                }
                                fSuccess = 0;
                                if (fLoaded != 0) {
                                    if (header.m_submesh_count != 0) {
                                        m_pSubmeshes = static_cast<W8OctSubmesh*>(malloc(
                                            (header.m_submesh_count + 1) * sizeof(W8OctSubmesh)));
                                        if (m_pSubmeshes == 0) {
                                            fLoaded = 0;
                                            strcpy(acMessage,
                                                   "ReadOctFile: Couldn't allocate submesh array.");
                                        } else {
                                            fLoaded = FileRead(hOctFile, m_pSubmeshes,
                                                               (header.m_submesh_count + 1) *
                                                                   sizeof(W8OctSubmesh),
                                                               &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read submesh array.");
                                            }
                                            g_octree_bytes_read += uiRead;
                                            if (fLoaded != 0) {
                                                unsigned int* scan;
                                                unsigned int remaining;

                                                limit = 0;
                                                scan = &m_pSubmeshes[0].polygon_count;
                                                remaining = header.m_submesh_count + 1;
                                                do {
                                                    if (limit < *scan) {
                                                        limit = *scan;
                                                    }
                                                    scan += 4;
                                                    --remaining;
                                                } while (remaining != 0);
                                                ++limit;
                                                if (limit > 9999) {
                                                    srAssertFail("(i2 < 10000)", OCTREE_CPP, 0x179,
                                                                 0);
                                                }
                                                g_octree_storage_ = static_cast<unsigned int*>(
                                                    malloc(limit * sizeof(unsigned int)));
                                                if (g_octree_storage_ == 0) {
                                                    fLoaded = 0;
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't allocate polygon "
                                                           "index list for regions.");
                                                } else {
                                                    for (index = 0; index < limit; ++index) {
                                                        g_octree_storage_[index] = index;
                                                    }
                                                }
                                            }
                                        }
                                        if (m_meshCount != 0) {
                                            m_pAlphaBits = new BitArray(m_meshCount);
                                            if (m_pAlphaBits == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits", OCTREE_CPP, 0x18a,
                                                    "ReadOctFile: Failure allocating Alpha Bits.");
                                            }
                                            if (m_pAlphaBits->Load(hOctFile) == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits->Load(hOctFile)", OCTREE_CPP,
                                                    0x18b,
                                                    "ReadOctFile: Failure reading Alpha Bits.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumParticles != 0) {
                                            m_pusMeshParticleLookup = static_cast<unsigned short*>(
                                                malloc(m_meshCount * 2 + 2));
                                            if (m_pusMeshParticleLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticleLookup", OCTREE_CPP, 0x191,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshParticleLookup,
                                                               m_meshCount * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage, "ReadOctFile: Couldn't read Mesh "
                                                                  "Particle Lookup Table.");
                                            }
                                            g_octree_bytes_read += uiRead;
                                            m_pusMeshParticles = static_cast<unsigned short*>(
                                                malloc(m_usMeshParticlesLen * 2));
                                            if (m_pusMeshParticles == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticles", OCTREE_CPP, 0x198,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Link Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshParticles,
                                                               m_usMeshParticlesLen * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage, "ReadOctFile: Couldn't read Mesh "
                                                                  "Particle Link Table.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumProps != 0) {
                                            m_pusMeshPropLookup = static_cast<unsigned short*>(
                                                malloc(m_meshCount * 2 + 2));
                                            if (m_pusMeshPropLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshPropLookup", OCTREE_CPP, 0x1a1,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshPropLookup,
                                                               m_meshCount * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage, "ReadOctFile: Couldn't read Mesh "
                                                                  "Prop Lookup Table.");
                                            }
                                            g_octree_bytes_read += uiRead;
                                            m_pusMeshProps = static_cast<unsigned short*>(
                                                malloc(m_usMeshPropsLen * 2));
                                            if (m_pusMeshProps == 0) {
                                                srAssertFail(
                                                    "m_pusMeshProps", OCTREE_CPP, 0x1a8,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop Link "
                                                    "Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshProps,
                                                               m_usMeshPropsLen * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage, "ReadOctFile: Couldn't read Mesh "
                                                                  "Prop Link Table.");
                                            }
                                        }
                                    }
                                    fSuccess = 0;
                                    if (fLoaded != 0) {
                                        fLoaded = FileRead(hOctFile, &uiTerminator, 4, &uiRead);
                                        if (fLoaded == 0 || uiTerminator != -1) {
                                            srAssertFail(
                                                "fSuccess && (uiTerminator==0xffffffff)",
                                                OCTREE_CPP, 0x1b2,
                                                "ReadOctFile: Mesh, Prop, and Particle data longer "
                                                "than expected.");
                                        }
                                        fSuccess = 0;
                                        if (fLoaded != 0) {
                                            if (header.m_path_nodes != 0) {
                                                pathing = new W8PathingService();
                                                if (pathing == 0) {
                                                    goto finish;
                                                }
                                                pathing->ConfigureForLevel(
                                                    header.m_path_nodes, header.m_region_cell,
                                                    header.m_path_clearance, header.m_bounds,
                                                    m_owned_0c0);
                                                fLoaded = pathing->ReadPathNodes(hOctFile);
                                            }
                                            fSuccess = 0;
                                            if (fLoaded != 0) {
                                                if (m_ulNumProps != 0 &&
                                                    header.m_prop_sun_bits != 0) {
                                                    m_pPropSunBits = new BitArray(m_ulNumProps);
                                                    if (m_pPropSunBits == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits", OCTREE_CPP, 0x1c5,
                                                            "ReadOctFile: Couldn't allocate Prop "
                                                            "Sun Bits.");
                                                    }
                                                    if (m_pPropSunBits->Load(hOctFile) == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits->Load(hOctFile)",
                                                            OCTREE_CPP, 0x1c6,
                                                            "ReadOctFile: Failure reading Prop Sun "
                                                            "Bits.");
                                                    }
                                                }
                                                fSuccess =
                                                    FileRead(hOctFile, &uiTerminator, 4, &uiRead);
                                                if (fSuccess == 0) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't read octree file "
                                                           "terminator.");
                                                }
                                                if (uiTerminator != -1) {
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Octree file longer than "
                                                           "expected.");
                                                    fSuccess = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

finish:
    SetOctreeGameData(0);
    *game_data = 0;
    if (fSuccess != 0 && header.m_gd_surface_stream_len != 0) {
        pGameData = new W8GameData(hOctFile, false);
        if (pGameData == 0) {
            strcpy(acMessage, "ReadOctFile: Octree file longer than expected.");
            fSuccess = 0;
        } else {
            fSuccess = FileRead(hOctFile, &uiTerminator, 4, &uiRead);
            if (fSuccess == 0 || uiTerminator != -1) {
                srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0x1e2,
                             "ReadOctFile: GameData portion of Oct file longer than expected.");
            }
        }
    }
    FileClose(hOctFile);
    if (fSuccess != 0) {
        g_octree = this;
        pGameData->octree = this;
        *game_data = pGameData;
        SetOctreeGameData(pGameData);
        m_region_links_ready = ReadRegionLinkFile(m_owned_0c0);
        LoadPointFiles(m_owned_0c0);
        if (pathing != 0) {
            pathing->ReadWaypointFile();
        }
        return;
    }
failed:
    g_octree = 0;
    m_spatial.flags |= 0x80000000;
}

/* Answer whether this level's OCT/PVL/WGD/LVL set is present, current, and
   newer than the raw sources. Negative means skip the octree, zero means the
   set is ready, and positive means the preprocessed files should be rebuilt. */
// FUNCTION: WIZ8 0x0042ccc0
int CheckLevelAssetSet(const char* level_path)
{
    unsigned short version;
    char copy[260];
    char pvl_path[260];
    char wgd_path[260];
    char lvl_path[260];
    int rebuild = 0;
    int file;
    char* extension;

    if (g_octree_disabled) {
        return -1;
    }

    strcpy(copy, level_path);
    TrimAndLowercaseString(copy);
    if (strstr(copy, "sky") != 0) {
        return -1;
    }
    if (FileExists("CD.ROM") != 0) {
        return 0;
    }

    strcpy(copy, level_path);
    extension = strrchr(copy, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    sprintf(pvl_path, "%s.PVL", copy);
    if (FileExists(const_cast<char*>(level_path)) != 0 && FileExists(pvl_path) != 0 &&
        (file = FileOpen(const_cast<char*>(level_path), 1, 0)) != 0 &&
        FileRead(file, &version, 2, 0) != 0) {
        if (version < W8OctFileHeader::VERSION) {
            if (g_octree_disabled) {
                return -1;
            }
            rebuild = 1;
            FileClose(file);
        } else {
            if (version > W8OctFileHeader::VERSION) {
                FileClose(file);
                ShutdownWithErrorBox(
                    "EXE OUT OF DATE: Program is older than File version--There is a new "
                    "executable available.");
            }
            FileClose(file);
        }
    } else {
        rebuild = 1;
    }

    sprintf(lvl_path, "%s.LVL", copy);
    if (FileExists(lvl_path) == 0) {
        return -rebuild;
    }
    sprintf(wgd_path, "%s.WGD", copy);
    if (FileExists(wgd_path) == 0) {
        ReportStartupMessage(
            "Could not find WGD file. Cannot find or build current preprocessed files.");
        ReportStartupMessage("Attempting to run with LVL file only -- NO COLLISION DATA.");
        ReportStartupMessage(0);
        return -rebuild;
    }

    if (FileIsOlderThanFile(const_cast<char*>(level_path), pvl_path, 0) == 0) {
        if (FileIsOlderThanFile(pvl_path, wgd_path, 10) == 0 &&
            FileIsOlderThanFile(pvl_path, lvl_path, 10) == 0) {
            return rebuild;
        }
    } else if (FileIsOlderThanFile(const_cast<char*>(level_path), wgd_path, 10) == 0) {
        if (FileIsOlderThanFile(const_cast<char*>(level_path), lvl_path, 10) == 0) {
            return rebuild;
        }
        return 1;
    }
    return 1;
}

/* Reset the owned tables and query state before applying an OCT header.
   Retail leaves the region masks and positional flag untouched here. */
// FUNCTION: WIZ8 0x0042d040
void W8Octree::Reset()
{
    g_octree_storage_ = 0;
    g_octree_state = 0;
    m_spatial.Reset();
    m_branches = 0;
    m_leaves = 0;
    m_leaf_grid_dimensions.z = 0;
    m_leaf_grid_dimensions.y = 0;
    m_leaf_grid_dimensions.x = 0;
    m_leaf_lookup = 0;
    m_branch_count = 0;
    m_leaf_count = 0;
    m_vertex_count = 0;
    m_leaf_polygon_stream_len = 0;
    m_gd_surface_stream_len = 0;
    m_trigger_count = 0;
    m_region_list_len = 0;
    m_unknown_13c = 0;
    m_polygon_index_stream = 0;
    m_aulPolyLookup = 0;
    m_gd_surface_index_stream = 0;
    m_region_index_stream = 0;
    m_trigger_indices = 0;
    m_pfRegsVisited = 0;
    m_pSubmeshes = 0;
    object_registry = 0;
    m_pRegionLinks = 0;
    m_visited_polygon_bits = 0;
    m_owned_154 = 0;
    m_projected_regions = 0;
    m_current_regions = 0;
    m_previous_regions = 0;
    m_point_count = 0;
    m_sample_points = 0;
    m_owned_0c0 = 0;
    m_region_cell = 0;
    m_path_clearance = 0;
    pathing = 0;
    m_fAccumulating = false;
    m_pusMeshParticleLookup = 0;
    m_pusMeshParticles = 0;
    m_usMeshParticlesLen = 0;
    m_pusMeshPropLookup = 0;
    m_pusMeshProps = 0;
    m_usMeshPropsLen = 0;
    m_ulNumParticles = 0;
    m_linked_particles = 0;
    m_visible_particles = 0;
    m_linked_props = 0;
    m_visible_props = 0;
    m_particles_to_disable = 0;
    m_props_to_disable = 0;
    m_papProps = 0;
    m_papParticles = 0;
    m_usNumPropsLoaded = 0;
    m_usNumParticlesLoaded = 0;
    m_trace_skip_flag = 0;
    m_visited_object_bits = 0;
    m_accumulated_regions = 0;
    m_owned_19c = 0;
    m_owned_1a0 = 0;
    m_owned_1a4 = 0;
    m_root_mesh_count = 0;
    m_kind1_submesh_count = 0;
    m_alpha_polygon_count = 0;
    m_meshCount = 0;
    m_pAlphaBits = 0;
    m_gd_result_count = 0;
    m_aulGDObjs = 0;

    memset(&view, 0, sizeof(view));
    /* 0x0042D1F1 then re-zeroes the four bounds inline, which
       Reset above already did through the callee. */
    m_spatial.m_clipped_minimum.SetZero();
    m_spatial.m_clipped_maximum.SetZero();
    m_spatial.m_working_minimum.SetZero();
    m_spatial.m_working_maximum.SetZero();
    m_unknown_27c[0] = 0;
    m_unknown_27c[1] = 0;
    m_unknown_27c[2] = 0;
    m_unknown_27c[3] = 0;
    m_unknown_27c[4] = 0;
    m_unknown_27c[5] = 0;
    m_visibility_suspended = false;
    m_reset_visibility = false;
    m_projected_regions_valid = false;
    m_region_links_ready = false;
    m_region_links_dirty = false;
    m_points_dirty = false;
    m_unknown_299 = 0;
    m_sun_count = 0;
    prop_sun_base = 0;
    m_pPropSunBits = 0;
    m_ulNumProps = 0;
    current_prop = -1;
}

/* ReadOctFile calls this after Reset. The 0xf5-byte packed record has one
   canonical source model shared with WriteOctFile. */
// FUNCTION: WIZ8 0x0042d2a0
void W8Octree::Initialize(const W8OctFileHeader* header)
{
    unsigned short level;

    if (header != 0) {
        m_spatial.m_extent = header->m_extent;
        m_spatial.m_cell_size = header->m_cell_size;
        m_spatial.m_node_extent = header->m_node_extent;

        m_unknown_27c[0] = 0;
        m_unknown_27c[1] = 0;
        m_unknown_27c[2] = 0;
        m_unknown_27c[3] = 0;
        m_unknown_27c[4] = 0;
        m_unknown_27c[5] = 0;
        m_spatial.m_minimum = header->m_bounds[0];
        m_spatial.m_maximum = header->m_bounds[1];
        m_spatial.m_clipped_minimum = header->m_bounds[2];
        m_spatial.m_clipped_maximum = header->m_bounds[3];
        m_spatial.m_working_minimum = header->m_bounds[4];
        m_spatial.m_working_maximum = header->m_bounds[5];
        m_leaf_grid_dimensions = header->m_grid_dims;

        m_spatial.m_leaf_grid_stride_x = m_leaf_grid_dimensions.y * m_leaf_grid_dimensions.z;
        m_spatial.m_leaf_grid_stride_y = m_leaf_grid_dimensions.z;
        m_spatial.m_depth = header->m_depth;
        m_spatial.m_region_id_bound = header->m_region_id_bound;
        m_spatial.m_region_count = header->m_region_count;
        m_spatial.m_leaf_level = header->m_leaf_level;
        m_spatial.submesh_count = header->m_submesh_count;
        m_root_mesh_count = header->m_root_mesh_count;
        m_kind1_submesh_count = header->m_kind1_submesh_count;
        m_meshCount = header->m_mesh_total;
        m_region_mask = 0;
        m_spatial.m_region_cells_per_axis = 1;
        for (level = 0; level < m_spatial.m_leaf_level; ++level) {
            m_region_mask = m_region_mask * 2 + 1;
            m_spatial.m_region_cells_per_axis <<= 1;
        }
        m_depth_mask = 0;
        for (level = 0; level < m_spatial.m_depth; ++level) {
            m_depth_mask = m_depth_mask * 2 + 1;
        }
        m_branch_count = header->m_branch_count;
        m_leaf_count = header->m_leaf_count;
        m_spatial.m_polygon_count = header->m_polygon_count;
        m_vertex_count = header->m_vertex_count;
        m_spatial.m_item_count = header->m_surface_count;
        m_leaf_polygon_stream_len = header->m_leaf_polygon_stream_len;
        m_gd_surface_stream_len = header->m_gd_surface_stream_len;
        m_trigger_count = header->m_trigger_count;
        m_region_list_len = header->m_region_list_len;
        m_spatial.m_region_grid_cell = header->m_region_grid_cell;
        m_region_cell = header->m_region_cell;
        m_path_clearance = header->m_path_clearance;
        m_spatial.m_max_region_radius = header->m_max_region_radius;
        m_ulNumProps = header->m_prop_count;
        m_usMeshParticlesLen = header->m_particle_len;
        m_usMeshPropsLen = header->m_prop_len;
        m_ulNumParticles = header->m_particle_count;

        if (m_ulNumParticles != 0) {
            m_linked_particles = new BitArray(m_ulNumParticles);
            m_visible_particles = new BitArray(m_ulNumParticles);
            m_particles_to_disable = new BitArray(m_ulNumParticles);
            m_papParticles = static_cast<stParticle**>(malloc(m_ulNumParticles * 4 + 8));
        }
        if (m_ulNumProps != 0) {
            m_linked_props = new BitArray(m_ulNumProps);
            m_visible_props = new BitArray(m_ulNumProps);
            m_props_to_disable = new BitArray(m_ulNumProps);
            m_papProps = static_cast<W8Prop**>(malloc(m_ulNumProps * 4 + 8));
        }

        m_visited_polygon_bits = new BitArray(header->m_polygon_count);
        m_owned_154 = new BitArray(header->m_region_id_bound + 1);
        m_projected_regions = new BitArray(header->m_submesh_count + 1);
        m_current_regions = new BitArray(header->m_submesh_count + 1);
        m_previous_regions = new BitArray(header->m_submesh_count + 1);
        m_visited_object_bits =
            new BitArray(header->m_surface_count < 5000 ? 5000 : header->m_surface_count);
        m_accumulated_regions = new BitArray(header->m_submesh_count + 1);
        m_owned_19c = new BitArray(header->m_polygon_count);

        object_registry = new W8OctreeObjectRegistry;
        m_pRegionLinks = new W8HashTable<unsigned int, unsigned short>;
        m_pRegionLinks->Clear();

        unsigned int visited_size = header->m_region_id_bound + 1;
        m_pfRegsVisited = static_cast<unsigned char*>(malloc(visited_size));
        if (m_pfRegsVisited == 0) {
            srAssertFail("m_pfRegsVisited", "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                         0x36a, "InitOctree: Couldn't allocate m_pfRegsVisited.");
        }
        memset(m_pfRegsVisited, 0, visited_size);
        m_reset_visibility = true;
    }

    m_aulGDObjs = static_cast<unsigned long*>(malloc(40000));
    if (m_aulGDObjs == 0) {
        srAssertFail("m_aulGDObjs", "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", 0x372,
                     "InitOctree: Couldn't allocate m_aulGDObjs.");
    }
    m_spatial.m_level_kind = 3;
    m_fAccumulating = true;
    ToggleUpdateSuspension(0);
}

// FUNCTION: WIZ8 0x0042e440
void W8Octree::AddLoadedProp(W8Prop* prop)
{
    if (m_fAccumulating) {
        if (m_usNumPropsLoaded >= static_cast<unsigned short>(m_ulNumProps)) {
            srAssertFail("m_usNumPropsLoaded<(UINT16)m_ulNumProps",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", 0x485,
                         "Too many props loaded for Octree");
        }
        m_papProps[m_usNumPropsLoaded] = prop;
        m_usNumPropsLoaded++;
        m_papProps[m_usNumPropsLoaded] = 0;
    }
}

// FUNCTION: WIZ8 0x0042e4c0
void W8Octree::AddLoadedParticle(stParticle* particle)
{
    if (m_fAccumulating) {
        if (m_usNumParticlesLoaded >= static_cast<unsigned short>(m_ulNumParticles)) {
            srAssertFail("m_usNumParticlesLoaded<(UINT16)m_ulNumParticles",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", 0x49d,
                         "Too many particles loaded for Octree");
        }
        m_papParticles[m_usNumParticlesLoaded] = particle;
        m_usNumParticlesLoaded++;
        m_papParticles[m_usNumParticlesLoaded] = 0;
    }
}

/* Complete non-deleting teardown. LoadWorld and DestroyWorld both perform the
   matching operator delete only after this method returns. */
// FUNCTION: WIZ8 0x0042de60
W8Octree::~W8Octree()
{
    if (m_region_links_dirty) {
        SaveRegionLinks(m_owned_0c0);
    }
    if (m_points_dirty) {
        SavePoints(m_owned_0c0);
    }
    if (pathing != 0) {
        pathing->SaveWaypointSnapshot(false);
    }

    free(m_aulGDObjs);
    if (m_branches != 0) {
        free(m_branches);
    }
    if (m_leaves != 0) {
        free(m_leaves);
    }
    if (m_polygon_index_stream != 0) {
        free(m_polygon_index_stream);
    }
    if (m_aulPolyLookup != 0) {
        free(m_aulPolyLookup);
    }
    delete m_pAlphaBits;
    if (m_leaf_lookup != 0) {
        free(m_leaf_lookup);
    }
    if (m_gd_surface_index_stream != 0) {
        free(m_gd_surface_index_stream);
    }
    if (m_region_index_stream != 0) {
        free(m_region_index_stream);
    }
    if (m_trigger_indices != 0) {
        free(m_trigger_indices);
    }

    if (m_spatial.m_region_volumes != 0) {
        free(m_spatial.m_region_volumes);
    }

    if (m_pRegionLinks != 0) {
        m_pRegionLinks->Clear();
        delete m_pRegionLinks;
        m_pRegionLinks = 0;
    }

    if (m_pfRegsVisited != 0) {
        free(m_pfRegsVisited);
    }
    delete m_visited_polygon_bits;
    delete m_owned_154;
    delete m_projected_regions;
    delete m_current_regions;
    delete m_previous_regions;

    if (m_pusMeshParticleLookup != 0) {
        free(m_pusMeshParticleLookup);
    }
    m_pusMeshParticleLookup = 0;
    if (m_pusMeshParticles != 0) {
        free(m_pusMeshParticles);
    }
    m_pusMeshParticles = 0;
    if (m_pusMeshPropLookup != 0) {
        free(m_pusMeshPropLookup);
    }
    m_pusMeshPropLookup = 0;
    if (m_pusMeshProps != 0) {
        free(m_pusMeshProps);
    }
    m_pusMeshProps = 0;
    if (m_papProps != 0) {
        free(m_papProps);
    }
    if (m_papParticles != 0) {
        free(m_papParticles);
    }

    delete m_linked_particles;
    delete m_visible_particles;
    delete m_linked_props;
    delete m_visible_props;
    delete m_particles_to_disable;
    delete m_props_to_disable;

    m_point_count = 0;
    if (m_sample_points != 0) {
        delete[] m_sample_points;
    }
    if (m_owned_0c0 != 0) {
        free(m_owned_0c0);
    }
    delete m_visited_object_bits;
    delete m_accumulated_regions;
    delete m_owned_19c;
    delete m_owned_1a0;
    delete m_owned_1a4;

    delete object_registry;

    m_visited_polygon_bits = 0;
    m_owned_154 = 0;
    m_projected_regions = 0;
    m_visited_object_bits = 0;
    m_accumulated_regions = 0;
    m_owned_19c = 0;
    m_owned_1a0 = 0;
    m_owned_1a4 = 0;

    if (m_pSubmeshes != 0) {
        free(m_pSubmeshes);
    }
    if (g_octree_storage_ != 0) {
        free(g_octree_storage_);
    }
    delete pathing;
    pathing = 0;
    if (g_octree_trace_node != 0) {
        delete g_octree_trace_node;
        g_octree_trace_node = 0;
    }
    g_octree_storage_ = 0;
    g_octree = 0;
    srMaterialIFace* default_material = g_oct_mesh_default_material;
    delete default_material;
    g_oct_mesh_default_material = 0;
    g_oct_mesh_default_texture = 0;
    delete g_oct_mesh_default_shader;
    g_oct_mesh_default_shader = 0;
    delete m_pPropSunBits;
    m_pPropSunBits = 0;
}

/* Running index into the prop-sunlight bit stream, shared across the octree
   so consecutive levels pick up where the previous one's props ended. */

// GLOBAL: WIZ8 0x006598ac
int g_prop_sun_index;

/* Attach the prop-sunlight bit array, but only one that has been sized. */
// FUNCTION: WIZ8 0x0042e3e0
void W8Octree::SetPropSunBits(BitArray* bits)
{
    if (bits->bit_count != 0) {
        m_pPropSunBits = bits;
    }
}

/* Test whether prop `offset` past the octree's base index has its sunlight
   bit set; the tested index also becomes the shared running index. A
   negative offset instead checkpoints the shared index into the base, which
   is how ReadWorldProps starts each level's props on a fresh range. With no
   array attached the answer is always no. */
// FUNCTION: WIZ8 0x0042e400
bool W8Octree::TestPropSunBit(int offset)
{
    if (offset < 0) {
        prop_sun_base = g_prop_sun_index;
        return false;
    }
    g_prop_sun_index = prop_sun_base + offset;
    if (m_pPropSunBits != 0 && m_pPropSunBits->Test(g_prop_sun_index)) {
        return true;
    }
    return false;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8Octree::VisitPointCopy(unsigned short location_id, srVector3T<float>* position)
{
    srVector3T<float> copy;

    copy = *position;
    UpdateMonsterLocation(location_id, &copy);
}

/* Remove every kind-12 and kind-13 registry pairing owned by one location
   id; each by_object hit also drops the mirrored by_cell entry.
   MonsterManager/NPC Manager drive this during location teardown.
   Retail verified at 0x0042E650/0x00438D50: the just-removed slot is passed
   back as the FindNextEntry cursor even though RemoveAt has pushed that
   entry onto the free list; the walk then follows the overwritten
   next_index, which is the free head (-1 in practice), ending the loop. */
// FUNCTION: WIZ8 0x0042e650
void W8Octree::UnregisterLocationObjects(unsigned int location_id)
{
    UnregisterLocationObject(location_id, W8_OCTREE_KIND_LOCATION);
    UnregisterLocationObject(location_id, W8_OCTREE_KIND_NAVIGATOR);
}

/* Remove the registered pairings for one (location, kind) object key — used
   by Navigator teardown with kind 0xd.  The removed-slot cursor reuse is
   retail verified (the same sequence is inlined at 0x0042E880): FindNextEntry
   follows entries[slot].next_index after RemoveAt has overwritten it with
   the free-list head. */
// FUNCTION: WIZ8 0x0042e880
void W8Octree::UnregisterLocationObject(unsigned int location_id, W8OctreeObjectKind kind)
{
    object_registry->UnregisterObject(kind, location_id + 1);
}

/* Collect object ids of `kind` from every cell under the `origin`-swept
   `delta` segment grown by `extent`; the extent takes the segment length as a
   floor. `*results` carries the destination buffer in and out. */
// FUNCTION: WIZ8 0x0042ed60
int W8Octree::CollectObjectsAlongSegment(unsigned long** results, const srVector3T<float>* origin,
                                         const srVector3T<float>* delta, float extent,
                                         unsigned short kind)
{
    srVector3T<float> low;
    srVector3T<float> high;
    srVector3T<int> low_cell;
    srVector3T<int> high_cell;
    srVector3T<int> cell;

    g_octree_state = *results;
    if (g_octree_state == 0) {
        g_octree_state = m_aulGDObjs;
        *results = g_octree_state;
    }
    m_gd_result_count = 0;
    float radius = delta->Length();
    if (extent < radius) {
        extent = radius;
    }
    for (int axis = 0; axis < 3; ++axis) {
        float bound = (&origin->x)[axis] - extent;
        if ((&delta->x)[axis] <= g_float_zero) {
            (&low.x)[axis] = bound + (&delta->x)[axis];
            bound = extent + (&origin->x)[axis];
        } else {
            (&low.x)[axis] = bound;
            bound = extent + (&origin->x)[axis] + (&delta->x)[axis];
        }
        (&high.x)[axis] = bound;
    }
    m_visited_object_bits->ClearAll();
    CollectVisibleRegions(&low, &low_cell, 0, 0);
    CollectVisibleRegions(&high, &high_cell, 0, 0);
    if (low_cell.x <= high_cell.x) {
        cell.x = low_cell.x;
        do {
            if (cell.x >= 0 && cell.x < static_cast<int>(m_leaf_grid_dimensions.x) /* c-style-cast-ok:
                    cell coordinate vs grid dimension */) {
                cell.y = low_cell.y;
                while (cell.y <= high_cell.y) {
                    if (cell.y >= 0 && cell.y < static_cast<int>(m_leaf_grid_dimensions.y) /* c-style-cast-ok:
                            cell coordinate vs grid dimension */) {
                        for (cell.z = low_cell.z; cell.z <= high_cell.z; ++cell.z) {
                            if (cell.z >= 0 && cell.z < static_cast<int>(m_leaf_grid_dimensions.z)
                                /* c-style-cast-ok: cell coordinate vs grid dimension */) {
                                CollectObjectsInCell(&cell, kind);
                            }
                        }
                    }
                    ++cell.y;
                }
            }
            ++cell.x;
        } while (cell.x <= high_cell.x);
    }
    g_octree_state = 0;
    return static_cast<int>(m_gd_result_count); /* c-style-cast-ok: the shared count field is
        stored unsigned */
}

/* The kind-12 convenience query: a zero exclusion maps to -1 (none). OctPath
   uses it to list the location ids near a mover. */

// FUNCTION: WIZ8 0x0042ef00
int W8Octree::QueryLocationsInBox(unsigned long** results, const srVector3T<float>* lower,
                                  const srVector3T<float>* upper, unsigned short exclusion)
{
    int excluded = -1;
    if (exclusion != 0) {
        excluded = exclusion;
    }
    return QueryObjects(results, lower, upper, W8_OCTREE_KIND_LOCATION, excluded);
}

/* AABB occupancy test: GD triangles, kind-12 location objects (each
   monster's navigator radius inflates the box) and collidable-prop surfaces
   all answer "occupied". */
// FUNCTION: WIZ8 0x0042ef30
unsigned char W8Octree::TestBoxOccupied(const srVector3T<float>* lower,
                                        const srVector3T<float>* upper)
{
    unsigned long* objects = 0;
    unsigned int count =
        static_cast<unsigned int>(QueryObjects(&objects, lower, upper, W8_OCTREE_KIND_SURFACE, -1));
    unsigned int index;
    for (index = 0; index < count; ++index) {
        W8GDSurface* surface = g_octree_game_data->m_pSurfaces + objects[index];
        srVector3T<float> bounds[2];
        srVector3T<float> triangle[3];
        bounds[0] = *lower;
        bounds[1] = *upper;
        triangle[0] = g_octree_game_data->m_pVertices[surface->vertex_indices[0]];
        triangle[1] = g_octree_game_data->m_pVertices[surface->vertex_indices[1]];
        triangle[2] = g_octree_game_data->m_pVertices[surface->vertex_indices[2]];
        if (TestSpatialTriangle(bounds, triangle, surface->Normal()) != 0) {
            return 1;
        }
    }
    count = static_cast<unsigned int>(
        QueryObjects(&objects, lower, upper, W8_OCTREE_KIND_LOCATION, -1));
    for (index = 0; index < count; ++index) {
        unsigned int monster_index = MonsterGetIndexByLocationID(
            0x62d, "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", objects[index], true);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info != 0 && info->p3D != 0) {
            srVector3T<float> position = info->p3D->GetPosition();
            float radius = info->p3D->radius;
            if (lower->x - radius < position.x && position.x < radius + upper->x &&
                lower->y - radius < position.y && position.y < radius + upper->y &&
                lower->z - radius < position.z && position.z < radius + upper->z) {
                return 1;
            }
        }
    }
    count =
        static_cast<unsigned int>(QueryObjects(&objects, lower, upper, W8_OCTREE_KIND_PROP, -1));
    for (index = 0; index < count; ++index) {
        W8Prop* prop = *g_world->collidable_props->GetAt(objects[index]);
        if (prop->GetActivationState() != 0 && prop->m_gd_prop != 0) {
            GDProp* gd_prop = prop->m_gd_prop;
            for (int surface_index = 0; surface_index < gd_prop->m_surface_count; ++surface_index) {
                W8GDSurface* surface = gd_prop->m_pGDSurfaces + surface_index;
                srVector3T<float> bounds[2];
                srVector3T<float> triangle[3];
                bounds[0] = *lower;
                bounds[1] = *upper;
                triangle[0] = gd_prop->m_pVertices[surface->vertex_indices[0]];
                triangle[1] = gd_prop->m_pVertices[surface->vertex_indices[1]];
                triangle[2] = gd_prop->m_pVertices[surface->vertex_indices[2]];
                if (TestSpatialTriangle(bounds, triangle, surface->Normal()) != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Box query over the shared query buffer: `*objects` carries the destination
   buffer in and out (null selects m_aulGDObjs), `excluded` is an object id
   pre-marked in the dedupe set (-1 = none). The world bounds convert to cell
   coordinates and the inclusive cell box is clipped against the three grid
   dimensions. Returns the collected entry count. */
// FUNCTION: WIZ8 0x0042f280
int W8Octree::QueryObjects(unsigned long** objects, const srVector3T<float>* lower,
                           const srVector3T<float>* upper, unsigned short kind, int excluded)
{
    srVector3T<int> start;
    srVector3T<int> end;
    srVector3T<int> cell;

    g_octree_state = *objects;
    if (g_octree_state == 0) {
        g_octree_state = m_aulGDObjs;
        *objects = g_octree_state;
    }
    m_gd_result_count = 0;
    m_visited_object_bits->ClearAll();
    if (excluded >= 0) {
        m_visited_object_bits->Set(excluded);
    }
    WorldPositionToCell(lower, &start);
    WorldPositionToCell(upper, &end);
    for (cell.x = start.x; cell.x <= end.x; ++cell.x) {
        if (cell.x < 0 || static_cast<int>(m_leaf_grid_dimensions.x) <= cell.x /* c-style-cast-ok:
                cell coordinate vs grid dimension */) {
            continue;
        }
        for (cell.y = start.y; cell.y <= end.y; ++cell.y) {
            if (cell.y < 0 || static_cast<int>(m_leaf_grid_dimensions.y) <= cell.y /* c-style-cast-ok:
                    cell coordinate vs grid dimension */) {
                continue;
            }
            for (cell.z = start.z; cell.z <= end.z; ++cell.z) {
                if (cell.z >= 0 && cell.z < static_cast<int>(m_leaf_grid_dimensions.z) /* c-style-cast-ok:
                        cell coordinate vs grid dimension */) {
                    CollectObjectsInCell(&cell, kind);
                }
            }
        }
    }
    g_octree_state = 0;
    return m_gd_result_count;
}

/* Append the objects of `kind` inside one cell to the shared query buffer.
   Kind 3 walks the static tree (or the direct leaf grid when one exists) to a
   leaf whose gd-polygon stream lists surface ids. Kinds 8, 9 and 12 query the
   object registry's cell hash; kinds above 12 accept both the 12 and 13
   halfwords packed into each registry value. Every append deduplicates
   through m_visited_object_bits and the buffer stops at 10000 entries. */
// FUNCTION: WIZ8 0x0042f400
unsigned int W8Octree::CollectObjectsInCell(const srVector3T<int>* cell, unsigned short kind)
{
    unsigned int found = 0;
    unsigned int leaf_index;
    int slot;

    switch (kind) {
    case W8_OCTREE_KIND_SURFACE:
        if (m_leaf_lookup == 0) {
            leaf_index = FindLeaf(cell);
        } else {
            leaf_index = m_leaf_lookup[m_spatial.m_leaf_grid_stride_y * cell->y +
                                       m_spatial.m_leaf_grid_stride_x * cell->x + cell->z];
        }
        if (leaf_index != 0 && m_leaves[leaf_index].gd_polygon_offset != 0) {
            const unsigned long* stream =
                m_gd_surface_index_stream + m_leaves[leaf_index].gd_polygon_offset;
            found = *stream;
            for (unsigned int index = 0; index < found; ++index) {
                if (9999 < m_gd_result_count) {
                    return found;
                }
                ++stream;
                if (!m_visited_object_bits->Set(*stream)) {
                    g_octree_state[m_gd_result_count] = *stream;
                    ++m_gd_result_count;
                }
            }
        }
        break;
    default:
        if (W8_OCTREE_KIND_LOCATION < kind) {
            if (cell != 0) {
                g_octree_query_cell = PackOctreeCellKey(cell->x, cell->y, cell->z);
                g_octree_query_slot = -1;
            }
            slot =
                object_registry->by_cell->FindNextEntry(&g_octree_query_cell, g_octree_query_slot);
            g_octree_query_slot = slot;
            if (slot >= 0) {
                unsigned int packed =
                    static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                g_octree_query_kind = static_cast<unsigned short>(OctreeKeyKind(packed));
                g_octree_query_id = static_cast<unsigned short>(packed);
                while (m_gd_result_count < 10000) {
                    short entry_kind = static_cast<short>(OctreeKeyKind(packed));
                    if ((entry_kind == W8_OCTREE_KIND_LOCATION ||
                         entry_kind == W8_OCTREE_KIND_NAVIGATOR) &&
                        !m_visited_object_bits->Set(OctreeKeyId(packed) - 1)) {
                        g_octree_state[m_gd_result_count] = g_octree_query_id - 1;
                        ++m_gd_result_count;
                    }
                    slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell,
                                                                   g_octree_query_slot);
                    g_octree_query_slot = slot;
                    if (slot < 0) {
                        return 0;
                    }
                    packed =
                        static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                    g_octree_query_kind = static_cast<unsigned short>(OctreeKeyKind(packed));
                    g_octree_query_id = static_cast<unsigned short>(packed);
                }
            }
            break;
        }
        /* fall through */
    case W8_OCTREE_KIND_PROP:
    case W8_OCTREE_KIND_WAYPOINT:
    case W8_OCTREE_KIND_LOCATION:
        if (cell != 0) {
            g_octree_query_cell = PackOctreeCellKey(cell->x, cell->y, cell->z);
            g_octree_query_slot = -1;
        }
        slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell, g_octree_query_slot);
        g_octree_query_slot = slot;
        if (slot >= 0) {
            unsigned int packed =
                static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
            g_octree_query_kind = static_cast<unsigned short>(OctreeKeyKind(packed));
            g_octree_query_id = static_cast<unsigned short>(packed);
            while (m_gd_result_count < 10000) {
                if ((static_cast<unsigned short>(OctreeKeyKind(packed)) == kind) &&
                    !m_visited_object_bits->Set(OctreeKeyId(packed) - 1)) {
                    g_octree_state[m_gd_result_count] = g_octree_query_id - 1;
                    ++m_gd_result_count;
                }
                slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell,
                                                               g_octree_query_slot);
                g_octree_query_slot = slot;
                if (slot < 0) {
                    return 0;
                }
                packed = static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                g_octree_query_kind = static_cast<unsigned short>(OctreeKeyKind(packed));
                g_octree_query_id = static_cast<unsigned short>(packed);
            }
        }
        break;
    }
    return found;
}

/* Register the navigator's world position in the spatial cell registry.
   Navigator IDs are stored with a one-based bias. */
// FUNCTION: WIZ8 0x0042e810
void W8Octree::RegisterNavigatorCell(int id, const srVector3T<float>* position)
{
    srVector3T<int> point;

    WorldPositionToCell(position, &point);
    object_registry->RegisterObjectCell(W8_OCTREE_KIND_NAVIGATOR, id + 1, &point);
}

/* Convert a world position to cell coordinates, tracking whether it stays
   inside the spatial minimum/maximum box. Callers only use the coordinates;
   the in-range result the image also computes is not consumed. */
// FUNCTION: WIZ8 0x00431440
srVector3T<int>* W8Octree::WorldPositionToCell(const srVector3T<float>* position,
                                               srVector3T<int>* point)
{
    bool inside = true;

    for (int axis = 0; axis < 3; ++axis) {
        (&point->x)[axis] = static_cast<int>(
            (((&position->x)[axis] - (&m_spatial.m_minimum.x)[axis]) / m_spatial.m_node_extent));

        if ((&position->x)[axis] < (&m_spatial.m_minimum.x)[axis] ||
            (&position->x)[axis] > (&m_spatial.m_maximum.x)[axis]) {
            inside = false;
        }
    }
    return inside ? point : 0;
}

/* Clamp a position under the spatial ceiling, settle it to the ground through
   the surface walk, and keep the settled height only when something was hit.
   The walk reports through the flag; the height it wrote is discarded on a
   miss. */
// FUNCTION: WIZ8 0x00431DA0
void W8Octree::AdjustPosition(srVector3T<float>* position, unsigned int mode)
{
    srVector3T<float> adjusted;
    bool hit = false;

    if (m_spatial.m_clipped_maximum.y < position->y) {
        position->y = m_spatial.m_clipped_maximum.y;
    }
    adjusted = *position;
    SettleToGround(&adjusted, &hit, mode, 500.0f);
    if (hit) {
        position->y = adjusted.y;
    }
}

/* Step one navigator's movement tail towards its target.

   With a pathing service present the work belongs to that service, and which of
   its two entry points runs is the link mode's decision. Without one this walks
   the target itself. The direction is the full vector to the target with its
   height component dropped, so the step stays in the horizontal plane; its
   length is clamped to the distance remaining, and reaching the target is what
   the return value reports. The new position is mirrored into the attachment's
   own three copies. */
// FUNCTION: WIZ8 0x00434620
unsigned int W8Octree::AdvanceNavigator(W8NavigatorMovementState* movement, float radius,
                                        float separation)
{
    srVector3T<float> vecDir;
    srVector3T<float> vecPos;
    W8NavigatorAttachment* attachment;
    float distance;
    float step;
    bool reached = true;

    if (pathing != 0) {
        if (g_navigator_link_mode == 0) {
            return pathing->StepAlongPath(movement, radius, separation);
        }
        return pathing->StepMonsterAlongPath(movement, radius, separation);
    }
    if (g_navigator_link_mode != 0) {
        return 1;
    }
    vecDir = movement->target_position - movement->position;
    vecDir.y = 0.0f;
    distance = vecDir.xz().Length();
    step = g_game_time_accumulator->GetFrameDelta() * movement->movement_scale * g_rate *
           g_world_scale;
    if (step >= distance) {
        step = distance;
    } else {
        reached = false;
    }
    if (((vecDir.x * vecDir.x + vecDir.z * vecDir.z)) != g_double_zero) {
        vecDir.SetLength(step);
    }
    vecPos = vecDir + movement->position;
    movement->position = vecPos;
    attachment = movement->attachment;
    *attachment->path_positions = vecPos;
    attachment->path_length_origin = *attachment->path_positions;
    attachment->segment_start = *attachment->path_positions;
    return reached;
}

/* Settle both ends of a portal transition onto the ground before the move is
   applied. Each end is capped at the octree's height ceiling first, and its
   settled height is only taken when the drop actually hit something. The pair
   is then handed on together, which is why one body carries both. */
// FUNCTION: WIZ8 0x00434a30
void W8Octree::AdjustPortalDestination(srVector3T<float>* destination,
                                       const srVector3T<float>* source)
{
    srVector3T<float> local_destination;
    srVector3T<float> local_source;
    srVector3T<float> probe;
    bool hit;

    if (pathing == 0) {
        return;
    }
    if (!pathing->m_waypoint_editing && pathing->m_ulNumWayPoints != 0) {
        return;
    }
    local_destination = *destination;
    local_source = *source;

    hit = false;
    if (local_destination.y > m_spatial.m_clipped_maximum.y) {
        local_destination.y = m_spatial.m_clipped_maximum.y;
    }
    probe = local_destination;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit) {
        local_destination.y = probe.y;
    }

    hit = false;
    if (local_source.y > m_spatial.m_clipped_maximum.y) {
        local_source.y = m_spatial.m_clipped_maximum.y;
    }
    probe = local_source;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit) {
        local_source.y = probe.y;
    }
    pathing->EditTeleportalLink(&local_destination, &local_source);
}

/* Release the location-variable names and empty their parallel value and
   level vectors. Trigger.cpp creates the names as copied character arrays. */

/* The .oct writers stage at most 0x100 records through a stack buffer per
   FileWrite call. */

template <class Vector>
static BOOLEAN WriteStagedVectorArray(int file, const Vector* values, int count)
{
    Vector staging[0x100];
    int written = 0;
    int chunk_count;
    int index;
    BOOLEAN success = TRUE;

    while (success && written < count) {
        chunk_count = count - written;
        if (chunk_count > 0x100) {
            chunk_count = 0x100;
        }
        if (chunk_count != 0) {
            for (index = 0; index < chunk_count; ++index) {
                staging[index] = values[written + index];
            }
            written += chunk_count;
            success &= FileWrite(file, staging, chunk_count * sizeof(staging[0]), 0);
        }
    }
    return success;
}

// FUNCTION: WIZ8 0x004372E0
BOOLEAN WriteVector4Array(int file, const srVector4T<float>* values, int count)
{
    return WriteStagedVectorArray(file, values, count);
}

// FUNCTION: WIZ8 0x00437390
BOOLEAN WriteVector3Array(int file, const srVector3T<float>* values, int count)
{
    return WriteStagedVectorArray(file, values, count);
}

// FUNCTION: WIZ8 0x00437430
BOOLEAN WriteVector2Array(int file, const srVector2T<float>* values, int count)
{
    return WriteStagedVectorArray(file, values, count);
}

// FUNCTION: WIZ8 0x004374C0
bool ReadVector4Array(int file, srVector4T<float>* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector4T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x004374E0
bool ReadVector3Array(int file, void* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector3T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437510
bool ReadVector2Array(int file, srVector2T<float>* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector2T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437540
float PointToSegmentDistance(srVector3T<float>* point, const srVector3T<float>* from,
                             const srVector3T<float>* to, bool clamp_point, float* out_t)
{
    srVector3T<float> direction = *to - *from;
    srVector3T<float> offset = *point - *from;
    srVector3T<float> remaining = offset;
    float t = DotProduct(offset, direction) / direction.LengthSquared();
    if (g_double_zero < t) {
        if (t <= g_double_one) {
            remaining -= direction * t;
        } else {
            remaining = *point - *to;
        }
    }
    if (clamp_point) {
        if (g_double_zero <= t) {
            if (g_double_one < t) {
                *point = *to;
            } else {
                *point = *from + direction * t;
            }
        } else {
            *point = *from;
        }
    }
    if (out_t != 0) {
        if (g_double_zero <= t) {
            if (t <= g_double_one) {
                *out_t = t;
            } else {
                *out_t = 1.0f;
            }
        } else {
            *out_t = 0.0f;
        }
    }
    return remaining.Length();
}

// FUNCTION: WIZ8 0x00437760
float PointToSegmentDistance2D(srVector2T<float>* point, const srVector2T<float>* from,
                               const srVector2T<float>* to, bool clamp_point, float* out_t)
{
    float dx = to->x - from->x;
    float dy = to->y - from->y;
    float offset_x = point->x - from->x;
    float offset_y = point->y - from->y;
    float t = (offset_x * dx + offset_y * dy) / (dx * dx + dy * dy);
    if (static_cast<float>(g_double_zero) < t) {
        if (t <= static_cast<float>(g_double_one)) {
            offset_x = offset_x - dx * t;
            offset_y = offset_y - dy * t;
        } else {
            offset_x = point->x - to->x;
            offset_y = point->y - to->y;
        }
    }
    if (clamp_point) {
        if (static_cast<float>(g_double_zero) <= t) {
            if (static_cast<float>(g_double_one) < t) {
                *point = *to;
            } else {
                point->Set(dx * t + from->x, dy * t + from->y);
            }
        } else {
            *point = *from;
        }
    }
    if (out_t != 0) {
        if (static_cast<float>(g_double_zero) <= t) {
            if (t <= static_cast<float>(g_double_one)) {
                *out_t = t;
            } else {
                *out_t = 1.0f;
            }
        } else {
            *out_t = 0.0f;
        }
    }
    return static_cast<float>(sqrt(offset_x * offset_x + offset_y * offset_y));
}

/* Grow `minimum`/`maximum` to include `point`, returning whether any bound
   moved. Retail's second comparison tests point->z where point->y is meant
   (0x0043790F FCOMPs [ECX+8]) - the proven quirk stays. */
// FUNCTION: WIZ8 0x004378f0
char GrowBoundsByPoint(const srVector3T<float>* point, srVector3T<float>* minimum,
                       srVector3T<float>* maximum)
{
    bool changed = false;
    if (minimum->x > point->x) {
        minimum->x = point->x;
        changed = true;
    }
    if (minimum->y > point->z) {
        minimum->y = point->y;
        changed = true;
    }
    if (minimum->z > point->z) {
        minimum->z = point->z;
        changed = true;
    }
    if (maximum->x < point->x) {
        maximum->x = point->x;
        changed = true;
    }
    if (maximum->y < point->y) {
        maximum->y = point->y;
        changed = true;
    }
    if (maximum->z < point->z) {
        maximum->z = point->z;
        return 1;
    }
    return changed;
}

/* Whether `point` lies within `radius` of the bounds box, as the squared
   distance from the box-clamped point. Retail's z clamp picks bounds->minimum.z
   where bounds->maximum.z is meant when the point sits above the box
   (0x0043871B FLDs [ECX+8]) - the proven quirk stays. */
// FUNCTION: WIZ8 0x004386a0
bool SphereNearBounds(const srVector3T<float>* point, float radius, const W8BoundingBox* bounds)
{
    float closest_x;
    float closest_y;
    float closest_z;
    if (point->x < bounds->minimum.x) {
        closest_x = bounds->minimum.x;
    } else if (point->x <= bounds->maximum.x) {
        closest_x = point->x;
    } else {
        closest_x = bounds->maximum.x;
    }
    if (point->y < bounds->minimum.y) {
        closest_y = bounds->minimum.y;
    } else if (point->y <= bounds->maximum.y) {
        closest_y = point->y;
    } else {
        closest_y = bounds->maximum.y;
    }
    if (point->z < bounds->minimum.z) {
        closest_z = bounds->minimum.z;
    } else if (point->z <= bounds->maximum.z) {
        closest_z = point->z;
    } else {
        closest_z = bounds->minimum.z;
    }
    float distance_squared = (point->y - closest_y) * (point->y - closest_y) +
                             (point->x - closest_x) * (point->x - closest_x) +
                             (point->z - closest_z) * (point->z - closest_z);
    return distance_squared < radius * radius;
}

unsigned int W8Octree::QueryNearbyLocations(const srVector3T<float>* position, float spacing,
                                            unsigned long** candidates)
{
    float expand = spacing * g_monster_proximity_radius_scale;
    srVector3T<float> low;
    low.Set(position->x - expand, position->y - expand, position->z - expand);
    srVector3T<float> high;
    high.Set(expand + position->x, expand + position->y, expand + position->z);
    *candidates = static_cast<unsigned long*>(operator new(0x400));
    return static_cast<unsigned int>(
        QueryObjects(candidates, &low, &high, W8_OCTREE_KIND_LOCATION, -1));
}

/* Lateral column offsets for the scatter search below: the inner loop walks
   three columns (0, +1, -1) per ring, or all five when more than ten
   positions are wanted. */
// GLOBAL: WIZ8 0x00605ae8
static float g_scatter_column_offsets[5] = {0.0f, 1.0f, -1.0f, 2.0f, -2.0f};

/* Ring-scatter search for up to `count` clear positions around `position`.
   Each ring steps `spacing` units out along the `yaw`-rotated frame and
   columns offset laterally through g_scatter_column_offsets; the
   first accepted candidate becomes the reference the remaining candidates
   must span to. With `proximity_check` set each candidate is also rejected
   inside the startup navigator's or any nearby monster's radius. `flatten_y`
   relaxes the tight vertical bound during search and rewrites every accepted
   y to the original source height afterwards. */
// FUNCTION: WIZ8 0x00437980
unsigned int W8Octree::FindScatterPositions(const srVector3T<float>* position, float yaw,
                                            float spacing, unsigned int count,
                                            srVector3T<float>* positions, bool proximity_check,
                                            bool flatten_y)
{
    float source_y = position->y;
    unsigned int found = 0;
    unsigned long* candidates = 0;
    unsigned int columns = 3;
    if (count > 10) {
        columns = 5;
    }
    float angle = NormalizeAngle(yaw + g_monster_rotation_offset);
    double cos_angle = cos(angle);
    srVector3T<float> source;
    source = *position;
    double sin_angle = sin(angle);
    float cos_step = spacing * cos_angle;
    float sin_step = spacing * sin_angle;
    float neg_cos_step = -cos_step;
    unsigned int monsters = 0;
    if (proximity_check) {
        monsters = QueryNearbyLocations(position, spacing, &candidates);
    }
    unsigned int ring;
    for (ring = 0; ring < 10; ++ring) {
        if (count <= found) {
            break;
        }
        if (columns != 0) {
            const float* column_offset = g_scatter_column_offsets;
            unsigned int column = 0;
            do {
                if (count <= found) {
                    break;
                }
                float jitter_ring;
                float jitter_column;
                if (found == 0) {
                    jitter_ring = 0.0f;
                    jitter_column = g_float_zero;
                } else {
                    jitter_ring = Random(1000) * g_scatter_outer_ring_jitter_scale -
                                  g_generator_jitter_fraction;
                    jitter_column = Random(1000) * g_scatter_outer_ring_jitter_scale -
                                    g_generator_jitter_fraction;
                }
                srVector3T<float> candidate;
                candidate.x = sin_step * (ring + jitter_ring) + source.x +
                              neg_cos_step * (jitter_column + *column_offset);
                candidate.y = source.y + g_world_scale;
                candidate.z = (ring + jitter_ring) * cos_step + source.z +
                              (jitter_column + *column_offset) * sin_step;
                float ground = SettlePositionToGround(&candidate, 0);
                float height = candidate.y - ground;
                if (g_double_one <= fabsf(height) &&
                    fabsf(height) <= g_navigator_vertical_snap_limit &&
                    (flatten_y || fabsf(height) <= g_double_twenty_five_hundred)) {
                    candidate.y = ground;
                    if (!proximity_check) {
                        if (pathing->SnapWaypointPosition(&candidate, false)) {
                            goto accept;
                        }
                    } else {
                        float reach = spacing * g_float_half;
                        if (pathing->TestPathCellClearance(&candidate, reach, false)) {
                            srVector3T<float> navigator = g_startup_world->GetPosition();
                            float dx = navigator.x - candidate.x;
                            float dy = navigator.y - candidate.y;
                            float dz = navigator.z - candidate.z;
                            float separation = g_startup_world->radius + reach;
                            if (separation * separation <= dx * dx + dy * dy + dz * dz) {
                                unsigned int m;
                                for (m = 0; m < monsters; ++m) {
                                    W8Monster* monster = GetMonsterByLocationID(candidates[m]);
                                    srVector3T<float> monster_position = monster->GetPosition();
                                    float mdx = monster_position.x - candidate.x;
                                    float mdy = monster_position.y - candidate.y;
                                    float mdz = monster_position.z - candidate.z;
                                    float clearance = monster->radius + reach;
                                    if (mdx * mdx + mdy * mdy + mdz * mdz < clearance * clearance) {
                                        goto next_cell;
                                    }
                                }
                                goto accept;
                            }
                        }
                    }
                    goto next_cell;
                accept:
                    if (found == 0) {
                        positions[0] = candidate;
                        source = candidate;
                        found = 1;
                    } else if (pathing == 0 ||
                               pathing->TestWaypointSpan(&candidate, &source, false, false)) {
                        positions[found] = candidate;
                        ++found;
                    }
                }
            next_cell:
                ++column;
                ++column_offset;
            } while (column < columns);
        }
    }
    if (candidates != 0) {
        delete[] candidates;
    }
    if (flatten_y && found != 0) {
        for (unsigned int index = 0; index < found; ++index) {
            positions[index].y = source_y;
        }
    }
    return found;
}

// FUNCTION: WIZ8 0x00437f30
unsigned int W8Octree::FindNavigatorPosition(srVector3T<float>* source, float yaw, float radius,
                                             unsigned int count, srVector3T<float>* positions,
                                             bool first_only, bool settle_any_height,
                                             bool avoid_triggers, int mode,
                                             bool require_waypoint_span)
{
    unsigned int found = 0;
    int found_i = 9999;
    int found_j = 9999;
    float source_y = source->y;
    unsigned long* candidates = 0;
    float camera_radius = g_startup_world->movement.alternate_radius;
    bool placed = false;
    float separation = (CalcRangeDistance(W8_RANGE_TOUCH) + radius) * g_float_half + camera_radius;
    float angle = NormalizeAngle(yaw + g_monster_rotation_offset);
    double cos_angle = cos(angle);
    double sin_angle = sin(angle);
    float cos_radius = radius * cos_angle;
    float sin_radius = radius * sin_angle;
    float neg_cos_radius = -cos_radius;
    source->y += g_world_scale;
    float ground = SettlePositionToGround(source, 0);
    float height = source->y - ground;
    if (g_double_one <= fabsf(height) && fabsf(height) <= g_navigator_vertical_snap_limit &&
        (settle_any_height || fabsf(height) <= g_double_twenty_five_hundred)) {
        source->y = ground;
    }
    unsigned int monsters = 0;
    if (first_only) {
        monsters = QueryNearbyLocations(source, radius, &candidates);
    }
    int ring_upper = 0;
    if (-1 < mode) {
        int lower = 0;
        do {
            if (count <= found) {
                break;
            }
            int i = lower;
            int upper = ring_upper;
            if (lower <= ring_upper) {
                do {
                    if (count <= found) {
                        break;
                    }
                    int j = lower;
                    if (placed) {
                        goto advance_ring;
                    }
                    do {
                        if (count <= found || placed) {
                            break;
                        }
                        if ((i <= lower || upper <= i || j <= lower || upper <= j) &&
                            (i != found_i || j != found_j)) {
                            float jitter_i;
                            float jitter_j;
                            if (found == 0) {
                                jitter_i = 0.0f;
                                jitter_j = g_float_zero;
                            } else {
                                jitter_i = Random(1000) * g_scatter_inner_ring_jitter_scale -
                                           g_float_one_tenth;
                                jitter_j = Random(1000) * g_scatter_inner_ring_jitter_scale -
                                           g_float_one_tenth;
                            }
                            srVector3T<float> candidate;
                            candidate.x = sin_radius * (i + jitter_i) + source->x +
                                          neg_cos_radius * (jitter_j + j);
                            candidate.y = source->y + g_world_scale;
                            candidate.z = (i + jitter_i) * cos_radius + source->z +
                                          (jitter_j + j) * sin_radius;
                            ground = SettlePositionToGround(&candidate, 0);
                            height = candidate.y - ground;
                            if (g_double_one <= fabsf(height) &&
                                fabsf(height) <= g_navigator_vertical_snap_limit &&
                                (settle_any_height ||
                                 fabsf(height) <= g_double_twenty_five_hundred)) {
                                candidate.y = ground;
                                unsigned char clear;
                                if (!first_only) {
                                    clear = pathing->SnapWaypointPosition(&candidate, false);
                                } else {
                                    clear = pathing->TestPathCellClearance(
                                        &candidate, radius * g_float_half, false);
                                }
                                if (clear != 0 && (!avoid_triggers ||
                                                   !InsideDestinationTrigger(
                                                       candidate.x, candidate.y, candidate.z))) {
                                    if (first_only) {
                                        srVector3T<float> camera = g_startup_world->GetPosition();
                                        float camera_dx = camera.x - candidate.x;
                                        float camera_dy = camera.y - candidate.y;
                                        float camera_dz = camera.z - candidate.z;
                                        if (camera_dx * camera_dx + camera_dy * camera_dy +
                                                camera_dz * camera_dz <
                                            separation * separation) {
                                            goto next_cell;
                                        }
                                        unsigned int m = 0;
                                        if (monsters != 0) {
                                            float reach = radius * g_float_half;
                                            do {
                                                W8Monster* monster =
                                                    GetMonsterByLocationID(candidates[m]);
                                                srVector3T<float> position = monster->GetPosition();
                                                float monster_dx = position.x - candidate.x;
                                                float monster_dy = position.y - candidate.y;
                                                float monster_dz = position.z - candidate.z;
                                                float clearance =
                                                    monster->movement.alternate_radius + reach;
                                                if (monster_dx * monster_dx +
                                                        monster_dy * monster_dy +
                                                        monster_dz * monster_dz <
                                                    clearance * clearance) {
                                                    goto next_cell;
                                                }
                                                ++m;
                                            } while (m < monsters);
                                        }
                                    }
                                    if (!require_waypoint_span || pathing == 0 ||
                                        pathing->TestWaypointSpan(&candidate, source, false,
                                                                  false)) {
                                        if (found == 0) {
                                            placed = true;
                                            found_i = i;
                                            found_j = j;
                                        }
                                        positions[found] = candidate;
                                        ++found;
                                    } else if (found != 0) {
                                        unsigned int k = 0;
                                        srVector3T<float>* existing = positions;
                                        do {
                                            if (pathing->TestWaypointSpan(&candidate, existing,
                                                                          false, false)) {
                                                positions[found] = candidate;
                                                ++found;
                                                break;
                                            }
                                            ++k;
                                            ++existing;
                                        } while (k < found);
                                    }
                                }
                            }
                        }
                    next_cell:
                        ++j;
                    } while (j <= ring_upper);
                    ++i;
                } while (i <= upper);
            }
            if (placed) {
            advance_ring:
                --upper;
                ++lower;
                placed = false;
            }
            ring_upper = upper + 1;
            --lower;
        } while (ring_upper <= mode);
    }
    if (candidates != 0) {
        delete candidates;
    }
    if (settle_any_height && 0 < static_cast<int>(found)) {
        srVector3T<float>* out = positions;
        unsigned int remaining = found;
        do {
            out->y = source_y;
            ++out;
            --remaining;
        } while (remaining != 0);
    }
    return found;
}
