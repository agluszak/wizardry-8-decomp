#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"

/* Address quarantine 0041f261-0042403f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern unsigned char g_flag_00652da7;
extern const double g_double_005ebc18;
extern const float g_float_005ebcf8;

// FUNCTION: WIZ8 0x00420D40
void Function420D40(srNode* parent, W8Camera005EBE14* camera)
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    g_gd_camera_65a0f8->Method00476440(parent, camera);
}

// FUNCTION: WIZ8 0x00420E00
void Function420E00()
{
    g_gd_camera_65a0f8->Method00478E00(1);
}

// FUNCTION: WIZ8 0x00420F70
void Function420F70()
{
    g_gd_camera_65a0f8->Method00478CC0();
    g_flag_00652da7 = 0;
}

// FUNCTION: WIZ8 0x00420FD0
void Function420FD0(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->Method00477440(
        0.0f, (float)(scale * degrees), 0);
}

// FUNCTION: WIZ8 0x00421000
void Function421000(float degrees)
{
    double scale = g_double_005ebc18 * g_float_005ebcf8;
    g_gd_camera_65a0f8->Method00476C30(
        0.0f, (float)(scale * degrees));
}

// FUNCTION: WIZ8 0x00421030
void Function421030(srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->Method00476610(rotation, g_level_data_00652dac);
}

/* Two whole-body reads through the pointer at 0x0065A0F8. The first hands back
   the twelve bytes at 0x8C as one block; the second converts the float at 0x04
   from radians to degrees and truncates it through the CRT's _ftol, which the
   original reaches as a tail jump because the conversion is the whole return
   value. The scale is one ULP above the float nearest 180/pi, so the original
   spelled it as a decimal literal rather than computing it from a pi constant;
   the literal here is the shortest decimal that reproduces the stored bytes. */
// FUNCTION: WIZ8 0x00421070
void GetPosition421070(W8Position* position)
{
    *position = g_gd_camera_65a0f8->m_position_08c;
}

// FUNCTION: WIZ8 0x00421100
void Function421100(float distance, W8Position* output)
{
    W8Position result = *output;
    g_gd_camera_65a0f8->Method00478CE0(distance, &result);
    *output = result;
}

// FUNCTION: WIZ8 0x00421150
void Function421150(float distance, W8Position* output)
{
    g_gd_camera_65a0f8->Method00478CE0(distance, output);
}

// FUNCTION: WIZ8 0x004213E0
void Function4213E0(
    float* angle, float* pitch, srMatrix3T<float>* rotation)
{
    g_gd_camera_65a0f8->Method00478720(*angle);
    g_gd_camera_65a0f8->Method004784C0(*pitch);
    *angle = g_gd_camera_65a0f8->m_angle_004;
    *pitch = g_gd_camera_65a0f8->m_positional_008;
    if (rotation != 0) {
        g_gd_camera_65a0f8->Method00478BD0(rotation);
    }
}

// FUNCTION: WIZ8 0x00421550
int GetAngleInDegrees421550(void)
{
    return (int)(g_gd_camera_65a0f8->m_angle_004 * 57.295784f);
}

// FUNCTION: WIZ8 0x00421F30
IDirectDraw2* GetDirectDraw2(void)
{
    return g_direct_draw2_6596a0;
}
