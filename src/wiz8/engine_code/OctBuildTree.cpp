#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/materials.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define OCT_BUILD_TREE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildTree.cpp"

// GLOBAL: WIZ8 0x005ec188
float g_float_005ec188 = 1.000100016593933f;

// GLOBAL: WIZ8 0x00659a48
void* g_oct_build_scratch_00659a48;
/* The running count of surfaces appended into the scratch buffer by the
   leaf collector. Saved and restored around nested collects. */
// GLOBAL: WIZ8 0x00659a38
unsigned long g_oct_build_count_00659a38;
/* The caller's result slot during a segment collect; retail writes it but
   no recovered reader exists. */
// GLOBAL: WIZ8 0x00659a44
void* g_oct_build_out_00659a44;

char CollectSurfacePredicate004474C0(W8GDSurface* surface, short kind);

W8OctBuildLinkLists::W8OctBuildLinkLists() : m_usCurrent(0), padding_02(0)
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
        link->surface_00 = surface;
        return link;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00446330
W8OctBuildNode00446330::W8OctBuildNode00446330()
{
    memset(this, 0, 10 * sizeof(unsigned long));
    positional_28 = 0;
    leaf_kind_2a = 0;
    positional_2c = 0;
}

// FUNCTION: WIZ8 0x00446350
W8OctBuildNode00446330::~W8OctBuildNode00446330()
{
    if (leaf_kind_2a != 0) {
        memset(this, 0, 10 * sizeof(unsigned long));
        return;
    }
    for (int child = 0; child != 8; ++child) {
        delete children_00[child];
    }
}

/* Build the cubic spatial domain used to insert processed GameData surfaces.
   The caller supplies local copies of the level bounds because this constructor
   deliberately expands them by half a leaf on every axis. */
