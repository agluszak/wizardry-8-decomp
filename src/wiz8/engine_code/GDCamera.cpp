#include "wiz8/engine_code/GDCamera.h"

#include "surrender/srNode.h"
#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/float_constants.h"
#include "wiz8/game_state.h"
#include "wiz8/utility.h"

#include <math.h>
#include <float.h>

extern const double g_camera_view_factor_005ec300;
extern const double g_camera_view_factor_005ec538;
extern const double g_camera_view_factor_005ec568;
extern const float g_zero_005ebb34;
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
extern const double g_camera_smoothing_scale_005ec580;
extern const double g_camera_step_average_005ebe80;
extern const float g_camera_input_deceleration_005ec590;
extern const float g_camera_negative_input_deceleration_005ec58c;
extern const float g_camera_velocity_stop_scale_005ec588;
extern const float g_camera_negative_velocity_epsilon_005ec594;
extern const float g_camera_velocity_factor_005ec55c;
extern const float g_camera_step_factor_005ebc7c;
extern const double g_camera_pi_005ec2a0;
extern const float g_camera_horizontal_margin_005ec574;
extern const float g_camera_vertical_margin_005ec570;
extern const float g_camera_half_pi_005ec3fc;
extern float g_startup_depth_603ac8;
extern float g_camera_transition_speed_65a0f4;
extern float g_camera_max_yaw_velocity_609ea4;
extern float g_camera_level_forward_scale_603aac;
extern unsigned char g_flag_00683f97;
extern unsigned char g_flag_006875a5;
extern "C" void MarkRendererReady(void);
extern float Function4BE420(
    const W8Position* from, const W8Position* to);
extern float Function4BE490(
    const W8Position* from, const W8Position* to);

// FUNCTION: WIZ8 0x00476140
GDCamera::GDCamera()
{
    srVector3T<float> temporary;
    srVector3T<float> final_temporary;
    srMatrix3T<float>* first_matrix = &m_pitch_rotation;
    srMatrix3T<float>* second_matrix = &m_yaw_rotation;
    float pitch;
    float angle;

    m_positional_000 = 0;
    m_target_angle_098 = 0.0f;
    m_target_pitch_09c = 0.0f;
    m_position_08c.x = 0.0f;
    m_position_08c.y = 0.0f;
    m_position_08c.z = 0.0f;
    m_position_08c.y = g_startup_depth_603ac8;
    m_transition_active = 0;

    pitch = 0.0f;
    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_pitch = pitch;

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
    m_yaw = angle;

    second_matrix->vectors[0] = *temporary.method_00421680(1.0, 0.0, 0.0);
    second_matrix->vectors[1] = *temporary.method_00421680(0.0, 1.0, 0.0);
    second_matrix->vectors[2] = *final_temporary.method_00421680(0.0, 0.0, 1.0);
    if ((double)angle != g_zero_005ebb40) {
        second_matrix->method_00438F90(sin((double)angle), cos((double)angle));
    }
    MarkRendererReady();

    m_frame_elapsed = 0.0f;
    m_transition_active = 0;
    m_forced_transition = 0;
    m_target_angle_098 = 0.0f;
    m_target_pitch_09c = 0.0f;
    m_start_angle_0a0 = 0.0f;
    m_start_pitch_0a4 = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
    m_pitch_velocity_0ac = 0.0f;
    m_angle_distance_0b0 = 0.0f;
    m_pitch_distance_0b4 = 0.0f;
    m_transition_duration_0b8 = 0.0f;
    m_manual_input_timer = new W8IntervalGate(1.0f, 0, 1);

    m_rotation = *second_matrix;
    m_rotation.method_00421A40(m_pitch_rotation);
}

/* Creates the game camera when a parent is supplied, otherwise installs or
   updates a caller-provided camera. Both paths finish by applying the game
   camera owner's current rotation. */
