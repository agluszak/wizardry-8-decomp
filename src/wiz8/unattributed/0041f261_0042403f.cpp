#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"

/* Address quarantine 0041f261-0042403f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern unsigned char g_flag_00652da7;
extern const double g_double_005ebc18;
extern const float g_float_005ebcf8;

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
