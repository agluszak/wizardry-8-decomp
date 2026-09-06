#include "wiz8/local_code/Controls.h"
#include "wiz8/xstatus.h"

/*
 * Local Screens\RCSCommon.cpp.
 *
 * Two panels the review-character screens share. Each owns a Controls object
 * and one widget inside it, and each has the same pair of bodies: one that
 * redraws the panel and one that tears both down.
 */

extern Controls* g_rcs_panel_a_0069c3c4;
extern Controls* g_rcs_panel_b_0069c3c8;
extern W8WidgetBase005ED5BC* g_rcs_widget_a_0069c3c0;
extern W8WidgetBase005ED5BC* g_rcs_widget_b_0069c400;
extern int g_rcs_mode_0064cbe8;
/* Ask the first panel to redraw all of itself. A null rectangle is how
   Controls::Invalidate spells "the whole area", so these are not a separate
   one-argument redraw slot - they are the panel class Local Code\Controls.cpp
   models, reached through its second vtable slot. */
// FUNCTION: WIZ8 0x005b6590
void RedrawRcsPanelA(void)
{
    g_rcs_panel_a_0069c3c4->Invalidate(0);
}

/* Redraw the second. */
// FUNCTION: WIZ8 0x005b68d0
void RedrawRcsPanelB(void)
{
    g_rcs_panel_b_0069c3c8->Invalidate(0);
}

/* Tear the first panel down, then destroy its separate widget. */
// FUNCTION: WIZ8 0x005b6540
void DestroyRcsPanelA(void)
{
    Controls* panel = g_rcs_panel_a_0069c3c4;

    if (panel != 0) {
        delete panel;
        g_rcs_panel_a_0069c3c4 = 0;
    }
    if (g_rcs_widget_a_0069c3c0 != 0) {
        delete g_rcs_widget_a_0069c3c0;
        g_rcs_widget_a_0069c3c0 = 0;
    }
}

/* The same for the second panel and its widget. */
// FUNCTION: WIZ8 0x005b6880
void DestroyRcsPanelB(void)
{
    Controls* panel = g_rcs_panel_b_0069c3c8;

    if (panel != 0) {
        delete panel;
        g_rcs_panel_b_0069c3c8 = 0;
    }
    if (g_rcs_widget_b_0069c400 != 0) {
        delete g_rcs_widget_b_0069c400;
        g_rcs_widget_b_0069c400 = 0;
    }
}

/* Bring the second panel up to date. Its widget is available only in the two
   leading modes, out of combat and out of camp; enabling it also invalidates the
   panel, disabling it does not. Either way the panel is then updated. */
// FUNCTION: WIZ8 0x005b68e0
void UpdateRcsPanelB(void)
{
    if ((g_rcs_mode_0064cbe8 == 0 || g_rcs_mode_0064cbe8 == 1) &&
        gXStatus.fCombatMode == 0 && gXStatus.fCampMode == 0) {
        if (g_rcs_widget_b_0069c400->m_flag_5 == 0) {
            g_rcs_widget_b_0069c400->SetEnabled(1);
            g_rcs_panel_b_0069c3c8->Invalidate(0);
        }
    }
    else if (g_rcs_widget_b_0069c400->m_flag_5 != 0) {
        g_rcs_widget_b_0069c400->SetEnabled(0);
    }
    g_rcs_panel_b_0069c3c8->Redraw();
}