// FUNCTION: WIZ8 0x00446390
W8OctBuildTree00446390::W8OctBuildTree00446390(float leaf_size, srVector3T<float>* minimum,
                                               srVector3T<float>* maximum,
                                               unsigned short item_limit, short extent_mode)
    : spatial_00(0)
{
    spatial_00.Reset0046CDC0();
    link_lists_9c = 0;
    positional_a0 = 0;
    positional_a4 = 0;
    positional_a8 = 0;
    positional_ac = 0;
    padding_ae = 0;
    positional_b0 = 0;
    use_owned_nodes_b4 = 0;
    padding_b5[0] = 0;
    padding_b5[1] = 0;
    padding_b5[2] = 0;
    positional_b8 = 0;

    if (leaf_size < g_float_005ebc64) {
        ReportBuildStatus00497690(7, "Leaf Size too small--try a larger leaf size!");
    }
    spatial_00.flags_00 = 0;
    spatial_00.item_limit_48 = item_limit;
    spatial_00.level_kind_6c = 2;
    spatial_00.extent_04 = 0.0f;
    spatial_00.root_90 = 0;
    spatial_00.owned_98 = 0;
    spatial_00.depth_44 = 0;

    if (minimum != 0 || maximum != 0) {
        float half_leaf = leaf_size * g_float_005ebc7c;
        float* source_minimum = &minimum->x;
        float* source_maximum = &maximum->x;
        float* stored_minimum = &spatial_00.clipped_minimum_24.x;
        for (int axis = 0; axis != 3; ++axis) {
            source_minimum[axis] -= half_leaf;
            source_maximum[axis] += half_leaf;
            (&spatial_00.minimum_0c.x)[axis] = source_minimum[axis];
            stored_minimum[axis] = source_minimum[axis];
            (&spatial_00.working_minimum_78.x)[axis] = source_minimum[axis];
            (&spatial_00.clipped_maximum_30.x)[axis] = source_maximum[axis];
            (&spatial_00.working_maximum_84.x)[axis] = source_maximum[axis];
            float span = source_maximum[axis] - source_minimum[axis];
            if (spatial_00.extent_04 < span) {
                spatial_00.extent_04 = span;
            }
        }

        spatial_00.depth_44 = 0;
        if (extent_mode == 0) {
            spatial_00.node_extent_70 = spatial_00.extent_04;
            while (leaf_size + leaf_size <= spatial_00.node_extent_70 && spatial_00.depth_44 < 6) {
                spatial_00.node_extent_70 *= g_float_005ebc7c;
                ++spatial_00.depth_44;
            }
        } else if (extent_mode == 1) {
            spatial_00.node_extent_70 = leaf_size;
            spatial_00.cell_size_08 = leaf_size;
            while (spatial_00.cell_size_08 < spatial_00.extent_04) {
                if (spatial_00.depth_44 > 6) {
                    break;
                }
                spatial_00.cell_size_08 += spatial_00.cell_size_08;
                ++spatial_00.depth_44;
            }
            if (spatial_00.depth_44 > 6) {
                ReportBuildStatus00497690(7, "Leaf Size too small--try a larger leaf size!");
            }
            spatial_00.extent_04 = spatial_00.cell_size_08;
        } else {
            spatial_00.node_extent_70 = spatial_00.extent_04;
            while (leaf_size + leaf_size <= spatial_00.node_extent_70 && spatial_00.depth_44 < 6) {
                spatial_00.node_extent_70 *= g_float_005ebc7c;
                ++spatial_00.depth_44;
            }
            if (extent_mode == 2) {
                float doubled = spatial_00.node_extent_70 + spatial_00.node_extent_70;
                if (doubled - leaf_size < leaf_size - spatial_00.node_extent_70) {
                    --spatial_00.depth_44;
                    spatial_00.node_extent_70 = doubled;
                }
            }
        }

        spatial_00.cell_size_08 = spatial_00.node_extent_70 * g_float_005ec188;
        spatial_00.maximum_18.x = minimum->x + spatial_00.extent_04;
        spatial_00.maximum_18.y = minimum->y + spatial_00.extent_04;
        spatial_00.maximum_18.z = minimum->z + spatial_00.extent_04;
        g_oct_build_scratch_00659a48 = malloc(40000);
        spatial_00.polygon_count_3c = 1;
        spatial_00.item_count_40 = 0;
        spatial_00.root_90 = 0;
        spatial_00.owned_98 = 0;
        link_lists_9c = new W8OctBuildLinkLists;
    }
}

/* Release the recursive node tree, the global construction scratch buffer and
   every allocated link bank.  The first-member spatial value performs its own
   shallow teardown after this body returns. */
// FUNCTION: WIZ8 0x004466d0
W8OctBuildTree00446390::~W8OctBuildTree00446390()
{
    delete spatial_00.root_90;
    if (g_oct_build_scratch_00659a48 != 0) {
        free(g_oct_build_scratch_00659a48);
    }
    g_oct_build_scratch_00659a48 = 0;

    spatial_00.owned_98 = 0;
    if (link_lists_9c != 0) {
        for (int index = 0; index != 100; ++index) {
            if (link_lists_9c->m_apLinkLists[index] != 0) {
                free(link_lists_9c->m_apLinkLists[index]);
            }
            link_lists_9c->m_apLinkLists[index] = 0;
            link_lists_9c->m_ausLinkCounts[index] = 0;
        }
        delete link_lists_9c;
    }
}

/* Reject triangles outside the build domain, lazily create the root node, and
   then hand the complete typed working record to the recursive inserter. */
