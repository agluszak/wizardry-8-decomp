#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/Octree.h"

#include <stdlib.h>
#include <string.h>

extern void Function497690(int channel, const char* message);
extern float g_float_005ed038;

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
    positional_104 = 0;
    positional_108 = 0;
    positional_10c = 0;
    positional_110 = 0;
    positional_114 = 0;
    positional_118 = 0;
    positional_11c = 0;
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

    tree->m_pusMeshParticleLookup =
        static_cast<unsigned short*>(positional_104);
    positional_104 = 0;
    tree->m_pusMeshParticles = static_cast<unsigned short*>(positional_108);
    positional_108 = 0;
    tree->m_positional_0e8 = positional_10c;
    tree->m_pusMeshPropLookup = static_cast<unsigned short*>(positional_110);
    positional_110 = 0;
    tree->m_pusMeshProps = static_cast<unsigned short*>(positional_114);
    tree->m_positional_0f4 = positional_118;
    positional_114 = 0;
    tree->m_ulNumParticles = positional_11c;
    tree->m_ulNumProps = positional_120;

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
