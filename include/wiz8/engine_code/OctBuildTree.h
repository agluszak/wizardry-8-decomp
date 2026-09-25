#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_TREE_H

#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/geometry.h"

struct W8OctBuildLink {
    void* surface_00; /* region polygons and GD surfaces share the build links */
    W8OctBuildLink* next_04;
};

class OctPreTree;

struct W8OctBuildLinkLists {
    W8OctBuildLinkLists();
    W8OctBuildLink* GetNewLink(void* surface);

    unsigned short m_usCurrent;
    unsigned short padding_02;
    W8OctBuildLink* m_apLinkLists[100];
    unsigned short m_ausLinkCounts[100];
};

struct W8OctBuildNode {
    W8OctBuildNode();
    ~W8OctBuildNode();

    unsigned char RearrangeNodePolys004AF7B0(short current_depth, short target_depth);
    int CollectLinkedSurfaces(short current_depth, short target_depth, short mode);
    int CollectSurfaceArray(short mode);
    unsigned long ConvertToOctPreTree(unsigned short depth, OctPreTree* tree);

    union {
        W8OctBuildNode* children_00[8];
        W8OctBuildLink* links_00[8];
        void** surface_arrays_00[8]; /* elements follow the insert mode */
        unsigned short* region_arrays_00[8];
    };
    unsigned long padding_20;
    unsigned long padding_24;
    unsigned short region_28;
    unsigned short leaf_kind_2a;
    unsigned short provisional_region_2c;
    unsigned short positional_2e;
};

/* A zero-storage node variant with independently evidenced behavior: its
   constructor at 0x004AF760 increments the live-node counter after invoking
   the ordinary node constructor.  Its original spelling and source owner are
   still unresolved. */
struct W8CountedOctBuildNode : W8OctBuildNode {
    W8CountedOctBuildNode();
    ~W8CountedOctBuildNode();
};

/* Original owner: Engine Code\OctBuildTree.cpp.  The source path proves the
   build-tree unit, while the address suffix keeps the still-unrecovered class
   spelling explicit.  Non-polymorphic like runtime W8Octree: neither the
   constructor at 0x00446390 nor the destructor at 0x004466D0 stores a vptr;
   the vtables emitted near this TU belong to vector/template material, not to
   this type. */
struct W8OctBuildTree {
    W8OctBuildTree(float leaf_size, srVector3T<float>* minimum, srVector3T<float>* maximum,
                   unsigned short item_limit, short extent_mode);
    ~W8OctBuildTree();

    unsigned char InsertSurface00446820(W8GDSurface* surface, unsigned long mode);
    unsigned char InsertSurfaceRecursive004469F0(W8OctSpatialState* working, W8GDSurface* surface,
                                                 srVector3T<float>* plane_point,
                                                 unsigned long mode);
    int CollectObjectsAlongSegment00446D80(int** results, const srVector3T<float>* from,
                                           const srVector3T<float>* to, float half_angle,
                                           float extent, unsigned short kind);
    /* Append `payload` to the node's `kind` link list, growing the tail and
       raising the leaf counter plus the tree's deepest-list watermark. */
    void AppendLink(W8OctBuildNode* node, void* payload, short kind);
    /* Recursive box descent for the segment collect: classify the state's box
       against `bounds` (six floats: min then max), then collect the leaf,
       descend the octants, or skip the node entirely. */
    int CollectRecursive(W8OctSpatialState* state, const float* bounds, short kind);
    /* Leaf collector: walks the per-kind link lists and appends qualifying
       surfaces to the shared scratch array, deduplicating by pointer or by
       the 0x2000 flag mark. */
    int CollectLeaf(W8OctBuildNode* node, short depth, short kind);
    /* Box-vs-bounds classification: 2 when the box sits fully inside bounds,
       1 on a partial overlap, 0 when disjoint. `leaf` early-outs the corner
       scan at the bottom octree level. */
    int ClassifyBoxBounds(const float* box, const float* bounds, char leaf);

    W8OctSpatialState spatial_00;
    W8OctBuildLinkLists* link_lists_9c;
    unsigned long leaf_polygon_count_a0;
    unsigned long gd_surface_count_a4;
    unsigned long leaf_count_a8;
    unsigned short max_leaf_regions_ac;
    unsigned short padding_ae;
    unsigned long region_assignments_b0;
    unsigned char use_owned_nodes_b4;
    unsigned char padding_b5[3];
    unsigned long deepest_link_list_b8;
};

static_assert(sizeof(W8OctBuildLink) == 8, "W8OctBuildLink_must_be_8");
static_assert(sizeof(W8OctBuildLinkLists) == 0x25c, "W8OctBuildLinkLists_must_be_0x25c");
static_assert(sizeof(W8OctBuildNode) == 0x30, "W8OctBuildNode00446330_must_be_0x30");
static_assert(sizeof(W8CountedOctBuildNode) == 0x30, "W8CountedOctBuildNode004AF760_must_be_0x30");
static_assert(sizeof(W8OctBuildTree) == 0xbc, "W8OctBuildTree_must_be_0xbc");

extern float g_float_005ec188;
extern void* g_oct_build_scratch_00659a48;

#endif
