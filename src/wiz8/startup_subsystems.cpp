#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/regions.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/render_state.h"
#include "Font.h"
#include "FileMan.h"
#include "vobject.h"

#include <stdlib.h>
#include <string.h>

// GLOBAL: WIZ8 0x0065beaf
unsigned char g_flag_65beaf;


// GLOBAL: WIZ8 0x006835f4
int g_calligraphy_shadow_font_6835f4;
// GLOBAL: WIZ8 0x006835f8
int g_calligraphy_font_6835f8;
HVOBJECT g_button_font_object_6835fc;
int g_engraved_font_683600;
HVOBJECT g_wiz_text_font_object_683604;
int g_monster_damage_font_683608;
HVOBJECT g_tiny_mono_font_object_68360c;
HVOBJECT g_calligraphy_shadow_font_object_683610;
// GLOBAL: WIZ8 0x00683614
int g_options_detail_font_683614;
HVOBJECT g_large_font_object_683618;
HVOBJECT g_options_detail_font_object_68361c;
HVOBJECT g_small_font_object_683620;
HVOBJECT g_engraved_font_object_683624;
HVOBJECT g_calligraphy_font_object_683628;
HVOBJECT g_profession_font_object_68362c;
int g_wiz_text_mono_font_683630;
HVOBJECT g_smfnt_font_object_683634;
HVOBJECT g_small_font_secondary_object_683638;
HVOBJECT g_font12point1_object_68363c;
// GLOBAL: WIZ8 0x00683640
int g_wiz_text_font_683640;
int g_embossed_font_683644;
int g_font12point1_683648;
HVOBJECT g_monster_damage_font_object_68364c;
HVOBJECT g_options_title_font_object_683650;
int g_dialog_font_683654;
int g_profession_font_683658;
HVOBJECT g_wiz_text_bold_font_object_68365c;
// GLOBAL: WIZ8 0x00683660
int g_font_683660;
// GLOBAL: WIZ8 0x00683664
int g_wiz_text_bold_font_683664;
int g_font10arial_683668;
int g_small_font_secondary_68366c;
// GLOBAL: WIZ8 0x00683670
int g_button_font_683670;
int g_large_font_683674;
int g_small_font_683678;
HVOBJECT g_font10arial_object_68367c;
HVOBJECT g_wiz_text_font_secondary_object_683680;
HVOBJECT g_dialog_font_object_683684;
HVOBJECT g_embossed_font_object_683688;
int g_options_title_font_68368c;
int ghTinyMonoFont;
// GLOBAL: WIZ8 0x00683694
int g_smfnt_font_683694;

// GLOBAL: WIZ8 0x0068edfc
unsigned short* g_font_palette_calligraphy_68edfc;
// GLOBAL: WIZ8 0x0068ee00
unsigned short* g_font_palette_options_detail_68ee00;
// GLOBAL: WIZ8 0x0068ee04
unsigned short* g_font_palette_button_68ee04;
// GLOBAL: WIZ8 0x0068ee08
unsigned short* g_colour_68ee08;
// GLOBAL: WIZ8 0x0068ee0c
unsigned short* g_font_palette_wiz_text_bold_68ee0c;
// GLOBAL: WIZ8 0x0068ee10
unsigned short* g_font_palette_smfnt_68ee10;
// GLOBAL: WIZ8 0x0068ee14
unsigned short* g_font_palette_wiz_text_68ee14;
// GLOBAL: WIZ8 0x0068ee18
unsigned short* g_font_palette_calligraphy_shadow_68ee18;
unsigned short* g_font_state_palettes_68ee1c[15];


/* The game-specific font catalog layered over SGP's source-owned font and
   video-object managers.  The individual globals are intentional: consumers
   select fonts by role, while this initializer preserves the retail load and
   derived-object order. */
