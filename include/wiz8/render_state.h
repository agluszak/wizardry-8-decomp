#ifndef WIZ8_RENDER_STATE_H
#define WIZ8_RENDER_STATE_H

#include "wiz8/wiz8_windows.h"

class srColorSurface;
class srGERD;
class srMaterial;
class srNode;
class stSurface2D;

extern "C" {

extern unsigned char* g_render_options_65a118;
extern srGERD* g_gerd_659634;
extern LPDIRECTDRAWSURFACE2 g_primary_surface_6596a8;
extern stSurface2D* g_surface_node_659664;
extern srMaterial* g_blit_material_65967c;
extern srColorSurface* g_mouse_surface_659688;
extern srNode* g_surface_nodes_654adc[0x12c0];
extern unsigned char g_block_652ddc[0x12c0];
/* Globals the accessor bodies below reach and that more than one recovered
   unit needs. They were each declared locally in the unit that first used
   them; holding them here instead is what stops a second unit from spelling
   the same object a second, divergent way. */
extern IDirectDraw2* g_direct_draw2_6596a0;
extern int g_dword_65962c;
extern int g_renderer_mode_603d74;
/* The pair the mode-select bodies write together, promoted here for the same
   reason: renderer_window.cpp defines them and the recovered setters read them
   from outside it. */
extern int g_dword_6596ec;
extern int g_dword_6596f0;
/* Defined by bringup_gates.cpp, written by a setter recovered outside it. */
extern int g_dword_5ff5f0;

extern int g_surface_state_6595dc;
extern int g_surface_state_654ad8;
extern int g_viewport_left_6595e8;
extern int g_viewport_top_6595ec;
extern int g_viewport_right_6595f0;
extern int g_viewport_bottom_6595f4;
extern int g_dword_6596d8;

}

#endif
