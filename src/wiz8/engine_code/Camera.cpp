#include "wiz8/engine_code/Camera.h"

#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/music_playlist.h"
#include "surrender/srCamera.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/startup_world.h"

#include <string.h>

/* Retail Engine Code\Camera.cpp. The TU's single anchored function is
   0x0048F2F0 below; the only other evidence is the path string and it owns no
   globals beyond the saved-environment flag. Nothing in GameData.cpp or
   world_selection.cpp falls in this hull. The GDCamera cluster
   0x476140-0x478EB0 is NOT Camera.cpp: it sits in the gap between
   stMeshModel.cpp and AmbientSound.cpp, ~0x7A40 below this anchor, and remains
   a reconstructed owner in GDCamera.cpp. */

// GLOBAL: WIZ8 0x0060AA64
int g_saved_environment_flag_60aa64 = 1;

/* Find the named camera path in the world's list and toggle it. Retail
   callers push the flag as a plain int and the body forwards it raw to
   UpdateCameraPathState0048F2F0. */
// FUNCTION: WIZ8 0x0048F280
void UpdateCameraPathStateByName(W8World* world, const char* name, int active)
{
    /* Retail calls ILLength unconditionally (0x0048F28C) before testing the
       list pointer; ILLength is null-tolerant (returns 0 on a null head),
       so the call precedes the guard in the original. */
    // c-style-cast-ok: retail calls ILLength on this W8PList field at 0x0048F28C; the circa-2000 authored spelling is the C cast.
    unsigned int count = ILLength((W8IList*)world->plsCameras);
    if (world->plsCameras != 0 && count != 0) {
        for (int index = 0; index < static_cast<int>(count); ++index) {
            W8CameraPath* path = static_cast<W8CameraPath*>(PLGet(world->plsCameras, index));
            if (_stricmp(path->name_00, name) == 0) {
                UpdateCameraPathState0048F2F0(world, path, active);
                return;
            }
        }
    }
}

// FUNCTION: WIZ8 0x0048F2F0
void UpdateCameraPathState0048F2F0(W8World* world, W8CameraPath* path, int active)
{
    W8NpcState* npc;
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    unsigned int index;
    srVector3T<float> target;
    srVector3T<float> position;
    srMatrix3T<float> rotation;
    float angle;
    float pitch;

    if (path->active_14 == 0 && active != 0) {
        g_camera_path_active_0065ba70 = 1;
        path->active_14 = 1;
        PathAISetValue004A9F60(path->path_18, 0.0f);
        path->path_18->last_update_tick = GetTickCount();
        path->path_18->distance_travelled = 0.0f;
        path->path_18->upright_3b = 1;
        g_saved_environment_flag_60aa64 = SetEnvironmentLoadFlag(0);
        return;
    }
    if (path->active_14 == 0 || active != 0) {
        return;
    }
    g_camera_path_active_0065ba70 = 0;
    rotation.SetIdentity();
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 1.0f;
    world->camera->getRotation(rotation);
    ApplyCameraRotation(&rotation);
    path->active_14 = 0;
    SetEnvironmentLoadFlag(g_saved_environment_flag_60aa64);
    if (g_status_685170.current_level == 1) {
        if (_stricmp(path->name_00, "Camera01") == 0) {
            QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x721);
            QueueNpcMessageLine(W8_NPC_MSG_PATH2_TRIGGER, 0);
            return;
        }
        if (_stricmp(path->name_00, "Camera02") == 0) {
            npc = GetNpcStateByKind(0x33);
        } else {
            if (_stricmp(path->name_00, "Camera03") != 0) {
                return;
            }
            group = FindFirstMonsterByID(0x18c);
            if (group != 0) {
                index = MonsterGetIndexByLocationID(
                    0x100, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp", group->leader_id_9f,
                    1);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                PointCameraAtMonster(monster_info, 1, 1);
                position = monster_info->p3D->GetPosition();
                target = position;
                if (g_settings_6850c8.camera_rotation_mode == 1) {
                    if (g_settings_6850c8.camera_rotation_style == 0) {
                        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(&target, &angle,
                                                                           &pitch) == 0) {
                            CameraSnapToTarget(&target);
                        }
                    } else if (g_settings_6850c8.camera_rotation_style == 1 &&
                               g_gd_camera_65a0f8->ComputeTrackingOrientation(&target, &angle,
                                                                              &pitch) == 0) {
                        g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
                    }
                }
                MonsterForwardReferencePosition(monster_info->p3D, 0);
            }
            npc = GetNpcStateByKind(0x34);
        }
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, 0, 1, 0);
        }
        return;
    }
    if (g_status_685170.current_level == 4 && _stricmp(path->name_00, "CameraPath2") != 0) {
        if (_stricmp(path->name_00, "CameraPath3") == 0) {
            ClearMainGameTargetState();
            group = FindFirstMonsterByID(0x1b4);
            if (group != 0) {
                index = MonsterGetIndexByLocationID(
                    0xd0, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp", group->leader_id_9f,
                    1);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                MonsterForwardReferencePosition(monster_info->p3D, 0);
                return;
            }
        } else if (_stricmp(path->name_00, "CameraPath4") == 0) {
            ResetLevelDataVectors0041F0D0();
            return;
        }
    }
}

