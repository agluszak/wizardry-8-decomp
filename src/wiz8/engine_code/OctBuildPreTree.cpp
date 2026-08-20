#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include "surrender/srHeap.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void Function497690(int channel, const char* message);
extern float g_float_005ed038;
extern float g_float_005ec52c;
extern const float g_world_scale_005ebc40;
extern const float g_startup_near_limit_005ec000;

#define OCT_BUILD_PRE_TREE_CPP \
    "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildPreTree.cpp"

struct W8OctRegionPolygon {
    unsigned char positional_00[0x32];
    unsigned short region_32;
    unsigned char positional_34[0x40];
};

struct W8OctRegionGameData {
    unsigned long positional_00;
    void* positional_04;
    unsigned long polygon_count_08;
    W8OctRegionPolygon* polygons_0c;
};

/* This zero-storage node form is constructed by OctBuildTree when its
   pre-tree ownership mode is active.  Its immediately following conversion
   methods and the first assertion-backed boundary at 0x004B19F0 establish
   Engine Code\OctBuildPreTree.cpp as the owning cluster. */
// FUNCTION: WIZ8 0x004af760
W8CountedOctBuildNode004AF760::W8CountedOctBuildNode004AF760()
{
    ++g_value_65be60;
}

// FUNCTION: WIZ8 0x004af780
W8CountedOctBuildNode004AF760::~W8CountedOctBuildNode004AF760()
{
    if (children_00[1] != 0) {
        free(children_00[1]);
    }
    if (g_value_65be60 != 0) {
        --g_value_65be60;
    }
}

/* Replace the two polygon-link chains used by pre-tree leaves with compact,
   null-terminated surface arrays. */
