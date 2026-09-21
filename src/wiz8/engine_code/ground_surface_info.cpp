#include "wiz8/engine_code/SoundEvent.h"

#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Octree.h"

/* Unresolved fragment: 0x00420CA0 is address-adjacent to GameData code, but
   no retail source-path evidence attributes its original translation unit.
   Keep the body separate until that ownership is established. */

/* Settle a copy of the point against the active GameData octree and expose
   the footstep selectors of the surface recorded by the trace. Retail still
   reads value_54 after the no-GameData fallback, so preserve that latent null
   dereference instead of adding a defensive guard. */
// FUNCTION: WIZ8 0x00420ca0
float GetGroundSurfaceInfo(const srVector3T<float>* position, char* surface, char* material)
{
    srVector3T<float> candidate = *position;
    float height;

    if (g_octree_game_data_00652db0 != 0 &&
        g_octree_game_data_00652db0->positional_04 != 0) {
        height = g_octree_game_data_00652db0->positional_04->SettleToGround(
            &candidate, 0, 1, 500.0f);
    } else {
        height = position->y;
    }

    if (g_octree_game_data_00652db0->value_54 != 0) {
        *surface = g_octree_game_data_00652db0
                       ->m_pSurfaces[g_octree_game_data_00652db0->value_54]
                       .footstep_surface_3c;
        *material = g_octree_game_data_00652db0
                        ->m_pSurfaces[g_octree_game_data_00652db0->value_54]
                        .footstep_material_3d;
        return height;
    }

    *material = 0;
    *surface = 0;
    return height;
}
