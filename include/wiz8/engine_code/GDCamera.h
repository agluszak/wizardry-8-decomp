#pragma once

#include "surrender/srMath.h"
#include "wiz8/geometry.h"

class srNode;
class W8Camera005EBE14;
class W8Object005EBCFC;
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

    W8Camera005EBE14* Method00476440(
        srNode* parent, W8Camera005EBE14* camera);   /* 0x00476440 */
    void Method00476610(
        srMatrix3T<float>* rotation,
        W8LevelDataRecord* context);                /* 0x00476610 */
    void Method00476950(const W8Position* target);   /* 0x00476950 */
    void Method00476C30(float pitch, float angle);   /* 0x00476C30 */
    unsigned char Method00476F90(
        const W8Position* target,
        unsigned char preserve_pitch);              /* 0x00476F90 */
    unsigned char Method00477180(
        const W8Position* target,
        float* angle,
        float* pitch);                              /* 0x00477180 */
    unsigned char Method00477440(
        float target_pitch, float target_angle,
        unsigned char force);                       /* 0x00477440 */
    void Method004776A0(float elapsed);              /* 0x004776A0 */
    void Method00477B90(float input);                /* 0x00477B90 */
    void Method00477EB0(float input);                /* 0x00477EB0 */
    void Method00478290();                           /* 0x00478290 */
    void Method004784C0(float pitch);                /* 0x004784C0 */
    void Method00478720(float angle);                /* 0x00478720 */
    void Method004788E0(float angle, float pitch);   /* 0x004788E0 */
    void Method00478BD0(srMatrix3T<float>* output);  /* 0x00478BD0 */
    void Method00478CC0();                          /* 0x00478CC0 */
    void Method00478CE0(
        float distance, W8Position* output);         /* 0x00478CE0 */
    void Method00478E00(unsigned char enabled);      /* 0x00478E00 */

    unsigned long m_positional_000;                  /* 0x000 */
    float m_angle_004;                               /* 0x004 */
    float m_positional_008;                          /* 0x008 */
    srMatrix3T<float> m_matrix_00c;                   /* 0x00c */
    srMatrix3T<float> m_matrix_030;                   /* 0x030 */
    srMatrix3T<float> m_matrix_054;                   /* 0x054 */
    srVector3T<float> m_direction_078;                /* 0x078 */
    float m_elapsed_084;                             /* 0x084 */
    unsigned char m_flag_088;                        /* 0x088 */
    unsigned char m_flag_089;                        /* 0x089 */
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
    W8Object005EBCFC* m_owned_0bc;                   /* 0x0bc */
};

extern GDCamera* g_gd_camera_65a0f8;
extern W8Camera005EBE14* g_game_camera_65a0fc;

void Function420D40(srNode* parent, W8Camera005EBE14* camera);
void Function420E00();
void Function420F70();
void Function420FD0(float degrees);
void Function421000(float degrees);
void Function421030(srMatrix3T<float>* rotation);
void Function421100(float distance, W8Position* output);
void Function421150(float distance, W8Position* output);
void Function4213E0(
    float* angle, float* pitch, srMatrix3T<float>* rotation);

static_assert(sizeof(GDCamera) == 0xc0, "GDCamera_must_be_0xc0");