// FUNCTION: WIZ8 0x00476440
W8Camera* GDCamera::CreateOrAttachCamera(
    srNode* parent, W8Camera* camera)
{
    if (parent != 0) {
        srVector3T<double> position;

        g_game_camera_65a0fc = new W8Camera(parent);
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

    m_transition_active = 0;
    m_forced_transition = 0;
    g_game_camera_65a0fc->setRotation(m_rotation);
    return g_game_camera_65a0fc;
}

// FUNCTION: WIZ8 0x00476610
void GDCamera::ApplyRotationMatrix(
    srMatrix3T<float>* rotation, W8LevelDataRecord* context)
{
    float forward_x = rotation->vectors[0].z;
    float forward_y = rotation->vectors[1].z;
    float forward_z = rotation->vectors[2].z;
    float angle = 0.0f;
    float pitch = 0.0f;

    if (forward_x != g_zero_005ebb34
        || forward_y != g_zero_005ebb34
        || forward_z != g_float_005ebb38) {
        if (context != 0) {
            context->camera_forward_4c.x =
                forward_x * g_camera_level_forward_scale_603aac;
            context->camera_forward_4c.y =
                forward_y * g_camera_level_forward_scale_603aac;
            context->camera_forward_4c.z =
                forward_z * g_camera_level_forward_scale_603aac;
            context->scaled_camera_forward_7c.x =
                context->camera_forward_4c.x * context->camera_scale_14;
            context->scaled_camera_forward_7c.y =
                context->camera_forward_4c.y * context->camera_scale_14;
            context->scaled_camera_forward_7c.z =
                context->camera_forward_4c.z * context->camera_scale_14;
        }

        if (forward_y > g_float_005ebb38) {
            forward_y = g_float_005ebb38;
        } else if (forward_y < g_negative_one_005ebc38) {
            forward_y = g_negative_one_005ebc38;
        }
        pitch = (float)acos((double)forward_y) - g_camera_half_pi_005ec3fc;

        float horizontal_scale =
            g_float_005ebb38
            / (float)sqrt((double)(forward_x * forward_x
                                   + forward_z * forward_z));
        forward_x *= horizontal_scale;
        forward_z *= horizontal_scale;
        if (forward_z > g_float_005ebb38) {
            forward_z = g_float_005ebb38;
        } else if (forward_z < g_negative_one_005ebc38) {
            forward_z = g_negative_one_005ebc38;
        }
        angle = (float)acos((double)forward_z);
        if (forward_x < g_zero_005ebb34) {
            angle = g_camera_angle_period_005ec54c - angle;
        }
    }

    if (_finite((double)angle) != 0 && _finite((double)pitch) != 0) {
        SetOrientation(angle, pitch);
        *rotation = m_rotation;
    }
}

// FUNCTION: WIZ8 0x00476950
void GDCamera::SnapToTarget(const W8Position* target)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return;
        }
        W8IntervalGate* timer = m_manual_input_timer;
        if (timer->IsFinished() == 0) {
            timer->PollElapsedIntervals();
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
    if (y >= g_float_005ebb38) {
        y = g_float_005ebb38;
    } else if (y < g_negative_one_005ebc38) {
        y = g_negative_one_005ebc38;
    }
    float pitch = (float)-asin((double)y);
    if (z >= g_float_005ebb38) {
        z = g_float_005ebb38;
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
        W8IntervalGate* timer = m_manual_input_timer;
        if (timer->IsFinished() == 0) {
            timer->PollElapsedIntervals();
        }
        if (timer->IsFinished() == 0) {
            return;
        }
    }

    m_target_pitch_09c = pitch;
    m_target_angle_098 = angle;
    m_positional_000 = 0x80;
    m_transition_active = 0;
    SetYaw(angle);
    SetPitch(pitch);
    m_pitch_velocity_0ac = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
}

// FUNCTION: WIZ8 0x00476C30
void GDCamera::SetOrientationImmediate(float pitch, float angle)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return;
        }
        W8IntervalGate* timer = m_manual_input_timer;
        if (timer->IsFinished() == 0) {
            timer->PollElapsedIntervals();
        }
        if (timer->IsFinished() == 0) {
            return;
        }
    }

    m_target_pitch_09c = pitch;
    m_target_angle_098 = angle;
    m_positional_000 = 0x80;
    m_transition_active = 0;
    SetYaw(angle);
    SetPitch(pitch);
    m_pitch_velocity_0ac = 0.0f;
    m_angle_velocity_0a8 = 0.0f;
}

