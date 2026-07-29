#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"
#include "Font.h"
#include "FileMan.h"
#include "vobject.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

W8ScreenStateStorage g_screen_state_0068ec78 = { -1 };
W8ScreenStateStorage g_dword_68ed10 = { -1 };
unsigned char g_flag_68edac;
void* g_stack_68eda8;
int g_dword_686a70 = -1;
unsigned char g_flag_68ed14;
int g_dword_68ed18;
unsigned char g_save_slot_68ed28[0x100];
unsigned char g_flag_6850d4;
unsigned short g_word_6850ed;
unsigned char g_block_68f2d8[0xc4e0];
unsigned char g_flag_65beaf;

extern unsigned short g_selected_item_0069c4b4;
unsigned char g_cd_marker_present_69b7d0;
void* g_small_subsystem_69c130;

struct W8StartupGridRow {
    int x1;
    int y1;
    int x2;
    int y2;
    int unknown_10;
    int unknown_14;
    int unknown_18;
};

W8StartupGridRow g_startup_grid_647da0[8];

extern unsigned char InitializeVector005EEA28(void);
extern void InitializeMenuVideoObjectCatalog(void);
extern void Function549090(int object, int frame);
extern unsigned short* Function5492E0(int object, int frame);
extern void Function5CF250(int font, unsigned char enabled,
                           unsigned char foreground, unsigned char background);

int g_calligraphy_shadow_font_6835f4;
int g_calligraphy_font_6835f8;
HVOBJECT g_button_font_object_6835fc;
int g_engraved_font_683600;
HVOBJECT g_wiz_text_font_object_683604;
int g_monster_damage_font_683608;
HVOBJECT g_tiny_mono_font_object_68360c;
HVOBJECT g_calligraphy_shadow_font_object_683610;
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
int g_wiz_text_font_683640;
int g_embossed_font_683644;
int g_font12point1_683648;
HVOBJECT g_monster_damage_font_object_68364c;
HVOBJECT g_options_title_font_object_683650;
int g_dialog_font_683654;
int g_profession_font_683658;
HVOBJECT g_wiz_text_bold_font_object_68365c;
int g_font_683660;
int g_wiz_text_bold_font_683664;
int g_font10arial_683668;
int g_small_font_secondary_68366c;
int g_button_font_683670;
int g_large_font_683674;
int g_small_font_683678;
HVOBJECT g_font10arial_object_68367c;
HVOBJECT g_wiz_text_font_secondary_object_683680;
HVOBJECT g_dialog_font_object_683684;
HVOBJECT g_embossed_font_object_683688;
int g_options_title_font_68368c;
int g_tiny_mono_font_683690;
int g_smfnt_font_683694;

unsigned short* g_font_palette_calligraphy_68edfc;
unsigned short* g_font_palette_options_detail_68ee00;
unsigned short* g_font_palette_button_68ee04;
unsigned short* g_colour_68ee08;
unsigned short* g_font_palette_wiz_text_bold_68ee0c;
unsigned short* g_font_palette_smfnt_68ee10;
unsigned short* g_font_palette_wiz_text_68ee14;
unsigned short* g_font_palette_calligraphy_shadow_68ee18;
unsigned short* g_font_state_palettes_68ee1c[15];

/* Until wiz8-9qy restores the retail cursor-image transfer, starting at -1
   prevents the synchronization call from pretending an unloaded cursor frame
   exists.  The cursor scene itself remains the real SurRender object. */
int g_cursor_state_00683fdb = -1;
int g_dword_683fdf;
int g_dword_683fe3;
unsigned char g_item_in_hand_shown_006874ca;
W8ItemInstance g_item_in_hand = { -1 };

// FUNCTION: WIZ8 0x005BC800
unsigned char InitializeSubsystemFlag(void)
{
    g_selected_item_0069c4b4 = 0;
    return 1;
}

static unsigned char InitializeCdMarker(void)
{
    g_cd_marker_present_69b7d0 = FileExistsNoDB("CD.ROM") != 0;
    return 1;
}

/* The review and inventory screens share these two regular grids.  The third
   irregular grid in the complete lifecycle initializer remains tracked by
   wiz8-a69; it is not touched by the main-menu screen. */
static unsigned char InitializeMenuRegionGrids(void)
{
    unsigned int index;
    for (index = 0; index != 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x30);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x27);
        SetRegionBounds(index + 0xea, x + 6, y + 5, x + 0x33, y + 0x29);
    }
    for (index = 0; index != 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39);
        SetRegionBounds(index + 0xf4, x + 0xb, y + 0xc0,
                        x + 0x39, y + 0xf6);
        SetRegionBounds(index + 0x108, x + 0x200, y + 0xc0,
                        x + 0x22e, y + 0xf6);
    }
    for (index = 0; index != 6; ++index) {
        unsigned short x = static_cast<unsigned short>((index % 3) * 0xd5);
        unsigned short y = static_cast<unsigned short>((index / 3) * 0x8c);
        SetRegionBounds(index + 0x119, x + 0x1d, y + 0xc3,
                        x + 0xb6, y + 0x128);
    }
    return 1;
}

// FUNCTION: WIZ8 0x0055F7B0
unsigned char InitializeStartupGrid(void)
{
    unsigned int index;
    for (index = 0; index != 8; ++index) {
        W8StartupGridRow& row = g_startup_grid_647da0[index];
        row.x1 = (index & 1) << 9;
        row.y1 = (index >> 1) * 0x55 + 0x12;
        row.x2 = row.x1 + 0x7f;
        row.y2 = row.y1 + 0x55;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005A9B00
unsigned char AllocateSmallStartupSubsystem(void)
{
    if (!g_small_subsystem_69c130) {
        g_small_subsystem_69c130 = malloc(0x38);
        if (!g_small_subsystem_69c130) {
            return 0;
        }
        memset(g_small_subsystem_69c130, 0, 0x38);
    }
    return 1;
}

unsigned char InitializeMenuStartupSubsystems(void)
{
    if (!InitializeSubsystemFlag()) return 0;
    if (!InitializeCdMarker()) return 0;
    if (!InitializeMenuRegionGrids()) return 0;
    if (!InitializeStartupGrid()) return 0;
    if (!InitializeVector005EEA28()) return 0;
    if (!AllocateSmallStartupSubsystem()) return 0;
    InitializeMenuVideoObjectCatalog();
    return 1;
}

/* The game-specific font catalog layered over SGP's source-owned font and
   video-object managers.  The individual globals are intentional: consumers
   select fonts by role, while this initializer preserves the retail load and
   derived-object order. */
// FUNCTION: WIZ8 0x004E27A0
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
    LOAD_FONT(g_tiny_mono_font_683690, "Data\\Fonts\\TinyMonoFont.sti");
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
    g_tiny_mono_font_object_68360c = GetFontObject(g_tiny_mono_font_683690);
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
        Function549090(0x1e5, index);
        g_font_state_palettes_68ee1c[index] = Function5492E0(0x1e5, index);
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
    Function5CF250(g_dialog_font_683654, 1, 0xff, 0);
    return 1;
}

}
