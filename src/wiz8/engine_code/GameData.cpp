#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/FormationAndFacing.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/Noise.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/xstatus.h"
#include "soundman.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "surrender/srShader.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "surrender/srCamera.h"
#include "random.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>
#include "wiz8/engine_code/3d.h"

// GLOBAL: WIZ8 0x00652da8
unsigned int* g_level_flags_00652da8;
// GLOBAL: WIZ8 0x00652dac
W8LevelDataRecord* g_level_data_00652dac;
// GLOBAL: WIZ8 0x00652da7
unsigned char g_flag_00652da7;

// FUNCTION: WIZ8 0x00420bd0
float SettlePositionToGround00420BD0(const srVector3T<float>* position, unsigned char* hit)
{
    srVector3T<float> candidate = *position;
    if (g_octree_game_data_00652db0 != 0 && g_octree_game_data_00652db0->positional_04 != 0) {
        return g_octree_game_data_00652db0->positional_04->SettleToGround(&candidate, hit, 1,
                                                                          500.0f);
    }
    float height = position->y;
    if (hit != 0) {
        *hit = 0;
    }
    return height;
}

/*
 * Engine Code\GameData.cpp.
 *
 * The bits of the level the party is currently standing in. One global points
 * at that record, and the accessors below read and write single bits of the
 * flag word that leads it. Nothing here establishes what the bits mean, so
 * each is named for the bit it touches; three of them are read together by
 * bodies that do say something about the record.
 */

enum {
    W8_LEVEL_FLAG_0 = 0x001,
    W8_LEVEL_FLAG_NO_SOUND_ENVIRONMENT = 0x008,
    W8_LEVEL_FLAG_4 = 0x010,
    W8_LEVEL_FLAG_MOVEMENT_ACTIVE = 0x020,
    W8_LEVEL_FLAG_5_TO_7 = 0x0e0,
    W8_LEVEL_FLAG_6 = 0x040,
    W8_LEVEL_FLAG_MOVEMENT_RESET = 0x080,
    W8_LEVEL_FLAG_8 = 0x100,
    W8_LEVEL_FLAG_9 = 0x200
};

// GLOBAL: WIZ8 0x00652dba
unsigned char g_level_override_00652dba;
// GLOBAL: WIZ8 0x00652dce
unsigned char g_flag_00652dce;

/* Resolve one surface's three vertex indices through the active processed
   GameData vertex table.  The retail comparison is signed and accepts an index
   equal to vertex_count, so that historical boundary behavior is preserved. */
// FUNCTION: WIZ8 0x004214d0
unsigned char LoadSurfaceVertices004214D0(srVector3T<float>* output, const int* vertex_indices)
{
    short index = 0;
    do {
        if (g_octree_game_data_00652db0->m_iNumVertices < vertex_indices[index]) {
            return 0;
        }
        output[index] = g_octree_game_data_00652db0->m_pVertices[vertex_indices[index]];
        ++index;
    } while (index < 3);
    return 1;
}

/* Ensure the shared game-data object exists, then run its update. */
// FUNCTION: WIZ8 0x0041F1F0
void UpdateSharedGameDataObject0041F1F0()
{
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            return;
        }
    }
    g_game_time_accumulator_6598bc->Update();
}

// FUNCTION: WIZ8 0x0041F260
void UpdateGameDataRuntime0041F260()
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera;
        if (g_gd_camera_65a0f8 == 0) {
            srAssertFail("gpGDCamera", "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 2739,
                         0);
        }
    }
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            g_game_time_accumulator_6598bc->Update();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        ResumeSharedGameTimers00439CA0();
        g_flag_00652dce = 0;
    }
    g_game_time_accumulator_6598bc->Update();
}

/* Apply world-render camera-motion flags into `rotation` and mirror the yaw
   rotation into `saved`. ECX is the owning W8GameData; the body reads only
   globals. */
// FUNCTION: WIZ8 0x0041F330
void W8GameData::ApplyCameraMotionFlags0041F330(unsigned int flags, srMatrix3T<float>* rotation,
                                                srMatrix3T<float>* saved)
{
    float pitch_input;
    float yaw_input;
    unsigned short timer_flags;
    unsigned int level_flags;
    W8LevelDataRecord* level;
    W8EnvironRecord* environ_record;

    if (g_environ_00652DB4 == 0) {
        environ_record = new W8EnvironRecord;
        if (environ_record != 0) {
            environ_record->value_00 = 0;
            environ_record->value_04 = 0;
            environ_record->value_08 = 0;
            environ_record->value_14 = -g_navigator_gravity_00603acc;
            environ_record->value_10 = 0;
            environ_record->value_18 = 0;
            environ_record->value_20 = 1.0f;
            environ_record->vector_24.Set(0.0, 0.0, 0.0);
            environ_record->value_1c = 0.05f;
            environ_record->value_30 = g_default_world_height_00603ac8;
            environ_record->value_40 = 1.0f;
            environ_record->value_34 =
                g_camera_level_forward_scale_603aac * g_navigator_linked_radius_scale_005ebc98;
            /* Retail stores g_float_00603abc at +0x38 and g_float_00603ab8 at
               +0x3c (RescaleToReference's pairing); GDFileIO's default-bank
               init currently spells the reverse before FileRead overwrites. */
            environ_record->value_3c = g_float_00603ab8;
            environ_record->value_38 = g_float_00603abc;
        }
        g_environ_00652DB4 = environ_record;
        if (g_environ_00652DB4 == 0) {
            ShutdownWithErrorBox("TrackRotation: Could not allocate gpEnviron.\n");
        }
    }

    timer_flags = g_game_time_accumulator_6598bc->m_flags;
    if ((timer_flags & 8) != 0) {
        return;
    }
    if (g_shared_timer_paused != 0 && (timer_flags & 1) == 0) {
        return;
    }
    if (g_shared_timer_flag_d1 != 0) {
        return;
    }
    if ((timer_flags & 0x10) != 0) {
        return;
    }

    if ((g_gd_camera_65a0f8->m_positional_000 & 0x80) != 0) {
        g_gd_camera_65a0f8->m_positional_000 &= ~0x80u;
        MarkRendererReady();
    }

    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_9;
        level = g_level_data_00652dac;
        if (AnyCharacterEngaged() == 0 || ((level_flags = level->flags) & 0xc0) != 0) {
            level_flags = level->flags;
            flags &= 0xff00;
            if ((level_flags & W8_LEVEL_FLAG_MOVEMENT_ACTIVE) == 0) {
                level->flags = level_flags & ~W8_LEVEL_FLAG_8;
            }
        } else if ((level_flags & W8_LEVEL_FLAG_MOVEMENT_ACTIVE) != 0) {
            if ((level_flags & W8_LEVEL_FLAG_8) != 0) {
                flags |= 0x80u;
            } else {
                flags &= ~0x80u;
            }
        } else if ((flags & 0x80) != 0) {
            level->flags = level_flags | W8_LEVEL_FLAG_8;
        } else {
            level->flags = level_flags & ~W8_LEVEL_FLAG_8;
        }
    }

    pitch_input = 0.0f;
    yaw_input = 0.0f;
    if ((flags & 0x800) != 0) {
        pitch_input = 0.1745329350233078f;
    } else if ((flags & 0x400) != 0) {
        pitch_input = -0.1745329350233078f;
    }
    if ((flags & 0x100) != 0) {
        yaw_input = -0.1745329350233078f;
    } else if ((flags & 0x200) != 0) {
        yaw_input = 0.1745329350233078f;
    }
    if ((flags & 0x2000) != 0) {
        g_camera_max_yaw_velocity_609ea4 = 0.116355285f;
        yaw_input = yaw_input * g_float_005ebc3c;
    } else {
        g_camera_max_yaw_velocity_609ea4 = 0.3490658700466156f;
    }

    if ((pitch_input != g_float_005ebb34 || yaw_input != g_float_005ebb34) &&
        (g_level_data_00652dac->flags & W8_LEVEL_FLAG_6) == 0) {
        g_navigator_position_changed_659c11 = 1;
    }
    if (g_flag_00652da7 != 0) {
        g_gd_camera_65a0f8->SetManualControlActive(1);
    }
    g_gd_camera_65a0f8->ApplyPitchInput(pitch_input);
    g_gd_camera_65a0f8->ApplyYawInput(yaw_input);
    g_gd_camera_65a0f8->Update(g_game_time_accumulator_6598bc->GetValue28());
    if ((flags & 0x1000) == 0) {
        g_gd_camera_65a0f8->GetRotationMatrix(rotation);
    }
    *saved = g_gd_camera_65a0f8->m_yaw_rotation;
}

// GLOBAL: WIZ8 0x00652dcd
unsigned char g_level_motion_fast_00652dcd;
// GLOBAL: WIZ8 0x00652db9
bool g_level_footstep_pending_00652db9;
// GLOBAL: WIZ8 0x00603ac0
float g_camera_motion_clamp_00603ac0 = 1125.0f;
// GLOBAL: WIZ8 0x00603ac4
float g_camera_motion_divisor_00603ac4 = 3000.0f;
// GLOBAL: WIZ8 0x00603ad4
int g_level_footstep_sound_00603ad4 = -1;
// GLOBAL: WIZ8 0x00652dd0
float g_level_footstep_time_00652dd0;
// GLOBAL: WIZ8 0x005ebc50
const double g_motion_delta_epsilon_005ebc50 = 0.10000000149011612;
// GLOBAL: WIZ8 0x005ebcd4
const float g_footstep_fall_threshold_005ebcd4 = -250.0f;
// GLOBAL: WIZ8 0x00652940
srVector3T<float> g_origin_652940;

/* Advance the camera under world-render motion flags. ECX is the owning
   W8GameData; the body mostly reads globals. Zero elapsed (`camera_scale_14`)
   returns before the motion helpers. */
// FUNCTION: WIZ8 0x0041F5F0
unsigned char W8GameData::ApplyCameraMotion0041F5F0(unsigned int flags, srVector3T<float>* position,
                                                    srVector3T<float>* delta,
                                                    srMatrix3T<float>* saved)
{
    unsigned short timer_flags;
    unsigned int level_flags;
    W8LevelDataRecord* level;
    float forward_scale;
    float component;
    char fast_move;
    char moved;
    srVector3T<float> new_position;
    srVector3T<float> from_origin;

    if (g_level_data_00652dac == 0) {
        level = new W8LevelDataRecord;
        g_level_flags_00652da8 = &level->flags;
        g_level_data_00652dac = level;
        if (g_level_flags_00652da8 == 0) {
            ShutdownWithErrorBox("TrackMovement: Could not allocate gpMovement.\n");
        }
        PauseSharedGameTimers00439BC0();
        g_shared_timer_flag_d2 = 1;
        g_level_motion_fast_00652dcd = 0;
    }

    timer_flags = g_game_time_accumulator_6598bc->m_flags;
    if ((timer_flags & 8) != 0 || (g_shared_timer_paused != 0 && (timer_flags & 1) == 0) ||
        g_shared_timer_flag_d1 != 0) {
        if (g_shared_timer_flag_d2 == 0) {
            return 0;
        }
        if ((timer_flags & 1) != 0) {
            return 0;
        }
        g_shared_timer_flag_d2 = 0;
        if (g_shared_timer_flag_d1 == 0) {
            ResumeSharedGameTimers00439CA0();
        }
    }

    level = g_level_data_00652dac;
    level_flags = level->flags;
    if ((((level_flags & W8_LEVEL_FLAG_6) != 0 && (level_flags & W8_LEVEL_FLAG_4) != 0) &&
         ((level_flags & W8_LEVEL_FLAG_0) == 0 && g_byte_00659a64 == 0)) ||
        (g_game_time_accumulator_6598bc->m_flags & 0x10) != 0) {
        return 0;
    }

    if (AnyCharacterEngaged() == 0 || ((level_flags = level->flags) & 0xc0) != 0) {
        level_flags = level->flags;
        flags &= 0xff00;
        if ((level_flags & W8_LEVEL_FLAG_MOVEMENT_ACTIVE) == 0) {
            level_flags &= ~W8_LEVEL_FLAG_8;
            level->flags = level_flags;
        }
    } else if ((level_flags & W8_LEVEL_FLAG_MOVEMENT_ACTIVE) == 0) {
        if ((flags & 0x80) == 0) {
            level->flags = level_flags & ~W8_LEVEL_FLAG_8;
        } else {
            level->flags = level_flags | W8_LEVEL_FLAG_8;
        }
    } else if ((level_flags & W8_LEVEL_FLAG_8) == 0) {
        flags &= ~0x80u;
    } else {
        flags |= 0x80u;
    }

    level = g_level_data_00652dac;
    forward_scale = g_camera_level_forward_scale_603aac;
    level->flags &= 0xffffffec;
    level->camera_scale_14 = g_game_time_accumulator_6598bc->GetValue28();
    level->camera_position_34 = *position;
    level->vector_64.SetZero();
    level->vector_a0.SetZero();
    level->vector_40.SetZero();
    level->sound_environment_0c = -1;
    level->sound_environment_alt_0d = -1;

    if (g_level_data_00652dac->camera_scale_14 == g_float_005ebb34) {
        return 0;
    }

    if (flags == 0) {
        g_level_data_00652dac->vector_40.Set(0.0f, 0.0f, 0.0f);
        level->vector_70.SetZero();
    }

    delta->Set(0.0f, 0.0f, 0.0f);
    level = g_level_data_00652dac;
    if ((flags & 4) != 0) {
        component = forward_scale + g_level_data_00652dac->vector_40.z;
        g_level_data_00652dac->vector_40.z = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.z = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.z = -g_camera_motion_clamp_00603ac0;
        }
    }
    level = g_level_data_00652dac;
    if ((flags & 8) != 0) {
        component = g_level_data_00652dac->vector_40.z - forward_scale;
        g_level_data_00652dac->vector_40.z = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.z = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.z = -g_camera_motion_clamp_00603ac0;
        }
    }
    level = g_level_data_00652dac;
    if ((flags & 1) != 0) {
        component = g_level_data_00652dac->vector_40.x - forward_scale;
        g_level_data_00652dac->vector_40.x = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.x = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.x = -g_camera_motion_clamp_00603ac0;
        }
    }
    level = g_level_data_00652dac;
    if ((flags & 2) != 0) {
        component = forward_scale + g_level_data_00652dac->vector_40.x;
        g_level_data_00652dac->vector_40.x = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.x = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.x = -g_camera_motion_clamp_00603ac0;
        }
    }
    level = g_level_data_00652dac;
    if ((flags & 0x10) != 0 && (g_environment_load_flag_00603ad0 == 0 || g_environ_00652DB4 == 0)) {
        component = forward_scale + g_level_data_00652dac->vector_40.y;
        g_level_data_00652dac->vector_40.y = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.y = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.y = -g_camera_motion_clamp_00603ac0;
        }
    }
    level = g_level_data_00652dac;
    if ((flags & 0x20) != 0 && g_environment_load_flag_00603ad0 == 0) {
        component = g_level_data_00652dac->vector_40.y - forward_scale;
        g_level_data_00652dac->vector_40.y = component;
        if (component > g_camera_motion_clamp_00603ac0) {
            level->vector_40.y = g_camera_motion_clamp_00603ac0;
        } else if (component < -g_camera_motion_clamp_00603ac0) {
            level->vector_40.y = -g_camera_motion_clamp_00603ac0;
        }
    }

    fast_move = 0;
    if ((flags & 0x80) != 0) {
        fast_move = 1;
    }
    if (g_level_data_00652dac->vector_40.Length() > g_camera_motion_clamp_00603ac0) {
        g_level_data_00652dac->vector_40.SetLength(g_camera_motion_clamp_00603ac0);
    }
    g_level_motion_fast_00652dcd = g_level_data_00652dac->ApplySavedMotionMatrix00420810(
        g_level_motion_fast_00652dcd, fast_move, saved);

    if (g_environment_load_flag_00603ad0 == 0) {
        *delta = g_level_data_00652dac->vector_a0;
        moved = static_cast<float>(g_motion_delta_epsilon_005ebc50) < delta->Length();
        g_level_data_00652dac->UpdateMotionProgress0041FF90(g_level_motion_fast_00652dcd, moved);
        if (moved == 0) {
            goto after_move;
        }
    } else {
        moved = AdvanceEnvironmentMotion0041AB40();
        g_level_data_00652dac->UpdateMotionProgress0041FF90(g_level_motion_fast_00652dcd, moved);
        *delta = g_level_data_00652dac->vector_a0;
        if (delta->Length() <= static_cast<float>(g_motion_delta_epsilon_005ebc50)) {
            if (moved == 0) {
                goto after_move;
            }
        } else {
            moved = 1;
        }
    }

    new_position.Set(position->x + delta->x, position->y + delta->y, position->z + delta->z);
    from_origin = new_position - g_origin_652940;
    if (sqrtf(DotProduct(from_origin, from_origin)) != static_cast<float>(g_zero_005ebb40)) {
        MarkRendererReady();
        g_gd_camera_65a0f8->m_position_08c = new_position;
    }

