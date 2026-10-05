#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define OCT_BUILD_TREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildTree.cpp"

// GLOBAL: WIZ8 0x005ec188
const float g_float_005ec188 = 1.000100016593933f;

// GLOBAL: WIZ8 0x00659a48
W8GDSurface** g_oct_build_scratch;
/* The running count of surfaces appended into the scratch buffer by the
   leaf collector. Saved and restored around nested collects. */
// GLOBAL: WIZ8 0x00659a38
static unsigned long g_oct_build_count;
/* The caller's result slot during a segment collect; retail writes it but
   no recovered reader exists. */
// GLOBAL: WIZ8 0x00659a44
static void* g_oct_build_out;

char CollectSurfacePredicate(W8GDSurface* surface, short kind);

W8OctBuildLinkLists::W8OctBuildLinkLists() : m_usCurrent(0), unknown_02(0)
{
    for (int index = 0; index != 100; ++index) {
        m_apLinkLists[index] = 0;
        m_ausLinkCounts[index] = 0;
    }
}

/* The original member names come from this body's assertion.  Each bank owns
   50,000 eight-byte links.  A fresh zeroed link records the surface while its
   next pointer remains null. */
// FUNCTION: WIZ8 0x00446250
W8OctBuildLink* W8OctBuildLinkLists::GetNewLink(void* surface)
{
    if (m_ausLinkCounts[m_usCurrent] > 49999) {
        ++m_usCurrent;
    }
    if (m_usCurrent < 100) {
        if (m_apLinkLists[m_usCurrent] == 0) {
            m_apLinkLists[m_usCurrent] =
                static_cast<W8OctBuildLink*>(malloc(50000 * sizeof(W8OctBuildLink)));
            if (m_apLinkLists[m_usCurrent] == 0) {
                srAssertFail("m_apLinkLists[m_usCurrent]", OCT_BUILD_TREE_CPP, 140,
                             "GetNewLink: Couldn't allocate m_apLinkLists.");
            }
            memset(m_apLinkLists[m_usCurrent], 0, 50000 * sizeof(W8OctBuildLink));
        }
        W8OctBuildLink* link = &m_apLinkLists[m_usCurrent][m_ausLinkCounts[m_usCurrent]++];
        link->surface = surface;
        return link;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00446330
W8OctBuildNode::W8OctBuildNode()
{
    memset(this, 0, 10 * sizeof(unsigned long));
    leaf_kind = 0;
    region = 0;
    provisional_region = 0;
}

// FUNCTION: WIZ8 0x00446350
W8OctBuildNode::~W8OctBuildNode()
{
    if (leaf_kind != 0) {
        memset(this, 0, 10 * sizeof(unsigned long));
        return;
    }
    for (int child = 0; child != 8; ++child) {
        delete children[child];
    }
}

/* Build the cubic spatial domain used to insert processed GameData surfaces.
   The caller supplies local copies of the level bounds because this constructor
   deliberately expands them by half a leaf on every axis. */
// FUNCTION: WIZ8 0x00446390
W8OctBuildTree::W8OctBuildTree(float leaf_size, srVector3T<float>* minimum,
                               srVector3T<float>* maximum, unsigned short item_limit,
                               short extent_mode)
    : spatial(0)
{
    spatial.Reset();
    link_lists = 0;
    leaf_polygon_count = 0;
    gd_surface_count = 0;
    leaf_count = 0;
    max_leaf_regions = 0;
    unknown_ae = 0;
    region_assignments = 0;
    use_owned_nodes = false;
    unknown_b5[0] = 0;
    unknown_b5[1] = 0;
    unknown_b5[2] = 0;
    deepest_link_list = 0;

    if (leaf_size < g_float_005ebc64) {
        ReportBuildStatus(7, "Leaf Size too small--try a larger leaf size!\n");
    }
    spatial.flags = 0;
    spatial.m_item_limit = item_limit;
    spatial.m_level_kind = 2;
    spatial.m_extent = 0.0f;
    spatial.m_root = 0;
    spatial.m_triangle_vertices = 0;
    spatial.m_depth = 0;

    if (minimum != 0 || maximum != 0) {
        float half_leaf = leaf_size * g_float_005ebc7c;
        float* source_minimum = &minimum->x;
        float* source_maximum = &maximum->x;
        float* stored_minimum = &spatial.m_clipped_minimum.x;
        for (int axis = 0; axis != 3; ++axis) {
            source_minimum[axis] -= half_leaf;
            source_maximum[axis] += half_leaf;
            (&spatial.m_minimum.x)[axis] = source_minimum[axis];
            stored_minimum[axis] = source_minimum[axis];
            (&spatial.m_working_minimum.x)[axis] = source_minimum[axis];
            (&spatial.m_clipped_maximum.x)[axis] = source_maximum[axis];
            (&spatial.m_working_maximum.x)[axis] = source_maximum[axis];
            float span = source_maximum[axis] - source_minimum[axis];
            if (spatial.m_extent < span) {
                spatial.m_extent = span;
            }
        }

        spatial.m_depth = 0;
        if (extent_mode == 0) {
            spatial.m_node_extent = spatial.m_extent;
            while (leaf_size + leaf_size <= spatial.m_node_extent && spatial.m_depth < 6) {
                spatial.m_node_extent *= g_float_005ebc7c;
                ++spatial.m_depth;
            }
        } else if (extent_mode == 1) {
            spatial.m_node_extent = leaf_size;
            spatial.m_cell_size = leaf_size;
            while (spatial.m_cell_size < spatial.m_extent) {
                if (spatial.m_depth > 6) {
                    break;
                }
                spatial.m_cell_size += spatial.m_cell_size;
                ++spatial.m_depth;
            }
            if (spatial.m_depth > 6) {
                ReportBuildStatus(7, "Leaf Size too small--try a larger leaf size!\n");
            }
            spatial.m_extent = spatial.m_cell_size;
        } else {
            spatial.m_node_extent = spatial.m_extent;
            while (leaf_size + leaf_size <= spatial.m_node_extent && spatial.m_depth < 6) {
                spatial.m_node_extent *= g_float_005ebc7c;
                ++spatial.m_depth;
            }
            if (extent_mode == 2) {
                float doubled = spatial.m_node_extent + spatial.m_node_extent;
                if (doubled - leaf_size < leaf_size - spatial.m_node_extent) {
                    --spatial.m_depth;
                    spatial.m_node_extent = doubled;
                }
            }
        }

        spatial.m_cell_size = spatial.m_node_extent * g_float_005ec188;
        spatial.m_maximum.Set(minimum->x + spatial.m_extent, minimum->y + spatial.m_extent,
                                 minimum->z + spatial.m_extent);
        g_oct_build_scratch = static_cast<W8GDSurface**>(malloc(40000));
        spatial.m_polygon_count = 1;
        spatial.m_item_count = 0;
        spatial.m_root = 0;
        spatial.m_triangle_vertices = 0;
        link_lists = new W8OctBuildLinkLists;
    }
}

/* Release the recursive node tree, the global construction scratch buffer and
   every allocated link bank.  The first-member spatial value performs its own
   shallow teardown after this body returns. */
// FUNCTION: WIZ8 0x004466d0
W8OctBuildTree::~W8OctBuildTree()
{
    delete spatial.m_root;
    if (g_oct_build_scratch != 0) {
        free(g_oct_build_scratch);
    }
    g_oct_build_scratch = 0;

    spatial.m_triangle_vertices = 0;
    if (link_lists != 0) {
        for (int index = 0; index != 100; ++index) {
            if (link_lists->m_apLinkLists[index] != 0) {
                free(link_lists->m_apLinkLists[index]);
            }
            link_lists->m_apLinkLists[index] = 0;
            link_lists->m_ausLinkCounts[index] = 0;
        }
        delete link_lists;
    }
}

/* Recursive node teardown: internal nodes delete the eight children, leaf
   nodes (leaf_kind != 0) clear the ten link/array slots. */

/* Reject triangles outside the build domain, lazily create the root node, and
   then hand the complete typed working record to the recursive inserter. */
// FUNCTION: WIZ8 0x00446820
unsigned char W8OctBuildTree::InsertSurface(W8GDSurface* surface, unsigned long mode)
{
    W8OctSpatialState working(&spatial);
    srVector3T<float> vertices[3];
    srVector3T<float> plane_point;
    srVector3T<float>* plane = &plane_point;

    if (static_cast<short>(mode) == 3) {
        if (LoadSurfaceVertices(vertices, surface->vertex_indices) == 0) {
            plane = 0;
        } else {
            plane_point = surface->plane.normal;
        }
    }
    if (TestSpatialTriangle(&spatial.m_minimum, vertices, plane) == 0) {
        return 0;
    }

    if (spatial.m_root == 0) {
        if (!use_owned_nodes) {
            spatial.m_root = new W8OctBuildNode;
        } else {
            spatial.m_root = new W8CountedOctBuildNode;
        }
    }
    working.m_root = spatial.m_root;
    working.m_triangle_vertices = vertices;
    working.m_depth = 0;
    working.m_level_kind = 1;
    if (InsertSurfaceRecursive(&working, surface, &plane_point, mode) == 0) {
        return 0;
    }
    ++spatial.m_item_count;
    return 1;
}

/* Descend through every overlapping octant.  Branch nodes own child nodes;
   leaf nodes reuse the same eight slots as per-mode linked-list heads. */
// FUNCTION: WIZ8 0x004469f0
unsigned char W8OctBuildTree::InsertSurfaceRecursive(W8OctSpatialState* working,
                                                     W8GDSurface* surface,
                                                     srVector3T<float>* plane_point,
                                                     unsigned long mode)
{
    W8OctSpatialState child(working);
    bool inserted = false;

    if (spatial.m_depth < working->m_depth) {
        spatial.m_depth = working->m_depth;
    }

    if (working->m_extent <= working->m_cell_size) {
        W8OctBuildNode* node = working->m_root;
        AppendLink(node, surface, static_cast<short>(mode));
        inserted = true;
    } else {
        float half_extent = working->m_extent * g_float_005ebc7c;
        child.m_cell_size = working->m_cell_size;
        child.m_depth = working->m_depth + 1;

        short octant = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++octant) {
                    child.m_minimum.x = working->m_minimum.x + x * half_extent;
                    child.m_maximum.x = child.m_minimum.x + half_extent;
                    child.m_minimum.y = working->m_minimum.y + y * half_extent;
                    child.m_maximum.y = child.m_minimum.y + half_extent;
                    child.m_minimum.z = working->m_minimum.z + z * half_extent;
                    child.m_maximum.z = child.m_minimum.z + half_extent;

                    if (TestSpatialTriangle(&child.m_minimum, working->m_triangle_vertices,
                                            plane_point) != 0) {
                        W8OctBuildNode* node = working->m_root;
                        if (node->children[octant] == 0) {
                            if (!use_owned_nodes) {
                                node->children[octant] = new W8OctBuildNode;
                            } else {
                                node->children[octant] = new W8CountedOctBuildNode;
                            }
                        }
                        child.m_root = node->children[octant];
                        child.m_triangle_vertices = working->m_triangle_vertices;
                        if (InsertSurfaceRecursive(&child, surface, plane_point, mode) != 0) {
                            inserted = true;
                        }
                    }
                }
            }
        }
    }
    return inserted;
}

