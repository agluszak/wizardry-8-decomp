#pragma once

unsigned char InitializeMenuFonts(void);

#include "vobject.h"
#include "Types.h"

/* The game-specific font catalog: role-selected font handles, their derived
   video objects, and the palette pointers the menu initializer fills in.
   startup_subsystems.cpp owns the definitions; every consumer includes this
   header instead of redeclaring them, so all references keep the C linkage
   the definitions carry. */
extern "C" {
extern int g_calligraphy_shadow_font_6835f4;
extern int g_calligraphy_font_6835f8;
extern HVOBJECT g_button_font_object_6835fc;
extern int g_engraved_font_683600;
extern HVOBJECT g_wiz_text_font_object_683604;
extern int g_monster_damage_font_683608;
extern HVOBJECT g_tiny_mono_font_object_68360c;
extern HVOBJECT g_calligraphy_shadow_font_object_683610;
extern int g_options_detail_font_683614;
extern HVOBJECT g_large_font_object_683618;
extern HVOBJECT g_options_detail_font_object_68361c;
extern HVOBJECT g_small_font_object_683620;
extern HVOBJECT g_engraved_font_object_683624;
extern HVOBJECT g_calligraphy_font_object_683628;
extern HVOBJECT g_profession_font_object_68362c;
extern int g_wiz_text_mono_font_683630;
extern HVOBJECT g_smfnt_font_object_683634;
extern HVOBJECT g_small_font_secondary_object_683638;
extern HVOBJECT g_font12point1_object_68363c;
extern int g_wiz_text_font_683640;
extern int g_embossed_font_683644;
extern int g_font12point1_683648;
extern HVOBJECT g_monster_damage_font_object_68364c;
extern HVOBJECT g_options_title_font_object_683650;
extern int g_dialog_font_683654;
extern int g_profession_font_683658;
extern HVOBJECT g_wiz_text_bold_font_object_68365c;
extern int g_font_683660;
extern int g_wiz_text_bold_font_683664;
extern int g_font10arial_683668;
extern int g_small_font_secondary_68366c;
extern int g_button_font_683670;
extern int g_large_font_683674;
extern int g_small_font_683678;
extern HVOBJECT g_font10arial_object_68367c;
extern HVOBJECT g_wiz_text_font_secondary_object_683680;
extern HVOBJECT g_dialog_font_object_683684;
extern HVOBJECT g_embossed_font_object_683688;
extern int g_options_title_font_68368c;
extern int g_tiny_mono_font_683690;
extern int g_smfnt_font_683694;
extern unsigned short* g_font_palette_calligraphy_68edfc;
extern unsigned short* g_font_palette_options_detail_68ee00;
extern unsigned short* g_font_palette_button_68ee04;
extern unsigned short* g_colour_68ee08;
extern unsigned short* g_font_palette_wiz_text_bold_68ee0c;
extern unsigned short* g_font_palette_smfnt_68ee10;
extern unsigned short* g_font_palette_wiz_text_68ee14;
extern unsigned short* g_font_palette_calligraphy_shadow_68ee18;
extern unsigned short* g_font_state_palettes_68ee1c[15];
}