/* The far distance at which the camera aims at a monster's lower height
   offset rather than its head. */
// GLOBAL: WIZ8 0x005EBCDC
float g_float_005ebcdc = 2000.0f;

/* Turn the camera to face a monster: when rotation tracking is off the
   monster's own combat target still drives it if that target is the selected
   character. Otherwise the force flag decides whether to orient at all, and
   animate chooses the eased transition over the snap. The aim point is the
   head height, or the lower offset when the two nearly coincide or the
   monster is far away. */
// FUNCTION: WIZ8 0x0048F650
void PointCameraAtMonster(W8MonsterInfo* monster_info, unsigned char force, unsigned char animate)
{
    srVector3T<float> position;
    float pitch;
    float angle;
    W8Monster* monster;
    unsigned char track;

    if (g_settings_6850c8.camera_rotation_mode == 0 &&
        monster_info->Target.iType == W8_TARGET_KIND_CHARACTER &&
        monster_info->Target.iChar == g_status_685170.selected_character) {
        track = 1;
    } else {
        track = force;
        if (track == 0 && g_settings_6850c8.camera_rotation_mode != 1) {
            return;
        }
    }
    monster = monster_info->p3D;
    if (monster->IsRenderable004C7C00(1) == 0) {
        return;
    }
    if (monster->movement_0c0.height_offset_0b8 -
                monster->movement_0c0.secondary_height_offset_0bc <
            g_float_005ebc64 ||
        monster->GetDistanceToPlayer004C7CB0() > g_float_005ebcdc) {
        position = monster->movement_0c0.position_040;
        position.y += monster->movement_0c0.secondary_height_offset_0bc;
    } else {
        position = monster->movement_0c0.position_040;
        position.y += monster->movement_0c0.height_offset_0b8;
    }
    if (track != 0) {
        if (animate == 0) {
            CameraSnapToTarget(&position);
            return;
        }
        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(&position, &angle, &pitch) == 0) {
            g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
        }
        return;
    }
    if (g_settings_6850c8.camera_rotation_mode != 1) {
        return;
    }
    if (g_settings_6850c8.camera_rotation_style == 1) {
        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(&position, &angle, &pitch) == 0) {
            g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
        }
    } else if (g_settings_6850c8.camera_rotation_style == 0 &&
               g_gd_camera_65a0f8->ComputeTrackingOrientation(&position, &pitch, &angle) == 0) {
        CameraSnapToTarget(&position);
    }
}

/* Turn the camera to face a world position: without the force flag the camera
   only moves when tracking mode is enabled, snapping or easing per the
   configured rotation style; with it the orientation is always updated -
   snapped unless animate asks for the transition. */
// FUNCTION: WIZ8 0x0048F800
void PointCameraAtTarget(srVector3T<float>* position, unsigned char force, unsigned char animate)
{
    float angle;
    float pitch;

    if (force != 0) {
        if (animate == 0) {
            CameraSnapToTarget(position);
            return;
        }
        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(position, &angle, &pitch) == 0) {
            g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
        }
        return;
    }
    if (g_settings_6850c8.camera_rotation_mode != 1) {
        return;
    }
    if (g_settings_6850c8.camera_rotation_style == 1) {
        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(position, &angle, &pitch) == 0) {
            g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
        }
    } else if (g_settings_6850c8.camera_rotation_style == 0 &&
               g_gd_camera_65a0f8->ComputeTrackingOrientation(position, &pitch, &angle) == 0) {
        CameraSnapToTarget(position);
    }
}
