#ifndef WIZ8_ENGINE_CODE_OCT_PRE_TREE_H
#define WIZ8_ENGINE_CODE_OCT_PRE_TREE_H

#include "surrender/srMath.h"

/* A reusable, non-polymorphic 0x9c spatial record.  Octree.cpp constructs one
   at the start of its 0x29c owner, while OctBuildTree.cpp constructs the same
   value at the start of its 0xbc build tree and uses standalone copies while
   inserting surfaces.  Same-address construction proves the common value
   boundary, but not whether either owner used inheritance or a first member. */
struct W8OctSpatialState0046CCC0 {
    explicit W8OctSpatialState0046CCC0(
        const W8OctSpatialState0046CCC0* source = 0);
    ~W8OctSpatialState0046CCC0();

    void Reset0046CDC0();

    unsigned long flags_00;
    float extent_04;
    float cell_size_08;
    srVector3T<float> minimum_0c;
    srVector3T<float> maximum_18;
    srVector3T<float> clipped_minimum_24;
    srVector3T<float> clipped_maximum_30;
    unsigned long state_3c;
    unsigned long item_count_40;
    unsigned short depth_44;
    unsigned short positional_46;
    unsigned short item_limit_48;
    unsigned char positional_4a[8];
    unsigned short positional_52;
    unsigned char positional_54[4];
    unsigned short positional_58;
    unsigned short positional_5a;
    void* owned_5c;
    unsigned long positional_60;
    unsigned long positional_64;
    unsigned long positional_68;
    unsigned short level_kind_6c;
    unsigned short positional_6e;
    float node_extent_70;
    unsigned long positional_74;
    srVector3T<float> working_minimum_78;
    srVector3T<float> working_maximum_84;
    void* root_90;
    unsigned long positional_94;
    void* owned_98;
};

unsigned char TestSpatialTriangle0046CE60(
    const srVector3T<float>* bounds,
    const srVector3T<float>* vertices,
    const srVector3T<float>* plane_normal);

static_assert(sizeof(W8OctSpatialState0046CCC0) == 0x9c,
              "W8OctSpatialState0046CCC0_must_be_0x9c");

#endif
