#include <cstdlib>
#include <cstring>
#include <io.h>
#include <sys/stat.h>

#include "surrender/srHeap.h"
#include "surrender/srMath.h"
#include "surrender/srScene.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/sr_api.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Navigator.h"

#include <math.h>

#define OCTREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp"

// FUNCTION: WIZ8 0x00433a70
void W8Octree::GetPathSurfaceNormal00433A70(
    const srVector3T<float>* position, srVector3T<float>* normal)
{
    if (pathing_180 != 0) {
        pathing_180->GetPathSurfaceNormal0045B730(position, normal);
        return;
    }
    normal->x = 0.0f;
    normal->y = 1.0f;
    normal->z = 0.0f;
}

// GLOBAL: WIZ8 0x00659770
unsigned long g_octree_storage_00659770;

// GLOBAL: WIZ8 0x00659890
unsigned long g_octree_state_00659890;
// GLOBAL: WIZ8 0x00659894
srNode* g_octree_trace_node_00659894;
// GLOBAL: WIZ8 0x00659898
unsigned char g_octree_update_suspended_00659898;
// GLOBAL: WIZ8 0x00659899
unsigned char g_octree_trace_enabled_00659899;
extern void Function518510(void* notice);
extern unsigned char g_navigator_link_mode_00659c10;

/* Build a packed four-byte colour from four components and answer its
   address. The receiver is the output slot. */
/* Draw the probe box through the world camera. */

extern const float g_world_scale_005ebc40;

// GLOBAL: WIZ8 0x006598a4
W8Octree* g_octree_6598a4;

// GLOBAL: WIZ8 0x006598a8
unsigned char g_flag_6598a8;

// FUNCTION: WIZ8 0x0042bc00
void NoOct(void)
{
    g_flag_6598a8 = 1;
}

// GLOBAL: WIZ8 0x005ec02c
static const float NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE = 50.0f;

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
    horizontal_fov_cosine_1f8 = (float)cos(horizontal_fov_1f0);
    vertical_fov_cosine_1fc = (float)cos(vertical_fov_1f4);
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
                m_pSubmeshes[static_cast<stModelInstance*>(
                                g_world->psrMeshes[mesh_index])
                                ->state_17c +
                            1]
                    .mesh_04 = mesh_index;
                m_pSubmeshes[mesh_index + 1].flags_00 &= 0xffffffc7;
                stModelInstance* mesh = static_cast<stModelInstance*>(
                    g_world->psrMeshes[mesh_index]);
                if (mesh != 0) {
                    mesh->setFlag(srNode::FLAG_POSITIONAL_0);
                    mesh->setFlag(srNode::FLAG_POSITIONAL_1);
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
    CollectVisibleRegions00430D50(
        &camera_location_1c0, reinterpret_cast<int*>(m_positional_204), 0, 1);
    CollectVisibleCells0042FE90();
    if (pathing_180 != 0) {
        srVector3T<float> dof;
        GetWorldCursorPosition00490BF0(&dof);
        if (dof.x != g_float_005ebb34 || dof.y != g_float_005ebb34 ||
            dof.z != g_float_005ebb34) {
            srVector3T<float> probe = dof;
            pathing_180->UpdatePathVisualization0045BC40(
                &probe, &camera_dof_1cc);
        }
        else {
            pathing_180->UpdatePathVisualization0045BC40(
                &camera_location_1c0, &camera_dof_1cc);
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
            stModelInstance* mesh = static_cast<stModelInstance*>(
                g_world->psrMeshes[submesh->mesh_04]);
            if (mesh != 0) {
                mesh->setFlag(srNode::FLAG_POSITIONAL_0);
                mesh->setFlag(srNode::FLAG_POSITIONAL_1);
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
            stModelInstance* mesh = static_cast<stModelInstance*>(
                g_world->psrMeshes[mesh_index]);
            if (mesh != 0) {
                mesh->clearFlag(srNode::FLAG_POSITIONAL_0);
                mesh->clearFlag(srNode::FLAG_POSITIONAL_1);
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
                    srAssertFail("usProp<=m_usMeshParticlesLen", OCTREE_CPP,
                                 0xa07, "Particle lookup index out of range");
                }
                unsigned short particle = m_pusMeshParticles[link];
                ++link;
                if (particle <= m_usNumParticlesLoaded &&
                    m_papParticles[particle] != 0) {
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

/* The two per-frame visibility helpers behind the cell walk. Both take only
   the receiver in ECX and their bodies are unrecovered, so they keep
   address-qualified names. */
/* Point the visibility filter at the octree so it answers over this frame's
   cell set. */

/* Expand the camera box over the spatial levels and collect the regions the
   camera can occupy.

   The camera cell is quantized twice, once against the node extent and once
   against the leaf cell size. The walk keeps the camera inside an expanding
   box while the node extent shrinks per level; the deepest level looks the
   camera cell up in the region-link table and adds every linked region to the
   projected set. The leaf the walk ends on contributes its region list through
   0x00431050. */
// FUNCTION: WIZ8 0x00430d50
unsigned char W8Octree::CollectVisibleRegions00430D50(
    srVector3T<float>* location, int* cells, float* depth, unsigned char mode)
{
    int link_cell[3];
    float box_min[3];
    float box_max[3];
    unsigned char inside = 1;

    for (int axis = 0; axis < 3; ++axis) {
        box_min[axis] = (&spatial_000.minimum_0c.x)[axis];
        box_max[axis] = (&spatial_000.maximum_18.x)[axis];
        cells[axis] =
            (int)(((&location->x)[axis] - box_min[axis]) /
                  spatial_000.node_extent_70);
        link_cell[axis] =
            (int)(((&location->x)[axis] - box_min[axis]) /
                  spatial_000.positional_54);
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
                 ((float)cells[axis] * spatial_000.node_extent_70 +
                  box_min[axis])) /
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
            }
            else {
                box_min[axis] = edge;
                child_index |= 1 << (2 - axis);
            }
        }
        if (level == spatial_000.positional_52) {
            unsigned int key =
                ((m_positional_140 * 0x100 + link_cell[0]) * 0x100 +
                 link_cell[1]) *
                    0x100 +
                link_cell[2];
            int slot = m_pRegionLinks_150->FindNextEntry(&key, -1);
            while (slot != -1) {
                m_projected_regions_15c->Set(
                    m_pRegionLinks_150->entries[slot].value);
                slot = m_pRegionLinks_150->FindNextEntry(&key, slot);
                m_projected_regions_valid_16a = 1;
            }
        }
        if (node != 0) {
            node = (int)m_owned_09c[node].children_04[child_index];
        }
        span *= g_float_005ebc7c;
    }
    if (node != 0) {
        unsigned long region_offset = m_owned_0a0[node].region_offset_04;
        if (region_offset != 0) {
            Function00431050(location, m_owned_148 + region_offset);
            return 1;
        }
    }
    Function00431050(location, 0);
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

    for (int index = 1; index < spatial_000.positional_46; ++index) {
        W8OctRegionVolume0049E460* volume = &spatial_000.owned_5c[index];

        if (m_projected_regions_valid_16a != 0 &&
            !m_projected_regions_15c->Test(volume->region_bit_0c)) {
            continue;
        }
        float dx = camera_location_1c0.x - volume->points_1c[0].x;
        float dy = camera_location_1c0.y - volume->points_1c[0].y;
        float dz = camera_location_1c0.z - volume->points_1c[0].z;

        if (dx * dx + dy * dy + dz * dz >= radius_squared) {
            continue;
        }
        unsigned char visible = PointInsideFrustum0046D880(
            &volume->points_1c[0], m_frustum_planes_21c);

        for (int point = 1; !visible && point < 9; ++point) {
            visible = PointInsideFrustum0046D880(
                &volume->points_1c[point], m_frustum_planes_21c);
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
    float extent = spatial_000.positional_60;
    float far_clip = far_clip_200;
    float sine = (float)sin(fov);
    float cosine = (float)cos(fov);
    float ratio = extent / cosine;
    float tangent = (float)tan(fov);
    float tangent_vertical = (float)tan(vertical_fov_1f4 * g_float_005ebc7c);
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

        (&corners[0].x)[axis] =
            (&camera_location_1c0.x)[axis] - (extent / sine) * dof;
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
    BuildPlaneFromPoints0046D660(
        &m_frustum_planes_21c[0], &corners[0], &corners[5], &corners[4]);
    BuildPlaneFromPoints0046D660(
        &m_frustum_planes_21c[1], &corners[0], &corners[4], &corners[6]);
    BuildPlaneFromPoints0046D660(
        &m_frustum_planes_21c[2], &corners[0], &corners[7], &corners[5]);
    BuildPlaneFromPoints0046D660(
        &m_frustum_planes_21c[3], &corners[0], &corners[6], &corners[7]);
    for (int index = 0; index < 6; ++index) {
        m_frustum_planes_21c[index].w += spatial_000.positional_54;
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
    short radius =
        (short)((int)(far_clip_200 / spatial_000.positional_54) + 1);
    short center[3];

    for (int axis = 0; axis < 3; ++axis) {
        center[axis] =
            (short)(int)(((&camera_location_1c0.x)[axis] -
                          (&spatial_000.minimum_0c.x)[axis]) /
                         spatial_000.positional_54);
    }
    unsigned int region_base = m_positional_140;
    for (short x = -radius; x <= radius; ++x) {
        short cell_x = center[0] + x;
        if (cell_x < 0 || cell_x >= spatial_000.positional_50) {
            continue;
        }
        for (short y = -radius; y <= radius; ++y) {
            short cell_y = center[1] + y;
            if (cell_y < 0 || cell_y >= spatial_000.positional_50) {
                continue;
            }
            for (short z = -radius; z <= radius; ++z) {
                short cell_z = center[2] + z;
                if (cell_z < 0 || cell_z >= spatial_000.positional_50) {
                    continue;
                }
                if (abs(x) < 2 && abs(y) < 2 && abs(z) < 2) {
                    int node = 1;
                    for (unsigned int mask = 1 << spatial_000.depth_44;
                         mask != 0;
                         mask >>= 1) {
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
                            node = (int)m_owned_09c[node]
                                       .children_04[child];
                        }
                    }
                    if (node != 0) {
                        unsigned short region =
                            m_owned_09c[node].positional_02;
                        if (region != 0) {
                            m_current_regions_160->Set(region);
                        }
                    }
                }
                else {
                    float offset =
                        spatial_000.positional_54 * g_float_005ebc7c;
                    srVector3T<float> point;
                    point.x = (float)cell_x * spatial_000.positional_54 +
                              offset + spatial_000.minimum_0c.x;
                    point.y = (float)cell_y * spatial_000.positional_54 +
                              spatial_000.minimum_0c.y + offset;
                    point.z = (float)cell_z * spatial_000.positional_54 +
                              spatial_000.minimum_0c.z + offset;
                    if (PointInsideFrustum0046D880(&point, m_frustum_planes_21c) ==
                        0) {
                        continue;
                    }
                    int node = 1;
                    for (unsigned int mask = 1 << spatial_000.depth_44;
                         mask != 0;
                         mask >>= 1) {
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
                            node = (int)m_owned_09c[node]
                                       .children_04[child];
                        }
                    }
                    if (node != 0) {
                        unsigned short region =
                            m_owned_09c[node].positional_02;
                        if (region != 0) {
                            m_current_regions_160->Set(region);
                        }
                    }
                }
            }
        }
    }
}

/* Write the octree's point array to a companion file.

   The level path supplies the base name and its existing extension is
   replaced with the point-file extension. A read-only file is made writable
   first. The count precedes the records, and the result reports either
   write. */
// FUNCTION: WIZ8 0x00432d60
unsigned char W8Octree::SavePoints00432D60(char* path)
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
            unsigned char wrote_points =
                FileWrite(file, m_sr_owned_174, m_positional_170 * 0xc, 0);
            result = wrote_count | wrote_points;
            FileClose(file);
        }
    }
    return result;
}

