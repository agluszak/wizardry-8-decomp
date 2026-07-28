#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

W8ScreenStateStorage g_dword_68ec78 = { -1 };
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

unsigned int g_subsystem_flag_69c4b4;
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

extern unsigned char FileExistsNoDB(const char* path);
extern unsigned char InitializeVector005EEA28(void);
extern void InitializeMenuVideoObjectCatalog(void);
extern int LoadFontFile(unsigned char* path);
extern void* GetFontObject(int font);
extern unsigned short* GetFontObjectPalette16BPP(int font);
struct FontTranslationTable;
extern FontTranslationTable* CreateEnglishTransTable(void);
extern unsigned char InitializeFontManager(
    unsigned short pixel_depth, FontTranslationTable* translation);
extern unsigned char SetFontDestBuffer(
    unsigned int destination, int left, int top, int right, int bottom,
    unsigned char wrap);

int g_menu_font_handles_6835f4[21];
void* g_menu_font_objects_6835fc[21];
int g_font_683660;
unsigned short* g_colour_68ee3c;
unsigned short* g_colour_68ee08;

/* Until wiz8-9qy restores the retail cursor-image transfer, starting at -1
   prevents the synchronization call from pretending an unloaded cursor frame
   exists.  The cursor scene itself remains the real SurRender object. */
int g_cursor_state_683fdb = -1;
int g_dword_683fdf;
int g_dword_683fe3;
int g_screen_68ec78 = -1;
unsigned char g_item_in_hand_shown_6874ca;
W8ItemInstance g_item_in_hand = { -1 };

// FUNCTION: WIZ8 0x005BC800
unsigned char InitializeSubsystemFlag(void)
{
    g_subsystem_flag_69c4b4 = 0;
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

/* Kept as an explicit boundary while wiz8-xb9 links the source-backed SGP
   font/video-object units.  It lets the data initializer expose independent
   startup dependencies without claiming that fonts are ready. */
unsigned char InitializeMenuFonts(void)
{
    static const char* const paths[21] = {
        "Data\\Fonts\\LargeFont.sti",
        "Data\\Fonts\\SmallFont.sti",
        "Data\\Fonts\\SmallFont.sti",
        "Data\\Fonts\\Wiz_Text_Font.sti",
        "Data\\Fonts\\CalligraphyFont.sti",
        "Data\\Fonts\\CalligraphyFontFullShadow.sti",
        "Data\\Fonts\\SmFnt.sti",
        "Data\\Fonts\\TinyMonoFont.sti",
        "Data\\Fonts\\ButtonFont.sti",
        "Data\\Fonts\\Engraved.sti",
        "Data\\Fonts\\Embossed.sti",
        "Data\\Fonts\\Wiz_Text_Font.sti",
        "Data\\Fonts\\Wiz_Text_Font_Bold.sti",
        "Data\\Fonts\\wiz_text_font_monopalette.sti",
        "Data\\Fonts\\Opt_title_font.sti",
        "Data\\Fonts\\Opt_detail_font.sti",
        "Data\\Fonts\\Profession.sti",
        "Data\\Fonts\\Font10Arial.sti",
        "Data\\Fonts\\dialog_font.sti",
        "Data\\Fonts\\monsterdamage_font.sti",
        "Data\\Fonts\\FONT12POINT1.sti"
    };
    unsigned int index;
    FontTranslationTable* translation;

    translation = CreateEnglishTransTable();
    if (!translation || !InitializeFontManager(16, translation)) {
        free(translation);
        return 0;
    }
    /* InitializeFontManager retains the symbol array and copies the small
       table header, exactly as the released SGP implementation specifies. */
    free(translation);
    SetFontDestBuffer(0xfffffff2u, 0, 0, 640, 480, 0);

    for (index = 0; index != 21; ++index) {
        int font = LoadFontFile(
            reinterpret_cast<unsigned char*>(const_cast<char*>(paths[index])));
        if (font < 0) {
            return 0;
        }
        g_menu_font_handles_6835f4[index] = font;
        g_menu_font_objects_6835fc[index] = GetFontObject(font);
    }
    g_font_683660 = g_menu_font_handles_6835f4[11];
    g_colour_68ee3c = GetFontObjectPalette16BPP(
        g_menu_font_handles_6835f4[11]);
    g_colour_68ee08 = g_colour_68ee3c;
    return 1;
}

}
