#include "wiz8/engine_code/GDCamera.h"

#include "surrender/srNode.h"
#include "wiz8/engine_code/Object005EBCFC.h"
#include "wiz8/engine_code/registry_classes.h"

#include <math.h>

extern const double g_camera_view_factor_005ec300;
extern const double g_camera_view_factor_005ec538;
extern const double g_camera_view_factor_005ec568;
extern const float g_zero_005ebb34;
extern const double g_zero_005ebb40;
extern const float g_camera_angle_period_005ec54c;
extern const float g_camera_angle_lower_005ec548;
extern const float g_camera_pitch_upper_005ec550;
extern const float g_camera_pitch_lower_005ec554;
extern float g_startup_depth_603ac8;
extern unsigned char g_flag_00683f97;
extern unsigned char g_flag_006875a5;
extern "C" void MarkRendererReady(void);

// FUNCTION: WIZ8 0x00476140
GDCamera::GDCamera()
{
    srVector3T<float> temporary;
    srVector3T<float> final_temporary;
    srMatrix3T<float>* first_matrix = &m_matrix_00c;
    srMatrix3T<float>* second_matrix = &m_matrix_030;
    float pitch;
    float angle;

    m_positional_000 = 0;
    m_positional_098 = 0;
    m_positional_09c = 0;
    m_position_08c.x = 0.0f;
    m_position_08c.y = 0.0f;
    m_position_08c.z = 0.0f;
    m_position_08c.y = g_startup_depth_603ac8;
    m_flag_088 = 0;

    pitch = 0.0f;
    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_positional_008 = pitch;

    temporary.x = 1.0f;
    temporary.y = 0.0f;
    temporary.z = 0.0f;
    first_matrix->vectors[0] = temporary;
    first_matrix->vectors[1] = *temporary.method_00421680(0.0, 1.0, 0.0);
    first_matrix->vectors[2] = *temporary.method_00421680(0.0, 0.0, 1.0);
    if ((double)pitch != g_zero_005ebb40) {
        first_matrix->method_00478EB0(sin((double)pitch), cos((double)pitch));
    }
    MarkRendererReady();

    angle = g_zero_005ebb34;
    while (angle > g_camera_angle_period_005ec54c) {
        angle -= g_camera_angle_period_005ec54c;
    }
    while (angle < g_camera_angle_lower_005ec548) {
        angle += g_camera_angle_period_005ec54c;
    }
    m_angle_004 = angle;

    second_matrix->vectors[0] = *temporary.method_00421680(1.0, 0.0, 0.0);
    second_matrix->vectors[1] = *temporary.method_00421680(0.0, 1.0, 0.0);
    second_matrix->vectors[2] = *final_temporary.method_00421680(0.0, 0.0, 1.0);
    if ((double)angle != g_zero_005ebb40) {
        second_matrix->method_00438F90(sin((double)angle), cos((double)angle));
    }
    MarkRendererReady();

    m_positional_084 = 0;
    m_flag_088 = 0;
    m_flag_089 = 0;
    m_positional_098 = 0;
    m_positional_09c = 0;
    m_positional_0a0 = 0;
    m_positional_0a4 = 0;
    m_positional_0a8 = 0;
    m_positional_0ac = 0;
    m_positional_0b0 = 0;
    m_positional_0b4 = 0;
    m_positional_0b8 = 0;
    m_owned_0bc = new W8Object005EBCFC(1.0f, 0, 1);

    m_matrix_054 = *second_matrix;
    m_matrix_054.method_00421A40(m_matrix_00c);
}

/* Creates the game camera when a parent is supplied, otherwise installs or
   updates a caller-provided camera. Both paths finish by applying the game
   camera owner's current rotation. */
// FUNCTION: WIZ8 0x00476440
W8Camera005EBE14* GDCamera::Method00476440(
    srNode* parent, W8Camera005EBE14* camera)
{
    if (parent != 0) {
        srVector3T<double> position;

        g_game_camera_65a0fc = new W8Camera005EBE14(parent);
        g_game_camera_65a0fc->setName("Sirtech Camera");
        position.x = m_position_08c.x;
        position.y = m_position_08c.y;
        position.z = m_position_08c.z;
        g_game_camera_65a0fc->setLocation(position);
        g_game_camera_65a0fc->setClipRange(250.0, 75000.0);
        g_game_camera_65a0fc->setRotation(0.0, 0.0, 0.0);
        g_game_camera_65a0fc->setEnvironmentRange(0.0f, 1.0f);
        double view = g_camera_view_factor_005ec538
                      * g_camera_view_factor_005ec300
                      * g_camera_view_factor_005ec568;
        g_game_camera_65a0fc->setViewPlane(view, view);
    } else {
        srVector3T<double> position;

        if (camera == 0) {
            if (g_game_camera_65a0fc == 0) {
                return 0;
            }
        } else {
            g_game_camera_65a0fc = camera;
        }
        position.x = m_position_08c.x;
        position.y = m_position_08c.y;
        position.z = m_position_08c.z;
        g_game_camera_65a0fc->setLocation(position);
        g_game_camera_65a0fc->setRotation(0.0, 0.0, 0.0);
    }

    m_flag_088 = 0;
    m_flag_089 = 0;
    g_game_camera_65a0fc->setRotation(m_matrix_054);
    return g_game_camera_65a0fc;
}

// FUNCTION: WIZ8 0x00478CC0
void GDCamera::Method00478CC0()
{
    m_positional_000 |= 0x20;
    Method00477440(0.0f, m_angle_004, 0);
}

// FUNCTION: WIZ8 0x00478E00
void GDCamera::Method00478E00(unsigned char enabled)
{
    if (enabled != 0 && g_flag_00683f97 == 0 && g_flag_006875a5 == 0) {
        m_positional_000 |= 1;
        m_owned_0bc->Method0043A530();
        m_flag_088 = 0;
        return;
    }
    m_positional_000 &= ~1UL;
}

// FUNCTION: WIZ8 0x00478EB0
srMatrix3T<float>* srMatrix3T<float>::method_00478EB0(
    double sine, double cosine)
{
    W8CameraMatrixRow004D6930 basis[3];
    srMatrix3T<float> rotation;

    basis[0].x = 1.0f;
    basis[0].y = 0.0f;
    basis[0].z = 0.0f;
    basis[1].x = 0.0f;
    basis[1].y = (float)cosine;
    basis[1].z = -(float)sine;
    basis[2].x = 0.0f;
    basis[2].y = (float)sine;
    basis[2].z = (float)cosine;
    rotation.vectors[0].x = basis[0].x;
    rotation.vectors[0].y = basis[0].y;
    rotation.vectors[0].z = basis[0].z;
    rotation.vectors[1].x = basis[1].x;
    rotation.vectors[1].y = basis[1].y;
    rotation.vectors[1].z = basis[1].z;
    rotation.vectors[2].x = basis[2].x;
    rotation.vectors[2].y = basis[2].y;
    rotation.vectors[2].z = basis[2].z;
    method_00421A40(rotation);
    return this;
}