/* Write the octree's region-link table to the .rlk companion file.

   The collected keys are the region ids from one up to the spatial region
   count, followed by the cell keys of the region grid offset by the link id.
   Each key's table values are appended in chain order; the count is written
   first, then the keys and their short values. */
// FUNCTION: WIZ8 0x004331f0
unsigned char W8Octree::SaveRegionLinks004331F0(char* path)
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
        for (unsigned int key = 1; key < spatial_000.positional_46; ++key) {
            for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1);
                 slot != -1;
                 slot = m_pRegionLinks_150->FindNextEntry(&key, slot)) {
                keys[count] = key;
                values[count] = m_pRegionLinks_150->entries[slot].value;
                ++count;
            }
        }
        unsigned int extent = 1 << spatial_000.positional_52;
        unsigned int base = m_positional_140 * 0x1000000;
        for (unsigned int x = 0; x < extent; ++x) {
            for (unsigned int y = 0; y < extent; ++y) {
                for (unsigned int z = 0; z < extent; ++z) {
                    unsigned int key = (x << 16) + (y << 8) + z + base;
                    for (int slot = m_pRegionLinks_150->FindNextEntry(&key, -1);
                         slot != -1;
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
        cell[axis] = (int)(((&camera.x)[axis] -
                            (&spatial_000.minimum_0c.x)[axis]) /
                           spatial_000.node_extent_70);
    }
    minimum.x =
        (float)cell[0] * spatial_000.node_extent_70 + spatial_000.minimum_0c.x;
    minimum.y =
        (float)cell[1] * spatial_000.node_extent_70 + spatial_000.minimum_0c.y;
    minimum.z =
        (float)cell[2] * spatial_000.node_extent_70 + spatial_000.minimum_0c.z;
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
unsigned long* __fastcall PackColour00433FB0(
    unsigned long* color, double red, double green, double blue, double alpha)
{
    unsigned char* bytes = reinterpret_cast<unsigned char*>(color); // reinterpret-ok: packed colour storage
    bytes[3] = (unsigned char)(red * g_double_005ebf60);
    bytes[2] = (unsigned char)(green * g_double_005ebf60);
    bytes[1] = (unsigned char)(blue * g_double_005ebf60);
    bytes[0] = (unsigned char)(alpha * g_double_005ebf60);
    return color;
}

/* Validate the current octree's region-to-mesh links against a scratch copy
   of the spatial state. The scratch copy is flattened to the leaf level with
   all region bounds enabled, and every reported bad link is posted as one
   notice. */
// FUNCTION: WIZ8 0x00433ab0
unsigned char W8Octree::ValidateRegionMeshLinks00433AB0()
{
    W8OctSpatialState0046CCC0 spatial(&spatial_000);
    spatial.depth_44 = 0;
    spatial.level_kind_6c = 1;
    spatial.positional_94 = 1;
    int bad_links = CountBadRegionMeshLinks00433B90(&spatial);
    if (bad_links != 0) {
        Function518510(FormatWideString(
            L" %d Bad Region-Mesh Links!", bad_links, g_small_font_683678, 1,
            1, 0, 0));
        return 0;
    }
    return 1;
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
    g_octree_update_suspended_00659898 =
        (g_octree_update_suspended_00659898 == 0);
    if (g_octree_update_suspended_00659898 == 0) {
        g_octree_trace_node_00659894->setFlag(srNode::FLAG_POSITIONAL_0);
        g_octree_trace_node_00659894->setFlag(srNode::FLAG_POSITIONAL_1);
        m_reset_visibility_168 = 1;
        MarkRendererReady();
        return;
    }
    for (unsigned short mesh_index = 0; mesh_index < m_meshCount_1b4;
         ++mesh_index) {
        m_pSubmeshes[static_cast<stModelInstance*>(g_world->psrMeshes[mesh_index])
                        ->state_17c +
                    1]
            .mesh_04 = mesh_index;
        m_pSubmeshes[mesh_index + 1].flags_00 &= 0xffffffc7;
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])
            ->setFlag(srNode::FLAG_POSITIONAL_0);
        static_cast<stModelInstance*>(world->psrMeshes[mesh_index])
            ->setFlag(srNode::FLAG_POSITIONAL_1);
    }
    memset(m_pfRegsVisited, 0, spatial_000.positional_58 + 1);
    if (g_octree_trace_node_00659894 == 0) {
        g_octree_trace_node_00659894 =
            g_octree_game_data_00652db0->CreateTraceModel0041C930();
        g_octree_trace_node_00659894->setParent(world->static_scene, 1);
        SetChainValue15C(
            reinterpret_cast<char*>(g_octree_trace_node_00659894), 2);
    }
    g_octree_trace_node_00659894->clearFlag(srNode::FLAG_POSITIONAL_0);
    g_octree_trace_node_00659894->clearFlag(srNode::FLAG_POSITIONAL_1);
}

// FUNCTION: WIZ8 0x00434250
unsigned char W8Octree::PrepareNavigatorTarget00434250(
    W8NavigatorMovementState* movement, float radius, float separation)
{
    unsigned char result = 0;
    unsigned char hit = 0;
    if (movement->target_position_04c.y > spatial_000.clipped_maximum_30.y) {
        movement->target_position_04c.y = spatial_000.clipped_maximum_30.y;
    }
    srVector3T<float> probe = movement->target_position_04c;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        movement->target_position_04c.y = probe.y;
    }
    if (pathing_180 == 0) {
        return 1;
    }
    srVector3T<float> delta;
    delta.x = movement->target_position_04c.x - movement->position_040.x;
    delta.y = 0.0f;
    delta.z = movement->target_position_04c.z - movement->position_040.z;
    if (sqrt(delta.x * delta.x + delta.z * delta.z) < NAVIGATOR_MINIMUM_HORIZONTAL_DISTANCE) {
        return 0;
    }
    if ((movement->attachment_0ac->flags_00 & 0x10000) == 0) {
        srVector3T<float> target = movement->target_position_04c;
        if (pathing_180->FindPathCell00459D60(&target, 0, 1) != 0) {
            if (pathing_180->TestWaypointSpan0045A1B0(&movement->position_040, &target, 0, 0) == 0) {
                movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040, &target);
                movement->attachment_0ac->separation_54 = separation;
                result = pathing_180->BuildAttachmentPath00460950(
                    movement->attachment_0ac, movement->unknown_000);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
                    pathing_180->AdvanceAttachmentWaypoint00462DE0(&movement->position_040, attachment);
                    movement->attachment_0ac->GetNextPosition00456660(&movement->target_position_04c);
                    return result;
                }
                result = pathing_180->ProbeAttachmentPath00462360(movement->attachment_0ac);
                if (result != 0) {
                    W8NavigatorAttachment* attachment = movement->attachment_0ac;
                    attachment->position_4c[attachment->path_position_index_08] =
                        movement->target_position_04c;
                    attachment->position_1c = attachment->position_4c[attachment->path_position_index_08];
                    return result;
                }
            } else {
                movement->attachment_0ac->InitializeSegment004563E0(
                    &movement->position_040, &movement->target_position_04c);
                result = 1;
            }
        }
        return result;
    }
    delta = movement->target_position_04c - movement->position_040;
    float squared_length = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    float gap = (float)sqrt(squared_length) - separation;
    if (gap < g_float_005ebb34) {
        movement->attachment_0ac->InitializeSegment004563E0(
            &movement->position_040, &movement->position_040);
        return 1;
    }
    if (gap < g_world_scale_005ebc40) {
        if (squared_length != g_zero_005ebb40) {
            float scale = (float)(gap * g_float_005ec028 / sqrt(squared_length));
            delta *= scale;
        }
        delta += movement->position_040;
        movement->attachment_0ac->InitializeSegment004563E0(&movement->position_040, &delta);
        return 1;
    }
    movement->attachment_0ac->InitializeSegment004563E0(
        &movement->position_040, &movement->target_position_04c);
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
    float dx = target.x - movement->position_040.x;
    float dy = target.y - movement->position_040.y;
    float dz = target.z - movement->position_040.z;
    if (movement->movement_scale_060 * g_world_scale_005ebc40 < sqrt(dx * dx + dy * dy + dz * dz)) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00434880