/* Append `payload` to the node's `kind` link list: bump the leaf counter and
   the tree watermark, then either extend the tail or seed the head. */
// FUNCTION: WIZ8 0x00446d00
void W8OctBuildTree::AppendLink(W8OctBuildNode* node, void* payload, short kind)
{
    ++node->leaf_kind;
    if (deepest_link_list < node->leaf_kind) {
        deepest_link_list = node->leaf_kind;
    }
    W8OctBuildLink* head = node->links[kind];
    if (head == 0) {
        node->links[kind] = link_lists->GetNewLink(payload);
        return;
    }
    W8OctBuildLink* next;
    for (next = head->next; next != 0; next = next->next) {
        head = next;
    }
    head->next = link_lists->GetNewLink(payload);
}

/* Segment query over the build tree: seed the caller's result array with the
   shared scratch buffer, expand the origin-to-origin+delta bounds by `extent`, then
   walk the tree. `half_angle` is unused. Collected surfaces carry the 0x2000
   visit mark, which this clears before returning the count. */
// FUNCTION: WIZ8 0x00446d80
int W8OctBuildTree::CollectObjectsAlongSegment(W8GDSurface*** results,
                                               const srVector3T<float>* origin,
                                               const srVector3T<float>* delta, float half_angle,
                                               float extent, unsigned short kind)
{
    W8OctSpatialState state(&spatial);
    srVector3T<float> bounds[2];
    unsigned long saved = 0;
    unsigned int index;

    if (*results == 0) {
        *results = g_oct_build_scratch;
        g_oct_build_out = g_oct_build_scratch;
    } else {
        saved = g_oct_build_count;
        g_oct_build_out = results;
    }
    g_oct_build_count = 0;

    float length = delta->Length();
    if (length > extent) {
        extent = length;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if ((&delta->x)[axis] > g_float_zero) {
            (&bounds[0].x)[axis] = (&origin->x)[axis] - extent;
            (&bounds[1].x)[axis] = extent + (&origin->x)[axis] + (&delta->x)[axis];
        } else {
            (&bounds[0].x)[axis] = (&origin->x)[axis] - extent + (&delta->x)[axis];
            (&bounds[1].x)[axis] = extent + (&origin->x)[axis];
        }
    }
    state.m_depth = 0;
    state.m_level_kind = 1;
    CollectRecursive(&state, bounds, static_cast<short>(kind));

    int count;
    if (saved != 0) {
        count = g_oct_build_count;
        g_oct_build_count = saved;
    } else {
        count = g_oct_build_count;
    }
    index = 0;
    if (count != 0) {
        do {
            W8GDSurface* surface = (*results)[index];
            ++index;
            surface->flags &= ~0x2000;
        } while (index < static_cast<unsigned int>(count));
    }
    return count;
}

