#include "wiz8/engine_code/OctPreTree.h"
#include "wiz8/float_constants.h"

#include <string.h>

/* Construct the spatial value used by both the runtime octree and the level
   build tree.  A source value describes the next child: its extent halves and
   its depth advances only when the source is the root-kind record. */
// FUNCTION: WIZ8 0x0046ccc0
W8OctSpatialState0046CCC0::W8OctSpatialState0046CCC0(
    const W8OctSpatialState0046CCC0* source)
{
    Reset0046CDC0();
    level_kind_6c = 1;
    if (source != 0) {
        for (int axis = 0; axis != 3; ++axis) {
            (&minimum_0c.x)[axis] = (&source->minimum_0c.x)[axis];
            (&maximum_18.x)[axis] = (&source->maximum_18.x)[axis];
            (&clipped_minimum_24.x)[axis] =
                (&source->clipped_minimum_24.x)[axis];
            (&clipped_maximum_30.x)[axis] =
                (&source->clipped_maximum_30.x)[axis];
        }
        if (source->level_kind_6c == 1) {
            extent_04 = source->extent_04 * g_float_005ebc7c;
            depth_44 = source->depth_44 + 1;
        }
        else {
            extent_04 = source->extent_04;
            depth_44 = source->depth_44;
        }
        cell_size_08 = source->cell_size_08;
        state_3c = source->state_3c;
        positional_58 = source->positional_58;
        positional_64 = source->positional_64;
        positional_68 = source->positional_68;
        node_extent_70 = source->node_extent_70;
        root_90 = source->root_90;
        positional_94 = source->positional_94;
        owned_98 = source->owned_98;
        flags_00 = source->flags_00;
        item_count_40 = source->item_count_40;
        positional_74 = source->positional_74;
        positional_46 = source->positional_46;
        positional_52 = source->positional_52;
        owned_5c = source->owned_5c;
        node_extent_70 = source->node_extent_70;
        positional_60 = source->positional_60;
    }
}

// FUNCTION: WIZ8 0x0046cdc0
void W8OctSpatialState0046CCC0::Reset0046CDC0()
{
    memset(this, 0, sizeof(*this));
}

// FUNCTION: WIZ8 0x0046cdd0
W8OctSpatialState0046CCC0::~W8OctSpatialState0046CCC0()
{
    owned_5c = 0;
    root_90 = 0;
    owned_98 = 0;
}