after_move:
    if (g_environ_00652DB4->value_05 == 0) {
        g_level_footstep_pending_00652db9 = 1;
    } else if (g_level_footstep_pending_00652db9 != 0) {
        if (g_level_data_00652dac->vector_64.y <= g_footstep_fall_threshold_005ebcd4) {
            PlayFootstep0047A440(g_level_data_00652dac->sound_environment_0c,
                                 g_level_data_00652dac->sound_environment_alt_0d, 1);
            g_level_data_00652dac->footstep_accumulator_10 = 0;
        }
        g_level_footstep_pending_00652db9 = 0;
    }
    UpdateLevelMovementAudio00420E20();
    if (moved == 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_9;
    } else {
        g_level_data_00652dac->flags |= W8_LEVEL_FLAG_9;
    }
    return moved;
}

// GLOBAL: WIZ8 0x005ebc48
const double g_motion_vector_epsilon_005ebc48 = 5.0;
// GLOBAL: WIZ8 0x00603ad1
unsigned char g_environment_motion_active_00603ad1 = 1;
// GLOBAL: WIZ8 0x00652db8
unsigned char g_environ_ground_latch_00652db8;

// FUNCTION: WIZ8 0x00421800
void W8EnvironRecord::SetScaledMotion00421800(const srVector3T<float>* motion)
{
    double inv_scale = g_double_005ebc30 / scale_0c;
    vector_24.x = motion->x * inv_scale;
    vector_24.y = motion->y * inv_scale;
    vector_24.z = motion->z * inv_scale;
}

// FUNCTION: WIZ8 0x00421850
void W8EnvironRecord::AddScaledMotion00421850(srVector3T<float>* position)
{
    position->x = vector_24.x * scale_0c + position->x;
    position->y = vector_24.y * scale_0c + position->y;
    position->z = vector_24.z * scale_0c + position->z;
}

// FUNCTION: WIZ8 0x0041FE20
unsigned char W8LevelDataRecord::ClampCameraToBounds0041FE20(const srVector3T<float>* minimum,
                                                             const srVector3T<float>* maximum)
{
    unsigned char clamped = 0;
    unsigned char below_min_y = 0;

    if (camera_position_34.y < minimum->y) {
        if (g_status_685170.value_2390 != 0) {
            vector_a0.y = maximum->y - minimum->y;
        }
        clamped = 1;
        below_min_y = 1;
    }
    if (camera_position_34.x < minimum->x) {
        clamped = 1;
        vector_a0.x = maximum->x - minimum->x;
    }
    if (camera_position_34.z < minimum->z) {
        clamped = 1;
        vector_a0.z = maximum->z - minimum->z;
    }
    if (maximum->x < camera_position_34.x) {
        clamped = 1;
        vector_a0.x = minimum->x - maximum->x;
    }
    if (camera_position_34.z <= maximum->z) {
        if (clamped == 0) {
            return 0;
        }
    } else {
        clamped = 1;
        vector_a0.z = minimum->z - maximum->z;
    }

    g_environ_00652DB4->vector_24.SetZero();
    if (below_min_y != 0) {
        if (g_status_685170.value_2390 != 0) {
            g_level_override_00652dba = 0;
            return clamped;
        }
        BeginPartyMovement();
    }
    return clamped;
}

// GLOBAL: WIZ8 0x005ebc5c
const float g_monster_motion_push_005ebc5c = 1.05f;

/* Probe active collidable props along the motion segment. On a hit, rewrites
   the caller's position into world space, may nudge `direction`, and latches
   plane / level-data contact fields used by the collision response pass. */
// FUNCTION: WIZ8 0x0041B770
W8GDSurface* W8GameData::ProbePropsAlongMotion0041B770(srVector3T<float>* direction,
                                                       srVector3T<float>* position,
                                                       srVector3T<float>* scratch,
                                                       float* nearest_distance)
{
    W8GDSurface* nearest_surface;
    W8GDSurface* surface;
    W8Prop* prop;
    GDProp* gd_prop;
    W8LevelDataRecord* level;
    unsigned long* objects;
    unsigned int count;
    unsigned int index;
    int surface_index;
    int hit_prop_id;
    unsigned char direction_zero;
    float hit_distance;
    float slope;
    float along_length;
    float residual_length;
    float facing;
    srVector3T<float> prop_delta;
    srVector3T<float> adjusted_direction;
    srVector3T<float> probe;
    srVector3T<float> test_direction;
    srVector3T<float> hit_point;
    srVector3T<float> normal;
    srVector3T<float> projected;
    srVector3T<float> residual;
    srVector3T<float> along_normal;

    nearest_surface = 0;
    if (g_world->collidable_props->GetCount() == 0) {
        return 0;
    }
    /* Function-local static plane ResolveCollision reads through hit_plane_38;
       atexit thunk at 0x0041BD50. Declared after the early-out so the guard
       matches retail control flow. */
    static srVector4T<float> s_prop_hit_plane_00652d90;
    objects = 0;
    count = static_cast<unsigned int>(
        positional_04->CollectObjectsAlongSegment(&objects, position, direction, 504.0f, 8));
    for (index = 0; index < count; ++index) {
        prop = *g_world->collidable_props->GetAt(objects[index]);
        if (prop->GetSetting6C() != 0) {
            gd_prop = prop->m_gd_prop;
            prop->flags_1c |= 0x10;
            if (gd_prop == 0) {
                prop->BuildOrRefreshPathingRepresentation();
                gd_prop = prop->m_gd_prop;
                if (gd_prop == 0) {
                    return 0;
                }
            }
            prop->GetDelta0044E130(&prop_delta, position);
            adjusted_direction.x = direction->x - prop_delta.x;
            adjusted_direction.y = direction->y - prop_delta.y;
            adjusted_direction.z = direction->z - prop_delta.z;
            probe = *position;
            if (adjusted_direction.x != g_float_005ebb34 ||
                adjusted_direction.y != g_float_005ebb34 ||
                adjusted_direction.z != g_float_005ebb34) {
                direction_zero = 0;
            } else {
                direction_zero = 1;
            }
            test_direction = adjusted_direction;
            for (surface_index = 0; surface_index < gd_prop->m_surface_count_14; ++surface_index) {
                surface = &gd_prop->m_pGDSurfaces[surface_index];
                surface->hit_plane_38 = 0;
                if (direction_zero != 0) {
                    test_direction.x = surface->plane_24[0];
                    test_direction.y = surface->plane_24[1];
                    test_direction.z = surface->plane_24[2];
                }
                if (surface->TestSegment0041CF90(&probe, &test_direction, &hit_distance,
                                                 gd_prop->m_pVertices) != 0) {
                    if (hit_distance < *nearest_distance) {
                        *nearest_distance = hit_distance;
                        hit_point = probe;
                        scratch->x = prop_delta.x;
                        scratch->y = prop_delta.y;
                        scratch->z = prop_delta.z;
                        nearest_surface = surface;
                        hit_prop_id = objects[index];
                    }
                    probe = *position;
                }
            }
        }
        level = g_level_data_00652dac;
    }
    if (nearest_surface != 0) {
        s_prop_hit_plane_00652d90.x = nearest_surface->plane_24[0];
        s_prop_hit_plane_00652d90.y = nearest_surface->plane_24[1];
        s_prop_hit_plane_00652d90.z = nearest_surface->plane_24[2];
        s_prop_hit_plane_00652d90.w = nearest_surface->plane_24[3];
        slope = nearest_surface->slope_48;
        projected.Set(scratch->x, scratch->y, scratch->z);
        residual.Set(scratch->x, scratch->y, scratch->z);
        along_normal.Set(scratch->x, scratch->y, scratch->z);
        normal.Set(s_prop_hit_plane_00652d90.x, s_prop_hit_plane_00652d90.y,
                   s_prop_hit_plane_00652d90.z);
        if (g_float_005ebc58 < normal.LengthSquared()) {
            along_normal = normal * (DotProduct(along_normal, normal) / normal.LengthSquared());
        }
        along_length = along_normal.Length();
        residual = (residual - along_normal) * static_cast<double>(slope);
        residual_length = residual.Length();
        if (projected.Length() <= g_float_005ebb38) {
            facing = 0.0f;
        } else {
            facing = DotProduct(projected, normal) / projected.Length();
            if (facing < g_float_005ebb34) {
                along_length = -along_length;
            }
        }
        level = g_level_data_00652dac;
        if (level->primary_contact_prop_id == -1) {
            level->primary_contact_prop_id = hit_prop_id;
        } else if (level->primary_contact_prop_id != hit_prop_id &&
                   level->secondary_contact_prop_id != hit_prop_id) {
            level->secondary_contact_prop_id = hit_prop_id;
        }
        if ((level->flags & 1) == 0 || level->residual_contact_length_18 < residual_length) {
            level->flags |= 1;
            level->vector_88 = projected;
            level->residual_contact_length_18 = residual_length;
            *direction += along_normal - residual;
            level->vector_94 = residual;
            level->vector_58 = residual / static_cast<double>(level->camera_scale_14);
        }
        if (level->contact_facing_1c < facing) {
            level->contact_facing_1c = facing;
            level->contact_normal_ac = normal;
        }
        s_prop_hit_plane_00652d90.w = s_prop_hit_plane_00652d90.w - along_length;
        nearest_surface->hit_plane_38 = &s_prop_hit_plane_00652d90;
        position->x = hit_point.x + scratch->x;
        position->y = hit_point.y + scratch->y;
        position->z = hit_point.z + scratch->z;
    }
    return nearest_surface;
}

// SYNTHETIC: WIZ8 0x0041BD50
// `dynamic atexit destructor for 's_prop_hit_plane_00652d90''

// FUNCTION: WIZ8 0x0041BD60
unsigned char W8GameData::ProbeMonstersAlongMotion0041BD60(srVector3T<float>* direction,
                                                           srVector3T<float>* position, int)
{
    unsigned long* objects;
    unsigned int count;
    unsigned int index;
    unsigned int monster_list_index;
    W8MonsterInfo* monster_info;
    W8Monster* monster;
    srVector3T<float> lower;
    srVector3T<float> upper;
    srVector3T<float> velocity;
    srVector3T<float> monster_position;
    srVector3T<float> adjustment;
    float extent;
    float adjusted_x;
    float adjusted_z;
    float horizontal;
    float dx;
    float dy;
    float dz;
    float planar;
    float radius;
    float push;
    float length_squared;
    float scale;
    double time_scale;
    unsigned char hit;

    objects = 0;
    hit = 0;
    extent = direction->Length() + g_runtime_world_scale_6081e8 + g_world_scale_005ebc40;
    lower.Set(position->x - extent, position->y - extent, position->z - extent);
    upper.Set(position->x + extent, position->y + extent, position->z + extent);
    count = static_cast<unsigned int>(
        g_octree_6598a4->QueryObjects(&objects, &lower, &upper, W8_OCTREE_KIND_LOCATION, -1));
    if (count == 0) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        if (objects[index] == 0) {
            return hit;
        }
        monster_list_index = MonsterGetIndexByLocationID(
            0x3a6, "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", objects[index], 1);
        monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (monster_info != 0 && monster_info->monster != 0 &&
            monster_info->monster->state_088 != 0) {
            monster = monster_info->monster;
            time_scale = g_rate_006068EC * g_game_time_accumulator_6598bc->GetValue28();
            monster->GetVelocity(&velocity);
            adjusted_x = direction->x - velocity.x * static_cast<float>(time_scale);
            adjusted_z = direction->z - velocity.z * static_cast<float>(time_scale);
            horizontal = sqrtf(adjusted_x * adjusted_x + adjusted_z * adjusted_z);
            monster_position = monster->GetPosition();
            dx = monster_position.x - (adjusted_x + position->x);
            dy = monster_position.y - position->y;
            dz = monster_position.z - (adjusted_z + position->z);
            planar = sqrtf(dz * dz + dx * dx);
            radius = monster->radius_084;
            if (monster_info->fInCombat != 0) {
                radius = radius + g_world_scale_005ebc40;
            }
            float abs_dy = dy;
            // reinterpret-ok: retail clears the sign bit of the spilled dy float
            *reinterpret_cast<unsigned int*>(&abs_dy) &= 0x7fffffffu;
            if (abs_dy < g_float_005ebc64 && planar < radius + g_world_scale_005ebc40) {
                push = g_monster_motion_push_005ebc5c - (planar - radius) * g_float_005ebc60;
                if (g_monster_motion_push_005ebc5c < push) {
                    push = g_monster_motion_push_005ebc5c;
                }
                length_squared = dz * dz + dx * dx;
                adjustment.x = dx;
                adjustment.y = dy;
                adjustment.z = dz;
                // reinterpret-ok: retail zeros the push y spill after storing dy
                *reinterpret_cast<unsigned int*>(&adjustment.y) = 0;
                if (length_squared != static_cast<float>(g_zero_005ebb40)) {
                    scale = -(push * horizontal) / sqrtf(length_squared);
                    adjustment.x = dx * scale;
                    adjustment.z = dz * scale;
                }
                hit = 1;
                direction->x = adjustment.x + direction->x;
                direction->y = adjustment.y + direction->y;
                direction->z = adjustment.z + direction->z;
            }
        }
    }
    return hit;
}

