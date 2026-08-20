#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"
#include "Font.h"
#include "FileMan.h"
#include "vobject.h"

#include <stdlib.h>
#include <string.h>

unsigned char PleaseWaitScreenInitialize(void);

extern "C" {

// GLOBAL: WIZ8 0x0068EC78
W8ScreenStateRuntime g_screen_state_0068ec78 = { -1 };
// GLOBAL: WIZ8 0x0068ED10
W8ScreenStateRuntime g_dword_68ed10 = { -1 };
unsigned char g_flag_68edac;
void* g_stack_68eda8;
// GLOBAL: WIZ8 0x00686A70
int g_current_level = -1;
unsigned short g_word_6850ed;
W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];
unsigned char g_flag_65beaf;

extern unsigned short g_selected_item_0069c4b4;
// GLOBAL: WIZ8 0x0069C130
void* g_small_subsystem_69c130;
// GLOBAL: WIZ8 0x0069C0F4
W8CampScreenState0069C0F4* g_camp_screen_0069c0f4;

// GLOBAL: WIZ8 0x0069C4CC
int g_journal_font_69c4cc;
// GLOBAL: WIZ8 0x0069C4D0
unsigned short* g_journal_font_palette_69c4d0;
// GLOBAL: WIZ8 0x0069C4D8
unsigned short* g_journal_font_original_palette_69c4d8;

/* 0x0064CBF0: the twelve camp-screen regions the layout rules do not cover,
   given as explicit rectangles. The region initializer reads the first four
   fields; the trailing five are never touched there and stay positional. */
struct W8CampScreenRegion {
    int x;                 /* 0x00 */
    int y;                 /* 0x04 */
    int width;             /* 0x08 */
    int height;            /* 0x0c */
    int unknown_10;
    int unknown_14;
    int unknown_18;
    int unknown_1c;
    int unknown_20;
};

// GLOBAL: WIZ8 0x0064CBF0
const W8CampScreenRegion g_camp_screen_regions_64cbf0[12] = {
    { 0x0bc, 0x0b0, 0x2d, 0x39, 0x00, 0x01, 0x0ee, 0x0c1, 0 },
    { 0x1af, 0x0c6, 0x28, 0x28, 0x28, 0x08, 0x1aa, 0x0de, 1 },
    { 0x1af, 0x0f1, 0x28, 0x28, 0x24, 0x08, 0x1aa, 0x0f1, 1 },
    { 0x178, 0x0b0, 0x26, 0x49, 0x2c, 0x09, 0x173, 0x0b0, 1 },
    { 0x086, 0x0f1, 0x39, 0x3a, 0x04, 0x02, 0x0c4, 0x11c, 0 },
    { 0x183, 0x101, 0x23, 0x2b, 0x20, 0x07, 0x17e, 0x10b, 1 },
    { 0x09e, 0x134, 0x22, 0x65, 0x0c, 0x03, 0x0a0, 0x134, 0 },
    { 0x190, 0x134, 0x22, 0x48, 0x18, 0x06, 0x1b0, 0x134, 1 },
    { 0x079, 0x134, 0x22, 0x65, 0x08, 0x03, 0x07b, 0x154, 0 },
    { 0x1b5, 0x134, 0x22, 0x48, 0x1c, 0x06, 0x1d1, 0x154, 1 },
    { 0x0cb, 0x167, 0x22, 0x4d, 0x10, 0x04, 0x0f2, 0x18f, 0 },
    { 0x16a, 0x17a, 0x1a, 0x3a, 0x14, 0x05, 0x165, 0x19f, 1 }
};

extern void InitializeCampScreenRegions(void);
extern void Function5B7230(void);

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

/* Zero is the retail BSS state. The first screen synchronization replaces it
   with the default cursor and then records the normal -1 state. */
int g_cursor_state_00683fdb;
int g_dword_683fdf;
int g_dword_683fe3;

// FUNCTION: WIZ8 0x005bc800
unsigned char InitializeSubsystemFlag(void)
{
    g_selected_item_0069c4b4 = 0;
    return 1;
}

/* Lifecycle record 11's initializer and finalizer. The initializer loads the
   journal font, saves the palette the font arrived with, and takes a second
   palette from frame 0 of video object 0x1B9; the finalizer puts the original
   palette back and releases the one it took. The two together are the second
   proof that a record's fifth slot is its finalizer rather than a second
   initializer - record 10's allocate/free pair is the first. */