// FUNCTION: WIZ8 0x00446820
unsigned char W8OctBuildTree00446390::InsertSurface00446820(W8GDSurface* surface,
                                                            unsigned long mode)
{
    W8OctSpatialState working(&spatial_00);
    srVector3T<float> vertices[3];
    srVector3T<float> plane_point;
    srVector3T<float>* plane = &plane_point;

    if ((short)mode == 3) {
        if (LoadSurfaceVertices004214D0(vertices, surface->vertex_indices_18) == 0) {
            plane = 0;
        } else {
            plane_point.x = surface->plane_24[0];
            plane_point.y = surface->plane_24[1];
            plane_point.z = surface->plane_24[2];
        }
    }
    if (TestSpatialTriangle0046CE60(&spatial_00.minimum_0c, vertices, plane) == 0) {
        return 0;
    }

    if (spatial_00.root_90 == 0) {
        if (use_owned_nodes_b4 == 0) {
            spatial_00.root_90 = new W8OctBuildNode00446330;
        } else {
            spatial_00.root_90 = new W8CountedOctBuildNode004AF760;
        }
    }
    working.root_90 = spatial_00.root_90;
    working.owned_98 = vertices;
    working.depth_44 = 0;
    working.level_kind_6c = 1;
    if (InsertSurfaceRecursive004469F0(&working, surface, &plane_point, mode) == 0) {
        return 0;
    }
    ++spatial_00.item_count_40;
    return 1;
}

/* Descend through every overlapping octant.  Branch nodes own child nodes;
   leaf nodes reuse the same eight slots as per-mode linked-list heads. */
// FUNCTION: WIZ8 0x004469f0
unsigned char W8OctBuildTree00446390::InsertSurfaceRecursive004469F0(W8OctSpatialState* working,
                                                                     W8GDSurface* surface,
                                                                     srVector3T<float>* plane_point,
                                                                     unsigned long mode)
{
    W8OctSpatialState child(working);
    unsigned char inserted = 0;

    if (spatial_00.depth_44 < working->depth_44) {
        spatial_00.depth_44 = working->depth_44;
    }

    if (working->extent_04 <= working->cell_size_08) {
        W8OctBuildNode00446330* node = working->root_90;
        ++node->leaf_kind_2a;
        if (positional_b8 < node->leaf_kind_2a) {
            positional_b8 = node->leaf_kind_2a;
        }

        W8OctBuildLink*& head = node->links_00[(short)mode];
        if (head == 0) {
            head = link_lists_9c->GetNewLink(surface);
        } else {
            W8OctBuildLink* tail = head;
            while (tail->next_04 != 0) {
                tail = tail->next_04;
            }
            tail->next_04 = link_lists_9c->GetNewLink(surface);
        }
        inserted = 1;
    } else {
        float half_extent = working->extent_04 * g_float_005ebc7c;
        child.cell_size_08 = working->cell_size_08;
        child.depth_44 = working->depth_44 + 1;

        short octant = 0;
        for (int x = 0; x != 2; ++x) {
            for (int y = 0; y != 2; ++y) {
                for (int z = 0; z != 2; ++z, ++octant) {
                    child.minimum_0c.x = working->minimum_0c.x + x * half_extent;
                    child.maximum_18.x = child.minimum_0c.x + half_extent;
                    child.minimum_0c.y = working->minimum_0c.y + y * half_extent;
                    child.maximum_18.y = child.minimum_0c.y + half_extent;
                    child.minimum_0c.z = working->minimum_0c.z + z * half_extent;
                    child.maximum_18.z = child.minimum_0c.z + half_extent;

                    if (TestSpatialTriangle0046CE60(&child.minimum_0c, working->owned_98,
                                                    plane_point) != 0) {
                        W8OctBuildNode00446330* node = working->root_90;
                        if (node->children_00[octant] == 0) {
                            if (use_owned_nodes_b4 == 0) {
                                node->children_00[octant] = new W8OctBuildNode00446330;
                            } else {
                                node->children_00[octant] = new W8CountedOctBuildNode004AF760;
                            }
                        }
                        child.root_90 = node->children_00[octant];
                        child.owned_98 = working->owned_98;
                        if (InsertSurfaceRecursive004469F0(&child, surface, plane_point, mode) !=
                            0) {
                            inserted = 1;
                        }
                    }
                }
            }
        }
    }
    return inserted;
}

