#pragma once

#include "surrender/srMath.h"
#include "wiz8/geometry.h"

class srNode;
class W8Camera;
class W8IntervalGate;
struct W8LevelDataRecord;

/* The 12-byte empty-constructed row helper passed to VC6's three-element
   vector-constructor iterator by the camera rotation expressions. Nothing in
   the image exposes its source name, so the constructor address remains its
   stable identity. */
class W8CameraMatrixRow004D6930 {
public:
    W8CameraMatrixRow004D6930();                    /* 0x004D6930 */

    float x;
    float y;
    float z;
};

static_assert(sizeof(W8CameraMatrixRow004D6930) == 0x0c,
              "W8CameraMatrixRow004D6930_must_be_0x0c");

static_assert(sizeof(srMatrix3T<float>) == 0x24,
              "srMatrix3T_float_must_be_0x24");

/* Engine Code\Camera.cpp. GameData.cpp's original `gpGDCamera` assertion
   identifies the owner allocated at 0x0065A0F8; its constructor allocation
   proves the complete 0xC0-byte extent. Positional members remain named by
   offset until Camera.cpp's consumers establish their original roles. */
class GDCamera {
public:
    GDCamera();                                      /* 0x00476140 */

    W8Camera* CreateOrAttachCamera(
        srNode* parent, W8Camera* camera);   /* 0x00476440 */
    void ApplyRotationMatrix(
        srMatrix3T<float>* rotation,
        W8LevelDataRecord* context);                /* 0x00476610 */
    void SnapToTarget(const W8Position* target);   /* 0x00476950 */
    void SetOrientationImmediate(float pitch, float angle);   /* 0x00476C30 */
    unsigned char LookAt(
        const W8Position* target,
        unsigned char preserve_pitch);              /* 0x00476F90 */
    unsigned char ComputeTrackingOrientation(
        const W8Position* target,
        float* angle,
        float* pitch);                              /* 0x00477180 */
    unsigned char BeginOrientationTransition(
        float target_pitch, float target_angle,
        unsigned char force);                       /* 0x00477440 */
    void Update(float elapsed);              /* 0x004776A0 */
    void ApplyYawInput(float input);                /* 0x00477B90 */
    void ApplyPitchInput(float input);                /* 0x00477EB0 */
    void BrakePitchAtLimit();                           /* 0x00478290 */
    void SetPitch(float pitch);                /* 0x004784C0 */
    void SetYaw(float angle);                /* 0x00478720 */
    void SetOrientation(float angle, float pitch);   /* 0x004788E0 */
    void GetRotationMatrix(srMatrix3T<float>* output);  /* 0x00478BD0 */
    void BeginLeveling();                          /* 0x00478CC0 */
    void GetForwardPoint(
        float distance, W8Position* output);         /* 0x00478CE0 */
    void SetManualControlActive(unsigned char enabled);      /* 0x00478E00 */

    unsigned long m_positional_000;                  /* 0x000 */
    float m_yaw;                               /* 0x004 */
    float m_pitch;                          /* 0x008 */
    srMatrix3T<float> m_pitch_rotation;                   /* 0x00c */
    srMatrix3T<float> m_yaw_rotation;                   /* 0x030 */
    srMatrix3T<float> m_rotation;                   /* 0x054 */
    srVector3T<float> m_direction_078;                /* 0x078 */
    float m_frame_elapsed;                             /* 0x084 */
    unsigned char m_transition_active;                        /* 0x088 */
    unsigned char m_forced_transition;                        /* 0x089 */
    unsigned char m_padding_08a[2];
    W8Position m_position_08c;                       /* 0x08c */
    float m_target_angle_098;                        /* 0x098 */
    float m_target_pitch_09c;                        /* 0x09c */
    float m_start_angle_0a0;                         /* 0x0a0 */
    float m_start_pitch_0a4;                         /* 0x0a4 */
    float m_angle_velocity_0a8;                      /* 0x0a8 */
    float m_pitch_velocity_0ac;                      /* 0x0ac */
    float m_angle_distance_0b0;                      /* 0x0b0 */
    float m_pitch_distance_0b4;                      /* 0x0b4 */
    float m_transition_duration_0b8;                 /* 0x0b8 */
    W8IntervalGate* m_manual_input_timer;                   /* 0x0bc */
};

extern GDCamera* g_gd_camera_65a0f8;
extern W8Camera* g_game_camera_65a0fc;

W8Camera* CreateOrSetGameCamera(
    srNode* parent, W8Camera* camera);
float GetCameraYawRadians();
void BeginManualCameraControl();
void LevelCamera();
void TurnCameraToDegrees(float degrees);
void SetCameraYawDegrees(float degrees);
void ApplyCameraRotation(srMatrix3T<float>* rotation);
void Function421100(float distance, W8Position* output);
void Function421150(float distance, W8Position* output);
void SetCameraOrientation(
    float* angle, float* pitch, srMatrix3T<float>* rotation);

static_assert(sizeof(GDCamera) == 0xc0, "GDCamera_must_be_0xc0");
