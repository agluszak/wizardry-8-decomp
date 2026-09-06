#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/xstatus.h"

#include "wiz8/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/geometry.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"

void __fastcall Function5A6E20(void* options);
void NoOp(void);
void ShutdownDisplayList(void);
void ResetRegions(void);
void Function425570(int value);
int Function40B290(void);
void SetViewport(int left, int top, int right, int bottom);
void UpdateHeldItemCursor(void);
void Function406DC0(int font, unsigned short* palette);

extern int g_dword_647bc0;
extern int g_font_683660;
extern int g_options_detail_font_683614;
extern unsigned short* g_colour_68ee08;
extern void* g_small_subsystem_69c130;
extern unsigned char g_flag_689b32;
extern unsigned char g_flag_6f04e8;
extern unsigned char g_flag_6f04ed;

void Function40B510(unsigned short event, unsigned short x, unsigned short y,
                    char right_button, char left_button);
unsigned int Function4F1360(int x, int y);
void RequestScreenTransition(void);
void Function426790(void);
void Function427230(int enabled);
void RepositionAmbientSounds0047A600(W8World* world);
void UpdateAmbientSounds0047A3E0(W8World* world);

/* Shared zero-initialized wide string: binary-wide DATA references read it as
   empty text and as a swprintf format argument. No recovered writer owns it
   yet; this definition only anchors the address until that function lands. */
// GLOBAL: WIZ8 0x00689b34
wchar_t g_wchar_00689b34;

/* Layout flag handed to every row-registered child text control.  No other
   recovered site reads it yet; ownership stays with this constant until a
   writer shows a wider family. */
// GLOBAL: WIZ8 0x005ed590
extern const unsigned int g_W8TextControlLayoutMask005ED590 = 0x40;

// GLOBAL: WIZ8 0x0069c1c4
int g_flag_69c1c4;
// GLOBAL: WIZ8 0x0069c1c8
unsigned char g_flag_69c1c8;
// GLOBAL: WIZ8 0x0069c138
unsigned char g_options_values_0069c138;
// GLOBAL: WIZ8 0x0069c254
W8OptionsScreen* g_options_screen_0069c254;

/* Menu rows come from the shared source table: the button's own geometry and
   text parameters sit in row[9..17], the item id in row[0], and the two
   optional child controls in row[1..4] and row[5..8].  Each child joins the
   button's owner panel, registers the button's listener subobject, and picks
   up the shared layout flag. */
// FUNCTION: WIZ8 0x005a7370
W8OptionsMenuButton005EED3C::W8OptionsMenuButton005EED3C(Controls* owner, const int* row)
    : W8TextControl005ED604(owner, 0xffffffff, row[9], row[10], row[11], row[12],
                            0xef, 0, row[13], row[14], row[15], row[16], row[17])
{
    m_item_id_0bc = row[0];

    if (row[1] != -1) {
        W8TextControl005ED604* control = new W8TextControl005ED604(
            m_pPanel, 0xffffffff, row[1], row[2], row[3], row[4],
            -1, -1, -1, -1, -1, -1, -1);
        control->m_listener = this;
        control->AddLayoutFlags(g_W8TextControlLayoutMask005ED590);
    }

    if (row[5] != -1) {
        W8TextControl005ED604* control = new W8TextControl005ED604(
            m_pPanel, 0xffffffff, row[5], row[6], row[7], row[8],
            -1, -1, -1, -1, -1, -1, -1);
        control->m_listener = this;
        control->AddLayoutFlags(g_W8TextControlLayoutMask005ED590);
    }
}

/* The shared panel base sizes itself from the chrome sprite of its render
   target and registers one region-set row per concrete panel index. */
// FUNCTION: WIZ8 0x005a81e0
W8OptionsPanel::W8OptionsPanel(int region_index)
    : Controls(0x104, 0, 0, 0, 0xee, 0, 1),
      m_current_04c(0), m_content_top_050(0x18)
{
    AcquireRegionSet((unsigned int*)g_small_subsystem_69c130 + region_index);

    short width;
    short height;
    Function549660(0xf2, 0, 0, &width, &height);
    right = origin_x + (unsigned short)width;
    bottom = origin_y + (unsigned short)height;
}

/* The menu set builds itself from the shared options font: its panel bounds
   follow the rendered chrome sprite, then the two page arrows are constructed
   and registered with the listener subobject, and the page text starts as the
   shared empty wide-string global until UpdateMenuSet fills it. */