unsigned char W8Octree::PrepareNavigatorPatrol00434880(
    W8NavigatorMovementState* movement, float minimum, float maximum)
{
    unsigned char result = 0;
    if (pathing_180 != 0) {
        srVector3T<float> velocity = movement->velocity_034;
        movement->attachment_0ac->InitializeSegment004563E0(
            &movement->position_040, &movement->target_position_04c);
        result = pathing_180->BuildPatrolPath00461960(
            movement->attachment_0ac, movement->unknown_000,
            &movement->target_position_04c, minimum, &velocity, maximum);
        if (result == 0) {
            return 0;
        }
        double step = movement->movement_scale_060 * g_world_scale_005ebc40;
        movement->attachment_0ac->GetNextPosition00456660(&movement->target_position_04c);
        srVector3T<float> delta;
        delta = movement->target_position_04c - movement->position_040;
        float squared_length = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (step < sqrt(squared_length) && squared_length != g_zero_005ebb40) {
            float scale = (float)(step / sqrt(squared_length));
            delta *= scale;
        }
        movement->target_position_04c = delta + movement->position_040;
    }
    return result;
}

// FUNCTION: WIZ8 0x00434a00
unsigned char W8Octree::LinkNavigatorTarget00434A00(
    W8NavigatorMovementState* movement, const srVector3T<float>* target, float separation)
{
    if (pathing_180 != 0) {
        return pathing_180->LinkAttachmentTarget004612A0(
            movement->attachment_0ac, movement->unknown_000, target, separation);
    }
    return 0;
}

/* ReadOctFile's own direct callees. Their bodies are not recovered, so they
   keep address-qualified names. */
extern void ReadWaypointFile0043A0F0(void);
/* The cell-walk probes and the trace helpers the two line-of-sight bodies use.
   None of their bodies are recovered, so they keep address-qualified names. */

