#include "wiz8/local_screens/OptionsScreen.h"

#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/music_playlist.h"
#include "wiz8/screen_state.h"

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
extern unsigned short* g_colour_68ee08;

// GLOBAL: WIZ8 0x0069c1c4
int g_flag_69c1c4;
// GLOBAL: WIZ8 0x0069c1c8
unsigned char g_flag_69c1c8;
// GLOBAL: WIZ8 0x0069c138
unsigned char g_options_values_0069c138;
// GLOBAL: WIZ8 0x0069c254
W8OptionsScreen* g_options_screen_0069c254;

/* The dialog callback's teardown decision is part of the Options screen's
   state transition: only an accepted close outside combat saves an active
   party, then the common audio-state update runs for every accepted close. */
// FUNCTION: WIZ8 0x005a9ac0
void W8OptionsScreen::OnDialogClosed(unsigned char reason, int)
{
    if (reason != 0) {
        if (g_status_685170.game_started_000c != 0 &&
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
    if (g_options_screen_0069c254 != 0) {
        W8OptionsScreen* screen = g_options_screen_0069c254;

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