// FUNCTION: WIZ8 0x00476F90
unsigned char GDCamera::LookAt(
    const W8Position* target, unsigned char preserve_pitch)
{
    if (g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return 0;
        }
        W8IntervalGate* timer = m_manual_input_timer;
        if (timer->IsFinished() == 0) {
            timer->PollElapsedIntervals();
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
        pitch = m_pitch;
    } else {
        if (y >= g_float_005ebb38) {
            y = g_float_005ebb38;
        } else if (y < g_negative_one_005ebc38) {
            y = g_negative_one_005ebc38;
        }
        pitch = (float)-asin((double)y);
    }
    if (z >= g_float_005ebb38) {
        z = g_float_005ebb38;
    } else if (z < g_negative_one_005ebc38) {
        z = g_negative_one_005ebc38;
    }
    float angle = (float)acos((double)z);
    if (x < g_zero_005ebb34) {
        angle = g_camera_angle_period_005ec014 - angle;
    }
    return BeginOrientationTransition(pitch, angle, 0);
}

// FUNCTION: WIZ8 0x00477180
unsigned char GDCamera::ComputeTrackingOrientation(
    const W8Position* target, float* angle, float* pitch)
{
    float lower_margin = -g_camera_vertical_margin_005ec570;
    if (g_level_block->camera_mode_100 == 1
        || g_level_block->camera_mode_100 == 2) {
        lower_margin =
            -g_camera_vertical_margin_005ec570 * g_camera_step_factor_005ebc7c;
    }

    float angle_delta =
        (NormalizeAngle(Function4BE420(&m_position_08c, target))
         + g_camera_angle_period_005ec014)
        - (NormalizeAngle(m_yaw) + g_camera_angle_period_005ec014);
    float pitch_delta =
        (Function4BE490(&m_position_08c, target)
         + g_camera_angle_period_005ec014)
        - (m_pitch + g_camera_angle_period_005ec014);
    if ((double)fabs((double)angle_delta) > g_camera_pi_005ec2a0) {
        if (angle_delta >= 0.0f) {
            angle_delta -= g_camera_angle_period_005ec014;
        } else {
            angle_delta += g_camera_angle_period_005ec014;
        }
    }
    if ((double)fabs((double)pitch_delta) > g_camera_pi_005ec2a0) {
        if (pitch_delta >= 0.0f) {
            pitch_delta -= g_camera_angle_period_005ec014;
        } else {
            pitch_delta += g_camera_angle_period_005ec014;
        }
    }

    if ((target->x != m_position_08c.x || target->z != m_position_08c.z)
        && ((float)fabs((double)angle_delta) > g_camera_horizontal_margin_005ec574
            || pitch_delta >= g_camera_vertical_margin_005ec570
            || pitch_delta <= lower_margin)) {
        if ((float)fabs((double)angle_delta)
            > g_camera_horizontal_margin_005ec574) {
            float correction =
                g_camera_horizontal_margin_005ec574
                * g_camera_step_factor_005ebc7c;
            angle_delta += angle_delta >= 0.0f ? -correction : correction;
        }
        if (pitch_delta < g_camera_vertical_margin_005ec570) {
            float correction =
                g_camera_vertical_margin_005ec570
                * g_camera_step_factor_005ebc7c;
            pitch_delta += pitch_delta >= 0.0f ? -correction : correction;
        }
        if (pitch_delta > lower_margin) {
            float correction =
                -lower_margin * g_camera_step_factor_005ebc7c;
            pitch_delta += pitch_delta >= 0.0f ? -correction : correction;
        }

        *angle = NormalizeAngle(angle_delta + m_yaw);
        *pitch = pitch_delta + m_pitch;
        if (*pitch > g_camera_pitch_upper_005ec550
            || *pitch < g_camera_pitch_lower_005ec554) {
            *pitch = m_pitch;
        }
        return 0;
    }

    *angle = m_yaw;
    *pitch = m_pitch;
    return 1;
}

