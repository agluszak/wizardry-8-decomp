#include "wiz8/engine_code/GDCamera.h"

// FUNCTION: WIZ8 0x00420e10
bool IsCameraTransitionActive(void)
{
    return g_gd_camera->m_transition_active;
}
