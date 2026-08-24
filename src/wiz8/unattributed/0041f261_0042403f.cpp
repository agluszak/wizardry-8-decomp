#include "wiz8/float_constants.h"
#include "wiz8/screen_state.h"
#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Object0043A910.h"
#include "wiz8/engine_code/game_timer.h"
#include "surrender/srExtension.h"
#include "surrender/srGERD.h"
#include "surrender/srImporter.h"

#include <stdio.h>

/* Address quarantine 0041f261-0042403f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern unsigned char g_flag_00652da7;
extern unsigned char g_flag_00652dce;
extern const double g_double_005ebc18;
extern const float g_float_005ebcf8;
extern W8Object0043A910* g_object_6598bc;

// FUNCTION: WIZ8 0x00420b40
float Function420B40(int value)
{
    if (g_object_6598bc == 0) {
        g_object_6598bc = new W8Object0043A910;
        if (g_object_6598bc == 0) {
            return g_float_005ebb34;
        }
    }
    if (g_flag_00652dce != 0) {
        if ((value == 8 && g_screen_state_0068ec78.id == 7) || value == 4) {
            Function439CA0();
            g_flag_00652dce = 0;
        }
        else {
            return g_float_005ebb34;
        }
    }
    if (value == 1) {
        Function439BC0();
        g_flag_00652dce = 1;
    }
    return g_object_6598bc->GetValue28();
}

// FUNCTION: WIZ8 0x00420D40
srCamera* CreateOrSetGameCamera(
    srNode* parent, srCamera* camera)
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    return g_gd_camera_65a0f8->CreateOrAttachCamera(parent, camera);
}

// FUNCTION: WIZ8 0x00420DD0
float GetCameraYawRadians()
{
    return g_gd_camera_65a0f8->m_yaw;
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
    g_gd_camera_65a0f8->BeginOrientationTransition(
        0.0f, (float)(scale * degrees), 0);
}

// FUNCTION: WIZ8 0x00421000
void SetCameraYawDegrees(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->SetOrientationImmediate(
        0.0f, (float)(scale * degrees));
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

// FUNCTION: WIZ8 0x00421100
void Function421100(float distance, srVector3T<float>* output)
{
    srVector3T<float> result = *output;
    g_gd_camera_65a0f8->GetForwardPoint(distance, &result);
    *output = result;
}

// FUNCTION: WIZ8 0x00421150
void Function421150(float distance, srVector3T<float>* output)
{
    g_gd_camera_65a0f8->GetForwardPoint(distance, output);
}

// FUNCTION: WIZ8 0x004213E0
void SetCameraOrientation(
    float* angle, float* pitch, srMatrix3T<float>* rotation)
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

// FUNCTION: WIZ8 0x00421F30
IDirectDraw2* GetDirectDraw2(void)
{
    return g_direct_draw2_6596a0;
}

extern "C" unsigned char g_flag_6596f4;
int g_screenshot_index_659724;
int g_screenshot_page_659728;

// FUNCTION: WIZ8 0x004229e0
void Function4229E0(void)
{
    srSurfaceIOManager* surface_io_manager =
        srCore.getSurfaceIOManager();
    srExtension::load("JPEGImporter", 0);

    srColorSurfaceIFace* surface = g_gerd_659634->lockBuffer();
    int screenshot_index = g_screenshot_index_659724;
    if (surface != 0) {
        char filename[32];
        srSurfaceIOManager::ExportInfo options;
        options.unknown_00 = 0;
        options.unknown_04 = 1;
        options.option_string = 0;

        ++g_screenshot_index_659724;
        sprintf(filename, "Wiz8%5.5d.JPG", screenshot_index);
        if (g_flag_6596f4 == 0) {
            surface_io_manager->exportSurface(filename, *surface, options);
        }
        else {
            options.option_string = "QUALITY=0.35";
            Function439BC0();
            surface_io_manager->exportSurface(filename, *surface, options);
            Function439CA0();
        }
        g_gerd_659634->unlockBuffer();
    }
    g_screenshot_page_659728 = (g_screenshot_page_659728 - 1) & 1;
}