/* Environment-load camera advance: integrate level vectors, clamp environ
   gravity into the path, then walk up to five collision attempts against
   props/octree/geometry before committing crossed surfaces. */
// FUNCTION: WIZ8 0x0041AB40
unsigned char W8GameData::AdvanceEnvironmentMotion0041AB40()
{
    W8LevelDataRecord* level;
    W8EnvironRecord* environ_record;
    srVector3T<float> camera_position;
    srVector3T<float> adjusted_position;
    srVector3T<float> motion_delta;
    srVector3T<float> environ_delta;
    srVector3T<float> probe_position;
    srVector3T<float> hit_position;
    srVector3T<float> gravity;
    srVector3T<float> scaled;
    W8GDSurface* nearest_surface;
    W8GDSurface* surface;
    W8GDSurface* collisions[101];
    unsigned long* octree_hits;
    int* geometry_hits;
    int hit_count;
    int collision_count;
    int attempt;
    int index;
    int crossed_count;
    float nearest_distance;
    float hit_distance;
    float motion_length;
    float scale;
    srVector3T<float> scratch;
    unsigned char first_pass;
    unsigned char exhausted;
    unsigned char forced_exit;
    unsigned char prop_hit;
    srVector3T<float> geometry_from;
    srVector3T<float> geometry_to;

    octree_hits = 0;
    geometry_hits = 0;
    prop_hit = 0;
    if (g_level_data_00652dac->ClampCameraToBounds0041FE20(&minimum_08, &maximum_14) != 0) {
        return 0;
    }

    level = g_level_data_00652dac;
    level->flags &= ~3u;
    level->vector_64.x = level->vector_64.x + level->vector_58.x;
    level->vector_64.y = level->vector_64.y + level->vector_58.y;
    level->vector_64.z = level->vector_64.z + level->vector_58.z;
    level->vector_94.Set(level->vector_58.x * level->camera_scale_14,
                         level->vector_58.y * level->camera_scale_14,
                         level->vector_58.z * level->camera_scale_14);
    level->vector_a0 += level->vector_94;
    level->residual_contact_length_18 = 0.0f;
    level->contact_facing_1c = 0.0f;

    level = g_level_data_00652dac;
    camera_position = level->camera_position_34;
    adjusted_position = camera_position;
    if (geometry_index_00 == 0 && positional_04 == 0) {
        if (static_cast<float>(g_motion_delta_epsilon_005ebc50) < level->vector_a0.Length()) {
            return 1;
        }
        return 0;
    }

    g_environ_ground_latch_00652db8 = g_environ_00652DB4->value_04;
    g_environment_motion_active_00603ad1 = 1;
    if (level->vector_64.Length() <= static_cast<float>(g_motion_vector_epsilon_005ebc48)) {
        level->flags &= ~4u;
    } else {
        level->flags |= 4u;
    }

    environ_record = g_environ_00652DB4;
    environ_record->value_20 = environ_record->value_1c;
    environ_record->value_05 = 0;
    environ_record->scale_0c = level->camera_scale_14;
    gravity.Set(environ_record->value_10, environ_record->value_14, environ_record->value_18);
    scaled = gravity * static_cast<double>(environ_record->scale_0c);
    environ_record->vector_24 += scaled;
    if (g_camera_motion_divisor_00603ac4 < environ_record->vector_24.Length()) {
        environ_record->vector_24.SetLength(g_camera_motion_divisor_00603ac4);
    }
    environ_record->AddScaledMotion00421850(&level->vector_a0);

    if (level->camera_scale_14 == g_float_005ebb34) {
        level->vector_64.SetZero();
    } else {
        level->vector_64 = level->vector_a0 / static_cast<double>(level->camera_scale_14);
    }

    environ_delta = level->vector_a0;
    camera_position.x = adjusted_position.x;
    camera_position.y =
        (g_world_scale_005ebc40 - g_environ_00652DB4->value_30) + adjusted_position.y;
    camera_position.z = adjusted_position.z;
    motion_delta = environ_delta;
    adjusted_position.y = camera_position.y;

    if (m_iNumTriggers != 0) {
        if (bits_5c == 0) {
            IntegrateTriggers();
        } else {
            bits_5c->ClearAll();
        }
    }

    level = g_level_data_00652dac;
    nearest_distance = 1.0e8f;
    hit_position.Set(0.0f, 0.0f, 0.0f);
    level->secondary_contact_prop_id = -1;
    level->primary_contact_prop_id = -1;
    level->flags &= ~2u;
    level->contact_normal_ac.x = 0.0f;
    level->contact_normal_ac.y = 0.0f;
    level->contact_normal_ac.z = 0.0f;
    collision_count = 0;
    level->contact_facing_1c = 0.0f;
    first_pass = 1;
    nearest_surface = 0;
    forced_exit = 0;
    exhausted = 0;
    attempt = 0;

    for (;;) {
        attempt = attempt + 1;
        if (attempt < 6) {
            g_environment_motion_active_00603ad1 = 1;
            probe_position = camera_position;
            first_pass = 1;
            nearest_surface = 0;
            nearest_distance = 1.0e8f;
            if (geometry_index_00 == 0) {
                if (positional_04 != 0) {
                    if (attempt < 3) {
                        ProbeMonstersAlongMotion0041BD60(&motion_delta, &probe_position, 1);
                    }
                    nearest_surface = ProbePropsAlongMotion0041B770(&motion_delta, &probe_position,
                                                                    &scratch, &nearest_distance);
                    prop_hit = nearest_surface != 0;
                    if (prop_hit != 0) {
                        hit_position = probe_position;
                    }
                    probe_position = camera_position;
                    hit_count = positional_04->CollectObjectsAlongSegment(
                        &octree_hits, &camera_position, &motion_delta, 1000.0f, 3);
                } else {
                    hit_count = 0;
                }
            } else {
                geometry_to = motion_delta;
                geometry_from = camera_position;
                nearest_surface = 0;
                hit_count = geometry_index_00->CollectObjectsAlongSegment00446D80(
                    &geometry_hits, &geometry_from, &geometry_to, 1.57079637f, 2000.0f, 3);
            }
            if (((hit_count == 0 && prop_hit == 0) || g_environment_motion_active_00603ad1 == 0)) {
                exhausted = 1;
            }
        } else {
            g_environment_motion_active_00603ad1 = 0;
            exhausted = 1;
            forced_exit = 1;
            probe_position = camera_position;
            if ((g_level_data_00652dac->flags & 2) != 0 &&
                g_level_data_00652dac->ToggleBoundProps0041FF00() == 0) {
                environ_delta.Set(0.0f, 0.0f, 0.0f);
            }
        }

        while ((hit_count != 0 || prop_hit != 0) && g_environment_motion_active_00603ad1 != 0) {
            if (hit_count != 0) {
                probe_position = camera_position;
                for (index = 0; index < hit_count; ++index) {
                    if (positional_04 == 0) {
                        surface =
                            reinterpret_cast< // reinterpret-ok: geometry collect stores surface*
                                W8GDSurface*>(geometry_hits[index]);
                    } else {
                        surface = &m_pSurfaces[octree_hits[index]];
                    }
                    surface->hit_plane_38 = 0;
                    if ((surface->flags_00 & 0x1080) == 0 &&
                        surface->TestSegment0041CF90(&probe_position, &motion_delta, &hit_distance,
                                                     m_pVertices) != 0) {
                        if (hit_distance < nearest_distance) {
                            nearest_distance = hit_distance;
                            hit_position = probe_position;
                            nearest_surface = surface;
                        }
                        probe_position = camera_position;
                    }
                }
                if (first_pass != 0) {
                    if (nearest_surface == 0) {
                        exhausted = 1;
                    }
                    first_pass = 0;
                }
            }

            if (nearest_surface == 0) {
                hit_count = 0;
                prop_hit = 0;
            } else {
                g_environment_motion_active_00603ad1 = nearest_surface->ResolveCollision0041DC10(
                    &camera_position, &hit_position, &motion_delta, collision_count);
                probe_position.x = (camera_position.x + motion_delta.x) - adjusted_position.x;
                probe_position.y = (camera_position.y + motion_delta.y) - adjusted_position.y;
                probe_position.z = (camera_position.z + motion_delta.z) - adjusted_position.z;
                collisions[collision_count] = nearest_surface;
                collision_count = collision_count + 1;
                if (99 < collision_count) {
                    srAssertFail("lCollisions < 100",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 0x253, 0);
                }
                if (g_environment_motion_active_00603ad1 == 0) {
                    hit_count = 0;
                    prop_hit = 0;
                } else {
                    probe_position = camera_position;
                    nearest_surface = 0;
                    nearest_distance = 1.0e8f;
                    if (positional_04 == 0) {
                        if (geometry_index_00 != 0) {
                            geometry_hits = 0;
                            hit_count = geometry_index_00->CollectObjectsAlongSegment00446D80(
                                &geometry_hits, &geometry_from, &geometry_to, 1.57079637f, 2000.0f,
                                3);
                        } else {
                            hit_count = 0;
                        }
                    } else {
                        if (attempt < 3) {
                            ProbeMonstersAlongMotion0041BD60(&motion_delta, &probe_position, 1);
                        }
                        nearest_surface = ProbePropsAlongMotion0041B770(
                            &motion_delta, &probe_position, &scratch, &nearest_distance);
                        prop_hit = nearest_surface != 0;
                        if (prop_hit != 0) {
                            hit_position = probe_position;
                        }
                        probe_position = camera_position;
                        hit_count = positional_04->CollectObjectsAlongSegment(
                            &octree_hits, &camera_position, &motion_delta, 1000.0f, 3);
                    }
                }
            }
        }

        if (exhausted != 0) {
            if (positional_04 != 0) {
                probe_position = adjusted_position;
                motion_delta = environ_delta;
                hit_count = positional_04->CollectObjectsAlongSegment(
                    &octree_hits, &adjusted_position, &environ_delta, 1000.0f, 3);
                crossed_count = 0;
                for (index = 0; index < hit_count; ++index) {
                    surface = &m_pSurfaces[octree_hits[index]];
                    if ((surface->flags_00 & 0x1080) != 0 &&
                        surface->TestSegment0041CF90(&probe_position, &motion_delta, &hit_distance,
                                                     m_pVertices) != 0) {
                        if (0 < crossed_count) {
                            for (int swap = 0; swap < crossed_count; ++swap) {
                                W8GDSurface* prior = collisions[swap];
                                if (fabsf(surface->distance_34) < fabsf(prior->distance_34)) {
                                    collisions[swap] = surface;
                                    surface = prior;
                                }
                            }
                        }
                        collisions[crossed_count] = surface;
                        crossed_count = crossed_count + 1;
                    }
                }
                for (index = 0; index < crossed_count; ++index) {
                    ProcessCrossedSurface(collisions[index]);
                }
            }

            level = g_level_data_00652dac;
            g_environment_motion_active_00603ad1 = 0;
            motion_length = environ_delta.Length();
            if (motion_length <= g_float_005ebc3c ||
                (fabsf(environ_delta.y + motion_length) / motion_length <=
                     g_camera_snap_epsilon_005ebc2c &&
                 g_negative_one_005ebc38 <= environ_delta.y)) {
                environ_delta.Set(0.0f, 0.0f, 0.0f);
            } else {
                g_environment_motion_active_00603ad1 = 1;
            }

            scale = static_cast<float>(g_double_005ebc30) / g_level_data_00652dac->camera_scale_14;
            level->vector_64.x = environ_delta.x * scale;
            level->vector_64.y = environ_delta.y * scale;
            level->vector_64.z = environ_delta.z * scale;
            level->vector_a0 = environ_delta;
            if (g_environment_motion_active_00603ad1 == 0) {
                if ((g_level_data_00652dac->flags & 4) != 0 && forced_exit == 0) {
                    g_environ_ground_latch_00652db8 = 1;
                }
            } else {
                g_environ_ground_latch_00652db8 = 0;
            }
            g_environ_00652DB4->value_04 = g_environ_ground_latch_00652db8;

            if (m_iNumTriggers != 0) {
                index = bits_58->NextSetBit(1);
                while (index != 0) {
                    unsigned int trigger_index = static_cast<unsigned int>(index - 1);
                    if (bits_5c->Test(trigger_index) == 0) {
                        if (m_ppTriggers != 0) {
                            m_ppTriggers[trigger_index]->FinishAction();
                        }
                        bits_58->Clear(trigger_index);
                    }
                    index = bits_58->NextSetBit(0);
                }
            }
            return g_environment_motion_active_00603ad1;
        }

        environ_delta.x = (camera_position.x + motion_delta.x) - adjusted_position.x;
        environ_delta.y = (camera_position.y + motion_delta.y) - adjusted_position.y;
        environ_delta.z = (camera_position.z + motion_delta.z) - adjusted_position.z;
        camera_position = adjusted_position;
        if (0 < collision_count) {
            for (index = 0; index < collision_count; ++index) {
                collisions[index]->flags_00 &= ~8u;
            }
        }
        collision_count = 0;
        motion_delta = environ_delta;
    }
}

/* 0x005EBB34: one float constant with two independent readings - the level
   vector's "no value" here, and Controls.cpp's own range start. Neither is
   proven, so it keeps its address. */
/* Run the buffered prop id list through TestProp and report the id of the
   last prop that hit; an empty list reports -1. */
// FUNCTION: WIZ8 0x0041c0d0
int W8GameData::TestPropSurfaces(int count, unsigned long* ids, W8OctreeTrace* trace,
                                 char skip_flag, char gate)
{
    int last_hit = -1;
    if (count == 0) {
        return -1;
    }
    do {
        if (TestProp(*ids, trace, skip_flag, gate) != 0) {
            last_hit = *ids;
        }
        ++ids;
        --count;
    } while (count != 0);
    return last_hit;
}

/* Point the shared surface/vertex arrays at one collidable prop's GDProp
   tables, ray-test them and put the arrays back. Without a pre-tree the prop
   comes out of the world's collidable list, its pathing representation is
   built on demand, and the ray is moved into prop space through the prop's
   position delta; on a hit the caller's record is reseeded from the start to
   the world-space contact. `gate` skips flag-4 props when set. */
