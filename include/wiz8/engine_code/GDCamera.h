#pragma once

#include "surrender/srMath.h"
#include "wiz8/geometry.h"

class srNode;
class W8Camera005EBE14;

/* Engine Code\Camera.cpp. GameData.cpp's original `gpGDCamera` assertion
   identifies the owner allocated at 0x0065A0F8; its constructor allocation
   proves the complete 0xC0-byte extent. Positional members remain named by
   offset until Camera.cpp's consumers establish their original roles. */
class GDCamera {
public:
    GDCamera();                                      /* 0x00476140 */

    W8Camera005EBE14* Method00476440(
        srNode* parent, W8Camera005EBE14* camera);   /* 0x00476440 */

    unsigned char m_positional_000[4];               /* 0x000 */
    float m_angle_004;                               /* 0x004 */
    float m_positional_008;                          /* 0x008 */
    srMatrix3T<float> m_matrix_00c;                  /* 0x00c */
    srMatrix3T<float> m_matrix_030;                  /* 0x030 */
    srMatrix3T<float> m_matrix_054;                  /* 0x054 */
    unsigned char m_positional_078[0x10];            /* 0x078 */
    unsigned char m_flag_088;                        /* 0x088 */
    unsigned char m_flag_089;                        /* 0x089 */
    unsigned char m_padding_08a[2];
    W8Position m_position_08c;                       /* 0x08c */
    unsigned char m_positional_098[0x24];            /* 0x098 */
    void* m_owned_0bc;                               /* 0x0bc */
};

extern GDCamera* g_gd_camera_65a0f8;
extern W8Camera005EBE14* g_game_camera_65a0fc;

static_assert(sizeof(GDCamera) == 0xc0, "GDCamera_must_be_0xc0");