/* Recursive descent: classify the state's box against the query bounds, then
   collect the whole leaf subtree (2), walk the eight octants (1), or skip
   (0). A state already at the bottom level collects as a leaf either way. */
// FUNCTION: WIZ8 0x00446f20
int W8OctBuildTree::CollectRecursive(W8OctSpatialState* state, const srVector3T<float>* bounds,
                                     short kind)
{
    W8OctSpatialState child(state);
    int collected = 0;
    bool leaf;
    srVector3T<float> box[2];

    leaf = false;
    if (state->m_depth == spatial.m_depth) {
        leaf = true;
    }
    box[0] = state->m_minimum;
    box[1] = state->m_maximum;
    int verdict = ClassifyBoxBounds(box, bounds, leaf);
    if (verdict == 2 || (verdict == 1 && leaf)) {
        collected = CollectLeaf(state->m_root, state->m_depth, kind);
    } else if (verdict == 1) {
        short octant = 0;
        int x = 0;
        do {
            int y = 0;
            int y_count = 2;
            do {
                int z = 0;
                int z_count = 2;
                do {
                    W8OctBuildNode* node = state->m_root;
                    if (node->children[octant] != 0) {
                        child.m_minimum.x = x * child.m_extent + state->m_minimum.x;
                        child.m_maximum.x = child.m_minimum.x + child.m_extent;
                        child.m_minimum.y = y * child.m_extent + state->m_minimum.y;
                        child.m_maximum.y = child.m_minimum.y + child.m_extent;
                        child.m_minimum.z = z * child.m_extent + state->m_minimum.z;
                        child.m_maximum.z = child.m_minimum.z + child.m_extent;
                        child.m_root = node->children[octant];
                        collected += CollectRecursive(&child, bounds, kind);
                    }
                    ++octant;
                    ++z;
                    --z_count;
                } while (z_count != 0);
                ++y;
                --y_count;
            } while (y_count != 0);
            ++x;
        } while (octant < 8);
    }
    return collected;
}