// FUNCTION: WIZ8 0x00477440
unsigned char GDCamera::BeginOrientationTransition(
    float target_pitch, float target_angle, unsigned char force)
{
    if (force == 0 && g_flag_00683f97 == 0) {
        if ((m_positional_000 & 1) != 0) {
            return 0;
        }
        W8IntervalGate* timer = m_manual_input_timer;
        if (timer->IsFinished() == 0) {
            timer->PollElapsedIntervals();
        }
        if (timer->IsFinished() == 0) {
            return 0;
        }
    }

    m_target_pitch_09c = target_pitch;
    m_target_angle_098 = target_angle;
    m_forced_transition = force;
    m_transition_active = 0;

    float speed = g_camera_transition_speed_65a0f4;
    if (force != 0) {
        speed = g_camera_forced_speed_005ec560;
    }

    float raw_angle_distance =
        (float)fabs((double)(target_angle - m_yaw));
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
        (float)fabs((double)(target_pitch - m_pitch));
    if (m_angle_distance_0b0 + m_pitch_distance_0b4
        > g_camera_transition_epsilon_005ebc84) {
        m_start_pitch_0a4 = m_pitch;
        m_transition_active = 1;
        m_start_angle_0a0 = m_yaw;

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

        if ((target_angle < m_yaw
             && raw_angle_distance <= g_camera_half_period_005ec564)
            || (m_yaw <= target_angle
                && raw_angle_distance > g_camera_half_period_005ec564)) {
            m_angle_velocity_0a8 = -m_angle_velocity_0a8;
        }
        if (target_pitch < m_pitch) {
            m_pitch_velocity_0ac = -m_pitch_velocity_0ac;
        }
    }
    return m_transition_active;
}

// FUNCTION: WIZ8 0x004776A0
void GDCamera::Update(float elapsed)
{
    m_frame_elapsed = elapsed;
    if (m_transition_active == 0 && m_forced_transition == 0) {
        BrakePitchAtLimit();
        return;
    }

    float angle_traveled =
        (float)fabs((double)(m_start_angle_0a0 - m_yaw));
    if (angle_traveled > g_camera_half_period_005ec564) {
        angle_traveled = g_camera_angle_period_005ec014 - angle_traveled;
    }
    float pitch_traveled =
        (float)fabs((double)(m_start_pitch_0a4 - m_pitch));
    if (angle_traveled > m_angle_distance_0b0) {
        angle_traveled = m_angle_distance_0b0;
    }
    if (pitch_traveled > m_pitch_distance_0b4) {
        pitch_traveled = m_pitch_distance_0b4;
    }

    float phase;
    if (m_forced_transition == 0) {
        float doubled_progress;
        if (m_angle_distance_0b0 <= m_pitch_distance_0b4) {
            doubled_progress =
                (pitch_traveled + pitch_traveled) / m_pitch_distance_0b4;
        } else {
            doubled_progress =
                (angle_traveled + angle_traveled) / m_angle_distance_0b0;
        }
        float eased_input = g_float_005ebb38 - doubled_progress;
        if (eased_input > g_float_005ebb38) {
            eased_input = g_float_005ebb38;
        } else if (eased_input < g_negative_one_005ebc38) {
            eased_input = g_negative_one_005ebc38;
        }
        phase = (float)(acos((double)eased_input)
                            * g_camera_smoothing_scale_005ec580);
    } else if (m_angle_distance_0b0 <= m_pitch_distance_0b4) {
        phase = g_float_005ebb38
                - pitch_traveled / m_pitch_distance_0b4;
    } else {
        phase = g_float_005ebb38
                - angle_traveled / m_angle_distance_0b0;
    }

    float next_time = phase * m_transition_duration_0b8 + elapsed;
    if (next_time <= m_transition_duration_0b8) {
        float step;
        if (m_forced_transition == 0) {
            float next_weight = (float)sin(
                (double)(next_time / m_transition_duration_0b8)
                * (double)g_camera_half_period_005ec564);
            float current_weight = (float)sin(
                (double)phase * (double)g_camera_half_period_005ec564);
            step = (float)fabs(
                (double)((next_weight + current_weight) * elapsed)
                * g_camera_step_average_005ebe80);
        } else {
            step = elapsed;
        }
        m_yaw += step * m_angle_velocity_0a8;
        m_pitch += step * m_pitch_velocity_0ac;
        if (m_yaw > g_camera_angle_period_005ec014) {
            m_yaw -= g_camera_angle_period_005ec014;
        } else if (m_yaw < g_zero_005ebb34) {
            m_yaw += g_camera_angle_period_005ec014;
        }
    } else {
        m_pitch = m_target_pitch_09c;
        m_yaw = m_target_angle_098;
        if ((m_positional_000 & 0x20) == 0) {
            m_angle_velocity_0a8 = 0.0f;
            m_positional_000 &= ~0x40UL;
        }
        m_pitch_velocity_0ac = 0.0f;
        m_transition_active = 0;
        m_positional_000 &= ~0x20UL;
    }

    SetYaw(m_yaw);
    SetPitch(m_pitch);
}

