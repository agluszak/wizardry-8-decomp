#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <sys/stat.h>

#include "surrender/srCamera.h"
#include "surrender/srHeap.h"
#include "surrender/srMath.h"
#include "surrender/srScene.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
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
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
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
void W8Octree::GetPathSurfaceNormal00433A70(const srVector3T<float>* position,
                                            srVector3T<float>* normal)
{
    if (pathing_180 != 0) {
        pathing_180->GetPathSurfaceNormal0045B730(position, normal);
        return;
    }
    normal->x = 0.0f;
    normal->y = 1.0f;
    normal->z = 0.0f;
}

// FUNCTION: WIZ8 0x00434170
void W8Octree::UpdatePathVisualization()
{
    if (pathing_180 != 0) {
        srVector3T<float> cursor;
        GetWorldCursorPosition00490BF0(&cursor);
        /* Retail reads the y component twice (proven in disassembly: both
           fcomp loads use the y slot) and never tests x. */
        if (cursor.y >= g_float_005ebb34 || cursor.y >= g_float_005ebb34 ||
            cursor.z >= g_float_005ebb34) {
            srVector3T<float> point = cursor;
            pathing_180->UpdatePathVisualization0045BC40(&point, &camera_dof_1cc);
            return;
        }
        pathing_180->UpdatePathVisualization0045BC40(&camera_location_1c0, &camera_dof_1cc);
    }
}

// GLOBAL: WIZ8 0x00659760
int g_octree_query_slot_00659760;
// GLOBAL: WIZ8 0x00659764
unsigned short g_octree_query_kind_00659764;
// GLOBAL: WIZ8 0x00659766
unsigned short g_octree_query_id_00659766;

// GLOBAL: WIZ8 0x00659770
unsigned int* g_octree_storage_00659770;

// GLOBAL: WIZ8 0x00659774
unsigned int g_octree_query_cell_00659774;

// GLOBAL: WIZ8 0x00659778
GETFILESTRUCT g_octree_file_search_00659778;

// GLOBAL: WIZ8 0x006598b2
unsigned char g_octree_file_search_active_006598b2;

// GLOBAL: WIZ8 0x00606810
char g_octree_point_extension_00606810[] = ".pts";

// GLOBAL: WIZ8 0x006068a0
char g_octree_file_search_wildcard_006068a0[] = "*";

// GLOBAL: WIZ8 0x00659890
unsigned long* g_octree_state_00659890;
// GLOBAL: WIZ8 0x00659894
srNode* g_octree_trace_node_00659894;
// GLOBAL: WIZ8 0x00659898
unsigned char g_octree_update_suspended_00659898;
// GLOBAL: WIZ8 0x00659899
unsigned char g_octree_trace_enabled_00659899;

/* Build a packed four-byte colour from four components and answer its
   address. The receiver is the output slot. */
/* Draw the probe box through the world camera. */

// GLOBAL: WIZ8 0x006598a4
W8Octree* g_octree_6598a4;

// GLOBAL: WIZ8 0x006068a4
char g_region_link_extension_006068a4[] = ".rlk";

// GLOBAL: WIZ8 0x006598a8
unsigned char g_flag_6598a8;
// GLOBAL: WIZ8 0x006598b0
unsigned short g_octree_region_debug_last_006598b0;

/* Renderer switches toggled across a region-link build; their other
   consumers are unrecovered render routines. */
// GLOBAL: WIZ8 0x0065a0ec
unsigned char g_flag_0065a0ec;
// GLOBAL: WIZ8 0x0065a0ed
unsigned char g_flag_0065a0ed;
// GLOBAL: WIZ8 0x0065a0ee
unsigned char g_flag_0065a0ee;
// GLOBAL: WIZ8 0x0065a146
unsigned char g_flag_0065a146;

// FUNCTION: WIZ8 0x0042bc00
void NoOct(void)
{
    g_flag_6598a8 = 1;
}

// GLOBAL: WIZ8 0x005ec02c
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;

/* Noise falloff on the trace resolver's sphere-distance penalty: the farther
   the probe is along the ray, the more the candidate's effective distance is
   discounted. Shared with CreateTraceModel. */
// GLOBAL: WIZ8 0x005ebc78
float g_float_005ebc78 = 0.15000000596046448f;
/* Vertical snap ceiling for navigator placement: the source may rise or fall
   at most this many units before a candidate is rejected outright. */
// GLOBAL: WIZ8 0x005ec038
double g_double_005ec038 = 5000.0;
/* Fixed camera tilt (15 degrees below horizontal) the region-link projector
   applies to every sampled direction. */
// GLOBAL: WIZ8 0x005ec008
double g_double_005ec008 = -0.26179999113082886;
/* Circle-coverage bound just under 2*pi: when samples*fov still falls short,
   one more direction is added. */
// GLOBAL: WIZ8 0x005ec010
float g_float_005ec010 = 6.282185077667236f;
/* Jitter scale applied to the Random(1000) roll for scatter-ring candidates
   past the first; 0.0004 * 1000 spans 0.4 units. */
// GLOBAL: WIZ8 0x005ec044
float g_float_005ec044 = 0.00040000001899898052f;
/* Multiplier on the placement radius that gives the monster-proximity query
   box its extent. */
// GLOBAL: WIZ8 0x005ec048
float g_float_005ec048 = 15.0f;
/* Jitter scale applied to the Random(1000) roll for ring candidates past the
   first; 0.0002 * 1000 spans 0.2 units. */
// GLOBAL: WIZ8 0x005ec050
float g_float_005ec050 = 0.00020000000949949026f;

// FUNCTION: WIZ8 0x0042f7e0
void W8Octree::UpdateCameraVisibility0042F7E0()
{
    W8World* world = GetWorld();
    if (g_octree_update_suspended_00659898 != 0 || m_positional_294 != 0) {
        return;
    }

    far_clip_200 = static_cast<float>(WorldGetFarClip(world));
    world->camera->getLocation(camera_location_1c0);
    horizontal_fov_1f0 = static_cast<float>(world->camera->getHorizontalFOV());
    vertical_fov_1f4 = static_cast<float>(world->camera->getVerticalFOV());

    srMatrix3T<float> rotation;
    world->camera->getRotation(rotation);
    rotation_column_1d8.x = 1.0f + rotation.vectors[0].x;
    rotation_column_1d8.y = rotation.vectors[1].x;
    rotation_column_1d8.z = rotation.vectors[2].x;
    rotation_column_1e4.x = 1.0f + rotation.vectors[0].y;
    rotation_column_1e4.y = rotation.vectors[1].y;
    rotation_column_1e4.z = rotation.vectors[2].y;

    srVector3T<double> dof = world->camera->getWorldSpaceDOF();
    camera_dof_1cc = dof;
    horizontal_fov_cosine_1f8 = static_cast<float>(cos(horizontal_fov_1f0));
    vertical_fov_cosine_1fc = static_cast<float>(cos(vertical_fov_1f4));
    m_owned_190->ClearAll();
    m_projected_regions_15c->ClearAll();
    UpdateVisibility004304A0();
    if (g_octree_trace_enabled_00659899 != 0) {
        UpdateWorldTrace00433EB0();
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
void W8Octree::UpdateVisibility004304A0()
{
    unsigned short mesh_index;
    unsigned int index;
    int bit;

    if (m_reset_visibility_168 != 0) {
        m_reset_visibility_168 = 0;
        for (mesh_index = 0; mesh_index < m_meshCount_1b4; ++mesh_index) {
            if (g_world->psrMeshes[mesh_index] != 0) {
                m_pSubmeshes
                    [static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index])->state_17c + 1]
                        .mesh_04 = mesh_index;
                m_pSubmeshes[mesh_index + 1].flags_00 &= 0xffffffc7;
                stModelInstance* mesh =
                    static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index]);
                if (mesh != 0) {
                    mesh->setFlag(srNode::FLAG_DISABLE);
                    mesh->setFlag(srNode::FLAG_TERMINATE);
                }
            }
        }
        for (index = 0; index < m_usNumPropsLoaded; ++index) {
            m_papProps[index]->SetSetting6C(0);
        }
        if (m_pusMeshProps != 0) {
            for (index = 0; m_pusMeshProps[index] != 0; ++index) {
                m_papProps[m_pusMeshProps[index] - 1]->SetSetting6C(1);
            }
        }
        for (index = 0; index < m_usNumParticlesLoaded; ++index) {
            m_papParticles[index]->SetTraversalEnabled00498D90(0);
        }
        if (m_pusMeshParticles != 0) {
            for (index = 0; m_pusMeshParticles[index] != 0; ++index) {
                m_papParticles[m_pusMeshParticles[index] - 1]->SetTraversalEnabled00498D90(1);
            }
        }
        ValidateRegionMeshLinks00433AB0();
        m_previous_regions_164->ClearAll();
        m_accumulated_regions_198->ClearAll();
        if (m_ulNumParticles != 0) {
            m_visible_particles_100->ClearAll();
        }
        if (m_ulNumProps != 0) {
            m_visible_props_108->ClearAll();
        }
    }

    m_current_regions_160->ClearAll();
    m_projected_regions_15c->ClearAll();
    if (m_ulNumParticles != 0) {
        m_particles_to_disable_10c->ClearAll();
        m_linked_particles_0fc->ClearAll();
    }
    if (m_ulNumProps != 0) {
        m_linked_props_104->ClearAll();
        m_props_to_disable_110->ClearAll();
    }
    m_projected_regions_valid_16a = 0;
    m_positional_16b = 0;
    CollectVisibleRegions00430D50(&camera_location_1c0, m_positional_204, 0, 1);
    CollectVisibleCells0042FE90();
    if (pathing_180 != 0) {
        srVector3T<float> dof;
        GetWorldCursorPosition00490BF0(&dof);
        /* Same proven retail quirk as UpdatePathVisualization: y is tested
           twice and x is never examined. */
        if (dof.y >= g_float_005ebb34 || dof.y >= g_float_005ebb34 || dof.z >= g_float_005ebb34) {
            srVector3T<float> probe = dof;
            pathing_180->UpdatePathVisualization0045BC40(&probe, &camera_dof_1cc);
        } else {
            pathing_180->UpdatePathVisualization0045BC40(&camera_location_1c0, &camera_dof_1cc);
        }
    }
    if (m_projected_regions_valid_16a != 0) {
        m_current_regions_160->IntersectWith(*m_projected_regions_15c);
    }
    m_accumulated_regions_198->UnionWith(*m_current_regions_160);
    bit = m_previous_regions_164->NextSetBit(1);
    while (bit != 0) {
        --bit;
        if (!m_current_regions_160->Test(bit) && bit != 0) {
            W8OctSubmesh* submesh = &m_pSubmeshes[bit];
            submesh->flags_00 &= 0xfffffff7;
            stModelInstance* mesh =
                static_cast<stModelInstance*>(g_world->psrMeshes[submesh->mesh_04]);
            if (mesh != 0) {
                mesh->setFlag(srNode::FLAG_DISABLE);
                mesh->setFlag(srNode::FLAG_TERMINATE);
            }
        }
        bit = m_previous_regions_164->NextSetBit(0);
    }
    bit = m_current_regions_160->NextSetBit(1);
    while (bit != 0) {
        --bit;
        if (!m_previous_regions_164->Test(bit) && bit != 0) {
            W8OctSubmesh* submesh = &m_pSubmeshes[bit];
            int mesh_index = submesh->mesh_04;
            submesh->flags_00 |= 0x28;
            stModelInstance* mesh = static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index]);
            if (mesh != 0) {
                mesh->clearFlag(srNode::FLAG_DISABLE);
                mesh->clearFlag(srNode::FLAG_TERMINATE);
            }
        }
        MarkMeshLinksVisible00430A70(bit);
        bit = m_current_regions_160->NextSetBit(0);
    }
    if (m_ulNumParticles != 0) {
        for (index = 0; m_pusMeshParticles[index] != 0; ++index) {
            m_linked_particles_0fc->Set(m_pusMeshParticles[index] - 1);
        }
        m_particles_to_disable_10c->SetToComplementOf(*m_linked_particles_0fc);
        m_particles_to_disable_10c->IntersectWith(*m_visible_particles_100);
        bit = m_linked_particles_0fc->NextSetBit(1);
        while (bit != 0) {
            --bit;
            if (!m_visible_particles_100->Test(bit)) {
                m_papParticles[bit]->SetTraversalEnabled00498D90(1);
            }
            bit = m_linked_particles_0fc->NextSetBit(0);
        }
        bit = m_particles_to_disable_10c->NextSetBit(1);
        while (bit != 0) {
            m_papParticles[bit - 1]->SetTraversalEnabled00498D90(0);
            bit = m_particles_to_disable_10c->NextSetBit(0);
        }
        m_visible_particles_100->CopyFrom(*m_linked_particles_0fc);
    }
    if (m_ulNumProps != 0) {
        for (index = 0; m_pusMeshProps[index] != 0; ++index) {
            m_linked_props_104->Set(m_pusMeshProps[index] - 1);
        }
        m_props_to_disable_110->SetToComplementOf(*m_linked_props_104);
        m_props_to_disable_110->IntersectWith(*m_visible_props_108);
        bit = m_linked_props_104->NextSetBit(1);
        while (bit != 0) {
            --bit;
            if (!m_visible_props_108->Test(bit)) {
                m_papProps[bit]->SetSetting6C(1);
            }
            bit = m_linked_props_104->NextSetBit(0);
        }
        bit = m_props_to_disable_110->NextSetBit(1);
        while (bit != 0) {
            m_papProps[bit - 1]->SetSetting6C(0);
            bit = m_props_to_disable_110->NextSetBit(0);
        }
        m_visible_props_108->CopyFrom(*m_linked_props_104);
    }
    m_previous_regions_164->CopyFrom(*m_current_regions_160);
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
                                     unsigned int flags, char only_accumulated)
{
    unsigned int level_mask = m_region_mask_140;
    unsigned int limit = spatial_000.region_cells_per_axis_50 - 1;

    int first[3];
    int last[3];
    int axis;
    for (axis = 0; axis < 3; ++axis) {
        first[axis] =
            static_cast<int>(((&point->x)[axis] - radius - (&spatial_000.minimum_0c.x)[axis]) /
                             spatial_000.region_grid_cell_54) -
            1;
        if (first[axis] < 0) {
            first[axis] = 0;
        }
        last[axis] =
            static_cast<int>(((&point->x)[axis] + radius - (&spatial_000.minimum_0c.x)[axis]) /
                             spatial_000.region_grid_cell_54) +
            1;
        if (limit < static_cast<unsigned int>(last[axis])) {
            last[axis] = limit;
        }
    }

    m_current_regions_160->ClearAll();
    int cell[3];
    for (cell[0] = first[0]; cell[0] <= last[0]; ++cell[0]) {
        for (cell[1] = first[1]; cell[1] <= last[1]; ++cell[1]) {
            for (cell[2] = first[2]; cell[2] <= last[2]; ++cell[2]) {
                int node = 1;
                int bit = 1 << spatial_000.depth_44;
                while (bit != 0 && node != 0) {
                    if ((level_mask & bit) != 0) {
                        int octant = 0;
                        if ((cell[0] & bit) != 0) {
                            octant = 4;
                        }
                        if ((cell[1] & bit) != 0) {
                            octant += 2;
                        }
                        if ((cell[2] & bit) != 0) {
                            octant += 1;
                        }
                        node = m_owned_09c[node].children_04[octant];
                    }
                    bit = bit / 2;
                }
                if (bit == 0 && node != 0) {
                    unsigned int region = m_owned_09c[node].region_02;
                    if (region != 0) {
                        m_current_regions_160->Set(region);
                    }
                }
            }
        }
    }

    unsigned int region = 1;
    if (1 < spatial_000.region_count_46) {
        do {
            if (SphereInsideFrustum0046D8D0(point, radius,
                                            spatial_000.owned_5c[region].planes_88) != 0 &&
                spatial_000.owned_5c[region].region_bit_0c != 0) {
                m_current_regions_160->Set(spatial_000.owned_5c[region].region_bit_0c);
            }
            ++region;
        } while (region < spatial_000.region_count_46);
    }

    if (m_ulNumProps == 0) {
        flags &= 0xffffff00;
    } else if ((flags & 0xff) != 0) {
        m_linked_props_104->ClearAll();
    }
    if (only_accumulated != 0) {
        m_current_regions_160->IntersectWith(*m_accumulated_regions_198);
    }

    int bit = m_current_regions_160->NextSetBit(1);
    while (bit != 0) {
        unsigned int submesh = bit - 1;
        if (submesh != 0) {
            stModelInstance* instance =
                static_cast<stModelInstance*>(g_world->psrMeshes[m_pSubmeshes[submesh].mesh_04]);
            if (instance != 0) {
                srModel* model = instance->model();
                if (model != 0) {
                    srVector3T<float> bounds[2];
                    model->getBoundingBox(bounds[0], bounds[1]);
                    if (SphereNearBounds(&point->x, radius, &bounds[0].x) != 0) {
                        out->Add(instance);
                    }
                }
            }
            if ((flags & 0xff) != 0) {
                MarkMeshLinksVisible00430A70(submesh);
            }
        }
        bit = m_current_regions_160->NextSetBit(0);
    }

    if ((flags & 0xff) != 0) {
        bit = m_linked_props_104->NextSetBit(1);
        while (bit != 0) {
            srModelInstance* instance = m_papProps[bit - 2]->ToggleRepAnimationDefault();
            if (instance != 0) {
                srModel* model = instance->model();
                if (model != 0) {
                    srVector3T<float> center;
                    float sphere_radius;
                    model->getBoundingSphere(center, sphere_radius);
                    srVector3T<float> prop_position;
                    m_papProps[bit - 1]->GetPosition0044E2C0(&prop_position);
                    center.x += prop_position.x;
                    center.y += prop_position.y;
                    center.z += prop_position.z;
                    sphere_radius += radius;
                    if ((center.x - point->x) * (center.x - point->x) +
                            (center.y - point->y) * (center.y - point->y) +
                            (center.z - point->z) * (center.z - point->z) <
                        sphere_radius * sphere_radius) {
                        out->Add(static_cast<stModelInstance*>(instance));
                    }
                }
            }
            bit = m_linked_props_104->NextSetBit(0);
        }
    }
    return out->count;
}

/* Add every particle and prop linked to one mesh to the live visibility sets.

   The lookup gives a 1-based start into a packed link table; each run is
   terminated by a zero. A linked entry whose slot is empty is skipped. */
// FUNCTION: WIZ8 0x00430a70
void W8Octree::MarkMeshLinksVisible00430A70(unsigned int mesh)
{
    if (mesh > m_meshCount_1b4) {
        return;
    }
    if (m_ulNumParticles != 0) {
        unsigned short link = m_pusMeshParticleLookup[mesh];
        if (link != 0 && m_pusMeshParticles[link] != 0) {
            do {
                if (m_usMeshParticlesLen_0e8 < link) {
                    srAssertFail("usProp<=m_usMeshParticlesLen", OCTREE_CPP, 0xa07,
                                 "Particle lookup index out of range");
                }
                unsigned short particle = m_pusMeshParticles[link];
                ++link;
                if (particle <= m_usNumParticlesLoaded && m_papParticles[particle] != 0) {
                    m_linked_particles_0fc->Set(particle - 1);
                }
            } while (m_pusMeshParticles[link] != 0);
        }
    }
    if (m_ulNumProps != 0) {
        unsigned short link = m_pusMeshPropLookup[mesh];
        if (link != 0 && m_pusMeshProps[link] != 0) {
            do {
                if (m_usMeshPropsLen_0f4 < link) {
                    srAssertFail("usProp<=m_usMeshPropsLen", OCTREE_CPP, 0xa1b,
                                 "Prop lookup index out of range");
                }
                unsigned short prop = m_pusMeshProps[link];
                ++link;
                if (prop <= m_usNumPropsLoaded && m_papProps[prop] != 0) {
                    m_linked_props_104->Set(prop - 1);
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
   ProjectLinkedRegionsForLocation00431050. */
// FUNCTION: WIZ8 0x00431050
short W8Octree::ProjectLinkedRegionsForLocation00431050(srVector3T<float>* location,
                                                        unsigned short* region_list)
{
    if (m_positional_169 == 0) {
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

            W8OctRegionVolume* volume = &spatial_000.owned_5c[region_index];
            if (PointInsideFrustum0046D880(location, volume->planes_88) == 0) {
                continue;
            }
            if (match_count == 0) {
                m_projected_regions_15c->ClearAll();
                m_projected_regions_valid_16a = 0;
            }
            ++match_count;
            unsigned int key = region_index;
            for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1); slot != -1;
                 slot = m_pRegionLinks_150->FindNextEntry(&key, slot)) {
                m_projected_regions_15c->Set(m_pRegionLinks_150->entries[slot].value);
                m_projected_regions_valid_16a = 1;
            }
        }
        if (match_count != 0) {
            m_positional_16b = 1;
            return match_count;
        }
    }

    char point_region_label[128];
    strcpy(point_region_label, expected_regions);

    if (spatial_000.region_count_46 < 2) {
        return 0;
    }

    unsigned short reported_region = 0;
    for (unsigned short region_index = 1; region_index < spatial_000.region_count_46;
         ++region_index) {
        if (match_count != 0) {
            break;
        }
        W8OctRegionVolume* volume = &spatial_000.owned_5c[region_index];
        if (PointInsideFrustum0046D880(location, volume->planes_88) == 0) {
            continue;
        }
        if (reported_region == 0 && region_index != g_octree_region_debug_last_006598b0) {
            sprintf(expected_regions, "Point in region %d, expected regions: %s\n", region_index,
                    point_region_label);
            NoOp();
            g_octree_region_debug_last_006598b0 = region_index;
            reported_region = region_index;
        }
        m_projected_regions_15c->ClearAll();
        m_projected_regions_valid_16a = 0;
        match_count = 1;
        unsigned int key = region_index;
        for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1); slot != -1;
             slot = m_pRegionLinks_150->FindNextEntry(&key, slot)) {
            m_projected_regions_15c->Set(m_pRegionLinks_150->entries[slot].value);
            m_projected_regions_valid_16a = 1;
        }
    }

    if (match_count == 0) {
        return 0;
    }
    m_positional_16b = 1;
    return match_count;
}

