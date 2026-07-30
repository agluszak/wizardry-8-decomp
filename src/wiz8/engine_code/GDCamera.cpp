#include "wiz8/engine_code/GDCamera.h"

#include "surrender/srNode.h"
#include "wiz8/engine_code/Object005EBCFC.h"
#include "wiz8/engine_code/registry_classes.h"

#include <math.h>

extern const double g_camera_view_factor_005ec300;
extern const double g_camera_view_factor_005ec538;
extern const double g_camera_view_factor_005ec568;
extern const float g_zero_005ebb34;
extern const float g_one_005ebb38;
extern const double g_zero_005ebb40;
extern const float g_negative_one_005ebc38;
extern const float g_camera_snap_epsilon_005ebc2c;
extern const float g_camera_transition_epsilon_005ebc84;
extern const float g_camera_angle_period_005ec014;
extern const float g_camera_angle_period_005ec54c;
extern const float g_camera_angle_lower_005ec548;
extern const float g_camera_pitch_upper_005ec550;
extern const float g_camera_pitch_lower_005ec554;
extern const float g_camera_transition_duration_factor_005ec558;
extern const float g_camera_forced_speed_005ec560;
extern const float g_camera_half_period_005ec564;
extern const float g_camera_angle_dead_zone_005ec578;
extern const float g_camera_transition_duration_scale_005ec57c;
extern float g_startup_depth_603ac8;
extern float g_camera_transition_speed_65a0f4;
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
    m_target_angle_098 = 0.0f;
    m_target_pitch_09c = 0.0f;
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

    m_elapsed_084 = 0.0f;
    m_flag_088 = 0;
    m_flag_089 = 0;
    m_target_angle_098 = 0.0f;
    m_target_pitch_09c = 0.0f;
    m_start_angle_0a0 = 0.0f;
    m_start_pitch_0a4 = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
    m_pitch_velocity_0ac = 0.0f;
    m_angle_distance_0b0 = 0.0f;
    m_pitch_distance_0b4 = 0.0f;
    m_transition_duration_0b8 = 0.0f;
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

// FUNCTION: WIZ8 0x00476950
void GDCamera::Method00476950(const W8Position* target)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return;
        }
        W8Object005EBCFC* timer = m_owned_0bc;
        if (timer->IsFinished() == 0) {
            timer->Method0043A5D0();
        }
        if (timer->IsFinished() == 0) {
            return;
        }
    }

    float x = target->x - m_position_08c.x;
    float y = target->y - m_position_08c.y;
    float z = target->z - m_position_08c.z;
    float length_squared = x * x + y * y + z * z;
    float length = (float)sqrt((double)length_squared);
    if (length < 1.0) {
        return;
    }
    if ((double)length_squared != g_zero_005ebb40) {
        float scale = (float)(1.0 / sqrt((double)length_squared));
        x *= scale;
        y *= scale;
        z *= scale;
    }

    float horizontal_scale =
        (float)(1.0 / sqrt((double)(x * x + z * z)));
    x *= horizontal_scale;
    z *= horizontal_scale;
    if (y >= g_one_005ebb38) {
        y = g_one_005ebb38;
    } else if (y < g_negative_one_005ebc38) {
        y = g_negative_one_005ebc38;
    }
    float pitch = (float)-asin((double)y);
    if (z >= g_one_005ebb38) {
        z = g_one_005ebb38;
    } else if (z < g_negative_one_005ebc38) {
        z = g_negative_one_005ebc38;
    }
    float angle = (float)acos((double)z);
    if (x < g_zero_005ebb34) {
        angle = g_camera_angle_period_005ec014 - angle;
    }

    m_target_pitch_09c = pitch;
    m_target_angle_098 = angle;
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return;
        }
        W8Object005EBCFC* timer = m_owned_0bc;
        if (timer->IsFinished() == 0) {
            timer->Method0043A5D0();
        }
        if (timer->IsFinished() == 0) {
            return;
        }
    }

    m_target_pitch_09c = pitch;
    m_target_angle_098 = angle;
    m_positional_000 = 0x80;
    m_flag_088 = 0;
    Method00478720(angle);
    Method004784C0(pitch);
    m_pitch_velocity_0ac = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
}

