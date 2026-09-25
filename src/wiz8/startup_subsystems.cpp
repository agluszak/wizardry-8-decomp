#include "wiz8/layouts/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/regions.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/engine_code/Video2.h"
#include "Font.h"
#include "FileMan.h"
#include "vobject.h"

#include <stdlib.h>
#include <string.h>

/* Unresolved fragment: the globals here sit in the unbracketed .data tail
   and the lone function 0x004E27A0 lies in the anchored gap between Arnika.cpp
   (ends 0x004E24E0) and Gameloop.cpp (starts 0x004E34B0), the same interval
   as game_init.cpp's two functions. No original-TU ownership is proven. */

// GLOBAL: WIZ8 0x0065beaf
bool g_texture_cache_enabled;

// GLOBAL: WIZ8 0x006835f4
int g_calligraphy_shadow_font;
// GLOBAL: WIZ8 0x006835f8
int g_calligraphy_font;
// GLOBAL: WIZ8 0x006835FC
HVOBJECT g_button_font_object;
// GLOBAL: WIZ8 0x00683600
int g_engraved_font;
// GLOBAL: WIZ8 0x00683604
HVOBJECT g_wiz_text_font_object;
// GLOBAL: WIZ8 0x00683608
int g_monster_damage_font;
// GLOBAL: WIZ8 0x0068360C
HVOBJECT g_tiny_mono_font_object;
// GLOBAL: WIZ8 0x00683610
HVOBJECT g_calligraphy_shadow_font_object;
// GLOBAL: WIZ8 0x00683614
int g_options_detail_font_683614;
// GLOBAL: WIZ8 0x00683618
HVOBJECT g_large_font_object;
// GLOBAL: WIZ8 0x0068361C
HVOBJECT g_options_detail_font_object;
// GLOBAL: WIZ8 0x00683620
HVOBJECT g_small_font_object;
// GLOBAL: WIZ8 0x00683624
HVOBJECT g_engraved_font_object;
// GLOBAL: WIZ8 0x00683628
HVOBJECT g_calligraphy_font_object;
// GLOBAL: WIZ8 0x0068362C
HVOBJECT g_profession_font_object;
// GLOBAL: WIZ8 0x00683630
int g_wiz_text_mono_font;
// GLOBAL: WIZ8 0x00683634
HVOBJECT g_smfnt_font_object;
// GLOBAL: WIZ8 0x00683638
HVOBJECT g_small_font_secondary_object;
// GLOBAL: WIZ8 0x0068363C
HVOBJECT g_font12point1_object;
// GLOBAL: WIZ8 0x00683640
int g_wiz_text_font;
// GLOBAL: WIZ8 0x00683644
int g_embossed_font;
// GLOBAL: WIZ8 0x00683648
int g_font12point1;
// GLOBAL: WIZ8 0x0068364C
HVOBJECT g_monster_damage_font_object;
// GLOBAL: WIZ8 0x00683650
HVOBJECT g_options_title_font_object;
// GLOBAL: WIZ8 0x00683654
int g_dialog_font_683654;
// GLOBAL: WIZ8 0x00683658
int g_profession_font;
// GLOBAL: WIZ8 0x0068365C
HVOBJECT g_wiz_text_bold_font_object;
// GLOBAL: WIZ8 0x00683660
int g_font_683660;
// GLOBAL: WIZ8 0x00683664
int g_wiz_text_bold_font;
// GLOBAL: WIZ8 0x00683668
int g_font10arial;
// GLOBAL: WIZ8 0x0068366C
int g_small_font_secondary;
// GLOBAL: WIZ8 0x00683670
int g_button_font;
// GLOBAL: WIZ8 0x00683674
int g_large_font;
// GLOBAL: WIZ8 0x00683678
int g_small_font;
// GLOBAL: WIZ8 0x0068367C
HVOBJECT g_font10arial_object;
// GLOBAL: WIZ8 0x00683680
HVOBJECT g_wiz_text_font_secondary_object;
// GLOBAL: WIZ8 0x00683684
HVOBJECT g_dialog_font_object;
// GLOBAL: WIZ8 0x00683688
HVOBJECT g_embossed_font_object;
// GLOBAL: WIZ8 0x0068368C
int g_options_title_font;
// GLOBAL: WIZ8 0x00683690
int ghTinyMonoFont;
// GLOBAL: WIZ8 0x00683694
int g_smfnt_font;

// GLOBAL: WIZ8 0x0068edfc
unsigned short* g_font_palette_calligraphy;
// GLOBAL: WIZ8 0x0068ee00
unsigned short* g_font_palette_options_detail;
// GLOBAL: WIZ8 0x0068ee04
unsigned short* g_font_palette_button;
// GLOBAL: WIZ8 0x0068ee08
unsigned short* g_colour_68ee08;
// GLOBAL: WIZ8 0x0068ee0c
unsigned short* g_font_palette_wiz_text_bold;
// GLOBAL: WIZ8 0x0068ee10
unsigned short* g_font_palette_smfnt;
// GLOBAL: WIZ8 0x0068ee14
unsigned short* g_font_palette_wiz_text;
// GLOBAL: WIZ8 0x0068ee18
unsigned short* g_font_palette_calligraphy_shadow;
// GLOBAL: WIZ8 0x0068EE1C
unsigned short* g_font_state_palettes[15];

/* The game-specific font catalog layered over SGP's source-owned font and
   video-object managers.  The individual globals are intentional: consumers
   select fonts by role, while this initializer preserves the retail load and
   derived-object order. */
