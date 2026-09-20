#include "wiz8/engine_code/OctBuildPreTree.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/LevelFile.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "surrender/srHeap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GLOBAL: WIZ8 0x0065be60
int g_value_65be60;
// GLOBAL: WIZ8 0x0065be58
unsigned long g_value_65be58;
// GLOBAL: WIZ8 0x0065be64
void** g_pointer_65be64;
// GLOBAL: WIZ8 0x0065be68
W8GDSurface** g_pointer_65be68;
// GLOBAL: WIZ8 0x0065be5c
unsigned short* g_pointer_65be5c;
// GLOBAL: WIZ8 0x0065be6c
unsigned short g_value_65be6c;

// GLOBAL: WIZ8 0x005ed034
float g_float_005ed034 = -0.009999999776482582f;
// GLOBAL: WIZ8 0x005ed038
float g_float_005ed038 = 4000.0f;
// GLOBAL: WIZ8 0x005ec52c
float g_float_005ec52c = 3.0f;

#define OCT_BUILD_PRE_TREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildPreTree.cpp"

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
unsigned char W8OctBuildNode00446330::RearrangeNodePolys004AF7B0(short current_depth,
                                                                 short target_depth)
{
    if (current_depth == target_depth) {
        for (short mode = 2; mode < 4; ++mode) {
            if (leaf_kind_2a != 0 && links_00[mode] != 0) {
                g_value_65be58 = 0;
                CollectLinkedSurfaces004AF8F0(current_depth, target_depth, mode);
                void** surfaces =
                    static_cast<void**>(malloc(g_value_65be58 * sizeof(void*) + sizeof(void*)));
                if (surfaces == 0) {
                    ReportBuildStatus00497690(
                        7, "RearrangeNodePolys: Could not allocate poly list.\n");
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
    } else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                children_00[child]->RearrangeNodePolys004AF7B0(current_depth + 1, target_depth);
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004af8f0
int W8OctBuildNode00446330::CollectLinkedSurfaces004AF8F0(short current_depth, short target_depth,
                                                          short mode)
{
    int count = 0;
    if (current_depth == target_depth) {
        if (1 < mode && mode < 4 && links_00[mode] != 0) {
            W8OctBuildLink* link = links_00[mode];
            do {
                g_pointer_65be64[g_value_65be58++] = link->surface_00;
                if (10000 < g_value_65be58) {
                    ReportBuildStatus00497690(7, "OctBuildPreTree::m_ppPolyList too long.");
                    return 0;
                }
                link = link->next_04;
                ++count;
            } while (link != 0);
        }
    } else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                count += children_00[child]->CollectLinkedSurfaces004AF8F0(current_depth + 1,
                                                                           target_depth, mode);
            }
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x004af9b0
int W8OctBuildNode00446330::CollectSurfaceArray004AF9B0(short mode)
{
    if (leaf_kind_2a != 0 && positional_2c == 0) {
        void** surfaces = surface_arrays_00[mode];
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
unsigned long W8OctBuildNode00446330::ConvertToOctPreTree004AFA30(unsigned short depth,
                                                                  OctPreTree* tree)
{
    unsigned long node_index;

    if (depth == tree->spatial_000.depth_44) {
        node_index = tree->m_leaf_count_0b8++;

        g_value_65be58 = 0;
        CollectSurfaceArray004AF9B0(2);
        if (g_value_65be58 != 0) {
            unsigned long unique_count = 0;
            for (unsigned long source = 0; source < g_value_65be58; ++source) {
                bool found = false;
                for (unsigned long existing = 0; existing < source && !found; ++existing) {
                    if (g_pointer_65be64[source] == g_pointer_65be64[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_pointer_65be64[unique_count++] = g_pointer_65be64[source];
                }
            }
            g_value_65be58 = unique_count;
            tree->m_owned_0d0[tree->polygon_cursor_3a0] = unique_count;
            tree->m_owned_0a0[node_index].polygon_offset_08 = tree->polygon_cursor_3a0;
            ++tree->polygon_cursor_3a0;
            for (unsigned long surface = 0; surface < g_value_65be58; ++surface) {
                tree->m_owned_0d0[tree->polygon_cursor_3a0++] =
                    static_cast<W8OctRegionPolygon*>(g_pointer_65be64[surface])->ordinal_04;
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
                for (unsigned long existing = 0; existing < source && !found; ++existing) {
                    if (g_pointer_65be64[source] == g_pointer_65be64[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_pointer_65be64[unique_count++] = g_pointer_65be64[source];
                }
            }
            g_value_65be58 = unique_count;
            tree->m_owned_12c[tree->m_gd_surface_stream_len_124] = unique_count;
            tree->m_owned_0a0[node_index].gd_polygon_offset_0c = tree->m_gd_surface_stream_len_124;
            ++tree->m_gd_surface_stream_len_124;
            for (unsigned long surface = 0; surface < g_value_65be58; ++surface) {
                tree->m_owned_12c[tree->m_gd_surface_stream_len_124++] =
                    static_cast<W8GDSurface*>(g_pointer_65be64[surface])->index_04;
            }
            free(surface_arrays_00[3]);
            surface_arrays_00[3] = 0;
        }

        unsigned short* regions = region_arrays_00[1];
        if (regions != 0 && *regions != 0) {
            tree->m_owned_0a0[node_index].region_offset_04 = tree->m_region_list_len_138;
            while (*regions != 0) {
                tree->m_owned_148[tree->m_region_list_len_138++] = *regions++;
            }
            tree->m_owned_148[tree->m_region_list_len_138++] = 0;
        }
        if (region_arrays_00[1] != 0) {
            free(region_arrays_00[1]);
            region_arrays_00[1] = 0;
        }
    } else {
        node_index = tree->m_branch_count_0b4++;
        for (unsigned long child = 0; child < 8; ++child) {
            if (children_00[child] != 0) {
                tree->m_owned_09c[node_index].children_04[child] =
                    children_00[child]->ConvertToOctPreTree004AFA30(depth + 1, tree);
                delete static_cast<W8CountedOctBuildNode004AF760*>(children_00[child]);
                children_00[child] = 0;
            }
        }
        tree->m_owned_09c[node_index].region_02 = positional_28;
        tree->m_owned_09c[node_index].positional_00 = positional_2c;
    }
    return node_index;
}

/* Extend the surface build tree with the storage consumed by the destructive
   pre-tree conversion pass. */
// FUNCTION: WIZ8 0x004afda0
OctBuildPreTree::OctBuildPreTree(float leaf_size, srVector3T<float>* minimum,
                                 srVector3T<float>* maximum, unsigned short item_limit,
                                 unsigned long path_capacity, short extent_mode)
    : W8OctBuildTree00446390(leaf_size, minimum, maximum, item_limit, extent_mode)
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
    m_pulRegPaths = 0;
    region_path_count_f0 = 0;
    region_bits_f8 = 0;
    m_psrvRegCenters = 0;
    positional_124 = 0;
    positional_128 = 0;
    positional_100 = 0;
    g_pointer_65be64 = static_cast<void**>(malloc(10000 * sizeof(void*)));
    g_pointer_65be68 = static_cast<W8GDSurface**>(malloc(10000 * sizeof(W8GDSurface*)));
    mesh_particle_lookup_104 = 0;
    mesh_particles_108 = 0;
    mesh_particle_count_10c = 0;
    mesh_prop_lookup_110 = 0;
    mesh_props_114 = 0;
    mesh_prop_count_118 = 0;
    particle_count_11c = 0;
    prop_count_120 = 0;
}

/* Deduplicate and repack the shared geometry: weld-chain every flagged vertex
   to its canonical copy, drop unused vertices and degenerate polygons, rebuild
   both arrays, then re-insert every surviving polygon and continue into the
   region assignment pass. Normalizes each vertex normal on the way. */
// FUNCTION: WIZ8 0x004afea0
unsigned char OctBuildPreTree::SortGeometry004AFEA0(W8OctPreTreeGeometry* geometry)
{
    game_data_134 = geometry;
    unsigned long next_index = 1;
    unsigned long vertex = 1;
    if (1 < geometry->vertex_count_00) {
        do {
            geometry->vertices_04[vertex].normal_count_18 = 0;
            geometry = game_data_134;
            ++vertex;
        } while (vertex < geometry->vertex_count_00);
    }
    unsigned long polygon = 1;
    if (1 < geometry->polygon_count_08) {
        do {
            W8OctRegionPolygon* poly = &geometry->polygons_0c[polygon];
            if (poly->degenerate_30 == 0) {
                ++poly->vertices_34[0]->normal_count_18;
                ++poly->vertices_34[1]->normal_count_18;
                ++poly->vertices_34[2]->normal_count_18;
            }
            geometry = game_data_134;
            ++polygon;
        } while (polygon < geometry->polygon_count_08);
    }
    vertex = 1;
    if (1 < geometry->vertex_count_00) {
        do {
            W8OctPreTreeVertex* vert = &geometry->vertices_04[vertex];
            vert->normal_24.Normalize();
            if ((vert->flags_00 & 1) == 0) {
                if (vert->normal_count_18 == 0) {
                    vert->flags_00 |= 1;
                } else {
                    vert->vertex_index_04 = next_index;
                    ++next_index;
                }
            } else {
                vert->vertex_index_04 =
                    geometry->vertices_04[vert->vertex_index_04].vertex_index_04;
            }
            geometry = game_data_134;
            ++vertex;
        } while (vertex < geometry->vertex_count_00);
    }
    W8OctPreTreeVertex* new_vertices =
        static_cast<W8OctPreTreeVertex*>(malloc((next_index + 1) * sizeof(W8OctPreTreeVertex)));
    if (new_vertices == 0) {
        ReportBuildStatus00497690(7, "SortGeometry: Could not allocate pNewVerts");
        return 0;
    }
    unsigned long new_vertex_count = 1;
    W8OctPreTreeVertex* new_vertex = new_vertices + 1;
    vertex = 1;
    if (1 < geometry->vertex_count_00) {
        do {
            W8OctPreTreeVertex* vert = &geometry->vertices_04[vertex];
            if ((vert->flags_00 & 1) == 0) {
                ++new_vertex_count;
                *new_vertex = *vert;
                ++new_vertex;
            }
            geometry = game_data_134;
            ++vertex;
        } while (vertex < geometry->vertex_count_00);
    }
    polygon = 1;
    unsigned long next_polygon = 1;
    if (1 < geometry->polygon_count_08) {
        do {
            W8OctRegionPolygon* poly = &geometry->polygons_0c[polygon];
            if (poly->degenerate_30 == 0) {
                poly->ordinal_04 = next_polygon;
                poly->face_48.vertices[0] = poly->vertices_34[0]->vertex_index_04;
                poly->face_48.vertices[1] = poly->vertices_34[1]->vertex_index_04;
                poly->face_48.vertices[2] = poly->vertices_34[2]->vertex_index_04;
                poly->vertices_34[0] = new_vertices + poly->vertices_34[0]->vertex_index_04;
                poly->vertices_34[1] = new_vertices + poly->vertices_34[1]->vertex_index_04;
                ++next_polygon;
                poly->vertices_34[2] = new_vertices + poly->vertices_34[2]->vertex_index_04;
            }
            geometry = game_data_134;
            ++polygon;
        } while (polygon < geometry->polygon_count_08);
    }
    W8OctRegionPolygon* new_polygons =
        static_cast<W8OctRegionPolygon*>(malloc((next_polygon + 1) * sizeof(W8OctRegionPolygon)));
    if (new_polygons == 0) {
        ReportBuildStatus00497690(7, "SortGeometry: Could not allocate pNewPolys");
        return 0;
    }
    unsigned long new_polygon_count = 1;
    W8OctRegionPolygon* new_polygon = new_polygons + 1;
    polygon = 1;
    if (1 < geometry->polygon_count_08) {
        do {
            W8OctRegionPolygon* poly = &geometry->polygons_0c[polygon];
            if (poly->degenerate_30 == 0) {
                ++new_polygon_count;
                *new_polygon = *poly;
                ++new_polygon;
            }
            geometry = game_data_134;
            ++polygon;
        } while (polygon < geometry->polygon_count_08);
    }
    free(geometry->vertices_04);
    free(geometry->polygons_0c);
    geometry->vertex_count_00 = new_vertex_count;
    geometry->polygon_count_08 = new_polygon_count;
    geometry->vertices_04 = new_vertices;
    geometry->polygons_0c = new_polygons;
    polygon = 1;
    if (1 < geometry->polygon_count_08) {
        do {
            if (InsertSurface004B02F0(&geometry->polygons_0c[polygon], 2) == 0) {
                char message[1024];
                sprintf(message, "SortGeometry: Polygon %d cannot be inserted into tree",
                        static_cast<int>(polygon));
                ReportBuildStatus00497690(7, message);
                return 0;
            }
            ++polygon;
        } while (polygon < geometry->polygon_count_08);
    }
    return AssignPolygonRegions004B1280(geometry);
}

/* Seed a temporary root when the build tree is still empty, account for the
   inserted polygon kind, and route the region polygon through the recursive
   inserter. Unlike UpdateRegionForGeometry the root here is the plain node;
   the mode counter mirrors that function's 2/3 split. */
// FUNCTION: WIZ8 0x004b02f0
unsigned char OctBuildPreTree::InsertSurface004B02F0(W8OctRegionPolygon* polygon,
                                                     unsigned long mode)
{
    W8OctSpatialState working(&spatial_00);
    if (spatial_00.root_90 == 0) {
        working.root_90 = new W8OctBuildNode00446330;
        spatial_00.root_90 = working.root_90;
    }
    if (static_cast<short>(mode) == 2) {
        ++spatial_00.polygon_count_3c;
    } else if (static_cast<short>(mode) == 3) {
        ++spatial_00.item_count_40;
    }
    working.depth_44 = 0;
    working.level_kind_6c = 1;
    return InsertSurfaceRecursive004B03E0(&working, polygon, mode);
}

/* Descend the build octree to the leaf holding the region polygon: subdivide
   while the working depth sits above the tree's bottom level, creating child
   nodes on demand and counting them per level. At the leaf, a first-time node
   reports progress and collects its overlapping region ids; every leaf bumps
   its kind counter and appends the polygon to the mode link list. Depth 16 is
   the hard floor and returns failure. */
// FUNCTION: WIZ8 0x004b03e0
unsigned char OctBuildPreTree::InsertSurfaceRecursive004B03E0(W8OctSpatialState* working,
                                                              W8OctRegionPolygon* polygon,
                                                              unsigned long mode)
{
    W8OctSpatialState child(working);
    unsigned char inserted = 0;

    if (working->depth_44 < 0x10) {
        if (working->depth_44 < spatial_00.depth_44) {
            srVector3T<float> vertices[3];
            short octant = 0;
            for (int x = 0; x != 2; ++x) {
                for (int y = 0; y != 2; ++y) {
                    for (int z = 0; z != 2; ++z, ++octant) {
                        child.minimum_0c.x = x * child.extent_04 + working->minimum_0c.x;
                        child.maximum_18.x = child.minimum_0c.x + child.extent_04;
                        child.minimum_0c.y = y * child.extent_04 + working->minimum_0c.y;
                        child.maximum_18.y = child.minimum_0c.y + child.extent_04;
                        child.minimum_0c.z = z * child.extent_04 + working->minimum_0c.z;
                        child.maximum_18.z = child.minimum_0c.z + child.extent_04;
                        for (int corner = 0; corner != 3; ++corner) {
                            vertices[corner] = polygon->vertices_34[corner]->position_0c;
                        }
                        if (TestSpatialTriangle0046CE60(&child.minimum_0c, vertices,
                                                        reinterpret_cast<  // reinterpret-ok: plane_08's leading three floats are the unit normal
                                                            const srVector3T<float>*>(
                                                            polygon->plane_08)) != 0) {
                            W8OctBuildNode00446330* parent = working->root_90;
                            if (parent->children_00[octant] == 0) {
                                ++level_counts_c4[working->depth_44];
                                parent->children_00[octant] = new W8OctBuildNode00446330;
                            }
                            child.root_90 = parent->children_00[octant];
                            if (InsertSurfaceRecursive004B03E0(&child, polygon, mode) != 0) {
                                inserted = 1;
                            }
                        }
                    }
                }
            }
        } else {
            W8OctBuildNode00446330* node = working->root_90;
            if (node->leaf_kind_2a == 0) {
                ReportBuildStatus00497690(2, 0);
                ++positional_a8;
                W8BoundingBox leaf_bounds;
                leaf_bounds.minimum = working->minimum_0c;
                leaf_bounds.maximum = working->maximum_18;
                FindLeafRegions004B1090(node, &leaf_bounds);
            }
            ++node->leaf_kind_2a;
            if (static_cast<short>(mode) == 2) {
                ++positional_a0;
            } else if (static_cast<short>(mode) == 3) {
                ++positional_a4;
            }
            AppendLink00446D00(node, reinterpret_cast<  // reinterpret-ok: the build link lists carry region polygons here
                W8GDSurface*>(polygon), static_cast<short>(mode));
            inserted = 1;
        }
    }
    return inserted;
}

/* Seed a temporary root when the build tree is still empty, account for the
   inserted geometry kind, and route the point or box through the ordinary
   recursive region-map walk. */
// FUNCTION: WIZ8 0x004b06e0
unsigned char OctBuildPreTree::UpdateRegionForGeometry004B06E0(const srVector3T<float>* geometry,
                                                               short value, short mode)
{
    W8OctSpatialState working(&spatial_00);
    if (spatial_00.root_90 == 0) {
        working.root_90 = new W8CountedOctBuildNode004AF760;
        spatial_00.root_90 = working.root_90;
    }
    if (mode == 2) {
        ++spatial_00.polygon_count_3c;
    } else if (mode == 3) {
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
unsigned char OctBuildPreTree::UpdateRegionMap004B07E0(const W8OctSpatialState* spatial,
                                                       const srVector3T<float>* geometry,
                                                       short value, short mode)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
    /* The decompiled body reads this storage only after the same short-circuit
   chain that clang's flow analysis cannot see through; retail leaves it
   uninitialised on the failed-read path. Suppress only this diagnostic. */
    W8OctSpatialState child(spatial);
    unsigned char changed = 0;

    if (child.depth_44 >= 16) {
        return 0;
    }

    if (child.depth_44 < spatial_00.depth_44) {
        int child_index = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++child_index) {
                    child.minimum_0c.x = x * child.extent_04 + spatial->minimum_0c.x;
                    child.maximum_18.x = child.minimum_0c.x + child.extent_04;
                    child.minimum_0c.y = y * child.extent_04 + spatial->minimum_0c.y;
                    child.maximum_18.y = child.minimum_0c.y + child.extent_04;
                    child.minimum_0c.z = z * child.extent_04 + spatial->minimum_0c.z;
                    child.maximum_18.z = child.minimum_0c.z + child.extent_04;

                    unsigned char intersects;
                    if (mode == 6) {
                        intersects = PointInsideBounds0046D4D0(&child.minimum_0c, geometry);
                    } else if (mode == 5) {
                        intersects = BoundsOverlap0046D470(&child.minimum_0c, geometry);
                    }

                    W8OctBuildNode00446330* parent = spatial->root_90;
                    child.root_90 = parent->children_00[child_index];
                    if (intersects != 0 && child.root_90 != 0) {
                        W8OctBuildNode00446330* node = child.root_90;
                        unsigned short region = node->positional_28;
                        if (region != 0) {
                            if (mode == 6) {
                                positional_12c->Remove(&region, &value);
                                positional_12c->Insert(&region, &value);
                            } else if (mode == 5) {
                                positional_130->Remove(&region, &value);
                                positional_130->Insert(&region, &value);
                            }
                            changed = 1;
                        }
                        if (UpdateRegionMap004B07E0(&child, geometry, value, mode) != 0) {
                            changed = 1;
                        }
                    }
                }
            }
        }
    } else {
        W8OctBuildNode00446330* node = child.root_90;
        unsigned short region = node->positional_28;
        if (region != 0) {
            if (mode == 6) {
                positional_12c->Remove(&region, &value);
                positional_12c->Insert(&region, &value);
            } else if (mode == 5) {
                positional_130->Remove(&region, &value);
                positional_130->Insert(&region, &value);
            }
            changed = 1;
        }
    }
    return changed;
#pragma clang diagnostic pop
}

#pragma pack(push, 1)
/* One 0x6a record in the .cub region file: a dword copied to the volume's
   +0x10, an unaligned dword copied to +0x18, and the eight frustum corners
   scaled into world units. */
struct W8CubRegionRecord {
    unsigned long value_00;
    unsigned char pad_04[2];
    unsigned long value_06;
    srVector3T<float> corners_0a[8];
};
#pragma pack(pop)
static_assert(sizeof(W8CubRegionRecord) == 0x6a, "W8CubRegionRecord_must_be_0x6a");

/* Load the optional .cub region file beside the level: a version -5 header
   word precedes the region count, and each record supplies the eight frustum
   corners of one region volume, scaled by the world scale. The first point of
   each volume becomes the corner average, then the corners are sorted and the
   six frustum planes built. Answers the region bound, or zero on any read or
   allocation failure. */
// FUNCTION: WIZ8 0x004b0c90
unsigned short OctBuildPreTree::LoadRegionFile004B0C90(const char* stem, srVector3T<float>* minimum,
                                                       srVector3T<float>* maximum)
{
    spatial_00.owned_5c = 0;
    spatial_00.region_count_46 = 0;
    spatial_00.region_id_bound_58 = 0;
    char path[1024];
    sprintf(path, "%s.cub", stem);
    HANDLE file = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == 0 || file == (HANDLE)-1) {
        ReportBuildStatus00497690(6, "WARNING: Could not find and\\or open region file.\n\n");
        return 0;
    }
    ReportBuildStatus00497690(6, "Reading Region File...\n");
    DWORD read;
    int count;
    unsigned char ok = ReadFile(file, &count, 4, &read, 0) & 1;
    if (ok == 0) {
        return 0;
    }
    if (count < 0) {
        if (count != -5) {
            ReportBuildStatus00497690(7, "Wrong version for .cub file--get new plug-in!\n");
            return 0;
        }
        ok &= ReadFile(file, &count, 4, &read, 0);
        if (ok == 0) {
            return 0;
        }
    }
    if (count == 0 || 0xffff < count) {
        return 0;
    }
    spatial_00.region_id_bound_58 = static_cast<unsigned short>(count + 1);
    spatial_00.owned_5c = static_cast<W8OctRegionVolume*>(
        malloc((spatial_00.region_id_bound_58 + 1) * sizeof(W8OctRegionVolume)));
    if (spatial_00.owned_5c == 0) {
        ReportBuildStatus00497690(7, "ReadRegions: Could not allocate region list.\n");
        return 0;
    }
    memset(spatial_00.owned_5c, 0, (spatial_00.region_id_bound_58 + 1) * sizeof(W8OctRegionVolume));
    /* Computed per region but never read: the retail outside-bounds flag is
       dead state kept for fidelity. */
    bool outside = false;
    for (unsigned short region = 1; region < spatial_00.region_id_bound_58; ++region) {
        W8CubRegionRecord record;
        ok &= ReadFile(file, &record, 0x6a, &read, 0);
        if (ok == 0) {
            return 0;
        }
        W8OctRegionVolume* volume = spatial_00.owned_5c + region;
        volume->value_10 = record.value_00;
        volume->value_14 = 0;
        volume->region_04 = region;
        volume->region_bit_0c = region;
        for (int corner = 0; corner != 8; ++corner) {
            volume->points_1c[corner + 1] = record.corners_0a[corner] * g_world_scale_005ebc40;
            volume->points_1c[0] += record.corners_0a[corner] * g_world_scale_005ebc40 * 0.125f;
        }
        for (int axis = 0; axis != 3 && !outside; ++axis) {
            outside = true;
            for (int corner = 0; corner != 8; ++corner) {
                if ((&minimum->x)[axis] <= (&volume->points_1c[corner + 1].x)[axis]) {
                    outside = false;
                }
            }
        }
        for (int axis2 = 0; axis2 != 3 && !outside; ++axis2) {
            outside = true;
            for (int corner = 0; corner != 8; ++corner) {
                if ((&volume->points_1c[corner + 1].x)[axis2] <= (&maximum->x)[axis2]) {
                    outside = false;
                }
            }
        }
        volume->value_18 = record.value_06;
        SortFrustumCorners0046DA20(&volume->points_1c[1]);
        BuildFrustumPlanes0046D7E0(&volume->points_1c[1], volume->planes_88);
    }
    CloseHandle(file);
    ReportBuildStatus00497690(6, path);
    spatial_00.region_count_46 = spatial_00.region_id_bound_58;
    return spatial_00.region_id_bound_58;
}

/* Fill the leaf's region-id list with every enabled region volume overlapping
   `bounds`. The list is a malloc'd run of up to 0x32 ids terminated by a zero
   slot; the first allocation reports failure through the build log. */
// FUNCTION: WIZ8 0x004b1090
void OctBuildPreTree::FindLeafRegions004B1090(W8OctBuildNode00446330* node,
                                              const W8BoundingBox* bounds)
{
    for (unsigned short region = 1; region < spatial_00.region_count_46; ++region) {
        if ((spatial_00.owned_5c[region].positional_00 & 4) == 0 &&
            BoundsInsideFrustum0046D920(&spatial_00.owned_5c[region], bounds) != 0) {
            if (node->region_arrays_00[1] == 0) {
                unsigned short* list = static_cast<unsigned short*>(malloc(100));
                if (list == 0) {
                    ReportBuildStatus00497690(7, "Could not allocate region list.\n");
                    return;
                }
                for (int i = 0; i != 50; ++i) {
                    list[i] = 0;
                }
                node->region_arrays_00[1] = list;
            }
            unsigned short* list = node->region_arrays_00[1];
            unsigned short slot = 0;
            if (list[0] != 0) {
                do {
                    if (0x31 < slot) {
                        ReportBuildStatus00497690(7, "Too many regions in FindLeafRegions.\n");
                        return;
                    }
                    ++slot;
                } while (list[slot] != 0);
            }
            list[slot] = region;
            if (positional_ac < slot + 1) {
                positional_ac = slot + 1;
            }
            ++positional_b0;
        }
    }
}

/* Assign a polygon's region: test its representative point against each
   region volume's frustum, then fall back to the corner vertices' assigned
   regions. Multiple hits mark the polygon shared; a single assignment counts
   on the volume's polygon total. */
// FUNCTION: WIZ8 0x004b1190
void OctBuildPreTree::AssignPolygonRegion004B1190(W8OctRegionPolygon* polygon)
{
    unsigned short hits = 0;
    if (spatial_00.owned_5c == 0) {
        return;
    }
    if (polygon->region_32 != 0) {
        return;
    }
    unsigned short region = 1;
    if (1 < spatial_00.region_id_bound_58) {
        do {
            if (polygon->InsideFrustumPlanes004CFAE0(spatial_00.owned_5c[region].planes_88) != 0) {
                ++hits;
                if (polygon->region_32 == 0) {
                    polygon->region_32 = region;
                }
                polygon->flags_00 |= 8;
            }
            ++region;
        } while (region < spatial_00.region_id_bound_58);
    }
    if (hits == 0) {
        for (int corner = 0; corner != 3; ++corner) {
            W8OctPreTreeVertex* vertex = polygon->vertices_34[corner];
            short vertex_region = vertex->region_08;
            if (vertex_region != 0 || (vertex->flags_00 & 4) != 0) {
                ++hits;
                if (polygon->region_32 == 0) {
                    polygon->region_32 = vertex_region;
                }
                if ((vertex->flags_00 & 4) != 0) {
                    polygon->flags_00 |= 4;
                }
            }
        }
    }
    if (1 < hits) {
        polygon->flags_00 |= 4;
    }
    if (polygon->region_32 != 0 && (polygon->flags_00 & 4) == 0) {
        ++spatial_00.owned_5c[polygon->region_32].value_14;
    }
}

/* Region assignment pass run by SortGeometry: builds each vertex's
   polygon-reference run, assigns vertex regions from the region volumes that
   contain them, assigns polygon regions, resolves shared polygons, compacts
   emptied regions through the positional_100 remap table and finishes in
   BuildRegions. */
// FUNCTION: WIZ8 0x004b1280
unsigned char OctBuildPreTree::AssignPolygonRegions004B1280(W8OctPreTreeGeometry* geometry)
{
    ReportBuildStatus00497690(6, "Inserting polygons and vertices into regions...\n");
    geometry->max_face_count_20 = 0;
    if (1 < geometry->vertex_count_00) {
        for (unsigned long vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
            if (geometry->max_face_count_20 < geometry->vertices_04[vertex].normal_count_18) {
                geometry->max_face_count_20 = geometry->vertices_04[vertex].normal_count_18;
            }
        }
    }
    unsigned long polygon = 1;
    if (1 < geometry->polygon_count_08) {
        do {
            W8OctRegionPolygon* poly = &geometry->polygons_0c[polygon];
            for (int corner = 0; corner != 3; ++corner) {
                W8OctPreTreeVertex* vertex =
                    &geometry->vertices_04[poly->vertices_34[corner]->vertex_index_04];
                CheckArrayLength004CFB70(&vertex->face_indices_44, vertex->face_count_40, 5);
                vertex->face_indices_44[vertex->face_count_40] = polygon;
                ++vertex->face_count_40;
            }
            ++polygon;
        } while (polygon < geometry->polygon_count_08);
    }
    if (spatial_00.owned_5c != 0) {
        unsigned long vertex = 1;
        if (1 < geometry->vertex_count_00) {
            do {
                W8OctPreTreeVertex* vert = &geometry->vertices_04[vertex];
                unsigned short hits = 0;
                for (unsigned short region = 1; region < spatial_00.region_id_bound_58; ++region) {
                    if (PointInsideFrustum0046D880(&vert->position_0c,
                                                   spatial_00.owned_5c[region].planes_88) != 0) {
                        ++hits;
                        vert->region_08 = region;
                    }
                }
                if (1 < hits) {
                    vert->flags_00 |= 4;
                    vert->region_08 = 0;
                }
                ++vertex;
            } while (vertex < geometry->vertex_count_00);
        }
        polygon = 1;
        if (1 < geometry->polygon_count_08) {
            do {
                AssignPolygonRegion004B1190(&geometry->polygons_0c[polygon]);
                geometry->polygons_0c[polygon].visited_31 = 0;
                ++polygon;
            } while (polygon < geometry->polygon_count_08);
        }
        polygon = 1;
        if (1 < geometry->polygon_count_08) {
            do {
                W8OctRegionPolygon* poly = &geometry->polygons_0c[polygon];
                if ((poly->flags_00 & 4) != 0) {
                    SplitSharedPolygon004B1780(geometry, polygon);
                    poly->flags_00 &= ~0xcU;
                }
                ++polygon;
            } while (polygon < geometry->polygon_count_08);
        }
        unsigned short bound = spatial_00.region_id_bound_58;
        unsigned short region = 1;
        unsigned short next = 2;
        unsigned long slot = 1;
        if (1 < bound) {
            do {
                if (spatial_00.owned_5c[slot].value_14 == 0) {
                    if (positional_100 == 0) {
                        positional_100 = static_cast<unsigned short*>(malloc(bound * 2 + 2));
                        memset(positional_100, 0, bound * 2 + 2);
                        for (unsigned short id = 0; id < spatial_00.region_count_46; ++id) {
                            positional_100[id] = id;
                        }
                    }
                    positional_100[slot] = 0;
                    for (unsigned short id = slot + 1; id < spatial_00.region_count_46; ++id) {
                        --positional_100[id];
                    }
                    for (unsigned short i = next; i < spatial_00.region_id_bound_58; ++i) {
                        spatial_00.owned_5c[i - 1] = spatial_00.owned_5c[i];
                        spatial_00.owned_5c[i - 1].region_04 = i - 1;
                        spatial_00.owned_5c[i - 1].region_bit_0c = i - 1;
                    }
                    for (polygon = 1; polygon < geometry->polygon_count_08; ++polygon) {
                        unsigned short* poly_region = &geometry->polygons_0c[polygon].region_32;
                        if (region < *poly_region) {
                            --*poly_region;
                        }
                    }
                    for (vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
                        unsigned short* vert_region = &geometry->vertices_04[vertex].region_08;
                        if (region < *vert_region) {
                            --*vert_region;
                        }
                    }
                    --region;
                    --next;
                    --spatial_00.region_id_bound_58;
                }
                bound = spatial_00.region_id_bound_58;
                ++region;
                ++next;
                ++slot;
            } while (region < bound);
        }
    }
    BuildRegions004B19F0();
    unsigned long count = spatial_00.polygon_count_3c;
    polygon = 1;
    if (1 < count) {
        do {
            if (geometry->polygons_0c[polygon].region_32 == 0) {
                char message[256];
                sprintf(message, "Poly %d not found in correct region.\n", static_cast<int>(polygon));
                ReportBuildStatus00497690(6, message);
            }
            count = spatial_00.polygon_count_3c;
            ++polygon;
        } while (polygon < count);
    }
    return 1;
}

/* Rewrite every leaf's region list through the positional_100 remap table,
   dropping ids that remap to zero and re-terminating the list. Called with a
   null node once the table is ready: it reruns against the real root and then
   frees the table. Depth counts down against the leaf level and never walks
   past 0x10. */
// FUNCTION: WIZ8 0x004b16b0
void OctBuildPreTree::RemapNodeRegions004B16B0(W8OctBuildNode00446330* node, int depth)
{
    if (node == 0) {
        if (positional_100 != 0) {
            RemapNodeRegions004B16B0(spatial_00.root_90, 0);
            free(positional_100);
            positional_100 = 0;
        }
        return;
    }
    if (static_cast<short>(depth) < 0x10) {
        if (static_cast<short>(depth) < spatial_00.depth_44) {
            for (int child_index = 0; child_index != 8; ++child_index) {
                if (node->children_00[child_index] != 0) {
                    RemapNodeRegions004B16B0(node->children_00[child_index], depth + 1);
                }
            }
            return;
        }
        unsigned short* list = node->region_arrays_00[1];
        if (list != 0) {
            int count = 0;
            int kept = 0;
            unsigned short* read = list;
            unsigned short* write = list;
            for (; *read != 0 && count < 0x32; ++count) {
                unsigned short region = positional_100[*read];
                *write = region;
                if (region != 0) {
                    ++kept;
                    ++write;
                }
                ++read;
            }
            list[kept] = 0;
        }
    }
}

/* Resolve a shared polygon (flags bit2): histogram the neighboring polygons'
   regions collected through the corner vertices' face runs, pick the most
   frequent region the polygon actually touches, assign it and count it on
   the volume. Answers the polygon's region_32; recurses into unvisited
   neighbors so shared regions propagate. */
// FUNCTION: WIZ8 0x004b1780
unsigned short OctBuildPreTree::SplitSharedPolygon004B1780(W8OctPreTreeGeometry* geometry,
                                                           int index)
{
    W8OctRegionPolygon* polygon = &geometry->polygons_0c[index];
    if ((polygon->flags_00 & 4) == 0) {
        return polygon->region_32;
    }
    unsigned short regions[20];
    unsigned short counts[20];
    memset(regions, 0, sizeof(regions));
    polygon->visited_31 = 1;
    memset(counts, 0, sizeof(counts));
    unsigned short found = 0;
    for (int corner = 0; corner != 3; ++corner) {
        W8OctPreTreeVertex* vertex = polygon->vertices_34[corner];
        if (vertex->flag_0a == 0) {
            vertex->flag_0a = 1;
            int* face = vertex->face_indices_44;
            for (unsigned int n = vertex->face_count_40; n != 0; --n) {
                if (geometry->polygons_0c[*face].visited_31 == 0) {
                    unsigned short region = SplitSharedPolygon004B1780(geometry, *face);
                    if (region != 0) {
                        unsigned short slot = 0;
                        if (found != 0) {
                            do {
                                if (regions[slot] == region) {
                                    break;
                                }
                                ++slot;
                            } while (slot < found);
                        }
                        if (slot == found) {
                            ++counts[found];
                            regions[found] = region;
                            ++found;
                        } else {
                            ++counts[slot];
                        }
                    }
                }
                ++face;
            }
            vertex->flag_0a = 0;
        }
    }
    for (unsigned short pass = 0; pass < found; ++pass) {
        for (unsigned short slot = pass; slot < found; ++slot) {
            if (counts[slot] < counts[slot + 1]) {
                unsigned short swap = counts[slot];
                counts[slot] = counts[slot + 1];
                counts[slot + 1] = swap;
                swap = regions[slot];
                regions[slot] = regions[slot + 1];
                regions[slot + 1] = swap;
            }
        }
    }
    for (unsigned short slot = 0; slot < found; ++slot) {
        unsigned short region = regions[slot];
        unsigned char inside;
        if ((polygon->flags_00 & 8) == 0) {
            inside = 0;
            for (int corner = 0; corner < 3; ++corner) {
                if (PointInsideFrustum0046D880(&polygon->vertices_34[corner]->position_0c,
                                               spatial_00.owned_5c[region].planes_88) != 0) {
                    inside = 1;
                    break;
                }
            }
        } else {
            inside = polygon->InsideFrustumPlanes004CFAE0(spatial_00.owned_5c[region].planes_88);
        }
        if (inside != 0) {
            polygon->flags_00 &= ~0xcU;
            polygon->region_32 = region;
            ++spatial_00.owned_5c[region].value_14;
            slot = found;
        }
    }
    if ((polygon->flags_00 & 4) == 0) {
        polygon->visited_31 = 0;
    }
    return polygon->region_32;
}

/* Choose the region-tree depth that fits the requested path capacity, build
   the temporary spatial hierarchy, assign every discovered path to its build
   node, and then derive the persistent region metadata. */
// FUNCTION: WIZ8 0x004b19f0
unsigned short OctBuildPreTree::BuildRegions004B19F0()
{
    W8OctSpatialState working(&spatial_00);

    if (spatial_00.region_id_bound_58 == 0) {
        spatial_00.region_id_bound_58 = 1;
    }
    spatial_00.region_count_46 = spatial_00.region_id_bound_58;
    selected_depth_c0 = spatial_00.depth_44;
    region_path_count_f0 = 0;

    while (path_capacity_bc < level_counts_c4[selected_depth_c0]) {
        --selected_depth_c0;
    }

    spatial_00.leaf_level_52 = 0;
    int last_level = selected_depth_c0 - 1;
    float extent = spatial_00.extent_04;
    while (static_cast<int>(spatial_00.leaf_level_52) < last_level) {
        float next_extent = extent * g_float_005ebc7c;
        if (fabs(spatial_00.region_grid_cell_54 - extent) <
            fabs(spatial_00.region_grid_cell_54 - next_extent)) {
            break;
        }
        ++spatial_00.leaf_level_52;
        extent = next_extent;
    }

    unsigned long level_count = level_counts_c4[spatial_00.leaf_level_52];
    m_pulRegPaths = static_cast<unsigned long*>(malloc(level_count * sizeof(unsigned long) + 8));
    if (m_pulRegPaths == 0) {
        srAssertFail("m_pulRegPaths", OCT_BUILD_PRE_TREE_CPP, 0x6f1, 0);
    }

    m_psrvRegCenters = static_cast<srVector3T<float>*>(srHeap.allocate(
        (level_count + 2 + spatial_00.region_id_bound_58) * sizeof(srVector3T<float>)));
    if (m_psrvRegCenters == 0) {
        srAssertFail("m_psrvRegCenters", OCT_BUILD_PRE_TREE_CPP, 0x6f3, 0);
    }

    spatial_00.region_grid_cell_54 = working.cell_size_08;
    positional_124 = new W8HashTable<unsigned short, unsigned long>;
    AssignInitialRegions004B1D90(&working);
    positional_128 = new W8HashTable<unsigned int, short>;
    region_bits_f8 = new BitArray(spatial_00.region_id_bound_58);

    for (unsigned short path_index = 0; path_index < region_path_count_f0; ++path_index) {
        unsigned long path = m_pulRegPaths[path_index];
        W8OctBuildNode00446330* node = FindNode004B23F0(path);
        if (node->leaf_kind_2a != 0 && node->leaf_kind_2a < 25) {
            MergeAdjacentRegion004B2450(node, path);
        }
    }

    for (unsigned long polygon = 1; polygon < game_data_134->polygon_count_08; ++polygon) {
        if (game_data_134->polygons_0c[polygon].region_32 == 0) {
            char message[252];
            sprintf(message, "Poly %d not found in ANY region.\n", (int)polygon);
            ReportBuildStatus00497690(6, message);
        }
    }

    FinalizeRegionMapping004B2A20();
    AssignRegionFromSurfaces004B3050(&working);
    ValidatePolygonRegions004B3330();

    srHeap.free(m_psrvRegCenters);
    m_psrvRegCenters = 0;
    if (region_bits_f8 != 0) {
        delete region_bits_f8;
    }
    region_bits_f8 = 0;

    if (spatial_00.region_count_46 == 1) {
        spatial_00.region_count_46 = 0;
    }
    return spatial_00.region_id_bound_58;
}

/* Assign a new region to every selected-depth cell containing an unclaimed
   polygon.  The packed path and center are retained for the later adjacency
   merge, while the farthest referenced vertex establishes the region radius
   used by that merge. */
// FUNCTION: WIZ8 0x004b1d90
void OctBuildPreTree::AssignInitialRegions004B1D90(const W8OctSpatialState* spatial)
{
    W8OctSpatialState child(spatial);
    if (spatial->depth_44 >= 16) {
        return;
    }

    if (spatial->depth_44 == spatial_00.leaf_level_52) {
        g_value_65be58 = 0;
        W8OctBuildNode00446330* node = spatial->root_90;
        node->CollectLinkedSurfaces004AF8F0(spatial_00.leaf_level_52, spatial_00.depth_44, 2);

        unsigned long unique_count = 0;
        for (unsigned long source = 0; source < g_value_65be58; ++source) {
            bool found = false;
            for (unsigned long existing = 0; existing < source && !found; ++existing) {
                if (g_pointer_65be64[source] == g_pointer_65be64[existing]) {
                    found = true;
                }
            }
            if (!found) {
                g_pointer_65be64[unique_count++] = g_pointer_65be64[source];
            }
        }
        g_value_65be58 = unique_count;

        int contained_count = 0;
        for (unsigned long index = 0; index < g_value_65be58; ++index) {
            W8OctRegionPolygon* polygon = static_cast<W8OctRegionPolygon*>(g_pointer_65be64[index]);
            if (polygon->region_32 == 0 &&
                polygon->ContainsPoint004CFB30(&spatial->minimum_0c) != 0) {
                ++contained_count;
                polygon->region_32 = spatial_00.region_id_bound_58;
            }
        }

        if (contained_count != 0) {
            srVector3T<float>& center = m_psrvRegCenters[spatial_00.region_id_bound_58];
            center = (spatial->maximum_18 + spatial->minimum_0c) * g_float_005ebc7c;

            m_pulRegPaths[region_path_count_f0++] = spatial->positional_94;

            for (unsigned long index = 0; index < g_value_65be58; ++index) {
                W8OctRegionPolygon* polygon =
                    static_cast<W8OctRegionPolygon*>(g_pointer_65be64[index]);
                if (polygon->region_32 == spatial_00.region_id_bound_58) {
                    for (int vertex_index = 0; vertex_index != 3; ++vertex_index) {
                        const srVector3T<float>& position =
                            polygon->vertices_34[vertex_index]->position_0c;
                        float distance = (center - position).Length();
                        if (spatial_00.max_region_radius_60 < distance) {
                            spatial_00.max_region_radius_60 = distance;
                        }
                    }
                }
            }

            positional_124->Insert(&spatial_00.region_id_bound_58, &spatial->positional_94);
            node->positional_28 = spatial_00.region_id_bound_58;
            node->positional_2c = node->positional_28;
            ++spatial_00.region_id_bound_58;
            node->leaf_kind_2a = contained_count;
        }
        return;
    }

    unsigned long path = spatial->positional_94;
    int high = ((int)(signed char)(path >> 23) & ~1) + 1;
    int x_base = (int)(signed char)(path >> 15) & ~1;
    int y_base = (int)(signed char)(path >> 7) & ~1;
    signed char z_base = (signed char)((signed char)path << 1);
    short child_index = 0;
    for (int x = 0; x != 2; ++x) {
        for (int y = 0; y != 2; ++y) {
            for (int z = 0; z != 2; ++z, ++child_index) {
                W8OctBuildNode00446330* parent = spatial->root_90;
                W8OctBuildNode00446330* node = parent->children_00[child_index];
                if (node != 0) {
                    child.minimum_0c.x = x * child.extent_04 + spatial->minimum_0c.x;
                    child.maximum_18.x = child.minimum_0c.x + child.extent_04;
                    child.minimum_0c.y = y * child.extent_04 + spatial->minimum_0c.y;
                    child.maximum_18.y = child.minimum_0c.y + child.extent_04;
                    child.minimum_0c.z = z * child.extent_04 + spatial->minimum_0c.z;
                    child.maximum_18.z = child.minimum_0c.z + child.extent_04;
                    child.positional_94 =
                        ((high * 0x100 + x_base + x) * 0x100 + y_base + y) * 0x100 + z_base + z;
                    child.root_90 = node;
                    AssignInitialRegions004B1D90(&child);
                }
            }
        }
    }
}

/* Follow the active depth bits in a packed octree path. Each active bit
   contributes one x/y/z child selector, from the highest level down. */
// FUNCTION: WIZ8 0x004b23f0
W8OctBuildNode00446330* OctBuildPreTree::FindNode004B23F0(unsigned int path)
{
    W8OctBuildNode00446330* node = spatial_00.root_90;
    unsigned int active_levels = path >> 24;
    unsigned char x = (unsigned char)(path >> 16);
    unsigned char y = (unsigned char)(path >> 8);
    unsigned int z = path & 0xff;
    for (unsigned int level = 0x80; node != 0 && level != 0; level >>= 1) {
        if ((active_levels & level) != 0) {
            int child = 0;
            if ((x & level) != 0) {
                child = 4;
            }
            if ((y & level) != 0) {
                child += 2;
            }
            if ((z & level) != 0) {
                child += 1;
            }
            node = node->children_00[child];
        }
    }
    return node;
}

/* Try the negative and positive neighbor along each path axis, stopping after
   the first region that can be merged. The high byte remains the depth mask;
   the other three bytes are the x/y/z cell coordinates. */
// FUNCTION: WIZ8 0x004b2450
unsigned char OctBuildPreTree::MergeAdjacentRegion004B2450(W8OctBuildNode00446330* node,
                                                           unsigned int path)
{
    int cell[4];
    cell[0] = path & 0xff000000;
    cell[1] = (path >> 16) & 0xff;
    cell[2] = (path >> 8) & 0xff;
    cell[3] = path & 0xff;

    if (cell[1] != 0) {
        --cell[1];
        if (MergeRegion004B25C0(node, cell) != 0) {
            return 1;
        }
        ++cell[1];
    }
    ++cell[1];
    if ((cell[1] & (1 << (spatial_00.leaf_level_52 + 1))) == 0 &&
        MergeRegion004B25C0(node, cell) != 0) {
        return 1;
    }
    --cell[1];

    if (cell[2] != 0) {
        --cell[2];
        if (MergeRegion004B25C0(node, cell) != 0) {
            return 1;
        }
        ++cell[2];
    }
    ++cell[2];
    if ((cell[2] & (1 << (spatial_00.leaf_level_52 + 1))) == 0 &&
        MergeRegion004B25C0(node, cell) != 0) {
        return 1;
    }
    --cell[2];

    if (cell[3] != 0) {
        --cell[3];
        if (MergeRegion004B25C0(node, cell) != 0) {
            return 1;
        }
        ++cell[3];
    }
    ++cell[3];
    if ((cell[3] & (1 << (spatial_00.leaf_level_52 + 1))) == 0 &&
        MergeRegion004B25C0(node, cell) != 0) {
        return 1;
    }
    return 0;
}

/* Merge one neighboring region into the region carried by the adjacent build
   node. Every packed path is re-keyed in the region table and the surviving
   center becomes the population-weighted average of both groups. */
// FUNCTION: WIZ8 0x004b25c0
unsigned char OctBuildPreTree::MergeRegion004B25C0(W8OctBuildNode00446330* node, const int* cell)
{
    unsigned long neighbor_path = ((cell[1] * 0x100 + cell[2]) * 0x100 + cell[3]) + cell[0];
    W8OctBuildNode00446330* neighbor = FindNode004B23F0(neighbor_path);
    if (neighbor == 0 || neighbor->leaf_kind_2a == 0 || neighbor->leaf_kind_2a >= 100) {
        return 0;
    }

    unsigned short neighbor_region = neighbor->positional_28;
    unsigned short node_region = node->positional_28;
    if (neighbor_region == node_region) {
        return 0;
    }

    srVector3T<float>& node_center = m_psrvRegCenters[node_region];
    srVector3T<float>& neighbor_center = m_psrvRegCenters[neighbor_region];
    float distance = (node_center - neighbor_center).Length();
    if (!(distance < spatial_00.region_grid_cell_54 * g_float_005ec52c)) {
        return 0;
    }

    unsigned short combined_count = node->leaf_kind_2a + neighbor->leaf_kind_2a;
    region_bits_f8->Set(neighbor_region);

    unsigned long neighbor_count = 0;
    int position = -1;
    while ((position = positional_124->FindNextEntry(&neighbor_region, position)) != -1) {
        ++neighbor_count;
        W8OctBuildNode00446330* member = FindNode004B23F0(positional_124->entries[position].value);
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
        (node_center * (double)moved_count + neighbor_center * (double)neighbor_count) /
        (double)(moved_count + neighbor_count);
    return 1;
}

/* Compact provisional submesh ids into the final region range, rewrite every
   polygon and path-table entry through that map, and derive aggregate bounds
   for the validation pass. */
// FUNCTION: WIZ8 0x004b2a20
void OctBuildPreTree::FinalizeRegionMapping004B2A20()
{
    unsigned short* region_map = static_cast<unsigned short*>(
        malloc(spatial_00.region_id_bound_58 * sizeof(unsigned short)));
    memset(region_map, 0, spatial_00.region_id_bound_58 * sizeof(unsigned short));

    unsigned short next_region = spatial_00.region_count_46;
    for (unsigned short mapping_index = 0; mapping_index < region_path_count_f0; ++mapping_index) {
        W8OctBuildNode00446330* node = FindNode004B23F0(m_pulRegPaths[mapping_index]);
        if (node->leaf_kind_2a != 0) {
            unsigned short provisional = node->positional_2c;
            if (node->positional_28 == provisional) {
                region_map[provisional] = next_region++;
            } else {
                region_map[provisional] = node->positional_28;
            }
        }
    }

    unsigned short final_region_count = next_region;
    W8BoundingBox* region_bounds =
        static_cast<W8BoundingBox*>(malloc(final_region_count * sizeof(W8BoundingBox)));
    for (unsigned short region = 0; region < final_region_count; ++region) {
        region_bounds[region].minimum.x = 1000000.0f;
        region_bounds[region].minimum.y = 1000000.0f;
        region_bounds[region].minimum.z = 1000000.0f;
        region_bounds[region].maximum.x = -1000000.0f;
        region_bounds[region].maximum.y = -1000000.0f;
        region_bounds[region].maximum.z = -1000000.0f;
    }

    for (unsigned long polygon_index = 1; polygon_index < game_data_134->polygon_count_08;
         ++polygon_index) {
        W8OctRegionPolygon& polygon = game_data_134->polygons_0c[polygon_index];
        unsigned short region = polygon.region_32;
        if (region >= spatial_00.region_count_46) {
            if (positional_124->Lookup(&region) != 0) {
                polygon.region_32 = region_map[region];
            } else {
                polygon.region_32 = region_map[region_map[region]];
            }
        }

        region = polygon.region_32;
        if (region >= final_region_count) {
            char message[256];
            sprintf(message, "Polygon %d in invalid submesh %d\n", (int)polygon_index,
                    (unsigned int)region);
            ReportBuildStatus00497690(6, message);
        }

        for (int axis = 0; axis != 3; ++axis) {
            for (int vertex = 0; vertex != 3; ++vertex) {
                float value = (&polygon.vertices_34[vertex]->position_0c.x)[axis];
                if (value < (&region_bounds[region].minimum.x)[axis]) {
                    (&region_bounds[region].minimum.x)[axis] = value;
                }
                if ((&region_bounds[region].maximum.x)[axis] < value) {
                    (&region_bounds[region].maximum.x)[axis] = value;
                }
            }
        }
    }

    for (unsigned short path_index = 0; path_index < region_path_count_f0; ++path_index) {
        unsigned long path = m_pulRegPaths[path_index];
        W8OctBuildNode00446330* node = FindNode004B23F0(path);
        unsigned short old_region = node->positional_28;
        positional_124->Remove(&old_region, &path);
        node->positional_28 = region_map[old_region];
        positional_124->Insert(&node->positional_28, &path);
        if (node->positional_28 >= final_region_count) {
            char message[256];
            sprintf(message, "Invalid submesh %d\n", (unsigned int)node->positional_28);
            ReportBuildStatus00497690(6, message);
        }
    }

    spatial_00.submesh_count_74 = final_region_count;
    ValidatePolygonRegions004B3330();
    ValidateRegionBounds004B35B0(region_bounds);
    free(region_bounds);
    free(region_map);
}

/* Descend to the selected region depth, collect the surfaces below each
   unassigned node, and choose the most frequent nonzero surface region. */
// FUNCTION: WIZ8 0x004b3050
void OctBuildPreTree::AssignRegionFromSurfaces004B3050(const W8OctSpatialState* spatial)
{
    W8OctSpatialState child(spatial);
    if (spatial->depth_44 > 15) {
        return;
    }

    if (spatial->depth_44 != spatial_00.leaf_level_52) {
        short child_index = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++child_index) {
                    W8OctBuildNode00446330* parent = spatial->root_90;
                    W8OctBuildNode00446330* node = parent->children_00[child_index];
                    if (node != 0) {
                        child.minimum_0c.x = x * child.extent_04 + spatial->minimum_0c.x;
                        child.maximum_18.x = child.minimum_0c.x + child.extent_04;
                        child.minimum_0c.y = y * child.extent_04 + spatial->minimum_0c.y;
                        child.maximum_18.y = child.minimum_0c.y + child.extent_04;
                        child.minimum_0c.z = z * child.extent_04 + spatial->minimum_0c.z;
                        child.maximum_18.z = child.minimum_0c.z + child.extent_04;
                        child.root_90 = node;
                        AssignRegionFromSurfaces004B3050(&child);
                    }
                }
            }
        }
        return;
    }

    W8OctBuildNode00446330* node = spatial->root_90;
    if (node->positional_28 != 0) {
        return;
    }

    g_value_65be58 = 0;
    node->CollectLinkedSurfaces004AF8F0(spatial_00.leaf_level_52, spatial_00.depth_44, 2);

    unsigned long unique_count = 0;
    for (unsigned long source = 0; source < g_value_65be58; ++source) {
        bool present = false;
        for (unsigned long previous = 0; previous < source && !present; ++previous) {
            if (g_pointer_65be64[source] == g_pointer_65be64[previous]) {
                present = true;
            }
        }
        if (!present) {
            g_pointer_65be64[unique_count++] = g_pointer_65be64[source];
        }
    }

    unsigned short regions[20] = {0};
    unsigned short counts[20] = {0};
    unsigned short selected_region = 0;
    unsigned short selected_count = 0;
    for (unsigned long index = 0; index < unique_count; ++index) {
        unsigned short region =
            static_cast<W8OctRegionPolygon*>(g_pointer_65be64[index])->region_32;
        short slot = 0;
        while (regions[slot] != 0 && regions[slot] != region) {
            ++slot;
        }
        if (regions[slot] == 0) {
            regions[slot] = region;
        }
        ++counts[slot];
        if (selected_count < counts[slot]) {
            selected_region = regions[slot];
            selected_count = counts[slot];
        }
    }

    g_value_65be58 = unique_count;
    node->positional_28 = selected_region;
    node->leaf_kind_2a = selected_count;
}

/* Verify that each polygon carrying a generated region id resolves back to a
   build node with that id. A coordinate just below a grid plane is checked in
   both the truncated cell and its negative neighbor. */
// FUNCTION: WIZ8 0x004b3330
void OctBuildPreTree::ValidatePolygonRegions004B3330()
{
    unsigned int active_levels = 0;
    for (unsigned int depth = 0; depth < spatial_00.leaf_level_52; ++depth) {
        active_levels |= 1 << (depth + 24);
    }

    for (unsigned long polygon_index = 1; polygon_index < game_data_134->polygon_count_08;
         ++polygon_index) {
        W8OctRegionPolygon& polygon = game_data_134->polygons_0c[polygon_index];
        if (polygon.region_32 < spatial_00.region_count_46) {
            continue;
        }

        float relative_x = polygon.position_18.x - spatial_00.minimum_0c.x;
        int x = static_cast<int>(relative_x / spatial_00.region_grid_cell_54);
        short x_count = 1;
        if (g_float_005ed034 < x * spatial_00.region_grid_cell_54 - relative_x) {
            --x;
            x_count = 2;
        }

        float relative_y = polygon.position_18.y - spatial_00.minimum_0c.y;
        int y = static_cast<int>(relative_y / spatial_00.region_grid_cell_54);
        short y_count = 1;
        if (g_float_005ed034 < y * spatial_00.region_grid_cell_54 - relative_y) {
            --y;
            y_count = 2;
        }

        float relative_z = polygon.position_18.z - spatial_00.minimum_0c.z;
        int z = static_cast<int>(relative_z / spatial_00.region_grid_cell_54);
        short z_count = 1;
        if (g_float_005ed034 < z * spatial_00.region_grid_cell_54 - relative_z) {
            --z;
            z_count = 2;
        }

        bool found = false;
        for (int x_offset = 0; x_offset < x_count; ++x_offset) {
            for (int y_offset = 0; y_offset < y_count; ++y_offset) {
                for (int z_offset = 0; z_offset < z_count; ++z_offset) {
                    unsigned long path =
                        active_levels +
                        (((x + x_offset) * 0x100 + (y + y_offset)) * 0x100 + (z + z_offset));
                    W8OctBuildNode00446330* node = FindNode004B23F0(path);
                    if (node->positional_28 == polygon.region_32) {
                        found = true;
                    }
                }
            }
        }

        if (!found) {
            char message[256];
            sprintf(message, "Poly %d not found in correct region.\n", (int)polygon_index);
            ReportBuildStatus00497690(6, message);
        }
    }
}

/* Check every path assigned to a region against both the build-node region id
   and the aggregate automesh bounds produced for that region. */
// FUNCTION: WIZ8 0x004b35b0
void OctBuildPreTree::ValidateRegionBounds004B35B0(const W8BoundingBox* region_bounds)
{
    for (unsigned long region_index = 1; region_index < spatial_00.submesh_count_74;
         ++region_index) {
        unsigned short region = (unsigned short)region_index;
        int entry = -1;
        while ((entry = positional_124->FindNextEntry(&region, entry)) != -1) {
            unsigned long path = positional_124->entries[entry].value;
            srVector3T<float> minimum;
            minimum.x = static_cast<float>((path >> 16) & 0xff) * spatial_00.region_grid_cell_54 +
                        spatial_00.minimum_0c.x;
            minimum.y = static_cast<float>((path >> 8) & 0xff) * spatial_00.region_grid_cell_54 +
                        spatial_00.minimum_0c.y;
            minimum.z = static_cast<float>(path & 0xff) * spatial_00.region_grid_cell_54 +
                        spatial_00.minimum_0c.z;
            srVector3T<float> maximum;
            maximum.x = minimum.x + spatial_00.region_grid_cell_54;
            maximum.y = minimum.y + spatial_00.region_grid_cell_54;
            maximum.z = minimum.z + spatial_00.region_grid_cell_54;

            W8OctBuildNode00446330* node = FindNode004B23F0(path);
            if (node != 0) {
                if (node->positional_28 != region) {
                    ReportBuildStatus00497690(7, "Region has wrong automesh.");
                }
                const W8BoundingBox& bounds = region_bounds[region_index];
                if (maximum.x < bounds.minimum.x || bounds.maximum.x < minimum.x ||
                    maximum.y < bounds.minimum.y || bounds.maximum.y < minimum.y ||
                    maximum.z < bounds.minimum.z || bounds.maximum.z < minimum.z) {
                    ReportBuildStatus00497690(
                        7, "AutoMesh has no vertices inside region. You probably "
                           "have an old .cub file!");
                }
            }
        }
    }
}

/* Associate every non-cloud particle with the regions containing its origin
   or any corner of its serialized bounds. Particles still outside the built
   regions lead the compact list, followed by one null-terminated list per
   region; the lookup table stores each region's offset into that list. */
// FUNCTION: WIZ8 0x004b3820
unsigned char
OctBuildPreTree::BuildParticleRegions004B3820(const W8LevelFileParticleSystem* particles,
                                              int particle_count)
{
    particle_count_11c = particle_count;
    positional_12c = new W8HashTable<unsigned short, short>;

    short region_particles[4998];
    region_particles[0] = 0;
    unsigned short list_count = 0;
    int particle_number = 0;

    for (int particle_index = 0; particle_index < particle_count; ++particle_index) {
        const W8LevelParticleRecord004BD0D0& particle = particles[particle_index].particle_01;
        char name[64];
        strcpy(name, particle.name);
        _strupr(name);
        if (strncmp(name, "CLOUD", 5) == 0) {
            continue;
        }

        ++particle_number;
        short particle_value = (short)particle_number;
        srVector3T<float> position;
        position = particle.location * g_world_scale_005ebc40;

        bool mapped = false;
        for (unsigned short region_index = 1; region_index < spatial_00.region_count_46;
             ++region_index) {
            W8OctRegionVolume* volume = spatial_00.owned_5c + region_index;
            if (volume->ContainsPoint0049E460(&position) != 0) {
                unsigned short region = volume->region_04;
                bool present = false;
                int entry = -1;
                while ((entry = positional_12c->FindNextEntry(&region, entry)) != -1) {
                    if (positional_12c->entries[entry].value == particle_value) {
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

        if (UpdateRegionForGeometry004B06E0(&position, particle_value, 6) == 0) {
            srVector3T<float> extent;
            bool has_bounds = false;
            if (particle.bounds_mode == 1) {
                extent = particle.bounds_extent * g_startup_near_limit_005ec000;
                has_bounds = true;
            } else if (particle.bounds_mode == 2 && particle.bounds_radius > g_float_005ebb34) {
                extent = particle.bounds_origin * g_world_scale_005ebc40;
                has_bounds = true;
            }

            if (has_bounds) {
                srVector3T<float> minimum;
                srVector3T<float> maximum;
                minimum = position - extent;
                maximum = position + extent;

                for (int x = 0; x != 2; ++x) {
                    for (int y = 0; y != 2; ++y) {
                        for (int z = 0; z != 2; ++z) {
                            srVector3T<float> corner;
                            corner.x = x == 0 ? minimum.x : maximum.x;
                            corner.y = y == 0 ? minimum.y : maximum.y;
                            corner.z = z == 0 ? minimum.z : maximum.z;

                            bool corner_mapped = false;
                            for (unsigned short region_index = 1;
                                 region_index < spatial_00.region_count_46; ++region_index) {
                                W8OctRegionVolume* volume = spatial_00.owned_5c + region_index;
                                if (volume->ContainsPoint0049E460(&corner) != 0) {
                                    unsigned short region = volume->region_04;
                                    bool present = false;
                                    int entry = -1;
                                    while ((entry = positional_12c->FindNextEntry(&region,
                                                                                  entry)) != -1) {
                                        if (positional_12c->entries[entry].value ==
                                            particle_value) {
                                            present = true;
                                            break;
                                        }
                                    }
                                    if (!present) {
                                        positional_12c->Insert(&region, &particle_value);
                                    }
                                    corner_mapped = true;
                                    mapped = true;
                                }
                            }
                            if (!corner_mapped &&
                                UpdateRegionForGeometry004B06E0(&corner, particle_value, 6) != 0) {
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
    } else {
        list_count = 1;
    }

    unsigned short region_count = static_cast<unsigned short>(spatial_00.submesh_count_74);
    mesh_particle_lookup_104 =
        static_cast<unsigned short*>(malloc(region_count * sizeof(unsigned short)));
    memset(mesh_particle_lookup_104, 0, region_count * sizeof(unsigned short));

    for (unsigned short region = 1; region < region_count; ++region) {
        int entry = -1;
        bool first = true;
        while ((entry = positional_12c->FindNextEntry(&region, entry)) != -1) {
            if (first) {
                mesh_particle_lookup_104[region] = list_count;
                first = false;
            }
            region_particles[list_count++] = positional_12c->entries[entry].value;
        }
        if (!first) {
            region_particles[list_count++] = 0;
        }
    }

    mesh_particles_108 = static_cast<unsigned short*>(malloc(list_count * sizeof(unsigned short)));
    if (mesh_particles_108 != 0) {
        memcpy(mesh_particles_108, region_particles, list_count * sizeof(unsigned short));
    }
    mesh_particle_count_10c = list_count;
    delete positional_12c;
    return 1;
}

/* Accumulate the two preprocessing geometry banks into one region-to-prop
   table. The first call owns the shared scratch list; the final call appends
   the second bank, emits the compact lookup/list pair, and releases scratch. */
// FUNCTION: WIZ8 0x004b3f90
unsigned char OctBuildPreTree::BuildGeometryRegions004B3F90(const W8LevelFileProp* records,
                                                            int record_count, int base_index,
                                                            unsigned char finalize)
{
    if (finalize == 0) {
        positional_130 = new W8HashTable<unsigned short, short>;
        g_pointer_65be5c = static_cast<unsigned short*>(malloc(10000));
        g_pointer_65be5c[0] = 0;
        g_value_65be6c = 0;
    }

    for (int record_index = 0; record_index < record_count; ++record_index) {
        const W8LevelFileProp& record = records[record_index];
        short value = (short)(base_index + 1 + record_index);
        bool mapped = false;

        for (unsigned short region_index = 1; region_index < spatial_00.region_count_46;
             ++region_index) {
            W8OctRegionVolume* volume = spatial_00.owned_5c + region_index;
            for (unsigned char bounds_index = 0; bounds_index < record.anim_obj_53.num_bound_box_47;
                 ++bounds_index) {
                const srVector3T<float>* bounds =
                    &record.anim_obj_53.pBoundBox[bounds_index].minimum_00;
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
                                while ((entry = positional_130->FindNextEntry(&region, entry)) !=
                                       -1) {
                                    if (positional_130->entries[entry].value == value) {
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
        for (unsigned char bounds_index = 0; bounds_index < record.anim_obj_53.num_bound_box_47;
             ++bounds_index) {
            const srVector3T<float>* bounds =
                &record.anim_obj_53.pBoundBox[bounds_index].minimum_00;
            for (int endpoint = 0; endpoint != 2; ++endpoint) {
                srVector3T<float> point;
                point = bounds[endpoint] * g_world_scale_005ebc40;
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

        if (UpdateRegionForGeometry004B06E0(aggregate, value, 5) == 0 && !mapped) {
            g_pointer_65be5c[g_value_65be6c++] = value;
        }
    }

    if (finalize != 0) {
        if (g_value_65be6c == 0) {
            g_value_65be6c = 1;
        } else {
            g_pointer_65be5c[g_value_65be6c++] = 0;
        }

        unsigned long region_count = spatial_00.submesh_count_74;
        mesh_prop_lookup_110 =
            static_cast<unsigned short*>(malloc(region_count * sizeof(unsigned short)));
        for (unsigned long region_value = 1; region_value < region_count; ++region_value) {
            unsigned short region = (unsigned short)region_value;
            mesh_prop_lookup_110[region_value] = 0;
            int entry = -1;
            bool first = true;
            while ((entry = positional_130->FindNextEntry(&region, entry)) != -1) {
                if (first) {
                    mesh_prop_lookup_110[region_value] = g_value_65be6c;
                    first = false;
                }
                g_pointer_65be5c[g_value_65be6c++] = positional_130->entries[entry].value;
            }
            if (!first) {
                g_pointer_65be5c[g_value_65be6c++] = 0;
            }
        }

        mesh_props_114 =
            static_cast<unsigned short*>(malloc(g_value_65be6c * sizeof(unsigned short)));
        if (mesh_props_114 != 0) {
            memcpy(mesh_props_114, g_pointer_65be5c, g_value_65be6c * sizeof(unsigned short));
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
OctPreTree* OctBuildPreTree::BuildOctPreTree004B4640()
{
    OctPreTree* tree = new OctPreTree;
    if (tree == 0) {
        return 0;
    }

    tree->m_owned_09c = static_cast<W8OctPreTreeBranch*>(
        malloc((g_value_65be60 * 9 + 0x12) * sizeof(unsigned long)));
    if (tree->m_owned_09c == 0) {
        return 0;
    }
    memset(tree->m_owned_09c, 0, (g_value_65be60 * 9 + 0x12) * sizeof(unsigned long));

    tree->m_owned_0a0 =
        static_cast<W8OctPreTreeLeaf*>(malloc((positional_a8 + 2) * sizeof(W8OctPreTreeLeaf)));
    if (tree->m_owned_0a0 == 0) {
        return 0;
    }
    memset(tree->m_owned_0a0, 0, (positional_a8 + 2) * sizeof(W8OctPreTreeLeaf));

    tree->m_owned_0d0 =
        static_cast<unsigned long*>(malloc(positional_a0 * 2 * sizeof(unsigned long)));
    if (tree->m_owned_0d0 == 0) {
        return 0;
    }
    memset(tree->m_owned_0d0, 0, positional_a0 * 2 * sizeof(unsigned long));

    tree->m_owned_148 = static_cast<unsigned short*>(malloc(positional_a8 * 0x50));
    if (tree->m_owned_148 == 0) {
        return 0;
    }
    memset(tree->m_owned_148, 0, positional_a8 * 0x50);

    tree->m_owned_12c =
        static_cast<unsigned long*>(malloc(positional_a4 * 2 * sizeof(unsigned long)));
    if (tree->m_owned_12c == 0) {
        return 0;
    }
    memset(tree->m_owned_12c, 0, positional_a4 * 2 * sizeof(unsigned long));

    tree->spatial_000.region_count_46 = spatial_00.region_count_46;
    tree->spatial_000.leaf_level_52 = spatial_00.leaf_level_52;
    tree->spatial_000.submesh_count_74 = spatial_00.submesh_count_74;
    tree->spatial_000.region_id_bound_58 = spatial_00.region_id_bound_58;

    tree->m_pusMeshParticleLookup = mesh_particle_lookup_104;
    mesh_particle_lookup_104 = 0;
    tree->m_pusMeshParticles = mesh_particles_108;
    mesh_particles_108 = 0;
    tree->m_usMeshParticlesLen_0e8 = mesh_particle_count_10c;
    tree->m_pusMeshPropLookup = mesh_prop_lookup_110;
    mesh_prop_lookup_110 = 0;
    tree->m_pusMeshProps = mesh_props_114;
    tree->m_usMeshPropsLen_0f4 = mesh_prop_count_118;
    mesh_props_114 = 0;
    tree->m_ulNumParticles = particle_count_11c;
    tree->m_ulNumProps = prop_count_120;

    tree->spatial_000.depth_44 = spatial_00.depth_44;
    tree->spatial_000.node_extent_70 = spatial_00.node_extent_70;
    tree->spatial_000.max_region_radius_60 = spatial_00.max_region_radius_60 < g_float_005ed038
                                                 ? spatial_00.max_region_radius_60
                                                 : g_float_005ed038;
    while (selected_depth_c0 < tree->spatial_000.depth_44) {
        ReportBuildStatus00497690(6, "Collapsing tree by one level.");
        --tree->spatial_000.depth_44;
        tree->spatial_000.node_extent_70 += tree->spatial_000.node_extent_70;
    }

    tree->spatial_000.extent_04 = spatial_00.extent_04;
    tree->spatial_000.cell_size_08 = spatial_00.cell_size_08;
    tree->spatial_000.owned_5c = spatial_00.owned_5c;
    tree->game_data_3a4 = game_data_134;
    tree->positional_3a8 = positional_138;
    tree->positional_3ac = positional_13c;
    tree->positional_3b0 = positional_b8;
    tree->spatial_000.item_count_40 = spatial_00.item_count_40;
    tree->spatial_000.polygon_count_3c = spatial_00.polygon_count_3c;
    tree->m_owned_190 = new BitArray(spatial_00.polygon_count_3c);
    tree->spatial_000.region_grid_cell_54 = spatial_00.region_grid_cell_54;
    tree->spatial_000.region_cells_per_axis_50 = spatial_00.region_cells_per_axis_50;
    tree->automesh_cells_29c = positional_124;
    positional_124 = 0;

    for (int axis = 0; axis != 3; ++axis) {
        (&tree->spatial_000.minimum_0c.x)[axis] = (&spatial_00.minimum_0c.x)[axis];
        (&tree->spatial_000.maximum_18.x)[axis] = (&spatial_00.maximum_18.x)[axis];
        (&tree->spatial_000.clipped_minimum_24.x)[axis] = (&spatial_00.clipped_minimum_24.x)[axis];
        (&tree->spatial_000.clipped_maximum_30.x)[axis] = (&spatial_00.clipped_maximum_30.x)[axis];
    }

    tree->m_branch_count_0b4 = 1;
    tree->m_leaf_count_0b8 = 1;
    tree->polygon_cursor_3a0 = 1;
    tree->m_region_list_len_138 = 1;
    tree->m_gd_surface_stream_len_124 = 1;
    tree->m_positional_13c = 0;
    tree->m_trigger_count_128 = 0;

    W8OctBuildNode00446330* root = spatial_00.root_90;
    root->ConvertToOctPreTree004AFA30(0, tree);
    delete root;
    spatial_00.root_90 = 0;
    if (tree->m_region_list_len_138 == 1) {
        tree->m_region_list_len_138 = 0;
    }

    for (int grid_axis = 0; grid_axis != 3; ++grid_axis) {
        (&tree->m_leaf_grid_dim_x_0a4)[grid_axis] =
            (int)(((&tree->spatial_000.clipped_maximum_30.x)[grid_axis] -
                   (&tree->spatial_000.clipped_minimum_24.x)[grid_axis]) /
                  tree->spatial_000.node_extent_70) +
            1;
    }
    tree->m_owned_0b0 = static_cast<unsigned long*>(
        malloc(tree->m_leaf_grid_dim_z_0ac * tree->m_leaf_grid_dim_x_0a4 *
               tree->m_leaf_grid_dim_y_0a8 * sizeof(unsigned long)));
    unsigned long cell_index = 0;
    int point[3];
    for (point[0] = 0; point[0] < static_cast<int>(tree->m_leaf_grid_dim_x_0a4); ++point[0]) {
        for (point[1] = 0; point[1] < static_cast<int>(tree->m_leaf_grid_dim_y_0a8); ++point[1]) {
            for (point[2] = 0; point[2] < static_cast<int>(tree->m_leaf_grid_dim_z_0ac);
                 ++point[2]) {
                tree->m_owned_0b0[cell_index] = tree->FindLeaf00433660(point);
                if (tree->m_leaf_count_0b8 < tree->m_owned_0b0[cell_index]) {
                    tree->m_owned_0b0[cell_index] = 0;
                }
                ++cell_index;
            }
        }
    }
    tree->spatial_000.leaf_grid_stride_y_68 = tree->m_leaf_grid_dim_z_0ac;
    tree->spatial_000.leaf_grid_stride_x_64 =
        tree->m_leaf_grid_dim_z_0ac * tree->m_leaf_grid_dim_y_0a8;
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