// FUNCTION: WIZ8 0x00477B90
void GDCamera::ApplyYawInput(float input)
{
    if (input != g_zero_005ebb34) {
        if (g_flag_00683f97 == 0 && g_flag_006875a5 == 0) {
            m_positional_000 |= 1;
            m_manual_input_timer->Arm();
            m_transition_active = 0;
        } else {
            m_positional_000 &= ~1UL;
        }
    }

    if (m_transition_active == 0 || (m_positional_000 & 0x20) != 0) {
        unsigned char decelerating_negative = 0;
        unsigned char decelerating_positive = 0;
        if (input == g_zero_005ebb34) {
            if (m_angle_velocity_0a8 < g_zero_005ebb34) {
                input = g_camera_input_deceleration_005ec590;
                decelerating_negative = 1;
            } else if (m_angle_velocity_0a8 > g_zero_005ebb34) {
                input = g_camera_negative_input_deceleration_005ec58c;
                decelerating_positive = 1;
            } else {
                m_angle_velocity_0a8 = 0.0f;
                m_positional_000 &= ~0x40UL;
                return;
            }
        }

        m_angle_velocity_0a8 += input * m_frame_elapsed;
        if ((decelerating_negative != 0
             && m_angle_velocity_0a8 > g_zero_005ebb34)
            || (decelerating_positive != 0
                && m_angle_velocity_0a8 < g_zero_005ebb34)) {
            m_angle_velocity_0a8 = 0.0f;
            m_positional_000 &= ~0x40UL;
            return;
        }
        if (m_angle_velocity_0a8 > g_camera_max_yaw_velocity_609ea4) {
            m_angle_velocity_0a8 = g_camera_max_yaw_velocity_609ea4;
        } else if (m_angle_velocity_0a8 < -g_camera_max_yaw_velocity_609ea4) {
            m_angle_velocity_0a8 = -g_camera_max_yaw_velocity_609ea4;
        }

        m_yaw += m_frame_elapsed * m_angle_velocity_0a8;
        if (m_yaw > g_camera_angle_period_005ec54c) {
            m_yaw -= g_camera_angle_period_005ec014;
        }
        if (m_yaw < g_camera_angle_lower_005ec548) {
            m_yaw += g_camera_angle_period_005ec014;
        }
        if ((m_positional_000 & 0x20) != 0) {
            m_target_angle_098 = m_yaw;
        }
        SetYaw(m_yaw);
        m_positional_000 |= 0x40;
    }
}