/* Append `payload` to the node's `kind` link list: bump the leaf counter and
   the tree watermark, then either extend the tail or seed the head. The same
   body is inlined inside InsertSurfaceRecursive's leaf path. */
// FUNCTION: WIZ8 0x00446d00
void W8OctBuildTree00446390::AppendLink00446D00(W8OctBuildNode00446330* node, void* payload,
                                                short kind)
{
    ++node->leaf_kind_2a;
    if (positional_b8 < node->leaf_kind_2a) {
        positional_b8 = node->leaf_kind_2a;
    }
    W8OctBuildLink* head = node->links_00[kind];
    if (head == 0) {
        node->links_00[kind] = link_lists_9c->GetNewLink(payload);
        return;
    }
    W8OctBuildLink* next;
    for (next = head->next_04; next != 0; next = next->next_04) {
        head = next;
    }
    head->next_04 = link_lists_9c->GetNewLink(payload);
}

/* Segment query over the build tree: seed the caller's result array with the
   shared scratch buffer, expand the segment's axis bounds by `extent`, then
   walk the tree. `half_angle` is unused. Collected surfaces carry the 0x2000
   visit mark, which this clears before returning the count. */
// FUNCTION: WIZ8 0x00446d80
int W8OctBuildTree00446390::CollectObjectsAlongSegment00446D80(int** results,
                                                               const srVector3T<float>* from,
                                                               const srVector3T<float>* to,
                                                               float half_angle, float extent,
                                                               unsigned short kind)
{
    W8OctSpatialState state(&spatial_00);
    float bounds[6];
    unsigned long saved = 0;
    unsigned int index;

    if (*results == 0) {
        *results = static_cast<int*>(g_oct_build_scratch_00659a48);
        g_oct_build_out_00659a44 = g_oct_build_scratch_00659a48;
    } else {
        saved = g_oct_build_count_00659a38;
        g_oct_build_out_00659a44 = results;
    }
    g_oct_build_count_00659a38 = 0;

    float length = to->Length();
    if (length > extent) {
        extent = length;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if ((&to->x)[axis] > g_float_005ebb34) {
            bounds[axis] = (&from->x)[axis] - extent;
            bounds[axis + 3] = extent + (&from->x)[axis] + (&to->x)[axis];
        } else {
            bounds[axis] = (&from->x)[axis] - extent + (&to->x)[axis];
            bounds[axis + 3] = extent + (&from->x)[axis];
        }
    }
    state.depth_44 = 0;
    state.level_kind_6c = 1;
    CollectRecursive00446F20(&state, bounds, static_cast<short>(kind));

    int count;
    if (saved != 0) {
        count = g_oct_build_count_00659a38;
        g_oct_build_count_00659a38 = saved;
    } else {
        count = g_oct_build_count_00659a38;
    }
    index = 0;
    if (count != 0) {
        do {
            W8GDSurface* surface = reinterpret_cast< // reinterpret-ok: scratch slot stores surface*
                W8GDSurface**>(*results)[index];
            ++index;
            surface->flags_00 &= ~0x2000;
        } while (index < static_cast<unsigned int>(count));
    }
    return count;
}

/* Recursive descent: classify the state's box against the query bounds, then
   collect the whole leaf subtree (2), walk the eight octants (1), or skip
   (0). A state already at the bottom level collects as a leaf either way. */