// FUNCTION: WIZ8 0x0041c140
unsigned char W8GameData::TestProp(int prop_id, W8OctreeTrace* trace, char skip_flag, char gate)
{
    W8GDSurface* saved_surfaces = m_pSurfaces;
    srVector3T<float>* saved_vertices = m_pVertices;
    unsigned char hit = 0;
    GDProp* gd_prop;
    W8Prop* prop;

    if (g_oct_pre_tree_659c74 == 0) {
        prop = *g_world->collidable_props->GetAt(prop_id);
        gd_prop = prop->m_gd_prop;
        prop->flags_1c |= 0x10;
        if (gd_prop == 0) {
            prop->BuildOrRefreshPathingRepresentation();
            gd_prop = prop->m_gd_prop;
        }
    } else {
        gd_prop = static_cast<GDProp*>(*g_oct_pre_tree_659c74->props_3b8->GetAt(prop_id));
    }
    m_pSurfaces = gd_prop->m_pGDSurfaces;
    m_pVertices = gd_prop->m_pVertices;
    if (g_oct_pre_tree_659c74 == 0) {
        if (gate == 0 || (gd_prop->m_flags_00 & 4) == 0) {
            srVector3T<float> start = trace->start_00;
            srVector3T<float> end = trace->end_0c;
            srVector3T<float> delta;
            prop->GetDelta0044E130(&delta, &start);
            end.x -= delta.x;
            end.y -= delta.y;
            end.z -= delta.z;
            W8OctreeTrace prop_trace(&start, &end);
            hit = TestTraceResult(gd_prop->m_surface_count_14, 0, &prop_trace, skip_flag, 0);
            if (hit != 0) {
                end.x = prop_trace.end_0c.x + delta.x;
                end.y = prop_trace.end_0c.y + delta.y;
                end.z = prop_trace.end_0c.z + delta.z;
                trace->Reseed(&start, &end);
            }
        }
    } else {
        hit = TestTraceResult(gd_prop->m_surface_count_14, 0, trace, skip_flag, 0);
    }
    m_pSurfaces = saved_surfaces;
    m_pVertices = saved_vertices;
    return hit;
}

/* Ray the trace record against `count` surfaces: all of m_pSurfaces in order
   when `surface_ids` is null, else just the listed indexes. The value_88 flag
   admits flag-4 surfaces only, 0x1080-marked surfaces are skipped outright,
   `skip_flag` drops 0x8000-marked ones, and a nonzero positional_44 needs a
   passing `mode` roll. Each accepted surface's plane is tested both sides of
   the segment; a point-in-triangle pass on the contact keeps the closest hit,
   storing index_04 into value_54, the contact into end_0c and the hit
   distance into hit_limit_24/length_28. */
// FUNCTION: WIZ8 0x0041c330
char W8GameData::TestTraceResult(int count, unsigned long* surface_ids, W8OctreeTrace* trace,
                                 char skip_flag, int mode)
{
    char hit = 0;
    float best_x;
    float best_y;
    float best_z;

    value_54 = 0;
    if (count != 0) {
        unsigned long* id = surface_ids;
        int index = 0;
        int remaining = count;
        do {
            W8GDSurface* surface;
            if (surface_ids == 0) {
                surface = m_pSurfaces + index;
            } else {
                surface = m_pSurfaces + *id;
            }
            if (((value_88 == 0 || (surface->flags_00 & 4) != 0) &&
                 (surface->flags_00 & 0x1080) == 0 &&
                 (skip_flag == 0 || (surface->flags_00 & 0x8000) == 0)) &&
                (surface->positional_44 == 0 ||
                 (mode != -1 && (mode < 2 || static_cast<int>(surface->positional_44) < mode) &&
                  (mode != 1 ||
                   (static_cast<int>(surface->positional_44) < 100 &&
                    static_cast<int>(surface->positional_44) < static_cast<int>(Random(100)))))) &&
                surface->plane_24[0] * trace->step_18.x + surface->plane_24[1] * trace->step_18.y +
                        surface->plane_24[2] * trace->step_18.z <=
                    g_float_005ebb34) {
                float hit_distance = surface->plane_24[0] * trace->start_00.x +
                                     surface->plane_24[1] * trace->start_00.y +
                                     surface->plane_24[2] * trace->start_00.z +
                                     surface->plane_24[3];
                if (hit_distance <= trace->hit_limit_24 && g_float_005ebb34 < hit_distance) {
                    srVector3T<float> contact;
                    if (g_float_005ebb38 <= hit_distance) {
                        float back = surface->plane_24[0] * trace->end_0c.x +
                                     surface->plane_24[1] * trace->end_0c.y +
                                     surface->plane_24[2] * trace->end_0c.z + surface->plane_24[3];
                        if (g_float_005ebb38 <= back) {
                            goto next;
                        }
                        back = -back;
                        if (g_float_005ebb38 <= back || trace->hit_limit_24 < trace->length_28) {
                            hit_distance =
                                (hit_distance / (back + hit_distance)) * trace->length_28;
                            contact.x = trace->step_18.x * hit_distance + trace->start_00.x;
                            contact.y = trace->step_18.y * hit_distance + trace->start_00.y;
                            contact.z = trace->step_18.z * hit_distance + trace->start_00.z;
                        } else {
                            contact = trace->end_0c;
                            hit_distance = trace->length_28;
                        }
                    } else {
                        contact = trace->start_00;
                    }
                    srVector3T<float> vertices[3];
                    vertices[0] = m_pVertices[surface->vertex_indices_18[0]];
                    vertices[1] = m_pVertices[surface->vertex_indices_18[1]];
                    vertices[2] = m_pVertices[surface->vertex_indices_18[2]];
                    if (PointInsideTriangle0046D530(vertices, surface->flags_00 & 3, &contact) !=
                            0 &&
                        hit_distance < trace->hit_limit_24) {
                        value_54 = surface->index_04;
                        hit = 1;
                        trace->hit_limit_24 = hit_distance;
                        best_y = contact.y;
                        best_z = contact.z;
                        best_x = contact.x;
                    }
                }
            }
        next:
            /* Retail verified at 0x0041C627: the cursor advances
               unconditionally even when `surface_ids` is null, so `++id` on a
               null pointer is the retail behavior rather than a defect. */
            ++id;
            ++index;
            --remaining;
        } while (remaining != 0);
        if (hit != 0) {
            trace->end_0c.x = best_x;
            trace->end_0c.y = best_y;
            trace->end_0c.z = best_z;
            trace->length_28 = trace->hit_limit_24;
            return hit;
        }
    }
    return 0;
}

/* Selects a switch interface's state: the matching state group's conditional
   polygons clear surface flag 0x10 while every other state's polygons set
   it. Asserts the interface id and the state slice bounds. */
// FUNCTION: WIZ8 0x0041c680
char W8GameData::SetInterfaceState(int iID, int state)
{
    int iStates;
    int poly_count;
    W8GDInterface* interface_rec;
    W8GDInterfaceState* states;
    int* polys;

    if (iID == 0 || m_iNumInterfaces <= iID) {
        srAssertFail("iID && iID < m_iNumInterfaces",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 0x4e0, 0);
    }
    interface_rec = m_pInterfaces + iID;
    iStates = interface_rec->state_count_04;
    if (m_iNumStates < interface_rec->iStates + iStates) {
        srAssertFail("(m_pInterfaces[iID].iStates + iStates) <= m_iNumStates",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 0x4e2, 0);
    }
    states = m_pStates + interface_rec->iStates;
    for (; iStates != 0; --iStates) {
        poly_count = states->poly_count_04;
        polys = m_piCondPolys + states->poly_first_08;
        if (states->group_00 == state) {
            for (; poly_count != 0; --poly_count) {
                m_pSurfaces[*polys].flags_00 &= ~0x10;
                ++polys;
            }
        } else {
            for (; poly_count != 0; --poly_count) {
                m_pSurfaces[*polys].flags_00 |= 0x10;
                ++polys;
            }
        }
        ++states;
    }
    return 1;
}

/* Apply one surface the mover's trace crossed, nearest first: an environment
   boundary (flag 0x1000) swaps the active environment record, carrying the
   live blend fields across; a trigger surface runs its trigger in the
   crossing direction unless the bit sets already hold it. */
// FUNCTION: WIZ8 0x0041c770
void W8GameData::ProcessCrossedSurface(W8GDSurface* surface)
{
    W8EnvironRecord* record;
    W8EnvironRecord* current;
    Trigger* trigger;
    int direction;

    if ((surface->flags_00 & 0x1000) != 0) {
        if (surface->distance_34 > 0.0f) {
            current = g_environ_00652DB4;
            record = m_ppEnvirons[surface->trigger_index_08];
            record->value_04 = current->value_04;
            record->value_05 = current->value_05;
            record->value_08 = current->value_08;
            record->scale_0c = current->scale_0c;
            record->value_20 = current->value_20;
            record->vector_24 = current->vector_24;
            g_environ_00652DB4 = record;
            return;
        }
        current = g_environ_00652DB4;
        if (current == m_ppEnvirons[surface->trigger_index_08]) {
            record = m_ppEnvirons[0];
            record->value_04 = current->value_04;
            record->value_05 = current->value_05;
            record->value_08 = current->value_08;
            record->scale_0c = current->scale_0c;
            record->value_20 = current->value_20;
            record->vector_24 = current->vector_24;
            g_environ_00652DB4 = record;
        }
        return;
    }
    direction = 1;
    if (surface->distance_34 < 0.0f) {
        direction = -1;
    }
    if (m_ppTriggers == 0 || bits_5c->Set(surface->trigger_index_08) != 0 ||
        bits_58->Set(surface->trigger_index_08) != 0) {
        return;
    }
    trigger = m_ppTriggers[surface->trigger_index_08];
    if (trigger->initial_action_22a == 0x10 && surface->value_40 > 0.0f) {
        surface->value_40 = 0.0f;
        if (direction == 1) {
            bits_5c->Clear(surface->trigger_index_08);
            bits_58->Clear(surface->trigger_index_08);
            return;
        }
    }
    if (direction != 0) {
        trigger->Run(direction);
        if (trigger->action_state_232 == 1 || trigger->action_state_232 == 4) {
            bits_5c->Clear(surface->trigger_index_08);
            bits_58->Clear(surface->trigger_index_08);
        }
    }
}

/* Builds the octree trace model and answers its scene node. Every surface
   becomes one polygon whose three vertices copy the surface's corner
   positions; flag-4 surfaces mark their vertices in the bit set. The DIG pass
   then writes a direction vector per vertex: marked vertices keep the surface
   normal (flattened to point straight up when it tilts below 0.5), unmarked
   vertices get a scaled/biased horizontal direction renormalized to 0.3. */
// FUNCTION: WIZ8 0x0041c930
stModelInstance* W8GameData::CreateTraceModel0041C930()
{
    BitArray selected(m_iNumSurfaces * 3 + 10);
    stMeshModel* mesh = new stMeshModel(m_iNumSurfaces, m_iNumSurfaces * 3);
    if (mesh == 0) {
        srAssertFail("pstMeshModel", "C:\\Projects\\Wizardry 8\\Engine Code\\GameData.cpp", 0x56d,
                     "ModelGameData::Read -- Could not create pstMeshModel.\n");
    }
    mesh->autoRelease();
    mesh->flags_3a0 &= ~1U;
    srVector3i* poly_vertices = mesh->getPolyVertex();
    srPtr<srTextureIFace>* poly_textures = mesh->getPolyTexture(0, 0, 1);
    srVector3T<float>* vertex_locs = mesh->getVertexLoc();
    srVector2T<float>* texcoords = mesh->getVertexTexCoords(0, 0, 1);
    srPtr<srMaterialIFace>* vertex_materials =
        mesh->getVertexMaterial(0, static_cast<srMeshModel::e_side>(0), 1);
    unsigned long* shade_indices = mesh->getVertexShadeIndex(1);
    int vertex = 0;
    for (int index = 0; index < m_iNumSurfaces; ++index) {
        W8GDSurface* surface = m_pSurfaces + index;
        poly_textures[index] = g_path_texture_00652dc0;
        if ((surface->flags_00 & 4) != 0) {
            selected.Set(vertex);
            selected.Set(vertex + 1);
            selected.Set(vertex + 2);
        }
        texcoords[vertex].x = 0.0f;
        texcoords[vertex].y = 0.0f;
        vertex_materials[vertex] = g_path_material_00652dbc;
        poly_vertices[index].x = vertex;
        shade_indices[vertex] = vertex;
        vertex_locs[vertex] = m_pVertices[surface->vertex_indices_18[0]];
        texcoords[vertex + 1].x = 0.0f;
        texcoords[vertex + 1].y = 0.0f;
        vertex_materials[vertex + 1] = g_path_material_00652dbc;
        poly_vertices[index].y = vertex + 1;
        shade_indices[vertex + 1] = vertex + 1;
        vertex_locs[vertex + 1] = m_pVertices[surface->vertex_indices_18[1]];
        texcoords[vertex + 2].x = 0.0f;
        texcoords[vertex + 2].y = 0.0f;
        vertex_materials[vertex + 2] = g_path_material_00652dbc;
        poly_vertices[index].z = vertex + 2;
        shade_indices[vertex + 2] = vertex + 2;
        vertex_locs[vertex + 2] = m_pVertices[surface->vertex_indices_18[2]];
        vertex += 3;
    }
    srShader shader;
    CopyLevelDataHandle(&shader.value, &g_path_shader_00652dc4.value);
    mesh->setShader(shader, 0);
    if ((mesh->control_state_390 & 8) == 0) {
        mesh->control_state_390 |= 8;
        mesh->control_state_390 |= 8;
    }
    srVector3T<float>* normals = mesh->getVertexNormal();
    srVector3T<float>* dig = mesh->getVertexDIG(0, 1);
    for (int i = 0; i < vertex; ++i) {
        if (selected.Test(i)) {
            dig[i] = normals[i];
            if (dig[i].y < g_float_005ebc7c) {
                dig[i].y = 0.0f;
                dig[i].Normalize();
                dig[i].y = 1.0f;
                dig[i].Normalize();
            }
        } else {
            dig[i].x = normals[i].x * g_float_005ebc78 + g_float_005ebc78;
            dig[i].y = 0.0f;
            dig[i].z = normals[i].z * g_float_005ebc78 + g_float_005ebc78;
            float length = dig[i].Length();
            if (g_double_005ebc70 <= length) {
                if (length < 0.3) {
                    dig[i].SetLength(0.3);
                }
            } else {
                dig[i].x = 0.15f;
                dig[i].z = 0.15f;
            }
        }
    }
    mesh->setName("GameData_Mesh");
    mesh->flag_3cc = 0;
    mesh->flags_3a0 &= ~2U;
    stModelInstance* instance = CreateModelInstance0046F5C0(mesh);
    instance->setName("GameData_Mesh");
    return instance;
}

/* Copy one four-byte handle over another. */
// FUNCTION: WIZ8 0x0041cf80
void CopyLevelDataHandle(unsigned long* destination, const unsigned long* source)
{
    *destination = *source;
}

