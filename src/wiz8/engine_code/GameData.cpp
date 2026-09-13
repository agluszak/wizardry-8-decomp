#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/OctBuildTree.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/BitArray.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/screen_state.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>

// GLOBAL: WIZ8 0x00652dac
W8LevelDataRecord* g_level_data_00652dac;

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
    W8_LEVEL_FLAG_4 = 0x010,
    W8_LEVEL_FLAG_5_TO_7 = 0x0e0,
    W8_LEVEL_FLAG_6 = 0x040,
    W8_LEVEL_FLAG_8 = 0x100,
    W8_LEVEL_FLAG_9 = 0x200
};

extern unsigned char g_level_override_00652dba;
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
        if (g_octree_game_data_00652db0->vertex_count_20 < vertex_indices[index]) {
            return 0;
        }
        output[index] = g_octree_game_data_00652db0->vertices_24[vertex_indices[index]];
        ++index;
    } while (index < 3);
    return 1;
}

/* Ensure the shared game-data object exists, then run its update. */
// FUNCTION: WIZ8 0x0041F1F0
void UpdateSharedGameDataObject0041F1F0()
{
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            return;
        }
    }
    g_object_6598bc->Update();
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
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            g_object_6598bc->Update();
            return;
        }
    }
    if (g_flag_00652dce != 0) {
        ResumeSharedGameTimers00439CA0();
        g_flag_00652dce = 0;
    }
    g_object_6598bc->Update();
}
/* 0x005EBB34: one float constant with two independent readings - the level
   vector's "no value" here, and Controls.cpp's own range start. Neither is
   proven, so it keeps its address. */
/* Copy one four-byte handle over another. */
// FUNCTION: WIZ8 0x0041cf80
void CopyLevelDataHandle(int* destination, const int* source)
{
    *destination = *source;
}

/* VC6 vector constructor iterator, emitted for an ordinary array construction.
   This is compiler support, not an authored Wizardry callback wrapper. */
// LIBRARY: WIZ8 0x0041e880
// vector constructor iterator

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
unsigned char HasLevelDataVector(void)
{
    if (g_level_data_00652dac == 0) {
        return 0;
    }
    if ((g_level_data_00652dac->flags & W8_LEVEL_FLAG_0) != 0 &&
        (g_level_data_00652dac->vector_88[0] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[1] != g_float_005ebb34 ||
         g_level_data_00652dac->vector_88[2] != g_float_005ebb34)) {
        return 1;
    }
    return 0;
}

// GLOBAL: WIZ8 0x00652db4
W8EnvironRecord* g_environ_00652DB4;

// FUNCTION: WIZ8 0x0041AA40
void ResetCurrentEnvironment0041AA40(void)
{
    if (g_environ_00652DB4 != 0) {
        if (g_octree_game_data_00652db0 != 0 && g_octree_game_data_00652db0->environs_84 != 0) {
            g_environ_00652DB4 = g_octree_game_data_00652db0->environs_84[0];
        }
        g_environ_00652DB4->value_24 = 0;
        g_environ_00652DB4->value_28 = 0;
        g_environ_00652DB4->value_2c = 0;
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

// GLOBAL: WIZ8 0x00652da7
unsigned char g_flag_00652da7;
// GLOBAL: WIZ8 0x005ebc18
const double g_double_005ebc18 = 3.141592653589793;
// GLOBAL: WIZ8 0x005ebcf0
const float g_float_005ebcf0 = 57.295784f;
// GLOBAL: WIZ8 0x005ebca0
const float g_float_005ebca0 = 6.0f;
// GLOBAL: WIZ8 0x00652940
srVector3T<float> g_origin_652940;

// FUNCTION: WIZ8 0x00420b40
float MoveTimer(int value)
{
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
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
    return g_object_6598bc->GetValue28();
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

// FUNCTION: WIZ8 0x00420F70
void LevelCamera()
{
    g_gd_camera_65a0f8->BeginLeveling();
    g_flag_00652da7 = 0;
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
void GetCameraOrientation(float* angle, float* pitch)
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
void SetCameraOrientation(float* angle, float* pitch, srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->SetYaw(*angle);
    g_gd_camera_65a0f8->SetPitch(*pitch);
    *angle = g_gd_camera_65a0f8->m_yaw;
    *pitch = g_gd_camera_65a0f8->m_pitch;
    if (rotation != 0) {
        g_gd_camera_65a0f8->GetRotationMatrix(rotation);
    }
}

// FUNCTION: WIZ8 0x00421550
int GetCameraYawDegrees(void)
{
    return (int)(g_gd_camera_65a0f8->m_yaw * 57.295784f);
}

/* Mark the renderer ready and copy the point into the game camera when it
   sits anywhere but the origin. */
// FUNCTION: WIZ8 0x00421090
void PlacePartyAtPoint(const srVector3T<float>* point)
{
    srVector3T<float> delta = *point - g_origin_652940;
    if (sqrtf(DotProduct(delta, delta)) != g_zero_005ebb40) {
        MarkRendererReady();
        g_gd_camera_65a0f8->m_position_08c.x = point->x;
        g_gd_camera_65a0f8->m_position_08c.y = point->y;
        g_gd_camera_65a0f8->m_position_08c.z = point->z;
    }
}