// FUNCTION: WIZ8 0x00446f20
int W8OctBuildTree00446390::CollectRecursive00446F20(W8OctSpatialState* state, const float* bounds,
                                                     short kind)
{
    W8OctSpatialState child(state);
    int collected = 0;
    char leaf;
    float box[6];

    leaf = 0;
    if (state->depth_44 == spatial_00.depth_44) {
        leaf = 1;
    }
    for (int axis = 0; axis < 3; ++axis) {
        box[axis] = (&state->minimum_0c.x)[axis];
        box[axis + 3] = (&state->maximum_18.x)[axis];
    }
    int verdict = ClassifyBoxBounds00447310(box, bounds, leaf);
    if (verdict == 2 || (verdict == 1 && leaf != 0)) {
        collected = CollectLeaf00447110(state->root_90, state->depth_44, kind);
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
                    W8OctBuildNode00446330* node = state->root_90;
                    if (node->children_00[octant] != 0) {
                        child.minimum_0c.x = x * child.extent_04 + state->minimum_0c.x;
                        child.maximum_18.x = child.minimum_0c.x + child.extent_04;
                        child.minimum_0c.y = y * child.extent_04 + state->minimum_0c.y;
                        child.maximum_18.y = child.minimum_0c.y + child.extent_04;
                        child.minimum_0c.z = z * child.extent_04 + state->minimum_0c.z;
                        child.maximum_18.z = child.minimum_0c.z + child.extent_04;
                        child.root_90 = node->children_00[octant];
                        collected += CollectRecursive00446F20(&child, bounds, kind);
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
int W8OctBuildTree00446390::CollectLeaf00447110(W8OctBuildNode00446330* node, short depth,
                                                short kind)
{
    W8OctBuildLink* link;
    W8GDSurface* surface;
    W8GDSurface** scan;
    unsigned long index;
    int collected = 0;
    int second;

    if (depth == spatial_00.depth_44) {
        if (kind == 9) {
            link = node->links_00[3];
            while (link != 0) {
                surface = static_cast<W8GDSurface*>(link->surface_00);
                if ((surface->flags_00 & 0x2000) == 0) {
                    surface->flags_00 |= 0x2000;
                    static_cast<W8GDSurface**>(
                        g_oct_build_scratch_00659a48)[g_oct_build_count_00659a38] =
                        static_cast<W8GDSurface*>(link->surface_00);
                    ++g_oct_build_count_00659a38;
                    ++collected;
                }
                link = link->next_04;
            }
            second = 0;
            if (node->links_00[4] != 0) {
                for (link = node->links_00[4]; link != 0; link = link->next_04) {
                    surface = static_cast<W8GDSurface*>(link->surface_00);
                    index = 0;
                    scan = static_cast<W8GDSurface**>(g_oct_build_scratch_00659a48);
                    while (index < g_oct_build_count_00659a38) {
                        if (*scan == surface) {
                            goto next_link_4;
                        }
                        ++index;
                        ++scan;
                    }
                    static_cast<W8GDSurface**>(
                        g_oct_build_scratch_00659a48)[g_oct_build_count_00659a38] = surface;
                    ++g_oct_build_count_00659a38;
                    ++second;
                next_link_4:;
                }
            }
            return collected + second;
        }
        if (kind == 10) {
            if (node->links_00[7] != 0) {
                for (link = node->links_00[7]; link != 0; link = link->next_04) {
                    surface = static_cast<W8GDSurface*>(link->surface_00);
                    index = 0;
                    scan = static_cast<W8GDSurface**>(g_oct_build_scratch_00659a48);
                    while (index < g_oct_build_count_00659a38) {
                        if (*scan == surface) {
                            goto next_link_7;
                        }
                        ++index;
                        ++scan;
                    }
                    static_cast<W8GDSurface**>(
                        g_oct_build_scratch_00659a48)[g_oct_build_count_00659a38] = surface;
                    ++g_oct_build_count_00659a38;
                    ++collected;
                next_link_7:;
                }
            }
            second = 0;
            /* Retail's kind-10 path dereferences a link head at +0x2c, beyond
               the eight-slot union member — in the proven 0x30-byte node that
               is the positional_2c/positional_2e ushort pair, which
               OctBuildPreTree writes as the leaf's provisional region index
               (node->positional_2c = node->positional_28; FinalizeRegionMapping
               reads it back as a ushort). No producer appends at a kind above
               4, so the read is of ushort region-index storage; retained as
               the observed retail read of dead code. */
            // reinterpret-ok: dead kind-10 path reads the proven ushort region-index pair at +0x2c as a link head
            for (link = *reinterpret_cast<W8OctBuildLink**>(&node->positional_2c); link != 0;
                 link = link->next_04) {
                if (CollectSurfacePredicate004474C0(static_cast<W8GDSurface*>(link->surface_00),
                                                    0xb) != 0) {
                    static_cast<W8GDSurface**>(
                        g_oct_build_scratch_00659a48)[g_oct_build_count_00659a38] =
                        static_cast<W8GDSurface*>(link->surface_00);
                    ++g_oct_build_count_00659a38;
                    ++second;
                }
            }
            return second + collected;
        }
        for (link = node->links_00[kind]; link != 0; link = link->next_04) {
            if (CollectSurfacePredicate004474C0(static_cast<W8GDSurface*>(link->surface_00),
                                                kind) != 0) {
                static_cast<W8GDSurface**>(
                    g_oct_build_scratch_00659a48)[g_oct_build_count_00659a38] =
                    static_cast<W8GDSurface*>(link->surface_00);
                ++g_oct_build_count_00659a38;
                ++collected;
            }
        }
        return collected;
    }
    for (int child = 0; child != 8; ++child) {
        if (node->children_00[child] != 0) {
            collected += CollectLeaf00447110(node->children_00[child], depth + 1, kind);
        }
    }
    return collected;
}

/* Corner-based box classification: 2 when every box corner sits inside
   bounds, 1 on partial overlap (a box corner inside bounds, or when no box
   corner is inside, a bounds corner inside the box), 0 when disjoint. `leaf`
   stops the scan on the first inside corner at the bottom octree level. */
// FUNCTION: WIZ8 0x00447310
int W8OctBuildTree00446390::ClassifyBoxBounds00447310(const float* box, const float* bounds,
                                                      char leaf)
{
    short x;
    short y;
    short z;
    bool all_inside = true;
    int inside = 0;

    for (x = 0; x < 2; ++x) {
        for (y = 0; y < 2; ++y) {
            for (z = 0; z < 2; ++z) {
                float corner_x = box[x * 3];
                float corner_y = box[y * 3 + 1];
                float corner_z = box[z * 3 + 2];
                if (corner_x < bounds[0] || corner_x >= bounds[3] || corner_y < bounds[1] ||
                    corner_y >= bounds[4] || corner_z < bounds[2] || corner_z >= bounds[5]) {
                    all_inside = 0;
                    if (inside != 0) {
                        x = y = z = 2;
                    }
                } else {
                    inside = 1;
                    if (all_inside == 0 || leaf != 0) {
                        x = y = z = 2;
                    }
                }
            }
        }
    }
    if (all_inside != 0) {
        return 2;
    }
    if (inside == 0) {
        for (x = 0; x < 2; ++x) {
            for (y = 0; y < 2; ++y) {
                for (z = 0; z < 2; ++z) {
                    float corner_x = bounds[x * 3];
                    float corner_y = bounds[y * 3 + 1];
                    float corner_z = bounds[z * 3 + 2];
                    if (corner_x >= box[0] && corner_x <= box[3] && corner_y >= box[1] &&
                        corner_y <= box[4] && corner_z >= box[2] && corner_z <= box[5]) {
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
char CollectSurfacePredicate004474C0(W8GDSurface* surface, short kind)
{
    char result = 0;
    if (kind != 3) {
        if (g_oct_build_count_00659a38 != 0) {
            for (unsigned long index = 0; index < g_oct_build_count_00659a38; ++index) {
                if (surface == static_cast<W8GDSurface**>(g_oct_build_scratch_00659a48)[index]) {
                    return 0;
                }
            }
            return 1;
        }
        result = 1;
    } else {
        if ((surface->flags_00 & 0x2000) == 0) {
            surface->flags_00 |= 0x2000;
            result = 1;
        }
    }
    return result;
}