// FUNCTION: WIZ8 0x00476C30
void GDCamera::Method00476C30(float pitch, float angle)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return;
        }
        W8Object005EBCFC* timer = m_owned_0bc;
        if (timer->IsFinished() == 0) {
            timer->Method0043A5D0();
        }
        if (timer->IsFinished() == 0) {
            return;
        }
    }

    m_target_pitch_09c = pitch;
    m_target_angle_098 = angle;
    m_positional_000 = 0x80;
    m_flag_088 = 0;
    Method00478720(angle);
    Method004784C0(pitch);
    m_pitch_velocity_0ac = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
}

// FUNCTION: WIZ8 0x00476F90
unsigned char GDCamera::Method00476F90(
    const W8Position* target, unsigned char preserve_pitch)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return 0;
        }
        W8Object005EBCFC* timer = m_owned_0bc;
        if (timer->IsFinished() == 0) {
            timer->Method0043A5D0();
        }
        if (timer->IsFinished() == 0) {
            return 0;
        }
    }

    float x = target->x - m_position_08c.x;
    float y = target->y - m_position_08c.y;
    float z = target->z - m_position_08c.z;
    float length_squared = x * x + y * y + z * z;
    float length = (float)sqrt((double)length_squared);
    if (length < 1.0) {
        return 0;
    }
    if ((double)length_squared != g_zero_005ebb40) {
        float scale = (float)(1.0 / sqrt((double)length_squared));
        x *= scale;
        y *= scale;
        z *= scale;
    }

    float horizontal_scale =
        (float)(1.0 / sqrt((double)(x * x + z * z)));
    x *= horizontal_scale;
    z *= horizontal_scale;
    float pitch;
    if (preserve_pitch != 0) {
        pitch = m_positional_008;
    } else {
        if (y >= g_one_005ebb38) {
            y = g_one_005ebb38;
        } else if (y < g_negative_one_005ebc38) {
            y = g_negative_one_005ebc38;
        }
        pitch = (float)-asin((double)y);
    }
    if (z >= g_one_005ebb38) {
        z = g_one_005ebb38;
    } else if (z < g_negative_one_005ebc38) {
        z = g_negative_one_005ebc38;
    }
    float angle = (float)acos((double)z);
    if (x < g_zero_005ebb34) {
        angle = g_camera_angle_period_005ec014 - angle;
    }
    return Method00477440(pitch, angle, 0);
}

