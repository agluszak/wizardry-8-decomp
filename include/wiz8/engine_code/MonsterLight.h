#pragma once

#include "surrender/srLight.h"

/* Monster's copy constructor establishes ownership of this complete object.
   Its registry identity deliberately remains srLight: the concrete vtable is
   the evidence that distinguishes this specialization. */
// VTABLE: WIZ8 0x005ECD18 MonsterLight
// VTABLE: WIZ8 0x005ECD0C srVertexProcessor
class MonsterLight : public srLight {
public:
    MonsterLight(srNode* parent, unsigned char cycle_color, float range,
                 const srVector3T<float>* first_color,
                 const srVector3T<float>* second_color); /* 0x0049D500 */
    MonsterLight(const MonsterLight& other);             /* 0x0049D660 */
    void SetVisible0049D970(char visible);
    void SetRange(float range);
    void Update0049D990(const srVector3T<float>* position);
    void StartFadeOut();

    virtual ~MonsterLight() override; /* 0x0049E0D0 */

    /* The retail secondary table at 0x005ECD0C carries real override slots for
       both vertex-processor hooks: isActive's body (0x004D6190) is the
       constant-return-1 frame folded across the identically-shaped overrides
       that survive in the image, and process's (0x005B1BE0) is the shared
       ret-4 no-op. Neither has a surviving MonsterLight name, so the bodies
       remain inert here while their emissions stay owned by the folded
       originals they collided with. */
    virtual int isActive(srVertexPipe&) override
    {
        return 1;
    }
    virtual void process(srVertexPipe&) override {}

public:
    float m_vertical_offset_228;          /* 0x228 */
    srVector3T<float> m_color_first_22c;  /* 0x22c */
    srVector3T<float> m_color_second_238; /* 0x238 */
    float m_start_time_244;               /* 0x244 */
    unsigned char m_cycle_color_248;      /* 0x248 */
    unsigned char m_fade_out_249;         /* 0x249 */
    unsigned char m_padding_24a[6];
};

static_assert(sizeof(MonsterLight) == 0x250, "MonsterLight_must_be_0x250");