// GLOBAL: WIZ8 0x005ebc80
const float g_float_005ebc80 = -0.1f;
// GLOBAL: WIZ8 0x005ebc8c
const float g_float_005ebc8c = 150.0f;
// GLOBAL: WIZ8 0x005ebc94
const float g_float_005ebc94 = 165.0f;
// GLOBAL: WIZ8 0x005ebc9c
const float g_float_005ebc9c = -0.8f;
// GLOBAL: WIZ8 0x005ebca8
const double g_double_005ebca8 = 0.0010000000474974513;
// GLOBAL: WIZ8 0x005ebcb0
const float g_float_005ebcb0 = 0.99999f;
// GLOBAL: WIZ8 0x005ebcb4
const float g_float_005ebcb4 = -0.999f;
// GLOBAL: WIZ8 0x005ebcb8
const float g_float_005ebcb8 = -0.5f;
// GLOBAL: WIZ8 0x005ebcc0
const double g_double_005ebcc0 = 0.33333298563957214;

static unsigned char SegmentCrossesEdge0041D7A0(const float* seg_start, const float* seg_end,
                                                const float* edge_a, const float* edge_b,
                                                unsigned int axis);

/* Clip the motion segment against this surface's plane and triangle. On a hit
   `from` advances to the contact point, `hit_distance` returns the travelled
   length and distance_34 takes the surface's updated limit. Ordinary surfaces
   gate on the contact band below distance_34; flag-0x1080 surfaces (special)
   instead test whether the segment crossed the plane, and lift `from` by the
   level-flag-8 camera offset. */
// FUNCTION: WIZ8 0x0041CF90
unsigned char W8GDSurface::TestSegment0041CF90(srVector3T<float>* from,
                                               const srVector3T<float>* direction,
                                               float* hit_distance, srVector3T<float>* vertices)
{
    unsigned int flags = flags_00;
    unsigned char special = 0;
    unsigned char crossed = 0;
    bool inside = false;
    if ((flags & 0x18) != 0) {
        return 0;
    }
    srVector3T<float> point = *from;
    if ((flags & 0x1080) != 0) {
        special = 1;
        if ((g_level_data_00652dac->flags & 8) != 0) {
            point.y += g_float_005ebc94;
        }
    }
    srVector3T<float> unit_dir = *direction;
    unit_dir.Normalize();
    srVector3T<float> normal;
    normal.x = plane_24[0];
    normal.y = plane_24[1];
    normal.z = plane_24[2];
    float segment_length = direction->Length();
    if (segment_length < g_float_005ebc90) {
        if (special == 0) {
            g_environment_motion_active_00603ad1 = special;
            return 0;
        }
    } else if (special == 0 &&
               normal.x * unit_dir.x + unit_dir.y * normal.y + unit_dir.z * normal.z >=
                   g_float_005ebb34) {
        return 0;
    }
    srVector3T<float> end = point + *direction;
    float dist_start =
        point.x * plane_24[0] + point.y * plane_24[1] + point.z * plane_24[2] + plane_24[3];
    if (special == 0 && dist_start < g_float_005ebb34) {
        return 0;
    }
    float limit = value_40;
    if ((flags & 4) == 0 && special == 0) {
        limit = value_40 - (g_float_005ebb38 - fabsf(normal.y)) * g_float_005ebc8c;
    }
    float dist_end = end.x * plane_24[0] + end.y * plane_24[1] + end.z * plane_24[2] + plane_24[3];
    if (special != 0) {
        if (dist_end * dist_start > g_camera_snap_epsilon_005ebc2c) {
            if (value_40 < g_float_005ebb38 || dist_end < g_float_005ebb34) {
                return 0;
            }
            if (dist_end >= limit + g_float_005ebc88 && dist_start >= limit + g_float_005ebc88) {
                return 0;
            }
        }
    } else {
        if (limit + g_float_005ebc88 <= dist_end) {
            return 0;
        }
        if (dist_start - dist_end < g_camera_transition_epsilon_005ebc84) {
            return 0;
        }
    }
    int axis = flags & 3;
    short comp_u = (axis + 1) % 3;
    short comp_v = (axis + 2) % 3;
    float start_proj[3];
    start_proj[0] = point.x - normal.x * dist_start;
    start_proj[1] = point.y - normal.y * dist_start;
    start_proj[2] = point.z - normal.z * dist_start;
    float end_proj[3];
    end_proj[0] = end.x - normal.x * dist_end;
    end_proj[1] = end.y - normal.y * dist_end;
    end_proj[2] = end.z - normal.z * dist_end;
    float verts[9];
    verts[0] = vertices[vertex_indices_18[0]].x;
    verts[1] = vertices[vertex_indices_18[0]].y;
    verts[2] = vertices[vertex_indices_18[0]].z;
    verts[3] = vertices[vertex_indices_18[1]].x;
    verts[4] = vertices[vertex_indices_18[1]].y;
    verts[5] = vertices[vertex_indices_18[1]].z;
    verts[6] = vertices[vertex_indices_18[2]].x;
    verts[7] = vertices[vertex_indices_18[2]].y;
    verts[8] = vertices[vertex_indices_18[2]].z;
    for (short edge = 0; edge < 3; ++edge) {
        if (crossed != 0) {
            break;
        }
        short next = (edge + 1) % 3;
        if (SegmentCrossesEdge0041D7A0(start_proj, end_proj, verts + edge * 3, verts + next * 3,
                                       axis) != 0) {
            crossed = 1;
        }
        float edge_low = verts[edge * 3 + comp_v];
        float edge_high = verts[next * 3 + comp_v];
        if ((edge_low <= end_proj[comp_v] && end_proj[comp_v] < edge_high) ||
            (edge_high <= end_proj[comp_v] && end_proj[comp_v] < edge_low)) {
            float crossing = (verts[next * 3 + comp_u] - verts[edge * 3 + comp_u]) *
                                 (end_proj[comp_v] - edge_low) / (edge_high - edge_low) +
                             verts[edge * 3 + comp_u];
            if (crossing > end_proj[comp_u]) {
                inside = !inside;
            }
        }
    }
    float t_span = dist_start - dist_end;
    float fraction;
    if (t_span < g_float_005ebb38) {
        fraction = g_float_005ebb38;
    } else {
        fraction = (dist_start - limit) / t_span;
    }
    if (fraction < g_float_005ebc80 && special == 0) {
        fraction = g_float_005ebc80;
    }
    if (fraction > g_float_005ebb38) {
        return 0;
    }
    float hit;
    if (crossed == 0 && inside == 0) {
        if (special != 0) {
            return 0;
        }
        srVector3T<float> chosen;
        if (dist_start / t_span <= g_float_005ebb38) {
            chosen = point;
        } else {
            chosen = end;
        }
        if (ClampHitToEdge0041D9D0(&chosen, vertices, &limit) == 0) {
            return 0;
        }
        fraction = (dist_start - limit) / t_span;
        if (fraction >= g_float_005ebb38) {
            return 0;
        }
        if (fraction < g_float_005ebc80) {
            fraction = g_float_005ebc80;
        }
        hit = fraction * segment_length;
        if (segment_length - hit < g_camera_transition_epsilon_005ebc84) {
            return 0;
        }
        *hit_distance = hit;
        distance_34 = limit;
        *from = point + unit_dir * hit;
        return 1;
    }
    hit = fraction * segment_length;
    *hit_distance = hit;
    if (special == 0) {
        distance_34 = limit;
        *from = point + unit_dir * hit;
        return 1;
    }
    distance_34 = hit;
    if (dist_start <= g_float_005ebb34) {
        distance_34 = -fabsf(hit);
    } else {
        distance_34 = fabsf(hit);
    }
    *hit_distance = fabsf(*hit_distance);
    return 1;
}

/* 2D segment-vs-edge test in the plane perpendicular to `axis`: the projected
   motion segment seg_start→seg_end must overlap edge_a→edge_b on both free
   axes and their line-crossing parameters must both fall inside [0,1]. */