// FUNCTION: WIZ8 0x004e27a0
unsigned char InitializeMenuFonts(void)
{
    char path[64];
    unsigned int index;

#define LOAD_FONT(destination, filename) \
    strcpy(path, filename);              \
    destination = LoadFontFile((UINT8*)path)

    LOAD_FONT(g_large_font_683674, "Data\\Fonts\\LargeFont.sti");
    LOAD_FONT(g_small_font_683678, "Data\\Fonts\\SmallFont.sti");
    LOAD_FONT(g_small_font_secondary_68366c, "Data\\Fonts\\SmallFont.sti");
    LOAD_FONT(g_wiz_text_font_683640, "Data\\Fonts\\Wiz_Text_Font.sti");
    LOAD_FONT(g_calligraphy_font_6835f8, "Data\\Fonts\\CalligraphyFont.sti");
    LOAD_FONT(g_calligraphy_shadow_font_6835f4,
              "Data\\Fonts\\CalligraphyFontFullShadow.sti");
    LOAD_FONT(g_smfnt_font_683694, "Data\\Fonts\\SmFnt.sti");
    LOAD_FONT(ghTinyMonoFont, "Data\\Fonts\\TinyMonoFont.sti");
    LOAD_FONT(g_button_font_683670, "Data\\Fonts\\ButtonFont.sti");
    LOAD_FONT(g_engraved_font_683600, "Data\\Fonts\\Engraved.sti");
    LOAD_FONT(g_embossed_font_683644, "Data\\Fonts\\Embossed.sti");
    LOAD_FONT(g_font_683660, "Data\\Fonts\\Wiz_Text_Font.sti");
    LOAD_FONT(g_wiz_text_bold_font_683664,
              "Data\\Fonts\\Wiz_Text_Font_Bold.sti");
    LOAD_FONT(g_wiz_text_mono_font_683630,
              "Data\\Fonts\\wiz_text_font_monopalette.sti");
    LOAD_FONT(g_options_title_font_68368c, "Data\\Fonts\\Opt_title_font.sti");
    LOAD_FONT(g_options_detail_font_683614, "Data\\Fonts\\Opt_detail_font.sti");
    LOAD_FONT(g_profession_font_683658, "Data\\Fonts\\Profession.sti");
    LOAD_FONT(g_font10arial_683668, "Data\\Fonts\\Font10Arial.sti");
    LOAD_FONT(g_dialog_font_683654, "Data\\Fonts\\dialog_font.sti");
    LOAD_FONT(g_monster_damage_font_683608,
              "Data\\Fonts\\monsterdamage_font.sti");
    LOAD_FONT(g_font12point1_683648, "Data\\Fonts\\FONT12POINT1.sti");

#undef LOAD_FONT

    g_large_font_object_683618 = GetFontObject(g_large_font_683674);
    g_small_font_object_683620 = GetFontObject(g_small_font_683678);
    g_small_font_secondary_object_683638 = GetFontObject(g_small_font_secondary_68366c);
    g_wiz_text_font_object_683604 = GetFontObject(g_wiz_text_font_683640);
    g_calligraphy_font_object_683628 = GetFontObject(g_calligraphy_font_6835f8);
    g_calligraphy_shadow_font_object_683610 = GetFontObject(g_calligraphy_shadow_font_6835f4);
    g_smfnt_font_object_683634 = GetFontObject(g_smfnt_font_683694);
    g_tiny_mono_font_object_68360c = GetFontObject(ghTinyMonoFont);
    g_button_font_object_6835fc = GetFontObject(g_button_font_683670);
    g_engraved_font_object_683624 = GetFontObject(g_engraved_font_683600);
    g_embossed_font_object_683688 = GetFontObject(g_embossed_font_683644);
    g_wiz_text_font_secondary_object_683680 = GetFontObject(g_font_683660);
    g_wiz_text_bold_font_object_68365c = GetFontObject(g_wiz_text_bold_font_683664);
    g_options_title_font_object_683650 = GetFontObject(g_options_title_font_68368c);
    g_options_detail_font_object_68361c = GetFontObject(g_options_detail_font_683614);
    g_profession_font_object_68362c = GetFontObject(g_profession_font_683658);
    g_font10arial_object_68367c = GetFontObject(g_font10arial_683668);
    g_dialog_font_object_683684 = GetFontObject(g_dialog_font_683654);
    g_monster_damage_font_object_68364c = GetFontObject(g_monster_damage_font_683608);
    g_font12point1_object_68363c = GetFontObject(g_font12point1_683648);

    CreateObjectPaletteTables(g_large_font_object_683618, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_small_font_object_683620, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_small_font_secondary_object_683638, HVOBJECT_GLOW_RED);
    CreateObjectPaletteTables(g_wiz_text_font_object_683604, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_calligraphy_font_object_683628, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_calligraphy_shadow_font_object_683610,
                              HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_smfnt_font_object_683634, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_tiny_mono_font_object_68360c, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_button_font_object_6835fc, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_engraved_font_object_683624, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_embossed_font_object_683688, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_wiz_text_font_secondary_object_683680,
                              HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_wiz_text_bold_font_object_68365c,
                              HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_options_title_font_object_683650,
                              HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_options_detail_font_object_68361c,
                              HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_profession_font_object_68362c, HVOBJECT_GLOW_BLUE);
    CreateObjectPaletteTables(g_dialog_font_object_683684, HVOBJECT_GLOW_GREEN);
    CreateObjectPaletteTables(g_font12point1_object_68363c, HVOBJECT_GLOW_GREEN);

    for (index = 0; index != 15; ++index) {
        EnsureCatalogFrameLoaded(0x1e5, index);
        g_font_state_palettes_68ee1c[index] =
            CopyCatalogImagePalette16BPP(0x1e5, index);
        if (!g_font_state_palettes_68ee1c[index]) {
            return 0;
        }
    }

    g_font_palette_smfnt_68ee10 = GetFontObjectPalette16BPP(g_smfnt_font_683694);
    g_font_palette_calligraphy_68edfc = GetFontObjectPalette16BPP(g_calligraphy_font_6835f8);
    g_font_palette_calligraphy_shadow_68ee18 =
        GetFontObjectPalette16BPP(g_calligraphy_shadow_font_6835f4);
    g_font_palette_wiz_text_68ee14 = GetFontObjectPalette16BPP(g_wiz_text_font_683640);
    g_font_palette_button_68ee04 = GetFontObjectPalette16BPP(g_button_font_683670);
    g_colour_68ee08 = GetFontObjectPalette16BPP(g_font_683660);
    g_font_palette_wiz_text_bold_68ee0c =
        GetFontObjectPalette16BPP(g_wiz_text_bold_font_683664);
    g_font_palette_options_detail_68ee00 =
        GetFontObjectPalette16BPP(g_options_detail_font_683614);
    ConfigureDialogFont(g_dialog_font_683654, 1, 0xff, 0);
    return 1;
}