// FUNCTION: WIZ8 0x00430BF0
unsigned int W8Octree::GetSectorForPosition(const srVector3T<float>* position)
{
    srVector3T<float> point = *position;
    for (unsigned int region = 1; region < spatial_000.region_count_46; ++region) {
        if (PointInsideFrustum0046D880(&point, spatial_000.owned_5c[region].planes_88) != 0) {
            return spatial_000.owned_5c[region].region_bit_0c;
        }
    }

    unsigned long levels = m_region_mask_140;
    int cell[3];
    for (int axis = 0; axis < 3; ++axis) {
        int coordinate =
            static_cast<int>(((&position->x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                             spatial_000.region_grid_cell_54);
        if (coordinate < 0 || coordinate >= spatial_000.region_cells_per_axis_50) {
            return 0;
        }
        cell[axis] = coordinate;
    }

    unsigned long node = 1;
    for (int mask = 1 << spatial_000.depth_44; mask != 0; mask /= 2) {
        if (node == 0) {
            return 0;
        }
        if ((levels & mask) != 0) {
            int child = 0;
            if ((cell[0] & mask) != 0) {
                child = 4;
            }
            if ((cell[1] & mask) != 0) {
                child += 2;
            }
            if ((cell[2] & mask) != 0) {
                ++child;
            }
            node = m_owned_09c[node].children_04[child];
        }
    }
    if (node == 0) {
        return 0;
    }
    return m_owned_09c[node].region_02;
}

// FUNCTION: WIZ8 0x00430d50
unsigned char W8Octree::CollectVisibleRegions00430D50(srVector3T<float>* location, int* cells,
                                                      float* depth, unsigned char mode)
{
    int link_cell[3];
    float box_min[3];
    float box_max[3];
    unsigned char inside = 1;

    for (int axis = 0; axis < 3; ++axis) {
        box_min[axis] = (&spatial_000.minimum_0c.x)[axis];
        box_max[axis] = (&spatial_000.maximum_18.x)[axis];
        cells[axis] =
            static_cast<int>((((&location->x)[axis] - box_min[axis]) / spatial_000.node_extent_70));
        link_cell[axis] = static_cast<int>(
            (((&location->x)[axis] - box_min[axis]) / spatial_000.region_grid_cell_54));
        if ((&location->x)[axis] < box_min[axis]) {
            --cells[axis];
            inside = 0;
        }
        if ((&location->x)[axis] > box_max[axis]) {
            inside = 0;
        }
        if (depth != 0) {
            depth[axis] =
                ((&location->x)[axis] -
                 (static_cast<float>(cells[axis]) * spatial_000.node_extent_70 + box_min[axis])) /
                    spatial_000.node_extent_70 -
                g_float_005ebc7c;
        }
    }
    if (!inside) {
        return 0;
    }
    if (mode == 0) {
        return 1;
    }
    float span = spatial_000.extent_04 * g_float_005ebc7c;
    int node = 1;
    for (short level = 0; level < spatial_000.depth_44; ++level) {
        unsigned int child_index = 0;
        for (int axis = 0; axis < 3; ++axis) {
            float edge = span + box_min[axis];
            if ((&location->x)[axis] < edge) {
                box_max[axis] = edge;
            } else {
                box_min[axis] = edge;
                child_index |= 1 << (2 - axis);
            }
        }
        if (level == spatial_000.leaf_level_52) {
            unsigned int key =
                ((m_region_mask_140 * 0x100 + link_cell[0]) * 0x100 + link_cell[1]) * 0x100 +
                link_cell[2];
            int slot = m_pRegionLinks_150->FindNextEntry(&key, -1);
            while (slot != -1) {
                m_projected_regions_15c->Set(m_pRegionLinks_150->entries[slot].value);
                slot = m_pRegionLinks_150->FindNextEntry(&key, slot);
                m_projected_regions_valid_16a = 1;
            }
        }
        if (node != 0) {
            node = static_cast<int>(m_owned_09c[node].children_04[child_index]);
        }
        span *= g_float_005ebc7c;
    }
    if (node != 0) {
        unsigned long region_offset = m_owned_0a0[node].region_offset_04;
        if (region_offset != 0) {
            ProjectLinkedRegionsForLocation00431050(location, m_owned_148 + region_offset);
            return 1;
        }
    }
    ProjectLinkedRegionsForLocation00431050(location, 0);
    return 1;
}

/* Project each candidate region volume against the camera frustum and add the
   visible ones to the current region set. The first corner is tested against
   the far-clip sphere before the eight remaining corners are projected. */
// FUNCTION: WIZ8 0x004301c0
void W8Octree::MarkVisibleRegions004301C0()
{
    float radius = far_clip_200;
    float radius_squared = radius * radius;

    for (int index = 1; index < spatial_000.region_count_46; ++index) {
        W8OctRegionVolume* volume = &spatial_000.owned_5c[index];

        if (m_projected_regions_valid_16a != 0 &&
            !m_projected_regions_15c->Test(volume->region_bit_0c)) {
            continue;
        }
        srVector3T<float> delta = camera_location_1c0 - volume->points_1c[0];

        if (delta.LengthSquared() >= radius_squared) {
            continue;
        }
        unsigned char visible =
            PointInsideFrustum0046D880(&volume->points_1c[0], m_frustum_planes_21c);

        for (int point = 1; !visible && point < 9; ++point) {
            visible = PointInsideFrustum0046D880(&volume->points_1c[point], m_frustum_planes_21c);
        }
        if (visible != 0 && volume->region_bit_0c != 0) {
            m_current_regions_160->Set(volume->region_bit_0c);
        }
    }
}

/* Build the four side frustum planes from the camera basis and far clip, and
   accumulate the two far-plane offsets the region projection reads. */
// FUNCTION: WIZ8 0x004302e0
void W8Octree::BuildFrustumPlanes004302E0()
{
    float fov = horizontal_fov_1f0 * g_float_005ebc7c;
    float extent = spatial_000.max_region_radius_60;
    float far_clip = far_clip_200;
    float sine = static_cast<float>(sin(fov));
    float cosine = static_cast<float>(cos(fov));
    float ratio = extent / cosine;
    float tangent = static_cast<float>(tan(fov));
    float tangent_vertical = static_cast<float>(tan(vertical_fov_1f4 * g_float_005ebc7c));
    srVector3T<float> corners[8];

    m_frustum_planes_21c[4].w = 0.0f;
    m_frustum_planes_21c[5].w = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        float dof = (&camera_dof_1cc.x)[axis];
        float column1 = (&rotation_column_1d8.x)[axis];
        float column2 = (&rotation_column_1e4.x)[axis];
        float w = far_clip * dof;
        float a = (tangent * far_clip + ratio) * column1;
        float b = (tangent_vertical * far_clip + ratio) * column2;

        (&corners[0].x)[axis] = (&camera_location_1c0.x)[axis] - (extent / sine) * dof;
        (&corners[1].x)[axis] = a;
        (&corners[2].x)[axis] = b;
        (&corners[3].x)[axis] = w;
        (&corners[4].x)[axis] = (w - a) + b;
        (&corners[5].x)[axis] = (w + b) + a;
        (&corners[6].x)[axis] = (w - a) - b;
        (&corners[7].x)[axis] = (w + a) - b;
        (&corners[4].x)[axis] += (&camera_location_1c0.x)[axis];
        (&corners[5].x)[axis] += (&camera_location_1c0.x)[axis];
        (&corners[6].x)[axis] += (&camera_location_1c0.x)[axis];
        (&corners[7].x)[axis] += (&camera_location_1c0.x)[axis];
        (&m_frustum_planes_21c[4].x)[axis] = dof;
        m_frustum_planes_21c[4].w -= dof * (&corners[0].x)[axis];
        (&m_frustum_planes_21c[5].x)[axis] = -dof;
        m_frustum_planes_21c[5].w -= -dof * (&corners[4].x)[axis];
    }
    BuildPlaneFromPoints0046D660(&m_frustum_planes_21c[0], &corners[0], &corners[5], &corners[4]);
    BuildPlaneFromPoints0046D660(&m_frustum_planes_21c[1], &corners[0], &corners[4], &corners[6]);
    BuildPlaneFromPoints0046D660(&m_frustum_planes_21c[2], &corners[0], &corners[7], &corners[5]);
    BuildPlaneFromPoints0046D660(&m_frustum_planes_21c[3], &corners[0], &corners[6], &corners[7]);
    for (int index = 0; index < 6; ++index) {
        m_frustum_planes_21c[index].w += spatial_000.region_grid_cell_54;
    }
}

/* Collect the cells around the camera into the current region set.

   Two setup helpers refresh the frame state. The leaf cell radius comes from
   the far clip over the cell size, and the camera cell is quantized against
   the same size. Every cell within that radius of the camera cell that stays
   inside the spatial extent is considered: the near cells descend the branch
   array directly, while the far cells filter through
   PointInsideFrustum0046D880 first. Either way the reached node contributes
   the region stored at its branch head. */
// FUNCTION: WIZ8 0x0042fe90
void W8Octree::CollectVisibleCells0042FE90()
{
    BuildFrustumPlanes004302E0();
    MarkVisibleRegions004301C0();
    short radius = static_cast<short>(
        (static_cast<int>((far_clip_200 / spatial_000.region_grid_cell_54)) + 1));
    short center[3];

    for (int axis = 0; axis < 3; ++axis) {
        center[axis] = static_cast<short>(
            static_cast<int>((((&camera_location_1c0.x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                              spatial_000.region_grid_cell_54)));
    }
    unsigned int region_base = m_region_mask_140;
    for (short x = -radius; x <= radius; ++x) {
        short cell_x = center[0] + x;
        if (cell_x < 0 || cell_x >= spatial_000.region_cells_per_axis_50) {
            continue;
        }
        for (short y = -radius; y <= radius; ++y) {
            short cell_y = center[1] + y;
            if (cell_y < 0 || cell_y >= spatial_000.region_cells_per_axis_50) {
                continue;
            }
            for (short z = -radius; z <= radius; ++z) {
                short cell_z = center[2] + z;
                if (cell_z < 0 || cell_z >= spatial_000.region_cells_per_axis_50) {
                    continue;
                }
                if (abs(x) < 2 && abs(y) < 2 && abs(z) < 2) {
                    int node = 1;
                    for (unsigned int mask = 1 << spatial_000.depth_44; mask != 0; mask >>= 1) {
                        if (node == 0) {
                            break;
                        }
                        if ((region_base & mask) != 0) {
                            int child = 0;
                            if (cell_x & mask) {
                                child = 4;
                            }
                            if (cell_y & mask) {
                                child += 2;
                            }
                            if (cell_z & mask) {
                                ++child;
                            }
                            node = static_cast<int>(m_owned_09c[node].children_04[child]);
                        }
                    }
                    if (node != 0) {
                        unsigned short region = m_owned_09c[node].region_02;
                        if (region != 0) {
                            m_current_regions_160->Set(region);
                        }
                    }
                } else {
                    float offset = spatial_000.region_grid_cell_54 * g_float_005ebc7c;
                    srVector3T<float> point;
                    point.x = static_cast<float>(cell_x) * spatial_000.region_grid_cell_54 +
                              offset + spatial_000.minimum_0c.x;
                    point.y = static_cast<float>(cell_y) * spatial_000.region_grid_cell_54 +
                              spatial_000.minimum_0c.y + offset;
                    point.z = static_cast<float>(cell_z) * spatial_000.region_grid_cell_54 +
                              spatial_000.minimum_0c.z + offset;
                    if (PointInsideFrustum0046D880(&point, m_frustum_planes_21c) == 0) {
                        continue;
                    }
                    int node = 1;
                    for (unsigned int mask = 1 << spatial_000.depth_44; mask != 0; mask >>= 1) {
                        if (node == 0) {
                            break;
                        }
                        if ((region_base & mask) != 0) {
                            int child = 0;
                            if (cell_x & mask) {
                                child = 4;
                            }
                            if (cell_y & mask) {
                                child += 2;
                            }
                            if (cell_z & mask) {
                                ++child;
                            }
                            node = static_cast<int>(m_owned_09c[node].children_04[child]);
                        }
                    }
                    if (node != 0) {
                        unsigned short region = m_owned_09c[node].region_02;
                        if (region != 0) {
                            m_current_regions_160->Set(region);
                        }
                    }
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x004329a0
unsigned char FindNextLevelFile(char* name)
{
    if (name == 0) {
        g_octree_file_search_active_006598b2 = 0;
        return 0;
    }

    if (g_octree_file_search_active_006598b2 == 0) {
        char pattern[256];
        char extension[52];

        strcpy(pattern, name);
        char* extension_start = strrchr(pattern, '.');
        extension[0] = '\0';
        if (extension_start != 0) {
            strcpy(extension, extension_start);
            *extension_start = '\0';
        }
        strcat(pattern, g_octree_file_search_wildcard_006068a0);
        strcat(pattern, extension);
        g_octree_file_search_active_006598b2 =
            GetFileFirst(pattern, &g_octree_file_search_00659778);
    } else {
        g_octree_file_search_active_006598b2 = GetFileNext(&g_octree_file_search_00659778);
    }
    if (g_octree_file_search_active_006598b2 == 0) {
        GetFileClose(&g_octree_file_search_00659778);
        return 0;
    }

    char* separator = strrchr(name, '\\');
    if (separator != 0) {
        separator[1] = '\0';
    }
    strcat(name, g_octree_file_search_00659778.zFileName);
    return g_octree_file_search_active_006598b2;
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
    strcat(name, g_octree_point_extension_00606810);

    FindNextLevelFile(0);
    unsigned char first = 1;
    unsigned char read_ok = 0;
    while (FindNextLevelFile(name) != 0) {
        if (first != 0) {
            first = 0;
        } else {
            m_positional_16d = 1;
        }
        int file = FileOpen(name, 1, 0);
        if (file == 0) {
            return 0;
        }
        if (FileRead(file, &m_positional_170, 4, 0) == 0) {
            FileClose(file);
            return 0;
        }
        m_sr_owned_174 =
            static_cast<srVector3T<float>*>(srHeap.allocate((m_positional_170 + 1) * 0xc));
        if (m_sr_owned_174 == 0) {
            FileClose(file);
            return 0;
        }
        read_ok = FileRead(file, m_sr_owned_174, m_positional_170 * 0xc, 0);
        FileClose(file);
    }
    if (read_ok != 0) {
        return read_ok;
    }
    srHeap.free(m_sr_owned_174);
    m_positional_170 = 0;
    return 0;
}

/* Write the octree's point array to a companion file.

   The level path supplies the base name and its existing extension is
   replaced with the point-file extension. A read-only file is made writable
   first. The count precedes the records, and the result reports either
   write. */
// FUNCTION: WIZ8 0x00432d60
bool W8Octree::SavePoints00432D60(char* path)
{
    char name[256];
    unsigned char result = 0;

    strcpy(name, path);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, ".pts");
    if (FileExists(name) != 0) {
        if (_access(name, 2) != 0) {
            _chmod(name, 0x180);
        }
    }
    int file = FileOpen(name, 2, 0);
    if (file != 0) {
        if (m_positional_170 != 0 && m_sr_owned_174 != 0) {
            unsigned char wrote_count = FileWrite(file, &m_positional_170, 4, 0);
            unsigned char wrote_points = FileWrite(file, m_sr_owned_174, m_positional_170 * 0xc, 0);
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
    unsigned char result = 0;
    char name[256];

    strcpy(name, level_name);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, g_region_link_extension_006068a4);
    if (FileExists(name) == 0) {
        return 0;
    }
    file = FileOpen(name, 1, 0);
    if (file == 0) {
        return 0;
    }
    if (FileRead(file, &count, 4, 0) == 0) {
        FileClose(file);
        return 0;
    }

    keys = static_cast<unsigned int*>(malloc(count * 4));
    values = static_cast<unsigned short*>(malloc(count * 2));
    if (keys == 0 || values == 0) {
        FileClose(file);
        free(keys);
        free(values);
        return 0;
    }
    if (FileRead(file, keys, count * 4, 0) == 0 || FileRead(file, values, count * 2, 0) == 0) {
        FileClose(file);
        free(keys);
        free(values);
        return 0;
    }
    if (m_pRegionLinks_150 == 0) {
        m_pRegionLinks_150 = new W8HashTable<unsigned int, unsigned short>;
    }
    for (unsigned int index = 0; index < count; ++index) {
        m_pRegionLinks_150->Remove(&keys[index], &values[index]);
        m_pRegionLinks_150->Insert(&keys[index], &values[index]);
    }
    result = 1;
    FileClose(file);
    free(keys);
    free(values);
    if (result != 0) {
        m_positional_169 = 1;
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
    if (1 < spatial_000.region_count_46) {
        unsigned int index = 1;
        do {
            if (region != 0) {
                return region;
            }
            if (spatial_000.owned_5c[index].ContainsPoint0049E460(point) != 0) {
                region = index;
            }
            ++index;
        } while (index < spatial_000.region_count_46);
        if (region != 0) {
            return region;
        }
    }
    int cell[3];
    for (int axis = 0; axis < 3; ++axis) {
        cell[axis] = static_cast<int>(((&point->x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                      spatial_000.region_grid_cell_54);
    }
    return (cell[0] * 0x100 + cell[1]) * 0x100 + cell[2] + m_region_mask_140 * 0x1000000;
}

/* Record `region_key` against every mesh marked in m_projected_regions_15c:
   unseen (region, mesh) pairs are inserted into m_pRegionLinks_150 and the
   link list is logged once per call. */
// FUNCTION: WIZ8 0x004327f0
void W8Octree::RecordRegionMeshLinks(unsigned int region_key)
{
    bool reported = false;
    if (region_key == 0) {
        return;
    }
    int bit = m_projected_regions_15c->NextSetBit(1);
    if (bit == 0) {
        return;
    }
    char text[256];
    do {
        unsigned short mesh = static_cast<unsigned short>(bit - 1);
        int slot = m_pRegionLinks_150->FindNextEntry(&region_key, -1);
        while (slot != -1 && m_pRegionLinks_150->entries[slot].value != mesh) {
            slot = m_pRegionLinks_150->FindNextEntry(&region_key, slot);
        }
        if (slot == -1) {
            if (!reported) {
                if (region_key < spatial_000.region_count_46) {
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
            m_pRegionLinks_150->Insert(&region_key, &mesh);
        }
        bit = m_projected_regions_15c->NextSetBit(0);
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
unsigned int W8Octree::SampleRegionLinks(const srVector3T<float>* point, char descend,
                                         char clear_sets, unsigned int region_key)
{
    bool linked = false;
    W8World* world = GetWorld();
    W8GrowableVector<stModelInstance*> meshes(5);
    W8GrowableVector<int> cells;
    camera_location_1c0 = *point;
    far_clip_200 = spatial_000.extent_04;
    char text[104];
    sprintf(text, "Sample point: %f, %f, %f, Links Found: ", static_cast<double>(point->x),
            static_cast<double>(point->y), static_cast<double>(point->z));
    NoOp();
    srVector3T<double> location;
    location.x = point->x;
    location.y = point->y;
    location.z = point->z;
    world->camera->setLocation(location);
    horizontal_fov_1f0 = static_cast<float>(world->camera->getHorizontalFOV());
    vertical_fov_1f4 = static_cast<float>(world->camera->getVerticalFOV());
    if (region_key == 0) {
        region_key = RegionKeyForPoint(point);
    }
    unsigned int key = region_key;
    if (clear_sets != 0) {
        m_projected_regions_15c->ClearAll();
        m_previous_regions_164->ClearAll();
    }
    m_projected_regions_valid_16a = 0;
    short samples = static_cast<short>(static_cast<int>(
        g_camera_angle_period_005ec014 / horizontal_fov_1f0 + g_camera_snap_epsilon_005ebc2c));
    if (static_cast<float>(samples) * horizontal_fov_1f0 < g_float_005ec010) {
        ++samples;
    }
    if (samples > 0) {
        double cos_tilt = cos(g_double_005ec008);
        double sin_tilt = sin(g_double_005ec008);
        for (int direction = 0; direction < samples; ++direction) {
            float angle = static_cast<float>(direction) * horizontal_fov_1f0;
            srMatrix3T<float> frame;
            frame.vectors[0].x = 1.0f;
            frame.vectors[0].y = 0.0f;
            frame.vectors[0].z = 0.0f;
            frame.vectors[1].Set(0.0, 1.0, 0.0);
            frame.vectors[2].Set(0.0, 0.0, 1.0);
            if (angle != g_zero_005ebb40) {
                frame.RotateAboutY(sin(static_cast<double>(angle)),
                                   cos(static_cast<double>(angle)));
            }
            srVector3T<float> tilt_first;
            srVector3T<float> tilt_second;
            srVector3T<float> tilt_third;
            tilt_third.Set(0.0, sin_tilt, cos_tilt);
            tilt_second.Set(0.0, cos_tilt, -sin_tilt);
            tilt_first.Set(1.0, 0.0, 0.0);
            srMatrix3T<float> tilt;
            tilt.SetRows(tilt_first, tilt_second, tilt_third);
            frame.MultiplyBy(tilt);
            world->camera->setRotation(frame);
            rotation_column_1d8.x = frame.vectors[0].x;
            srVector3T<float> unit;
            unit.Set(1.0, 0.0, 0.0);
            rotation_column_1d8.y = DotProduct(frame.vectors[1], unit);
            rotation_column_1d8.z = DotProduct(frame.vectors[2], unit);
            float right_y = frame.vectors[0].y;
            unit.Set(0.0, 1.0, 0.0);
            rotation_column_1e4.x = right_y;
            rotation_column_1e4.y = DotProduct(frame.vectors[1], unit);
            rotation_column_1e4.z = DotProduct(frame.vectors[2], unit);
            srVector3T<double> dof = world->camera->getWorldSpaceDOF();
            camera_dof_1cc = dof;
            m_current_regions_160->ClearAll();
            CollectVisibleCells0042FE90();
            int bit = m_current_regions_160->NextSetBit(1);
            bool all_known = true;
            if (bit != 0) {
                do {
                    unsigned int cell = static_cast<unsigned int>(bit - 1);
                    meshes.Add(static_cast<stModelInstance*>(
                        g_world->psrMeshes[m_pSubmeshes[cell].mesh_04]));
                    if (!m_projected_regions_15c->Test(cell)) {
                        int slot = m_pRegionLinks_150->FindNextEntry(&key, -1);
                        while (slot != -1 && m_pRegionLinks_150->entries[slot].value !=
                                                 static_cast<unsigned short>(cell)) {
                            slot = m_pRegionLinks_150->FindNextEntry(&key, slot);
                        }
                        if (slot == -1) {
                            if (cell != 0) {
                                m_pSubmeshes[cell].flags_00 |= 0x28;
                                srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh_04];
                                if (node != 0) {
                                    node->clearFlag(srNode::FLAG_DISABLE);
                                    if (descend != 0) {
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
                            m_pSubmeshes[cell].flags_00 &= 0xfffffff7;
                            srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh_04];
                            if (node != 0) {
                                node->setFlag(srNode::FLAG_DISABLE);
                                node->setFlag(srNode::FLAG_TERMINATE);
                            }
                        }
                        cells.Add(0);
                    }
                    bit = m_current_regions_160->NextSetBit(0);
                } while (bit != 0);
                if (!all_known) {
                    BeginRenderProbe00428910();
                    for (int i = 0; i < meshes.GetCount(); ++i) {
                        stModelInstance* mesh = *meshes.GetAt(i);
                        if (mesh != 0) {
                            unsigned int faces = MeasureNodeRender004289E0(mesh);
                            all_known = 9 < faces;
                            srNode* child = mesh->firstChild();
                            while (child != 0 && descend != 0) {
                                faces = MeasureNodeRender004289E0(child);
                                if (9 < faces) {
                                    all_known = true;
                                }
                                child = child->firstChild();
                            }
                            if (!all_known) {
                                unsigned int candidate = *cells.GetAt(i);
                                if (candidate != 0) {
                                    m_pSubmeshes[candidate].flags_00 &= 0xfffffff7;
                                    srNode* node =
                                        g_world->psrMeshes[m_pSubmeshes[candidate].mesh_04];
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
                                unsigned int faces = MeasureNodeRender004289E0(mesh);
                                if (9 < faces) {
                                    sprintf(text, "%d, ", cell);
                                    NoOp();
                                    m_projected_regions_15c->Set(cell);
                                    linked = true;
                                }
                                srNode* child = mesh->firstChild();
                                while (child != 0 && descend != 0) {
                                    faces = MeasureNodeRender004289E0(child);
                                    if (9 < faces) {
                                        m_projected_regions_15c->Set(cell);
                                        linked = true;
                                    }
                                    child = child->firstChild();
                                }
                                m_pSubmeshes[cell].flags_00 &= 0xfffffff7;
                                srNode* node = g_world->psrMeshes[m_pSubmeshes[cell].mesh_04];
                                if (node != 0) {
                                    node->setFlag(srNode::FLAG_DISABLE);
                                    node->setFlag(srNode::FLAG_TERMINATE);
                                }
                            }
                        }
                    }
                    EndRenderProbe004289C0();
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
void W8Octree::BuildRegionLinks(char rebuild_all)
{
    W8World* world = GetWorld();
    bool aborted = false;
    unsigned int last_key = 0;
    time_t started = time(0);
    W8HashTable<unsigned int, unsigned short>* links =
        new W8HashTable<unsigned int, unsigned short>;
    unsigned short z_step = rebuild_all != 0 ? 1 : 3;
    float stride = static_cast<float>(z_step) * m_region_cell_178;
    short rows = static_cast<short>(static_cast<int>(spatial_000.extent_04 / stride));
    for (unsigned int prop = 0; prop < m_usNumPropsLoaded; ++prop) {
        m_papProps[prop]->SetSetting6C(0);
    }
    for (unsigned int particle = 0; particle < m_usNumParticlesLoaded; ++particle) {
        m_papParticles[particle]->SetTraversalEnabled00498D90(0);
    }
    SetViewportMode(0);
    SetScaledViewport00425DA0(0, 0, 0x140, 0xf0);
    srVector3T<float> saved_location;
    srMatrix3T<float> saved_rotation;
    world->camera->getLocation(saved_location);
    world->camera->getRotation(saved_rotation);
    g_light_update_flags_0060bfdc &= ~1u;
    g_flag_0065a146 = 1;
    SetCameraLightMode00483E80(2);
    g_flag_0065a0ec = 1;
    g_flag_0065a0ed = 1;
    double saved_width = world->camera->getHorizontalFOV();
    double saved_height = world->camera->getVerticalFOV();
    world->camera->setViewPlane(2.094395102, 2.094395102);
    DisableAllRenderOptions0047B5D0();
    for (unsigned int mesh = 0; mesh < m_meshCount_1b4; ++mesh) {
        srNode* node = g_world->psrMeshes[mesh];
        if (node != 0) {
            node->setFlag(srNode::FLAG_DISABLE);
            node->setFlag(srNode::FLAG_TERMINATE);
        }
    }
    links->Clear();
    W8HashTable<unsigned int, unsigned short>* saved_links = m_pRegionLinks_150;
    m_pRegionLinks_150 = links;
    if (rows != 0) {
        for (unsigned short x_index = 0; x_index < static_cast<unsigned short>(rows); ++x_index) {
            if (aborted) {
                break;
            }
            srVector3T<float> point;
            point.x = m_region_cell_178 * g_float_005ebc7c + static_cast<float>(x_index) * stride +
                      spatial_000.minimum_0c.x;
            unsigned short z_index = rebuild_all == 0 && (x_index & 1) != 0 ? z_step : 0;
            if (!(spatial_000.working_minimum_78.x < point.x &&
                  point.x < spatial_000.working_maximum_84.x)) {
                continue;
            }
            for (; z_index < static_cast<unsigned short>(rows); z_index += z_step) {
                if (aborted) {
                    break;
                }
                point.z = m_region_cell_178 * g_float_005ebc7c +
                          static_cast<float>(z_index) * stride + spatial_000.minimum_0c.z;
                if (!(spatial_000.working_minimum_78.z < point.z &&
                      point.z < spatial_000.working_maximum_84.z)) {
                    continue;
                }
                MSG message;
                if (PeekMessageA(&message, 0, 0, 0, 0) != 0 &&
                    GetMessageA(&message, 0, 0, 0) != 0) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                    InputAtom input;
                    if (DequeueEvent(&input) != 0 && input.usEvent == KEY_DOWN) {
                        if (input.usParam == 0xd) {
                            x_index += 2;
                            z_index = 0;
                        } else if (input.usParam == 0x1b) {
                            aborted = true;
                            if (g_build_level_links_0065bd2c != 0) {
                                g_build_level_links_0065bd2c = 0;
                            }
                            continue;
                        }
                    }
                }
                point.y = spatial_000.minimum_0c.y + spatial_000.extent_04;
                while (pathing_180->SnapToLowerPathCell00463290(&point, 1) != 0) {
                    point.y += g_default_world_height_00603ac8;
                    unsigned int key = RegionKeyForPoint(&point);
                    if (key != last_key) {
                        if (last_key != 0) {
                            RecordRegionMeshLinks(last_key);
                        }
                        m_projected_regions_15c->ClearAll();
                        m_previous_regions_164->ClearAll();
                        last_key = key;
                    }
                    SampleRegionLinks(&point, 0, 0, key);
                    point.y -= g_default_world_height_00603ac8 + g_startup_near_limit_005ec000;
                }
            }
        }
    }
    g_flag_0065a146 = 0;
    EnableAllRenderOptions();
    W8HashTable<unsigned int, unsigned short>* stale;
    if (rebuild_all == 0) {
        if (!aborted) {
            for (unsigned int point_index = 0; point_index < m_positional_170; ++point_index) {
                unsigned int key = SampleRegionLinks(&m_sr_owned_174[point_index], 1, 1, 0);
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
        strcat(point_path, g_octree_point_extension_00606810);
        if (FileExists(point_path)) {
            if (_access(point_path, 2) != 0) {
                _chmod(point_path, 0x180);
            }
            DeleteFileA(point_path);
        }
        srHeap.free(m_sr_owned_174);
        m_sr_owned_174 = 0;
        m_positional_170 = 0;
    }
    world->camera->setViewPlane(saved_width, saved_height);
    unsigned int camera_count = PLLength(world->plsCameras);
    if (!aborted && world->plsCameras != 0 && camera_count != 0) {
        for (int camera_index = 0; camera_index < static_cast<int>(camera_count); ++camera_index) {
            W8WorldCameraEntry* entry =
                static_cast<W8WorldCameraEntry*>(PLGet(world->plsCameras, camera_index));
            if (entry != 0 && entry->path != 0) {
                for (int node_index = 0; node_index < entry->path->nodes_0c->GetCount();
                     ++node_index) {
                    unsigned int key =
                        SampleRegionLinks(*entry->path->nodes_0c->GetAt(node_index), 1, 1, 0);
                    RecordRegionMeshLinks(key);
                }
            }
        }
    }
    srVector3T<double> restore_location;
    restore_location.x = saved_location.x;
    restore_location.y = saved_location.y;
    restore_location.z = saved_location.z;
    world->camera->setLocation(restore_location);
    world->camera->setRotation(saved_rotation);
    g_light_update_flags_0060bfdc |= 1u;
    SetCameraLightMode00483E80(3);
    g_flag_0065a0ec = 0;
    g_flag_0065a0ed = 0;
    SetScaledViewport00425DA0(0, 0, 0x280, 0x1e0);
    m_positional_16c = 1;
    m_positional_169 = 1;
    m_reset_visibility_168 = 1;
    unsigned int elapsed = static_cast<unsigned int>(static_cast<int>(difftime(time(0), started)));
    unsigned int minutes = elapsed / 60;
    unsigned int hours = 0;
    if (minutes > 0x3b) {
        hours = minutes / 60;
        minutes %= 60;
    }
    if (aborted) {
        stale = m_pRegionLinks_150;
        m_pRegionLinks_150 = saved_links;
        CreateMessageBox(FormatWideString(L"  Linking Aborted!  "), g_small_font_683678, 1, 1, 0,
                         0);
    } else {
        SaveRegionLinks004331F0(m_owned_0c0);
        if (hours == 0) {
            CreateMessageBox(
                FormatWideString(L"  Linking Time: %d Min, %d Sec  ", minutes, elapsed % 60),
                g_small_font_683678, 1, 1, 0, 0);
        } else {
            CreateMessageBox(FormatWideString(L"  Linking Time: %d Hours, %d Min, %d Sec  ", hours,
                                              minutes, elapsed % 60),
                             g_small_font_683678, 1, 1, 0, 0);
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
bool W8Octree::SaveRegionLinks004331F0(char* path)
{
    unsigned char result = 1;
    unsigned int* keys = 0;
    unsigned short* values = 0;
    int file = 0;

    if (m_pRegionLinks_150 == 0) {
        return 0;
    }
    if (path == 0) {
        return 0;
    }
    unsigned int capacity = m_pRegionLinks_150->bucket_count;
    if (capacity == 0) {
        return 0;
    }
    char name[256];
    strcpy(name, path);
    char* extension = strrchr(name, '.');
    if (extension != 0) {
        *extension = '\0';
    }
    strcat(name, ".rlk");
    if (FileExists(name) != 0) {
        if (_access(name, 2) != 0) {
            _chmod(name, 0x180);
        }
    }
    file = FileOpen(name, 2, 0);
    if (file == 0) {
        goto cleanup;
    }
    keys = static_cast<unsigned int*>(malloc(capacity * 4));
    values = static_cast<unsigned short*>(malloc(capacity * 2));
    if (keys == 0 || values == 0) {
        result = 0;
        goto cleanup;
    }
    {
        unsigned int count = 0;
        for (unsigned int key = 1; key < spatial_000.region_count_46; ++key) {
            for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1); slot != -1;
                 slot = m_pRegionLinks_150->FindNextEntry(&key, slot)) {
                keys[count] = key;
                values[count] = m_pRegionLinks_150->entries[slot].value;
                ++count;
            }
        }
        unsigned int extent = 1 << spatial_000.leaf_level_52;
        unsigned int base = m_region_mask_140 * 0x1000000;
        for (unsigned int x = 0; x < extent; ++x) {
            for (unsigned int y = 0; y < extent; ++y) {
                for (unsigned int z = 0; z < extent; ++z) {
                    unsigned int key = (x << 16) + (y << 8) + z + base;
                    for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1); slot != -1;
                         slot = m_pRegionLinks_150->FindNextEntry(&key, slot)) {
                        keys[count] = key;
                        values[count] = m_pRegionLinks_150->entries[slot].value;
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
unsigned char W8Octree::UpdateWorldTrace00433EB0()
{
    srVector3T<float> camera;
    int cell[3];
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    unsigned long color;

    GetCameraPosition(&camera);
    for (int axis = 0; axis < 3; ++axis) {
        cell[axis] = static_cast<int>(
            (((&camera.x)[axis] - (&spatial_000.minimum_0c.x)[axis]) / spatial_000.node_extent_70));
    }
    minimum.x = static_cast<float>(cell[0]) * spatial_000.node_extent_70 + spatial_000.minimum_0c.x;
    minimum.y = static_cast<float>(cell[1]) * spatial_000.node_extent_70 + spatial_000.minimum_0c.y;
    minimum.z = static_cast<float>(cell[2]) * spatial_000.node_extent_70 + spatial_000.minimum_0c.z;
    maximum.x = minimum.x + spatial_000.node_extent_70;
    maximum.y = minimum.y + spatial_000.node_extent_70;
    maximum.z = minimum.z + spatial_000.node_extent_70;
    unsigned long* packed = PackColour00433FB0(&color, 0.0, 1.0, 0.0, 0.0);
    DrawWorldBox0048DF30(g_world, minimum, maximum, *packed);
    return 1;
}

// GLOBAL: WIZ8 0x005ebf60
const double g_double_005ebf60 = 255.0;

/* Pack four filtered colour components into the caller's unsigned long: red
   lands in the top byte and alpha in the low one. */
// FUNCTION: WIZ8 0x00433fb0
unsigned long* __fastcall PackColour00433FB0(unsigned long* color, double red, double green,
                                             double blue, double alpha)
{
    unsigned char* bytes =
        reinterpret_cast<unsigned char*>(color); // reinterpret-ok: packed colour storage
    bytes[3] = static_cast<unsigned char>((red * g_double_005ebf60));
    bytes[2] = static_cast<unsigned char>((green * g_double_005ebf60));
    bytes[1] = static_cast<unsigned char>((blue * g_double_005ebf60));
    bytes[0] = static_cast<unsigned char>((alpha * g_double_005ebf60));
    return color;
}

/* Validate the current octree's region-to-mesh links against a scratch copy
   of the spatial state. The scratch copy is flattened to the leaf level with
   all region bounds enabled, and every reported bad link is posted as one
   notice. */
// FUNCTION: WIZ8 0x00433ab0
unsigned char W8Octree::ValidateRegionMeshLinks00433AB0()
{
    W8OctSpatialState spatial(&spatial_000);
    spatial.depth_44 = 0;
    spatial.level_kind_6c = 1;
    spatial.positional_94 = 1;
    int bad_links = CountBadRegionMeshLinks00433B90(&spatial);
    if (bad_links != 0) {
        CreateMessageBox(FormatWideString(L" %d Bad Region-Mesh Links!", bad_links),
                         g_small_font_683678, 1, 1, 0, 0);
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
int W8Octree::CountBadRegionMeshLinks00433B90(W8OctSpatialState* spatial)
{
    W8OctSpatialState local(spatial);
    int bad_links = 0;
    if (spatial->depth_44 < 0x10) {
        if (spatial->depth_44 == spatial_000.leaf_level_52) {
            unsigned short link = m_owned_09c[spatial->positional_94].region_02;
            if (link != 0) {
                stModelInstance* mesh =
                    static_cast<stModelInstance*>(g_world->psrMeshes[m_pSubmeshes[link].mesh_04]);
                if (mesh != 0) {
                    stMeshModel* model = static_cast<stMeshModel*>(mesh->model());
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
                    if (maximum.x < spatial->minimum_0c.x || spatial->maximum_18.x < minimum.x ||
                        maximum.y < spatial->minimum_0c.y || spatial->maximum_18.y < minimum.y ||
                        maximum.z < spatial->minimum_0c.z || spatial->maximum_18.z < minimum.z) {
                        bad_links = 1;
                    }
                }
            }
        } else {
            int x = 0;
            short child = 0;
            do {
                for (int y = 0; y < 2; ++y) {
                    for (int z = 0; z < 2; ++z) {
                        if (m_owned_09c[spatial->positional_94].children_04[child] != 0) {
                            local.minimum_0c.x =
                                static_cast<float>(x) * local.extent_04 + spatial->minimum_0c.x;
                            local.maximum_18.x = local.minimum_0c.x + local.extent_04;
                            local.minimum_0c.y =
                                static_cast<float>(y) * local.extent_04 + spatial->minimum_0c.y;
                            local.maximum_18.y = local.minimum_0c.y + local.extent_04;
                            local.minimum_0c.z =
                                static_cast<float>(z) * local.extent_04 + spatial->minimum_0c.z;
                            local.maximum_18.z = local.minimum_0c.z + local.extent_04;
                            bad_links += CountBadRegionMeshLinks00433B90(&local);
                        }
                        ++child;
                    }
                }
                ++x;
            } while (child < 8);
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
void W8Octree::ToggleUpdateSuspension00434020(W8World* world)
{
    if (world == 0) {
        g_octree_trace_node_00659894 = 0;
        g_octree_update_suspended_00659898 = 0;
        return;
    }
    g_octree_update_suspended_00659898 = (g_octree_update_suspended_00659898 == 0);
    if (g_octree_update_suspended_00659898 == 0) {
        g_octree_trace_node_00659894->setFlag(srNode::FLAG_DISABLE);
        g_octree_trace_node_00659894->setFlag(srNode::FLAG_TERMINATE);
        m_reset_visibility_168 = 1;
        MarkRendererReady();
        return;
    }
    for (unsigned short mesh_index = 0; mesh_index < m_meshCount_1b4; ++mesh_index) {
        m_pSubmeshes[static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index])->state_17c + 1]
            .mesh_04 = mesh_index;
        m_pSubmeshes[mesh_index + 1].flags_00 &= 0xffffffc7;
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])->setFlag(srNode::FLAG_DISABLE);
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])
            ->setFlag(srNode::FLAG_TERMINATE);
    }
    memset(m_pfRegsVisited, 0, spatial_000.region_id_bound_58 + 1);
    if (g_octree_trace_node_00659894 == 0) {
        g_octree_trace_node_00659894 = g_octree_game_data_00652db0->CreateTraceModel0041C930();
        g_octree_trace_node_00659894->setParent(world->static_scene, 1);
        SetChainValue15C(reinterpret_cast<char*>(g_octree_trace_node_00659894), 2);
    }
    g_octree_trace_node_00659894->clearFlag(srNode::FLAG_DISABLE);
    g_octree_trace_node_00659894->clearFlag(srNode::FLAG_TERMINATE);
}

// FUNCTION: WIZ8 0x00434220
unsigned char W8Octree::TestNoiseLineOfSight00434220(const srVector3T<float>* from,
                                                     srVector3T<float>* to, float* range, int* hops)
{
    return pathing_180->MeasureAttachmentPath004604B0(from, to, range, hops);
}

// FUNCTION: WIZ8 0x00434250
unsigned char W8Octree::PrepareNavigatorTarget00434250(W8NavigatorMovementState* movement,
                                                       float radius, float separation)
{
    unsigned char result = 0;
    unsigned char hit = 0;
    if (movement->target_position_04c.y > spatial_000.clipped_maximum_30.y) {
        movement->target_position_04c.y = spatial_000.clipped_maximum_30.y;
    }
    srVector3T<float> probe = movement->target_position_04c;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        movement->target_position_04c.y = probe.y;
    }
    if (pathing_180 == 0) {
        return 1;
    }
    srVector3T<float> delta = movement->target_position_04c - movement->position_040;
    delta.y = 0.0f;
    if (srVector2T<float>(delta.x, delta.z).Length() < NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE) {
        return 0;
    }
    if ((movement->attachment_0ac->flags_00 & 0x10000) == 0) {
        srVector3T<float> target = movement->target_position_04c;
        if (pathing_180->FindPathCell00459D60(&target, 0, 1) != 0) {
            if (pathing_180->TestWaypointSpan0045A1B0(&movement->position_040, &target, 0, 0) ==
                0) {
                movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040,
                                                                    &target);
                movement->attachment_0ac->separation_54 = separation;
                result = pathing_180->BuildAttachmentPath00460950(movement->attachment_0ac,
                                                                  movement->unknown_000);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c =
                        attachment->position_4c[attachment->path_position_index_08];
                    pathing_180->AdvanceAttachmentWaypoint00462DE0(&movement->position_040,
                                                                   attachment);
                    movement->attachment_0ac->GetNextPosition00456660(
                        &movement->target_position_04c);
                    return result;
                }
                result = pathing_180->ProbeAttachmentPath00462360(movement->attachment_0ac);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c =
                        attachment->position_4c[attachment->path_position_index_08];
                    return result;
                }
            } else {
                movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040,
                                                                    &movement->target_position_04c);
                result = 1;
            }
        }
        return result;
    }
    delta = movement->target_position_04c - movement->position_040;
    float length = delta.Length();
    float gap = length - separation;
    if (gap < g_float_005ebb34) {
        movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040,
                                                            &movement->position_040);
        return 1;
    }
    if (gap < g_world_scale_005ebc40) {
        delta.SetLength(gap * g_float_005ec028);
        delta += movement->position_040;
        movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040, &delta);
        return 1;
    }
    movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040,
                                                        &movement->target_position_04c);
    movement->attachment_0ac->separation_54 = separation;
    return pathing_180->PlanMovement00463460(movement, radius, separation) != 0;
}

// FUNCTION: WIZ8 0x004347d0
unsigned char __stdcall IsNavigatorAtTarget004347D0(W8NavigatorMovementState* movement)
{
    srVector3T<float> target;
    if (movement->attachment_0ac != 0) {
        W8NavigatorAttachment* attachment = movement->attachment_0ac;
        if (attachment->value_04 < attachment->path_position_index_08 ||
            (attachment->flags_00 & 0x80000) != 0) {
            return 0;
        }
        attachment->GetNextPosition00456660(&target);
    } else {
        target = movement->target_position_04c;
    }
    if (movement->movement_scale_060 * g_world_scale_005ebc40 <
        (target - movement->position_040).Length()) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00434880
unsigned char W8Octree::PrepareNavigatorPatrol00434880(W8NavigatorMovementState* movement,
                                                       float minimum, float maximum)
{
    unsigned char result = 0;
    if (pathing_180 != 0) {
        srVector3T<float> velocity = movement->velocity_034;
        movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040,
                                                            &movement->target_position_04c);
        result = pathing_180->BuildPatrolPath00461960(
            movement->attachment_0ac, movement->unknown_000, &movement->target_position_04c,
            minimum, &velocity, maximum);
        if (result == 0) {
            return 0;
        }
        double step = movement->movement_scale_060 * g_world_scale_005ebc40;
        movement->attachment_0ac->GetNextPosition00456660(&movement->target_position_04c);
        srVector3T<float> delta;
        delta = movement->target_position_04c - movement->position_040;
        if (step < delta.Length()) {
            delta.SetLength(step);
        }
        movement->target_position_04c = delta + movement->position_040;
    }
    return result;
}

// FUNCTION: WIZ8 0x00434a00
unsigned char W8Octree::LinkNavigatorTarget00434A00(W8NavigatorMovementState* movement,
                                                    const srVector3T<float>* target,
                                                    float separation)
{
    if (pathing_180 != 0) {
        return pathing_180->LinkAttachmentTarget004612A0(movement->attachment_0ac,
                                                         movement->unknown_000, target, separation);
    }
    return 0;
}

/* The cell-walk probes and the trace helpers the two line-of-sight bodies use.
   None of their bodies are recovered, so they keep address-qualified names. */

// GLOBAL: WIZ8 0x005ebcd0
float g_octree_cell_scale_005ebcd0 = 100.0f;
/* 0x00659888 accumulates every byte the loader reads, and 0x00652DB0 caches the
   game-data block LoadWorld hands back through its out parameter. */

// GLOBAL: WIZ8 0x00659888
unsigned long g_octree_bytes_read_00659888;

namespace {

void DestroyBitArray(BitArray*& bits)
{
    if (bits != 0) {
        delete bits;
        bits = 0;
    }
}

template <class T> T ReadHeader(const unsigned char* header, unsigned int offset)
{
    T result;
    memcpy(&result, header + offset, sizeof(result));
    return result;
}

template <class T> void WriteMember(W8Octree* octree, unsigned int offset, T value)
{
    // reinterpret-ok: writes packed octree members at their serialized byte offsets
    memcpy(reinterpret_cast<unsigned char*>(octree) + offset, &value, sizeof(value));
}

} // namespace

/* Follow one child bit per axis and level through the compact 9-word branch
   records.  Zero is the missing-child sentinel; live leaves start at one. */
// FUNCTION: WIZ8 0x00433660
unsigned long W8Octree::FindLeaf00433660(const int* point)
{
    unsigned long level = spatial_000.depth_44;
    unsigned long mask = 1 << spatial_000.depth_44;
    unsigned long node = 1;

    do {
        if (static_cast<long>(level) < 1) {
            break;
        }
        mask /= 2;
        int child = 0;
        if ((point[0] & mask) != 0) {
            child = 4;
        }
        if ((point[1] & mask) != 0) {
            child += 2;
        }
        if ((point[2] & mask) != 0) {
            child += 1;
        }
        node = m_owned_09c[node].children_04[child];
        --level;
    } while (node != 0);
    if (m_leaf_count_0b8 < node) {
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
    int bit = 1 << spatial_000.depth_44;
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
            node = m_owned_09c[node].children_04[octant];
        }
        bit = bit / 2;
    }
    return node;
}

/* Bounds-checked cell -> leaf index shared by the cell probes: the direct
   leaf grid when one exists, else the same masked branch descent. */
// FUNCTION: WIZ8 0x00433730
unsigned int W8Octree::LeafIndexForCell(const int* cell)
{
    if (cell[0] < 0 || static_cast<int>(m_leaf_grid_dim_x_0a4) <= cell[0] || cell[1] < 0 ||
        static_cast<int>(m_leaf_grid_dim_y_0a8) <= cell[1] || cell[2] < 0 ||
        static_cast<int>(m_leaf_grid_dim_z_0ac) <= cell[2]) {
        return 0;
    }
    if (m_owned_0b0 != 0) {
        return m_owned_0b0[spatial_000.leaf_grid_stride_x_64 * cell[0] + cell[2] +
                           spatial_000.leaf_grid_stride_y_68 * cell[1]];
    }
    unsigned int leaf_index = 1;
    unsigned int depth = spatial_000.depth_44;
    unsigned int bit = 1 << (spatial_000.depth_44 & 0x1f);
    do {
        if (static_cast<int>(depth) < 1) {
            break;
        }
        bit = bit / 2;
        int octant = 0;
        if ((bit & cell[0]) != 0) {
            octant = 4;
        }
        if ((cell[1] & bit) != 0) {
            octant += 2;
        }
        if ((bit & cell[2]) != 0) {
            octant += 1;
        }
        leaf_index = m_owned_09c[leaf_index].children_04[octant];
        --depth;
    } while (leaf_index != 0);
    if (m_leaf_count_0b8 < leaf_index) {
        leaf_index = 0;
    }
    return leaf_index;
}

/* Settle a point onto the geometry below it: seed a downward trace from the
   point raised by `limit` and march the cell column downward, optionally
   testing the cell's props first (a prop hit is remembered in current_prop
   and forfeits the geometry result) and always testing the level surfaces.
   The point's y drops to the contact on a hit and keeps its input value on a
   miss; `out_hit` receives the outcome byte when given. */
// FUNCTION: WIZ8 0x00433820
float W8Octree::SettleToGround(srVector3T<float>* position, unsigned char* out_hit, char test_props,
                               float limit)
{
    W8OctreeTrace trace;
    int cell[3];
    srVector3T<float> start;
    srVector3T<float> end;
    bool prop_hit = false;
    char hit = 0;

    m_gd_result_count_1b8 = 0;
    if (test_props != 0) {
        current_prop = -1;
    }
    m_owned_194->ClearAll();
    if (spatial_000.clipped_maximum_30.y < position->y) {
        position->y = spatial_000.clipped_maximum_30.y;
    }
    end.x = position->x;
    start.y = position->y + limit;
    end.y = position->y;
    end.z = position->z;
    start.x = position->x;
    start.z = position->z;
    cell[0] =
        static_cast<int>(((position->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    cell[1] =
        static_cast<int>(((position->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    cell[2] =
        static_cast<int>(((position->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    end.y =
        static_cast<float>((cell[1] - 1)) * spatial_000.node_extent_70 + spatial_000.minimum_0c.y;
    trace.Reseed(&start, &end);
    if (-1 < cell[1]) {
        do {
            if (hit != 0) {
                goto done;
            }
            if (test_props != 0) {
                g_octree_state_00659890 = m_aulGDObjs;
                m_gd_result_count_1b8 = 0;
                unsigned long before = m_gd_result_count_1b8;
                CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                if (m_gd_result_count_1b8 != before) {
                    int prop = g_octree_game_data_00652db0->TestPropSurfaces(
                        m_gd_result_count_1b8, m_aulGDObjs, &trace, 0, 0);
                    current_prop = prop;
                    if (prop >= 0) {
                        prop_hit = true;
                    }
                }
            }
            g_octree_game_data_00652db0->value_88 = 1;
            if (ProbeCellForTrace(cell) == 0) {
                goto descend;
            }
            hit = g_octree_game_data_00652db0->TestTraceResult(m_gd_result_count_1b8, m_aulGDObjs,
                                                               &trace, 0, 0);
            if (hit == 0) {
            descend:
                if (prop_hit) {
                    hit = 1;
                } else {
                    float level = static_cast<float>(cell[1]);
                    --cell[1];
                    start.y = level * spatial_000.node_extent_70 + spatial_000.minimum_0c.y;
                    end.y = end.y - spatial_000.node_extent_70;
                    trace.Reseed(&start, &end);
                }
            } else if (prop_hit) {
                current_prop = -1;
            }
            g_octree_game_data_00652db0->value_88 = 0;
        } while (-1 < cell[1]);
        if (hit != 0) {
        done:
            position->y = trace.end_0c.y;
            goto out;
        }
    }
    trace.end_0c.y = position->y;
out:
    if (out_hit != 0) {
        *out_hit = hit;
    }
    return trace.end_0c.y;
}

/* Snap a point onto the surface below: clamp it to the octree's clipped
   ceiling, probe the ground one world-scale unit lower and keep the settled
   height on a hit. The path builders and navigator placement use it to drop
   points onto terrain. */
// FUNCTION: WIZ8 0x00431d20
bool W8Octree::SnapToGround(srVector3T<float>* position, char mode)
{
    unsigned char hit = 0;
    if (position->y > spatial_000.clipped_maximum_30.y) {
        position->y = spatial_000.clipped_maximum_30.y;
    }
    srVector3T<float> probe = *position;
    probe.y = position->y - g_world_scale_005ebc40;
    SettleToGround(&probe, &hit, mode, 500.0f);
    if (hit != 0) {
        position->y = probe.y;
    }
    return hit != 0;
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
                              char allow_fallback)
{
    W8OctreeWalk walk;
    int cell[5];
    int end_cell[3];
    int step[4];
    unsigned char blocked = 0;
    int span;
    int error_0;
    int error_1;

    W8OctreeTrace trace(from, to);

    m_gd_result_count_1b8 = 0;
    m_owned_190->ClearAll();
    m_current_regions_160->ClearAll();
    cell[0] = static_cast<int>(((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    end_cell[0] =
        static_cast<int>(((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    cell[1] = static_cast<int>(((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    end_cell[1] =
        static_cast<int>(((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    cell[2] = static_cast<int>(((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    end_cell[2] =
        static_cast<int>(((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    span = abs(cell[2] - end_cell[2]) + abs(cell[1] - end_cell[1]) + abs(cell[0] - end_cell[0]);

    if (span < 2) {
        ProbeCellForBlockers(cell);
        blocked = TestProbeResult(&trace);
        if (blocked == 0 && span != 0) {
            ProbeCellForBlockers(end_cell);
            blocked = TestProbeResult(&trace);
        }
    } else {
        BuildCellWalk(from, to, &walk);
        cell[0] = walk.cell_00[0];
        cell[1] = walk.cell_00[1];
        cell[2] = walk.cell_00[2];
        cell[4] = walk.minor_axis_20;
        step[0] = walk.step_0c[0];
        step[1] = walk.step_0c[1];
        step[2] = walk.step_0c[2];
        step[3] = walk.count_24;
        cell[3] = 0;
        error_1 = walk.error_38;
        error_0 = walk.error_2c;
        if (walk.count_24 > 0) {
            do {
                if (blocked != 0) {
                    break;
                }
                if (ProbeCellForBlockers(cell) != 0) {
                    blocked = TestProbeResult(&trace);
                }
                if (error_0 < error_1) {
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers(cell) != 0) {
                            blocked = TestProbeResult(&trace);
                        }
                        if (error_1 < 0 && blocked == 0) {
                            cell[cell[4]] += step[cell[4]];
                            error_1 += walk.error_reset_3c;
                            if (ProbeCellForBlockers(cell) != 0) {
                                blocked = TestProbeResult(&trace);
                            }
                        }
                    }
                } else if (error_1 < 0 && blocked == 0) {
                    cell[cell[4]] += step[cell[4]];
                    error_1 += walk.error_reset_3c;
                    if (ProbeCellForBlockers(cell) != 0) {
                        blocked = TestProbeResult(&trace);
                    }
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers(cell) != 0) {
                            blocked = TestProbeResult(&trace);
                        }
                    }
                }
                cell[walk.major_axis_18] += step[walk.major_axis_18];
                error_1 -= walk.error_delta_34;
                error_0 -= walk.error_delta_28;
                ++cell[3];
            } while (cell[3] < step[3]);
        }
    }
    if (blocked != 0) {
        to->x = trace.end_0c.x;
        to->y = trace.end_0c.y;
        to->z = trace.end_0c.z;
    } else if (allow_fallback != 0 && TraceAgainstProps(from, to, 1, 1) != 0) {
        blocked = 1;
    }
    return blocked == 0;
}

// FUNCTION: WIZ8 0x00434f20
short W8Octree::TraceLineOfSight(const srVector3T<float>* from, srVector3T<float>* to,
                                 char trace_world, int from_location_id, int to_location_id,
                                 char visit_octree, int trace_mode)
{
    W8OctreeWalk walk;
    int cell[3];
    int end_cell[3];
    int step[3];

    char blocked = 0;
    char previous = 0;
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

    if (visit_octree != 0) {
        m_gd_result_count_1b8 = 0;
        m_owned_194->ClearAll();
        cell[0] =
            static_cast<int>(((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
        end_cell[0] =
            static_cast<int>(((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
        cell[1] =
            static_cast<int>(((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
        end_cell[1] =
            static_cast<int>(((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
        cell[2] =
            static_cast<int>(((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
        end_cell[2] =
            static_cast<int>(((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
        span = abs(cell[2] - end_cell[2]) + abs(cell[1] - end_cell[1]) + abs(cell[0] - end_cell[0]);

        if (span < 2) {
            ProbeCellForTrace(cell);
            blocked = g_octree_game_data_00652db0->TestTraceResult(
                m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134, 0);

            if (blocked == 0 && span != 0) {
                ProbeCellForTrace(end_cell);
                blocked = g_octree_game_data_00652db0->TestTraceResult(
                    m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134, 0);
            }
        } else {
            BuildCellWalk(from, to, &walk);
            cell[1] = walk.cell_00[1];
            cell[0] = walk.cell_00[0];
            cell[2] = walk.cell_00[2];
            minor_0 = walk.minor_axis_1c;
            major = walk.major_axis_18;
            minor_1 = walk.minor_axis_20;
            step[0] = walk.step_0c[0];
            count = walk.count_24;
            step[1] = walk.step_0c[1];
            step[2] = walk.step_0c[2];
            index = 0;
            error_1 = walk.error_38;
            error_0 = walk.error_2c;
            previous = 0;
            if (walk.count_24 > 0) {
                do {
                    blocked = previous;
                    if (blocked != 0) {
                        break;
                    }
                    if (ProbeCellForTrace(cell) != 0) {
                        blocked = g_octree_game_data_00652db0->TestTraceResult(
                            m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134, 0);
                    }
                    if (error_0 < error_1) {
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace(cell) != 0) {
                                blocked = g_octree_game_data_00652db0->TestTraceResult(
                                    m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134,
                                    0);
                            }
                            if (error_1 < 0 && blocked == 0) {
                                cell[minor_1] += step[minor_1];
                                error_1 += walk.error_reset_3c;
                                if (ProbeCellForTrace(cell) != 0) {
                                    blocked = g_octree_game_data_00652db0->TestTraceResult(
                                        m_gd_result_count_1b8, m_aulGDObjs, &trace,
                                        m_positional_134, 0);
                                }
                            }
                        }
                    } else if (error_1 < 0 && blocked == 0) {
                        cell[minor_1] += step[minor_1];
                        error_1 += walk.error_reset_3c;
                        if (ProbeCellForTrace(cell) != 0) {
                            blocked = g_octree_game_data_00652db0->TestTraceResult(
                                m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134, 0);
                        }
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace(cell) != 0) {
                                blocked = g_octree_game_data_00652db0->TestTraceResult(
                                    m_gd_result_count_1b8, m_aulGDObjs, &trace, m_positional_134,
                                    0);
                            }
                        }
                    }
                    cell[major] += step[major];
                    error_1 -= walk.error_delta_34;
                    error_0 -= walk.error_delta_28;
                    ++index;
                    previous = blocked;
                } while (index < count);
            }
        }
        if (trace_world == 0 || TraceAgainstProps(from, &trace.end_0c, 0, 0) == 0) {
            if (blocked == 0) {
                goto resolve;
            }
        } else {
            blocked = 1;
        }
        result = 1;
        if (blocked != 0) {
            to->x = trace.end_0c.x;
            to->y = trace.end_0c.y;
            to->z = trace.end_0c.z;
            return 1;
        }
    }
resolve:
    if (from_location_id > -3) {
        hit_location = to_location_id;
        if (ResolveTraceHit(&trace.start_00, &trace.end_0c, from_location_id, &hit_location,
                            to_location_id, 0, trace_mode) != 0) {
            to->x = trace.end_0c.x;
            to->y = trace.end_0c.y;
            to->z = trace.end_0c.z;
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
   monster's navigator unknown_090; `noise_adjust` applies the range-scaled
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
    inflate = g_runtime_world_scale_6081e8;
    if (inflate < g_position_height_epsilon_005ebfdc) {
        inflate = g_position_height_epsilon_005ebfdc;
    }
    if (hit_location == 0) {
        target = -3;
    } else {
        target = *hit_location;
    }
    high.x = from->x;
    high.y = from->y;
    high.z = from->z;
    low.x = from->x;
    low.y = from->y;
    low.z = from->z;
    if (low.x <= to->x) {
        high.x = to->x;
    } else {
        low.x = to->x;
    }
    if (low.y <= to->y) {
        high.y = to->y;
    } else {
        low.y = to->y;
    }
    if (low.z <= to->z) {
        high.z = to->z;
    } else {
        low.z = to->z;
    }
    low.x = low.x - inflate;
    low.y = low.y - inflate;
    low.z = low.z - inflate;
    high.x = high.x + inflate;
    high.y = high.y + inflate;
    high.z = high.z + inflate;
    if (target == -1) {
        radius = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
        navigator = g_startup_world_659c0c;
    } else {
        if (target < 1) {
            goto no_probes;
        }
        unsigned int monster_index = MonsterGetIndexByLocationID(0x1836, OCTREE_CPP, target, 1);
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
        if (info == 0 || info->monster == 0) {
            goto no_probes;
        }
        navigator = info->monster;
        radius = navigator->movement_0c0.alternate_radius_0b4;
    }
    if (navigator != 0 && pathing_180 != 0) {
        probe_set = pathing_180->CollectPathProbes004656A0(&navigator->movement_0c0, radius);
    }
no_probes:;
    unsigned int count = static_cast<unsigned int>(
        QueryObjects(&ids, &low, &high, W8_OCTREE_KIND_LOCATION, -1)); /* c-style-cast-ok:
            the shared query count field is stored unsigned */
    if (count != 0) {
        do {
            int id = ids[index];
            if (((excluded < 0) || (excluded != id)) && ((target < 0) || (location != id)) &&
                (probe_set == 0 || pathing_180->MatchesPathProbe00465970(id, 0, 0) == 0)) {
                unsigned int monster_index = MonsterGetIndexByLocationID(0x1851, OCTREE_CPP, id, 1);
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
                if (info != 0) {
                    W8Monster* monster = info->monster;
                    if (monster != 0 && monster->state_088 != 0 &&
                        (monster->unknown_090 & flags) == 0) {
                        center.x = monster->movement_0c0.position_040.x;
                        center.z = monster->movement_0c0.position_040.z;
                        center.y = monster->movement_0c0.position_040.y +
                                   monster->movement_0c0.height_offset_0b8;
                        float distance = PointToSegmentDistance00437540(&center, from, to, 1, 0);
                        if (noise_adjust != 0) {
                            distance =
                                distance - (static_cast<float>(
                                                sqrt((center.x - from->x) * (center.x - from->x) +
                                                     (center.y - from->y) * (center.y - from->y) +
                                                     (center.z - from->z) * (center.z - from->z))) /
                                                segment_length * g_float_005ebc78 +
                                            g_float_005ebc3c) *
                                               g_world_scale_005ebc40;
                            if (distance < g_float_005ebb34) {
                                distance = g_float_005ebb34;
                            }
                        }
                        float monster_radius = monster->radius_084;
                        if (distance < monster_radius) {
                            direction.x = to->x - from->x;
                            direction.y = to->y - from->y;
                            direction.z = to->z - from->z;
                            double length2 = static_cast<double>(
                                (static_cast<float>(direction.x) * direction.x +
                                 static_cast<float>(direction.y) * direction.y +
                                 static_cast<float>(direction.z) * direction.z));
                            offset = direction;
                            if (length2 !=
                                static_cast<double>(static_cast<float>(g_zero_005ebb40))) {
                                float fraction =
                                    (static_cast<float>(sqrt(length2)) -
                                     static_cast<float>(
                                         sqrt(static_cast<double>((monster_radius * monster_radius -
                                                                   distance * distance))))) /
                                    static_cast<float>(sqrt(length2));
                                offset.x = direction.x * fraction;
                                offset.y = direction.y * fraction;
                                offset.z = direction.z * fraction;
                            }
                            if (0.0 <= best) {
                                float span = offset.x * offset.x;
                                if (static_cast<float>(sqrt(static_cast<double>(
                                        (offset.z * offset.z + offset.y * offset.y + span)))) <
                                    static_cast<float>(best)) {
                                    best = static_cast<double>(
                                        static_cast<float>(sqrt(static_cast<double>(
                                            (offset.y * offset.y + offset.z * offset.z + span)))));
                                    best_index = index;
                                }
                            } else {
                                best = static_cast<double>(static_cast<float>(sqrt(
                                    static_cast<double>((offset.x * offset.x + offset.y * offset.y +
                                                         offset.z * offset.z)))));
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
            to->x = offset.x + from->x;
            to->y = offset.y + from->y;
            to->z = offset.z + from->z;
            if (hit_location != 0) {
                *hit_location = ids[best_index];
            }
            return 1;
        }
    }
    if (excluded != -1 && location != -1) {
        GetCameraPosition(&camera);
        center.x = camera.x;
        center.y = camera.y;
        center.z = camera.z;
        float distance = PointToSegmentDistance00437540(&center, from, to, 1, 0);
        if (noise_adjust != 0) {
            distance =
                distance - (static_cast<float>(sqrt((center.y - from->y) * (center.y - from->y) +
                                                    (center.z - from->z) * (center.z - from->z) +
                                                    (center.x - from->x) * (center.x - from->x))) /
                                segment_length * g_float_005ebc78 +
                            g_float_005ebc3c) *
                               g_world_scale_005ebc40;
            if (distance < g_float_005ebb34) {
                distance = g_float_005ebb34;
            }
        }
        float camera_radius = g_startup_world_659c0c->radius_084 * g_float_006081f4;
        if (distance < camera_radius) {
            offset.x = to->x - from->x;
            offset.y = to->y - from->y;
            offset.z = to->z - from->z;
            double length2 = static_cast<double>((static_cast<float>(offset.y) * offset.y +
                                                  static_cast<float>(offset.z) * offset.z +
                                                  static_cast<float>(offset.x) * offset.x));
            float z_scale;
            if (length2 == static_cast<double>(static_cast<float>(g_zero_005ebb40))) {
                z_scale = offset.z;
            } else {
                float fraction = (static_cast<float>(sqrt(length2)) -
                                  static_cast<float>(sqrt(static_cast<double>(
                                      (camera_radius * camera_radius - distance * distance))))) /
                                 static_cast<float>(sqrt(length2));
                offset.x = offset.x * fraction;
                offset.y = offset.y * fraction;
                z_scale = fraction * offset.z;
            }
            to->x = offset.x + from->x;
            to->y = offset.y + from->y;
            to->z = z_scale + from->z;
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

// TEMPLATE: WIZ8 0x00439290
// W8HashTable<unsigned int,int>::Grow

// TEMPLATE: WIZ8 0x00439140
// W8HashTable<unsigned int,short>::Grow

// TEMPLATE: WIZ8 0x004393e0
// W8HashTable<unsigned int,int>::AllocateEntry

// TEMPLATE: WIZ8 0x00438c90
// W8HashTable<unsigned int,int>::Remove

// TEMPLATE: WIZ8 0x00438d50
// W8HashTable<unsigned int,int>::FindNextEntry

// TEMPLATE: WIZ8 0x0055dbb0
// W8HashTable<unsigned int,int>::Insert

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
unsigned char W8OctreeObjectRegistry::MoveObjectToCell(int kind, int id, const int* point)
{
    unsigned int object_key = PackOctreeObjectKey(kind, id);
    unsigned int cell_key = PackOctreeCellKey(point[0], point[1], point[2]);
    int object_value = static_cast<int>(object_key);
    int cell_value = static_cast<int>(cell_key);

    by_object->Remove(&object_key, &cell_value);
    by_object->Insert(&object_key, &cell_value);
    by_cell->Remove(&cell_key, &object_value);
    by_cell->Insert(&cell_key, &object_value);
    return 1;
}

// FUNCTION: WIZ8 0x00436dc0
unsigned char W8OctreeObjectRegistry::UnregisterObject(int kind, int id)
{
    unsigned int object_key = PackOctreeObjectKey(kind, id);
    int object_value = static_cast<int>(object_key);
    unsigned char removed = 0;

    int slot = by_object->FindNextEntry(&object_key, -1);
    while (slot != -1) {
        unsigned int cell_key = static_cast<unsigned int>(by_object->entries[slot].value);
        int cell_value = static_cast<int>(cell_key);
        by_object->Remove(&object_key, &cell_value);
        by_cell->Remove(&cell_key, &object_value);
        removed = 1;
        slot = by_object->FindNextEntry(&object_key, slot);
    }
    return removed;
}

/* Record that one object now occupies one cell.

   The object's key is its kind in the high half and its id in the low half.
   When it is already registered somewhere, the cell it was in is compared
   against the one being queued and an unchanged pairing is left completely
   alone; otherwise the old pairing comes out of both indexes first. A
   location registration additionally maintains a second pairing under the
   navigator kind.

   The two indexes are the same pair AddCollidablePropBounds keeps: one keyed by
   cell, one keyed by object. */
// FUNCTION: WIZ8 0x00437000
unsigned char W8OctreeObjectRegistry::RegisterObjectCell(int kind, int id, const int* point)
{
    W8OctreeIndex* by_object = this->by_object;
    W8OctreeIndex* by_cell;
    W8OctreeEntry* entries;
    unsigned int object_key;
    unsigned int tagged_key;
    unsigned int hash;
    int cell_key;
    int occupied;
    int slot;
    int previous;
    int* bucket;

    object_key = PackOctreeObjectKey(kind & 0xffff, id);
    hash = (object_key >> 10 ^ object_key) >> 10 ^ object_key;
    slot = by_object->bucket_heads[hash & (by_object->bucket_count - 1)];
    while (slot != -1) {
        entries = by_object->entries;
        if (entries[slot].key == object_key) {
            occupied = entries[slot].value;
            if (occupied == 0) {
                break;
            }
            /* The unchanged-cell test is genuinely degenerate in retail: it
               never compares the old cell's x halfword and it compares the
               old y halfword shifted a byte too far, so it only recognizes a
               repeat of (0,0,z). Every other same-cell pairing falls through
               to the remove-and-reinsert path, which leaves the registry in
               the same state anyway. */
            if (point[0] == 0 && (((occupied - 1) & 0xff00) << 8) == point[1] &&
                OctreeCellKeyZ(occupied) == point[2]) {
                return 1;
            }
            bucket = by_object->bucket_heads + (hash & (by_object->bucket_count - 1));
            slot = *bucket;
            if (slot != -1) {
                entries = by_object->entries;
                previous = -1;
                for (;;) {
                    if (entries[slot].key == object_key) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        by_object->entries[slot].next_index = by_object->free_head;
                        by_object->free_head = slot;
                        break;
                    }
                    previous = slot;
                    slot = entries[slot].next_index;
                    if (slot == -1) {
                        break;
                    }
                }
            }
            by_cell = this->by_cell;
            bucket = by_cell->bucket_heads +
                     (((static_cast<unsigned int>(occupied) >> 10 ^ occupied) >> 10 ^ occupied) &
                      (by_cell->bucket_count - 1));
            if (*bucket != -1) {
                entries = by_cell->entries;
                slot = *bucket;
                previous = -1;
                do {
                    if (entries[slot].key == static_cast<unsigned int>(occupied) &&
                        entries[slot].value == static_cast<int>(object_key)) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        by_cell->entries[slot].next_index = by_cell->free_head;
                        by_cell->free_head = slot;
                        break;
                    }
                    previous = slot;
                    slot = entries[slot].next_index;
                } while (entries[previous].next_index != -1);
            }
            goto record;
        }
        slot = entries[slot].next_index;
    }
    if (static_cast<short>(kind) == W8_OCTREE_KIND_LOCATION) {
        cell_key = PackOctreeCellKey(point[0], point[1], point[2]);
        tagged_key = PackOctreeObjectKey(W8_OCTREE_KIND_NAVIGATOR, id);
        by_object->Remove(&tagged_key, &cell_key);
        by_object = this->by_object;
        slot = by_object->AllocateEntry();
        entries = by_object->entries;
        hash = ((tagged_key >> 10 ^ tagged_key) >> 10 ^ tagged_key) & (by_object->bucket_count - 1);
        entries[slot].key = tagged_key;
        entries[slot].value = cell_key;
        entries[slot].next_index = by_object->bucket_heads[hash];
        by_object->bucket_heads[hash] = slot;
        this->by_cell->Remove((const unsigned int*)&cell_key, (const int*)&tagged_key);
        this->by_cell->Insert((const unsigned int*)&cell_key, (const int*)&tagged_key);
    }
record:
    cell_key = PackOctreeCellKey(point[0], point[1], point[2]);
    this->by_object->Remove(&object_key, &cell_key);
    this->by_object->Insert(&object_key, &cell_key);
    this->by_cell->Remove((const unsigned int*)&cell_key, (const int*)&object_key);
    this->by_cell->Insert((const unsigned int*)&cell_key, (const int*)&object_key);
    return 1;
}

/* Register one collidable prop against every octree cell its bounding box
   touches.

   Two indexes are kept in step: one keyed by cell so a cell can name its props,
   one keyed by prop so a prop can name its cells. Each cell first drops any
   stale pairing in both directions before the new one goes in, which is what
   makes repeated calls for a moving prop safe. The cell key packs x, y and z
   into one dword a byte apart, and the prop key carries its id in the low half
   with a tag above it. */
// FUNCTION: WIZ8 0x0042eab0
void W8Octree::AddCollidablePropBounds(int index, const W8BoundingBox* bounds)
{
    int minimum[3];
    int maximum[3];
    W8OctreeIndex* by_prop;
    W8OctreeIndex* by_cell;
    W8OctreeEntry* entries;
    unsigned int prop_key;
    unsigned int cell_key;
    unsigned int hash;
    int slot;
    int axis;
    int x;
    int y;
    int z;

    for (axis = 0; axis < 3; ++axis) {
        minimum[axis] = static_cast<int>(
            ((bounds->minimum.x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    }
    minimum[0] = static_cast<int>(
        ((bounds->minimum.x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    minimum[1] = static_cast<int>(
        ((bounds->minimum.y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    minimum[2] = static_cast<int>(
        ((bounds->minimum.z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    maximum[0] = static_cast<int>(
        ((bounds->maximum.x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    maximum[1] = static_cast<int>(
        ((bounds->maximum.y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    maximum[2] = static_cast<int>(
        ((bounds->maximum.z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));

    prop_key = PackOctreeObjectKey(W8_OCTREE_KIND_PROP, index + 1);
    for (x = minimum[0]; x <= maximum[0]; ++x) {
        for (y = minimum[1]; y <= maximum[1]; ++y) {
            for (z = minimum[2]; z <= maximum[2]; ++z) {
                cell_key = PackOctreeCellKey(x, y, z);
                by_prop = object_registry->by_object;
                by_prop->Remove(&prop_key, (const int*)&cell_key);
                slot = by_prop->AllocateEntry();
                entries = by_prop->entries;
                hash = ((prop_key >> 10 ^ prop_key) >> 10 ^ prop_key) & (by_prop->bucket_count - 1);
                entries[slot].key = prop_key;
                entries[slot].value = cell_key;
                entries[slot].next_index = by_prop->bucket_heads[hash];
                by_prop->bucket_heads[hash] = slot;

                by_cell = object_registry->by_cell;
                by_cell->Remove(&cell_key, (const int*)&prop_key);
                if (by_cell->free_head == -1) {
                    by_cell->Grow();
                }
                slot = by_cell->free_head;
                entries = by_cell->entries;
                by_cell->free_head = entries[slot].next_index;
                entries[slot].key = cell_key;
                entries[slot].value = prop_key;
                hash = ((cell_key >> 10 ^ cell_key) >> 10 ^ cell_key) & (by_cell->bucket_count - 1);
                entries[slot].next_index = by_cell->bucket_heads[hash];
                by_cell->bucket_heads[hash] = slot;
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
    int from_cell[3];
    int to_cell[3];
    float fraction[3];
    float delta[3];
    int step[3];
    int extent[3];
    float cell_size;
    int cell;
    int axis;
    int longest = 0;
    int major = 0;
    int minor_0;
    int minor_1;
    int span;

    cell_size = spatial_000.node_extent_70 * g_octree_cell_scale_005ebcd0;
    cell = static_cast<int>(cell_size);
    from_cell[0] =
        static_cast<int>(((from->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0));
    to_cell[0] =
        static_cast<int>(((to->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0));
    from_cell[1] =
        static_cast<int>(((from->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0));
    to_cell[1] =
        static_cast<int>(((to->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0));
    from_cell[2] =
        static_cast<int>(((from->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0));
    to_cell[2] =
        static_cast<int>(((to->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0));

    for (axis = 0; axis < 3; ++axis) {
        span = to_cell[axis] - from_cell[axis];
        fraction[axis] = static_cast<float>((from_cell[axis] % cell)) / cell_size;
        delta[axis] = static_cast<float>(span);
        if (span < 0) {
            step[axis] = -1;
            span = -span;
        } else {
            step[axis] = 1;
            fraction[axis] = 1.0f - fraction[axis];
        }
        if (longest < span) {
            major = axis;
            longest = span;
        }
        extent[axis] = span;
    }

    minor_0 = (major + 1) % 3;
    minor_1 = (major + 2) % 3;
    walk->error_delta_28 =
        static_cast<int>((static_cast<float>(fabs(delta[minor_0] / delta[major])) * cell_size));
    walk->error_2c = static_cast<int>((cell_size * fraction[minor_0] -
                                       static_cast<float>(walk->error_delta_28) * fraction[major]));
    walk->error_delta_34 =
        static_cast<int>((static_cast<float>(fabs(delta[minor_1] / delta[major])) * cell_size));
    walk->error_38 = static_cast<int>((cell_size * fraction[minor_1] -
                                       static_cast<float>(walk->error_delta_34) * fraction[major]));
    walk->count_24 = longest % cell == 0 ? longest / cell + 1 : longest / cell + 2;

    walk->minor_axis_1c = minor_0;
    walk->error_reset_30 = cell;
    walk->error_reset_3c = cell;
    walk->minor_axis_20 = minor_1;
    walk->major_axis_18 = major;
    walk->cell_00[0] = from_cell[0] / cell;
    walk->cell_00[1] = from_cell[1] / cell;
    walk->cell_00[2] = from_cell[2] / cell;
    walk->step_0c[0] = step[0];
    walk->step_0c[1] = step[1];
    walk->step_0c[2] = step[2];
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
int W8Octree::TraceAgainstProps(const srVector3T<float>* from, srVector3T<float>* to, int value_3,
                                int value_4)
{
    W8OctreeWalk walk;
    int cell[4];
    int end_cell[3];
    int step[4];
    int span;
    int error_0;
    int error_1;

    W8OctreeTrace trace(from, to);
    g_octree_state_00659890 = m_aulGDObjs;
    m_gd_result_count_1b8 = 0;
    m_owned_194->ClearAll();
    cell[0] = static_cast<int>((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    end_cell[0] = static_cast<int>((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    cell[1] = static_cast<int>((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    end_cell[1] = static_cast<int>((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    cell[2] = static_cast<int>((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    end_cell[2] = static_cast<int>((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    span = abs(cell[2] - end_cell[2]) + abs(cell[1] - end_cell[1]) + abs(cell[0] - end_cell[0]);
    if (span < 2) {
        g_octree_state_00659890 = m_aulGDObjs;
        CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
        if (span != 0) {
            g_octree_state_00659890 = m_aulGDObjs;
            CollectObjectsInCell(end_cell, W8_OCTREE_KIND_PROP);
        }
    } else {
        BuildCellWalk(from, to, &walk);
        cell[0] = walk.cell_00[0];
        cell[3] = walk.minor_axis_20;
        step[2] = walk.step_0c[2];
        cell[1] = walk.cell_00[1];
        cell[2] = walk.cell_00[2];
        step[0] = walk.step_0c[0];
        step[1] = walk.step_0c[1];
        if (walk.count_24 > 0) {
            int major_step = step[walk.major_axis_18];
            int* major_cell = cell + walk.major_axis_18;
            int count = walk.count_24;
            error_1 = walk.error_38;
            error_0 = walk.error_2c;
            do {
                g_octree_state_00659890 = m_aulGDObjs;
                CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                if (error_0 < error_1) {
                    if (error_0 < 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        g_octree_state_00659890 = m_aulGDObjs;
                        CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                        if (error_1 < 0) {
                            cell[cell[3]] += step[cell[3]];
                            error_1 += walk.error_reset_3c;
                            g_octree_state_00659890 = m_aulGDObjs;
                            CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                        }
                    }
                } else if (error_1 < 0) {
                    cell[cell[3]] += step[cell[3]];
                    g_octree_state_00659890 = m_aulGDObjs;
                    error_1 += walk.error_reset_3c;
                    CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                    if (error_0 < 0) {
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        error_0 += walk.error_reset_30;
                        g_octree_state_00659890 = m_aulGDObjs;
                        CollectObjectsInCell(cell, W8_OCTREE_KIND_PROP);
                    }
                }
                *major_cell += major_step;
                error_0 -= walk.error_delta_28;
                error_1 -= walk.error_delta_34;
                --count;
            } while (count != 0);
        }
    }
    current_prop = g_octree_game_data_00652db0->TestPropSurfaces(m_gd_result_count_1b8, m_aulGDObjs,
                                                                 &trace, value_3, value_4);
    if (current_prop < 0) {
        return 0;
    }
    to->x = trace.end_0c.x;
    to->y = trace.end_0c.y;
    to->z = trace.end_0c.z;
    return current_prop + 1;
}

/* The cell probes share one lookup: bounds-check the cell against the grid,
   resolve its leaf index through the direct leaf grid when one exists else by
   descending the branch tree one octant per level, then drain the chosen leaf
   stream into m_aulGDObjs through the m_owned_194 dedupe set. ProbeCellForTrace
   reads the leaf's gd_polygon_offset_0c into the m_owned_12c surface-id stream;
   ProbeCellForBlockers reads polygon_offset_08 into the m_owned_0d0 index stream
   and maps each entry through m_aulPolyLookup to a (mesh<<16)|polygon key, marking
   the mesh in m_current_regions_160. The Append variant skips the result-count
   reset so successive cells accumulate. */
/* The cell->leaf lookup is inlined here exactly as retail does; the shared
   out-of-line copy (LeafIndexForCell, 0x00433730) is emitted for the
   0x00467xxx callers, which do not inline it. */
// FUNCTION: WIZ8 0x00435b00
int W8Octree::ProbeCellForTrace(const int* cell)
{
    unsigned int leaf_index = 0;

    if (cell[0] >= 0 && cell[0] < static_cast<int>(m_leaf_grid_dim_x_0a4) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[1] >= 0 && cell[1] < static_cast<int>(m_leaf_grid_dim_y_0a8) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[2] >= 0 && cell[2] < static_cast<int>(m_leaf_grid_dim_z_0ac) /* c-style-cast-ok: cell
            coordinate vs grid dimension */) {
        if (m_owned_0b0 != 0) {
            leaf_index = m_owned_0b0[spatial_000.leaf_grid_stride_x_64 * cell[0] + cell[2] +
                                     spatial_000.leaf_grid_stride_y_68 * cell[1]];
        } else {
            unsigned int depth = spatial_000.depth_44;
            unsigned int bit = 1 << (spatial_000.depth_44 & 0x1f);
            leaf_index = 1;
            do {
                if (static_cast<int>(depth) < 1) {
                    break;
                }
                bit = bit / 2;
                int octant = 0;
                if ((bit & cell[0]) != 0) {
                    octant = 4;
                }
                if ((cell[1] & bit) != 0) {
                    octant += 2;
                }
                if ((cell[2] & bit) != 0) {
                    octant += 1;
                }
                leaf_index = m_owned_09c[leaf_index].children_04[octant];
                --depth;
            } while (leaf_index != 0);
            if (m_leaf_count_0b8 < leaf_index) {
                leaf_index = 0;
            }
        }
    }
    m_gd_result_count_1b8 = 0;
    if (leaf_index != 0 && m_owned_0a0[leaf_index].gd_polygon_offset_0c != 0) {
        const unsigned long* stream = m_owned_12c + m_owned_0a0[leaf_index].gd_polygon_offset_0c;
        int remaining = *stream;
        while (remaining != 0) {
            ++stream;
            if (9999 < m_gd_result_count_1b8) {
                break;
            }
            if (m_owned_194->Set(*stream) == 0) {
                m_aulGDObjs[m_gd_result_count_1b8] = *stream;
                ++m_gd_result_count_1b8;
            }
            --remaining;
        }
    }
    return m_gd_result_count_1b8;
}

// FUNCTION: WIZ8 0x00435c40
int W8Octree::ProbeCellForBlockers(const int* cell)
{
    unsigned int leaf_index = 0;

    if (cell[0] >= 0 && cell[0] < static_cast<int>(m_leaf_grid_dim_x_0a4) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[1] >= 0 && cell[1] < static_cast<int>(m_leaf_grid_dim_y_0a8) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[2] >= 0 && cell[2] < static_cast<int>(m_leaf_grid_dim_z_0ac) /* c-style-cast-ok: cell
            coordinate vs grid dimension */) {
        if (m_owned_0b0 != 0) {
            leaf_index = m_owned_0b0[spatial_000.leaf_grid_stride_x_64 * cell[0] + cell[2] +
                                     spatial_000.leaf_grid_stride_y_68 * cell[1]];
        } else {
            unsigned int depth = spatial_000.depth_44;
            unsigned int bit = 1 << (spatial_000.depth_44 & 0x1f);
            leaf_index = 1;
            do {
                if (static_cast<int>(depth) < 1) {
                    break;
                }
                bit = bit / 2;
                int octant = 0;
                if ((bit & cell[0]) != 0) {
                    octant = 4;
                }
                if ((cell[1] & bit) != 0) {
                    octant += 2;
                }
                if ((cell[2] & bit) != 0) {
                    octant += 1;
                }
                leaf_index = m_owned_09c[leaf_index].children_04[octant];
                --depth;
            } while (leaf_index != 0);
            if (m_leaf_count_0b8 < leaf_index) {
                leaf_index = 0;
            }
        }
    }
    m_gd_result_count_1b8 = 0;
    if (leaf_index != 0 && m_owned_0a0[leaf_index].polygon_offset_08 != 0) {
        const unsigned long* stream = m_owned_0d0 + m_owned_0a0[leaf_index].polygon_offset_08;
        for (int remaining = *stream; remaining != 0; --remaining) {
            ++stream;
            if (m_owned_190->Set(*stream) == 0) {
                if (9999 < m_gd_result_count_1b8) {
                    break;
                }
                unsigned int key = m_aulPolyLookup[*stream];
                m_aulGDObjs[m_gd_result_count_1b8] = key;
                ++m_gd_result_count_1b8;
                m_current_regions_160->Set(key >> 0x10);
            }
        }
    }
    return m_gd_result_count_1b8;
}

// FUNCTION: WIZ8 0x00435da0
int W8Octree::ProbeCellForBlockersAppend(const int* cell)
{
    unsigned int leaf_index = 0;

    if (cell[0] >= 0 && cell[0] < static_cast<int>(m_leaf_grid_dim_x_0a4) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[1] >= 0 && cell[1] < static_cast<int>(m_leaf_grid_dim_y_0a8) /* c-style-cast-ok: cell
            coordinate vs grid dimension */ &&
        cell[2] >= 0 && cell[2] < static_cast<int>(m_leaf_grid_dim_z_0ac) /* c-style-cast-ok: cell
            coordinate vs grid dimension */) {
        if (m_owned_0b0 == 0) {
            unsigned int depth = spatial_000.depth_44;
            unsigned int bit = 1 << (spatial_000.depth_44 & 0x1f);
            leaf_index = 1;
            do {
                if (static_cast<int>(depth) < 1) {
                    break;
                }
                bit = bit / 2;
                int octant = 0;
                if ((bit & cell[0]) != 0) {
                    octant = 4;
                }
                if ((cell[1] & bit) != 0) {
                    octant += 2;
                }
                if ((cell[2] & bit) != 0) {
                    octant += 1;
                }
                leaf_index = m_owned_09c[leaf_index].children_04[octant];
                --depth;
            } while (leaf_index != 0);
            if (m_leaf_count_0b8 < leaf_index) {
                leaf_index = 0;
            }
        } else {
            leaf_index = m_owned_0b0[spatial_000.leaf_grid_stride_x_64 * cell[0] + cell[2] +
                                     spatial_000.leaf_grid_stride_y_68 * cell[1]];
        }
        if (leaf_index != 0 && m_owned_0a0[leaf_index].polygon_offset_08 != 0) {
            const unsigned long* stream = m_owned_0d0 + m_owned_0a0[leaf_index].polygon_offset_08;
            for (int remaining = *stream; remaining != 0; --remaining) {
                ++stream;
                if (m_owned_190->Set(*stream) == 0) {
                    if (9999 < m_gd_result_count_1b8) {
                        break;
                    }
                    unsigned int key = m_aulPolyLookup[*stream];
                    m_aulGDObjs[m_gd_result_count_1b8] = key;
                    ++m_gd_result_count_1b8;
                    m_current_regions_160->Set(key >> 0x10);
                }
            }
        }
    }
    return m_gd_result_count_1b8;
}

/* March the buffered (mesh<<16)|polygon keys, ray-test each live mesh
   instance's triangle against the record and keep the nearest contact: the
   record's end_0c returns the hit position and hit_limit_24 the distance. */
// FUNCTION: WIZ8 0x00435f00
unsigned char W8Octree::TestProbeResult(W8OctreeTrace* trace)
{
    bool hit = false;
    srVector3T<float> contact;

    for (unsigned int index = 0; index < m_gd_result_count_1b8; ++index) {
        unsigned int mesh_index = m_aulGDObjs[index] >> 0x10;
        if (mesh_index < m_meshCount_1b4 && g_world->psrMeshes[mesh_index] != 0 &&
            m_pAlphaBits->Test(mesh_index) == 0) {
            stMeshModel* model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->model());
            const srVector4T<float>* planes = model->getPolyEq();
            unsigned int polygon = m_aulGDObjs[index] & 0xffff;
            const srVector4T<float>* plane = planes + polygon;
            float t;
            float distance;
            srVector3T<float> point;
            if (plane->x * trace->step_18.x + trace->step_18.y * plane->y +
                        trace->step_18.z * plane->z <=
                    g_float_005ebb34 &&
                (distance = plane->y * trace->start_00.y + plane->x * trace->start_00.x +
                            plane->z * trace->start_00.z + plane->w,
                 distance <= trace->hit_limit_24) &&
                g_float_005ebb34 < distance) {
                if (g_float_005ebb38 <= distance) {
                    float back = plane->x * trace->end_0c.x + trace->end_0c.y * plane->y +
                                 trace->end_0c.z * plane->z + plane->w;
                    if (g_float_005ebb38 <= back) {
                        continue;
                    }
                    back = -back;
                    if (g_float_005ebb38 <= back || trace->hit_limit_24 < trace->length_28) {
                        t = (distance / (back + distance)) * trace->length_28;
                        point.x = trace->step_18.x * t;
                        point.y = trace->step_18.y * t;
                        point.x += trace->start_00.x;
                        point.y += trace->start_00.y;
                        point.z = t * trace->step_18.z + trace->start_00.z;
                    } else {
                        point = trace->end_0c;
                        t = trace->length_28;
                    }
                } else {
                    point = trace->start_00;
                    t = 0.0f;
                }
                float abs_x = plane->x < 0.0f ? -plane->x : plane->x;
                float abs_y = plane->y < 0.0f ? -plane->y : plane->y;
                float widest = abs_x;
                unsigned char axis = abs_x < abs_y;
                if (abs_x < abs_y) {
                    widest = abs_y;
                }
                if (widest < (plane->z < 0.0f ? -plane->z : plane->z)) {
                    axis = 2;
                }
                const srVector3i* poly_vertex = model->getPolyVertex() + polygon;
                const srVector3T<float>* vertices = model->getVertexLoc();
                srVector3T<float> triangle[3];
                triangle[0] = vertices[poly_vertex->x];
                triangle[1] = vertices[poly_vertex->y];
                triangle[2] = vertices[poly_vertex->z];
                if (PointInsideTriangle0046D530(triangle, axis, &point) != 0 &&
                    t < trace->hit_limit_24) {
                    trace->hit_limit_24 = t;
                    hit = true;
                    contact = point;
                }
            }
        }
    }
    if (hit) {
        trace->end_0c = contact;
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
    m_gd_result_count_1b8 = 0;
    m_owned_190->ClearAll();
    m_current_regions_160->ClearAll();

    srVector3T<float> bounds[2];
    bounds[0].x = center->x - radius;
    bounds[0].z = center->z - radius;
    bounds[1].y = center->y;
    bounds[0].y = center->y - height;
    bounds[1].x = center->x + radius;
    bounds[1].z = center->z + radius;

    int start[3];
    int end[3];
    int axis;
    for (axis = 0; axis < 3; ++axis) {
        start[axis] = static_cast<int>(((&bounds[0].x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                       spatial_000.node_extent_70);
        end[axis] = static_cast<int>(((&bounds[1].x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                     spatial_000.node_extent_70);
    }
    int cell[3];
    for (cell[0] = start[0]; cell[0] <= end[0]; ++cell[0]) {
        for (cell[1] = start[1]; cell[1] <= end[1]; ++cell[1]) {
            for (cell[2] = start[2]; cell[2] <= end[2]; ++cell[2]) {
                ProbeCellForBlockersAppend(cell);
            }
        }
    }

    unsigned int index;
    for (index = 0; index < m_gd_result_count_1b8; ++index) {
        unsigned int mesh_index = m_aulGDObjs[index] >> 0x10;
        stMeshModel* model = 0;
        if (mesh_index < m_meshCount_1b4 && g_world->psrMeshes[mesh_index] != 0 &&
            m_pAlphaBits->Test(mesh_index) == 0) {
            model = static_cast<stMeshModel*>(g_world->psrMeshes[mesh_index]->model());
            const srVector4T<float>* planes = model->getPolyEq();
            unsigned int polygon = m_aulGDObjs[index] & 0xffff;
            if (planes[polygon].y < g_float_005ebc7c) {
                m_aulGDObjs[index] = 0;
            }
        } else {
            m_aulGDObjs[index] = 0;
        }
        if (m_aulGDObjs[index] != 0) {
            const srVector4T<float>* plane = model->getPolyEq() + (m_aulGDObjs[index] & 0xffff);
            srVector3T<float> normal;
            normal.x = plane->x;
            normal.y = plane->y;
            normal.z = plane->z;
            const srVector3i* poly_vertex = model->getPolyVertex() + (m_aulGDObjs[index] & 0xffff);
            const srVector3T<float>* vertices = model->getVertexLoc();
            srVector3T<float> triangle[3];
            triangle[0] = vertices[poly_vertex->x];
            triangle[1] = vertices[poly_vertex->y];
            triangle[2] = vertices[poly_vertex->z];
            if (TestSpatialTriangle0046CE60(bounds, triangle, &normal) == 0) {
                m_aulGDObjs[index] = 0;
            }
        }
    }

    unsigned int count = 0;
    for (index = 0; index < m_gd_result_count_1b8; ++index) {
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
    int point[3];

    if (location_id == 0) {
        return;
    }
    monster_list_index = MonsterGetIndexByLocationID(0x4c4, OCTREE_CPP, location_id, 1);
    info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    if (info != 0 && info->monster != 0) {
        monster = info->monster;
        sector = GetSectorForPosition(position);
        if (sector == 0 || (mesh = g_world->psrMeshes[m_pSubmeshes[sector].mesh_04]) == 0) {
            monster->node_308 = 0;
        } else {
            monster->node_308 = mesh;
        }
    }
    point[0] =
        static_cast<int>(((position->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    point[1] =
        static_cast<int>(((position->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    point[2] =
        static_cast<int>(((position->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    object_registry->RegisterObjectCell(W8_OCTREE_KIND_LOCATION, queue_id, point);
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
        return 0;
    }
    if (m_owned_0c0 != 0) {
        free(m_owned_0c0);
    }
    m_owned_0c0 = static_cast<char*>(malloc(name_length + 1));
    if (m_owned_0c0 == 0) {
        return 0;
    }
    strcpy(m_owned_0c0, path);
    char* extension = strrchr(m_owned_0c0, '.');
    if (extension != 0 && extension - m_owned_0c0 > static_cast<int>(name_length) - 6) {
        *extension = '\0';
    }
    return 1;
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
    unsigned char header[0xf5];
    char acMessage[256];
    int hOctFile;
    unsigned int uiRead;
    int uiTerminator;
    unsigned char fSuccess;
    unsigned char fLoaded;
    void* block;
    unsigned int index;
    unsigned int limit;
    unsigned int name_length;
    char* extension;
    W8GameData* pGameData = 0;

    Reset();
    if (path == 0) {
        Initialize(0);
        return;
    }
    if (CheckLevelAssetSet0042CCC0(path) < 0) {
        goto failed;
    }
    if (CheckLevelAssetSet0042CCC0(path) > 0 && BuildPreprocessedFiles00492E60(path) == 0) {
        g_octree_6598a4 = 0;
        spatial_000.flags_00 |= 0x80000000;
        ReportStartupMessage004969D0("Cannot find or build current preprocessed files.");
        ReportStartupMessage004969D0(
            "Attempting to run with LVL file only -- SOME FEATURES DISABLED.");
        ReportStartupMessage004969D0(0);
        return;
    }
    hOctFile = FileOpen(const_cast<char*>(path), 1, 0);
    if (hOctFile == 0) {
        srAssertFail("hOctFile", OCTREE_CPP, 0xa3, "ReadOctFile: Couldn't open octree file.");
    }
    fSuccess = FileRead(hOctFile, header, 0xf5, &uiRead);
    g_octree_bytes_read_00659888 += uiRead;
    fLoaded = 0;
    if (fSuccess != 0) {
        fSuccess = FileRead(hOctFile, &uiTerminator, 4, &uiRead);
        if (fSuccess == 0 || uiTerminator != -1) {
            srAssertFail("fSuccess && (uiTerminator==0xffffffff)", OCTREE_CPP, 0xac,
                         "ReadOctFile: Header of Oct file longer than expected.");
        }
        fLoaded = 0;
        if (fSuccess != 0) {
            Initialize(header);
            name_length = strlen(path) + 1;
            if (name_length != 1) {
                if (m_owned_0c0 != 0) {
                    free(m_owned_0c0);
                }
                m_owned_0c0 = static_cast<char*>(malloc(name_length));
                if (m_owned_0c0 != 0) {
                    strcpy(m_owned_0c0, path);
                    extension = strrchr(m_owned_0c0, '.');
                    if (extension != 0 &&
                        extension - m_owned_0c0 > static_cast<int>((name_length - 7))) {
                        *extension = '\0';
                    }
                }
            }

            block = malloc((ReadHeader<unsigned long>(header, 0x6a) * 9 + 0x12) * 4);
            m_owned_09c = static_cast<W8OctPreTreeBranch*>(block);
            if (block == 0) {
                fSuccess = 0;
                strcpy(acMessage, "ReadOctFile: Couldn't allocate octree nodes.");
            } else {
                fSuccess = FileRead(hOctFile, block, ReadHeader<unsigned long>(header, 0x6a) * 0x24,
                                    &uiRead);
                if (fSuccess == 0) {
                    strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
                }
            }
            g_octree_bytes_read_00659888 += uiRead;
            fLoaded = 0;
            if (fSuccess != 0) {
                block = malloc((ReadHeader<unsigned long>(header, 0x6e) * 5 + 10) * 8);
                m_owned_0a0 = static_cast<W8OctPreTreeLeaf*>(block);
                if (block == 0) {
                    fSuccess = 0;
                    strcpy(acMessage, "ReadOctFile: Couldn't allocate octree leaves.");
                } else {
                    fSuccess = FileRead(hOctFile, block,
                                        ReadHeader<unsigned long>(header, 0x6e) * 0x28, &uiRead);
                    if (fSuccess == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read octree leaves.");
                    }
                }
                g_octree_bytes_read_00659888 += uiRead;
                fLoaded = 0;
                if (fSuccess != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x82) * 4 + 8);
                    m_owned_0d0 = static_cast<unsigned long*>(block);
                    if (block == 0) {
                        fLoaded = 0;
                        strcpy(acMessage,
                               "ReadOctFile: Couldn't allocate polygon index list for leaves.");
                    } else {
                        fLoaded = FileRead(hOctFile, block,
                                           ReadHeader<unsigned long>(header, 0x82) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage,
                                   "ReadOctFile: Couldn't read polygon index list for leaves.");
                        }
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
            }
        }
    }

    limit = m_leaf_grid_dim_x_0a4 * m_leaf_grid_dim_y_0a8 * m_leaf_grid_dim_z_0ac;
    if (fLoaded != 0 && limit < 250000) {
        block = malloc(limit * 4);
        m_owned_0b0 = static_cast<unsigned long*>(block);
        if (block == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate polygon index list for regions.");
            goto finish;
        }
        fLoaded = FileRead(
            hOctFile, block,
            m_leaf_grid_dim_x_0a4 * m_leaf_grid_dim_y_0a8 * m_leaf_grid_dim_z_0ac * 4, &uiRead);
        if (fLoaded == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't read octree nodes.");
        }
        g_octree_bytes_read_00659888 += uiRead;
    } else {
        m_owned_0b0 = 0;
    }

    fSuccess = 0;
    if (fLoaded != 0) {
        block = malloc(ReadHeader<unsigned long>(header, 0x72) * 4 + 8);
        m_aulPolyLookup = static_cast<unsigned long*>(block);
        if (block == 0) {
            fSuccess = 0;
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Poly Lookup table.");
        } else {
            fLoaded =
                FileRead(hOctFile, block, ReadHeader<unsigned long>(header, 0x72) * 4, &uiRead);
            if (fLoaded == 0) {
                strcpy(acMessage, "ReadOctFile: Couldn't read Poly Lookup table.");
            }
            g_octree_bytes_read_00659888 += uiRead;
            fSuccess = 0;
            if (fLoaded != 0) {
                if (ReadHeader<unsigned long>(header, 0x92) != 0) {
                    block = malloc(ReadHeader<unsigned long>(header, 0x92) * 2 + 4);
                    m_owned_148 = static_cast<unsigned short*>(block);
                    if (block == 0) {
                        fSuccess = 0;
                        strcpy(acMessage, "ReadOctFile: Couldn't allocate region list.");
                        goto finish;
                    }
                    fLoaded = FileRead(hOctFile, block, ReadHeader<unsigned long>(header, 0x92) * 2,
                                       &uiRead);
                    if (fLoaded == 0) {
                        strcpy(acMessage, "ReadOctFile: Couldn't read region list.");
                    }
                    g_octree_bytes_read_00659888 += uiRead;
                }
                fSuccess = 0;
                if (fLoaded != 0) {
                    if (ReadHeader<unsigned long>(header, 0x86) != 0) {
                        block = malloc(ReadHeader<unsigned long>(header, 0x86) * 4 + 8);
                        m_owned_12c = static_cast<unsigned long*>(block);
                        if (block == 0) {
                            fSuccess = 0;
                            strcpy(acMessage, "ReadOctFile: Couldn't allocate GD Poly list.");
                            goto finish;
                        }
                        fLoaded = FileRead(hOctFile, block,
                                           ReadHeader<unsigned long>(header, 0x86) * 4, &uiRead);
                        if (fLoaded == 0) {
                            strcpy(acMessage, "ReadOctFile: Couldn't read GD Poly list.");
                        }
                        g_octree_bytes_read_00659888 += uiRead;
                    }
                    fSuccess = 0;
                    if (fLoaded != 0) {
                        if (ReadHeader<unsigned long>(header, 0x8a) != 0) {
                            block = malloc(ReadHeader<unsigned long>(header, 0x8a) * 2 + 4);
                            m_owned_130 = block;
                            if (block == 0) {
                                fSuccess = 0;
                                strcpy(acMessage, "ReadOctFile: Couldn't allocate Trigger list.");
                                goto finish;
                            }
                            fLoaded =
                                FileRead(hOctFile, block,
                                         ReadHeader<unsigned long>(header, 0x8a) * 2, &uiRead);
                            if (fLoaded == 0) {
                                strcpy(acMessage, "ReadOctFile: Couldn't read Trigger list.");
                            }
                            g_octree_bytes_read_00659888 += uiRead;
                        }
                        fSuccess = 0;
                        if (fLoaded != 0) {
                            if (ReadHeader<unsigned short>(header, 0x96) > 1) {
                                block =
                                    malloc((ReadHeader<unsigned short>(header, 0x96) + 2) * 0xe8);
                                spatial_000.owned_5c = static_cast<W8OctRegionVolume*>(block);
                                if (block == 0) {
                                    fSuccess = 0;
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't allocate region array.");
                                    goto finish;
                                }
                                fLoaded = FileRead(hOctFile, block,
                                                   ReadHeader<unsigned short>(header, 0x96) * 0xe8,
                                                   &uiRead);
                                if (fLoaded == 0) {
                                    strcpy(acMessage, "ReadOctFile: Couldn't read region array.");
                                }
                                g_octree_bytes_read_00659888 += uiRead;
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
                                    if (ReadHeader<unsigned long>(header, 0x66) != 0) {
                                        block = malloc(
                                            (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10);
                                        m_pSubmeshes = static_cast<W8OctSubmesh*>(block);
                                        if (block == 0) {
                                            fLoaded = 0;
                                            strcpy(acMessage,
                                                   "ReadOctFile: Couldn't allocate submesh array.");
                                        } else {
                                            fLoaded = FileRead(
                                                hOctFile, block,
                                                (ReadHeader<unsigned long>(header, 0x66) + 1) *
                                                    0x10,
                                                &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read submesh array.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            if (fLoaded != 0) {
                                                unsigned int* scan;
                                                unsigned int remaining;

                                                limit = 0;
                                                scan = &m_pSubmeshes[0].polygon_count_0c;
                                                remaining =
                                                    ReadHeader<unsigned long>(header, 0x66) + 1;
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
                                                g_octree_storage_00659770 =
                                                    static_cast<unsigned int*>(malloc(limit * 4));
                                                if (g_octree_storage_00659770 == 0) {
                                                    fLoaded = 0;
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't allocate polygon "
                                                           "index list for regions.");
                                                } else {
                                                    for (index = 0; index < limit; ++index) {
                                                        g_octree_storage_00659770[index] = index;
                                                    }
                                                }
                                            }
                                        }
                                        if (m_meshCount_1b4 != 0) {
                                            m_pAlphaBits = new BitArray(m_meshCount_1b4);
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
                                                malloc(m_meshCount_1b4 * 2 + 2));
                                            if (m_pusMeshParticleLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticleLookup", OCTREE_CPP, 0x191,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshParticleLookup,
                                                               m_meshCount_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshParticles = static_cast<unsigned short*>(
                                                malloc(m_usMeshParticlesLen_0e8 * 2));
                                            if (m_pusMeshParticles == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticles", OCTREE_CPP, 0x198,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Link Table.");
                                            }
                                            fLoaded =
                                                FileRead(hOctFile, m_pusMeshParticles,
                                                         m_usMeshParticlesLen_0e8 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumProps != 0) {
                                            m_pusMeshPropLookup = static_cast<unsigned short*>(
                                                malloc(m_meshCount_1b4 * 2 + 2));
                                            if (m_pusMeshPropLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshPropLookup", OCTREE_CPP, 0x1a1,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshPropLookup,
                                                               m_meshCount_1b4 * 2 + 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                            g_octree_bytes_read_00659888 += uiRead;
                                            m_pusMeshProps = static_cast<unsigned short*>(
                                                malloc(m_usMeshPropsLen_0f4 * 2));
                                            if (m_pusMeshProps == 0) {
                                                srAssertFail(
                                                    "m_pusMeshProps", OCTREE_CPP, 0x1a8,
                                                    "ReadOctFile: Couldn't allocate Mesh Prop Link "
                                                    "Table.");
                                            }
                                            fLoaded = FileRead(hOctFile, m_pusMeshProps,
                                                               m_usMeshPropsLen_0f4 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
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
                                            if (ReadHeader<unsigned long>(header, 0x7e) != 0) {
                                                pathing_180 = new W8PathingService();
                                                if (pathing_180 == 0) {
                                                    goto finish;
                                                }
                                                pathing_180->ConfigureForLevel(
                                                    ReadHeader<unsigned long>(header, 0x7e),
                                                    static_cast<float>(
                                                        ReadHeader<unsigned long>(header, 0xac)),
                                                    ReadHeader<unsigned long>(header, 0xb4),
                                                    // reinterpret-ok: packed octree header stores a float vector at byte offset 0x0e
                                                    reinterpret_cast<const float*>(header + 0x0e),
                                                    m_owned_0c0);
                                                fLoaded = pathing_180->Load00458CE0(hOctFile);
                                            }
                                            fSuccess = 0;
                                            if (fLoaded != 0) {
                                                if (m_ulNumProps != 0 &&
                                                    ReadHeader<char>(header, 0xb8) != 0) {
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
    g_octree_game_data_00652db0 = 0;
    *game_data = 0;
    if (fSuccess != 0 && ReadHeader<unsigned long>(header, 0x86) != 0) {
        pGameData = new W8GameData(hOctFile, false);
        if (pGameData == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate submesh array.");
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
        g_octree_6598a4 = this;
        pGameData->positional_04 = this;
        *game_data = pGameData;
        g_octree_game_data_00652db0 = pGameData;
        ReadRegionLinkFile(m_owned_0c0);
        LoadPointFiles(m_owned_0c0);
        if (pathing_180 != 0) {
            pathing_180->ReadWaypointFile00459650();
        }
        return;
    }
failed:
    g_octree_6598a4 = 0;
    spatial_000.flags_00 |= 0x80000000;
}

/* Answer whether this level's OCT/PVL/WGD/LVL set is present, current, and
   newer than the raw sources. Negative means skip the octree, zero means the
   set is ready, and positive means the preprocessed files should be rebuilt. */
// FUNCTION: WIZ8 0x0042ccc0
int CheckLevelAssetSet0042CCC0(const char* level_path)
{
    unsigned short version;
    char copy[260];
    char pvl_path[260];
    char wgd_path[260];
    char lvl_path[260];
    int rebuild = 0;
    int file;
    char* extension;

    if (g_flag_6598a8 != 0) {
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
        if (version < 0x22) {
            if (g_flag_6598a8 != 0) {
                return -1;
            }
            rebuild = 1;
            FileClose(file);
        } else {
            if (version > 0x22) {
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
        ReportStartupMessage004969D0(
            "Could not find WGD file. Cannot find or build current preprocessed files.");
        ReportStartupMessage004969D0("Attempting to run with LVL file only -- NO COLLISION DATA.");
        ReportStartupMessage004969D0(0);
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

/* The raw 0x29c allocation is reset before ReadOctFile applies its header.
   The image spells the clears individually; memset expresses the same POD
   construction invariant while the positional fields are still unnamed. */
// FUNCTION: WIZ8 0x0042d040
void W8Octree::Reset()
{
    g_octree_storage_00659770 = 0;
    g_octree_state_00659890 = 0;
    spatial_000.Reset0046CDC0();
    memset(this, 0, sizeof(*this));
    current_prop = -1;
}

/* ReadOctFile calls this after Reset. The file header is packed and several
   values are intentionally unaligned, so each read is copied rather than
   reached through an incorrectly aligned C++ record. Positional writes retain
   the proven offsets until the corresponding octree concepts are named. */
// FUNCTION: WIZ8 0x0042d2a0
void W8Octree::Initialize(const void* raw_header)
{
    const unsigned char* header = static_cast<const unsigned char*>(raw_header);
    unsigned int axis;

    if (header != 0) {
        WriteMember(this, 0x04, ReadHeader<unsigned long>(header, 0x02));
        WriteMember(this, 0x08, ReadHeader<unsigned long>(header, 0x06));
        WriteMember(this, 0x70, ReadHeader<unsigned long>(header, 0x0a));
        for (axis = 0; axis < 3; ++axis) {
            WriteMember(this, 0x27c + axis * 4, 0UL);
            WriteMember(this, 0x288 + axis * 4, 0UL);
            WriteMember(this, 0x0c + axis * 4, ReadHeader<unsigned long>(header, 0x0e + axis * 4));
            WriteMember(this, 0x18 + axis * 4, ReadHeader<unsigned long>(header, 0x1a + axis * 4));
            WriteMember(this, 0x24 + axis * 4, ReadHeader<unsigned long>(header, 0x26 + axis * 4));
            WriteMember(this, 0x30 + axis * 4, ReadHeader<unsigned long>(header, 0x32 + axis * 4));
            WriteMember(this, 0x78 + axis * 4, ReadHeader<unsigned long>(header, 0x3e + axis * 4));
            WriteMember(this, 0x84 + axis * 4, ReadHeader<unsigned long>(header, 0x4a + axis * 4));
            WriteMember(this, 0xa4 + axis * 4, ReadHeader<unsigned long>(header, 0x56 + axis * 4));
        }

        WriteMember(this, 0x64, m_leaf_grid_dim_y_0a8 * m_leaf_grid_dim_z_0ac);
        WriteMember(this, 0x68, m_leaf_grid_dim_z_0ac);
        WriteMember(this, 0x44, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x58, ReadHeader<unsigned short>(header, 0x64));
        WriteMember(this, 0x46, ReadHeader<unsigned short>(header, 0x60));
        WriteMember(this, 0x52, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x74, ReadHeader<unsigned long>(header, 0x66));
        m_root_mesh_count_1a8 = ReadHeader<unsigned long>(header, 0x9a);
        m_kind1_submesh_count_1ac = ReadHeader<unsigned long>(header, 0xa2);
        m_meshCount_1b4 = ReadHeader<unsigned long>(header, 0x9e);
        WriteMember(this, 0xb4, ReadHeader<unsigned long>(header, 0x6a));
        WriteMember(this, 0xb8, ReadHeader<unsigned long>(header, 0x6e));
        WriteMember(this, 0x3c, ReadHeader<unsigned long>(header, 0x72));
        WriteMember(this, 0xc8, ReadHeader<unsigned long>(header, 0x76));
        WriteMember(this, 0x40, ReadHeader<unsigned long>(header, 0x7a));
        m_leaf_polygon_stream_len_0cc = ReadHeader<unsigned long>(header, 0x82);
        m_gd_surface_stream_len_124 = ReadHeader<unsigned long>(header, 0x86);
        m_trigger_count_128 = ReadHeader<unsigned long>(header, 0x8a);
        m_region_list_len_138 = ReadHeader<unsigned long>(header, 0x92);
        WriteMember(this, 0x54, ReadHeader<unsigned long>(header, 0xa6));
        m_region_cell_178 = ReadHeader<float>(header, 0xac);
        m_path_clearance_17c = ReadHeader<unsigned long>(header, 0xb4);
        WriteMember(this, 0x60, ReadHeader<unsigned long>(header, 0xb9));
        m_ulNumProps = ReadHeader<unsigned long>(header, 0xbd);
        m_usMeshParticlesLen_0e8 = ReadHeader<unsigned short>(header, 0xc5);
        m_usMeshPropsLen_0f4 = ReadHeader<unsigned short>(header, 0xc7);
        m_ulNumParticles = ReadHeader<unsigned long>(header, 0xc1);

        if (m_ulNumParticles != 0) {
            m_linked_particles_0fc = new BitArray(m_ulNumParticles);
            m_visible_particles_100 = new BitArray(m_ulNumParticles);
            m_particles_to_disable_10c = new BitArray(m_ulNumParticles);
            m_papParticles = static_cast<stParticle**>(malloc(m_ulNumParticles * 4 + 8));
        }
        if (m_ulNumProps != 0) {
            m_linked_props_104 = new BitArray(m_ulNumProps);
            m_visible_props_108 = new BitArray(m_ulNumProps);
            m_props_to_disable_110 = new BitArray(m_ulNumProps);
            m_papProps = static_cast<W8Prop**>(malloc(m_ulNumProps * 4 + 8));
        }

        m_owned_190 = new BitArray(ReadHeader<unsigned long>(header, 0x72));
        m_owned_154 = new BitArray(ReadHeader<unsigned short>(header, 0x64) + 1);
        m_projected_regions_15c = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_current_regions_160 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_previous_regions_164 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_194 = new BitArray(ReadHeader<unsigned long>(header, 0x7a) < 5000
                                       ? 5000
                                       : ReadHeader<unsigned long>(header, 0x7a));
        m_accumulated_regions_198 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_19c = new BitArray(ReadHeader<unsigned long>(header, 0x72));

        object_registry = new W8OctreeObjectRegistry;
        m_pRegionLinks_150 = new W8HashTable<unsigned int, unsigned short>;

        unsigned int visited_size = ReadHeader<unsigned short>(header, 0x64) + 1;
        m_pfRegsVisited = static_cast<unsigned char*>(malloc(visited_size));
        if (m_pfRegsVisited == 0) {
            srAssertFail("m_pfRegsVisited", "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                         0x36a, "InitOctree: Couldn't allocate m_pfRegsVisited.");
        }
        memset(m_pfRegsVisited, 0, visited_size);
        m_reset_visibility_168 = 1;
    }

    m_aulGDObjs = static_cast<unsigned long*>(malloc(40000));
    if (m_aulGDObjs == 0) {
        srAssertFail("m_aulGDObjs", "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", 0x372,
                     "InitOctree: Couldn't allocate m_aulGDObjs.");
    }
    WriteMember(this, 0x6c, static_cast<unsigned short>(3));
    m_fAccumulating = 1;
    ToggleUpdateSuspension00434020(0);
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
    if (m_positional_16c != 0) {
        SaveRegionLinks004331F0(m_owned_0c0);
    }
    if (m_positional_16d != 0) {
        SavePoints00432D60(m_owned_0c0);
    }
    if (pathing_180 != 0) {
        pathing_180->SaveWaypointSnapshot00459400(0);
    }

    free(m_aulGDObjs);
    free(m_owned_09c);
    free(m_owned_0a0);
    free(m_owned_0d0);
    free(m_aulPolyLookup);
    DestroyBitArray(m_pAlphaBits);
    free(m_owned_0b0);
    free(m_owned_12c);
    free(m_owned_148);
    free(m_owned_130);

    if (m_pRegionLinks_150 != 0) {
        m_pRegionLinks_150->Clear();
        delete m_pRegionLinks_150;
        m_pRegionLinks_150 = 0;
    }

    free(m_pfRegsVisited);
    DestroyBitArray(m_owned_190);
    DestroyBitArray(m_owned_154);
    DestroyBitArray(m_projected_regions_15c);
    DestroyBitArray(m_current_regions_160);
    DestroyBitArray(m_previous_regions_164);

    free(m_pusMeshParticleLookup);
    m_pusMeshParticleLookup = 0;
    free(m_pusMeshParticles);
    m_pusMeshParticles = 0;
    free(m_pusMeshPropLookup);
    m_pusMeshPropLookup = 0;
    free(m_pusMeshProps);
    m_pusMeshProps = 0;
    free(m_papProps);
    free(m_papParticles);

    DestroyBitArray(m_linked_particles_0fc);
    DestroyBitArray(m_visible_particles_100);
    DestroyBitArray(m_linked_props_104);
    DestroyBitArray(m_visible_props_108);
    DestroyBitArray(m_particles_to_disable_10c);
    DestroyBitArray(m_props_to_disable_110);

    m_positional_170 = 0;
    if (m_sr_owned_174 != 0) {
        srHeap.free(m_sr_owned_174);
    }
    free(m_owned_0c0);
    DestroyBitArray(m_owned_194);
    DestroyBitArray(m_accumulated_regions_198);
    DestroyBitArray(m_owned_19c);
    DestroyBitArray(m_owned_1a0);
    DestroyBitArray(m_owned_1a4);

    delete object_registry;

    free(m_pSubmeshes);
    if (pathing_180 != 0) {
        delete pathing_180;
        pathing_180 = 0;
    }
    DestroyBitArray(m_pPropSunBits);
}

/* Running index into the prop-sunlight bit stream, shared across the octree
   so consecutive levels pick up where the previous one's props ended. */

// GLOBAL: WIZ8 0x006598ac
int g_prop_sun_index_006598ac;

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
int W8Octree::TestPropSunBit(int offset)
{
    if (offset < 0) {
        prop_sun_base_184 = g_prop_sun_index_006598ac;
        return 0;
    }
    g_prop_sun_index_006598ac = prop_sun_base_184 + offset;
    if (m_pPropSunBits != 0 && m_pPropSunBits->Test(g_prop_sun_index_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8Octree::VisitPointCopy0042E620(unsigned short location_id, srVector3T<float>* position)
{
    srVector3T<float> copy;

    copy.x = position->x;
    copy.y = position->y;
    copy.z = position->z;
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
    W8OctreeIndex* by_object = object_registry->by_object;
    W8OctreeIndex* by_cell = object_registry->by_cell;
    unsigned int id = (location_id + 1) & 0xffff;
    unsigned int object_key = PackOctreeObjectKey(W8_OCTREE_KIND_LOCATION, id);
    int slot = by_object->FindNextEntry(&object_key, -1);
    while (slot != -1) {
        unsigned int cell_key = static_cast<unsigned int>(by_object->entries[slot].value);
        int object_value = static_cast<int>(object_key);
        by_object->RemoveAt(slot);
        by_cell->Remove(&cell_key, &object_value);
        slot = by_object->FindNextEntry(&object_key, slot);
    }
    object_key = PackOctreeObjectKey(W8_OCTREE_KIND_NAVIGATOR, id);
    slot = by_object->FindNextEntry(&object_key, -1);
    while (slot != -1) {
        unsigned int cell_key = static_cast<unsigned int>(by_object->entries[slot].value);
        int object_value = static_cast<int>(object_key);
        by_object->RemoveAt(slot);
        by_cell->Remove(&cell_key, &object_value);
        slot = by_object->FindNextEntry(&object_key, slot);
    }
}

/* Remove the registered pairings for one (location, kind) object key — used
   by Navigator teardown with kind 0xd.  The removed-slot cursor reuse is
   retail verified (the same sequence is inlined at 0x0042E880): FindNextEntry
   follows entries[slot].next_index after RemoveAt has overwritten it with
   the free-list head. */
// FUNCTION: WIZ8 0x0042e880
void W8Octree::UnregisterLocationObject(unsigned int location_id, int kind)
{
    W8OctreeIndex* by_object = object_registry->by_object;
    W8OctreeIndex* by_cell = object_registry->by_cell;
    unsigned int object_key = PackOctreeObjectKey(kind, location_id + 1);
    int slot = by_object->FindNextEntry(&object_key, -1);
    while (slot != -1) {
        unsigned int cell_key = static_cast<unsigned int>(by_object->entries[slot].value);
        int object_value = static_cast<int>(object_key);
        by_object->RemoveAt(slot);
        by_cell->Remove(&cell_key, &object_value);
        slot = by_object->FindNextEntry(&object_key, slot);
    }
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
    int low_cell[3];
    int high_cell[3];
    int cell[3];

    g_octree_state_00659890 = *results;
    if (g_octree_state_00659890 == 0) {
        g_octree_state_00659890 = m_aulGDObjs;
        *results = g_octree_state_00659890;
    }
    m_gd_result_count_1b8 = 0;
    float radius = delta->Length();
    if (extent < radius) {
        extent = radius;
    }
    for (int axis = 0; axis < 3; ++axis) {
        float bound = (&origin->x)[axis] - extent;
        if ((&delta->x)[axis] <= g_float_005ebb34) {
            (&low.x)[axis] = bound + (&delta->x)[axis];
            bound = extent + (&origin->x)[axis];
        } else {
            (&low.x)[axis] = bound;
            bound = extent + (&origin->x)[axis] + (&delta->x)[axis];
        }
        (&high.x)[axis] = bound;
    }
    m_owned_194->ClearAll();
    CollectVisibleRegions00430D50(&low, low_cell, 0, 0);
    CollectVisibleRegions00430D50(&high, high_cell, 0, 0);
    if (low_cell[0] <= high_cell[0]) {
        cell[0] = low_cell[0];
        do {
            if (cell[0] >= 0 && cell[0] < static_cast<int>(m_leaf_grid_dim_x_0a4) /* c-style-cast-ok:
                    cell coordinate vs grid dimension */) {
                cell[1] = low_cell[1];
                while (cell[1] <= high_cell[1]) {
                    if (cell[1] >= 0 && cell[1] < static_cast<int>(m_leaf_grid_dim_y_0a8) /* c-style-cast-ok:
                            cell coordinate vs grid dimension */) {
                        for (cell[2] = low_cell[2]; cell[2] <= high_cell[2]; ++cell[2]) {
                            if (cell[2] >= 0 && cell[2] < static_cast<int>(m_leaf_grid_dim_z_0ac)
                                /* c-style-cast-ok: cell coordinate vs grid dimension */) {
                                CollectObjectsInCell(cell, kind);
                            }
                        }
                    }
                    ++cell[1];
                }
            }
            ++cell[0];
        } while (cell[0] <= high_cell[0]);
    }
    g_octree_state_00659890 = 0;
    return static_cast<int>(m_gd_result_count_1b8); /* c-style-cast-ok: the shared count field is
        stored unsigned */
}

/* The kind-12 convenience query: a zero exclusion maps to -1 (none). OctPath
   uses it to list the location ids near a mover. */

// FUNCTION: WIZ8 0x0042ef00
unsigned int W8Octree::QueryLocationsInBox(unsigned long** results, const srVector3T<float>* lower,
                                           const srVector3T<float>* upper, unsigned short exclusion)
{
    unsigned int excluded = 0xffffffff;
    if (exclusion != 0) {
        excluded = exclusion;
    }
    return static_cast<unsigned int>(
        QueryObjects(results, lower, upper, W8_OCTREE_KIND_LOCATION,
                     static_cast<int>(excluded))); /* c-style-cast-ok: the
        exclusion id travels as a signed value so -1 can mean none */
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
    unsigned int index = 0;
    if (count != 0) {
        do {
            W8GDSurface* surface = g_octree_game_data_00652db0->m_pSurfaces + objects[index];
            srVector3T<float> bounds[2];
            srVector3T<float> triangle[3];
            for (int axis = 0; axis < 3; ++axis) {
                (&bounds[0].x)[axis] = (&lower->x)[axis];
                (&bounds[1].x)[axis] = (&upper->x)[axis];
                (&triangle[0].x)[axis] =
                    (&g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[0]]
                          .x)[axis];
                (&triangle[1].x)[axis] =
                    (&g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[1]]
                          .x)[axis];
                (&triangle[2].x)[axis] =
                    (&g_octree_game_data_00652db0->m_pVertices[surface->vertex_indices_18[2]]
                          .x)[axis];
            }
            if (TestSpatialTriangle0046CE60(bounds, triangle, surface->Normal()) != 0) {
                return 1;
            }
            ++index;
        } while (index < count);
    }
    count = static_cast<unsigned int>(
        QueryObjects(&objects, lower, upper, W8_OCTREE_KIND_LOCATION, -1));
    if (count != 0) {
        index = 0;
        do {
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x62d, "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp", objects[index], 1);
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (info != 0 && info->monster != 0) {
                srVector3T<float> position = info->monster->GetPosition();
                float radius = info->monster->radius_084;
                if (lower->x - radius < position.x && position.x < radius + upper->x &&
                    lower->y - radius < position.y && position.y < radius + upper->y &&
                    lower->z - radius < position.z && position.z < radius + upper->z) {
                    return 1;
                }
            }
            ++index;
        } while (index < count);
    }
    count =
        static_cast<unsigned int>(QueryObjects(&objects, lower, upper, W8_OCTREE_KIND_PROP, -1));
    if (count != 0) {
        index = 0;
        do {
            W8Prop* prop = *g_world->collidable_props->GetAt(objects[index]);
            if (prop->GetSetting6C() != 0 && prop->m_gd_prop != 0) {
                GDProp* gd_prop = prop->m_gd_prop;
                for (int surface_index = 0; surface_index < gd_prop->m_surface_count_14;
                     ++surface_index) {
                    W8GDSurface* surface = gd_prop->m_pGDSurfaces + surface_index;
                    srVector3T<float> bounds[2];
                    srVector3T<float> triangle[3];
                    for (int axis = 0; axis < 3; ++axis) {
                        (&bounds[0].x)[axis] = (&lower->x)[axis];
                        (&bounds[1].x)[axis] = (&upper->x)[axis];
                        (&triangle[0].x)[axis] =
                            (&gd_prop->m_pVertices[surface->vertex_indices_18[0]].x)[axis];
                        (&triangle[1].x)[axis] =
                            (&gd_prop->m_pVertices[surface->vertex_indices_18[1]].x)[axis];
                        (&triangle[2].x)[axis] =
                            (&gd_prop->m_pVertices[surface->vertex_indices_18[2]].x)[axis];
                    }
                    if (TestSpatialTriangle0046CE60(bounds, triangle, surface->Normal()) != 0) {
                        return 1;
                    }
                }
            }
            ++index;
        } while (index < count);
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
    int start[3];
    int end[3];
    int cell[3];

    g_octree_state_00659890 = *objects;
    if (g_octree_state_00659890 == 0) {
        g_octree_state_00659890 = m_aulGDObjs;
        *objects = g_octree_state_00659890;
    }
    m_gd_result_count_1b8 = 0;
    m_owned_194->ClearAll();
    if (excluded >= 0) {
        m_owned_194->Set(excluded);
    }
    start[0] =
        static_cast<int>(((lower->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    start[1] =
        static_cast<int>(((lower->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    start[2] =
        static_cast<int>(((lower->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    end[0] = static_cast<int>(((upper->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    end[1] = static_cast<int>(((upper->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    end[2] = static_cast<int>(((upper->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    for (cell[0] = start[0]; cell[0] <= end[0]; ++cell[0]) {
        if (cell[0] < 0 || static_cast<int>(m_leaf_grid_dim_x_0a4) <= cell[0] /* c-style-cast-ok:
                cell coordinate vs grid dimension */) {
            continue;
        }
        for (cell[1] = start[1]; cell[1] <= end[1]; ++cell[1]) {
            if (cell[1] < 0 || static_cast<int>(m_leaf_grid_dim_y_0a8) <= cell[1] /* c-style-cast-ok:
                    cell coordinate vs grid dimension */) {
                continue;
            }
            for (cell[2] = start[2]; cell[2] <= end[2]; ++cell[2]) {
                if (cell[2] >= 0 && cell[2] < static_cast<int>(m_leaf_grid_dim_z_0ac) /* c-style-cast-ok:
                        cell coordinate vs grid dimension */) {
                    CollectObjectsInCell(cell, kind);
                }
            }
        }
    }
    g_octree_state_00659890 = 0;
    return m_gd_result_count_1b8;
}

/* Append the objects of `kind` inside one cell to the shared query buffer.
   Kind 3 walks the static tree (or the direct leaf grid when one exists) to a
   leaf whose gd-polygon stream lists surface ids. Kinds 8, 9 and 12 query the
   object registry's cell hash; kinds above 12 accept both the 12 and 13
   halfwords packed into each registry value. Every append deduplicates
   through m_owned_194 and the buffer stops at 10000 entries. */
// FUNCTION: WIZ8 0x0042f400
unsigned int W8Octree::CollectObjectsInCell(const int* cell, unsigned short kind)
{
    unsigned int found = 0;
    unsigned int leaf_index;
    int slot;

    switch (kind) {
    case W8_OCTREE_KIND_SURFACE:
        if (m_owned_0b0 == 0) {
            unsigned int depth = spatial_000.depth_44;
            unsigned int bit;
            leaf_index = 1;
            bit = 1 << (spatial_000.depth_44 & 0x1f);
            do {
                if (static_cast<int>(depth) < 1) {
                    break;
                }
                bit = bit / 2;
                int octant = 0;
                if ((bit & cell[0]) != 0) {
                    octant = 4;
                }
                if ((cell[1] & bit) != 0) {
                    octant += 2;
                }
                if ((cell[2] & bit) != 0) {
                    octant += 1;
                }
                leaf_index = m_owned_09c[leaf_index].children_04[octant];
                --depth;
            } while (leaf_index != 0);
            if (m_leaf_count_0b8 < leaf_index) {
                leaf_index = 0;
            }
        } else {
            leaf_index = m_owned_0b0[spatial_000.leaf_grid_stride_y_68 * cell[1] +
                                     spatial_000.leaf_grid_stride_x_64 * cell[0] + cell[2]];
        }
        if (leaf_index != 0 &&
            ((unsigned long*)m_owned_0a0)[kind + leaf_index * 10] !=
                0 /* c-style-cast-ok: the leaf record is a ten-dword
                stream-offset table indexed by object kind */) {
            const unsigned long* stream =
                m_owned_12c + ((unsigned long*)m_owned_0a0) /* c-style-cast-ok: the leaf record
                is a ten-dword stream-offset table indexed by object kind */
                                  [kind + leaf_index * 10];
            found = *stream;
            if (found != 0) {
                unsigned int index = 0;
                do {
                    if (9999 < m_gd_result_count_1b8) {
                        return found;
                    }
                    ++stream;
                    if (m_owned_194->Set(*stream) == 0) {
                        g_octree_state_00659890[m_gd_result_count_1b8] = *stream;
                        ++m_gd_result_count_1b8;
                    }
                    ++index;
                } while (index < found);
            }
        }
        break;
    default:
        if (W8_OCTREE_KIND_LOCATION < kind) {
            if (cell != 0) {
                g_octree_query_cell_00659774 = PackOctreeCellKey(cell[0], cell[1], cell[2]);
                g_octree_query_slot_00659760 = -1;
            }
            slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell_00659774,
                                                           g_octree_query_slot_00659760);
            g_octree_query_slot_00659760 = slot;
            if (slot >= 0) {
                unsigned int packed =
                    static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                g_octree_query_kind_00659764 = static_cast<unsigned short>(OctreeKeyKind(packed));
                g_octree_query_id_00659766 = static_cast<unsigned short>(packed);
                while (m_gd_result_count_1b8 < 10000) {
                    short entry_kind = static_cast<short>(OctreeKeyKind(packed));
                    if ((entry_kind == W8_OCTREE_KIND_LOCATION ||
                         entry_kind == W8_OCTREE_KIND_NAVIGATOR) &&
                        m_owned_194->Set(OctreeKeyId(packed) - 1) == 0) {
                        g_octree_state_00659890[m_gd_result_count_1b8] =
                            g_octree_query_id_00659766 - 1;
                        ++m_gd_result_count_1b8;
                    }
                    slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell_00659774,
                                                                   g_octree_query_slot_00659760);
                    g_octree_query_slot_00659760 = slot;
                    if (slot < 0) {
                        return 0;
                    }
                    packed =
                        static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                    g_octree_query_kind_00659764 =
                        static_cast<unsigned short>(OctreeKeyKind(packed));
                    g_octree_query_id_00659766 = static_cast<unsigned short>(packed);
                }
            }
            break;
        }
        /* fall through */
    case W8_OCTREE_KIND_PROP:
    case W8_OCTREE_KIND_WAYPOINT:
    case W8_OCTREE_KIND_LOCATION:
        if (cell != 0) {
            g_octree_query_cell_00659774 = PackOctreeCellKey(cell[0], cell[1], cell[2]);
            g_octree_query_slot_00659760 = -1;
        }
        slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell_00659774,
                                                       g_octree_query_slot_00659760);
        g_octree_query_slot_00659760 = slot;
        if (slot >= 0) {
            unsigned int packed =
                static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
            g_octree_query_kind_00659764 = static_cast<unsigned short>(OctreeKeyKind(packed));
            g_octree_query_id_00659766 = static_cast<unsigned short>(packed);
            while (m_gd_result_count_1b8 < 10000) {
                if ((static_cast<unsigned short>(OctreeKeyKind(packed)) == kind) &&
                    m_owned_194->Set(OctreeKeyId(packed) - 1) == 0) {
                    g_octree_state_00659890[m_gd_result_count_1b8] = g_octree_query_id_00659766 - 1;
                    ++m_gd_result_count_1b8;
                }
                slot = object_registry->by_cell->FindNextEntry(&g_octree_query_cell_00659774,
                                                               g_octree_query_slot_00659760);
                g_octree_query_slot_00659760 = slot;
                if (slot < 0) {
                    return 0;
                }
                packed = static_cast<unsigned int>(object_registry->by_cell->entries[slot].value);
                g_octree_query_kind_00659764 = static_cast<unsigned short>(OctreeKeyKind(packed));
                g_octree_query_id_00659766 = static_cast<unsigned short>(packed);
            }
        }
        break;
    }
    return found;
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void W8Octree::QueueOctreeKind130042E810(int id, const srVector3T<float>* position)
{
    int point[3];

    point[0] =
        static_cast<int>(((position->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70));
    point[1] =
        static_cast<int>(((position->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70));
    point[2] =
        static_cast<int>(((position->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70));
    object_registry->RegisterObjectCell(W8_OCTREE_KIND_NAVIGATOR, id + 1, point);
}

/* Convert a world position to cell coordinates, tracking whether it stays
   inside the spatial minimum/maximum box. Callers only use the coordinates;
   the in-range result the image also computes is not consumed. */
// FUNCTION: WIZ8 0x00431440
int* W8Octree::WorldPositionToCell(const srVector3T<float>* position, int* point)
{
    unsigned char inside = 1;

    for (int axis = 0; axis < 3; ++axis) {
        point[axis] = static_cast<int>((((&position->x)[axis] - (&spatial_000.minimum_0c.x)[axis]) /
                                        spatial_000.node_extent_70));

        if ((&position->x)[axis] < (&spatial_000.minimum_0c.x)[axis] ||
            (&position->x)[axis] > (&spatial_000.maximum_18.x)[axis]) {
            inside = 0;
        }
    }
    return inside ? point : 0;
}

/* Clamp a position under the spatial ceiling, settle it to the ground through
   the surface walk, and keep the settled height only when something was hit.
   The walk reports through the flag; the height it wrote is discarded on a
   miss. */
// FUNCTION: WIZ8 0x00431DA0
void W8Octree::AdjustPosition00431DA0(srVector3T<float>* position, unsigned int mode)
{
    srVector3T<float> adjusted;
    unsigned char hit = 0;

    if (spatial_000.clipped_maximum_30.y < position->y) {
        position->y = spatial_000.clipped_maximum_30.y;
    }
    adjusted = *position;
    SettleToGround(&adjusted, &hit, mode, 500.0f);
    if (hit != 0) {
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
    unsigned char reached = 1;

    if (pathing_180 != 0) {
        if (g_navigator_link_mode_00659c10 == 0) {
            return pathing_180->StepAlongPath004669B0(movement, radius, separation);
        }
        return pathing_180->StepMonsterAlongPath00467150(movement, radius, separation);
    }
    if (g_navigator_link_mode_00659c10 != 0) {
        return 1;
    }
    vecDir = movement->target_position_04c - movement->position_040;
    vecDir.y = 0.0f;
    distance = srVector2T<float>(vecDir.x, vecDir.z).Length();
    step = g_game_time_accumulator_6598bc->GetValue28() * movement->movement_scale_060 *
           g_rate_006068EC * g_world_scale_005ebc40;
    if (step >= distance) {
        step = distance;
    } else {
        reached = 0;
    }
    if (static_cast<double>((vecDir.x * vecDir.x + vecDir.z * vecDir.z)) != g_zero_005ebb40) {
        vecDir.SetLength(step);
    }
    vecPos = vecDir + movement->position_040;
    movement->position_040 = vecPos;
    attachment = movement->attachment_0ac;
    *attachment->position_4c = vecPos;
    attachment->position_34 = *attachment->position_4c;
    attachment->position_10 = *attachment->position_4c;
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
    unsigned char hit;

    if (pathing_180 == 0) {
        return;
    }
    if (pathing_180->flag_1c8 == 0 && pathing_180->m_ulNumWayPoints != 0) {
        return;
    }
    local_destination = *destination;
    local_source = *source;

    hit = 0;
    if (local_destination.y > spatial_000.clipped_maximum_30.y) {
        local_destination.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_destination;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_destination.y = probe.y;
    }

    hit = 0;
    if (local_source.y > spatial_000.clipped_maximum_30.y) {
        local_source.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_source;
    SettleToGround(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_source.y = probe.y;
    }
    pathing_180->EditTeleportalLink(&local_destination, &local_source);
}

/* Release the location-variable names and empty their parallel value and
   level vectors. Trigger.cpp creates the names as copied character arrays. */

/* The .oct writers stage at most 0x100 records through a stack buffer per
   FileWrite call. */

// FUNCTION: WIZ8 0x004372E0
bool WriteVector4Array004372E0(int file, const srVector4T<float>* values, int count)
{
    srVector4T<float> staging[0x100];
    int written = 0;
    int index;
    unsigned char success = 1;

    while (written < count && success != 0) {
        int chunk_count = count - written;
        if (chunk_count > 0x100) {
            chunk_count = 0x100;
        }
        for (index = 0; index < chunk_count; ++index) {
            staging[index] = values[written + index];
        }
        written += chunk_count;
        success &= FileWrite(file, staging, chunk_count * sizeof(staging[0]), 0);
    }
    return success;
}

// FUNCTION: WIZ8 0x00437390
bool WriteVector3Array00437390(int file, const srVector3T<float>* values, int count)
{
    srVector3T<float> staging[0x100];
    int written = 0;
    int index;
    unsigned char success = 1;

    while (written < count) {
        int chunk_count = count - written;
        if (chunk_count > 0x100) {
            chunk_count = 0x100;
        }
        for (index = 0; index < chunk_count; ++index) {
            staging[index] = values[written + index];
        }
        written += chunk_count;
        success &= FileWrite(file, staging, chunk_count * sizeof(staging[0]), 0);
        if (success == 0) {
            return 0;
        }
    }
    return success;
}

// FUNCTION: WIZ8 0x00437430
bool WriteVector2Array00437430(int file, const srVector2T<float>* values, int count)
{
    srVector2T<float> staging[0x100];
    int written = 0;
    int index;
    unsigned char success = 1;

    while (written < count) {
        int chunk_count = count - written;
        if (chunk_count > 0x100) {
            chunk_count = 0x100;
        }
        for (index = 0; index < chunk_count; ++index) {
            staging[index] = values[written + index];
        }
        written += chunk_count;
        success &= FileWrite(file, staging, chunk_count * sizeof(staging[0]), 0);
        if (success == 0) {
            return 0;
        }
    }
    return success;
}

// FUNCTION: WIZ8 0x004374C0
bool ReadVector4Array004374C0(int file, srVector4T<float>* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector4T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x004374E0
bool ReadVector3Array004374E0(int file, srVector3T<float>* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector3T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437510
bool ReadVector2Array00437510(int file, srVector2T<float>* values, int count)
{
    return FileRead(file, values, count * sizeof(srVector2T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437540
float PointToSegmentDistance00437540(srVector3T<float>* point, const srVector3T<float>* from,
                                     const srVector3T<float>* to, char clamp_point, float* out_t)
{
    float dx = to->x - from->x;
    float dy = to->y - from->y;
    float dz = to->z - from->z;
    float offset_x = point->x - from->x;
    float offset_y = point->y - from->y;
    float offset_z = point->z - from->z;
    float t = (offset_x * dx + offset_y * dy + offset_z * dz) / (dx * dx + dy * dy + dz * dz);
    if (static_cast<float>(g_zero_005ebb40) < t) {
        if (t <= static_cast<float>(g_double_005ebc30)) {
            offset_x = offset_x - dx * t;
            offset_y = offset_y - dy * t;
            offset_z = offset_z - dz * t;
        } else {
            offset_x = point->x - to->x;
            offset_y = point->y - to->y;
            offset_z = point->z - to->z;
        }
    }
    if (clamp_point != 0) {
        if (static_cast<float>(g_zero_005ebb40) <= t) {
            if (static_cast<float>(g_double_005ebc30) < t) {
                point->x = to->x;
                point->y = to->y;
                point->z = to->z;
            } else {
                point->x = dx * t + from->x;
                point->y = dy * t + from->y;
                point->z = dz * t + from->z;
            }
        } else {
            point->x = from->x;
            point->y = from->y;
            point->z = from->z;
        }
    }
    if (out_t != 0) {
        if (static_cast<float>(g_zero_005ebb40) <= t) {
            if (t <= static_cast<float>(g_double_005ebc30)) {
                *out_t = t;
            } else {
                *out_t = 1.0f;
            }
        } else {
            *out_t = 0.0f;
        }
    }
    return static_cast<float>(
        sqrt(offset_x * offset_x + offset_z * offset_z + offset_y * offset_y));
}

// FUNCTION: WIZ8 0x00437760
float PointToSegmentDistance2D00437760(float* point, const float* from, const float* to,
                                       char clamp_point, float* out_t)
{
    float dx = to[0] - from[0];
    float dy = to[1] - from[1];
    float offset_x = point[0] - from[0];
    float offset_y = point[1] - from[1];
    float t = (offset_x * dx + offset_y * dy) / (dx * dx + dy * dy);
    if (static_cast<float>(g_zero_005ebb40) < t) {
        if (t <= static_cast<float>(g_double_005ebc30)) {
            offset_x = offset_x - dx * t;
            offset_y = offset_y - dy * t;
        } else {
            offset_x = point[0] - to[0];
            offset_y = point[1] - to[1];
        }
    }
    if (clamp_point != 0) {
        if (static_cast<float>(g_zero_005ebb40) <= t) {
            if (static_cast<float>(g_double_005ebc30) < t) {
                point[0] = to[0];
                point[1] = to[1];
            } else {
                point[0] = dx * t + from[0];
                point[1] = dy * t + from[1];
            }
        } else {
            point[0] = from[0];
            point[1] = from[1];
        }
    }
    if (out_t != 0) {
        if (static_cast<float>(g_zero_005ebb40) <= t) {
            if (t <= static_cast<float>(g_double_005ebc30)) {
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
   moved. Retail's second comparison tests point[2] where point[1] is meant
   (0x0043790F FCOMPs [ECX+8]) - the proven quirk stays. */
// FUNCTION: WIZ8 0x004378f0
char GrowBoundsByPoint(const float* point, float* minimum, float* maximum)
{
    char changed = 0;
    if (*minimum > *point) {
        *minimum = *point;
        changed = 1;
    }
    if (minimum[1] > point[2]) {
        minimum[1] = point[1];
        changed = 1;
    }
    if (minimum[2] > point[2]) {
        minimum[2] = point[2];
        changed = 1;
    }
    if (*maximum < *point) {
        *maximum = *point;
        changed = 1;
    }
    if (maximum[1] < point[1]) {
        maximum[1] = point[1];
        changed = 1;
    }
    if (maximum[2] < point[2]) {
        maximum[2] = point[2];
        return 1;
    }
    return changed;
}

/* Whether `point` lies within `radius` of the six-float bounds box, as the
   squared distance from the box-clamped point. Retail's z clamp picks
   bounds[2] (the minimum) where bounds[5] is meant when the point sits above
   the box (0x0043871B FLDs [ECX+8]) - the proven quirk stays. */
// FUNCTION: WIZ8 0x004386a0
char SphereNearBounds(const float* point, float radius, const float* bounds)
{
    float closest_x;
    float closest_y;
    float closest_z;
    if (point[0] < bounds[0]) {
        closest_x = bounds[0];
    } else if (point[0] <= bounds[3]) {
        closest_x = point[0];
    } else {
        closest_x = bounds[3];
    }
    if (point[1] < bounds[1]) {
        closest_y = bounds[1];
    } else if (point[1] <= bounds[4]) {
        closest_y = point[1];
    } else {
        closest_y = bounds[4];
    }
    if (point[2] < bounds[2]) {
        closest_z = bounds[2];
    } else if (point[2] < bounds[5]) {
        closest_z = point[2];
    } else {
        closest_z = bounds[2];
    }
    float distance_squared = (point[1] - closest_y) * (point[1] - closest_y) +
                             (point[0] - closest_x) * (point[0] - closest_x) +
                             (point[2] - closest_z) * (point[2] - closest_z);
    return distance_squared < radius * radius;
}

/* Lateral column offsets for the scatter search below: the inner loop walks
   three columns (0, +1, -1) per ring, or all five when more than ten
   positions are wanted. */
// GLOBAL: WIZ8 0x00605ae8
static const float g_scatter_column_offsets_00605ae8[5] = {0.0f, 1.0f, -1.0f, 2.0f, -2.0f};

/* Ring-scatter search for up to `count` clear positions around `position`.
   Each ring steps `spacing` units out along the `yaw`-rotated frame and
   columns offset laterally through g_scatter_column_offsets_00605ae8; the
   first accepted candidate becomes the reference the remaining candidates
   must span to. With `proximity_check` set each candidate is also rejected
   inside the startup navigator's or any nearby monster's radius. `flatten_y`
   relaxes the tight vertical bound during search and rewrites every accepted
   y to the original source height afterwards. */
// FUNCTION: WIZ8 0x00437980
unsigned int W8Octree::FindScatterPositions00437980(const srVector3T<float>* position, float yaw,
                                                    float spacing, unsigned int count,
                                                    srVector3T<float>* positions,
                                                    char proximity_check, char flatten_y)
{
    float source_y = position->y;
    unsigned int found = 0;
    unsigned long* candidates = 0;
    unsigned int columns = 3;
    if (count > 10) {
        columns = 5;
    }
    float angle = NormalizeAngle(yaw + g_monster_rotation_offset_005ec04c);
    float cos_angle = static_cast<float>(cos(static_cast<double>(angle)));
    srVector3T<float> source;
    source = *position;
    float sin_angle = static_cast<float>(sin(static_cast<double>(angle)));
    float cos_step = spacing * cos_angle;
    float sin_step = spacing * sin_angle;
    float neg_cos_step = -cos_step;
    unsigned int monsters = 0;
    if (proximity_check != 0) {
        float expand = spacing * g_float_005ec048;
        srVector3T<float> low;
        low.x = position->x - expand;
        low.y = position->y - expand;
        low.z = position->z - expand;
        srVector3T<float> high;
        high.x = expand + position->x;
        high.y = expand + position->y;
        high.z = expand + position->z;
        candidates = static_cast<unsigned long*>(operator new(0x400));
        monsters = static_cast<unsigned int>(QueryObjects(
            &candidates, &low, &high, W8_OCTREE_KIND_LOCATION, -1)); /* c-style-cast-ok:
            the shared query count field is stored unsigned */
    }
    unsigned int ring = 0;
    do {
        if (count <= found) {
            break;
        }
        if (columns != 0) {
            const float* column_offset = g_scatter_column_offsets_00605ae8;
            unsigned int column = 0;
            do {
                if (count <= found) {
                    break;
                }
                float jitter_ring;
                float jitter_column;
                if (found == 0) {
                    jitter_ring = 0.0f;
                    jitter_column = g_float_005ebb34;
                } else {
                    jitter_ring = static_cast<float>(Random(1000)) * g_float_005ec044 -
                                  g_generator_jitter_fraction;
                    jitter_column = static_cast<float>(Random(1000)) * g_float_005ec044 -
                                    g_generator_jitter_fraction;
                }
                srVector3T<float> candidate;
                candidate.x = sin_step * (static_cast<float>(ring) + jitter_ring) + source.x +
                              neg_cos_step * (jitter_column + *column_offset);
                candidate.y = source.y + g_world_scale_005ebc40;
                candidate.z = (static_cast<float>(ring) + jitter_ring) * cos_step + source.z +
                              (jitter_column + *column_offset) * sin_step;
                float ground = SettlePositionToGround00420BD0(&candidate, 0);
                float height = candidate.y - ground;
                if (static_cast<float>(g_double_005ebc30) <= fabsf(height) &&
                    fabsf(height) <= static_cast<float>(g_double_005ec038) &&
                    (flatten_y != 0 || fabsf(height) <= static_cast<float>(g_double_005ec030))) {
                    candidate.y = ground;
                    if (proximity_check == 0) {
                        if (pathing_180->SnapWaypointPosition00462E60(&candidate, 0) != 0) {
                            goto accept;
                        }
                    } else {
                        float reach = spacing * g_float_005ebc7c;
                        if (pathing_180->TestPathCellClearance00463040(&candidate, reach, 0) != 0) {
                            srVector3T<float> navigator = g_startup_world_659c0c->GetPosition();
                            float dx = navigator.x - candidate.x;
                            float dy = navigator.y - candidate.y;
                            float dz = navigator.z - candidate.z;
                            float separation = g_startup_world_659c0c->radius_084 + reach;
                            if (separation * separation <= dx * dx + dy * dy + dz * dz) {
                                unsigned int m = 0;
                                if (monsters != 0) {
                                    do {
                                        W8Monster* monster = GetMonsterByLocationID(candidates[m]);
                                        srVector3T<float> monster_position = monster->GetPosition();
                                        float mdx = monster_position.x - candidate.x;
                                        float mdy = monster_position.y - candidate.y;
                                        float mdz = monster_position.z - candidate.z;
                                        float clearance = monster->radius_084 + reach;
                                        if (mdx * mdx + mdy * mdy + mdz * mdz <
                                            clearance * clearance) {
                                            goto next_cell;
                                        }
                                        ++m;
                                    } while (m < monsters);
                                }
                                goto accept;
                            }
                        }
                    }
                    goto next_cell;
                accept:
                    if (found == 0) {
                        positions[0] = candidate;
                        source.x = candidate.x;
                        source.y = candidate.y;
                        source.z = candidate.z;
                        found = 1;
                    } else if (pathing_180 == 0 || pathing_180->TestWaypointSpan0045A1B0(
                                                       &candidate, &source, 0, 0) != 0) {
                        positions[found] = candidate;
                        ++found;
                    }
                }
            next_cell:
                ++column;
                ++column_offset;
            } while (column < columns);
        }
        ++ring;
    } while (ring < 10);
    if (candidates != 0) {
        delete[] candidates;
    }
    if (flatten_y != 0 && found != 0) {
        for (unsigned int index = 0; index < found; ++index) {
            positions[index].y = source_y;
        }
    }
    return found;
}

// FUNCTION: WIZ8 0x00437f30
unsigned int W8Octree::FindNavigatorPosition(srVector3T<float>* source, float yaw, float radius,
                                             unsigned int count, srVector3T<float>* positions,
                                             char first_only, char flag_2, char flag_3, int mode,
                                             char flag_4)
{
    unsigned int found = 0;
    int found_i = 9999;
    int found_j = 9999;
    float source_y = source->y;
    unsigned long* candidates = 0;
    float camera_radius = g_startup_world_659c0c->movement_0c0.alternate_radius_0b4;
    bool placed = false;
    float separation =
        (CalcRangeDistance(W8_RANGE_TOUCH) + radius) * g_float_005ebc7c + camera_radius;
    float angle = NormalizeAngle(yaw + g_monster_rotation_offset_005ec04c);
    float cos_angle = static_cast<float>(cos(static_cast<double>(angle)));
    float sin_angle = static_cast<float>(sin(static_cast<double>(angle)));
    float cos_radius = radius * cos_angle;
    float sin_radius = radius * sin_angle;
    float neg_cos_radius = -cos_radius;
    source->y = source->y + g_world_scale_005ebc40;
    float ground = SettlePositionToGround00420BD0(source, 0);
    float height = source->y - ground;
    if (static_cast<float>(g_double_005ebc30) <= fabsf(height) &&
        fabsf(height) <= static_cast<float>(g_double_005ec038) &&
        (flag_2 != 0 || fabsf(height) <= static_cast<float>(g_double_005ec030))) {
        source->y = ground;
    }
    unsigned int monsters = 0;
    if (first_only != 0) {
        float expand = radius * g_float_005ec048;
        srVector3T<float> low;
        low.x = source->x - expand;
        low.y = source->y - expand;
        low.z = source->z - expand;
        srVector3T<float> high;
        high.x = expand + source->x;
        high.y = expand + source->y;
        high.z = expand + source->z;
        candidates = static_cast<unsigned long*>(operator new(0x400));
        monsters = static_cast<unsigned int>(QueryObjects(
            &candidates, &low, &high, W8_OCTREE_KIND_LOCATION, -1)); /* c-style-cast-ok:
            the shared query count field is stored unsigned */
    }
    float ring_upper = 0.0f;
    if (-1 < mode) {
        int lower = 0;
        do {
            if (count <= found) {
                break;
            }
            int i = lower;
            float upper = ring_upper;
            if (lower <= static_cast<int>(ring_upper)) {
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
                        if ((i <= lower || static_cast<int>(upper) <= i || j <= lower ||
                             static_cast<int>(upper) <= j) &&
                            (i != found_i || j != found_j)) {
                            float jitter_i;
                            float jitter_j;
                            if (found == 0) {
                                jitter_i = 0.0f;
                                jitter_j = g_float_005ebb34;
                            } else {
                                jitter_i = static_cast<float>(Random(1000)) * g_float_005ec050 -
                                           g_float_005ebc3c;
                                jitter_j = static_cast<float>(Random(1000)) * g_float_005ec050 -
                                           g_float_005ebc3c;
                            }
                            srVector3T<float> candidate;
                            candidate.x = sin_radius * (static_cast<float>(i) + jitter_i) +
                                          source->x +
                                          neg_cos_radius * (jitter_j + static_cast<float>(j));
                            candidate.y = source->y + g_world_scale_005ebc40;
                            candidate.z = (static_cast<float>(i) + jitter_i) * cos_radius +
                                          source->z +
                                          (jitter_j + static_cast<float>(j)) * sin_radius;
                            ground = SettlePositionToGround00420BD0(&candidate, 0);
                            height = candidate.y - ground;
                            if (static_cast<float>(g_double_005ebc30) <= fabsf(height) &&
                                fabsf(height) <= static_cast<float>(g_double_005ec038) &&
                                (flag_2 != 0 ||
                                 fabsf(height) <= static_cast<float>(g_double_005ec030))) {
                                candidate.y = ground;
                                unsigned char clear;
                                if (first_only == 0) {
                                    clear =
                                        pathing_180->SnapWaypointPosition00462E60(&candidate, 0);
                                } else {
                                    clear = pathing_180->TestPathCellClearance00463040(
                                        &candidate, radius * g_float_005ebc7c, 0);
                                }
                                if (clear != 0 && (flag_3 == 0 || InsideDestinationTrigger00445940(
                                                                      candidate.x, candidate.y,
                                                                      candidate.z) == 0)) {
                                    if (first_only != 0) {
                                        srVector3T<float> camera =
                                            g_startup_world_659c0c->GetPosition();
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
                                            float reach = radius * g_float_005ebc7c;
                                            do {
                                                W8Monster* monster =
                                                    GetMonsterByLocationID(candidates[m]);
                                                srVector3T<float> position = monster->GetPosition();
                                                float monster_dx = position.x - candidate.x;
                                                float monster_dy = position.y - candidate.y;
                                                float monster_dz = position.z - candidate.z;
                                                float clearance =
                                                    monster->movement_0c0.alternate_radius_0b4 +
                                                    reach;
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
                                    if (flag_4 == 0 || pathing_180 == 0 ||
                                        pathing_180->TestWaypointSpan0045A1B0(&candidate, source, 0,
                                                                              0) != 0) {
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
                                            if (pathing_180->TestWaypointSpan0045A1B0(
                                                    &candidate, existing, 0, 0) != 0) {
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
                    } while (j <= static_cast<int>(ring_upper));
                    ++i;
                } while (i <= static_cast<int>(upper));
            }
            if (placed) {
            advance_ring:
                upper = static_cast<float>(static_cast<int>(upper) - 1);
                ++lower;
                placed = false;
            }
            ring_upper = static_cast<float>(static_cast<int>(upper) + 1);
            lower = lower - 1;
        } while (static_cast<int>(ring_upper) <= mode);
    }
    if (candidates != 0) {
        delete candidates;
    }
    if (flag_2 != 0 && 0 < static_cast<int>(found)) {
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