// FUNCTION: WIZ8 0x0041D7A0
static unsigned char SegmentCrossesEdge0041D7A0(const float* seg_start, const float* seg_end,
                                                const float* edge_a, const float* edge_b,
                                                unsigned int axis)
{
    unsigned int comp_u = (axis + 1) % 3;
    unsigned int comp_v = (axis + 2) % 3;
    float seg_du = seg_end[comp_u] - seg_start[comp_u];
    float edge_du = edge_a[comp_u] - edge_b[comp_u];
    float seg_u_low;
    float seg_u_high;
    if (seg_du <= g_float_005ebb34) {
        seg_u_low = seg_end[comp_u];
        seg_u_high = seg_start[comp_u];
    } else {
        seg_u_low = seg_start[comp_u];
        seg_u_high = seg_end[comp_u];
    }
    float edge_u_low;
    float edge_u_high;
    if (edge_du <= g_float_005ebb34) {
        edge_u_low = edge_a[comp_u];
        edge_u_high = edge_b[comp_u];
    } else {
        edge_u_low = edge_b[comp_u];
        edge_u_high = edge_a[comp_u];
    }
    if (seg_u_low <= edge_u_high && edge_u_low <= seg_u_high) {
        float seg_dv = seg_end[comp_v] - seg_start[comp_v];
        float edge_dv = edge_a[comp_v] - edge_b[comp_v];
        float seg_v_low;
        float seg_v_high;
        if (seg_dv <= g_float_005ebb34) {
            seg_v_low = seg_end[comp_v];
            seg_v_high = seg_start[comp_v];
        } else {
            seg_v_low = seg_start[comp_v];
            seg_v_high = seg_end[comp_v];
        }
        float edge_v_low;
        float edge_v_high;
        if (edge_dv <= g_float_005ebb34) {
            edge_v_low = edge_a[comp_v];
            edge_v_high = edge_b[comp_v];
        } else {
            edge_v_low = edge_b[comp_v];
            edge_v_high = edge_a[comp_v];
        }
        if (seg_v_low <= edge_v_high && edge_v_low <= seg_v_high) {
            float rel_u = seg_start[comp_u] - edge_a[comp_u];
            float rel_v = seg_start[comp_v] - edge_a[comp_v];
            float side_start = rel_u * edge_dv - rel_v * edge_du;
            float denom = seg_dv * edge_du - edge_dv * seg_du;
            if (denom <= g_float_005ebb34) {
                if (side_start > g_float_005ebb34 || side_start < denom) {
                    return 0;
                }
            } else {
                if (side_start < g_float_005ebb34 || side_start > denom) {
                    return 0;
                }
            }
            float side_end = rel_v * seg_du - rel_u * seg_dv;
            if (denom <= g_float_005ebb34) {
                if (side_end <= g_float_005ebb34 && side_end >= denom) {
                    return 1;
                }
            } else if (side_end >= g_float_005ebb34) {
                if (side_end > denom) {
                    return 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

/* Nearest-edge distance fixup for a segment that hit this surface's plane but
   missed the projected triangle. `limit` (value_40's contact margin) shrinks
   to the remaining in-plane travel before the nearest edge; fails when no
   edge improves it. */
// FUNCTION: WIZ8 0x0041D9D0
unsigned char W8GDSurface::ClampHitToEdge0041D9D0(const srVector3T<float>* point,
                                                  const srVector3T<float>* vertices, float* limit)
{
    float nearest_dist = 10000000.0f;
    short nearest_edge = 0;
    for (short edge = 0; edge < 3; ++edge) {
        srVector3T<float> probe = *point;
        float dist =
            PointToSegmentDistance00437540(&probe, vertices + vertex_indices_18[edge],
                                           vertices + vertex_indices_18[(edge + 1) % 3], 0, 0);
        if (dist < nearest_dist) {
            nearest_edge = edge;
            nearest_dist = dist;
        }
    }
    if (nearest_dist > *limit) {
        return 0;
    }
    float offset =
        -(point->x * plane_24[0] + point->y * plane_24[1] + point->z * plane_24[2] + plane_24[3]);
    srVector3T<float> projected;
    projected.x = plane_24[0] * offset + point->x;
    projected.y = plane_24[1] * offset + point->y;
    projected.z = plane_24[2] * offset + point->z;
    float edge_dist =
        PointToSegmentDistance00437540(&projected, vertices + vertex_indices_18[nearest_edge],
                                       vertices + vertex_indices_18[(nearest_edge + 1) % 3], 0, 0);
    if ((flags_00 & 4) != 0) {
        float threshold = *limit * g_float_005ebc7c;
        if (threshold < offset) {
            return 0;
        }
    }
    if ((flags_00 & 4) == 0) {
        if (plane_24[1] < g_float_005ebc9c) {
            edge_dist = edge_dist * g_navigator_linked_radius_scale_005ebc98;
        }
    } else {
        float scaled = *limit * g_navigator_mode3_scale_005ebca4;
        if (scaled < edge_dist) {
            edge_dist = (edge_dist - scaled) * g_float_005ebca0 + scaled;
        }
    }
    if (edge_dist < *limit) {
        *limit = sqrtf(*limit * *limit - edge_dist * edge_dist);
        return 1;
    }
    return 0;
}

/* Collision response for a hit surface: `origin` advances to `hit_point` and
   `direction` is bent along the contact plane; returns whether motion
   continues. The crossed contact planes persist across calls in the statics
   so sequential bounces wedge the slide between them. */
// FUNCTION: WIZ8 0x0041DC10
unsigned char W8GDSurface::ResolveCollision0041DC10(srVector3T<float>* origin,
                                                    const srVector3T<float>* hit_point,
                                                    srVector3T<float>* direction,
                                                    int collision_index)
{
    /* Statics 0x652d50/0x652d68/0x652d80 carry the latched entry direction and
       the first/second contact normals; their atexit thunks are the SYNTHETIC
       markers at 0x41e8d0/0x41e8c0/0x41e8b0. */
    static srVector3T<float> s_entry_direction_00652d50;
    static srVector3T<float> s_second_normal_00652d68;
    static srVector3T<float> s_first_normal_00652d80;
    static short s_collision_state_00652938;
    static float s_second_limit_0065294c;
    static int s_first_surface_00652d78;
    static float s_first_limit_00652da0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
    /* Retail keeps these collision counters as function-local statics but
       never reads them - the stores are recovered behavior, not dead code. */
    static int s_second_surface_00652d60;
    static short s_bounce_count_00652d7c;
#pragma clang diagnostic pop

    unsigned char recomputed = 0;
    unsigned char crossed = 0;
    srVector3T<float> crease_a;
    crease_a = 0.0f;
    if ((flags_00 & 4) != 0) {
        if ((flags_00 & 0x20) != 0 && g_byte_652da6 != 0 && g_flag_652da5 != 0) {
            g_environ_00652DB4->value_20 = 1.0f;
        } else if (g_environ_00652DB4->value_20 < slope_48) {
            g_environ_00652DB4->value_20 = slope_48;
        }
        g_environ_00652DB4->value_05 = 1;
        g_level_data_00652dac->flags |= 0x10;
        g_level_data_00652dac->sound_environment_0c = footstep_surface_3c;
        g_level_data_00652dac->sound_environment_alt_0d = footstep_material_3d;
    }
    float direction_length = direction->Length();
    if (direction_length == g_float_005ebb34) {
        return 0;
    }
    /* ProbePropsAlongMotion stores the prop's hit plane here; level surfaces
       leave it zero and use the embedded plane_24. */
    const srVector4T<float>* override_plane = hit_plane_38;
    srVector3T<float> normal;
    float plane_distance;
    if (override_plane == 0) {
        normal.x = plane_24[0];
        normal.y = plane_24[1];
        normal.z = plane_24[2];
        plane_distance = plane_24[3];
    } else {
        normal.x = override_plane->x;
        normal.y = override_plane->y;
        normal.z = override_plane->z;
        plane_distance = override_plane->w;
    }
    float adjusted_d = plane_distance - distance_34;
    W8LevelDataRecord* level = g_level_data_00652dac;
    if (level->primary_contact_prop_id > -1 && normal.x * level->contact_normal_ac.x +
                                                       normal.y * level->contact_normal_ac.y +
                                                       normal.z * level->contact_normal_ac.z <
                                                   g_float_005ebcb8) {
        level->flags |= 2;
    }
    flags_00 |= 8;
    float depth = -((origin->x + direction->x) * normal.x + (origin->y + direction->y) * normal.y +
                    (origin->z + direction->z) * normal.z + adjusted_d);
    distance_34 = depth;
    if (depth < g_float_005ebb34) {
        return 1;
    }
    srVector3T<float> unit;
    if (collision_index < 1) {
        if (collision_index != 0) {
            double inv_length = g_double_005ebc30 / direction_length;
            unit.Set(direction->x * inv_length, direction->y * inv_length,
                     direction->z * inv_length);
            s_entry_direction_00652d50 = unit;
        }
        s_collision_state_00652938 = 0;
        s_bounce_count_00652d7c = 0;
    }
    ++s_bounce_count_00652d7c;
    srVector3T<float> step = *hit_point - *origin;
    *direction = *direction - step;
    *origin = *hit_point;
    if (normal.z * s_entry_direction_00652d50.z + normal.y * s_entry_direction_00652d50.y +
            s_entry_direction_00652d50.x * normal.x <
        g_float_005ebcb4) {
        direction->x = 0.0f;
        direction->y = 0.0f;
        direction->z = 0.0f;
        g_environ_00652DB4->vector_24 = 0.0f;
        return 0;
    }
    double inv_length = g_double_005ebc30 / direction_length;
    unit.Set(direction->x * inv_length, direction->y * inv_length, direction->z * inv_length);
    srVector3T<float> slide;
    slide.x = normal.x * distance_34 + direction->x;
    slide.y = normal.y * distance_34 + direction->y;
    slide.z = normal.z * distance_34 + direction->z;
    ApplyEnvironContact0041EA90(&slide);
    srVector3T<float> slide_unit = slide;
    slide_unit.Normalize();
    float approach = DotProduct(slide_unit, s_entry_direction_00652d50);
    if (fabsf(approach) < g_camera_snap_epsilon_005ebc2c) {
        direction->x = 0.0f;
        direction->y = 0.0f;
        direction->z = 0.0f;
        return 0;
    }
    if (approach >= g_float_005ebc90) {
        if (s_collision_state_00652938 != 0) {
            srVector3T<float> ortho = s_entry_direction_00652d50;
            float normal_sq = DotProduct(normal, normal);
            if (normal_sq > g_float_005ebc58) {
                ortho = normal * (DotProduct(s_entry_direction_00652d50, normal) / normal_sq);
            }
            srVector3T<float> deflect = s_entry_direction_00652d50 - ortho;
            deflect.Normalize();
            if (DotProduct(unit, deflect) < g_float_005ebb34 && s_collision_state_00652938 == 1 &&
                CentroidsDiverging0041E8E0(s_first_surface_00652d78, &normal,
                                           &s_first_normal_00652d80) != 0) {
                crossed = 1;
            }
        }
    } else if (s_collision_state_00652938 != 0) {
        crossed = 1;
    }
    if (s_collision_state_00652938 != 0) {
        if (g_float_005ebcb0 < DotProduct(normal, s_first_normal_00652d80)) {
            if (s_first_limit_00652da0 <= adjusted_d) {
                return 1;
            }
            s_first_limit_00652da0 = adjusted_d;
            s_first_surface_00652d78 = index_04;
            *direction = slide;
            return 1;
        }
        if (crossed != 0) {
            crease_a = CrossProduct(normal, s_first_normal_00652d80);
            if (DotProduct(crease_a, unit) < g_float_005ebb34) {
                crease_a = -crease_a;
            }
        } else {
            recomputed = 0;
            if (DotProduct(slide_unit, s_first_normal_00652d80) > g_float_005ebc90) {
                s_first_normal_00652d80 = normal;
                s_first_surface_00652d78 = index_04;
                recomputed = 1;
                s_first_limit_00652da0 = adjusted_d;
            }
        }
        if (s_collision_state_00652938 == 2) {
            if (g_float_005ebcb0 < DotProduct(normal, s_second_normal_00652d68)) {
                if (s_second_limit_0065294c <= adjusted_d) {
                    return 1;
                }
                s_second_limit_0065294c = adjusted_d;
                s_second_surface_00652d60 = index_04;
                *direction = slide;
                return 1;
            }
            if (DotProduct(s_second_normal_00652d68 + s_first_normal_00652d80, normal) <
                g_camera_snap_epsilon_005ebc2c) {
                direction->x = 0.0f;
                direction->y = 0.0f;
                direction->z = 0.0f;
                return 0;
            }
            if (crossed != 0) {
                srVector3T<float> crease_b = CrossProduct(normal, s_second_normal_00652d68);
                if (DotProduct(crease_b, unit) < g_camera_transition_epsilon_005ebc84) {
                    crease_b = -crease_b;
                }
                float ahead_a = DotProduct(crease_a, s_entry_direction_00652d50);
                float ahead_b = DotProduct(crease_b, s_entry_direction_00652d50);
                if (ahead_a < g_camera_snap_epsilon_005ebc2c &&
                    ahead_b < g_camera_snap_epsilon_005ebc2c) {
                    direction->x = 0.0f;
                    direction->y = 0.0f;
                    direction->z = 0.0f;
                    return 0;
                }
                if (ahead_b <= ahead_a) {
                    float crease_sq = crease_b.LengthSquared();
                    if (crease_sq > g_float_005ebc58) {
                        slide = crease_b * (DotProduct(slide, crease_b) / crease_sq);
                    }
                    s_first_normal_00652d80 = normal;
                    s_first_limit_00652da0 = adjusted_d;
                    s_first_surface_00652d78 = index_04;
                } else {
                    float crease_sq = DotProduct(crease_a, crease_a);
                    if (crease_sq > g_float_005ebc58) {
                        slide = crease_a * (DotProduct(slide, crease_a) / crease_sq);
                    }
                    s_second_normal_00652d68 = normal;
                    s_second_surface_00652d60 = index_04;
                    s_second_limit_0065294c = adjusted_d;
                }
            } else if (DotProduct(slide_unit, s_second_normal_00652d68) >= g_float_005ebb34) {
                if (recomputed != 0) {
                    s_collision_state_00652938 = 1;
                } else {
                    s_second_normal_00652d68 = normal;
                    s_second_surface_00652d60 = index_04;
                    s_second_limit_0065294c = adjusted_d;
                }
            }
        } else if (crossed != 0) {
            if (DotProduct(crease_a, s_entry_direction_00652d50) < g_camera_snap_epsilon_005ebc2c) {
                direction->x = 0.0f;
                direction->y = 0.0f;
                direction->z = 0.0f;
                return 0;
            }
            ProjectVectorOntoVector00421440(&slide, &crease_a);
            s_second_normal_00652d68 = normal;
            s_collision_state_00652938 = s_collision_state_00652938 + 1;
            s_second_surface_00652d60 = index_04;
            s_second_limit_0065294c = adjusted_d;
        }
    } else {
        s_first_normal_00652d80 = normal;
        s_first_surface_00652d78 = index_04;
        s_collision_state_00652938 = 1;
        s_first_limit_00652da0 = adjusted_d;
    }
    *direction = slide;
    if (direction->Length() >= g_double_005ebca8) {
        return 1;
    }
    return 0;
}

// SYNTHETIC: WIZ8 0x0041E8B0
// `dynamic atexit destructor for 's_first_normal_00652d80''

// SYNTHETIC: WIZ8 0x0041E8C0
// `dynamic atexit destructor for 's_second_normal_00652d68''

// SYNTHETIC: WIZ8 0x0041E8D0
// `dynamic atexit destructor for 's_entry_direction_00652d50''

/* Whether `surface_index`'s triangle centroid sits farther from this surface's
   centroid than the from→to normal offset: used to tell genuinely different
   contact planes apart when wedging a slide. */
// FUNCTION: WIZ8 0x0041E8E0
unsigned char W8GDSurface::CentroidsDiverging0041E8E0(int surface_index,
                                                      const srVector3T<float>* from,
                                                      const srVector3T<float>* to)
{
    const W8GDSurface* other = g_octree_game_data_00652db0->m_pSurfaces + surface_index;
    const srVector3T<float>* vertices = g_octree_game_data_00652db0->m_pVertices;
    float this_x = g_float_005ebb34;
    float this_y = g_float_005ebb34;
    float this_z = g_float_005ebb34;
    float other_x = g_float_005ebb34;
    float other_y = g_float_005ebb34;
    float other_z = g_float_005ebb34;
    for (int vertex = 0; vertex < 3; ++vertex) {
        const srVector3T<float>* this_vertex = vertices + vertex_indices_18[vertex];
        this_x += this_vertex->x;
        this_y += this_vertex->y;
        this_z += this_vertex->z;
        const srVector3T<float>* other_vertex = vertices + other->vertex_indices_18[vertex];
        other_x += other_vertex->x;
        other_y += other_vertex->y;
        other_z += other_vertex->z;
    }
    float delta_x = this_x * g_double_005ebcc0 - other_x * g_double_005ebcc0;
    float delta_y = this_y * g_double_005ebcc0 - other_y * g_double_005ebcc0;
    float delta_z = this_z * g_double_005ebcc0 - other_z * g_double_005ebcc0;
    float separation_sq = delta_x * delta_x + delta_y * delta_y + delta_z * delta_z;
    float adjust_x = delta_x + (from->x - to->x);
    float adjust_y = delta_y + (from->y - to->y);
    float adjust_z = delta_z + (from->z - to->z);
    float adjusted_sq = adjust_x * adjust_x + adjust_y * adjust_y + adjust_z * adjust_z;
    if (separation_sq < adjusted_sq) {
        return 0;
    }
    return 1;
}

/* Environment contact response for a flag-4 (walkable) surface hit: pushes
   the slide back out of the plane by `depth`, removes the into-plane residual
   scaled by value_20 and the vertical attenuation, and reports the combined
   motion to the environ record. Direction tests against the surface normal
   decide whether any correction applies. */
// FUNCTION: WIZ8 0x0041EA90
unsigned char W8GDSurface::ApplyEnvironContact0041EA90(srVector3T<float>* direction)
{
    W8LevelDataRecord* level = g_level_data_00652dac;
    if ((flags_00 & 4) == 0) {
        level->vector_40 = 0.0f;
        level->vector_70.SetZero();
        return 0;
    }
    level->flags |= 0x10;
    level->sound_environment_0c = footstep_surface_3c;
    level->sound_environment_alt_0d = footstep_material_3d;
    srVector3T<float> normal;
    normal.x = plane_24[0];
    normal.y = plane_24[1];
    normal.z = plane_24[2];
    float factor = g_environ_00652DB4->value_20;
    srVector3T<float> slide = g_environ_00652DB4->vector_24 * g_environ_00652DB4->scale_0c;
    srVector3T<float> unit = slide;
    unit.Normalize();
    if (DotProduct(unit, normal) >= g_float_005ebcb4) {
        srVector3T<float> proj = slide;
        float normal_sq = normal.LengthSquared();
        if (normal_sq > g_float_005ebc58) {
            proj = normal * (DotProduct(slide, normal) / normal_sq);
        }
        if (DotProduct(proj, normal) <= g_float_005ebb34) {
            float depth = proj.Length();
            if (distance_34 < depth) {
                depth = distance_34;
            }
            float attenuation = 0.25f;
            if (slide.y < g_float_005ebb34) {
                attenuation = fabsf(slide.y) / slide.Length() * g_float_005ebccc +
                              g_navigator_vertical_phase_step_005ebcc8;
            }
            srVector3T<float> residual(slide.x - proj.x, slide.y - proj.y, slide.z - proj.z);
            residual.Set(residual.x * factor, residual.y * factor, residual.z * factor);
            residual.Set(residual.x * attenuation, residual.y * attenuation,
                         residual.z * attenuation);
            srVector3T<float> pushback;
            pushback.Set(normal.x * depth, normal.y * depth, normal.z * depth);
            slide += pushback;
            slide -= residual;
            *direction -= residual * factor;
            g_environ_00652DB4->SetScaledMotion00421800(&slide);
            return 1;
        }
        return 0;
    }
    slide = 0.0f;
    g_environ_00652DB4->vector_24 = slide / g_environ_00652DB4->scale_0c;
    g_environ_00652DB4->value_05 = 1;
    return 0;
}

/* VC6 vector constructor iterator, emitted for an ordinary array construction.
   This is compiler support, not an authored Wizardry callback wrapper. */
// LIBRARY: WIZ8 0x0041e880
// vector constructor iterator

// FUNCTION: WIZ8 0x0041EEE0
void ResetLevelMovement0041EEE0(float movement_limit, char reset, char fast_move)
{
    W8LevelDataRecord* level = g_level_data_00652dac;
    if (level != 0) {
        level->movement_limit_2c = movement_limit;
        level->real_elapsed_24 = 0.0f;
        level->frame_elapsed_28 = 0.0f;
        level->movement_progress_30 = 0.0f;
        level->camera_forward_4c.Set(0.0f, 0.0f, 0.0f);
        level->scaled_camera_forward_7c.Set(0.0f, 0.0f, 0.0f);
        level->flags |= W8_LEVEL_FLAG_MOVEMENT_ACTIVE;
        if (reset != 0) {
            level->flags |= W8_LEVEL_FLAG_MOVEMENT_RESET;
        } else {
            level->flags &= ~W8_LEVEL_FLAG_MOVEMENT_RESET;
        }
        if (fast_move != 0) {
            level->flags |= W8_LEVEL_FLAG_8;
        } else {
            level->flags &= ~W8_LEVEL_FLAG_8;
        }
    }
}

// FUNCTION: WIZ8 0x0041ef50
void ResetInactiveLevelDataVectors0041EF50(void)
{
    W8LevelDataRecord* data = g_level_data_00652dac;

    if (data != 0 && (data->flags & W8_LEVEL_FLAG_0) == 0) {
        data->vector_40.SetZero();
        data->camera_forward_4c.SetZero();
        data->scaled_camera_forward_7c.SetZero();
        data->vector_64.SetZero();
        data->vector_70.SetZero();
        data->vector_a0.SetZero();
    }
}

/* Bit eight: read, cleared and set by three neighbouring bodies. */
// FUNCTION: WIZ8 0x0041efb0
unsigned int GetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 8) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041efd0
void ClearLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041efe0
void SetLevelDataFlag8(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags |= W8_LEVEL_FLAG_8;
    }
}

// FUNCTION: WIZ8 0x0041eff0
unsigned int GetLevelDataFlag9(void)
{
    if (g_level_data_00652dac != 0) {
        return (g_level_data_00652dac->flags >> 9) & 1;
    }
    return 0;
}

/* Bit four, read out of the low byte rather than the whole word. */
// FUNCTION: WIZ8 0x0041f070
unsigned int GetLevelDataFlag4(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 4) & 1;
    }
    return 0;
}

/* Bits five through seven together, cleared as a group. */
// FUNCTION: WIZ8 0x0041f0c0
void ClearLevelDataFlags5To7(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_5_TO_7;
    }
}