// FUNCTION: WIZ8 0x005a8c90
W8OptionsMenuSet005EEFEC::W8OptionsMenuSet005EEFEC(unsigned int* shared_region_set)
    : Controls(0x11b, 0x1a8, 0, 0, 0xf3, 0, 0), m_pMenuSet(0)
{
    AcquireRegionSet(shared_region_set);

    short width;
    short height;
    Function549660(0xf3, 0, 0, &width, &height);
    right = origin_x + (unsigned short)width;
    bottom = origin_y + (unsigned short)height;

    m_previous_058 = new W8TextControl005ED604(
        this, 0xffffffff, 3, 3, 0, 0, 0xf4, 0, 0, 2, 1, -1, 3);
    m_previous_058->m_listener = this;

    m_next_054 = new W8TextControl005ED604(
        this, 0xffffffff, 0x11f, 3, 0, 0, 0xf4, 0, 4, 6, 5, -1, 7);
    m_next_054->m_listener = this;

    m_page_text_05c = new W8TextBuffer005ED5B8(
        (W8ControlsRect*)&origin_x, &g_wchar_00689b34, g_options_detail_font_683614,
        g_W8TextBufferLayoutMask005ED54C | g_W8TextBufferLayoutMask005ED554, 4);
}

/* The menu-set table has its own deleting destructor; the normal destructor
   clears the inherited Controls children before releasing its page text. */
// FUNCTION: WIZ8 0x005a8e60
W8OptionsMenuSet005EEFEC::~W8OptionsMenuSet005EEFEC()
{
    DestroyAllControls();
    delete m_page_text_05c;
}