/* Leaf collector: at the bottom level walk the node's link lists and append
   qualifying surfaces to the shared scratch buffer, otherwise recurse into
   the eight children with depth+1. Kind 9 marks list-3 surfaces with the
   0x2000 flag and dedup-scans list 4; kind 10 dedup-scans list 7 and runs the
   predicate on the 0xb list; other kinds run the predicate on list `kind`. */
// FUNCTION: WIZ8 0x00447110
int W8OctBuildTree::CollectLeaf(W8OctBuildNode* node, short depth, short kind)
{
    W8OctBuildLink* link;
    W8GDSurface* surface;
    W8GDSurface** scan;
    unsigned long index;
    int collected = 0;
    int second;

    if (depth == spatial.m_depth) {
        if (kind == 9) {
            link = node->links[3];
            while (link != 0) {
                surface = static_cast<W8GDSurface*>(link->surface);
                if ((surface->flags & 0x2000) == 0) {
                    surface->flags |= 0x2000;
                    g_oct_build_scratch[g_oct_build_count] =
                        static_cast<W8GDSurface*>(link->surface);
                    ++g_oct_build_count;
                    ++collected;
                }
                link = link->next;
            }
            second = 0;
            if (node->links[4] != 0) {
                for (link = node->links[4]; link != 0; link = link->next) {
                    surface = static_cast<W8GDSurface*>(link->surface);
                    index = 0;
                    scan = g_oct_build_scratch;
                    while (index < g_oct_build_count) {
                        if (*scan == surface) {
                            goto next_link_4;
                        }
                        ++index;
                        ++scan;
                    }
                    g_oct_build_scratch[g_oct_build_count] = surface;
                    ++g_oct_build_count;
                    ++second;
                next_link_4:;
                }
            }
            return collected + second;
        }
        if (kind == 10) {
            if (node->links[7] != 0) {
                for (link = node->links[7]; link != 0; link = link->next) {
                    surface = static_cast<W8GDSurface*>(link->surface);
                    index = 0;
                    scan = g_oct_build_scratch;
                    while (index < g_oct_build_count) {
                        if (*scan == surface) {
                            goto next_link_7;
                        }
                        ++index;
                        ++scan;
                    }
                    g_oct_build_scratch[g_oct_build_count] = surface;
                    ++g_oct_build_count;
                    ++collected;
                next_link_7:;
                }
            }
            second = 0;
            /* Retail's kind-10 path dereferences a link head at +0x2c, beyond
               the eight-slot union member — in the proven 0x30-byte node that
               is the provisional_region/positional ushort pair, which
               OctBuildPreTree writes as the leaf's provisional region index
               (node->provisional_region = node->region; FinalizeRegionMapping
               reads it back as a ushort). No producer appends at a kind above
               4, so the read is of ushort region-index storage; retained as
               the observed retail read of dead code. */
            // reinterpret-ok: dead kind-10 path reads the proven ushort region-index pair at +0x2c as a link head
            for (link = *reinterpret_cast<W8OctBuildLink**>(&node->provisional_region);
                 link != 0; link = link->next) {
                if (CollectSurfacePredicate(static_cast<W8GDSurface*>(link->surface), 0xb) !=
                    0) {
                    g_oct_build_scratch[g_oct_build_count] =
                        static_cast<W8GDSurface*>(link->surface);
                    ++g_oct_build_count;
                    ++second;
                }
            }
            return second + collected;
        }
        for (link = node->links[kind]; link != 0; link = link->next) {
            if (CollectSurfacePredicate(static_cast<W8GDSurface*>(link->surface), kind) != 0) {
                g_oct_build_scratch[g_oct_build_count] =
                    static_cast<W8GDSurface*>(link->surface);
                ++g_oct_build_count;
                ++collected;
            }
        }
        return collected;
    }
    for (int child = 0; child != 8; ++child) {
        if (node->children[child] != 0) {
            collected += CollectLeaf(node->children[child], depth + 1, kind);
        }
    }
    return collected;
}