// FUNCTION: WIZ8 0x0041f140
unsigned int GetLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        return ((unsigned char)g_level_data_00652dac->flags >> 6) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0041f160
void ClearLevelDataFlag6(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags &= ~W8_LEVEL_FLAG_6;
    }
}

/* Hand the level's pending real/frame elapsed times to the caller, fold them
   into the session accumulators, clear the pending pair, and report whether
   either was above the camera-transition epsilon. */
// FUNCTION: WIZ8 0x0041f170
unsigned char ConsumeLevelElapsedTime0041F170(float* real_elapsed, float* frame_elapsed)
{
    W8LevelDataRecord* record = g_level_data_00652dac;
    unsigned char elapsed = 0;
    if (record != 0) {
        *real_elapsed = record->real_elapsed_24;
        *frame_elapsed = record->frame_elapsed_28;
        elapsed = record->real_elapsed_24 > g_camera_transition_epsilon_005ebc84 ||
                  record->frame_elapsed_28 > g_camera_transition_epsilon_005ebc84;
        record->frame_elapsed_28 = 0.0f;
        record->real_elapsed_24 = 0.0f;
        g_status_685170.real_elapsed_2391 += *real_elapsed;
        g_status_685170.frame_elapsed_2395 += *frame_elapsed;
    }
    return elapsed;
}

/* Bit four again, but with a global override: with the bit down, the override
   being set is what withholds the answer. */
// FUNCTION: WIZ8 0x0041f090
int IsLevelDataFlag4EffectivelySet(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_4) == 0 && g_level_override_00652dba != 0) {
        return 0;
    }
    return 1;
}

/* Whether the level has a live vector at 0x88: bit zero has to be up and at
   least one of the three floats has to differ from the default. */
// FUNCTION: WIZ8 0x0041f010
bool HasLevelDataVector(void)
{
    if (g_level_data_00652dac == 0) {
        return false;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_0) != 0 &&
        (g_level_data_00652dac->vector_88.x != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88.y != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88.z != g_float_005ebb34)) {
        return true;
    }
    return false;
}

// GLOBAL: WIZ8 0x00652db4
W8EnvironRecord* g_environ_00652DB4;
// GLOBAL: WIZ8 0x00652dcc
unsigned char g_flag_00652dcc;

/* The camera-sway mode halves navigator gravity, mirrors it into the active
   environment record and swaps the camera forward scale; the flag guards both
   transitions so repeated triggers are idempotent. */
// FUNCTION: WIZ8 0x0041a960
void BeginCameraSway0041A960(void)
{
    if (g_camera_sway_active_652da4) {
        return;
    }
    g_camera_forward_scale_603ab4 = g_camera_level_forward_scale_603aac;
    g_navigator_gravity_00603acc = 93.75f;
    if (g_environ_00652DB4 != 0) {
        g_environ_00652DB4->value_14 = -93.75f;
    }
    g_camera_sway_active_652da4 = 1;
}

// FUNCTION: WIZ8 0x0041a9a0
void EndCameraSway0041A9A0(void)
{
    if (!g_camera_sway_active_652da4) {
        return;
    }
    g_camera_forward_scale_603ab4 = g_camera_default_forward_scale_603ab0;
    g_navigator_gravity_00603acc = 187.5f;
    if (g_environ_00652DB4 != 0) {
        g_environ_00652DB4->value_14 = -187.5f;
    }
    g_camera_sway_active_652da4 = 0;
}

/* Release the level-data record and its companion globals: free the 0xf4-byte
   record (whose destructor only tears down the +0xc4 interval gate), drop the
   shared game-time accumulator through its deleting destructor, and clear the
   environ, octree-data and secondary record pointers plus the teardown flag.
   ~W8GameData runs this first. */
// FUNCTION: WIZ8 0x0041a9e0
void W8GameData::ReleaseLevelData0041A9E0()
{
    g_environ_00652DB4 = 0;
    if (g_level_data_00652dac != 0) {
        delete g_level_data_00652dac;
    }
    g_level_data_00652dac = 0;
    g_level_flags_00652da8 = 0;
    if (g_game_time_accumulator_6598bc != 0) {
        delete g_game_time_accumulator_6598bc;
    }
    g_game_time_accumulator_6598bc = 0;
    g_octree_game_data_00652db0 = 0;
    if (g_flag_00652dcc != 0) {
        g_flag_00652dcc = 0;
    }
}

// FUNCTION: WIZ8 0x0041AA40
void ResetCurrentEnvironment0041AA40(void)
{
    if (g_environ_00652DB4 != 0) {
        if (g_octree_game_data_00652db0 != 0 && g_octree_game_data_00652db0->m_ppEnvirons != 0) {
            g_environ_00652DB4 = g_octree_game_data_00652db0->m_ppEnvirons[0];
        }
        g_environ_00652DB4->vector_24.Set(0.0f, 0.0f, 0.0f);
        if (g_environment_load_flag_00603ad0 != 0) {
            g_environ_00652DB4->value_20 = 1.0f;
        }
        g_environment_load_flag_00603ad0 = g_environment_load_flag_00603ad0 == 0;
        if (g_environment_load_flag_00603ad0 == 0) {
            g_level_override_00652dba = 0;
        }
        return;
    }
    g_environment_load_flag_00603ad0 = 0;
    g_level_override_00652dba = 0;
}

// FUNCTION: WIZ8 0x0041AAE0
unsigned char SetEnvironmentLoadFlag(unsigned char flag)
{
    srVector3T<float> zero_vector(0.0f, 0.0f, 0.0f);
    unsigned char previous = g_environment_load_flag_00603ad0;
    if (g_environ_00652DB4 != 0) {
        g_environ_00652DB4->vector_24 = zero_vector;
        if (flag == 0) {
            g_environ_00652DB4->value_20 = 1.0f;
        }
        g_environment_load_flag_00603ad0 = flag;
    }
    return previous;
}

// FUNCTION: WIZ8 0x0041F0D0
void ResetLevelDataVectors0041F0D0(void)
{
    if (g_level_data_00652dac != 0) {
        g_level_data_00652dac->flags |= 0x40;
        if ((g_level_data_00652dac->flags & 1) == 0) {
            g_level_data_00652dac->vector_40.SetZero();
            g_level_data_00652dac->camera_forward_4c.SetZero();
            g_level_data_00652dac->vector_64.SetZero();
            g_level_data_00652dac->vector_70.SetZero();
            g_level_data_00652dac->scaled_camera_forward_7c.SetZero();
            g_level_data_00652dac->vector_a0.SetZero();
        }
        g_level_data_00652dac->flags &= ~0x100U;
    }
}

/* Camera facade, move timer and the party placement entry. */

// GLOBAL: WIZ8 0x005ebc18
const double g_double_005ebc18 = 3.141592653589793;
// GLOBAL: WIZ8 0x005ebcf0
const float g_float_005ebcf0 = 57.295784f;
// GLOBAL: WIZ8 0x005ebca0
const float g_float_005ebca0 = 6.0f;

// FUNCTION: WIZ8 0x0041FCE0
void GetLevelSoundEnvironment0041FCE0(char* environment, char* secondary)
{
    W8LevelDataRecord* level = g_level_data_00652dac;
    if ((level->flags & W8_LEVEL_FLAG_NO_SOUND_ENVIRONMENT) != 0) {
        *secondary = -1;
        *environment = -1;
        return;
    }
    *environment = level->sound_environment_0c;
    *secondary = level->sound_environment_alt_0d;
}

// FUNCTION: WIZ8 0x00420b40
float MoveTimer(int value)
{
    if (g_game_time_accumulator_6598bc == 0) {
        g_game_time_accumulator_6598bc = new W8GameTimeAccumulator0043A910;
        if (g_game_time_accumulator_6598bc == 0) {
            return g_float_005ebb34;
        }
    }
    if (g_flag_00652dce != 0) {
        if ((value == 8 && g_current_screen_state.id == 7) || value == 4) {
            ResumeSharedGameTimers00439CA0();
            g_flag_00652dce = 0;
        } else {
            return g_float_005ebb34;
        }
    }
    if (value == 1) {
        PauseSharedGameTimers00439BC0();
        g_flag_00652dce = 1;
    }
    return g_game_time_accumulator_6598bc->GetValue28();
}

// FUNCTION: WIZ8 0x00420D40
srCamera* CreateOrSetGameCamera(srNode* parent, srCamera* camera)
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    return g_gd_camera_65a0f8->CreateOrAttachCamera(parent, camera);
}

// FUNCTION: WIZ8 0x00420DC0
float GetCameraYawInDegrees()
{
    return g_gd_camera_65a0f8->m_yaw * g_float_005ebcf0;
}

// FUNCTION: WIZ8 0x00420DD0
float GetCameraYawRadians()
{
    return g_gd_camera_65a0f8->m_yaw;
}

// FUNCTION: WIZ8 0x00420DE0
float GetCameraPitchInDegrees()
{
    return g_gd_camera_65a0f8->m_pitch * g_float_005ebcf0;
}

// FUNCTION: WIZ8 0x00420DF0
float GetCameraPitchRadians()
{
    return g_gd_camera_65a0f8->m_pitch;
}

// FUNCTION: WIZ8 0x00420E00
void BeginManualCameraControl()
{
    g_gd_camera_65a0f8->SetManualControlActive(1);
}

/* 0x00420F40: camera yaw in whole degrees, plus an optional copy of the
   yaw-rotation matrix. The diagnostics dump passes null and uses only the
   yaw. */
// FUNCTION: WIZ8 0x00420F40
int GetCameraYawAndRotation00420F40(srMatrix3T<float>* rotation)
{
    float degrees = g_gd_camera_65a0f8->m_yaw * g_float_005ebcf0;
    if (rotation != 0) {
        *rotation = g_gd_camera_65a0f8->m_yaw_rotation;
    }
    return static_cast<int>(degrees);
}

// FUNCTION: WIZ8 0x00420F70
void LevelCamera()
{
    g_gd_camera_65a0f8->BeginLeveling();
    g_flag_00652da7 = 0;
}

// FUNCTION: WIZ8 0x00420F90
void CameraLookAt(const srVector3T<float>* position)
{
    g_gd_camera_65a0f8->LookAt(position, 0);
}

// FUNCTION: WIZ8 0x00420FB0
void CameraSnapToTarget(const srVector3T<float>* target)
{
    g_gd_camera_65a0f8->SnapToTarget(target);
}

// FUNCTION: WIZ8 0x00420FD0
void TurnCameraToDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->BeginOrientationTransition(0.0f, (float)(scale * degrees), 0);
}

// FUNCTION: WIZ8 0x00421000
void SetCameraYawDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->SetOrientationImmediate(0.0f, (float)(scale * degrees));
}

// FUNCTION: WIZ8 0x00421030
void ApplyCameraRotation(srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->ApplyRotationMatrix(rotation, g_level_data_00652dac);
}

/* Two whole-body reads through the pointer at 0x0065A0F8. The first hands back
   the twelve bytes at 0x8C as one block; the second converts the float at 0x04
   from radians to degrees and truncates it through the CRT's _ftol, which the
   original reaches as a tail jump because the conversion is the whole return
   value. The scale is one ULP above the float nearest 180/pi, so the original
   spelled it as a decimal literal rather than computing it from a pi constant;
   the literal here is the shortest decimal that reproduces the stored bytes. */
// FUNCTION: WIZ8 0x00421070
void GetCameraPosition(srVector3T<float>* position)
{
    *position = g_gd_camera_65a0f8->m_position_08c;
}

/* Zero the two six-float CamPos angle records, then store the live yaw in the
   first and the live pitch in the second. GetWorldCameraState passes the yaw
   record at +0x24 as angle and the pitch record at +0x0c as pitch. */
// FUNCTION: WIZ8 0x004213A0
void GetCameraOrientation(W8CameraAngleRecord angle, W8CameraAngleRecord pitch)
{
    int i;

    for (i = 0; i < 6; ++i) {
        angle[i] = 0.0f;
    }
    for (i = 0; i < 6; ++i) {
        pitch[i] = 0.0f;
    }
    *angle = g_gd_camera_65a0f8->m_yaw;
    *pitch = g_gd_camera_65a0f8->m_pitch;
}

// FUNCTION: WIZ8 0x004213E0
void SetCameraOrientation(W8CameraAngleRecord angle, W8CameraAngleRecord pitch,
                          srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->SetYaw(*angle);
    g_gd_camera_65a0f8->SetPitch(*pitch);
    *angle = g_gd_camera_65a0f8->m_yaw;
    *pitch = g_gd_camera_65a0f8->m_pitch;
    if (rotation != 0) {
        g_gd_camera_65a0f8->GetRotationMatrix(rotation);
    }
}

/* 0x00421440: project `vector` onto `onto` in place; fails when the target
   direction is degenerate. */
// FUNCTION: WIZ8 0x00421440
unsigned char ProjectVectorOntoVector00421440(srVector3T<float>* vector,
                                              const srVector3T<float>* onto)
{
    float length_squared = DotProduct(*onto, *onto);
    if (length_squared <= g_float_005ebc58) {
        return 0;
    }
    *vector = *onto * (DotProduct(*vector, *onto) / length_squared);
    return 1;
}

/* Apply a saved yaw/pitch pair to the game camera for the world reload path.
   Retail reads the world camera node's rotation into the local first, then
   SetCameraOrientation (inlined) overwrites the same local with the updated
   matrix; the local is dead after the call. */
// FUNCTION: WIZ8 0x00421570
void RestoreWorldCameraOrientation00421570(W8CameraAngleRecord angle, W8CameraAngleRecord pitch,
                                           W8World* world)
{
    srMatrix3T<float> rotation;
    world->camera->getRotation(rotation);
    SetCameraOrientation(angle, pitch, &rotation);
}

// FUNCTION: WIZ8 0x00421550
int GetCameraYawDegrees(void)
{
    return (int)(g_gd_camera_65a0f8->m_yaw * 57.295784f);
}

/* Point-visibility query: returns whether the camera has line of sight to the
   given position through the world octree. Fails with no world loaded; with a
   world but no octree there is nothing to occlude, so it returns true. */
