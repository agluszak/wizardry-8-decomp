#include "wiz8/engine_code/registry_classes.h"

/* Address quarantine 00497af0-004adb20; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern "C" {
extern float g_light_scale_0060bfe0;
extern float g_light_scale_identity_005ebb38;
}

// FUNCTION: WIZ8 0x0049C8D0
void stLight::process(
    const srNode::ProcessInfo& info,
    srNode::e_processType type)
{
    if ((type == 1 || type == 3) &&
        g_light_scale_0060bfe0 != g_light_scale_identity_005ebb38) {
        float saved_scale = m_positional_98;
        m_positional_98 = saved_scale * g_light_scale_0060bfe0;
        srLight::process(info, type);
        m_positional_98 = saved_scale;
        return;
    }
    srLight::process(info, type);
}

// FUNCTION: WIZ8 0x0049DC40
srNode* MonsterLight::vslot7()
{
    srLight* instance = (srLight*)vInstance();
    *instance = *this;
    return instance;
}