// FUNCTION: WIZ8 0x004af7b0
unsigned char W8OctBuildNode00446330::RearrangeNodePolys004AF7B0(
    short current_depth, short target_depth)
{
    if (current_depth == target_depth) {
        for (short mode = 2; mode < 4; ++mode) {
            if (leaf_kind_2a != 0 && links_00[mode] != 0) {
                g_value_65be58 = 0;
                CollectLinkedSurfaces004AF8F0(
                    current_depth, target_depth, mode);
                W8GDSurface** surfaces = static_cast<W8GDSurface**>(
                    malloc(g_value_65be58 * sizeof(W8GDSurface*) +
                           sizeof(W8GDSurface*)));
                if (surfaces == 0) {
                    Function497690(
                        7,
                        "RearrangeNodePolys: Could not allocate poly list.\n");
                    return 0;
                }
                unsigned long index;
                for (index = 0; index < g_value_65be58; ++index) {
                    surfaces[index] = g_pointer_65be64[index];
                }
                surfaces[index] = 0;
                surface_arrays_00[mode] = surfaces;
            }
        }
    }
    else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                children_00[child]->RearrangeNodePolys004AF7B0(
                    current_depth + 1, target_depth);
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004af8f0
int W8OctBuildNode00446330::CollectLinkedSurfaces004AF8F0(
    short current_depth, short target_depth, short mode)
{
    int count = 0;
    if (current_depth == target_depth) {
        if (1 < mode && mode < 4 && links_00[mode] != 0) {
            W8OctBuildLink* link = links_00[mode];
            do {
                g_pointer_65be64[g_value_65be58++] = link->surface_00;
                if (10000 < g_value_65be58) {
                    Function497690(
                        7, "OctBuildPreTree::m_ppPolyList too long.");
                    return 0;
                }
                link = link->next_04;
                ++count;
            } while (link != 0);
        }
    }
    else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                count += children_00[child]->CollectLinkedSurfaces004AF8F0(
                    current_depth + 1, target_depth, mode);
            }
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x004af9b0
int W8OctBuildNode00446330::CollectSurfaceArray004AF9B0(short mode)
{
    if (leaf_kind_2a != 0 && positional_2c == 0) {
        W8GDSurface** surfaces = surface_arrays_00[mode];
        if (surfaces != 0) {
            while (*surfaces != 0) {
                g_pointer_65be64[g_value_65be58++] = *surfaces++;
            }
        }
        return g_value_65be58;
    }
    for (int child = 0; child != 8; ++child) {
        if (children_00[child] != 0) {
            children_00[child]->CollectSurfaceArray004AF9B0(mode);
        }
    }
    return g_value_65be58;
}

/* Consume the counted build nodes into the compact arrays owned by the
   runtime pre-tree.  Leaf polygon pointers are deduplicated before their
   persistent surface indices are appended; branch children are converted and
   destroyed as soon as their compact indices have been recorded. */
// FUNCTION: WIZ8 0x004afa30
unsigned long W8OctBuildNode00446330::ConvertToOctPreTree004AFA30(
    unsigned short depth, W8OctPreTree004679E0* tree)
{
    unsigned long node_index;

    if (depth == tree->spatial_000.depth_44) {
        node_index = tree->m_positional_0b8++;

        g_value_65be58 = 0;
        CollectSurfaceArray004AF9B0(2);
        if (g_value_65be58 != 0) {
            unsigned long unique_count = 0;
            for (unsigned long source = 0; source < g_value_65be58; ++source) {
                bool found = false;
                for (unsigned long existing = 0;
                     existing < source && !found;
                     ++existing) {
                    if (g_pointer_65be64[source] ==
                        g_pointer_65be64[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_pointer_65be64[unique_count++] =
                        g_pointer_65be64[source];
                }
            }
            g_value_65be58 = unique_count;
            tree->m_owned_0d0[tree->polygon_cursor_3a0] = unique_count;
            tree->m_owned_0a0[node_index].polygon_offset_08 =
                tree->polygon_cursor_3a0;
            ++tree->polygon_cursor_3a0;
            for (unsigned long surface = 0;
                 surface < g_value_65be58;
                 ++surface) {
                tree->m_owned_0d0[tree->polygon_cursor_3a0++] =
                    g_pointer_65be64[surface]->index_04;
            }
            free(surface_arrays_00[2]);
            surface_arrays_00[2] = 0;
        }

        g_value_65be58 = 0;
        CollectSurfaceArray004AF9B0(3);
        if (g_value_65be58 != 0) {
            unsigned long unique_count = 0;
            for (unsigned long source = 0; source < g_value_65be58; ++source) {
                bool found = false;
                for (unsigned long existing = 0;
                     existing < source && !found;
                     ++existing) {
                    if (g_pointer_65be64[source] ==
                        g_pointer_65be64[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_pointer_65be64[unique_count++] =
                        g_pointer_65be64[source];
                }
            }
            g_value_65be58 = unique_count;
            tree->m_owned_12c[tree->m_positional_124] = unique_count;
            tree->m_owned_0a0[node_index].gd_polygon_offset_0c =
                tree->m_positional_124;
            ++tree->m_positional_124;
            for (unsigned long surface = 0;
                 surface < g_value_65be58;
                 ++surface) {
                tree->m_owned_12c[tree->m_positional_124++] =
                    g_pointer_65be64[surface]->index_04;
            }
            free(surface_arrays_00[3]);
            surface_arrays_00[3] = 0;
        }

        unsigned short* regions = region_arrays_00[1];
        if (regions != 0 && *regions != 0) {
            tree->m_owned_0a0[node_index].region_offset_04 =
                tree->m_positional_138;
            while (*regions != 0) {
                tree->m_owned_148[tree->m_positional_138++] = *regions++;
            }
            tree->m_owned_148[tree->m_positional_138++] = 0;
        }
        if (region_arrays_00[1] != 0) {
            free(region_arrays_00[1]);
            region_arrays_00[1] = 0;
        }
    }
    else {
        node_index = tree->m_positional_0b4++;
        for (unsigned long child = 0; child < 8; ++child) {
            if (children_00[child] != 0) {
                tree->m_owned_09c[node_index].children_04[child] =
                    children_00[child]->ConvertToOctPreTree004AFA30(
                        depth + 1, tree);
                delete static_cast<W8CountedOctBuildNode004AF760*>(
                    children_00[child]);
                children_00[child] = 0;
            }
        }
        tree->m_owned_09c[node_index].positional_02 = positional_28;
        tree->m_owned_09c[node_index].positional_00 = positional_2c;
    }
    return node_index;
}

/* Extend the surface build tree with the storage consumed by the destructive
   pre-tree conversion pass. */
// FUNCTION: WIZ8 0x004afda0
W8OctBuildPreTree004AFDA0::W8OctBuildPreTree004AFDA0(
    float leaf_size,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum,
    unsigned short item_limit,
    unsigned long path_capacity,
    short extent_mode)
    : W8OctBuildTree00446390(
          leaf_size, minimum, maximum, item_limit, extent_mode)
{
    active_f4 = 1;
    use_owned_nodes_b4 = 1;
    game_data_134 = 0;
    positional_138 = 0;
    positional_13c = 0;
    positional_b8 = 0;
    memset(level_counts_c4, 0, sizeof(level_counts_c4));
    path_capacity_bc = path_capacity;
    selected_depth_c0 = 0;
    region_paths_ec = 0;
    region_path_count_f0 = 0;
    region_bits_f8 = 0;
    region_centers_fc = 0;
    positional_124 = 0;
    positional_128 = 0;
    positional_100 = 0;
    g_pointer_65be64 =
        static_cast<W8GDSurface**>(malloc(10000 * sizeof(W8GDSurface*)));
    g_pointer_65be68 =
        static_cast<W8GDSurface**>(malloc(10000 * sizeof(W8GDSurface*)));
    mesh_particle_lookup_104 = 0;
    mesh_particles_108 = 0;
    mesh_particle_count_10c = 0;
    mesh_prop_lookup_110 = 0;
    mesh_props_114 = 0;
    mesh_prop_count_118 = 0;
    particle_count_11c = 0;
    prop_count_120 = 0;
}

/* Seed a temporary root when the build tree is still empty, account for the
   inserted geometry kind, and route the point or box through the ordinary
   recursive region-map walk. */
// FUNCTION: WIZ8 0x004b06e0
unsigned char W8OctBuildPreTree004AFDA0::UpdateRegionForGeometry004B06E0(
    const srVector3T<float>* geometry,
    short value,
    short mode)
{
    W8OctSpatialState0046CCC0 working(&spatial_00);
    if (spatial_00.root_90 == 0) {
        working.root_90 = new W8CountedOctBuildNode004AF760;
        spatial_00.root_90 = working.root_90;
    }
    if (mode == 2) {
        ++spatial_00.state_3c;
    }
    else if (mode == 3) {
        ++spatial_00.item_count_40;
    }
    working.depth_44 = 0;
    working.level_kind_6c = 1;
    return UpdateRegionMap004B07E0(&working, geometry, value, mode);
}

/* Walk the build tree against either a point or an axis-aligned volume and
   refresh the selected region-to-short association for every intersected
   leaf. The temporary child record carries the exact subcell bounds into the
   recursive call. */
// FUNCTION: WIZ8 0x004b07e0
unsigned char W8OctBuildPreTree004AFDA0::UpdateRegionMap004B07E0(
    const W8OctSpatialState0046CCC0* spatial,
    const srVector3T<float>* geometry,
    short value,
    short mode)
{
    W8OctSpatialState0046CCC0 child(spatial);
    unsigned char changed = 0;

    if (child.depth_44 >= 16) {
        return 0;
    }

    if (child.depth_44 < spatial_00.depth_44) {
        int child_index = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++child_index) {
                    child.minimum_0c.x =
                        x * child.extent_04 + spatial->minimum_0c.x;
                    child.maximum_18.x =
                        child.minimum_0c.x + child.extent_04;
                    child.minimum_0c.y =
                        y * child.extent_04 + spatial->minimum_0c.y;
                    child.maximum_18.y =
                        child.minimum_0c.y + child.extent_04;
                    child.minimum_0c.z =
                        z * child.extent_04 + spatial->minimum_0c.z;
                    child.maximum_18.z =
                        child.minimum_0c.z + child.extent_04;

                    unsigned char intersects;
                    if (mode == 6) {
                        intersects = PointInsideBounds0046D4D0(
                            &child.minimum_0c, geometry);
                    }
                    else if (mode == 5) {
                        intersects = BoundsOverlap0046D470(
                            &child.minimum_0c, geometry);
                    }

                    W8OctBuildNode00446330* parent =
                        static_cast<W8OctBuildNode00446330*>(
                            spatial->root_90);
                    child.root_90 = parent->children_00[child_index];
                    if (intersects != 0 && child.root_90 != 0) {
                        W8OctBuildNode00446330* node =
                            static_cast<W8OctBuildNode00446330*>(
                                child.root_90);
                        unsigned short region = node->positional_28;
                        if (region != 0) {
                            if (mode == 6) {
                                positional_12c->Remove(&region, &value);
                                positional_12c->Insert(&region, &value);
                            }
                            else if (mode == 5) {
                                positional_130->Remove(&region, &value);
                                positional_130->Insert(&region, &value);
                            }
                            changed = 1;
                        }
                        if (UpdateRegionMap004B07E0(
                                &child, geometry, value, mode) != 0) {
                            changed = 1;
                        }
                    }
                }
            }
        }
    }
    else {
        W8OctBuildNode00446330* node =
            static_cast<W8OctBuildNode00446330*>(child.root_90);
        unsigned short region = node->positional_28;
        if (region != 0) {
            if (mode == 6) {
                positional_12c->Remove(&region, &value);
                positional_12c->Insert(&region, &value);
            }
            else if (mode == 5) {
                positional_130->Remove(&region, &value);
                positional_130->Insert(&region, &value);
            }
            changed = 1;
        }
    }
    return changed;
}

/* Choose the region-tree depth that fits the requested path capacity, build
   the temporary spatial hierarchy, assign every discovered path to its build
   node, and then derive the persistent region metadata. */
// FUNCTION: WIZ8 0x004b19f0
unsigned short W8OctBuildPreTree004AFDA0::BuildRegions004B19F0()
{
    W8OctSpatialState0046CCC0 working(&spatial_00);

    if (spatial_00.positional_58 == 0) {
        spatial_00.positional_58 = 1;
    }
    spatial_00.positional_46 = spatial_00.positional_58;
    selected_depth_c0 = spatial_00.depth_44;
    region_path_count_f0 = 0;

    while (path_capacity_bc < level_counts_c4[selected_depth_c0]) {
        --selected_depth_c0;
    }

    spatial_00.positional_52 = 0;
    int last_level = selected_depth_c0 - 1;
    float extent = spatial_00.extent_04;
    while ((int)spatial_00.positional_52 < last_level) {
        float next_extent = extent * g_float_005ebc7c;
        if (fabs(spatial_00.positional_54 - extent) <
            fabs(spatial_00.positional_54 - next_extent)) {
            break;
        }
        ++spatial_00.positional_52;
        extent = next_extent;
    }

    unsigned long level_count =
        level_counts_c4[spatial_00.positional_52];
    region_paths_ec = static_cast<unsigned long*>(
        malloc(level_count * sizeof(unsigned long) + 8));
    if (region_paths_ec == 0) {
        srAssertFail("m_pulRegPaths", OCT_BUILD_PRE_TREE_CPP, 0x6f1, 0);
    }

    region_centers_fc = static_cast<srVector3T<float>*>(srHeap.allocate(
        (level_count + 2 + spatial_00.positional_58) *
        sizeof(srVector3T<float>)));
    if (region_centers_fc == 0) {
        srAssertFail("m_psrvRegCenters", OCT_BUILD_PRE_TREE_CPP, 0x6f3, 0);
    }

    spatial_00.positional_54 = working.cell_size_08;
    positional_124 = new W8HashTable<unsigned short, unsigned long>;
    Function004B1D90(&working);
    positional_128 = new W8HashTable<unsigned int, short>;
    region_bits_f8 = new BitArray(spatial_00.positional_58);

    for (unsigned short index = 0; index < region_path_count_f0; ++index) {
        unsigned long path = region_paths_ec[index];
        W8OctBuildNode00446330* node = FindNode004B23F0(path);
        if (node->leaf_kind_2a != 0 && node->leaf_kind_2a < 25) {
            Function004B2450(node, path);
        }
    }

    for (unsigned long polygon = 1;
         polygon < game_data_134->polygon_count_08;
         ++polygon) {
        if (game_data_134->polygons_0c[polygon].region_32 == 0) {
            char message[252];
            sprintf(
                message, "Poly %d not found in ANY region.\n", (int)polygon);
            Function497690(6, message);
        }
    }

    Function004B2A20();
    Function004B3050(&working);
    Function004B3330();

    srHeap.free(region_centers_fc);
    region_centers_fc = 0;
    if (region_bits_f8 != 0) {
        region_bits_f8->FreeIndex();
        delete region_bits_f8;
    }
    region_bits_f8 = 0;

    if (spatial_00.positional_46 == 1) {
        spatial_00.positional_46 = 0;
    }
    return spatial_00.positional_58;
}

/* Merge one neighboring region into the region carried by the adjacent build
   node. Every packed path is re-keyed in the region table and the surviving
   center becomes the population-weighted average of both groups. */
// FUNCTION: WIZ8 0x004b25c0
unsigned char W8OctBuildPreTree004AFDA0::MergeRegion004B25C0(
    W8OctBuildNode00446330* node, const int* cell)
{
    unsigned long neighbor_path =
        ((cell[1] * 0x100 + cell[2]) * 0x100 + cell[3]) + cell[0];
    W8OctBuildNode00446330* neighbor = FindNode004B23F0(neighbor_path);
    if (neighbor == 0 || neighbor->leaf_kind_2a == 0 ||
        neighbor->leaf_kind_2a >= 100) {
        return 0;
    }

    unsigned short neighbor_region = neighbor->positional_28;
    unsigned short node_region = node->positional_28;
    if (neighbor_region == node_region) {
        return 0;
    }

    srVector3T<float>& node_center = region_centers_fc[node_region];
    srVector3T<float>& neighbor_center = region_centers_fc[neighbor_region];
    float delta_x = node_center.x - neighbor_center.x;
    float delta_y = node_center.y - neighbor_center.y;
    float delta_z = node_center.z - neighbor_center.z;
    float distance = (float)sqrt(
        delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);
    if (!(distance < spatial_00.positional_54 * g_float_005ec52c)) {
        return 0;
    }

    unsigned short combined_count =
        node->leaf_kind_2a + neighbor->leaf_kind_2a;
    region_bits_f8->Set(neighbor_region);

    unsigned long neighbor_count = 0;
    int position = -1;
    while ((position = positional_124->FindNextEntry(
                &neighbor_region, position)) != -1) {
        ++neighbor_count;
        W8OctBuildNode00446330* member =
            FindNode004B23F0(positional_124->entries[position].value);
        member->leaf_kind_2a = combined_count;
    }

    unsigned long moved_count = 0;
    unsigned long path = positional_124->Lookup(&node_region);
    while (path != 0) {
        ++moved_count;
        positional_124->Remove(&node_region, &path);
        W8OctBuildNode00446330* member = FindNode004B23F0(path);
        member->leaf_kind_2a = combined_count;
        member->positional_28 = neighbor_region;
        positional_124->Insert(&neighbor_region, &path);
        path = positional_124->Lookup(&node_region);
    }

    neighbor_center =
        (node_center * (double)moved_count +
         neighbor_center * (double)neighbor_count) /
        (double)(moved_count + neighbor_count);
    return 1;
}

/* Associate every non-cloud particle with the regions containing its origin
   or any corner of its serialized bounds. Particles still outside the built
   regions lead the compact list, followed by one null-terminated list per
   region; the lookup table stores each region's offset into that list. */
// FUNCTION: WIZ8 0x004b3820
unsigned char W8OctBuildPreTree004AFDA0::BuildParticleRegions004B3820(
    const W8VersionedLevelParticleRecord* particles,
    int particle_count)
{
    particle_count_11c = particle_count;
    positional_12c = new W8HashTable<unsigned short, short>;

    short region_particles[4998];
    region_particles[0] = 0;
    unsigned short list_count = 0;
    int particle_number = 0;

    for (int particle_index = 0;
         particle_index < particle_count;
         ++particle_index) {
        const W8LevelParticleRecord004BD0D0& particle =
            particles[particle_index].particle_001;
        char name[64];
        strcpy(name, particle.name);
        _strupr(name);
        if (strncmp(name, "CLOUD", 5) == 0) {
            continue;
        }

        ++particle_number;
        short particle_value = (short)particle_number;
        srVector3T<float> position;
        position.x = particle.location.x * g_world_scale_005ebc40;
        position.y = particle.location.y * g_world_scale_005ebc40;
        position.z = particle.location.z * g_world_scale_005ebc40;

        bool mapped = false;
        for (unsigned short region_index = 1;
             region_index < spatial_00.positional_46;
             ++region_index) {
            W8OctRegionVolume0049E460* volume =
                spatial_00.owned_5c + region_index;
            if (volume->ContainsPoint0049E460(&position) != 0) {
                unsigned short region = volume->region_04;
                bool present = false;
                int entry = -1;
                while ((entry = positional_12c->FindNextEntry(
                            &region, entry)) != -1) {
                    if (positional_12c->entries[entry].value ==
                        particle_value) {
                        present = true;
                        break;
                    }
                }
                if (!present) {
                    positional_12c->Insert(&region, &particle_value);
                }
                mapped = true;
            }
        }
        if (mapped) {
            continue;
        }

        if (UpdateRegionForGeometry004B06E0(
                &position, particle_value, 6) == 0) {
            srVector3T<float> extent;
            bool has_bounds = false;
            if (particle.bounds_mode == 1) {
                extent.x = particle.bounds_extent.x *
                           g_startup_near_limit_005ec000;
                extent.y = particle.bounds_extent.y *
                           g_startup_near_limit_005ec000;
                extent.z = particle.bounds_extent.z *
                           g_startup_near_limit_005ec000;
                has_bounds = true;
            }
            else if (particle.bounds_mode == 2 &&
                     particle.bounds_radius > g_float_005ebb34) {
                extent.x = particle.bounds_origin.x *
                           g_world_scale_005ebc40;
                extent.y = particle.bounds_origin.y *
                           g_world_scale_005ebc40;
                extent.z = particle.bounds_origin.z *
                           g_world_scale_005ebc40;
                has_bounds = true;
            }

            if (has_bounds) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;
                minimum.x = position.x - extent.x;
                minimum.y = position.y - extent.y;
                minimum.z = position.z - extent.z;
                maximum.x = position.x + extent.x;
                maximum.y = position.y + extent.y;
                maximum.z = position.z + extent.z;

                for (int x = 0; x != 2; ++x) {
                    for (int y = 0; y != 2; ++y) {
                        for (int z = 0; z != 2; ++z) {
                            srVector3T<float> corner;
                            corner.x = x == 0 ? minimum.x : maximum.x;
                            corner.y = y == 0 ? minimum.y : maximum.y;
                            corner.z = z == 0 ? minimum.z : maximum.z;

                            bool corner_mapped = false;
                            for (unsigned short region_index = 1;
                                 region_index < spatial_00.positional_46;
                                 ++region_index) {
                                W8OctRegionVolume0049E460* volume =
                                    spatial_00.owned_5c + region_index;
                                if (volume->ContainsPoint0049E460(
                                        &corner) != 0) {
                                    unsigned short region = volume->region_04;
                                    bool present = false;
                                    int entry = -1;
                                    while ((entry =
                                                positional_12c->FindNextEntry(
                                                    &region, entry)) != -1) {
                                        if (positional_12c->entries[entry]
                                                .value == particle_value) {
                                            present = true;
                                            break;
                                        }
                                    }
                                    if (!present) {
                                        positional_12c->Insert(
                                            &region, &particle_value);
                                    }
                                    corner_mapped = true;
                                    mapped = true;
                                }
                            }
                            if (!corner_mapped &&
                                UpdateRegionForGeometry004B06E0(
                                    &corner, particle_value, 6) != 0) {
                                mapped = true;
                            }
                        }
                    }
                }
            }

            if (!mapped) {
                region_particles[list_count++] = particle_value;
            }
        }
    }

    if (list_count != 0) {
        region_particles[list_count++] = 0;
    }
    else {
        list_count = 1;
    }

    unsigned short region_count =
        (unsigned short)spatial_00.positional_74;
    mesh_particle_lookup_104 = static_cast<unsigned short*>(
        malloc(region_count * sizeof(unsigned short)));
    memset(mesh_particle_lookup_104, 0,
           region_count * sizeof(unsigned short));

    for (unsigned short region = 1; region < region_count; ++region) {
        int entry = -1;
        bool first = true;
        while ((entry = positional_12c->FindNextEntry(
                    &region, entry)) != -1) {
            if (first) {
                mesh_particle_lookup_104[region] = list_count;
                first = false;
            }
            region_particles[list_count++] =
                positional_12c->entries[entry].value;
        }
        if (!first) {
            region_particles[list_count++] = 0;
        }
    }

    mesh_particles_108 = static_cast<unsigned short*>(
        malloc(list_count * sizeof(unsigned short)));
    if (mesh_particles_108 != 0) {
        memcpy(mesh_particles_108, region_particles,
               list_count * sizeof(unsigned short));
    }
    mesh_particle_count_10c = list_count;
    delete positional_12c;
    return 1;
}

/* Accumulate the two preprocessing geometry banks into one region-to-prop
   table. The first call owns the shared scratch list; the final call appends
   the second bank, emits the compact lookup/list pair, and releases scratch. */
// FUNCTION: WIZ8 0x004b3f90
unsigned char W8OctBuildPreTree004AFDA0::BuildGeometryRegions004B3F90(
    const W8OctRegionGeometryRecord004B3F90* records,
    int record_count,
    int base_index,
    unsigned char finalize)
{
    if (finalize == 0) {
        positional_130 = new W8HashTable<unsigned short, short>;
        g_pointer_65be5c = static_cast<unsigned short*>(malloc(10000));
        g_pointer_65be5c[0] = 0;
        g_value_65be6c = 0;
    }

    for (int record_index = 0;
         record_index < record_count;
         ++record_index) {
        const W8OctRegionGeometryRecord004B3F90& record =
            records[record_index];
        short value = (short)(base_index + 1 + record_index);
        bool mapped = false;

        for (unsigned short region_index = 1;
             region_index < spatial_00.positional_46;
             ++region_index) {
            W8OctRegionVolume0049E460* volume =
                spatial_00.owned_5c + region_index;
            for (unsigned char bounds_index = 0;
                 bounds_index < record.bounds_count_09a;
                 ++bounds_index) {
                const srVector3T<float>* bounds =
                    record.bounds_09b + bounds_index * 2;
                for (int x = 0; x != 2; ++x) {
                    for (int y = 0; y != 2; ++y) {
                        for (int z = 0; z != 2; ++z) {
                            srVector3T<float> corner;
                            corner.x = bounds[x].x * g_world_scale_005ebc40;
                            corner.y = bounds[y].y * g_world_scale_005ebc40;
                            corner.z = bounds[z].z * g_world_scale_005ebc40;
                            if (volume->ContainsPoint0049E460(&corner) != 0) {
                                unsigned short region = volume->region_04;
                                bool present = false;
                                int entry = -1;
                                while ((entry =
                                            positional_130->FindNextEntry(
                                                &region, entry)) != -1) {
                                    if (positional_130->entries[entry].value ==
                                        value) {
                                        present = true;
                                        break;
                                    }
                                }
                                if (!present) {
                                    positional_130->Insert(&region, &value);
                                }
                                mapped = true;
                            }
                        }
                    }
                }
            }
        }

        srVector3T<float> aggregate[2];
        aggregate[0].x = 1000000.0f;
        aggregate[0].y = 1000000.0f;
        aggregate[0].z = 1000000.0f;
        aggregate[1].x = -1000000.0f;
        aggregate[1].y = -1000000.0f;
        aggregate[1].z = -1000000.0f;
        for (unsigned char bounds_index = 0;
             bounds_index < record.bounds_count_09a;
             ++bounds_index) {
            const srVector3T<float>* bounds =
                record.bounds_09b + bounds_index * 2;
            for (int endpoint = 0; endpoint != 2; ++endpoint) {
                srVector3T<float> point;
                point.x = bounds[endpoint].x * g_world_scale_005ebc40;
                point.y = bounds[endpoint].y * g_world_scale_005ebc40;
                point.z = bounds[endpoint].z * g_world_scale_005ebc40;
                for (int axis = 0; axis != 3; ++axis) {
                    if ((&point.x)[axis] < (&aggregate[0].x)[axis]) {
                        (&aggregate[0].x)[axis] = (&point.x)[axis];
                    }
                    if ((&aggregate[1].x)[axis] < (&point.x)[axis]) {
                        (&aggregate[1].x)[axis] = (&point.x)[axis];
                    }
                }
            }
        }

        if (UpdateRegionForGeometry004B06E0(
                aggregate, value, 5) == 0 && !mapped) {
            g_pointer_65be5c[g_value_65be6c++] = value;
        }
    }

    if (finalize != 0) {
        if (g_value_65be6c == 0) {
            g_value_65be6c = 1;
        }
        else {
            g_pointer_65be5c[g_value_65be6c++] = 0;
        }

        unsigned long region_count = spatial_00.positional_74;
        mesh_prop_lookup_110 = static_cast<unsigned short*>(
            malloc(region_count * sizeof(unsigned short)));
        for (unsigned long region_value = 1;
             region_value < region_count;
             ++region_value) {
            unsigned short region = (unsigned short)region_value;
            mesh_prop_lookup_110[region_value] = 0;
            int entry = -1;
            bool first = true;
            while ((entry = positional_130->FindNextEntry(
                        &region, entry)) != -1) {
                if (first) {
                    mesh_prop_lookup_110[region_value] = g_value_65be6c;
                    first = false;
                }
                g_pointer_65be5c[g_value_65be6c++] =
                    positional_130->entries[entry].value;
            }
            if (!first) {
                g_pointer_65be5c[g_value_65be6c++] = 0;
            }
        }

        mesh_props_114 = static_cast<unsigned short*>(
            malloc(g_value_65be6c * sizeof(unsigned short)));
        if (mesh_props_114 != 0) {
            memcpy(mesh_props_114, g_pointer_65be5c,
                   g_value_65be6c * sizeof(unsigned short));
        }
        mesh_prop_count_118 = g_value_65be6c;
        prop_count_120 = record_count + base_index;
        delete positional_130;
        free(g_pointer_65be5c);
    }
    return 1;
}

/* Build the compact runtime pre-tree, transfer the auxiliary ownership that
   already has runtime form, consume the counted build-node hierarchy, and
   populate the dense cell-to-leaf lookup. */
// FUNCTION: WIZ8 0x004b4640
W8OctPreTree004679E0* W8OctBuildPreTree004AFDA0::BuildOctPreTree004B4640()
{
    W8OctPreTree004679E0* tree = new W8OctPreTree004679E0;
    if (tree == 0) {
        return 0;
    }

    tree->m_owned_09c = static_cast<W8OctPreTreeBranch*>(
        malloc((g_value_65be60 * 9 + 0x12) * sizeof(unsigned long)));
    if (tree->m_owned_09c == 0) {
        return 0;
    }
    memset(tree->m_owned_09c, 0,
           (g_value_65be60 * 9 + 0x12) * sizeof(unsigned long));

    tree->m_owned_0a0 = static_cast<W8OctPreTreeLeaf*>(
        malloc((positional_a8 + 2) * sizeof(W8OctPreTreeLeaf)));
    if (tree->m_owned_0a0 == 0) {
        return 0;
    }
    memset(tree->m_owned_0a0, 0,
           (positional_a8 + 2) * sizeof(W8OctPreTreeLeaf));

    tree->m_owned_0d0 = static_cast<unsigned long*>(
        malloc(positional_a0 * 2 * sizeof(unsigned long)));
    if (tree->m_owned_0d0 == 0) {
        return 0;
    }
    memset(tree->m_owned_0d0, 0,
           positional_a0 * 2 * sizeof(unsigned long));

    tree->m_owned_148 = static_cast<unsigned short*>(
        malloc(positional_a8 * 0x50));
    if (tree->m_owned_148 == 0) {
        return 0;
    }
    memset(tree->m_owned_148, 0, positional_a8 * 0x50);

    tree->m_owned_12c = static_cast<unsigned long*>(
        malloc(positional_a4 * 2 * sizeof(unsigned long)));
    if (tree->m_owned_12c == 0) {
        return 0;
    }
    memset(tree->m_owned_12c, 0,
           positional_a4 * 2 * sizeof(unsigned long));

    tree->spatial_000.positional_46 = spatial_00.positional_46;
    tree->spatial_000.positional_52 = spatial_00.positional_52;
    tree->spatial_000.positional_74 = spatial_00.positional_74;
    tree->spatial_000.positional_58 = spatial_00.positional_58;

    tree->m_pusMeshParticleLookup = mesh_particle_lookup_104;
    mesh_particle_lookup_104 = 0;
    tree->m_pusMeshParticles = mesh_particles_108;
    mesh_particles_108 = 0;
    tree->m_positional_0e8 = mesh_particle_count_10c;
    tree->m_pusMeshPropLookup = mesh_prop_lookup_110;
    mesh_prop_lookup_110 = 0;
    tree->m_pusMeshProps = mesh_props_114;
    tree->m_positional_0f4 = mesh_prop_count_118;
    mesh_props_114 = 0;
    tree->m_ulNumParticles = particle_count_11c;
    tree->m_ulNumProps = prop_count_120;

    tree->spatial_000.depth_44 = spatial_00.depth_44;
    tree->spatial_000.node_extent_70 = spatial_00.node_extent_70;
    tree->spatial_000.positional_60 =
        spatial_00.positional_60 < g_float_005ed038
            ? spatial_00.positional_60
            : g_float_005ed038;
    while (selected_depth_c0 < tree->spatial_000.depth_44) {
        Function497690(6, "Collapsing tree by one level.");
        --tree->spatial_000.depth_44;
        tree->spatial_000.node_extent_70 += tree->spatial_000.node_extent_70;
    }

    tree->spatial_000.extent_04 = spatial_00.extent_04;
    tree->spatial_000.cell_size_08 = spatial_00.cell_size_08;
    tree->spatial_000.owned_5c = spatial_00.owned_5c;
    tree->positional_3a4 = game_data_134;
    tree->positional_3a8 = positional_138;
    tree->positional_3ac = positional_13c;
    tree->positional_3b0 = positional_b8;
    tree->spatial_000.item_count_40 = spatial_00.item_count_40;
    tree->spatial_000.state_3c = spatial_00.state_3c;
    tree->m_owned_190 = new BitArray(spatial_00.state_3c);
    tree->spatial_000.positional_54 = spatial_00.positional_54;
    tree->spatial_000.positional_50 = spatial_00.positional_50;
    tree->positional_29c = positional_124;
    positional_124 = 0;

    for (int axis = 0; axis != 3; ++axis) {
        (&tree->spatial_000.minimum_0c.x)[axis] =
            (&spatial_00.minimum_0c.x)[axis];
        (&tree->spatial_000.maximum_18.x)[axis] =
            (&spatial_00.maximum_18.x)[axis];
        (&tree->spatial_000.clipped_minimum_24.x)[axis] =
            (&spatial_00.clipped_minimum_24.x)[axis];
        (&tree->spatial_000.clipped_maximum_30.x)[axis] =
            (&spatial_00.clipped_maximum_30.x)[axis];
    }

    tree->m_positional_0b4 = 1;
    tree->m_positional_0b8 = 1;
    tree->polygon_cursor_3a0 = 1;
    tree->m_positional_138 = 1;
    tree->m_positional_124 = 1;
    tree->m_positional_13c = 0;
    tree->m_positional_128 = 0;

    W8OctBuildNode00446330* root =
        static_cast<W8OctBuildNode00446330*>(spatial_00.root_90);
    root->ConvertToOctPreTree004AFA30(0, tree);
    delete root;
    spatial_00.root_90 = 0;
    if (tree->m_positional_138 == 1) {
        tree->m_positional_138 = 0;
    }

    for (int grid_axis = 0; grid_axis != 3; ++grid_axis) {
        (&tree->m_positional_0a4)[grid_axis] =
            (int)(((&tree->spatial_000.clipped_maximum_30.x)[grid_axis] -
                   (&tree->spatial_000.clipped_minimum_24.x)[grid_axis]) /
                  tree->spatial_000.node_extent_70) +
            1;
    }
    tree->m_owned_0b0 = malloc(
        tree->m_positional_0ac * tree->m_positional_0a4 *
        tree->m_positional_0a8 * sizeof(unsigned long));
    unsigned long cell_index = 0;
    int point[3];
    for (point[0] = 0; point[0] < (int)tree->m_positional_0a4; ++point[0]) {
        for (point[1] = 0; point[1] < (int)tree->m_positional_0a8; ++point[1]) {
            for (point[2] = 0; point[2] < (int)tree->m_positional_0ac;
                 ++point[2]) {
                static_cast<unsigned long*>(tree->m_owned_0b0)[cell_index] =
                    tree->FindLeaf00433660(point);
                if (tree->m_positional_0b8 <
                    static_cast<unsigned long*>(tree->m_owned_0b0)[cell_index]) {
                    static_cast<unsigned long*>(
                        tree->m_owned_0b0)[cell_index] = 0;
                }
                ++cell_index;
            }
        }
    }
    tree->spatial_000.positional_68 = tree->m_positional_0ac;
    tree->spatial_000.positional_64 =
        tree->m_positional_0ac * tree->m_positional_0a8;
    return tree;
}

// FUNCTION: WIZ8 0x004afe90
int GetValue65BE60(void)
{
    return g_value_65be60;
}

/* Two value specializations of the same 16-bit-key open hash table are
   emitted at the end of OctBuildPreTree.cpp. The generic definition lives in
   stHash.hpp; callers inline the same grow and allocation operations. */
// TEMPLATE: WIZ8 0x004b4bd0
// W8HashTable<unsigned short,unsigned long>::Lookup

// TEMPLATE: WIZ8 0x004b4c30
// W8HashTable<unsigned short,short>::Insert

// TEMPLATE: WIZ8 0x004b4dd0
// W8HashTable<unsigned short,short>::Remove

// TEMPLATE: WIZ8 0x004b4e70
// W8HashTable<unsigned short,unsigned long>::Grow

// TEMPLATE: WIZ8 0x004b4fc0
// W8HashTable<unsigned short,unsigned long>::AllocateEntry

// TEMPLATE: WIZ8 0x004b5130
// W8HashTable<unsigned short,short>::Grow

// TEMPLATE: WIZ8 0x004b5270
// W8HashTable<unsigned short,short>::AllocateEntry
