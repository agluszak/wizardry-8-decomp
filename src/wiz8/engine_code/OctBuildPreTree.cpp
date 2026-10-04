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
int g_build_node_instances;
// GLOBAL: WIZ8 0x0065be58
unsigned long g_poly_list_count;
// GLOBAL: WIZ8 0x0065be64
void** g_poly_list;
// GLOBAL: WIZ8 0x0065be68
W8GDSurface** g_gd_surface_list;
// GLOBAL: WIZ8 0x0065be5c
unsigned short* g_region_id_list;
// GLOBAL: WIZ8 0x0065be6c
unsigned short g_region_id_count;

// GLOBAL: WIZ8 0x005ed034
const float g_float_005ed034 = -0.009999999776482582f;
// GLOBAL: WIZ8 0x005ed038
const float g_float_005ed038 = 4000.0f;
// GLOBAL: WIZ8 0x005ec52c
const float g_float_005ec52c = 3.0f;

#define OCT_BUILD_PRE_TREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildPreTree.cpp"

/* This zero-storage node form is constructed by OctBuildTree when its
   pre-tree ownership mode is active.  Its immediately following conversion
   methods and the first assertion-backed boundary at 0x004B19F0 establish
   Engine Code\OctBuildPreTree.cpp as the owning cluster. */
// FUNCTION: WIZ8 0x004af760
W8CountedOctBuildNode::W8CountedOctBuildNode()
{
    ++g_build_node_instances;
}

// FUNCTION: WIZ8 0x004af780
W8CountedOctBuildNode::~W8CountedOctBuildNode()
{
    if (children_00[1] != 0) {
        free(children_00[1]);
    }
    if (g_build_node_instances != 0) {
        --g_build_node_instances;
    }
}

/* Replace the two polygon-link chains used by pre-tree leaves with compact,
   null-terminated surface arrays. */
