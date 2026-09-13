#include "wiz8/engine_code/Camera.h"

#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/music_playlist.h"
#include "surrender/srCamera.h"
#include "wiz8/npc_state.h"
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
int g_saved_environment_flag_60aa64;

// FUNCTION: WIZ8 0x0048F2F0
void UpdateCameraPathState0048F2F0(W8World* world, W8CameraPath* path, float fTime)
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

    if (path->active_14 == 0 && fTime != 0.0f) {
        g_level_runtime_flag_0065ba70 = 1;
        path->active_14 = 1;
        PathAISetValue004A9F60(path->path_18, 0.0f);
        path->path_18->tick_28 = GetTickCount();
        path->path_18->value_30 = 0.0f;
        path->path_18->unknown_3b = 1;
        g_saved_environment_flag_60aa64 = Function41AAE0(0);
        return;
    }
    if (path->active_14 == 0 || fTime != 0.0f) {
        return;
    }
    g_level_runtime_flag_0065ba70 = 0;
    rotation.SetIdentity();
    target.x = 0.0f;
    target.y = 0.0f;
    target.z = 1.0f;
    world->camera->getRotation(rotation);
    ApplyCameraRotation(&rotation);
    path->active_14 = 0;
    Function41AAE0(g_saved_environment_flag_60aa64);
    if (g_status_685170.current_level == 1) {
        if (_stricmp(path->name_00, "Camera01") == 0) {
            Function5289B0(9, 0x721);
            Function5289B0(0xe, 0);
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
                    0x100, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp", group->value_9f, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                Function48F650(monster_info, 1, 1);
                position = monster_info->monster->GetPosition();
                target.x = position.x;
                target.y = position.y;
                target.z = position.z;
                if (g_settings_6850c8.camera_rotation_mode == 1) {
                    if (g_settings_6850c8.camera_rotation_style == 0) {
                        if (g_gd_camera_65a0f8->ComputeTrackingOrientation(&target, &angle,
                                                                           &pitch) == 0) {
                            Function420FB0(&target);
                        }
                    } else if (g_settings_6850c8.camera_rotation_style == 1 &&
                               g_gd_camera_65a0f8->ComputeTrackingOrientation(&target, &angle,
                                                                              &pitch) == 0) {
                        g_gd_camera_65a0f8->BeginOrientationTransition(pitch, angle, 0);
                    }
                }
                MonsterForwardReferencePosition(monster_info->monster, 0);
            }
            npc = GetNpcStateByKind(0x34);
        }
        if (npc != 0) {
            Function56C5E0(npc, 0, 0, 1, 0);
        }
        return;
    }
    if (g_status_685170.current_level == 4 && _stricmp(path->name_00, "CameraPath2") != 0) {
        if (_stricmp(path->name_00, "CameraPath3") == 0) {
            ClearMainGameTargetState();
            group = FindFirstMonsterByID(0x1b4);
            if (group != 0) {
                index = MonsterGetIndexByLocationID(
                    0xd0, "C:\\Projects\\Wizardry 8\\Engine Code\\Camera.cpp", group->value_9f, 1);
                monster_info = MonsterGetScriptPartByLocationIndex(index);
                MonsterForwardReferencePosition(monster_info->monster, 0);
                return;
            }
        } else if (_stricmp(path->name_00, "CameraPath4") == 0) {
            ResetLevelDataVectors0041F0D0();
            return;
        }
    }
}
