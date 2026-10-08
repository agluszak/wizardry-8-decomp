#pragma once

#include "surrender/srLight.h"

/* Monster's light: srLight plus colour-cycle/fade state. Retail never
   installs a vtable of its own: the constructor at 0x0049D500 leaves the
   imported srLight vtables that ??0srLight stored, and the copy constructor
   at 0x0049D660 ends on the srLight vtables it reloads from SR.DLL. Every
   virtual call (srLight::isActive/process vertex lighting, the deleting
   destructor) therefore goes to srLight, so the class is novtable and adds
   no overrides. The tables at 0x005ECD18/0x005ECD0C are the local
   srClassSupport<srLight> instantiation (see MonsterLight.cpp). */
class __declspec(novtable) MonsterLight : public srLight {
public:
    MonsterLight(srNode* parent, bool cycle_color, float range,
                 const srVector3T<float>* first_color,
                 const srVector3T<float>* second_color); /* 0x0049D500 */
    MonsterLight(const MonsterLight& other);             /* 0x0049D660 */
    void SetVisible(bool visible);
    void SetRange(float range);
    void Update(const srVector3T<float>* position);
    void StartFadeOut();

public:
    float m_vertical_offset;          /* 0x228 */
    srVector3T<float> m_color_first;  /* 0x22c */
    srVector3T<float> m_color_second; /* 0x238 */
    float m_start_time;               /* 0x244 */
    bool m_cycle_color;               /* 0x248 */
    bool m_fade_out;                  /* 0x249 */
    unsigned char m_padding_24a[6];
};

static_assert(sizeof(MonsterLight) == 0x250, "MonsterLight_must_be_0x250");
