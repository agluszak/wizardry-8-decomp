#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>

#define OCT_BUILD_TREE_CPP \
    "C:\\Projects\\Wizardry 8\\Engine Code\\OctBuildTree.cpp"

extern float g_path_waypoint_exact_distance_005ebc64;
extern float g_float_005ec188;
extern void Function497690(int channel, const char* message);
extern void* g_oct_build_scratch_00659a48;
extern unsigned char TestSpatialTriangle0046CE60(
    const srVector3T<float>* tree_minimum,
    const srVector3T<float>* vertices,
    const srVector3T<float>* plane_point);

W8OctBuildLinkLists::W8OctBuildLinkLists()
    : m_usCurrent(0), padding_02(0)
{
    for (int index = 0; index != 100; ++index) {
        m_apLinkLists[index] = 0;
        m_ausLinkCounts[index] = 0;
    }
}

/* The original member names come from this body's assertion.  Each bank owns
   50,000 eight-byte links and a fresh link records both caller values. */
// FUNCTION: WIZ8 0x00446250
W8OctBuildLink* W8OctBuildLinkLists::GetNewLink00446250(
    unsigned long first, unsigned long second)
{
    if (m_ausLinkCounts[m_usCurrent] > 49999) {
        ++m_usCurrent;
    }
    if (m_usCurrent < 100) {
        if (m_apLinkLists[m_usCurrent] == 0) {
            m_apLinkLists[m_usCurrent] = static_cast<W8OctBuildLink*>(
                malloc(50000 * sizeof(W8OctBuildLink)));
            if (m_apLinkLists[m_usCurrent] == 0) {
                srAssertFail(
                    "m_apLinkLists[m_usCurrent]", OCT_BUILD_TREE_CPP, 140,
                    "GetNewLink: Couldn't allocate m_apLinkLists.");
            }
            memset(
                m_apLinkLists[m_usCurrent], 0,
                50000 * sizeof(W8OctBuildLink));
        }
        W8OctBuildLink* link =
            &m_apLinkLists[m_usCurrent][m_ausLinkCounts[m_usCurrent]++];
        link->value_00 = second;
        link->value_04 = first;
        return link;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00446330
W8OctBuildNode00446330::W8OctBuildNode00446330()
{
    memset(this, 0, sizeof(*this));
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
W8OctBuildTree00446390::W8OctBuildTree00446390(
    float leaf_size,
    srVector3T<float>* minimum,
    srVector3T<float>* maximum,
    unsigned short item_limit,
    short extent_mode)
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

    if (leaf_size < g_path_waypoint_exact_distance_005ebc64) {
        Function497690(7, "Leaf Size too small--try a larger leaf size!");
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
            while (leaf_size + leaf_size <= spatial_00.node_extent_70 &&
                   spatial_00.depth_44 < 6) {
                spatial_00.node_extent_70 *= g_float_005ebc7c;
                ++spatial_00.depth_44;
            }
        }
        else if (extent_mode == 1) {
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
                Function497690(
                    7, "Leaf Size too small--try a larger leaf size!");
            }
            spatial_00.extent_04 = spatial_00.cell_size_08;
        }
        else {
            spatial_00.node_extent_70 = spatial_00.extent_04;
            while (leaf_size + leaf_size <= spatial_00.node_extent_70 &&
                   spatial_00.depth_44 < 6) {
                spatial_00.node_extent_70 *= g_float_005ebc7c;
                ++spatial_00.depth_44;
            }
            if (extent_mode == 2) {
                float doubled =
                    spatial_00.node_extent_70 + spatial_00.node_extent_70;
                if (doubled - leaf_size <
                    leaf_size - spatial_00.node_extent_70) {
                    --spatial_00.depth_44;
                    spatial_00.node_extent_70 = doubled;
                }
            }
        }

        spatial_00.cell_size_08 =
            spatial_00.node_extent_70 * g_float_005ec188;
        spatial_00.maximum_18.x =
            minimum->x + spatial_00.extent_04;
        spatial_00.maximum_18.y =
            minimum->y + spatial_00.extent_04;
        spatial_00.maximum_18.z =
            minimum->z + spatial_00.extent_04;
        g_oct_build_scratch_00659a48 = malloc(40000);
        spatial_00.state_3c = 1;
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
    delete static_cast<W8OctBuildNode00446330*>(spatial_00.root_90);
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
unsigned char W8OctBuildTree00446390::InsertSurface00446820(
    W8GDSurface* surface, unsigned long mode)
{
    W8OctSpatialState0046CCC0 working(&spatial_00);
    srVector3T<float> vertices[3];
    srVector3T<float> plane_point;
    srVector3T<float>* plane = &plane_point;

    if ((short)mode == 3) {
        if (LoadSurfaceVertices004214D0(
                vertices, surface->vertex_indices_18) == 0) {
            plane = 0;
        }
        else {
            plane_point.x = surface->plane_24[0];
            plane_point.y = surface->plane_24[1];
            plane_point.z = surface->plane_24[2];
        }
    }
    if (TestSpatialTriangle0046CE60(
            &spatial_00.minimum_0c, vertices, plane) == 0) {
        return 0;
    }

    if (spatial_00.root_90 == 0) {
        if (use_owned_nodes_b4 == 0) {
            spatial_00.root_90 = new W8OctBuildNode00446330;
        }
        else {
            spatial_00.root_90 = new W8CountedOctBuildNode004AF760;
        }
    }
    working.root_90 = spatial_00.root_90;
    working.depth_44 = 0;
    working.level_kind_6c = 1;
    if (InsertSurfaceRecursive004469F0(
            &working, surface, &plane_point, mode) == 0) {
        return 0;
    }
    ++spatial_00.item_count_40;
    return 1;
}