// FUNCTION: WIZ8 0x00477EB0
void GDCamera::ApplyPitchInput(float input)
{
    if (input != g_zero_005ebb34
        && g_flag_00683f97 == 0 && g_flag_006875a5 == 0) {
        m_positional_000 |= 1;
        m_manual_input_timer->Arm();
        m_transition_active = 0;
    } else {
        m_positional_000 &= ~1UL;
    }
    if (m_transition_active != 0) {
        return;
    }
    if (input > g_zero_005ebb34 && (m_positional_000 & 8) != 0) {
        return;
    }
    if (input < g_zero_005ebb34 && (m_positional_000 & 4) != 0) {
        return;
    }

    unsigned char decelerating_negative = 0;
    unsigned char decelerating_positive = 0;
    if (input == g_zero_005ebb34) {
        if (m_pitch_velocity_0ac < g_camera_negative_velocity_epsilon_005ec594) {
            input = g_camera_input_deceleration_005ec590;
            decelerating_negative = 1;
        } else if (m_pitch_velocity_0ac > g_camera_transition_epsilon_005ebc84) {
            input = g_camera_negative_input_deceleration_005ec58c;
            decelerating_positive = 1;
        } else {
            m_pitch_velocity_0ac = 0.0f;
            return;
        }
    } else if (input <= g_zero_005ebb34) {
        m_positional_000 &= ~0x38UL;
    } else {
        m_positional_000 &= ~0x34UL;
    }

    m_pitch_velocity_0ac += input * m_frame_elapsed;
    if ((decelerating_negative != 0
         && m_pitch_velocity_0ac > g_zero_005ebb34)
        || (decelerating_positive != 0
            && m_pitch_velocity_0ac < g_zero_005ebb34)) {
        m_pitch_velocity_0ac = 0.0f;
        return;
    }
    if (m_pitch_velocity_0ac > g_camera_input_deceleration_005ec590) {
        m_pitch_velocity_0ac = g_camera_input_deceleration_005ec590;
    } else if (m_pitch_velocity_0ac
               < g_camera_negative_input_deceleration_005ec58c) {
        m_pitch_velocity_0ac = g_camera_negative_input_deceleration_005ec58c;
    }

    float stopping_distance =
        m_pitch_velocity_0ac * g_camera_velocity_stop_scale_005ec588
        * g_camera_velocity_factor_005ec55c * m_pitch_velocity_0ac
        * g_camera_step_factor_005ebc7c;
    if (m_pitch_velocity_0ac < g_zero_005ebb34) {
        stopping_distance = -stopping_distance;
    }
    m_pitch += m_frame_elapsed * m_pitch_velocity_0ac;
    if (m_pitch >= g_zero_005ebb34) {
        if (m_pitch + stopping_distance > g_camera_pitch_upper_005ec550) {
            m_positional_000 |= 0x18;
        }
    } else if (m_pitch + stopping_distance < g_camera_pitch_lower_005ec554) {
        m_positional_000 |= 0x14;
    }
    if (m_pitch > g_camera_pitch_upper_005ec550) {
        m_pitch = g_camera_pitch_upper_005ec550;
        m_pitch_velocity_0ac = 0.0f;
    }
    if (m_pitch < g_camera_pitch_lower_005ec554) {
        m_pitch = g_camera_pitch_lower_005ec554;
        m_pitch_velocity_0ac = 0.0f;
    }
    SetPitch(m_pitch);
}

// FUNCTION: WIZ8 0x00478290
void GDCamera::BrakePitchAtLimit()
{
    if ((m_positional_000 & 0x10) == 0) {
        return;
    }

    float limit = g_camera_pitch_upper_005ec550;
    if (m_pitch < g_zero_005ebb34) {
        limit = g_camera_pitch_lower_005ec554;
    }
    float braking_time =
        ((limit - m_pitch) / m_pitch_velocity_0ac) * 2.0f;
    if (braking_time < g_zero_005ebb34) {
        braking_time = -braking_time;
    }
    if (m_frame_elapsed <= braking_time) {
        float next_velocity =
            (g_float_005ebb38 - m_frame_elapsed / braking_time)
            * m_pitch_velocity_0ac;
        m_pitch +=
            (next_velocity + m_pitch_velocity_0ac) * m_frame_elapsed
            * g_camera_step_factor_005ebc7c;
        m_pitch_velocity_0ac = next_velocity;
    } else {
        if (m_pitch < g_zero_005ebb34) {
            m_pitch = g_camera_pitch_lower_005ec554;
        } else {
            m_pitch = g_camera_pitch_upper_005ec550;
        }
        m_positional_000 &= ~0x1cUL;
    }
    SetPitch(m_pitch);
}