// FUNCTION: WIZ8 0x005a8440
void W8OptionsPanel::SetActive(unsigned char active)
{
    EnableRegionSet(active);
    SetEnabled(active);
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005a84c0
void W8OptionsPanel::SetCurrent(int current)
{
    m_current_04c = current;
    Invalidate(0);
}

// FUNCTION: WIZ8 0x005a8b70
void W8OptionsPanelSet005EF01C::Advance()
{
    int current;

    if (m_mode_000 == 0) {
        current = m_current_00c;
        if (current < m_panels_010.count - 1) {
            (*m_panels_010.GetAt(current))->SetActive(0);
            current = m_current_00c + 1;
            m_current_00c = current;
            (*m_panels_010.GetAt(current))->SetActive(1);
        }
    }
    else if (m_panels_010.data[0]->m_current_04c < m_mode_000 - 1) {
        m_panels_010.data[0]->SetCurrent(m_panels_010.data[0]->m_current_04c + 1);
    }
}

// FUNCTION: WIZ8 0x005a8c00
void W8OptionsPanelSet005EF01C::Retreat()
{
    int current;

    if (m_mode_000 == 0) {
        current = m_current_00c;
        if (current > 0) {
            (*m_panels_010.GetAt(current))->SetActive(0);
            current = m_current_00c - 1;
            m_current_00c = current;
            (*m_panels_010.GetAt(current))->SetActive(1);
        }
    }
    else if (m_panels_010.data[0]->m_current_04c > 0) {
        m_panels_010.data[0]->SetCurrent(m_panels_010.data[0]->m_current_04c - 1);
    }
}

// FUNCTION: WIZ8 0x005a8ed0
void W8OptionsMenuSet005EEFEC::UpdateMenuSet()
{
    W8ControlsRect bounds;
    W8OptionsPanelSet005EF01C* panel_set;
    int current;
    int count;

    bounds.left = origin_y;
    bounds.top = right;
    bounds.right = bottom;
    if (m_pMenuSet == 0) {
        srAssertFail("m_pMenuSet",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\OptionsScreen.cpp",
                     0x5c9, 0);
    }
    panel_set = m_pMenuSet;
    if (panel_set->m_mode_000 == 0) {
        current = panel_set->m_current_00c;
        count = panel_set->m_panels_010.count;
    }
    else {
        current = panel_set->m_panels_010.data[0]->m_current_04c;
        count = panel_set->m_mode_000;
    }
    bounds.top = panel_set->unknown_008 != 0 ? 0x180 : 0x1a8;
    bounds.bottom = origin_x + bounds.top;
    SetBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    ++bounds.top;
    m_page_text_05c->SetLayoutBounds(&bounds, 1, 1);
    SetEnabled(panel_set->m_show_page_text_009 == 0 &&
                       (panel_set->unknown_008 != 0 || count > 1));
    if (panel_set->m_show_page_text_009 != 0) {
        m_next_054->SetVisible(current < count - 1);
        m_previous_058->SetVisible(current > 0);
        wchar_t text[0x10];

        swprintf(text, L"%d / %d", current + 1, count);
        m_page_text_05c->SetText(text, g_font_683660);
    }
    Invalidate(0);
}

/* The chrome listener is attached at the secondary base subobject.  The
   preceding control walks back and all other primary activations walk forward;
   both paths then recompute bounds, navigation availability, and page text. */
// FUNCTION: WIZ8 0x005a9050
void W8OptionsMenuSet005EEFEC::OnPrimary(W8TextControl005ED604* control)
{
    if (control == m_previous_058) {
        m_pMenuSet->Retreat();
    }
    else {
        m_pMenuSet->Advance();
    }
    UpdateMenuSet();
}

/* The dialog callback's teardown decision is part of the Options screen's
   state transition: only an accepted close outside combat saves an active
   party, then the common audio-state update runs for every accepted close. */
// FUNCTION: WIZ8 0x005a9ac0
void W8OptionsScreen::OnDialogClosed(unsigned char reason, int)
{
    if (reason != 0) {
        if (g_status_685170.game_started != 0 &&
            g_in_combat_00683f94 == 0 && AnyCharacterActive()) {
            AutoSaveIfAllowed(1);
        }
        Function591780();
    }
}

/* The selection-listener slot receives the originating control and its selected
   row.  The retail thunk deliberately ignores the control and chooses the
   panel without emitting another notification. */
// FUNCTION: WIZ8 0x005a98a0
void W8OptionsScreen::vslot00(W8Control005ED654*, int selected)
{
    SelectPanel(selected, 0);
}

/* State 10 first transfers its option values back to the shared settings
   block, then releases every controller-owned control before returning the
   display and region systems to their common screen boundary. */
// FUNCTION: WIZ8 0x005a9c70
unsigned char OptionsScreenLeave005A9C70(int)
{
    g_flag_69c1c4 = 1;
    Function5A6E20(&g_options_values_0069c138);
    W8OptionsScreen* screen = g_options_screen_0069c254;
    if (screen != 0) {
        screen->~W8OptionsScreen();
        ::operator delete(screen);
    }
    g_options_screen_0069c254 = 0;
    NoOp();
    ShutdownDisplayList();
    ResetRegions();
    return 1;
}

/* State 10 creates the controller after resetting the shared 2D screen
   systems.  The selected panel is determined by the transition mode, with the
   in-game branch preserving the combat and save-state overrides. */
// FUNCTION: WIZ8 0x005a9b50
unsigned char OptionsScreenEnter005A9B50()
{
    int selected;

    SetViewport(0, 0, 0x280, 0x1e0);
    Function425570(0);
    Function40B290();
    ResetRegions();
    UpdateHeldItemCursor();
    Function406DC0(g_font_683660, g_colour_68ee08);
    g_flag_69c1c4 = 0;
    Function5A6E20(&g_options_values_0069c138);
    g_options_screen_0069c254 = new W8OptionsScreen();
    g_options_screen_0069c254->CreateControls();

    if (g_screen_state_0068ec78.mode == 1) {
        selected = 4;
    }
    else if (g_screen_state_0068ec78.mode == 2) {
        selected = 5;
    }
    else if (g_screen_state_0068ec78.mode == 3) {
        if (g_dword_647bc0 == 4) {
            selected = 5;
        }
        else {
            selected = g_dword_647bc0;
            if (g_dword_647bc0 == 5) {
                if (g_in_combat_00683f94 != 0) {
                    selected = 4;
                }
                if (g_save_flag_00687599 != 0) {
                    selected = 0;
                }
            }
        }
    }
    else {
        selected = 0;
    }
    g_options_screen_0069c254->SelectPanel(selected, 1);
    g_flag_69c1c8 = 1;
    return 1;
}

/* The state-10 frame owns modal retirement, input refusal order and redraw
   sequencing.  Options-specific input gets first refusal, followed by shared
   region dispatch; only an unhandled Escape requests the screen transition. */
// FUNCTION: WIZ8 0x005a9cc0
void OptionsScreenFrame005A9CC0()
{
    W8ScreenPoint point;
    W8ScreenPoint current;
    InputAtom input;

    if (g_flag_689b32) {
        Function591780();
    }
    RepositionAmbientSounds0047A600(g_world);
    UpdateAmbientSounds0047A3E0(g_world);
    Function48F9E0();
    GetScreenPoint004284F0(&point);

    W8OptionsScreen* screen = g_options_screen_0069c254;
    if (screen->m_panel_05c != 0) {
        GetScreenPoint004284F0(&current);
        Function40B510(MOUSE_POS, static_cast<unsigned short>(current.x),
                       static_cast<unsigned short>(current.y),
                       g_flag_6f04ed, g_flag_6f04e8);
        screen = g_options_screen_0069c254;
    }

    W8ModalDialogBase** active_modal = &screen->m_active_modal;
    if (*active_modal != 0) {
        if ((*active_modal)->ProcessInput() == 0) {
            if (screen->m_modal_closing_01d == 0) {
                delete *active_modal;
                *active_modal = 0;
            }
            screen->m_input_pending_01c = 1;
            screen->m_controls_024->Invalidate(0);
            if (screen->m_selected_panel_020 != -1) {
                W8OptionsPanelSet005EF01C* panel_set =
                    screen->m_panel_038[screen->m_selected_panel_020];
                if (panel_set->unknown_00a != 0) {
                    (*panel_set->m_panels_010.GetAt(panel_set->m_current_00c))
                        ->Invalidate(0);
                }
                screen->m_menu_set_028->Invalidate(0);
            }
        }
        screen->m_modal_closing_01d = 0;
    }

    Function4F1360(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!screen->Function5A9720(&input) &&
            !DispatchScreenInput004F1910(&input) &&
            input.usEvent == KEY_DOWN && input.usParam == VK_ESCAPE) {
            RequestScreenTransition();
        }
    }
    screen->Function5A95F0();
    if (g_flag_69c1c8 != 0) {
        Function427230(0);
        Function426790();
        Function427230(1);
        g_flag_69c1c8 = 0;
    }
    Function426790();
}