// FUNCTION: WIZ8 0x004215e0
bool HasCameraLineOfSight(const srVector3T<float>* position)
{
    srVector3T<float> to = *position;
    if (g_world == 0) {
        return false;
    }
    srVector3T<float> from = g_gd_camera_65a0f8->m_position_08c;
    if (g_world->octree != 0) {
        return g_world->octree->HasLineOfSight(&from, &to, 1);
    }
    return true;
}

/* Mark the renderer ready and copy the point into the game camera when it
   sits anywhere but the origin. */
// FUNCTION: WIZ8 0x00421090
void PlacePartyAtPoint(const srVector3T<float>* point)
{
    srVector3T<float> delta = *point - g_origin_652940;
    if (sqrtf(DotProduct(delta, delta)) != g_zero_005ebb40) {
        MarkRendererReady();
        g_gd_camera_65a0f8->m_position_08c = *point;
    }
}

// FUNCTION: WIZ8 0x0041FD10
W8LevelDataRecord::W8LevelDataRecord() : interval_gate_c4()
{
    flags = 0;
    camera_scale_14 = 0;
    sound_environment_0c = 0;
    sound_environment_alt_0d = 0;
    residual_contact_length_18 = 0.0f;
    contact_facing_1c = 0.0f;
    speed_20 = 0;
    real_elapsed_24 = 0;
    frame_elapsed_28 = 0;
    movement_limit_2c = 0;
    movement_progress_30 = 0;
    flag_ec = 0;
    flag_ed = 0;
    value_f0 = 0;
    footstep_accumulator_10 = -2000.0f;
    primary_contact_prop_id = -1;
    secondary_contact_prop_id = -1;
    camera_position_34.SetZero();
    vector_40.SetZero();
    camera_forward_4c.SetZero();
    vector_58.SetZero();
    vector_64.SetZero();
    vector_70.SetZero();
    scaled_camera_forward_7c.SetZero();
    vector_88.SetZero();
    vector_94.SetZero();
    vector_a0.SetZero();
    contact_normal_ac = srVector3T<float>();
    contact_normal_scale_b8 = 0.0f;
    memset(unknown_bc, 0, sizeof(unknown_bc));
    contact_normal_scale_b8 = 1.0f;
    g_level_override_00652dba = 0;
}

// FUNCTION: WIZ8 0x00420470
unsigned char W8LevelDataRecord::IntegrateCameraForward00420470()
{
    float forward_length;
    float limit;
    float delta_length;
    float scale;
    bool cleared_vector_70;
    srVector3T<float> adjustment;
    srVector3T<float> combined;

    cleared_vector_70 = false;
    forward_length = camera_forward_4c.Length();
    limit = g_environ_00652DB4->value_38 * camera_scale_14;
    if (limit <= vector_40.Length()) {
        adjustment = camera_forward_4c;
        if (g_float_005ebc58 < vector_70.LengthSquared()) {
            adjustment =
                vector_70 * (DotProduct(adjustment, vector_70) / vector_70.LengthSquared());
        }
        adjustment -= camera_forward_4c;
        delta_length = adjustment.Length();
        limit = g_environ_00652DB4->value_20 * limit;
        if (delta_length <= limit) {
            if (delta_length < limit * g_camera_snap_epsilon_005ebc2c) {
                adjustment.Set(0.0f, 0.0f, 0.0f);
            }
        } else {
            adjustment.SetLength(limit);
        }
        scale = g_environ_00652DB4->value_3c * g_environ_00652DB4->value_20;
        vector_70 *= scale;
    } else {
        if (forward_length < limit) {
            vector_64.SetZero();
            return 0;
        }
        limit = g_environ_00652DB4->value_20 * limit;
        adjustment.Set(-camera_forward_4c.x, -camera_forward_4c.y, -camera_forward_4c.z);
        if (limit < forward_length) {
            adjustment.SetLength(limit);
        }
        vector_70.SetZero();
        cleared_vector_70 = true;
    }

    combined.Set(camera_forward_4c.x + vector_70.x, camera_forward_4c.y + vector_70.y,
                 camera_forward_4c.z + vector_70.z);
    combined.Set(combined.x + adjustment.x, combined.y + adjustment.y, combined.z + adjustment.z);
    vector_64 = combined;
    vector_70 = combined;
    if (cleared_vector_70) {
        if (DotProduct(vector_40, adjustment) > g_camera_transition_epsilon_005ebc84) {
            vector_64.SetZero();
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x00420810
unsigned char W8LevelDataRecord::ApplySavedMotionMatrix00420810(unsigned char prior_fast,
                                                                unsigned char fast_move,
                                                                const srMatrix3T<float>* saved)
{
    float horizontal;
    float clamp_scale;
    float vertical;

    if (prior_fast != 0 && fast_move == 0) {
        vector_40.SetZero();
        vector_70.SetZero();
    }
    vector_40 = saved->Transform(vector_40);
    vector_70.x = vector_40.x * camera_scale_14;
    vector_70.y = vector_40.y * camera_scale_14;
    vector_70.z = vector_40.z * camera_scale_14;
    if (IntegrateCameraForward00420470() == 0) {
        return 0;
    }

    horizontal = sqrtf(vector_64.z * vector_64.z + vector_64.x * vector_64.x);
    if (fast_move == 0) {
        clamp_scale = g_camera_level_forward_scale_603aac;
        if (prior_fast == 0) {
            if (clamp_scale < horizontal) {
                vector_64.x = (clamp_scale / horizontal) * vector_64.x;
                vector_64.z = (clamp_scale / horizontal) * vector_64.z;
            }
        } else if (g_camera_level_forward_scale_603aac < horizontal) {
            clamp_scale = g_camera_default_forward_scale_603ab0;
            if (clamp_scale < horizontal) {
                vector_64.x = (clamp_scale / horizontal) * vector_64.x;
                vector_64.z = (clamp_scale / horizontal) * vector_64.z;
            }
        } else {
            prior_fast = 0;
        }
    } else {
        prior_fast = 1;
        clamp_scale = g_camera_default_forward_scale_603ab0;
        if (clamp_scale < horizontal) {
            vector_64.x = (clamp_scale / horizontal) * vector_64.x;
            vector_64.z = (clamp_scale / horizontal) * vector_64.z;
        }
    }

    if (g_environment_load_flag_00603ad0 == 0) {
        vertical = vector_64.y;
        if (vertical > g_camera_level_forward_scale_603aac) {
            vertical = g_camera_level_forward_scale_603aac;
        } else if (vertical < -g_camera_level_forward_scale_603aac) {
            vertical = -g_camera_level_forward_scale_603aac;
        }
    } else {
        vertical = vector_64.y;
        if (vertical > g_camera_forward_scale_603ab4) {
            vertical = g_camera_forward_scale_603ab4;
        } else if (vertical < -g_camera_forward_scale_603ab4) {
            vertical = -g_camera_forward_scale_603ab4;
        }
    }
    vector_64.y = vertical;
    vector_a0.x = vector_64.x * camera_scale_14;
    vector_a0.y = vector_64.y * camera_scale_14;
    vector_a0.z = vector_64.z * camera_scale_14;
    return prior_fast;
}

// FUNCTION: WIZ8 0x00420A60
unsigned char W8LevelDataRecord::UpdateFootstepFromMotion00420A60()
{
    float dx;
    float dy;
    float dz;
    float distance;
    char large_radius;

    if ((flags & W8_LEVEL_FLAG_0) == 0) {
        dx = scaled_camera_forward_7c.x;
        dy = scaled_camera_forward_7c.y;
        dz = scaled_camera_forward_7c.z;
    } else {
        dx = scaled_camera_forward_7c.x - vector_88.x;
        dy = scaled_camera_forward_7c.y - vector_88.y;
        dz = scaled_camera_forward_7c.z - vector_88.z;
    }
    if ((flags & W8_LEVEL_FLAG_NO_SOUND_ENVIRONMENT) != 0 || (flags & W8_LEVEL_FLAG_4) == 0) {
        return 0;
    }
    distance = sqrtf(dx * dx + dy * dy + dz * dz) + footstep_accumulator_10;
    footstep_accumulator_10 = distance;
    if (g_float_005ebcdc < distance) {
        if (g_status_685170.search_mode == 0 && (flags & W8_LEVEL_FLAG_8) == 0) {
            large_radius = 0;
        } else {
            large_radius = 1;
        }
        AlertCombatNoise004F1150(large_radius);
        if (sound_environment_0c >= 0 && sound_environment_alt_0d >= 0) {
            PlayFootstep0047A440(sound_environment_0c, sound_environment_alt_0d, 0);
            while (g_float_005ebcdc < footstep_accumulator_10) {
                footstep_accumulator_10 -= g_float_005ebcdc;
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0041FF90
void W8LevelDataRecord::UpdateMotionProgress0041FF90(unsigned char fast_move, unsigned char moved)
{
    bool allow_override;
    float length;
    float scale;
    srVector3T<float> projected;
    srVector3T<float> gravity;
    srVector3T<float> environ_vector;

    allow_override = true;
    if ((flags & W8_LEVEL_FLAG_0) == 0) {
        vector_58.SetZero();
        vector_94.SetZero();
        if (g_float_005ebb34 < vector_64.y) {
            length = vector_64.Length();
            if (fast_move == 0) {
                if (g_camera_level_forward_scale_603aac < length) {
                    vector_64.SetLength(g_camera_level_forward_scale_603aac);
                    vector_a0.x = vector_64.x * camera_scale_14;
                    vector_a0.y = vector_64.y * camera_scale_14;
                    vector_a0.z = vector_64.z * camera_scale_14;
                }
            } else if (g_camera_default_forward_scale_603ab0 < length) {
                vector_64.SetLength(g_camera_default_forward_scale_603ab0);
                vector_a0.x = vector_64.x * camera_scale_14;
                vector_a0.y = vector_64.y * camera_scale_14;
                vector_a0.z = vector_64.z * camera_scale_14;
            }
        }
    }
    if (moved == 0) {
        if ((flags & W8_LEVEL_FLAG_NO_SOUND_ENVIRONMENT) != 0 || (flags & W8_LEVEL_FLAG_4) == 0) {
            allow_override = false;
        }
        camera_forward_4c.SetZero();
        scaled_camera_forward_7c.SetZero();
        footstep_accumulator_10 = 1800.0f;
    } else {
        camera_forward_4c.x = vector_64.x - vector_58.x;
        camera_forward_4c.y = vector_64.y - vector_58.y;
        camera_forward_4c.z = vector_64.z - vector_58.z;
        scaled_camera_forward_7c.x = camera_forward_4c.x * camera_scale_14;
        scaled_camera_forward_7c.y = camera_forward_4c.y * camera_scale_14;
        scaled_camera_forward_7c.z = camera_forward_4c.z * camera_scale_14;
        allow_override = UpdateFootstepFromMotion00420A60() != 0;
        camera_forward_4c.x -= g_environ_00652DB4->vector_24.x;
        camera_forward_4c.y -= g_environ_00652DB4->vector_24.y;
        camera_forward_4c.z -= g_environ_00652DB4->vector_24.z;
        projected = vector_58;
        gravity.Set(g_environ_00652DB4->value_10, g_environ_00652DB4->value_14,
                    g_environ_00652DB4->value_18);
        ProjectVectorOntoVector00421440(&projected, &gravity);
        environ_vector = g_environ_00652DB4->vector_24;
        if (environ_vector.Length() < projected.Length()) {
            g_environ_00652DB4->vector_24 = projected;
        }
        vector_40.SetZero();
        vector_70.SetZero();
    }

    vector_64 = vector_a0 * (static_cast<float>(g_double_005ebc30) / camera_scale_14);
    speed_20 = vector_a0.Length();
    if (speed_20 < g_camera_transition_epsilon_005ebc84) {
        speed_20 = 0;
        vector_a0.SetZero();
        vector_64.SetZero();
    }
    if ((flags & W8_LEVEL_FLAG_MOVEMENT_ACTIVE) == 0) {
        if ((flags & W8_LEVEL_FLAG_8) == 0) {
            real_elapsed_24 += speed_20;
        } else {
            frame_elapsed_28 += speed_20;
        }
    } else if ((flags & W8_LEVEL_FLAG_4) != 0) {
        if ((flags & W8_LEVEL_FLAG_8) == 0) {
            real_elapsed_24 += speed_20;
        } else {
            frame_elapsed_28 += speed_20;
        }
        scale = movement_progress_30;
        movement_progress_30 = speed_20 + scale;
        if (movement_limit_2c < speed_20 + scale) {
            camera_forward_4c.SetZero();
            scaled_camera_forward_7c.SetZero();
            footstep_accumulator_10 = 2000.0f;
            flags |= W8_LEVEL_FLAG_6;
            if (gXStatus.fPartyMovementMode != 0) {
                UpdateActivePartyMovement();
            }
        }
    }

    if (g_camera_sway_active_652da4 == 0) {
        if (g_level_override_00652dba == 0) {
            if (allow_override && value_f0 < g_float_005ebc3c) {
                g_level_override_00652dba = 1;
            }
        } else if (allow_override && g_float_005ebcd8 < value_f0) {
            HandleLevelOverride004EF9A0(value_f0);
        }
        value_f0 = -(vector_64.y / g_camera_motion_divisor_00603ac4);
        return;
    }
    value_f0 = 0;
}

// FUNCTION: WIZ8 0x00420E20
void UpdateLevelMovementAudio00420E20(void)
{
    float now;

    if (g_level_data_00652dac == 0) {
        return;
    }
    if (((g_gd_camera_65a0f8->m_positional_000 >> 6) & 1) == 0 ||
        (g_level_data_00652dac->flags & W8_LEVEL_FLAG_4) == 0 ||
        g_level_data_00652dac->vector_64.x != g_float_005ebb34 ||
        g_level_data_00652dac->vector_64.y != g_float_005ebb34 ||
        g_level_data_00652dac->vector_64.z != g_float_005ebb34) {
        if (g_level_footstep_sound_00603ad4 != -1) {
            SoundSetFadeVolume(g_level_footstep_sound_00603ad4, 0, 500, 1);
            g_level_footstep_sound_00603ad4 = -1;
        }
        return;
    }
    now = g_game_time_accumulator_6598bc->GetValue30();
    if (g_facing_tolerance_005ebcf4 < now - g_level_footstep_time_00652dd0 &&
        (g_level_footstep_time_00652dd0 = now,
         g_level_footstep_sound_00603ad4 == -1 ||
             SoundIsPlaying(g_level_footstep_sound_00603ad4) == 0)) {
        g_level_footstep_sound_00603ad4 =
            PlayFootstep0047A440(g_level_data_00652dac->sound_environment_0c,
                                 g_level_data_00652dac->sound_environment_alt_0d, 2);
    }
}

/* The record's only non-trivial member is the interval gate at +0xc4, so the
   whole destructor is that member's teardown - retail emits it as the
   `add ecx,0xc4` body previously read as an adjustor thunk. */
// FUNCTION: WIZ8 0x00421890
W8LevelDataRecord::~W8LevelDataRecord() {}