// FUNCTION: WIZ8 0x004784C0
void GDCamera::SetPitch(float pitch)
{
    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_pitch = pitch;

    m_pitch_rotation.vectors[0].x = 1.0f;
    m_pitch_rotation.vectors[0].y = 0.0f;
    m_pitch_rotation.vectors[0].z = 0.0f;
    m_pitch_rotation.vectors[1].x = 0.0f;
    m_pitch_rotation.vectors[1].y = 1.0f;
    m_pitch_rotation.vectors[1].z = 0.0f;
    m_pitch_rotation.vectors[2].x = 0.0f;
    m_pitch_rotation.vectors[2].y = 0.0f;
    m_pitch_rotation.vectors[2].z = 1.0f;
    if ((double)pitch != g_zero_005ebb40) {
        float sine = (float)sin((double)pitch);
        float cosine = (float)cos((double)pitch);
        srVector3T<float> rotation[3];
        rotation[0].x = 1.0f;
        rotation[0].y = 0.0f;
        rotation[0].z = 0.0f;
        rotation[1].x = 0.0f;
        rotation[1].y = cosine;
        rotation[1].z = -sine;
        rotation[2].x = 0.0f;
        rotation[2].y = sine;
        rotation[2].z = cosine;

        W8CameraMatrixRow004D6930 result[3];
        float* result_values = &result[0].x;
        const float* rotation_values = &rotation[0].x;
        for (int index = 0; index != 3; ++index) {
            srVector3T<float> column;
            column.x = rotation_values[index];
            column.y = rotation_values[index + 3];
            column.z = rotation_values[index + 6];
            result_values[index] =
                Function4218E0(m_pitch_rotation.vectors[0], column);
            result_values[index + 3] =
                Function4218E0(m_pitch_rotation.vectors[1], column);
            result_values[index + 6] =
                Function4218E0(m_pitch_rotation.vectors[2], column);
        }
        m_pitch_rotation.vectors[0].x = result[0].x;
        m_pitch_rotation.vectors[0].y = result[0].y;
        m_pitch_rotation.vectors[0].z = result[0].z;
        m_pitch_rotation.vectors[1].x = result[1].x;
        m_pitch_rotation.vectors[1].y = result[1].y;
        m_pitch_rotation.vectors[1].z = result[1].z;
        m_pitch_rotation.vectors[2].x = result[2].x;
        m_pitch_rotation.vectors[2].y = result[2].y;
        m_pitch_rotation.vectors[2].z = result[2].z;
    }
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x00478720
void GDCamera::SetYaw(float angle)
{
    while (angle > g_camera_angle_period_005ec54c) {
        angle -= g_camera_angle_period_005ec54c;
    }
    while (angle < g_camera_angle_lower_005ec548) {
        angle += g_camera_angle_period_005ec54c;
    }
    m_yaw = angle;

    m_yaw_rotation.vectors[0].x = 1.0f;
    m_yaw_rotation.vectors[0].y = 0.0f;
    m_yaw_rotation.vectors[0].z = 0.0f;
    m_yaw_rotation.vectors[1].x = 0.0f;
    m_yaw_rotation.vectors[1].y = 1.0f;
    m_yaw_rotation.vectors[1].z = 0.0f;
    m_yaw_rotation.vectors[2].x = 0.0f;
    m_yaw_rotation.vectors[2].y = 0.0f;
    m_yaw_rotation.vectors[2].z = 1.0f;
    if ((double)angle != g_zero_005ebb40) {
        m_yaw_rotation.method_00438F90(
            sin((double)angle), cos((double)angle));
    }
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x004788E0
void GDCamera::SetOrientation(float angle, float pitch)
{
    while (angle > g_camera_angle_period_005ec54c) {
        angle -= g_camera_angle_period_005ec54c;
    }
    while (angle < g_camera_angle_lower_005ec548) {
        angle += g_camera_angle_period_005ec54c;
    }
    m_yaw = angle;
    m_yaw_rotation.vectors[0].x = 1.0f;
    m_yaw_rotation.vectors[0].y = 0.0f;
    m_yaw_rotation.vectors[0].z = 0.0f;
    m_yaw_rotation.vectors[1].x = 0.0f;
    m_yaw_rotation.vectors[1].y = 1.0f;
    m_yaw_rotation.vectors[1].z = 0.0f;
    m_yaw_rotation.vectors[2].x = 0.0f;
    m_yaw_rotation.vectors[2].y = 0.0f;
    m_yaw_rotation.vectors[2].z = 1.0f;
    if ((double)angle != g_zero_005ebb40) {
        m_yaw_rotation.method_00438F90(
            sin((double)angle), cos((double)angle));
    }

    if (pitch > g_camera_pitch_upper_005ec550) {
        pitch = g_camera_pitch_upper_005ec550;
    }
    if (pitch < g_camera_pitch_lower_005ec554) {
        pitch = g_camera_pitch_lower_005ec554;
    }
    m_pitch = pitch;
    m_pitch_rotation.vectors[0].x = 1.0f;
    m_pitch_rotation.vectors[0].y = 0.0f;
    m_pitch_rotation.vectors[0].z = 0.0f;
    m_pitch_rotation.vectors[1].x = 0.0f;
    m_pitch_rotation.vectors[1].y = 1.0f;
    m_pitch_rotation.vectors[1].z = 0.0f;
    m_pitch_rotation.vectors[2].x = 0.0f;
    m_pitch_rotation.vectors[2].y = 0.0f;
    m_pitch_rotation.vectors[2].z = 1.0f;
    if ((double)pitch != g_zero_005ebb40) {
        m_pitch_rotation.method_00478EB0(
            sin((double)pitch), cos((double)pitch));
    }

    m_rotation = m_yaw_rotation;
    m_rotation.method_00421A40(m_pitch_rotation);
    MarkRendererReady();
}

// FUNCTION: WIZ8 0x00478BD0
void GDCamera::GetRotationMatrix(srMatrix3T<float>* output)
{
    srMatrix3T<float>* composed = &m_rotation;
    *composed = m_yaw_rotation;
    const float* right_values = &m_pitch_rotation.vectors[0].x;
    W8CameraMatrixRow004D6930 result[3];
    float* result_values = &result[0].x;
    const float* left_values = &composed->vectors[0].x;
    for (int index = 0; index != 3; ++index) {
        float x = right_values[index];
        float y = right_values[index + 3];
        float z = right_values[index + 6];

        result_values[index] = x * left_values[0] + y * left_values[1]
                               + z * left_values[2];
        result_values[index + 3] = x * left_values[3]
                                   + y * left_values[4]
                                   + z * left_values[5];
        result_values[index + 6] = x * left_values[6]
                                   + y * left_values[7]
                                   + z * left_values[8];
    }
    composed->vectors[0].x = result[0].x;
    composed->vectors[0].y = result[0].y;
    composed->vectors[0].z = result[0].z;
    composed->vectors[1].x = result[1].x;
    composed->vectors[1].y = result[1].y;
    composed->vectors[2].x = result[2].x;
    composed->vectors[1].z = result[1].z;
    composed->vectors[2].y = result[2].y;
    composed->vectors[2].z = result[2].z;
    *output = *composed;
}

// FUNCTION: WIZ8 0x00478CC0
void GDCamera::BeginLeveling()
{
    m_positional_000 |= 0x20;
    BeginOrientationTransition(0.0f, m_yaw, 0);
}

// FUNCTION: WIZ8 0x00478CE0
void GDCamera::GetForwardPoint(float distance, W8Position* output)
{
    m_rotation = m_yaw_rotation;
    m_rotation.method_00421A40(m_pitch_rotation);
    m_direction_078.x = 0.0f;
    m_direction_078.y = 0.0f;
    m_direction_078.z = 1.0f;

    float x = m_rotation.vectors[0].x * m_direction_078.x
              + m_rotation.vectors[0].y * m_direction_078.y
              + m_rotation.vectors[0].z * m_direction_078.z;
    float y = m_rotation.vectors[1].x * m_direction_078.x
              + m_rotation.vectors[1].y * m_direction_078.y
              + m_rotation.vectors[1].z * m_direction_078.z;
    float z = Function4218E0(m_rotation.vectors[2], m_direction_078);
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
void GDCamera::SetManualControlActive(unsigned char enabled)
{
    if (enabled != 0 && g_flag_00683f97 == 0 && g_flag_006875a5 == 0) {
        m_positional_000 |= 1;
        m_manual_input_timer->Arm();
        m_transition_active = 0;
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
