#pragma once

#ifdef __cplusplus
unsigned char InitializeMenuFonts(void);
#endif

#include "vobject.h"
#include "Types.h"
#include "wiz8/sgp_bridge.h"

/* The game-specific font catalog: role-selected font handles, their derived
   video objects, and the palette pointers the menu initializer fills in.
   startup_subsystems.cpp owns the definitions; every consumer includes this
   header instead of redeclaring them. Only ghTinyMonoFont reaches the SGP C
   translation units and stays C-linked through sgp_bridge.h. */
extern int g_calligraphy_shadow_font;
extern int g_calligraphy_font;
extern HVOBJECT g_button_font_object;
extern int g_engraved_font;
extern HVOBJECT g_wiz_text_font_object;
extern int g_monster_damage_font;
extern HVOBJECT g_tiny_mono_font_object;
extern HVOBJECT g_calligraphy_shadow_font_object;
extern int g_options_detail_font;
extern HVOBJECT g_large_font_object;
extern HVOBJECT g_options_detail_font_object;
extern HVOBJECT g_small_font_object;
extern HVOBJECT g_engraved_font_object;
extern HVOBJECT g_calligraphy_font_object;
extern HVOBJECT g_profession_font_object;
extern int g_wiz_text_mono_font;
extern HVOBJECT g_smfnt_font_object;
extern HVOBJECT g_small_font_secondary_object;
extern HVOBJECT g_font12point1_object;
extern int g_wiz_text_font;
extern int g_embossed_font;
extern int g_font12point1;
extern HVOBJECT g_monster_damage_font_object;
extern HVOBJECT g_options_title_font_object;
extern int g_dialog_font_683654;
extern int g_profession_font;
extern HVOBJECT g_wiz_text_bold_font_object;
extern int g_wiz_text_font_secondary;
extern int g_wiz_text_bold_font;
extern int g_font10arial;
extern int g_small_font_secondary;
extern int g_button_font;
extern int g_large_font;
extern int g_small_font;
extern HVOBJECT g_font10arial_object;
extern HVOBJECT g_wiz_text_font_secondary_object;
extern HVOBJECT g_dialog_font_object;
extern HVOBJECT g_embossed_font_object;
extern int g_options_title_font;
extern int g_smfnt_font;
extern unsigned short* g_font_palette_calligraphy;
extern unsigned short* g_font_palette_options_detail;
extern unsigned short* g_font_palette_button;
extern unsigned short* g_colour_68ee08;
extern unsigned short* g_font_palette_wiz_text_bold;
extern unsigned short* g_font_palette_smfnt;
extern unsigned short* g_font_palette_wiz_text;
extern unsigned short* g_font_palette_calligraphy_shadow;
extern unsigned short* g_font_state_palettes[15];