// FUNCTION: WIZ8 0x00477440
unsigned char GDCamera::Method00477440(
    float target_pitch, float target_angle, unsigned char force)
{
    if (force == 0 && g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return 0;
        }
        W8Object005EBCFC* timer = m_owned_0bc;
        if (timer->IsFinished() == 0) {
            timer->Method0043A5D0();
        }
        if (timer->IsFinished() == 0) {
            return 0;
        }
    }

    m_target_pitch_09c = target_pitch;
    m_target_angle_098 = target_angle;
    m_flag_089 = force;
    m_flag_088 = 0;

    float speed = g_camera_transition_speed_65a0f4;
    if (force != 0) {
        speed = g_camera_forced_speed_005ec560;
    }

    float raw_angle_distance =
        (float)fabs((double)(target_angle - m_angle_004));
    m_angle_distance_0b0 = raw_angle_distance;
    if (target_pitch != g_zero_005ebb34
        || raw_angle_distance >= g_camera_snap_epsilon_005ebc2c) {
        m_positional_000 = 0;
    } else {
        m_positional_000 &= 0x20;
    }

    while (m_angle_distance_0b0 > g_camera_angle_period_005ec014) {
        m_angle_distance_0b0 -= g_camera_angle_period_005ec014;
    }
    if (m_angle_distance_0b0 > g_camera_half_period_005ec564) {
        m_angle_distance_0b0 =
            g_camera_angle_period_005ec014 - m_angle_distance_0b0;
    }

    m_pitch_distance_0b4 =
        (float)fabs((double)(target_pitch - m_positional_008));
    if (m_angle_distance_0b0 + m_pitch_distance_0b4
        > g_camera_transition_epsilon_005ebc84) {
        m_start_pitch_0a4 = m_positional_008;
        m_flag_088 = 1;
        m_start_angle_0a0 = m_angle_004;

        if (m_angle_distance_0b0 <= m_pitch_distance_0b4) {
            m_pitch_velocity_0ac = speed;
            m_transition_duration_0b8 =
                m_pitch_distance_0b4
                * g_camera_transition_duration_scale_005ec57c
                * g_camera_transition_duration_factor_005ec558;
            if (m_angle_distance_0b0 <= g_camera_angle_dead_zone_005ec578) {
                m_angle_velocity_0a8 = 0.0f;
            } else {
                m_positional_000 |= 0x40;
                m_angle_velocity_0a8 =
                    (m_angle_distance_0b0 / m_pitch_distance_0b4) * speed;
            }
        } else {
            m_angle_velocity_0a8 = speed;
            m_positional_000 |= 0x40;
            m_pitch_velocity_0ac =
                (m_pitch_distance_0b4 / m_angle_distance_0b0) * speed;
            m_transition_duration_0b8 =
                m_angle_distance_0b0
                * g_camera_transition_duration_scale_005ec57c
                * g_camera_transition_duration_factor_005ec558;
        }

        if ((target_angle < m_angle_004
             && raw_angle_distance <= g_camera_half_period_005ec564)
            || (m_angle_004 <= target_angle
                && raw_angle_distance > g_camera_half_period_005ec564)) {
            m_angle_velocity_0a8 = -m_angle_velocity_0a8;
        }
        if (target_pitch < m_positional_008) {
            m_pitch_velocity_0ac = -m_pitch_velocity_0ac;
        }
    }
    return m_flag_088;
}

