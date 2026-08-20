#ifndef WIZ8_ENGINE_CODE_OCT_BUILD_TREE_H
#define WIZ8_ENGINE_CODE_OCT_BUILD_TREE_H

#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/geometry.h"

struct W8OctBuildLink {
    unsigned long value_00;
    unsigned long value_04;
};

struct W8OctBuildLinkLists {
    W8OctBuildLinkLists();
    W8OctBuildLink* GetNewLink00446250(
        unsigned long first, unsigned long second);

    unsigned short m_usCurrent;
    unsigned short padding_02;
    W8OctBuildLink* m_apLinkLists[100];
    unsigned short m_ausLinkCounts[100];
};

struct W8OctBuildNode00446330 {
    W8OctBuildNode00446330();
    ~W8OctBuildNode00446330();

    W8OctBuildNode00446330* children_00[8];
    unsigned long positional_20;
    unsigned long positional_24;
    unsigned short positional_28;
    unsigned short leaf_kind_2a;
    unsigned short positional_2c;
    unsigned short positional_2e;
};

/* A zero-storage node variant with independently evidenced behavior: its
   constructor at 0x004AF760 increments the live-node counter after invoking
   the ordinary node constructor.  Its original spelling and source owner are
   still unresolved. */
struct W8CountedOctBuildNode004AF760 : W8OctBuildNode00446330 {
    W8CountedOctBuildNode004AF760();
};

/* Original owner: Engine Code\OctBuildTree.cpp.  The source path proves the
   build-tree unit, while the address suffix keeps the still-unrecovered class
   spelling explicit. */
struct W8OctBuildTree00446390 {
    W8OctBuildTree00446390(
        float leaf_size,
        srVector3T<float>* minimum,
        srVector3T<float>* maximum,
        unsigned short item_limit,
        short extent_mode);
    ~W8OctBuildTree00446390();

    unsigned char InsertSurface00446820(
        W8GDSurface* surface, unsigned long mode);
    unsigned char InsertSurfaceRecursive004469F0(
        W8OctSpatialState0046CCC0* working,
        W8GDSurface* surface,
        srVector3T<float>* plane_point,
        unsigned long mode);

    W8OctSpatialState0046CCC0 spatial_00;
    W8OctBuildLinkLists* link_lists_9c;
    unsigned long positional_a0;
    unsigned long positional_a4;
    unsigned long positional_a8;
    unsigned short positional_ac;
    unsigned short padding_ae;
    unsigned long positional_b0;
    unsigned char use_owned_nodes_b4;
    unsigned char padding_b5[3];
    unsigned long positional_b8;
};

static_assert(sizeof(W8OctBuildLink) == 8, "W8OctBuildLink_must_be_8");
static_assert(sizeof(W8OctBuildLinkLists) == 0x25c,
              "W8OctBuildLinkLists_must_be_0x25c");
static_assert(sizeof(W8OctBuildNode00446330) == 0x30,
              "W8OctBuildNode00446330_must_be_0x30");
static_assert(sizeof(W8CountedOctBuildNode004AF760) == 0x30,
              "W8CountedOctBuildNode004AF760_must_be_0x30");
static_assert(sizeof(W8OctBuildTree00446390) == 0xbc,
              "W8OctBuildTree00446390_must_be_0xbc");

#endif
