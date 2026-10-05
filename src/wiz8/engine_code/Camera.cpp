#include "wiz8/engine_code/Camera.h"

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
static int g_saved_environment_flag = 1;

/* Find the named camera path in the world's list and toggle it. Retail
   callers push the flag as a plain int and the body forwards it raw to
   UpdateCameraPathState. */
// FUNCTION: WIZ8 0x0048F280
void UpdateCameraPathStateByName(W8World* world, const char* name, int active)
{
    /* Retail calls the length accessor unconditionally (0x0048F28C) before
       testing the list pointer; {PLLength, ILLength} is one linker-folded
       equivalence class, so PLLength is the correctly spelled name here.
       The accessor is null-tolerant (returns 0 on a null head), so the call
       precedes the guard in the original. */
    unsigned int count = PLLength(world->plsCameras);
    if (world->plsCameras != 0 && count != 0) {
        for (int index = 0; index < static_cast<int>(count); ++index) {
            W8CameraPath* path = GetWorldCameraPath(world, index);
            if (_stricmp(path->name0, name) == 0) {
                UpdateCameraPathState(world, path, active);
                return;
            }
        }
    }
}

// FUNCTION: WIZ8 0x0048F2F0
void UpdateCameraPathState(W8World* world, W8CameraPath* path, int active)
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

    if (!path->active && active != 0) {
        g_camera_path_active = true;
        path->active = true;
        PathAISetValue(path->path, 0.0f);
        PathAIResetTick(path->path);
        path->path->distance_travelled = 0.0f;
        path->path->upright = 1;
        g_saved_environment_flag = SetEnvironmentLoadFlag(0);
        return;
    }
    if (!path->active || active != 0) {
        return;
    }
    g_camera_path_active = false;
    rotation.SetIdentity();
    target.Set(0.0f, 0.0f, 1.0f);
    world->camera->getRotation(rotation);
    ApplyCameraRotation(&rotation);
    path->active = false;
    SetEnvironmentLoadFlag(g_saved_environment_flag);
    if (g_status.current_level == 1) {
        if (_stricmp(path->name0, "Camera01") == 0) {
            QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x721);
            QueueNpcMessageLine(W8_NPC_MSG_PATH2_TRIGGER, 0);
            return;
        }
        if (_stricmp(path->name0, "Camera02") == 0) {
            npc = GetNpcStateByKind(0x33);
        } else {
            if (_stricmp(path->name0, "Camera03") != 0) {
                return;
            }
            group = FindFirstMonsterByID(0x18c);
            if (group != 0) {
                index = MonsterGetIndexByLocationID(
                    0x100, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp",
                    group->leader_location_id, true);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                PointCameraAtMonster(monster_info, true, true);
                position = monster_info->p3D->GetPosition();
                target = position;
                if (g_settings.camera_rotation_mode == W8_CAMERA_ROTATION_ALL_TARGETS) {
                    if (g_settings.camera_rotation_style == W8_CAMERA_ROTATION_SNAP) {
                        if (g_gd_camera->ComputeTrackingOrientation(&target, &angle, &pitch) == 0) {
                            CameraSnapToTarget(&target);
                        }
                    } else if (g_settings.camera_rotation_style == W8_CAMERA_ROTATION_SMOOTH &&
                               g_gd_camera->ComputeTrackingOrientation(&target, &angle, &pitch) ==
                                   0) {
                        g_gd_camera->BeginOrientationTransition(pitch, angle, false);
                    }
                }
                MonsterForwardReferencePosition(monster_info->p3D, 0);
            }
            npc = GetNpcStateByKind(0x34);
        }
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, 0, true, 0);
        }
        return;
    }
    if (g_status.current_level == 4 && _stricmp(path->name0, "CameraPath2") != 0) {
        if (_stricmp(path->name0, "CameraPath3") == 0) {
            ClearMainGameTargetState();
            group = FindFirstMonsterByID(0x1b4);
            if (group != 0) {
                index = MonsterGetIndexByLocationID(
                    0xd0, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp",
                    group->leader_location_id, true);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                MonsterForwardReferencePosition(monster_info->p3D, 0);
                return;
            }
        } else if (_stricmp(path->name0, "CameraPath4") == 0) {
            ResetLevelDataVectors();
            return;
        }
    }
}