// FUNCTION: WIZ8 0x004e27a0
unsigned char InitializeMenuFonts(void)
{
    char path[64];
    unsigned int index;

#define LOAD_FONT(destination, filename)                                                           \
    strcpy(path, filename);                                                                        \
    destination = LoadFontFile(                                                                    \
        reinterpret_cast<UINT8*>(path) /* reinterpret-ok: SGP API declared UINT8* for text */)

    LOAD_FONT(g_large_font, "Data\\Fonts\\LargeFont.sti");
    LOAD_FONT(g_small_font, "Data\\Fonts\\SmallFont.sti");
    LOAD_FONT(g_small_font_secondary, "Data\\Fonts\\SmallFont.sti");
    LOAD_FONT(g_wiz_text_font, "Data\\Fonts\\Wiz_Text_Font.sti");
    LOAD_FONT(g_calligraphy_font, "Data\\Fonts\\CalligraphyFont.sti");
    LOAD_FONT(g_calligraphy_shadow_font, "Data\\Fonts\\CalligraphyFontFullShadow.sti");
    LOAD_FONT(g_smfnt_font, "Data\\Fonts\\SmFnt.sti");
    LOAD_FONT(ghTinyMonoFont, "Data\\Fonts\\TinyMonoFont.sti");
    LOAD_FONT(g_button_font, "Data\\Fonts\\ButtonFont.sti");
    LOAD_FONT(g_engraved_font, "Data\\Fonts\\Engraved.sti");
    LOAD_FONT(g_embossed_font, "Data\\Fonts\\Embossed.sti");
    LOAD_FONT(g_font_683660, "Data\\Fonts\\Wiz_Text_Font.sti");
    LOAD_FONT(g_wiz_text_bold_font, "Data\\Fonts\\Wiz_Text_Font_Bold.sti");
    LOAD_FONT(g_wiz_text_mono_font, "Data\\Fonts\\wiz_text_font_monopalette.sti");
    LOAD_FONT(g_options_title_font, "Data\\Fonts\\Opt_title_font.sti");
    LOAD_FONT(g_options_detail_font_683614, "Data\\Fonts\\Opt_detail_font.sti");
    LOAD_FONT(g_profession_font, "Data\\Fonts\\Profession.sti");
    LOAD_FONT(g_font10arial, "Data\\Fonts\\Font10Arial.sti");
    LOAD_FONT(g_dialog_font_683654, "Data\\Fonts\\dialog_font.sti");
    LOAD_FONT(g_monster_damage_font, "Data\\Fonts\\monsterdamage_font.sti");
    LOAD_FONT(g_font12point1, "Data\\Fonts\\FONT12POINT1.sti");

#undef LOAD_FONT

    g_large_font_object = GetFontObject(g_large_font);
    g_small_font_object = GetFontObject(g_small_font);
    g_small_font_secondary_object = GetFontObject(g_small_font_secondary);
    g_wiz_text_font_object = GetFontObject(g_wiz_text_font);
    g_calligraphy_font_object = GetFontObject(g_calligraphy_font);
    g_calligraphy_shadow_font_object = GetFontObject(g_calligraphy_shadow_font);
    g_smfnt_font_object = GetFontObject(g_smfnt_font);
    g_tiny_mono_font_object = GetFontObject(ghTinyMonoFont);
    g_button_font_object = GetFontObject(g_button_font);
    g_engraved_font_object = GetFontObject(g_engraved_font);
    g_embossed_font_object = GetFontObject(g_embossed_font);
    g_wiz_text_font_secondary_object = GetFontObject(g_font_683660);
    g_wiz_text_bold_font_object = GetFontObject(g_wiz_text_bold_font);
    g_options_title_font_object = GetFontObject(g_options_title_font);
    g_options_detail_font_object = GetFontObject(g_options_detail_font_683614);
    g_profession_font_object = GetFontObject(g_profession_font);
    g_font10arial_object = GetFontObject(g_font10arial);
    g_dialog_font_object = GetFontObject(g_dialog_font_683654);
    g_monster_damage_font_object = GetFontObject(g_monster_damage_font);
    g_font12point1_object = GetFontObject(g_font12point1);

    CreateObjectPaletteTables(g_large_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_small_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_small_font_secondary_object, HVOBJECT_GLOW_RED);
    CreateObjectPaletteTables(g_wiz_text_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_calligraphy_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_calligraphy_shadow_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_smfnt_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_tiny_mono_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_button_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_engraved_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_embossed_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_wiz_text_font_secondary_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_wiz_text_bold_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_options_title_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_options_detail_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_profession_font_object, HVOBJECT_GLOW_BLUE);
    CreateObjectPaletteTables(g_dialog_font_object, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_font12point1_object, HVOBJECT_GLOW_GREEN);

    for (index = 0; index != 15; ++index) {
        EnsureCatalogFrameLoaded(0x1e5, index);
        g_font_state_palettes[index] = CopyCatalogImagePalette16BPP(0x1e5, index);
        if (!g_font_state_palettes[index]) {
            return 0;
        }
    }

    g_font_palette_smfnt = GetFontObjectPalette16BPP(g_smfnt_font);
    g_font_palette_calligraphy = GetFontObjectPalette16BPP(g_calligraphy_font);
    g_font_palette_calligraphy_shadow = GetFontObjectPalette16BPP(g_calligraphy_shadow_font);
    g_font_palette_wiz_text = GetFontObjectPalette16BPP(g_wiz_text_font);
    g_font_palette_button = GetFontObjectPalette16BPP(g_button_font);
    g_colour_68ee08 = GetFontObjectPalette16BPP(g_font_683660);
    g_font_palette_wiz_text_bold = GetFontObjectPalette16BPP(g_wiz_text_bold_font);
    g_font_palette_options_detail = GetFontObjectPalette16BPP(g_options_detail_font_683614);
    ConfigureDialogFont(g_dialog_font_683654, 1, 0xff, 0);
    return 1;
}
