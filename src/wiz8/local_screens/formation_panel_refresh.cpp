#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSFormation.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/engine_code/Video2.h"

// FUNCTION: WIZ8 0x005B2980
void RefreshFormationPanel(bool show_portraits)
{
    if (g_formation_panel->m_fEnabled) {
        if (show_portraits) {
            g_formation_panel->Invalidate(0);
            InvalidateRegion(0xd6, 0x3c, 0x1ab, 0x12f, 0);
        }
        g_formation_panel->Redraw();
    }
}