// FUNCTION: WIZ8 0x004784C0
void GDCamera::Method004784C0(float pitch)
{
    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_positional_008 = pitch;

    m_matrix_00c.vectors[0].x = 1.0f;
    m_matrix_00c.vectors[0].y = 0.0f;
    m_matrix_00c.vectors[0].z = 0.0f;
    m_matrix_00c.vectors[1].x = 0.0f;
    m_matrix_00c.vectors[1].y = 1.0f;
    m_matrix_00c.vectors[1].z = 0.0f;
    m_matrix_00c.vectors[2].x = 0.0f;
    m_matrix_00c.vectors[2].y = 0.0f;
    m_matrix_00c.vectors[2].z = 1.0f;
    if ((double)pitch != g_zero_005ebb40) {
        m_matrix_00c.method_00478EB0(
            sin((double)pitch), cos((double)pitch));
    }
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x00478720
void GDCamera::Method00478720(float angle)
{
    while (angle > g_camera_angle_period_005ec54c) {
        angle -= g_camera_angle_period_005ec54c;
    }
    while (angle < g_camera_angle_lower_005ec548) {
        angle += g_camera_angle_period_005ec54c;
    }
    m_angle_004 = angle;

    m_matrix_030.vectors[0].x = 1.0f;
    m_matrix_030.vectors[0].y = 0.0f;
    m_matrix_030.vectors[0].z = 0.0f;
    m_matrix_030.vectors[1].x = 0.0f;
    m_matrix_030.vectors[1].y = 1.0f;
    m_matrix_030.vectors[1].z = 0.0f;
    m_matrix_030.vectors[2].x = 0.0f;
    m_matrix_030.vectors[2].y = 0.0f;
    m_matrix_030.vectors[2].z = 1.0f;
    if ((double)angle != g_zero_005ebb40) {
        m_matrix_030.method_00438F90(
            sin((double)angle), cos((double)angle));
    }
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x004788E0
void GDCamera::Method004788E0(float angle, float pitch)
{
    while (angle > g_camera_angle_period_005ec54c) {
        angle -= g_camera_angle_period_005ec54c;
    }
    while (angle < g_camera_angle_lower_005ec548) {
        angle += g_camera_angle_period_005ec54c;
    }
    m_angle_004 = angle;
    m_matrix_030.vectors[0].x = 1.0f;
    m_matrix_030.vectors[0].y = 0.0f;
    m_matrix_030.vectors[0].z = 0.0f;
    m_matrix_030.vectors[1].x = 0.0f;
    m_matrix_030.vectors[1].y = 1.0f;
    m_matrix_030.vectors[1].z = 0.0f;
    m_matrix_030.vectors[2].x = 0.0f;
    m_matrix_030.vectors[2].y = 0.0f;
    m_matrix_030.vectors[2].z = 1.0f;
    if ((double)angle != g_zero_005ebb40) {
        m_matrix_030.method_00438F90(
            sin((double)angle), cos((double)angle));
    }

    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_positional_008 = pitch;
    m_matrix_00c.vectors[0].x = 1.0f;
    m_matrix_00c.vectors[0].y = 0.0f;
    m_matrix_00c.vectors[0].z = 0.0f;
    m_matrix_00c.vectors[1].x = 0.0f;
    m_matrix_00c.vectors[1].y = 1.0f;
    m_matrix_00c.vectors[1].z = 0.0f;
    m_matrix_00c.vectors[2].x = 0.0f;
    m_matrix_00c.vectors[2].y = 0.0f;
    m_matrix_00c.vectors[2].z = 1.0f;
    if ((double)pitch != g_zero_005ebb40) {
        m_matrix_00c.method_00478EB0(
            sin((double)pitch), cos((double)pitch));
    }

    m_matrix_054 = m_matrix_030;
    m_matrix_054.method_00421A40(m_matrix_00c);
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x00478BD0
void GDCamera::Method00478BD0(srMatrix3T<float>* output)
{
    m_matrix_054 = m_matrix_030;
    m_matrix_054.method_00421A40(m_matrix_00c);
    *output = m_matrix_054;
}

// FUNCTION: WIZ8 0x00478CC0
void GDCamera::Method00478CC0()
{
    m_positional_000 |= 0x20;
    Method00477440(0.0f, m_angle_004, 0);
}

// FUNCTION: WIZ8 0x00478CE0
void GDCamera::Method00478CE0(float distance, W8Position* output)
{
    m_matrix_054 = m_matrix_030;
    m_matrix_054.method_00421A40(m_matrix_00c);
    m_direction_078.x = 0.0f;
    m_direction_078.y = 0.0f;
    m_direction_078.z = 1.0f;

    float x = m_matrix_054.vectors[0].x * m_direction_078.x
              + m_matrix_054.vectors[0].y * m_direction_078.y
              + m_matrix_054.vectors[0].z * m_direction_078.z;
    float y = m_matrix_054.vectors[1].x * m_direction_078.x
              + m_matrix_054.vectors[1].y * m_direction_078.y
              + m_matrix_054.vectors[1].z * m_direction_078.z;
    float z = m_matrix_054.vectors[2].x * m_direction_078.x
              + m_matrix_054.vectors[2].y * m_direction_078.y
              + m_matrix_054.vectors[2].z * m_direction_078.z;
    m_direction_078.x = x;
    m_direction_078.y = y;
    m_direction_078.z = z;
    output->x = x;
    output->y = y;
    output->z = z;

    float length_squared = output->x * output->x
                           + output->y * output->y
                           + output->z * output->z;
    if ((double)length_squared != g_zero_005ebb40) {
        float scale = distance / (float)sqrt((double)length_squared);
        output->x *= scale;
        output->y *= scale;
        output->z *= scale;
    }
    output->x += m_position_08c.x;
    output->y += m_position_08c.y;
    output->z += m_position_08c.z;
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