/* Corner-based box classification: 2 when every box corner sits inside
   bounds, 1 on partial overlap (a box corner inside bounds, or when no box
   corner is inside, a bounds corner inside the box), 0 when disjoint. `leaf`
   stops the scan on the first inside corner at the bottom octree level. */
// FUNCTION: WIZ8 0x00447310
int W8OctBuildTree::ClassifyBoxBounds(const srVector3T<float>* box, const srVector3T<float>* bounds,
                                      bool leaf)
{
    short x;
    short y;
    short z;
    bool all_inside = true;
    int inside = 0;

    for (x = 0; x < 2; ++x) {
        for (y = 0; y < 2; ++y) {
            for (z = 0; z < 2; ++z) {
                float corner_x = box[x].x;
                float corner_y = box[y].y;
                float corner_z = box[z].z;
                if (corner_x < bounds[0].x || corner_x >= bounds[1].x || corner_y < bounds[0].y ||
                    corner_y >= bounds[1].y || corner_z < bounds[0].z || corner_z >= bounds[1].z) {
                    all_inside = false;
                    if (inside != 0) {
                        x = y = z = 2;
                    }
                } else {
                    inside = 1;
                    if (!all_inside || leaf) {
                        x = y = z = 2;
                    }
                }
            }
        }
    }
    if (all_inside) {
        return 2;
    }
    if (inside == 0) {
        for (x = 0; x < 2; ++x) {
            for (y = 0; y < 2; ++y) {
                for (z = 0; z < 2; ++z) {
                    float corner_x = bounds[x].x;
                    float corner_y = bounds[y].y;
                    float corner_z = bounds[z].z;
                    if (corner_x >= box[0].x && corner_x <= box[1].x && corner_y >= box[0].y &&
                        corner_y <= box[1].y && corner_z >= box[0].z && corner_z <= box[1].z) {
                        inside = 1;
                        x = y = z = 2;
                    }
                }
            }
        }
    }
    return inside;
}

/* Per-surface collector predicate: kind-3 walks mark each surface with the
   0x2000 visit flag; other kinds dedup-scan the scratch buffer. Returns
   nonzero when the surface should be appended. */
// FUNCTION: WIZ8 0x004474c0
char CollectSurfacePredicate(W8GDSurface* surface, short kind)
{
    bool result = false;
    if (kind != 3) {
        if (g_oct_build_count != 0) {
            for (unsigned long index = 0; index < g_oct_build_count; ++index) {
                if (surface == g_oct_build_scratch[index]) {
                    return 0;
                }
            }
            return 1;
        }
        result = true;
    } else {
        if ((surface->flags & 0x2000) == 0) {
            surface->flags |= 0x2000;
            result = true;
        }
    }
    return result;
}