// FUNCTION: WIZ8 0x005bddd0
unsigned char InitializeJournalFont(void)
{
    g_journal_font_69c4cc = LoadFontFile((UINT8*)"Data\\Journal\\journal_font.sti");
    g_journal_font_original_palette_69c4d8 = GetFontObjectPalette16BPP(g_journal_font_69c4cc);
    g_journal_font_palette_69c4d0 = Function5492E0(0x1b9, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005bde10
unsigned char FinalizeJournalFont(void)
{
    SetFontObjectPalette16BPP(g_journal_font_69c4cc, g_journal_font_original_palette_69c4d8);
    free(g_journal_font_palette_69c4d0);
    return 1;
}

/* Lifecycle record 6's initializer - the camp record, which is what
   W8_SCREEN_CAMP selects. It drops the camp screen's state pointer rather than
   releasing it; the block is owned by the enter/leave pair. */
// FUNCTION: WIZ8 0x005a3500
unsigned char InitializeCampScreen(void)
{
    g_camp_screen_0069c0f4 = 0;
    InitializeCampScreenRegions();
    Function5B7230();
    return 1;
}

/* The four region blocks the camp screen lays out by rule, and the twelve it
   lays out from the explicit table below. Only the leading four fields of each
   record are read here, so the remaining five stay positional. */
// FUNCTION: WIZ8 0x005a4090
void InitializeCampScreenRegions(void)
{
    unsigned int index;
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x30 + 6);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x27 + 5);
        SetRegionBounds(index + 0xea, x, y, x + 0x2d, y + 0x24);
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0xb);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0xf4, x, y, x + 0x2e, y + 0x36);
    }
    for (index = 0; index < 12; ++index) {
        const W8CampScreenRegion& region = g_camp_screen_regions_64cbf0[index];
        SetRegionBounds(index + 0xfc,
                        static_cast<unsigned short>(region.x),
                        static_cast<unsigned short>(region.y),
                        static_cast<unsigned short>(region.x + region.width),
                        static_cast<unsigned short>(region.y + region.height));
    }
    for (index = 0; index < 8; ++index) {
        unsigned short x = static_cast<unsigned short>((index & 1) * 0x31 + 0x200);
        unsigned short y = static_cast<unsigned short>((index >> 1) * 0x39 + 0xc0);
        SetRegionBounds(index + 0x108, x, y, x + 0x2e, y + 0x36);
    }
}

/* The camp screen's remaining six regions, three across and two down. The
   initializer calls this immediately after the block above; nothing else
   reaches it, and nothing here names what the six cells hold. */
// FUNCTION: WIZ8 0x005b7230
void Function5B7230(void)
{
    unsigned int index;
    for (index = 0; index < 6; ++index) {
        unsigned short x = static_cast<unsigned short>((index % 3) * 0xd5 + 0x1d);
        unsigned short y = static_cast<unsigned short>((index / 3) * 0x8c + 0xc3);
        SetRegionBounds(index + 0x119, x, y, x + 0x99, y + 0x65);
    }
}

// FUNCTION: WIZ8 0x0055f7b0
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

// FUNCTION: WIZ8 0x005a9b00
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

/* The finalizer half of the pair above, and the fifth dword of lifecycle record
   10 - which is what establishes that the record's last slot is the finalizer
   rather than a second initializer: record 10's first slot allocates this exact
   block and its fifth releases it. The release is unguarded and leaves the
   pointer set, so it relies on running once at shutdown. */
// FUNCTION: WIZ8 0x005a9b30
unsigned char FreeSmallStartupSubsystem(void)
{
    ::operator delete(g_small_subsystem_69c130);
    return 1;
}

/* The menu bring-up subset of the thirteen lifecycle records, called in place of
   the walk over the real table. The camp record's region pair is called directly
   rather than through its own initializer: the review and inventory screens the
   menu reaches read those regions, and the initializer around them also drops the
   camp state pointer, which bring-up has no reason to do. Records whose remaining
   slots reach unrecovered bodies stay out until the table drives itself. */
unsigned char InitializeMenuStartupSubsystems(void)
{
    if (!InitializeSubsystemFlag()) return 0;
    if (!PleaseWaitScreenInitialize()) return 0;
    InitializeCampScreenRegions();
    Function5B7230();
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