// GLOBAL: WIZ8 0x005ebcd0
float g_octree_cell_scale_005ebcd0 = 100.0f;
extern unsigned short g_path_reserve_0060827a;
extern float g_path_span_scale_005ec344;
extern float g_path_limit_006081e8;
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

template <class T>
T ReadHeader(const unsigned char* header, unsigned int offset)
{
    T result;
    memcpy(&result, header + offset, sizeof(result));
    return result;
}

template <class T>
void WriteMember(W8Octree* octree, unsigned int offset, T value)
{
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
        if ((long)level < 1) {
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
    if (m_positional_0b8 < node) {
        return 0;
    }
    return node;
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
bool W8Octree::HasLineOfSight(
    const srVector3T<float>* from, srVector3T<float>* to, char allow_fallback)
{
    W8OctreeWalk walk;
    int cell[5];
    int step[4];
    unsigned char result[12];
    srVector3T<float> hit;
    unsigned char blocked = 0;
    int span;
    int error_0;
    int error_1;
    int index;

    SeedCellProbe00457640(from, to);
    m_positional_1b8 = 0;
    m_owned_190->ClearAll();
    m_current_regions_160->ClearAll();
    cell[0] = (int)((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    step[3] = (int)((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    cell[1] = (int)((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    step[2] = (int)((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    cell[2] = (int)((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    step[1] = (int)((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    span = abs(cell[2] - step[1]) + abs(cell[1] - step[2]) + abs(cell[0] - step[3]);
    if (span < 2) {
        ProbeCellForBlockers00435C40(cell);
        blocked = TestProbeResult00435F00(result);
        if (blocked == 0 && span != 0) {
            ProbeCellForBlockers00435C40(&step[1]);
            blocked = TestProbeResult00435F00(result);
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
                if (ProbeCellForBlockers00435C40(cell) != 0) {
                    blocked = TestProbeResult00435F00(result);
                }
                if (error_0 < error_1) {
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers00435C40(cell) != 0) {
                            blocked = TestProbeResult00435F00(result);
                        }
                        if (error_1 < 0 && blocked == 0) {
                            cell[cell[4]] += step[cell[4]];
                            error_1 += walk.error_reset_3c;
                            if (ProbeCellForBlockers00435C40(cell) != 0) {
                                blocked = TestProbeResult00435F00(result);
                            }
                        }
                    }
                } else if (error_1 < 0 && blocked == 0) {
                    cell[cell[4]] += step[cell[4]];
                    error_1 += walk.error_reset_3c;
                    if (ProbeCellForBlockers00435C40(cell) != 0) {
                        blocked = TestProbeResult00435F00(result);
                    }
                    if (error_0 < 0 && blocked == 0) {
                        error_0 += walk.error_reset_30;
                        cell[walk.minor_axis_1c] += step[walk.minor_axis_1c];
                        if (ProbeCellForBlockers00435C40(cell) != 0) {
                            blocked = TestProbeResult00435F00(result);
                        }
                    }
                }
                cell[walk.major_axis_18] += step[walk.major_axis_18];
                error_1 -= walk.error_delta_34;
                error_0 -= walk.error_delta_28;
                ++cell[3];
            } while (cell[3] < step[3]);
        }
        if (blocked == 0) {
            if (allow_fallback != 0 &&
                TraceAgainstProps00436510(from, to, 1, 1) != 0) {
                blocked = 1;
            }
            return blocked == 0;
        }
    }
    if (blocked != 0) {
        to->x = hit.x;
        to->y = hit.y;
        to->z = hit.z;
    }
    return blocked == 0;
}

// FUNCTION: WIZ8 0x00434f20
short W8Octree::TraceLineOfSight(
    const srVector3T<float>* from, const srVector3T<float>* to, char trace_world,
    int from_location_id, int to_location_id, char visit_octree, int trace_mode)
{
    W8OctreeWalk walk;
    int cell[5];
    int step[4];
    unsigned char result[12];
    srVector3T<float> hit;
    char blocked = 0;
    char previous = 0;
    int span;
    int error_0;
    int error_1;
    int minor_0;
    int minor_1;
    int index;
    int scratch;

    SeedCellProbe00457640(from, to);
    cell[3] = 0;
    if (visit_octree != 0) {
        m_positional_1b8 = 0;
        m_owned_194->ClearAll();
        cell[0] = (int)((from->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
        step[3] = (int)((to->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
        cell[1] = (int)((from->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
        step[1] = (int)((to->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
        cell[2] = (int)((from->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
        step[0] = (int)((to->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
        span = abs(cell[2] - step[0]) + abs(cell[1] - step[1]) + abs(cell[0] - step[3]);
        if (span < 2) {
            ProbeCellForTrace00435B00(cell);
            blocked = TestTraceResult0041C330(
                m_positional_1b8, m_aulGDObjs, result, m_positional_134, 0);
            if (blocked == 0 && span != 0) {
                ProbeCellForTrace00435B00(&step[3]);
                blocked = TestTraceResult0041C330(
                    m_positional_1b8, m_aulGDObjs, result, m_positional_134, 0);
            }
        } else {
            BuildCellWalk(from, to, &walk);
            cell[1] = walk.cell_00[1];
            cell[0] = walk.cell_00[0];
            cell[2] = walk.cell_00[2];
            minor_0 = walk.minor_axis_1c;
            scratch = walk.major_axis_18;
            minor_1 = walk.minor_axis_20;
            step[0] = walk.step_0c[0];
            cell[4] = walk.count_24;
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
                    if (ProbeCellForTrace00435B00(cell) != 0) {
                        blocked = TestTraceResult0041C330(
                            m_positional_1b8, m_aulGDObjs, result, m_positional_134, 0);
                    }
                    if (error_0 < error_1) {
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace00435B00(cell) != 0) {
                                blocked = TestTraceResult0041C330(
                                    m_positional_1b8, m_aulGDObjs, result,
                                    m_positional_134, 0);
                            }
                            if (error_1 < 0 && blocked == 0) {
                                cell[minor_1] += step[minor_1];
                                error_1 += walk.error_reset_3c;
                                if (ProbeCellForTrace00435B00(cell) != 0) {
                                    blocked = TestTraceResult0041C330(
                                        m_positional_1b8, m_aulGDObjs, result,
                                        m_positional_134, 0);
                                }
                            }
                        }
                    } else if (error_1 < 0 && blocked == 0) {
                        cell[minor_1] += step[minor_1];
                        error_1 += walk.error_reset_3c;
                        if (ProbeCellForTrace00435B00(cell) != 0) {
                            blocked = TestTraceResult0041C330(
                                m_positional_1b8, m_aulGDObjs, result,
                                m_positional_134, 0);
                        }
                        if (error_0 < 0 && blocked == 0) {
                            cell[minor_0] += step[minor_0];
                            error_0 += walk.error_reset_30;
                            if (ProbeCellForTrace00435B00(cell) != 0) {
                                blocked = TestTraceResult0041C330(
                                    m_positional_1b8, m_aulGDObjs, result,
                                    m_positional_134, 0);
                            }
                        }
                    }
                    cell[scratch] += step[scratch];
                    error_1 -= walk.error_delta_34;
                    error_0 -= walk.error_delta_28;
                    ++index;
                    previous = blocked;
                } while (index < cell[4]);
            }
        }
        if (trace_world != 0 &&
            TraceAgainstProps00436510(from, &hit, 0, 0) != 0) {
            blocked = 1;
        } else if (blocked == 0) {
            goto resolve;
        }
        cell[3] = 1;
        if (blocked != 0) {
            to = &hit;
            return 1;
        }
    }
resolve:
    if (from_location_id >= -2) {
        cell[4] = to_location_id;
        if (ResolveTraceHit004353F0(
                result, &hit, from_location_id, &cell[4], to_location_id, 0,
                trace_mode) != 0) {
            return -1;
        }
    }
    return (short)cell[3];
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

/* Record that one object now occupies one cell.

   The object's key is its kind in the high half and its id in the low half.
   When it is already registered somewhere, the cell it was in is compared
   against the one being queued and an unchanged pairing is left completely
   alone; otherwise the old pairing comes out of both indexes first. A kind of
   twelve additionally maintains a second pairing under its own tag.

   The two indexes are the same pair AddCollidablePropBounds keeps: one keyed by
   cell, one keyed by object. */
// FUNCTION: WIZ8 0x00437000
unsigned char W8OctreeObjectRegistry::RegisterObjectCell(
    int kind, int id, const int* point)
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

    object_key = (kind & 0xffff) * 0x10000 + (id & 0xffff);
    hash = (object_key >> 10 ^ object_key) >> 10 ^ object_key;
    slot = static_cast<int*>(by_object->bucket_heads)[hash & (by_object->bucket_count - 1)];
    while (slot != -1) {
        entries = static_cast<W8OctreeEntry*>(by_object->entries);
        if (entries[slot].key == object_key) {
            occupied = entries[slot].value;
            if (occupied == 0) {
                break;
            }
            if (point[0] == 0 &&
                (int)(((occupied - 1) & 0xff00) << 8) == point[1] &&
                ((occupied - 1) & 0xff) == point[2]) {
                return 1;
            }
            bucket = static_cast<int*>(by_object->bucket_heads) +
                (hash & (by_object->bucket_count - 1));
            slot = *bucket;
            if (slot != -1) {
                entries = static_cast<W8OctreeEntry*>(by_object->entries);
                previous = -1;
                for (;;) {
                    if (entries[slot].key == object_key) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        static_cast<W8OctreeEntry*>(by_object->entries)[slot].next_index =
                            by_object->free_head;
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
            bucket = static_cast<int*>(by_cell->bucket_heads) +
                ((((unsigned int)occupied >> 10 ^ occupied) >> 10 ^ occupied) &
                 (by_cell->bucket_count - 1));
            if (*bucket != -1) {
                entries = static_cast<W8OctreeEntry*>(by_cell->entries);
                slot = *bucket;
                previous = -1;
                do {
                    if (entries[slot].key == (unsigned int)occupied &&
                        entries[slot].value == (int)object_key) {
                        if (previous == -1) {
                            *bucket = entries[slot].next_index;
                        } else {
                            entries[previous].next_index = entries[slot].next_index;
                        }
                        static_cast<W8OctreeEntry*>(by_cell->entries)[slot].next_index =
                            by_cell->free_head;
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
    if ((short)kind == 0xc) {
        cell_key = ((point[0] << 8) + point[1]) * 0x100 + 1 + point[2];
        tagged_key = (id & 0xffff) + 0xd0000;
        by_object->Remove(&tagged_key, &cell_key);
        by_object = this->by_object;
        slot = by_object->AllocateEntry();
        entries = static_cast<W8OctreeEntry*>(by_object->entries);
        hash = ((tagged_key >> 10 ^ tagged_key) >> 10 ^ tagged_key) &
            (by_object->bucket_count - 1);
        entries[slot].key = tagged_key;
        entries[slot].value = cell_key;
        entries[slot].next_index = static_cast<int*>(by_object->bucket_heads)[hash];
        static_cast<int*>(by_object->bucket_heads)[hash] = slot;
        this->by_cell->Remove((const unsigned int*)&cell_key, (const int*)&tagged_key);
        this->by_cell->Insert((const unsigned int*)&cell_key, (const int*)&tagged_key);
    }
record:
    cell_key = ((point[0] << 8) + point[1]) * 0x100 + 1 + point[2];
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
void W8Octree::AddCollidablePropBounds(
    int index, const srVector3T<float>* bounds)
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
        minimum[axis] = (int)((bounds[0].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    }
    minimum[0] = (int)((bounds[0].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    minimum[1] = (int)((bounds[0].y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    minimum[2] = (int)((bounds[0].z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    maximum[0] = (int)((bounds[1].x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    maximum[1] = (int)((bounds[1].y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    maximum[2] = (int)((bounds[1].z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);

    prop_key = ((index + 1) & 0xffff) + 0x80000;
    for (x = minimum[0]; x <= maximum[0]; ++x) {
        for (y = minimum[1]; y <= maximum[1]; ++y) {
            for (z = minimum[2]; z <= maximum[2]; ++z) {
                cell_key = ((x << 8) + y) * 0x100 + 1 + z;
                by_prop = object_registry->by_object;
                by_prop->Remove(&prop_key, (const int*)&cell_key);
                slot = by_prop->AllocateEntry();
                entries = static_cast<W8OctreeEntry*>(by_prop->entries);
                hash = ((prop_key >> 10 ^ prop_key) >> 10 ^ prop_key) &
                    (by_prop->bucket_count - 1);
                entries[slot].key = prop_key;
                entries[slot].value = cell_key;
                entries[slot].next_index = static_cast<int*>(by_prop->bucket_heads)[hash];
                static_cast<int*>(by_prop->bucket_heads)[hash] = slot;

                by_cell = object_registry->by_cell;
                by_cell->Remove(&cell_key, (const int*)&prop_key);
                if (by_cell->free_head == -1) {
                    by_cell->Grow();
                }
                slot = by_cell->free_head;
                entries = static_cast<W8OctreeEntry*>(by_cell->entries);
                by_cell->free_head = entries[slot].next_index;
                entries[slot].key = cell_key;
                entries[slot].value = prop_key;
                hash = ((cell_key >> 10 ^ cell_key) >> 10 ^ cell_key) &
                    (by_cell->bucket_count - 1);
                entries[slot].next_index = static_cast<int*>(by_cell->bucket_heads)[hash];
                static_cast<int*>(by_cell->bucket_heads)[hash] = slot;
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
void W8Octree::BuildCellWalk(
    const srVector3T<float>* from, const srVector3T<float>* to, W8OctreeWalk* walk)
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
    cell = (int)cell_size;
    from_cell[0] = (int)((from->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0);
    to_cell[0] = (int)((to->x - spatial_000.minimum_0c.x) * g_octree_cell_scale_005ebcd0);
    from_cell[1] = (int)((from->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0);
    to_cell[1] = (int)((to->y - spatial_000.minimum_0c.y) * g_octree_cell_scale_005ebcd0);
    from_cell[2] = (int)((from->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0);
    to_cell[2] = (int)((to->z - spatial_000.minimum_0c.z) * g_octree_cell_scale_005ebcd0);

    for (axis = 0; axis < 3; ++axis) {
        span = to_cell[axis] - from_cell[axis];
        fraction[axis] = (float)(from_cell[axis] % cell) / cell_size;
        delta[axis] = (float)span;
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
    walk->error_delta_28 = (int)((float)fabs(delta[minor_0] / delta[major]) * cell_size);
    walk->error_2c = (int)(cell_size * fraction[minor_0] -
                           (float)walk->error_delta_28 * fraction[major]);
    walk->error_delta_34 = (int)((float)fabs(delta[minor_1] / delta[major]) * cell_size);
    walk->error_38 = (int)(cell_size * fraction[minor_1] -
                           (float)walk->error_delta_34 * fraction[major]);
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

/* Tell a monster which mesh it now stands on, then queue its move.

   A location the octree cannot resolve, or one whose submesh has no live model
   instance, clears the monster's cached mesh rather than leaving a stale one. */
// FUNCTION: WIZ8 0x0042e540
void W8Octree::UpdateMonsterLocation(
    unsigned short location_id, const srVector3T<float>* position)
{
    int queue_id = location_id + 1;
    unsigned int monster_list_index;
    W8MonsterInfo* info;
    W8Monster* monster;
    int sector;
    int mesh;
    int point[3];

    if (location_id == 0) {
        return;
    }
    monster_list_index =
        MonsterGetIndexByLocationID(0x4c4, OCTREE_CPP, location_id, 1);
    info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    if (info != 0 && info->monster != 0) {
        monster = info->monster;
        sector = GetSectorForPosition00430BF0(position);
        if (sector == 0 ||
            (mesh = reinterpret_cast<int*>(g_world->psrMeshes)
                 [m_pSubmeshes[sector].mesh_04]) == 0) {
            monster->node_308 = 0;
        } else {
            monster->node_308 = reinterpret_cast<srNode*>(mesh);
        }
    }
    point[0] = (int)((position->x - spatial_000.minimum_0c.x) / spatial_000.node_extent_70);
    point[1] = (int)((position->y - spatial_000.minimum_0c.y) / spatial_000.node_extent_70);
    point[2] = (int)((position->z - spatial_000.minimum_0c.z) / spatial_000.node_extent_70);
    object_registry->RegisterObjectCell(0xc, queue_id, point);
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
W8Octree::W8Octree(const char* path, void** game_data)
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
    void* pGameData = 0;

    Reset();
    if (path == 0) {
        Initialize(0);
        return;
    }
    if (CheckLevelAssetSet0042CCC0(path) < 0) {
        goto failed;
    }
    if (CheckLevelAssetSet0042CCC0(path) > 0 &&
        BuildPreprocessedFiles00492E60(path) == 0) {
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
        srAssertFail("hOctFile", OCTREE_CPP, 0xa3,
                     "ReadOctFile: Couldn't open octree file.");
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
                m_owned_0c0 = malloc(name_length);
                if (m_owned_0c0 != 0) {
                    strcpy(static_cast<char*>(m_owned_0c0), path);
                    extension = strrchr(static_cast<char*>(m_owned_0c0), '.');
                    if (extension != 0 &&
                        extension - static_cast<char*>(m_owned_0c0) >
                            (int)(name_length - 7)) {
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
                fSuccess = FileRead(
                    hOctFile, block, ReadHeader<unsigned long>(header, 0x6a) * 0x24, &uiRead);
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
                    fSuccess = FileRead(
                        hOctFile, block,
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
                        fLoaded = FileRead(
                            hOctFile, block,
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

    limit = m_positional_0a4 * m_positional_0a8 * m_positional_0ac;
    if (fLoaded != 0 && limit < 250000) {
        block = malloc(limit * 4);
        m_owned_0b0 = block;
        if (block == 0) {
            strcpy(acMessage, "ReadOctFile: Couldn't allocate polygon index list for regions.");
            goto finish;
        }
        fLoaded = FileRead(
            hOctFile, block,
            m_positional_0a4 * m_positional_0a8 * m_positional_0ac * 4, &uiRead);
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
        m_owned_0d4 = block;
        if (block == 0) {
            fSuccess = 0;
            strcpy(acMessage, "ReadOctFile: Couldn't allocate Poly Lookup table.");
        } else {
            fLoaded = FileRead(
                hOctFile, block, ReadHeader<unsigned long>(header, 0x72) * 4, &uiRead);
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
                    fLoaded = FileRead(
                        hOctFile, block,
                        ReadHeader<unsigned long>(header, 0x92) * 2, &uiRead);
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
                        fLoaded = FileRead(
                            hOctFile, block,
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
                                strcpy(acMessage,
                                       "ReadOctFile: Couldn't allocate Trigger list.");
                                goto finish;
                            }
                            fLoaded = FileRead(
                                hOctFile, block,
                                ReadHeader<unsigned long>(header, 0x8a) * 2, &uiRead);
                            if (fLoaded == 0) {
                                strcpy(acMessage, "ReadOctFile: Couldn't read Trigger list.");
                            }
                            g_octree_bytes_read_00659888 += uiRead;
                        }
                        fSuccess = 0;
                        if (fLoaded != 0) {
                            if (ReadHeader<unsigned short>(header, 0x96) > 1) {
                                block = malloc(
                                    (ReadHeader<unsigned short>(header, 0x96) + 2) * 0xe8);
                                spatial_000.owned_5c =
                                    static_cast<W8OctRegionVolume0049E460*>(
                                        block);
                                if (block == 0) {
                                    fSuccess = 0;
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't allocate region array.");
                                    goto finish;
                                }
                                fLoaded = FileRead(
                                    hOctFile, block,
                                    ReadHeader<unsigned short>(header, 0x96) * 0xe8, &uiRead);
                                if (fLoaded == 0) {
                                    strcpy(acMessage,
                                           "ReadOctFile: Couldn't read region array.");
                                }
                                g_octree_bytes_read_00659888 += uiRead;
                            }
                            fSuccess = 0;
                            if (fLoaded != 0) {
                                fLoaded = FileRead(
                                    hOctFile, &uiTerminator, 4, &uiRead);
                                if (fLoaded == 0 || uiTerminator != -1) {
                                    srAssertFail(
                                        "fSuccess && (uiTerminator==0xffffffff)",
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
                                                (ReadHeader<unsigned long>(header, 0x66) + 1) * 0x10,
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
                                                scan = &m_pSubmeshes[0].positional_0c;
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
                                                    srAssertFail("(i2 < 10000)", OCTREE_CPP,
                                                                 0x179, 0);
                                                }
                                                g_octree_storage_00659770 =
                                                    reinterpret_cast<unsigned long>(
                                                        malloc(limit * 4));
                                                if (g_octree_storage_00659770 == 0) {
                                                    fLoaded = 0;
                                                    strcpy(acMessage,
                                                           "ReadOctFile: Couldn't allocate polygon "
                                                           "index list for regions.");
                                                } else {
                                                    for (index = 0; index < limit; ++index) {
                                                        reinterpret_cast<unsigned int*>(
                                                            g_octree_storage_00659770)[index] =
                                                            index;
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
                                            if (BitArrayLoad0043AEC0(m_pAlphaBits, hOctFile) == 0) {
                                                srAssertFail(
                                                    "m_pAlphaBits->Load(hOctFile)", OCTREE_CPP,
                                                    0x18b,
                                                    "ReadOctFile: Failure reading Alpha Bits.");
                                            }
                                        }
                                        if (fLoaded != 0 && m_ulNumParticles != 0) {
                                            m_pusMeshParticleLookup =
                                                static_cast<unsigned short*>(
                                                    malloc(m_meshCount_1b4 * 2 + 2));
                                            if (m_pusMeshParticleLookup == 0) {
                                                srAssertFail(
                                                    "m_pusMeshParticleLookup", OCTREE_CPP, 0x191,
                                                    "ReadOctFile: Couldn't allocate Mesh Particle "
                                                    "Lookup Table.");
                                            }
                                            fLoaded = FileRead(
                                                hOctFile, m_pusMeshParticleLookup,
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
                                            fLoaded = FileRead(
                                                hOctFile, m_pusMeshParticles,
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
                                            fLoaded = FileRead(
                                                hOctFile, m_pusMeshPropLookup,
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
                                            fLoaded = FileRead(
                                                hOctFile, m_pusMeshProps,
                                                m_usMeshPropsLen_0f4 * 2, &uiRead);
                                            if (fLoaded == 0) {
                                                strcpy(acMessage,
                                                       "ReadOctFile: Couldn't read octree nodes.");
                                            }
                                        }
                                    }
                                    fSuccess = 0;
                                    if (fLoaded != 0) {
                                        fLoaded = FileRead(
                                            hOctFile, &uiTerminator, 4, &uiRead);
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
                                                    (float)ReadHeader<unsigned long>(header, 0xac),
                                                    ReadHeader<unsigned long>(header, 0xb4),
                                                    reinterpret_cast<const float*>(header + 0x0e),
                                                    static_cast<char*>(m_owned_0c0));
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
                                                    if (BitArrayLoad0043AEC0(
                                                            m_pPropSunBits, hOctFile) == 0) {
                                                        srAssertFail(
                                                            "m_pPropSunBits->Load(hOctFile)",
                                                            OCTREE_CPP, 0x1c6,
                                                            "ReadOctFile: Failure reading Prop Sun "
                                                            "Bits.");
                                                    }
                                                }
                                                fSuccess = FileRead(
                                                    hOctFile, &uiTerminator, 4, &uiRead);
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
        pGameData = new W8GameData(hOctFile, 0);
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
        *static_cast<W8Octree**>(pGameData) = this;
        *game_data = pGameData;
        g_octree_game_data_00652db0 = static_cast<W8GameData*>(pGameData);
        m_positional_169 = ReadLevelName00432E90(static_cast<char*>(m_owned_0c0));
        ApplyLevelName00432B80(static_cast<char*>(m_owned_0c0));
        if (pathing_180 != 0) {
            ReadWaypointFile0043A0F0();
        }
        return;
    }
failed:
    g_octree_6598a4 = 0;
    spatial_000.flags_00 |= 0x80000000;
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
    current_sector = -1;
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
            WriteMember(this, 0x0c + axis * 4,
                        ReadHeader<unsigned long>(header, 0x0e + axis * 4));
            WriteMember(this, 0x18 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x1a + axis * 4));
            WriteMember(this, 0x24 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x26 + axis * 4));
            WriteMember(this, 0x30 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x32 + axis * 4));
            WriteMember(this, 0x78 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x3e + axis * 4));
            WriteMember(this, 0x84 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x4a + axis * 4));
            WriteMember(this, 0xa4 + axis * 4,
                        ReadHeader<unsigned long>(header, 0x56 + axis * 4));
        }

        WriteMember(this, 0x64,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xa8) *
                        ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x68,
                    ReadHeader<unsigned long>(reinterpret_cast<unsigned char*>(this), 0xac));
        WriteMember(this, 0x44, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x58, ReadHeader<unsigned short>(header, 0x64));
        WriteMember(this, 0x46, ReadHeader<unsigned short>(header, 0x60));
        WriteMember(this, 0x52, ReadHeader<unsigned short>(header, 0x62));
        WriteMember(this, 0x74, ReadHeader<unsigned long>(header, 0x66));
        m_positional_1a8 = ReadHeader<unsigned long>(header, 0x9a);
        m_positional_1ac = ReadHeader<unsigned long>(header, 0xa2);
        m_meshCount_1b4 = ReadHeader<unsigned long>(header, 0x9e);
        WriteMember(this, 0xb4, ReadHeader<unsigned long>(header, 0x6a));
        WriteMember(this, 0xb8, ReadHeader<unsigned long>(header, 0x6e));
        WriteMember(this, 0x3c, ReadHeader<unsigned long>(header, 0x72));
        WriteMember(this, 0xc8, ReadHeader<unsigned long>(header, 0x76));
        WriteMember(this, 0x40, ReadHeader<unsigned long>(header, 0x7a));
        m_positional_0cc = ReadHeader<unsigned long>(header, 0x82);
        m_positional_124 = ReadHeader<unsigned long>(header, 0x86);
        m_positional_128 = ReadHeader<unsigned long>(header, 0x8a);
        m_positional_138 = ReadHeader<unsigned long>(header, 0x92);
        WriteMember(this, 0x54, ReadHeader<unsigned long>(header, 0xa6));
        m_positional_178 = ReadHeader<unsigned long>(header, 0xac);
        m_positional_17c = ReadHeader<unsigned long>(header, 0xb4);
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
        m_owned_194 = new BitArray(
            ReadHeader<unsigned long>(header, 0x7a) < 5000
                ? 5000
                : ReadHeader<unsigned long>(header, 0x7a));
        m_accumulated_regions_198 = new BitArray(ReadHeader<unsigned long>(header, 0x66) + 1);
        m_owned_19c = new BitArray(ReadHeader<unsigned long>(header, 0x72));

        object_registry = new W8OctreeObjectRegistry;
        m_pRegionLinks_150 = new W8HashTable<unsigned int, unsigned short>;

        unsigned int visited_size = ReadHeader<unsigned short>(header, 0x64) + 1;
        m_pfRegsVisited = static_cast<unsigned char*>(malloc(visited_size));
        if (m_pfRegsVisited == 0) {
            srAssertFail("m_pfRegsVisited",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                         0x36a, "InitOctree: Couldn't allocate m_pfRegsVisited.");
        }
        memset(m_pfRegsVisited, 0, visited_size);
        m_reset_visibility_168 = 1;
    }

    m_aulGDObjs = static_cast<unsigned long*>(malloc(40000));
    if (m_aulGDObjs == 0) {
        srAssertFail("m_aulGDObjs",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                     0x372, "InitOctree: Couldn't allocate m_aulGDObjs.");
    }
    WriteMember(this, 0x6c, static_cast<unsigned short>(3));
    m_fAccumulating = 1;
    ToggleUpdateSuspension00434020(0);
}

// FUNCTION: WIZ8 0x0042e440
void W8Octree::AddLoadedProp(void* prop)
{
    if (m_fAccumulating) {
        if (m_usNumPropsLoaded >= (unsigned short)m_ulNumProps) {
            srAssertFail(
                "m_usNumPropsLoaded<(UINT16)m_ulNumProps",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x485,
                "Too many props loaded for Octree");
        }
        m_papProps[m_usNumPropsLoaded] = static_cast<W8Prop*>(prop);
        m_usNumPropsLoaded++;
        m_papProps[m_usNumPropsLoaded] = 0;
    }
}

// FUNCTION: WIZ8 0x0042e4c0
void W8Octree::AddLoadedParticle(void* particle)
{
    if (m_fAccumulating) {
        if (m_usNumParticlesLoaded >= (unsigned short)m_ulNumParticles) {
            srAssertFail(
                "m_usNumParticlesLoaded<(UINT16)m_ulNumParticles",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Octree.cpp",
                0x49d,
                "Too many particles loaded for Octree");
        }
        m_papParticles[m_usNumParticlesLoaded] = static_cast<stParticle*>(particle);
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
        SaveRegionLinks004331F0(static_cast<char*>(m_owned_0c0));
    }
    if (m_positional_16d != 0) {
        SavePoints00432D60(static_cast<char*>(m_owned_0c0));
    }
    if (pathing_180 != 0) {
        pathing_180->SaveWaypointSnapshot00459400(0);
    }

    free(m_aulGDObjs);
    free(m_owned_09c);
    free(m_owned_0a0);
    free(m_owned_0d0);
    free(m_owned_0d4);
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

/* The node whose 0x1c is non-null is the only kind worth attaching. */

// GLOBAL: WIZ8 0x006598ac
int g_shared_mark_006598ac;


/* Attach a visited set to the walker, but only one that has been built. */
// FUNCTION: WIZ8 0x0042e3e0
void W8Octree::SetVisitedSet0042E3E0(BitArray* visited)
{
    if (visited->puiIndex != 0) {
        m_pPropSunBits = visited;
    }
}

/* Advance or reset the walk's mark. A negative offset takes the shared
   counter's value as this walk's base; anything else advances the shared
   counter and reports whether that mark has already been visited - which with
   no set attached is always no. */
// FUNCTION: WIZ8 0x0042e400
int W8Octree::MarkVisited0042E400(int offset)
{
    if (offset < 0) {
        mark_base_184 = g_shared_mark_006598ac;
        return 0;
    }
    g_shared_mark_006598ac = mark_base_184 + offset;
    if (m_pPropSunBits != 0 && m_pPropSunBits->Test(g_shared_mark_006598ac)) {
        return 1;
    }
    return 0;
}

/* Visit a point handed over by address, copied to the stack first so the
   caller's copy is not the one the traversal holds. */
// FUNCTION: WIZ8 0x0042e620
void W8Octree::VisitPointCopy0042E620(
    unsigned short location_id, srVector3T<float>* position)
{
    srVector3T<float> copy;

    copy.x = position->x;
    copy.y = position->y;
    copy.z = position->z;
    UpdateMonsterLocation(location_id, &copy);
}

/* Start a traversal of the twelfth kind. A limit of zero means no limit, which
   is what the -1 stands for. */
// FUNCTION: WIZ8 0x0042ef00
unsigned int __stdcall OctreeTraverseKind12(
    void* walker, void* arg_2, void* arg_3, unsigned short limit)
{
    unsigned int bound = (unsigned int)-1;

    if (limit != 0) {
        bound = limit;
    }
    return OctreeTraverse(walker, arg_2, arg_3, 0xc, bound);
}

/* Queue one node of the thirteenth kind, with its three coordinates converted
   from floating point - which is what puts three ftol calls in a row here. */
// FUNCTION: WIZ8 0x0042e810
void W8Octree::QueueOctreeKind130042E810(
    int id,
    const srVector3T<float>* position)
{
    int point[3];

    point[0] = (int)((position->x - spatial_000.minimum_0c.x) /
                     spatial_000.node_extent_70);
    point[1] = (int)((position->y - spatial_000.minimum_0c.y) /
                     spatial_000.node_extent_70);
    point[2] = (int)((position->z - spatial_000.minimum_0c.z) /
                     spatial_000.node_extent_70);
    object_registry->RegisterObjectCell(0xd, id + 1, point);
}

/* Convert a world position to cell coordinates, tracking whether it stays
   inside the spatial minimum/maximum box. Callers only use the coordinates;
   the in-range result the image also computes is not consumed. */
// FUNCTION: WIZ8 0x00431440
void W8Octree::WorldPositionToCell00431440(
    const srVector3T<float>* position, int* point)
{
    unsigned char inside = 1;

    for (int axis = 0; axis < 3; ++axis) {
        point[axis] = (int)(&position->x)[axis];
        if ((&position->x)[axis] < (&spatial_000.minimum_0c.x)[axis] ||
            (&spatial_000.minimum_0c.x)[axis + 3] < (&position->x)[axis]) {
            inside = 0;
        }
    }
    (void)inside;
}

/* Clamp a position under the spatial ceiling, settle it to the ground through
   the surface walk, and keep the settled height only when something was hit.
   The walk reports through the flag; the height it wrote is discarded on a
   miss. */
// FUNCTION: WIZ8 0x00431DA0
void W8Octree::AdjustPosition00431DA0(
    srVector3T<float>* position, unsigned int mode)
{
    srVector3T<float> adjusted;
    unsigned char hit = 0;

    if (spatial_000.clipped_maximum_30.y < position->y) {
        position->y = spatial_000.clipped_maximum_30.y;
    }
    adjusted = *position;
    SettleToGround00433820(&adjusted, &hit, mode, 200.0f);
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
unsigned int W8Octree::AdvanceNavigator(
    W8NavigatorMovementState* movement, float radius, float separation)
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
        return pathing_180->StepMonsterAlongPath00467150(
            movement, radius, separation);
    }
    if (g_navigator_link_mode_00659c10 != 0) {
        return 1;
    }
    vecDir = movement->target_position_04c - movement->position_040;
    vecDir.y = 0.0f;
    distance = (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
    step = g_object_6598bc->GetValue28() * movement->movement_scale_060 *
        g_rate_006068EC * g_world_scale_005ebc40;
    if (step >= distance) {
        step = distance;
    } else {
        reached = 0;
    }
    if ((double)(vecDir.x * vecDir.x + vecDir.z * vecDir.z) != g_zero_005ebb40) {
        vecDir.y = 0.0f;
        step = step / (float)sqrt(vecDir.x * vecDir.x + vecDir.z * vecDir.z);
        vecDir.x = vecDir.x * step;
        vecDir.z = vecDir.z * step;
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
void W8Octree::AdjustPortalDestination(
    srVector3T<float>* destination, const srVector3T<float>* source)
{
    srVector3T<float> local_destination;
    srVector3T<float> local_source;
    srVector3T<float> probe;
    unsigned char hit;

    if (pathing_180 == 0) {
        return;
    }
    if (pathing_180->flag_1c8 == 0 && pathing_180->m_ulNumSurfaces != 0) {
        return;
    }
    local_destination = *destination;
    local_source = *source;

    hit = 0;
    if (local_destination.y > spatial_000.clipped_maximum_30.y) {
        local_destination.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_destination;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_destination.y = probe.y;
    }

    hit = 0;
    if (local_source.y > spatial_000.clipped_maximum_30.y) {
        local_source.y = spatial_000.clipped_maximum_30.y;
    }
    probe = local_source;
    SettleToGround00433820(&probe, &hit, 1, 500.0f);
    if (hit != 0) {
        local_source.y = probe.y;
    }
    pathing_180->EditTeleportalLink(&local_destination, &local_source);
}

/* Release the location-variable names and empty their parallel value and
   level vectors. Trigger.cpp creates the names as copied character arrays. */

// FUNCTION: WIZ8 0x004374C0
unsigned char ReadVector4Array004374C0(
    int file, srVector4T<float>* values, int count)
{
    return FileRead(file, values,
        count * sizeof(srVector4T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x004374E0
unsigned char ReadVector3Array004374E0(
    int file, srVector3T<float>* values, int count)
{
    return FileRead(file, values,
        count * sizeof(srVector3T<float>), 0) & 1;
}

// FUNCTION: WIZ8 0x00437510
unsigned char ReadVector2Array00437510(
    int file, srVector2T<float>* values, int count)
{
    return FileRead(file, values,
        count * sizeof(srVector2T<float>), 0) & 1;
}
