#pragma once

unsigned char InitializeMenuFonts(void);

#include "vobject.h"
#include "Types.h"

/* The game-specific font catalog: role-selected font handles, their derived
   video objects, and the palette pointers the menu initializer fills in.
   startup_subsystems.cpp owns the definitions; every consumer includes this
   header instead of redeclaring them. */
extern int ghTinyMonoFont;
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
extern int g_wiz_dialog_font;
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
extern unsigned short* g_wiz_text_font_secondary_palette;
extern unsigned short* g_font_palette_wiz_text_bold;
extern unsigned short* g_font_palette_smfnt;
extern unsigned short* g_font_palette_wiz_text;
extern unsigned short* g_font_palette_calligraphy_shadow;
/* Frames of the notice palette catalog (Data\Fonts\Palette*.sti). */
enum W8FontPaletteIndex {
    W8_FONT_PALETTE_RED = 0,
    W8_FONT_PALETTE_GREEN = 1,
    W8_FONT_PALETTE_PURPLE = 2,
    W8_FONT_PALETTE_BLUE = 3,
    W8_FONT_PALETTE_ORANGE = 4,
    W8_FONT_PALETTE_YELLOW = 5,
    W8_FONT_PALETTE_PINK = 6,
    W8_FONT_PALETTE_BROWN = 7,
    W8_FONT_PALETTE_WHITE = 8,
    W8_FONT_PALETTE_RUST = 9,
    W8_FONT_PALETTE_BRONZE = 10,
    W8_FONT_PALETTE_GRAY = 11,
    W8_FONT_PALETTE_BEIGE = 12,
    W8_FONT_PALETTE_OPTIONS_GREEN = 13,
    W8_FONT_PALETTE_OPTIONS_WHITE = 14,
    W8_FONT_PALETTE_TEXT_BOX = 15,
    W8_FONT_PALETTE_COUNT = 16
};

extern unsigned short* g_font_state_palettes[15];
