#pragma once

#include "surrender/srMath.h"
#include "wiz8/geometry.h"

class srNode;
class W8Camera005EBE14;
class W8Object005EBCFC;

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
    unsigned char Method00477440(
        float positional_09c, float positional_098,
        unsigned char positional_089);              /* 0x00477440 */
    void Method00478CC0();                          /* 0x00478CC0 */
    void Method00478E00(unsigned char enabled);      /* 0x00478E00 */

    unsigned long m_positional_000;                  /* 0x000 */
    float m_angle_004;                               /* 0x004 */
    float m_positional_008;                          /* 0x008 */
    srMatrix3T<float> m_matrix_00c;                   /* 0x00c */
    srMatrix3T<float> m_matrix_030;                   /* 0x030 */
    srMatrix3T<float> m_matrix_054;                   /* 0x054 */
    unsigned char m_positional_078[0x0c];            /* 0x078 */
    unsigned long m_positional_084;                  /* 0x084 */
    unsigned char m_flag_088;                        /* 0x088 */
    unsigned char m_flag_089;                        /* 0x089 */
    unsigned char m_padding_08a[2];
    W8Position m_position_08c;                       /* 0x08c */
    unsigned long m_positional_098;                  /* 0x098 */
    unsigned long m_positional_09c;                  /* 0x09c */
    unsigned long m_positional_0a0;                  /* 0x0a0 */
    unsigned long m_positional_0a4;                  /* 0x0a4 */
    unsigned long m_positional_0a8;                  /* 0x0a8 */
    unsigned long m_positional_0ac;                  /* 0x0ac */
    unsigned long m_positional_0b0;                  /* 0x0b0 */
    unsigned long m_positional_0b4;                  /* 0x0b4 */
    unsigned long m_positional_0b8;                  /* 0x0b8 */
    W8Object005EBCFC* m_owned_0bc;                   /* 0x0bc */
};

extern GDCamera* g_gd_camera_65a0f8;
extern W8Camera005EBE14* g_game_camera_65a0fc;

static_assert(sizeof(GDCamera) == 0xc0, "GDCamera_must_be_0xc0");