/* The far distance at which the camera aims at a monster's lower height
   offset rather than its head. */
// GLOBAL: WIZ8 0x005EBCDC
const float g_float_005ebcdc = 2000.0f;

/* Turn the camera to face a monster: when rotation tracking is off the
   monster's own combat target still drives it if that target is the selected
   character. Otherwise the force flag decides whether to orient at all, and
   animate chooses the eased transition over the snap. The aim point is the
   head height, or the lower offset when the two nearly coincide or the
   monster is far away. */
// FUNCTION: WIZ8 0x0048F650
void PointCameraAtMonster(W8MonsterInfo* monster_info, bool force, bool animate)
{
    srVector3T<float> position;
    float pitch;
    float angle;
    W8Monster* monster;
    bool track;

    if (g_settings.camera_rotation_mode == W8_CAMERA_ROTATION_SELECTED_CHARACTER &&
        monster_info->Target.iType == W8_TARGET_KIND_CHARACTER &&
        monster_info->Target.iChar == g_status.selected_character) {
        track = true;
    } else {
        track = force;
        if (!track && g_settings.camera_rotation_mode != W8_CAMERA_ROTATION_ALL_TARGETS) {
            return;
        }
    }
    monster = monster_info->p3D;
    if (!monster->IsRenderable(true)) {
        return;
    }
    if (monster->movement.height_offset - monster->movement.secondary_height_offset <
            g_float_005ebc64 ||
        monster->GetDistanceToPlayer() > g_float_005ebcdc) {
        position = monster->movement.position;
        position.y += monster->movement.secondary_height_offset;
    } else {
        position = monster->movement.position;
        position.y += monster->movement.height_offset;
    }
    if (track) {
        if (!animate) {
            CameraSnapToTarget(&position);
            return;
        }
        if (g_gd_camera->ComputeTrackingOrientation(&position, &angle, &pitch) == 0) {
            g_gd_camera->BeginOrientationTransition(pitch, angle, false);
        }
        return;
    }
    if (g_settings.camera_rotation_mode != W8_CAMERA_ROTATION_ALL_TARGETS) {
        return;
    }
    if (g_settings.camera_rotation_style == W8_CAMERA_ROTATION_SMOOTH) {
        if (g_gd_camera->ComputeTrackingOrientation(&position, &angle, &pitch) == 0) {
            g_gd_camera->BeginOrientationTransition(pitch, angle, false);
        }
    } else if (g_settings.camera_rotation_style == W8_CAMERA_ROTATION_SNAP &&
               g_gd_camera->ComputeTrackingOrientation(&position, &pitch, &angle) == 0) {
        CameraSnapToTarget(&position);
    }
}

/* Turn the camera to face a world position: without the force flag the camera
   only moves when tracking mode is enabled, snapping or easing per the
   configured rotation style; with it the orientation is always updated -
   snapped unless animate asks for the transition. */
// FUNCTION: WIZ8 0x0048F800
void PointCameraAtTarget(srVector3T<float>* position, bool force, bool animate)
{
    float angle;
    float pitch;

    if (force) {
        if (!animate) {
            CameraSnapToTarget(position);
            return;
        }
        if (g_gd_camera->ComputeTrackingOrientation(position, &angle, &pitch) == 0) {
            g_gd_camera->BeginOrientationTransition(pitch, angle, false);
        }
        return;
    }
    if (g_settings.camera_rotation_mode != W8_CAMERA_ROTATION_ALL_TARGETS) {
        return;
    }
    switch (g_settings.camera_rotation_style) {
    case W8_CAMERA_ROTATION_SMOOTH:
        if (g_gd_camera->ComputeTrackingOrientation(position, &angle, &pitch) == 0) {
            g_gd_camera->BeginOrientationTransition(pitch, angle, false);
        }
        break;
    case W8_CAMERA_ROTATION_SNAP:
        if (g_gd_camera->ComputeTrackingOrientation(position, &pitch, &angle) == 0) {
            CameraSnapToTarget(position);
        }
        break;
    }
}