// FUNCTION: WIZ8 0x004af7b0
unsigned char W8OctBuildNode::RearrangeNodePolys(short current_depth, short target_depth)
{
    if (current_depth == target_depth) {
        for (short mode = 2; mode < 4; ++mode) {
            if (leaf_kind != 0 && links_00[mode] != 0) {
                g_poly_list_count = 0;
                CollectLinkedSurfaces(current_depth, target_depth, mode);
                void** surfaces =
                    static_cast<void**>(malloc(g_poly_list_count * sizeof(void*) + sizeof(void*)));
                if (surfaces == 0) {
                    ReportBuildStatus(7, "RearrangeNodePolys: Could not allocate poly list.\n");
                    return 0;
                }
                unsigned long index;
                for (index = 0; index < g_poly_list_count; ++index) {
                    surfaces[index] = g_poly_list[index];
                }
                surfaces[index] = 0;
                surface_arrays[mode] = surfaces;
            }
        }
    } else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                children_00[child]->RearrangeNodePolys(current_depth + 1, target_depth);
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004af8f0
int W8OctBuildNode::CollectLinkedSurfaces(short current_depth, short target_depth, short mode)
{
    int count = 0;
    if (current_depth == target_depth) {
        if (1 < mode && mode < 4 && links_00[mode] != 0) {
            W8OctBuildLink* link = links_00[mode];
            do {
                g_poly_list[g_poly_list_count++] = link->surface_00;
                if (10000 < g_poly_list_count) {
                    ReportBuildStatus(7, "OctBuildPreTree::m_ppPolyList too long.");
                    return 0;
                }
                link = link->next_04;
                ++count;
            } while (link != 0);
        }
    } else {
        for (int child = 0; child != 8; ++child) {
            if (children_00[child] != 0) {
                count += children_00[child]->CollectLinkedSurfaces(current_depth + 1, target_depth,
                                                                   mode);
            }
        }
    }
    return count;
}

// FUNCTION: WIZ8 0x004af9b0
int W8OctBuildNode::CollectSurfaceArray(short mode)
{
    if (leaf_kind != 0 && provisional_region_2c == 0) {
        void** surfaces = surface_arrays[mode];
        if (surfaces != 0) {
            while (*surfaces != 0) {
                g_poly_list[g_poly_list_count++] = *surfaces++;
            }
        }
        return g_poly_list_count;
    }
    for (int child = 0; child != 8; ++child) {
        if (children_00[child] != 0) {
            children_00[child]->CollectSurfaceArray(mode);
        }
    }
    return g_poly_list_count;
}

/* Consume the counted build nodes into the compact arrays owned by the
   runtime pre-tree.  Leaf polygon pointers are deduplicated before their
   persistent surface indices are appended; branch children are converted and
   destroyed as soon as their compact indices have been recorded. */
// FUNCTION: WIZ8 0x004afa30
unsigned long W8OctBuildNode::ConvertToOctPreTree(unsigned short depth, OctPreTree* tree)
{
    unsigned long node_index;

    if (depth == tree->m_spatial.m_depth) {
        node_index = tree->m_leaf_count++;

        g_poly_list_count = 0;
        CollectSurfaceArray(2);
        if (g_poly_list_count != 0) {
            unsigned long unique_count = 0;
            for (unsigned long source = 0; source < g_poly_list_count; ++source) {
                bool found = false;
                for (unsigned long existing = 0; existing < source && !found; ++existing) {
                    if (g_poly_list[source] == g_poly_list[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_poly_list[unique_count++] = g_poly_list[source];
                }
            }
            g_poly_list_count = unique_count;
            tree->m_polygon_index_stream[tree->polygon_cursor] = unique_count;
            tree->m_leaves[node_index].polygon_offset = tree->polygon_cursor;
            ++tree->polygon_cursor;
            for (unsigned long surface = 0; surface < g_poly_list_count; ++surface) {
                tree->m_polygon_index_stream[tree->polygon_cursor++] =
                    static_cast<W8OctRegionPolygon*>(g_poly_list[surface])->ordinal_04;
            }
            free(surface_arrays[2]);
            surface_arrays[2] = 0;
        }

        g_poly_list_count = 0;
        CollectSurfaceArray(3);
        if (g_poly_list_count != 0) {
            unsigned long unique_count = 0;
            for (unsigned long source = 0; source < g_poly_list_count; ++source) {
                bool found = false;
                for (unsigned long existing = 0; existing < source && !found; ++existing) {
                    if (g_poly_list[source] == g_poly_list[existing]) {
                        found = true;
                    }
                }
                if (!found) {
                    g_poly_list[unique_count++] = g_poly_list[source];
                }
            }
            g_poly_list_count = unique_count;
            tree->m_gd_surface_index_stream[tree->m_gd_surface_stream_len] = unique_count;
            tree->m_leaves[node_index].gd_polygon_offset = tree->m_gd_surface_stream_len;
            ++tree->m_gd_surface_stream_len;
            for (unsigned long surface = 0; surface < g_poly_list_count; ++surface) {
                tree->m_gd_surface_index_stream[tree->m_gd_surface_stream_len++] =
                    static_cast<W8GDSurface*>(g_poly_list[surface])->index_04;
            }
            free(surface_arrays[3]);
            surface_arrays[3] = 0;
        }

        unsigned short* regions = region_arrays[1];
        if (regions != 0 && *regions != 0) {
            tree->m_leaves[node_index].region_offset_04 = tree->m_region_list_len;
            while (*regions != 0) {
                tree->m_region_index_stream[tree->m_region_list_len++] = *regions++;
            }
            tree->m_region_index_stream[tree->m_region_list_len++] = 0;
        }
        if (region_arrays[1] != 0) {
            free(region_arrays[1]);
            region_arrays[1] = 0;
        }
    } else {
        node_index = tree->m_branch_count++;
        for (unsigned long child = 0; child < 8; ++child) {
            if (children_00[child] != 0) {
                tree->m_branches[node_index].children_04[child] =
                    children_00[child]->ConvertToOctPreTree(depth + 1, tree);
                delete static_cast<W8CountedOctBuildNode*>(children_00[child]);
                children_00[child] = 0;
            }
        }
        tree->m_branches[node_index].region_02 = region_28;
        tree->m_branches[node_index].provisional_region_00 = provisional_region_2c;
    }
    return node_index;
}

/* Extend the surface build tree with the storage consumed by the destructive
   pre-tree conversion pass. */
// FUNCTION: WIZ8 0x004afda0
OctBuildPreTree::OctBuildPreTree(float leaf_size, srVector3T<float>* minimum,
                                 srVector3T<float>* maximum, unsigned short item_limit,
                                 unsigned long path_capacity, short extent_mode)
    : W8OctBuildTree(leaf_size, minimum, maximum, item_limit, extent_mode)
{
    mesh_linking = 1;
    use_owned_nodes = 1;
    game_data_134 = 0;
    unknown_138 = 0;
    unknown_13c = 0;
    deepest_link_list_b8 = 0;
    memset(level_counts, 0, sizeof(level_counts));
    path_capacity_bc = path_capacity;
    selected_depth = 0;
    m_pulRegPaths = 0;
    region_path_count = 0;
    region_bits = 0;
    m_psrvRegCenters = 0;
    region_path_map = 0;
    positional_128 = 0;
    region_remap = 0;
    g_poly_list = static_cast<void**>(malloc(10000 * sizeof(void*)));
    g_gd_surface_list = static_cast<W8GDSurface**>(malloc(10000 * sizeof(W8GDSurface*)));
    mesh_particle_lookup = 0;
    mesh_particles = 0;
    mesh_particle_count = 0;
    mesh_prop_lookup = 0;
    mesh_props = 0;
    mesh_prop_count = 0;
    particle_count_11c = 0;
}

/* Deduplicate and repack the shared geometry: weld-chain every flagged vertex
   to its canonical copy, drop unused vertices and degenerate polygons, rebuild
   both arrays, then re-insert every surviving polygon and continue into the
   region assignment pass. Normalizes each vertex normal on the way. */
// FUNCTION: WIZ8 0x004afea0
unsigned char OctBuildPreTree::SortGeometry(W8OctPreTreeGeometry* geometry)
{
    game_data_134 = geometry;
    unsigned long next_index = 1;
    unsigned long vertex = 1;
    if (1 < geometry->vertex_count_00) {
        do {
            geometry->m_vertices[vertex].m_normal_count = 0;
            geometry = game_data_134;
            ++vertex;
        } while (vertex < geometry->vertex_count_00);
    }
    unsigned long polygon = 1;
    if (1 < geometry->m_polygon_count) {
        do {
            W8OctRegionPolygon* poly = &geometry->m_polygons[polygon];
            if (poly->degenerate == 0) {
                ++poly->vertices_34[0]->m_normal_count;
                ++poly->vertices_34[1]->m_normal_count;
                ++poly->vertices_34[2]->m_normal_count;
            }
            geometry = game_data_134;
            ++polygon;
        } while (polygon < geometry->m_polygon_count);
    }
    for (vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
        W8OctPreTreeVertex* vert = &geometry->m_vertices[vertex];
        vert->m_normal.Normalize();
        if ((vert->flags_00 & 1) == 0) {
            if (vert->m_normal_count == 0) {
                vert->flags_00 |= 1;
            } else {
                vert->m_vertex_index = next_index;
                ++next_index;
            }
        } else {
            vert->m_vertex_index = geometry->m_vertices[vert->m_vertex_index].m_vertex_index;
        }
        geometry = game_data_134;
    }
    W8OctPreTreeVertex* new_vertices =
        static_cast<W8OctPreTreeVertex*>(malloc((next_index + 1) * sizeof(W8OctPreTreeVertex)));
    if (new_vertices == 0) {
        ReportBuildStatus(7, "SortGeometry: Could not allocate pNewVerts");
        return 0;
    }
    unsigned long new_vertex_count = 1;
    W8OctPreTreeVertex* new_vertex = new_vertices + 1;
    for (vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
        W8OctPreTreeVertex* vert = &geometry->m_vertices[vertex];
        if ((vert->flags_00 & 1) == 0) {
            ++new_vertex_count;
            *new_vertex = *vert;
            ++new_vertex;
        }
        geometry = game_data_134;
    }
    polygon = 1;
    unsigned long next_polygon = 1;
    if (1 < geometry->m_polygon_count) {
        do {
            W8OctRegionPolygon* poly = &geometry->m_polygons[polygon];
            if (poly->degenerate == 0) {
                poly->ordinal_04 = next_polygon;
                poly->face_48.vertices[0] = poly->vertices_34[0]->m_vertex_index;
                poly->face_48.vertices[1] = poly->vertices_34[1]->m_vertex_index;
                poly->face_48.vertices[2] = poly->vertices_34[2]->m_vertex_index;
                poly->vertices_34[0] = new_vertices + poly->vertices_34[0]->m_vertex_index;
                poly->vertices_34[1] = new_vertices + poly->vertices_34[1]->m_vertex_index;
                ++next_polygon;
                poly->vertices_34[2] = new_vertices + poly->vertices_34[2]->m_vertex_index;
            }
            geometry = game_data_134;
            ++polygon;
        } while (polygon < geometry->m_polygon_count);
    }
    W8OctRegionPolygon* new_polygons =
        static_cast<W8OctRegionPolygon*>(malloc((next_polygon + 1) * sizeof(W8OctRegionPolygon)));
    if (new_polygons == 0) {
        ReportBuildStatus(7, "SortGeometry: Could not allocate pNewPolys");
        return 0;
    }
    unsigned long new_polygon_count = 1;
    W8OctRegionPolygon* new_polygon = new_polygons + 1;
    for (polygon = 1; polygon < geometry->m_polygon_count; ++polygon) {
        W8OctRegionPolygon* poly = &geometry->m_polygons[polygon];
        if (poly->degenerate == 0) {
            ++new_polygon_count;
            *new_polygon = *poly;
            ++new_polygon;
        }
        geometry = game_data_134;
    }
    free(geometry->m_vertices);
    free(geometry->m_polygons);
    geometry->vertex_count_00 = new_vertex_count;
    geometry->m_polygon_count = new_polygon_count;
    geometry->m_vertices = new_vertices;
    geometry->m_polygons = new_polygons;
    for (polygon = 1; polygon < geometry->m_polygon_count; ++polygon) {
        if (InsertSurface(&geometry->m_polygons[polygon], 2) == 0) {
            char message[1024];
            sprintf(message, "SortGeometry: Polygon %d cannot be inserted into tree",
                    static_cast<int>(polygon));
            ReportBuildStatus(7, message);
            return 0;
        }
    }
    return AssignPolygonRegions(geometry);
}

/* Seed a temporary root when the build tree is still empty, account for the
   inserted polygon kind, and route the region polygon through the recursive
   inserter. Unlike UpdateRegionForGeometry the root here is the plain node;
   the mode counter mirrors that function's 2/3 split. */
// FUNCTION: WIZ8 0x004b02f0
unsigned char OctBuildPreTree::InsertSurface(W8OctRegionPolygon* polygon, unsigned long mode)
{
    W8OctSpatialState working(&spatial_00);
    if (spatial_00.m_root == 0) {
        working.m_root = new W8OctBuildNode;
        spatial_00.m_root = working.m_root;
    }
    if (static_cast<short>(mode) == 2) {
        ++spatial_00.m_polygon_count;
    } else if (static_cast<short>(mode) == 3) {
        ++spatial_00.m_item_count;
    }
    working.m_depth = 0;
    working.m_level_kind = 1;
    return InsertSurfaceRecursive(&working, polygon, mode);
}

/* Descend the build octree to the leaf holding the region polygon: subdivide
   while the working depth sits above the tree's bottom level, creating child
   nodes on demand and counting them per level. At the leaf, a first-time node
   reports progress and collects its overlapping region ids; every leaf bumps
   its kind counter and appends the polygon to the mode link list. Depth 16 is
   the hard floor and returns failure. */
// FUNCTION: WIZ8 0x004b03e0
unsigned char OctBuildPreTree::InsertSurfaceRecursive(W8OctSpatialState* working,
                                                      W8OctRegionPolygon* polygon,
                                                      unsigned long mode)
{
    W8OctSpatialState child(working);
    bool inserted = 0;

    if (working->m_depth < 0x10) {
        if (working->m_depth < spatial_00.m_depth) {
            srVector3T<float> vertices[3];
            short octant = 0;
            for (int x = 0; x != 2; ++x) {
                for (int y = 0; y != 2; ++y) {
                    for (int z = 0; z != 2; ++z, ++octant) {
                        child.m_minimum.x = x * child.m_extent + working->m_minimum.x;
                        child.m_maximum.x = child.m_minimum.x + child.m_extent;
                        child.m_minimum.y = y * child.m_extent + working->m_minimum.y;
                        child.m_maximum.y = child.m_minimum.y + child.m_extent;
                        child.m_minimum.z = z * child.m_extent + working->m_minimum.z;
                        child.m_maximum.z = child.m_minimum.z + child.m_extent;
                        for (int corner = 0; corner != 3; ++corner) {
                            vertices[corner] = polygon->vertices_34[corner]->position_0c;
                        }
                        if (TestSpatialTriangle(&child.m_minimum, vertices,
                                                &polygon->plane_08.normal) != 0) {
                            W8OctBuildNode* parent = working->m_root;
                            if (parent->children_00[octant] == 0) {
                                ++level_counts[working->m_depth];
                                parent->children_00[octant] = new W8OctBuildNode;
                            }
                            child.m_root = parent->children_00[octant];
                            if (InsertSurfaceRecursive(&child, polygon, mode) != 0) {
                                inserted = 1;
                            }
                        }
                    }
                }
            }
        } else {
            W8OctBuildNode* node = working->m_root;
            if (node->leaf_kind == 0) {
                ReportBuildStatus(2, 0);
                ++leaf_count_a8;
                W8BoundingBox leaf_bounds;
                leaf_bounds.minimum = working->m_minimum;
                leaf_bounds.maximum = working->m_maximum;
                FindLeafRegions(node, &leaf_bounds);
            }
            ++node->leaf_kind;
            if (static_cast<short>(mode) == 2) {
                ++leaf_polygon_count;
            } else if (static_cast<short>(mode) == 3) {
                ++gd_surface_count;
            }
            AppendLink(node, polygon, static_cast<short>(mode));
            inserted = 1;
        }
    }
    return inserted;
}

/* Seed a temporary root when the build tree is still empty, account for the
   inserted geometry kind, and route the point or box through the ordinary
   recursive region-map walk. */
// FUNCTION: WIZ8 0x004b06e0
unsigned char OctBuildPreTree::UpdateRegionForGeometry(const srVector3T<float>* geometry,
                                                       short value, short mode)
{
    W8OctSpatialState working(&spatial_00);
    if (spatial_00.m_root == 0) {
        working.m_root = new W8CountedOctBuildNode;
        spatial_00.m_root = working.m_root;
    }
    if (mode == 2) {
        ++spatial_00.m_polygon_count;
    } else if (mode == 3) {
        ++spatial_00.m_item_count;
    }
    working.m_depth = 0;
    working.m_level_kind = 1;
    return UpdateRegionMap(&working, geometry, value, mode);
}

/* Walk the build tree against either a point or an axis-aligned volume and
   refresh the selected region-to-short association for every intersected
   leaf. The temporary child record carries the exact subcell bounds into the
   recursive call. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                                   \
    "-Wsometimes-uninitialized" // uninit-ok: retail tests the unset intersects byte for modes other than 5/6; that value controls region-map updates.
#pragma clang diagnostic ignored                                                                   \
    "-Wuninitialized" // uninit-ok: retail tests the unset intersects byte for modes other than 5/6; that value controls region-map updates.
// FUNCTION: WIZ8 0x004b07e0
unsigned char OctBuildPreTree::UpdateRegionMap(const W8OctSpatialState* spatial,
                                               const srVector3T<float>* geometry, short value,
                                               short mode)
{
    W8OctSpatialState child(spatial);
    bool changed = false;

    if (child.m_depth >= 16) {
        return 0;
    }

    if (child.m_depth < spatial_00.m_depth) {
        int child_index = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++child_index) {
                    child.m_minimum.x = x * child.m_extent + spatial->m_minimum.x;
                    child.m_maximum.x = child.m_minimum.x + child.m_extent;
                    child.m_minimum.y = y * child.m_extent + spatial->m_minimum.y;
                    child.m_maximum.y = child.m_minimum.y + child.m_extent;
                    child.m_minimum.z = z * child.m_extent + spatial->m_minimum.z;
                    child.m_maximum.z = child.m_minimum.z + child.m_extent;

                    /* Retail leaves this unset for modes outside 5/6. */
                    bool intersects;
                    if (mode == 6) {
                        intersects = PointInsideBoxBounds(&child.m_minimum, geometry);
                    } else if (mode == 5) {
                        intersects = BoundsOverlapStrict(&child.m_minimum, geometry);
                    }

                    W8OctBuildNode* parent = spatial->m_root;
                    child.m_root = parent->children_00[child_index];
                    if (intersects != 0 && child.m_root != 0) {
                        W8OctBuildNode* node = child.m_root;
                        unsigned short region = node->region_28;
                        if (region != 0) {
                            if (mode == 6) {
                                inside_region_map->Remove(&region, &value);
                                inside_region_map->Insert(&region, &value);
                            } else if (mode == 5) {
                                overlap_region_map->Remove(&region, &value);
                                overlap_region_map->Insert(&region, &value);
                            }
                            changed = 1;
                        }
                        if (UpdateRegionMap(&child, geometry, value, mode) != 0) {
                            changed = 1;
                        }
                    }
                }
            }
        }
    } else {
        W8OctBuildNode* node = child.m_root;
        unsigned short region = node->region_28;
        if (region != 0) {
            if (mode == 6) {
                inside_region_map->Remove(&region, &value);
                inside_region_map->Insert(&region, &value);
            } else if (mode == 5) {
                overlap_region_map->Remove(&region, &value);
                overlap_region_map->Insert(&region, &value);
            }
            changed = 1;
        }
    }
    return changed;
}
#pragma clang diagnostic pop

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
unsigned short OctBuildPreTree::LoadRegionFile(const char* stem, srVector3T<float>* minimum,
                                               srVector3T<float>* maximum)
{
    spatial_00.m_region_volumes = 0;
    spatial_00.m_region_count = 0;
    spatial_00.m_region_id_bound = 0;
    char path[1024];
    sprintf(path, "%s.cub", stem);
    HANDLE file = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == 0 || file == (HANDLE)-1) {
        ReportBuildStatus(6, "\nWARNING: Could not find and\\or open region file.\n\n");
        return 0;
    }
    ReportBuildStatus(6, "\nReading Region File...\n");
    DWORD read;
    int count;
    unsigned char ok = ReadFile(file, &count, 4, &read, 0) & 1;
    if (ok == 0) {
        return 0;
    }
    if (count < 0) {
        if (count != -5) {
            ReportBuildStatus(7, "Wrong version for .cub file--get new plug-in!\n");
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
    spatial_00.m_region_id_bound = static_cast<unsigned short>(count + 1);
    spatial_00.m_region_volumes = static_cast<W8OctRegionVolume*>(
        malloc((spatial_00.m_region_id_bound + 1) * sizeof(W8OctRegionVolume)));
    if (spatial_00.m_region_volumes == 0) {
        ReportBuildStatus(7, "ReadRegions: Could not allocate region list.\n");
        return 0;
    }
    memset(spatial_00.m_region_volumes, 0,
           (spatial_00.m_region_id_bound + 1) * sizeof(W8OctRegionVolume));
    /* Computed per region but never read: the retail outside-bounds flag is
       dead state kept for fidelity. */
    bool outside = false;
    for (unsigned short region = 1; region < spatial_00.m_region_id_bound; ++region) {
        W8CubRegionRecord record;
        ok &= ReadFile(file, &record, 0x6a, &read, 0);
        if (ok == 0) {
            return 0;
        }
        W8OctRegionVolume* volume = spatial_00.m_region_volumes + region;
        volume->value_10 = record.value_00;
        volume->m_polygon_count = 0;
        volume->m_region = region;
        volume->m_region_bit = region;
        for (int corner = 0; corner != 8; ++corner) {
            volume->m_points[corner + 1] = record.corners_0a[corner] * g_world_scale;
            volume->m_points[0] += record.corners_0a[corner] * g_world_scale * 0.125f;
        }
        for (int axis = 0; axis != 3 && !outside; ++axis) {
            outside = true;
            for (int corner = 0; corner != 8; ++corner) {
                if ((&minimum->x)[axis] <= (&volume->m_points[corner + 1].x)[axis]) {
                    outside = false;
                }
            }
        }
        for (int axis2 = 0; axis2 != 3 && !outside; ++axis2) {
            outside = true;
            for (int corner = 0; corner != 8; ++corner) {
                if ((&volume->m_points[corner + 1].x)[axis2] <= (&maximum->x)[axis2]) {
                    outside = false;
                }
            }
        }
        volume->value_18 = record.value_06;
        SortFrustumCorners(&volume->m_points[1]);
        BuildFrustumPlanes(&volume->m_points[1], volume->m_planes);
    }
    CloseHandle(file);
    ReportBuildStatus(6, path);
    spatial_00.m_region_count = spatial_00.m_region_id_bound;
    return spatial_00.m_region_id_bound;
}

/* Fill the leaf's region-id list with every enabled region volume overlapping
   `bounds`. The list is a malloc'd run of up to 0x32 ids terminated by a zero
   slot; the first allocation reports failure through the build log. */
// FUNCTION: WIZ8 0x004b1090
void OctBuildPreTree::FindLeafRegions(W8OctBuildNode* node, const W8BoundingBox* bounds)
{
    for (unsigned short region = 1; region < spatial_00.m_region_count; ++region) {
        if ((spatial_00.m_region_volumes[region].flags_00 & 4) == 0 &&
            BoundsInsideFrustum(&spatial_00.m_region_volumes[region], bounds) != 0) {
            if (node->region_arrays[1] == 0) {
                unsigned short* list = static_cast<unsigned short*>(malloc(100));
                if (list == 0) {
                    ReportBuildStatus(7, "Could not allocate region list in FindLeafRegions.\n");
                    return;
                }
                for (int i = 0; i != 50; ++i) {
                    list[i] = 0;
                }
                node->region_arrays[1] = list;
            }
            unsigned short* list = node->region_arrays[1];
            unsigned short slot = 0;
            if (list[0] != 0) {
                do {
                    if (0x31 < slot) {
                        ReportBuildStatus(7, "Too many regions in FindLeafRegions.\n");
                        return;
                    }
                    ++slot;
                } while (list[slot] != 0);
            }
            list[slot] = region;
            if (max_leaf_regions_ac < slot + 1) {
                max_leaf_regions_ac = slot + 1;
            }
            ++region_assignments;
        }
    }
}

/* Assign a polygon's region: test its representative point against each
   region volume's frustum, then fall back to the corner vertices' assigned
   regions. Multiple hits mark the polygon shared; a single assignment counts
   on the volume's polygon total. */
// FUNCTION: WIZ8 0x004b1190
void OctBuildPreTree::AssignPolygonRegion(W8OctRegionPolygon* polygon)
{
    unsigned short hits = 0;
    if (spatial_00.m_region_volumes == 0) {
        return;
    }
    if (polygon->region_32 != 0) {
        return;
    }
    unsigned short region = 1;
    if (1 < spatial_00.m_region_id_bound) {
        do {
            if (polygon->InsideFrustumPlanes(spatial_00.m_region_volumes[region].m_planes) != 0) {
                ++hits;
                if (polygon->region_32 == 0) {
                    polygon->region_32 = region;
                }
                polygon->flags_00 |= 8;
            }
            ++region;
        } while (region < spatial_00.m_region_id_bound);
    }
    if (hits == 0) {
        for (int corner = 0; corner != 3; ++corner) {
            W8OctPreTreeVertex* vertex = polygon->vertices_34[corner];
            short vertex_region = vertex->m_region;
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
        ++spatial_00.m_region_volumes[polygon->region_32].m_polygon_count;
    }
}

/* Region assignment pass run by SortGeometry: builds each vertex's
   polygon-reference run, assigns vertex regions from the region volumes that
   contain them, assigns polygon regions, resolves shared polygons, compacts
   emptied regions through the region_remap remap table and finishes in
   BuildRegions. */
// FUNCTION: WIZ8 0x004b1280
unsigned char OctBuildPreTree::AssignPolygonRegions(W8OctPreTreeGeometry* geometry)
{
    ReportBuildStatus(6, "Inserting polygons and vertices into regions...\n");
    geometry->m_max_face_count = 0;
    if (1 < geometry->vertex_count_00) {
        for (unsigned long vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
            if (geometry->m_max_face_count < geometry->m_vertices[vertex].m_normal_count) {
                geometry->m_max_face_count = geometry->m_vertices[vertex].m_normal_count;
            }
        }
    }
    unsigned long polygon = 1;
    if (1 < geometry->m_polygon_count) {
        do {
            W8OctRegionPolygon* poly = &geometry->m_polygons[polygon];
            for (int corner = 0; corner != 3; ++corner) {
                W8OctPreTreeVertex* vertex =
                    &geometry->m_vertices[poly->vertices_34[corner]->m_vertex_index];
                geometry->CheckArrayLength(&vertex->face_indices, vertex->face_count_40, 5);
                vertex->face_indices[vertex->face_count_40] = polygon;
                ++vertex->face_count_40;
            }
            ++polygon;
        } while (polygon < geometry->m_polygon_count);
    }
    if (spatial_00.m_region_volumes != 0) {
        unsigned long vertex = 1;
        if (1 < geometry->vertex_count_00) {
            do {
                W8OctPreTreeVertex* vert = &geometry->m_vertices[vertex];
                unsigned short hits = 0;
                for (unsigned short region = 1; region < spatial_00.m_region_id_bound; ++region) {
                    if (PointInsideFrustum(&vert->position_0c,
                                           spatial_00.m_region_volumes[region].m_planes) != 0) {
                        ++hits;
                        vert->m_region = region;
                    }
                }
                if (1 < hits) {
                    vert->flags_00 |= 4;
                    vert->m_region = 0;
                }
                ++vertex;
            } while (vertex < geometry->vertex_count_00);
        }
        for (polygon = 1; polygon < geometry->m_polygon_count; ++polygon) {
            AssignPolygonRegion(&geometry->m_polygons[polygon]);
            geometry->m_polygons[polygon].visited_31 = 0;
        }
        for (polygon = 1; polygon < geometry->m_polygon_count; ++polygon) {
            W8OctRegionPolygon* poly = &geometry->m_polygons[polygon];
            if ((poly->flags_00 & 4) != 0) {
                SplitSharedPolygon(geometry, polygon);
                poly->flags_00 &= ~0xcU;
            }
        }
        unsigned short bound = spatial_00.m_region_id_bound;
        unsigned short region = 1;
        unsigned short next = 2;
        unsigned long slot = 1;
        if (1 < bound) {
            do {
                if (spatial_00.m_region_volumes[slot].m_polygon_count == 0) {
                    if (region_remap == 0) {
                        region_remap = static_cast<unsigned short*>(malloc(bound * 2 + 2));
                        memset(region_remap, 0, bound * 2 + 2);
                        for (unsigned short id = 0; id < spatial_00.m_region_count; ++id) {
                            region_remap[id] = id;
                        }
                    }
                    region_remap[slot] = 0;
                    for (unsigned short id = slot + 1; id < spatial_00.m_region_count; ++id) {
                        --region_remap[id];
                    }
                    for (unsigned short i = next; i < spatial_00.m_region_id_bound; ++i) {
                        spatial_00.m_region_volumes[i - 1] = spatial_00.m_region_volumes[i];
                        spatial_00.m_region_volumes[i - 1].m_region = i - 1;
                        spatial_00.m_region_volumes[i - 1].m_region_bit = i - 1;
                    }
                    for (polygon = 1; polygon < geometry->m_polygon_count; ++polygon) {
                        unsigned short* poly_region = &geometry->m_polygons[polygon].region_32;
                        if (region < *poly_region) {
                            --*poly_region;
                        }
                    }
                    for (vertex = 1; vertex < geometry->vertex_count_00; ++vertex) {
                        unsigned short* vert_region = &geometry->m_vertices[vertex].m_region;
                        if (region < *vert_region) {
                            --*vert_region;
                        }
                    }
                    --region;
                    --next;
                    --spatial_00.m_region_id_bound;
                }
                bound = spatial_00.m_region_id_bound;
                ++region;
                ++next;
                ++slot;
            } while (region < bound);
        }
    }
    BuildRegions();
    unsigned long count = spatial_00.m_polygon_count;
    for (polygon = 1; polygon < count; ++polygon) {
        if (geometry->m_polygons[polygon].region_32 == 0) {
            char message[256];
            sprintf(message, "Poly %d not found in ANY region.\n", static_cast<int>(polygon));
            ReportBuildStatus(6, message);
        }
        count = spatial_00.m_polygon_count;
    }
    return 1;
}

/* Rewrite every leaf's region list through the region_remap remap table,
   dropping ids that remap to zero and re-terminating the list. Called with a
   null node once the table is ready: it reruns against the real root and then
   frees the table. Depth counts down against the leaf level and never walks
   past 0x10. */
// FUNCTION: WIZ8 0x004b16b0
void OctBuildPreTree::RemapNodeRegions(W8OctBuildNode* node, int depth)
{
    if (node == 0) {
        if (region_remap != 0) {
            RemapNodeRegions(spatial_00.m_root, 0);
            free(region_remap);
            region_remap = 0;
        }
        return;
    }
    if (static_cast<short>(depth) < 0x10) {
        if (static_cast<short>(depth) < spatial_00.m_depth) {
            for (int child_index = 0; child_index != 8; ++child_index) {
                if (node->children_00[child_index] != 0) {
                    RemapNodeRegions(node->children_00[child_index], depth + 1);
                }
            }
            return;
        }
        unsigned short* list = node->region_arrays[1];
        if (list != 0) {
            int count = 0;
            int kept = 0;
            unsigned short* read = list;
            unsigned short* write = list;
            for (; *read != 0 && count < 0x32; ++count) {
                unsigned short region = region_remap[*read];
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
unsigned short OctBuildPreTree::SplitSharedPolygon(W8OctPreTreeGeometry* geometry, int index)
{
    W8OctRegionPolygon* polygon = &geometry->m_polygons[index];
    if ((polygon->flags_00 & 4) == 0) {
        return polygon->region_32;
    }
    unsigned short regions[20];
    unsigned short counts[20];
    memset(regions, 0, sizeof(regions));
    polygon->visited_31 = true;
    memset(counts, 0, sizeof(counts));
    unsigned short found = 0;
    for (int corner = 0; corner != 3; ++corner) {
        W8OctPreTreeVertex* vertex = polygon->vertices_34[corner];
        if (vertex->m_visited == 0) {
            vertex->m_visited = 1;
            int* face = vertex->face_indices;
            for (unsigned int n = vertex->face_count_40; n != 0; --n) {
                if (geometry->m_polygons[*face].visited_31 == 0) {
                    unsigned short region = SplitSharedPolygon(geometry, *face);
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
            vertex->m_visited = 0;
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
                if (PointInsideFrustum(&polygon->vertices_34[corner]->position_0c,
                                       spatial_00.m_region_volumes[region].m_planes) != 0) {
                    inside = 1;
                    break;
                }
            }
        } else {
            inside = polygon->InsideFrustumPlanes(spatial_00.m_region_volumes[region].m_planes);
        }
        if (inside != 0) {
            polygon->flags_00 &= ~0xcU;
            polygon->region_32 = region;
            ++spatial_00.m_region_volumes[region].m_polygon_count;
            slot = found;
        }
    }
    if ((polygon->flags_00 & 4) == 0) {
        polygon->visited_31 = false;
    }
    return polygon->region_32;
}

/* Choose the region-tree depth that fits the requested path capacity, build
   the temporary spatial hierarchy, assign every discovered path to its build
   node, and then derive the persistent region metadata. */
// FUNCTION: WIZ8 0x004b19f0
unsigned short OctBuildPreTree::BuildRegions()
{
    W8OctSpatialState working(&spatial_00);

    if (spatial_00.m_region_id_bound == 0) {
        spatial_00.m_region_id_bound = 1;
    }
    spatial_00.m_region_count = spatial_00.m_region_id_bound;
    selected_depth = spatial_00.m_depth;
    region_path_count = 0;

    while (path_capacity_bc < level_counts[selected_depth]) {
        --selected_depth;
    }

    spatial_00.m_leaf_level = 0;
    int last_level = selected_depth - 1;
    float extent = spatial_00.m_extent;
    while (static_cast<int>(spatial_00.m_leaf_level) < last_level) {
        float next_extent = extent * g_float_005ebc7c;
        if (fabs(spatial_00.m_region_grid_cell - extent) <
            fabs(spatial_00.m_region_grid_cell - next_extent)) {
            break;
        }
        ++spatial_00.m_leaf_level;
        extent = next_extent;
    }

    unsigned long level_count = level_counts[spatial_00.m_leaf_level];
    m_pulRegPaths = static_cast<unsigned long*>(malloc(level_count * sizeof(unsigned long) + 8));
    if (m_pulRegPaths == 0) {
        srAssertFail("m_pulRegPaths", OCT_BUILD_PRE_TREE_CPP, 0x6f1, 0);
    }

    m_psrvRegCenters = new srVector3T<float>[level_count + 2 + spatial_00.m_region_id_bound];
    if (m_psrvRegCenters == 0) {
        srAssertFail("m_psrvRegCenters", OCT_BUILD_PRE_TREE_CPP, 0x6f3, 0);
    }

    spatial_00.m_region_grid_cell = working.m_cell_size;
    region_path_map = new W8HashTable<unsigned short, unsigned long>;
    AssignInitialRegions(&working);
    positional_128 = new W8HashTable<unsigned int, short>;
    region_bits = new BitArray(spatial_00.m_region_id_bound);

    for (unsigned short path_index = 0; path_index < region_path_count; ++path_index) {
        unsigned long path = m_pulRegPaths[path_index];
        W8OctBuildNode* node = FindNode(path);
        if (node->leaf_kind != 0 && node->leaf_kind < 25) {
            MergeAdjacentRegion(node, path);
        }
    }

    for (unsigned long polygon = 1; polygon < game_data_134->m_polygon_count; ++polygon) {
        if (game_data_134->m_polygons[polygon].region_32 == 0) {
            char message[252];
            sprintf(message, "Poly %d not found in ANY region.\n", static_cast<int>(polygon));
            ReportBuildStatus(6, message);
        }
    }

    FinalizeRegionMapping();
    AssignRegionFromSurfaces(&working);
    ValidatePolygonRegions();

    delete[] m_psrvRegCenters;
    m_psrvRegCenters = 0;
    if (region_bits != 0) {
        delete region_bits;
    }
    region_bits = 0;

    if (spatial_00.m_region_count == 1) {
        spatial_00.m_region_count = 0;
    }
    return spatial_00.m_region_id_bound;
}

/* Assign a new region to every selected-depth cell containing an unclaimed
   polygon.  The packed path and center are retained for the later adjacency
   merge, while the farthest referenced vertex establishes the region radius
   used by that merge. */
// FUNCTION: WIZ8 0x004b1d90
void OctBuildPreTree::AssignInitialRegions(const W8OctSpatialState* spatial)
{
    W8OctSpatialState child(spatial);
    if (spatial->m_depth >= 16) {
        return;
    }

    if (spatial->m_depth == spatial_00.m_leaf_level) {
        g_poly_list_count = 0;
        W8OctBuildNode* node = spatial->m_root;
        node->CollectLinkedSurfaces(spatial_00.m_leaf_level, spatial_00.m_depth, 2);

        unsigned long unique_count = 0;
        for (unsigned long source = 0; source < g_poly_list_count; ++source) {
            bool found = false;
            for (unsigned long existing = 0; existing < source && !found; ++existing) {
                if (g_poly_list[source] == g_poly_list[existing]) {
                    found = true;
                }
            }
            if (!found) {
                g_poly_list[unique_count++] = g_poly_list[source];
            }
        }
        g_poly_list_count = unique_count;

        int contained_count = 0;
        for (unsigned long index = 0; index < g_poly_list_count; ++index) {
            W8OctRegionPolygon* polygon = static_cast<W8OctRegionPolygon*>(g_poly_list[index]);
            if (polygon->region_32 == 0 && polygon->ContainsPoint(&spatial->m_minimum) != 0) {
                ++contained_count;
                polygon->region_32 = spatial_00.m_region_id_bound;
            }
        }

        if (contained_count != 0) {
            srVector3T<float>& center = m_psrvRegCenters[spatial_00.m_region_id_bound];
            center = (spatial->m_maximum + spatial->m_minimum) * g_float_005ebc7c;

            m_pulRegPaths[region_path_count++] = spatial->m_node_index;

            for (unsigned long index = 0; index < g_poly_list_count; ++index) {
                W8OctRegionPolygon* polygon = static_cast<W8OctRegionPolygon*>(g_poly_list[index]);
                if (polygon->region_32 == spatial_00.m_region_id_bound) {
                    for (int vertex_index = 0; vertex_index != 3; ++vertex_index) {
                        const srVector3T<float>& position =
                            polygon->vertices_34[vertex_index]->position_0c;
                        float distance = (center - position).Length();
                        if (spatial_00.m_max_region_radius < distance) {
                            spatial_00.m_max_region_radius = distance;
                        }
                    }
                }
            }

            region_path_map->Insert(&spatial_00.m_region_id_bound, &spatial->m_node_index);
            node->region_28 = spatial_00.m_region_id_bound;
            node->provisional_region_2c = node->region_28;
            ++spatial_00.m_region_id_bound;
            node->leaf_kind = contained_count;
        }
        return;
    }

    unsigned long path = spatial->m_node_index;
    int high = (static_cast<int>(static_cast<signed char>(path >> 23)) & ~1) + 1;
    int x_base = static_cast<int>(static_cast<signed char>(path >> 15)) & ~1;
    int y_base = static_cast<int>(static_cast<signed char>(path >> 7)) & ~1;
    signed char z_base = static_cast<signed char>(static_cast<signed char>(path) << 1);
    short child_index = 0;
    for (int x = 0; x != 2; ++x) {
        for (int y = 0; y != 2; ++y) {
            for (int z = 0; z != 2; ++z, ++child_index) {
                W8OctBuildNode* parent = spatial->m_root;
                W8OctBuildNode* node = parent->children_00[child_index];
                if (node != 0) {
                    child.m_minimum.x = x * child.m_extent + spatial->m_minimum.x;
                    child.m_maximum.x = child.m_minimum.x + child.m_extent;
                    child.m_minimum.y = y * child.m_extent + spatial->m_minimum.y;
                    child.m_maximum.y = child.m_minimum.y + child.m_extent;
                    child.m_minimum.z = z * child.m_extent + spatial->m_minimum.z;
                    child.m_maximum.z = child.m_minimum.z + child.m_extent;
                    child.m_node_index =
                        ((high * 0x100 + x_base + x) * 0x100 + y_base + y) * 0x100 + z_base + z;
                    child.m_root = node;
                    AssignInitialRegions(&child);
                }
            }
        }
    }
}

/* Follow the active depth bits in a packed octree path. Each active bit
   contributes one x/y/z child selector, from the highest level down. */
// FUNCTION: WIZ8 0x004b23f0
W8OctBuildNode* OctBuildPreTree::FindNode(unsigned int path)
{
    W8OctBuildNode* node = spatial_00.m_root;
    unsigned int active_levels = path >> 24;
    unsigned char x = static_cast<unsigned char>(path >> 16);
    unsigned char y = static_cast<unsigned char>(path >> 8);
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
unsigned char OctBuildPreTree::MergeAdjacentRegion(W8OctBuildNode* node, unsigned int path)
{
    int cell[4];
    cell[0] = path & 0xff000000;
    cell[1] = (path >> 16) & 0xff;
    cell[2] = (path >> 8) & 0xff;
    cell[3] = path & 0xff;

    if (cell[1] != 0) {
        --cell[1];
        if (MergeRegion(node, cell) != 0) {
            return 1;
        }
        ++cell[1];
    }
    ++cell[1];
    if ((cell[1] & (1 << (spatial_00.m_leaf_level + 1))) == 0 && MergeRegion(node, cell) != 0) {
        return 1;
    }
    --cell[1];

    if (cell[2] != 0) {
        --cell[2];
        if (MergeRegion(node, cell) != 0) {
            return 1;
        }
        ++cell[2];
    }
    ++cell[2];
    if ((cell[2] & (1 << (spatial_00.m_leaf_level + 1))) == 0 && MergeRegion(node, cell) != 0) {
        return 1;
    }
    --cell[2];

    if (cell[3] != 0) {
        --cell[3];
        if (MergeRegion(node, cell) != 0) {
            return 1;
        }
        ++cell[3];
    }
    ++cell[3];
    if ((cell[3] & (1 << (spatial_00.m_leaf_level + 1))) == 0 && MergeRegion(node, cell) != 0) {
        return 1;
    }
    return 0;
}

/* Merge one neighboring region into the region carried by the adjacent build
   node. Every packed path is re-keyed in the region table and the surviving
   center becomes the population-weighted average of both groups. */
// FUNCTION: WIZ8 0x004b25c0
unsigned char OctBuildPreTree::MergeRegion(W8OctBuildNode* node, const int* cell)
{
    unsigned long neighbor_path = ((cell[1] * 0x100 + cell[2]) * 0x100 + cell[3]) + cell[0];
    W8OctBuildNode* neighbor = FindNode(neighbor_path);
    if (neighbor == 0 || neighbor->leaf_kind == 0 || neighbor->leaf_kind >= 100) {
        return 0;
    }

    unsigned short neighbor_region = neighbor->region_28;
    unsigned short node_region = node->region_28;
    if (neighbor_region == node_region) {
        return 0;
    }

    srVector3T<float>& node_center = m_psrvRegCenters[node_region];
    srVector3T<float>& neighbor_center = m_psrvRegCenters[neighbor_region];
    float distance = (node_center - neighbor_center).Length();
    if (!(distance < spatial_00.m_region_grid_cell * g_float_005ec52c)) {
        return 0;
    }

    unsigned short combined_count = node->leaf_kind + neighbor->leaf_kind;
    region_bits->Set(neighbor_region);

    unsigned long neighbor_count = 0;
    int position = -1;
    while ((position = region_path_map->FindNextEntry(&neighbor_region, position)) != -1) {
        ++neighbor_count;
        W8OctBuildNode* member = FindNode(region_path_map->entries[position].value);
        member->leaf_kind = combined_count;
    }

    unsigned long moved_count = 0;
    unsigned long path = region_path_map->Lookup(&node_region);
    while (path != 0) {
        ++moved_count;
        region_path_map->Remove(&node_region, &path);
        W8OctBuildNode* member = FindNode(path);
        member->leaf_kind = combined_count;
        member->region_28 = neighbor_region;
        region_path_map->Insert(&neighbor_region, &path);
        path = region_path_map->Lookup(&node_region);
    }

    neighbor_center = (node_center * static_cast<double>(moved_count) +
                       neighbor_center * static_cast<double>(neighbor_count)) /
                      static_cast<double>(moved_count + neighbor_count);
    return 1;
}

/* Compact provisional submesh ids into the final region range, rewrite every
   polygon and path-table entry through that map, and derive aggregate bounds
   for the validation pass. */
// FUNCTION: WIZ8 0x004b2a20
void OctBuildPreTree::FinalizeRegionMapping()
{
    unsigned short* region_map =
        static_cast<unsigned short*>(malloc(spatial_00.m_region_id_bound * sizeof(unsigned short)));
    memset(region_map, 0, spatial_00.m_region_id_bound * sizeof(unsigned short));

    unsigned short next_region = spatial_00.m_region_count;
    for (unsigned short mapping_index = 0; mapping_index < region_path_count; ++mapping_index) {
        W8OctBuildNode* node = FindNode(m_pulRegPaths[mapping_index]);
        if (node->leaf_kind != 0) {
            unsigned short provisional = node->provisional_region_2c;
            if (node->region_28 == provisional) {
                region_map[provisional] = next_region++;
            } else {
                region_map[provisional] = node->region_28;
            }
        }
    }

    unsigned short final_region_count = next_region;
    W8BoundingBox* region_bounds =
        static_cast<W8BoundingBox*>(malloc(final_region_count * sizeof(W8BoundingBox)));
    for (unsigned short region = 0; region < final_region_count; ++region) {
        region_bounds[region].minimum.Set(1000000.0f, 1000000.0f, 1000000.0f);
        region_bounds[region].maximum.Set(-1000000.0f, -1000000.0f, -1000000.0f);
    }

    for (unsigned long polygon_index = 1; polygon_index < game_data_134->m_polygon_count;
         ++polygon_index) {
        W8OctRegionPolygon& polygon = game_data_134->m_polygons[polygon_index];
        unsigned short region = polygon.region_32;
        if (region >= spatial_00.m_region_count) {
            if (region_path_map->Lookup(&region) != 0) {
                polygon.region_32 = region_map[region];
            } else {
                polygon.region_32 = region_map[region_map[region]];
            }
        }

        region = polygon.region_32;
        if (region >= final_region_count) {
            char message[256];
            sprintf(message, "Polygon %d in invalid submesh %d\n", static_cast<int>(polygon_index),
                    static_cast<unsigned int>(region));
            ReportBuildStatus(6, message);
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

    for (unsigned short path_index = 0; path_index < region_path_count; ++path_index) {
        unsigned long path = m_pulRegPaths[path_index];
        W8OctBuildNode* node = FindNode(path);
        unsigned short old_region = node->region_28;
        region_path_map->Remove(&old_region, &path);
        node->region_28 = region_map[old_region];
        region_path_map->Insert(&node->region_28, &path);
        if (node->region_28 >= final_region_count) {
            char message[256];
            sprintf(message, "Invalid submesh %d\n", static_cast<unsigned int>(node->region_28));
            ReportBuildStatus(6, message);
        }
    }

    spatial_00.submesh_count_74 = final_region_count;
    ValidatePolygonRegions();
    ValidateRegionBounds(region_bounds);
    free(region_bounds);
    free(region_map);
}

/* Descend to the selected region depth, collect the surfaces below each
   unassigned node, and choose the most frequent nonzero surface region. */
// FUNCTION: WIZ8 0x004b3050
void OctBuildPreTree::AssignRegionFromSurfaces(const W8OctSpatialState* spatial)
{
    W8OctSpatialState child(spatial);
    if (spatial->m_depth > 15) {
        return;
    }

    if (spatial->m_depth != spatial_00.m_leaf_level) {
        short child_index = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++child_index) {
                    W8OctBuildNode* parent = spatial->m_root;
                    W8OctBuildNode* node = parent->children_00[child_index];
                    if (node != 0) {
                        child.m_minimum.x = x * child.m_extent + spatial->m_minimum.x;
                        child.m_maximum.x = child.m_minimum.x + child.m_extent;
                        child.m_minimum.y = y * child.m_extent + spatial->m_minimum.y;
                        child.m_maximum.y = child.m_minimum.y + child.m_extent;
                        child.m_minimum.z = z * child.m_extent + spatial->m_minimum.z;
                        child.m_maximum.z = child.m_minimum.z + child.m_extent;
                        child.m_root = node;
                        AssignRegionFromSurfaces(&child);
                    }
                }
            }
        }
        return;
    }

    W8OctBuildNode* node = spatial->m_root;
    if (node->region_28 != 0) {
        return;
    }

    g_poly_list_count = 0;
    node->CollectLinkedSurfaces(spatial_00.m_leaf_level, spatial_00.m_depth, 2);

    unsigned long unique_count = 0;
    for (unsigned long source = 0; source < g_poly_list_count; ++source) {
        bool present = false;
        for (unsigned long previous = 0; previous < source && !present; ++previous) {
            if (g_poly_list[source] == g_poly_list[previous]) {
                present = true;
            }
        }
        if (!present) {
            g_poly_list[unique_count++] = g_poly_list[source];
        }
    }

    unsigned short regions[20] = {0};
    unsigned short counts[20] = {0};
    unsigned short selected_region = 0;
    unsigned short selected_count = 0;
    for (unsigned long index = 0; index < unique_count; ++index) {
        unsigned short region = static_cast<W8OctRegionPolygon*>(g_poly_list[index])->region_32;
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

    g_poly_list_count = unique_count;
    node->region_28 = selected_region;
    node->leaf_kind = selected_count;
}

/* Verify that each polygon carrying a generated region id resolves back to a
   build node with that id. A coordinate just below a grid plane is checked in
   both the truncated cell and its negative neighbor. */
// FUNCTION: WIZ8 0x004b3330
void OctBuildPreTree::ValidatePolygonRegions()
{
    unsigned int active_levels = 0;
    for (unsigned int depth = 0; depth < spatial_00.m_leaf_level; ++depth) {
        active_levels |= 1 << (depth + 24);
    }

    for (unsigned long polygon_index = 1; polygon_index < game_data_134->m_polygon_count;
         ++polygon_index) {
        W8OctRegionPolygon& polygon = game_data_134->m_polygons[polygon_index];
        if (polygon.region_32 < spatial_00.m_region_count) {
            continue;
        }

        float relative_x = polygon.position_18.x - spatial_00.m_minimum.x;
        int x = static_cast<int>(relative_x / spatial_00.m_region_grid_cell);
        short x_count = 1;
        if (g_float_005ed034 < x * spatial_00.m_region_grid_cell - relative_x) {
            --x;
            x_count = 2;
        }

        float relative_y = polygon.position_18.y - spatial_00.m_minimum.y;
        int y = static_cast<int>(relative_y / spatial_00.m_region_grid_cell);
        short y_count = 1;
        if (g_float_005ed034 < y * spatial_00.m_region_grid_cell - relative_y) {
            --y;
            y_count = 2;
        }

        float relative_z = polygon.position_18.z - spatial_00.m_minimum.z;
        int z = static_cast<int>(relative_z / spatial_00.m_region_grid_cell);
        short z_count = 1;
        if (g_float_005ed034 < z * spatial_00.m_region_grid_cell - relative_z) {
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
                    W8OctBuildNode* node = FindNode(path);
                    if (node->region_28 == polygon.region_32) {
                        found = true;
                    }
                }
            }
        }

        if (!found) {
            char message[256];
            sprintf(message, "Poly %d not found in correct region.\n",
                    static_cast<int>(polygon_index));
            ReportBuildStatus(6, message);
        }
    }
}

/* Check every path assigned to a region against both the build-node region id
   and the aggregate automesh bounds produced for that region. */
// FUNCTION: WIZ8 0x004b35b0
void OctBuildPreTree::ValidateRegionBounds(const W8BoundingBox* region_bounds)
{
    for (unsigned long region_index = 1; region_index < spatial_00.submesh_count_74;
         ++region_index) {
        unsigned short region = static_cast<unsigned short>(region_index);
        int entry = -1;
        while ((entry = region_path_map->FindNextEntry(&region, entry)) != -1) {
            unsigned long path = region_path_map->entries[entry].value;
            srVector3T<float> minimum;
            minimum.Set(
                ((path >> 16) & 0xff) * spatial_00.m_region_grid_cell + spatial_00.m_minimum.x,
                ((path >> 8) & 0xff) * spatial_00.m_region_grid_cell + spatial_00.m_minimum.y,
                (path & 0xff) * spatial_00.m_region_grid_cell + spatial_00.m_minimum.z);
            srVector3T<float> maximum;
            maximum.Set(minimum.x + spatial_00.m_region_grid_cell,
                        minimum.y + spatial_00.m_region_grid_cell,
                        minimum.z + spatial_00.m_region_grid_cell);

            W8OctBuildNode* node = FindNode(path);
            if (node != 0) {
                if (node->region_28 != region) {
                    ReportBuildStatus(7, "Region has wrong automesh.");
                }
                const W8BoundingBox& bounds = region_bounds[region_index];
                if (maximum.x < bounds.minimum.x || bounds.maximum.x < minimum.x ||
                    maximum.y < bounds.minimum.y || bounds.maximum.y < minimum.y ||
                    maximum.z < bounds.minimum.z || bounds.maximum.z < minimum.z) {
                    ReportBuildStatus(7, "AutoMesh has no vertices inside region. You probably "
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
unsigned char OctBuildPreTree::BuildParticleRegions(const W8LevelFileParticleSystem* particles,
                                                    int particle_count)
{
    particle_count_11c = particle_count;
    inside_region_map = new W8HashTable<unsigned short, short>;

    short region_particles[4998];
    region_particles[0] = 0;
    unsigned short list_count = 0;
    int particle_number = 0;

    for (int particle_index = 0; particle_index < particle_count; ++particle_index) {
        const W8LevelParticleRecord& particle = particles[particle_index].particle;
        char name[64];
        strcpy(name, particle.name);
        _strupr(name);
        if (strncmp(name, "CLOUD", 5) == 0) {
            continue;
        }

        ++particle_number;
        short particle_value = static_cast<short>(particle_number);
        srVector3T<float> position;
        position = particle.location * g_world_scale;

        bool mapped = false;
        for (unsigned short region_index = 1; region_index < spatial_00.m_region_count;
             ++region_index) {
            W8OctRegionVolume* volume = spatial_00.m_region_volumes + region_index;
            if (volume->ContainsPoint(&position) != 0) {
                unsigned short region = volume->m_region;
                bool present = false;
                int entry = -1;
                while ((entry = inside_region_map->FindNextEntry(&region, entry)) != -1) {
                    if (inside_region_map->entries[entry].value == particle_value) {
                        present = true;
                        break;
                    }
                }
                if (!present) {
                    inside_region_map->Insert(&region, &particle_value);
                }
                mapped = true;
            }
        }
        if (mapped) {
            continue;
        }

        if (UpdateRegionForGeometry(&position, particle_value, 6) == 0) {
            srVector3T<float> extent;
            bool has_bounds = false;
            if (particle.bounds_mode == 1) {
                extent = particle.bounds_extent * g_startup_near_limit;
                has_bounds = true;
            } else if (particle.bounds_mode == 2 && particle.bounds_radius > g_float_zero) {
                extent = particle.bounds_origin * g_world_scale;
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
                                 region_index < spatial_00.m_region_count; ++region_index) {
                                W8OctRegionVolume* volume =
                                    spatial_00.m_region_volumes + region_index;
                                if (volume->ContainsPoint(&corner) != 0) {
                                    unsigned short region = volume->m_region;
                                    bool present = false;
                                    int entry = -1;
                                    while ((entry = inside_region_map->FindNextEntry(
                                                &region, entry)) != -1) {
                                        if (inside_region_map->entries[entry].value ==
                                            particle_value) {
                                            present = true;
                                            break;
                                        }
                                    }
                                    if (!present) {
                                        inside_region_map->Insert(&region, &particle_value);
                                    }
                                    corner_mapped = true;
                                    mapped = true;
                                }
                            }
                            if (!corner_mapped &&
                                UpdateRegionForGeometry(&corner, particle_value, 6) != 0) {
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
    mesh_particle_lookup =
        static_cast<unsigned short*>(malloc(region_count * sizeof(unsigned short)));
    memset(mesh_particle_lookup, 0, region_count * sizeof(unsigned short));

    for (unsigned short region = 1; region < region_count; ++region) {
        int entry = -1;
        bool first = true;
        while ((entry = inside_region_map->FindNextEntry(&region, entry)) != -1) {
            if (first) {
                mesh_particle_lookup[region] = list_count;
                first = false;
            }
            region_particles[list_count++] = inside_region_map->entries[entry].value;
        }
        if (!first) {
            region_particles[list_count++] = 0;
        }
    }

    mesh_particles = static_cast<unsigned short*>(malloc(list_count * sizeof(unsigned short)));
    if (mesh_particles != 0) {
        memcpy(mesh_particles, region_particles, list_count * sizeof(unsigned short));
    }
    mesh_particle_count = list_count;
    delete inside_region_map;
    return 1;
}

/* Accumulate the two preprocessing geometry banks into one region-to-prop
   table. The first call owns the shared scratch list; the final call appends
   the second bank, emits the compact lookup/list pair, and releases scratch. */
// FUNCTION: WIZ8 0x004b3f90
unsigned char OctBuildPreTree::BuildGeometryRegions(const W8LevelFileProp* records,
                                                    int record_count, int base_index, bool finalize)
{
    if (finalize == 0) {
        overlap_region_map = new W8HashTable<unsigned short, short>;
        g_region_id_list = static_cast<unsigned short*>(malloc(10000));
        g_region_id_list[0] = 0;
        g_region_id_count = 0;
    }

    for (int record_index = 0; record_index < record_count; ++record_index) {
        const W8LevelFileProp& record = records[record_index];
        short value = static_cast<short>(base_index + 1 + record_index);
        bool mapped = false;

        for (unsigned short region_index = 1; region_index < spatial_00.m_region_count;
             ++region_index) {
            W8OctRegionVolume* volume = spatial_00.m_region_volumes + region_index;
            for (unsigned char bounds_index = 0; bounds_index < record.anim_obj.num_bound_box;
                 ++bounds_index) {
                const srVector3T<float>* bounds = &record.anim_obj.pBoundBox[bounds_index].minimum;
                for (int x = 0; x != 2; ++x) {
                    for (int y = 0; y != 2; ++y) {
                        for (int z = 0; z != 2; ++z) {
                            srVector3T<float> corner;
                            corner.Set(bounds[x].x * g_world_scale, bounds[y].y * g_world_scale,
                                       bounds[z].z * g_world_scale);
                            if (volume->ContainsPoint(&corner) != 0) {
                                unsigned short region = volume->m_region;
                                bool present = false;
                                int entry = -1;
                                while ((entry = overlap_region_map->FindNextEntry(
                                            &region, entry)) != -1) {
                                    if (overlap_region_map->entries[entry].value == value) {
                                        present = true;
                                        break;
                                    }
                                }
                                if (!present) {
                                    overlap_region_map->Insert(&region, &value);
                                }
                                mapped = true;
                            }
                        }
                    }
                }
            }
        }

        srVector3T<float> aggregate[2];
        aggregate[0].Set(1000000.0f, 1000000.0f, 1000000.0f);
        aggregate[1].Set(-1000000.0f, -1000000.0f, -1000000.0f);
        for (unsigned char bounds_index = 0; bounds_index < record.anim_obj.num_bound_box;
             ++bounds_index) {
            const srVector3T<float>* bounds = &record.anim_obj.pBoundBox[bounds_index].minimum;
            for (int endpoint = 0; endpoint != 2; ++endpoint) {
                srVector3T<float> point;
                point = bounds[endpoint] * g_world_scale;
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

        if (UpdateRegionForGeometry(aggregate, value, 5) == 0 && !mapped) {
            g_region_id_list[g_region_id_count++] = value;
        }
    }

    if (finalize != 0) {
        if (g_region_id_count == 0) {
            g_region_id_count = 1;
        } else {
            g_region_id_list[g_region_id_count++] = 0;
        }

        unsigned long region_count = spatial_00.submesh_count_74;
        mesh_prop_lookup =
            static_cast<unsigned short*>(malloc(region_count * sizeof(unsigned short)));
        for (unsigned long region_value = 1; region_value < region_count; ++region_value) {
            unsigned short region = static_cast<unsigned short>(region_value);
            mesh_prop_lookup[region_value] = 0;
            int entry = -1;
            bool first = true;
            while ((entry = overlap_region_map->FindNextEntry(&region, entry)) != -1) {
                if (first) {
                    mesh_prop_lookup[region_value] = g_region_id_count;
                    first = false;
                }
                g_region_id_list[g_region_id_count++] =
                    overlap_region_map->entries[entry].value;
            }
            if (!first) {
                g_region_id_list[g_region_id_count++] = 0;
            }
        }

        mesh_props =
            static_cast<unsigned short*>(malloc(g_region_id_count * sizeof(unsigned short)));
        if (mesh_props != 0) {
            memcpy(mesh_props, g_region_id_list, g_region_id_count * sizeof(unsigned short));
        }
        mesh_prop_count = g_region_id_count;
        prop_count_120 = record_count + base_index;
        delete overlap_region_map;
        free(g_region_id_list);
    }
    return 1;
}

/* Build the compact runtime pre-tree, transfer the auxiliary ownership that
   already has runtime form, consume the counted build-node hierarchy, and
   populate the dense cell-to-leaf lookup. */
// FUNCTION: WIZ8 0x004b4640
OctPreTree* OctBuildPreTree::BuildOctPreTree()
{
    OctPreTree* tree = new OctPreTree;
    if (tree == 0) {
        return 0;
    }

    tree->m_branches = static_cast<W8OctPreTreeBranch*>(
        malloc((g_build_node_instances * 9 + 0x12) * sizeof(unsigned long)));
    if (tree->m_branches == 0) {
        return 0;
    }
    memset(tree->m_branches, 0, (g_build_node_instances * 9 + 0x12) * sizeof(unsigned long));

    tree->m_leaves =
        static_cast<W8OctPreTreeLeaf*>(malloc((leaf_count_a8 + 2) * sizeof(W8OctPreTreeLeaf)));
    if (tree->m_leaves == 0) {
        return 0;
    }
    memset(tree->m_leaves, 0, (leaf_count_a8 + 2) * sizeof(W8OctPreTreeLeaf));

    tree->m_polygon_index_stream =
        static_cast<unsigned long*>(malloc(leaf_polygon_count * 2 * sizeof(unsigned long)));
    if (tree->m_polygon_index_stream == 0) {
        return 0;
    }
    memset(tree->m_polygon_index_stream, 0, leaf_polygon_count * 2 * sizeof(unsigned long));

    tree->m_region_index_stream = static_cast<unsigned short*>(malloc(leaf_count_a8 * 0x50));
    if (tree->m_region_index_stream == 0) {
        return 0;
    }
    memset(tree->m_region_index_stream, 0, leaf_count_a8 * 0x50);

    tree->m_gd_surface_index_stream =
        static_cast<unsigned long*>(malloc(gd_surface_count * 2 * sizeof(unsigned long)));
    if (tree->m_gd_surface_index_stream == 0) {
        return 0;
    }
    memset(tree->m_gd_surface_index_stream, 0, gd_surface_count * 2 * sizeof(unsigned long));

    tree->m_spatial.m_region_count = spatial_00.m_region_count;
    tree->m_spatial.m_leaf_level = spatial_00.m_leaf_level;
    tree->m_spatial.submesh_count_74 = spatial_00.submesh_count_74;
    tree->m_spatial.m_region_id_bound = spatial_00.m_region_id_bound;

    tree->m_pusMeshParticleLookup = mesh_particle_lookup;
    mesh_particle_lookup = 0;
    tree->m_pusMeshParticles = mesh_particles;
    mesh_particles = 0;
    tree->m_usMeshParticlesLen = mesh_particle_count;
    tree->m_pusMeshPropLookup = mesh_prop_lookup;
    mesh_prop_lookup = 0;
    tree->m_pusMeshProps = mesh_props;
    tree->m_usMeshPropsLen = mesh_prop_count;
    mesh_props = 0;
    tree->m_ulNumParticles = particle_count_11c;
    tree->m_ulNumProps = prop_count_120;

    tree->m_spatial.m_depth = spatial_00.m_depth;
    tree->m_spatial.m_node_extent = spatial_00.m_node_extent;
    tree->m_spatial.m_max_region_radius = spatial_00.m_max_region_radius < g_float_005ed038
                                              ? spatial_00.m_max_region_radius
                                              : g_float_005ed038;
    while (selected_depth < tree->m_spatial.m_depth) {
        ReportBuildStatus(6, "Collapsing tree by one level.\n");
        --tree->m_spatial.m_depth;
        tree->m_spatial.m_node_extent += tree->m_spatial.m_node_extent;
    }

    tree->m_spatial.m_extent = spatial_00.m_extent;
    tree->m_spatial.m_cell_size = spatial_00.m_cell_size;
    tree->m_spatial.m_region_volumes = spatial_00.m_region_volumes;
    tree->game_data_3a4 = game_data_134;
    tree->unknown_3a8 = unknown_138;
    tree->unknown_3ac = unknown_13c;
    tree->deepest_link_list_3b0 = deepest_link_list_b8;
    tree->m_spatial.m_item_count = spatial_00.m_item_count;
    tree->m_spatial.m_polygon_count = spatial_00.m_polygon_count;
    tree->m_visited_polygon_bits = new BitArray(spatial_00.m_polygon_count);
    tree->m_spatial.m_region_grid_cell = spatial_00.m_region_grid_cell;
    tree->m_spatial.m_region_cells_per_axis = spatial_00.m_region_cells_per_axis;
    tree->automesh_cells = region_path_map;
    region_path_map = 0;

    for (int axis = 0; axis != 3; ++axis) {
        (&tree->m_spatial.m_minimum.x)[axis] = (&spatial_00.m_minimum.x)[axis];
        (&tree->m_spatial.m_maximum.x)[axis] = (&spatial_00.m_maximum.x)[axis];
        (&tree->m_spatial.m_clipped_minimum.x)[axis] = (&spatial_00.m_clipped_minimum.x)[axis];
        (&tree->m_spatial.m_clipped_maximum.x)[axis] = (&spatial_00.m_clipped_maximum.x)[axis];
    }

    tree->m_branch_count = 1;
    tree->m_leaf_count = 1;
    tree->polygon_cursor = 1;
    tree->m_region_list_len = 1;
    tree->m_gd_surface_stream_len = 1;
    tree->m_unknown_13c = 0;
    tree->m_trigger_count = 0;

    W8OctBuildNode* root = spatial_00.m_root;
    root->ConvertToOctPreTree(0, tree);
    delete root;
    spatial_00.m_root = 0;
    if (tree->m_region_list_len == 1) {
        tree->m_region_list_len = 0;
    }

    for (int grid_axis = 0; grid_axis != 3; ++grid_axis) {
        (&tree->m_leaf_grid_dim_x)[grid_axis] =
            static_cast<int>(((&tree->m_spatial.m_clipped_maximum.x)[grid_axis] -
                              (&tree->m_spatial.m_clipped_minimum.x)[grid_axis]) /
                             tree->m_spatial.m_node_extent) +
            1;
    }
    tree->m_leaf_lookup =
        static_cast<unsigned long*>(malloc(tree->m_leaf_grid_dim_z * tree->m_leaf_grid_dim_x *
                                           tree->m_leaf_grid_dim_y * sizeof(unsigned long)));
    unsigned long cell_index = 0;
    srVector3T<int> point;
    for (point.x = 0; point.x < static_cast<int>(tree->m_leaf_grid_dim_x); ++point.x) {
        for (point.y = 0; point.y < static_cast<int>(tree->m_leaf_grid_dim_y); ++point.y) {
            for (point.z = 0; point.z < static_cast<int>(tree->m_leaf_grid_dim_z); ++point.z) {
                tree->m_leaf_lookup[cell_index] = tree->FindLeaf(&point);
                if (tree->m_leaf_count < tree->m_leaf_lookup[cell_index]) {
                    tree->m_leaf_lookup[cell_index] = 0;
                }
                ++cell_index;
            }
        }
    }
    tree->m_spatial.m_leaf_grid_stride_y = tree->m_leaf_grid_dim_z;
    tree->m_spatial.m_leaf_grid_stride_x = tree->m_leaf_grid_dim_z * tree->m_leaf_grid_dim_y;
    return tree;
}

// FUNCTION: WIZ8 0x004afe90
int GetBuildNodeInstanceCount(void)
{
    return g_build_node_instances;
}

/* Region-path entries have 16-bit keys and four-byte values (Lookup caller
   0x004B25C0; Grow owner 0x004B19F0). Region maps at +0x12C/+0x130 have word
   keys/values (callers 0x004B3F90 and 0x004B07E0). These flows establish storage
   families, not unsigned-long spelling or word-value signedness. */
