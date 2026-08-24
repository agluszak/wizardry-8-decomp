#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/game_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/dialog_base.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/music_playlist.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/utility.h"

#include "Font.h"
#include "FileMan.h"
#include "soundman.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * Local Screens\PleaseWaitScreen.cpp.
 *
 * The unit is named by the five assertions its frame handler at 0x00590FA0
 * embeds, at lines 279, 293, 296, 320 and 332. The other three bodies here are
 * the same lifecycle record's remaining slots - record 4 of the table at
 * 0x00647BC8 - and they share this screen's private state with the named one:
 * the descriptor at 0x0069B7C8 is built by the entry handler, read by the frame
 * handler and released by the leave, and the font at 0x0069B7C0 is loaded by the
 * initializer and installed by the frame handler. The record and the shared
 * state are what place them; no assertion names them individually.
 */

extern "C" {

/* The screen's descriptor. The entry handler mallocs it, clears it and fills the
   tail from the screen-state record it was entered with; the frame handler reads
   the tail and writes the caption; the leave releases it. */
struct W8LevelLoadDescriptor {
    wchar_t caption[0x78];         /* 0x000, written by wcscpy and swprintf */
    int mode;                      /* 0x0f0, the screen state's own mode */
    int parameter;                 /* 0x0f4 */
    int parameter_2;               /* 0x0f8 */
    unsigned char waiting;         /* 0x0fc, gates the polling path */
    char name[0x3f];               /* 0x0fd, bounded only by the next field */
    void* save_payload;            /* 0x13c */
    unsigned long entered_tick;    /* 0x140 */
    int caption_y;                 /* 0x144 */
};

// GLOBAL: WIZ8 0x0069B7C0
int g_level_load_font_69b7c0;
// GLOBAL: WIZ8 0x0069B7C4
int g_value_69b7c4;
// GLOBAL: WIZ8 0x0069B7C8
W8LevelLoadDescriptor* g_load_descriptor_69b7c8;
// GLOBAL: WIZ8 0x0069B7CC
W8ModalDialogBase* g_swap_disc_dialog_69b7cc;
// GLOBAL: WIZ8 0x0069B7D0
unsigned char g_cd_marker_present_69b7d0;


}

void ResetRegions(void);
unsigned char SetFlag603C60(void);
unsigned char ClearFlag603C60(void);
/* Engine Code\Levels.cpp owns this with C++ linkage. */

extern "C" {

extern void NoOp(void);
extern void ShutdownDisplayList(void);
extern int Function40B290(void);
extern void ConfigurePresentation00413FD0(int a, int b, int c, int d, int e);
extern unsigned char Function42B6F0(int level);
extern int Function42B720(int level);
extern void Function425570(int value);
/* 0x00412A10; the reviewed identity Ghidra carries. Nothing defines it yet. */
extern void RefreshSlfArchives(void);
extern int Function509750(void);
extern void Function58FD30(void);
extern void Function407650(int x, int y, const char* format, const wchar_t* text);
extern unsigned char SetValue5FF5F0(int font);
extern void Function422F10(void);
extern void Function426790(void);
extern void Function512C40(void);
extern void Function5092F0(int* level, int* entrance);
extern void Function5063E0(void);
extern void Function58AC00(int channel, const wchar_t* text, int a, int b, int c);
extern unsigned char Function42AF60(int level, int entrance);
extern void Function5159E0(int value);
extern int Function55EC10(void);
extern unsigned int LoadGame(const char* slot_name);
extern unsigned char SaveGame(const char* slot_name, void* screenshot);
extern void RequestScreenTransition(void);
extern void SetPendingScreenState(int value);
extern void SetValue64D8AC(unsigned long value);
extern unsigned char g_flag_689b2c;

/* 0x0064BF8C: one video-object id per level, the backdrop the Please Wait
   screen shows while that level loads. 0x00605820 indexes the level's name in
   the string list. Both are bounded by 0x2F, with 0xE4 as the backdrop the
   frame handler falls back to. */
extern int g_level_backdrops_64bf8c[];
extern unsigned short g_level_name_indices_605820[];

}

/* The path the five assertions carry. */
#define PLEASE_WAIT_SCREEN_CPP "C:\\Projects\\Wizardry 8\\Local Screens\\PleaseWaitScreen.cpp"

/* Lifecycle record 4's initializer. The marker write is guarded, so a missing
   CD.ROM leaves the flag at its BSS zero rather than storing a comparison
   result. */
// FUNCTION: WIZ8 0x00590db0
unsigned char PleaseWaitScreenInitialize(void)
{
    if (FileExistsNoDB("CD.ROM")) {
        g_cd_marker_present_69b7d0 = 1;
    }
    g_level_load_font_69b7c0 = LoadFontFile((UINT8*)"Data\\Level Load\\levelload_font.sti");
    return 1;
}

/* The entry handler. It builds the descriptor once and fills the tail from the
   screen-state record it was entered with, which is what establishes that
   record's leading fields: mode at 0x04, then three parameters, then a name at
   0x18. Mode 0 is the new game - it clears the fact state and deletes the running
   save; modes 1 and 2 carry a name; mode 3 carries two parameters and no name.

   The descriptor is reached through the global at every store rather than through
   a local, which is the original's own shape. */
// FUNCTION: WIZ8 0x00590de0
unsigned char PleaseWaitScreenEnter(void)
{
    if (!g_load_descriptor_69b7c8) {
        g_load_descriptor_69b7c8 =
            (W8LevelLoadDescriptor*)malloc(sizeof(W8LevelLoadDescriptor));
        if (!g_load_descriptor_69b7c8) {
            return 0;
        }
        memset(g_load_descriptor_69b7c8, 0, sizeof(W8LevelLoadDescriptor));
        g_load_descriptor_69b7c8->mode = g_screen_state_0068ec78.mode;
        switch (g_load_descriptor_69b7c8->mode) {
        case 0:
            InitializeFactState();
            g_load_descriptor_69b7c8->parameter = Function509750();
            Function58FD30();
            DeleteFileA("Saves\\CurrentGame.SAV");
            break;
        case 1:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.parameter;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.name);
            break;
        case 2:
            g_load_descriptor_69b7c8->parameter = g_current_level;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.name);
            g_load_descriptor_69b7c8->save_payload = g_screen_state_0068ec78.parameter_3;
            break;
        case 3:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.parameter;
            g_load_descriptor_69b7c8->parameter_2 = g_screen_state_0068ec78.parameter_2;
            break;
        }
    }
    Function40B290();
    ResetRegions();
    ConfigurePresentation00413FD0(0x500, 0, 0, 0x280, 0x1e0);
    SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
    Function425570(0);
    ClearFlag603C60();
    g_load_descriptor_69b7c8->caption_y = 0;
    g_load_descriptor_69b7c8->entered_tick = GetTickCount();
    g_value_69b7c4 = 0;
    return 1;
}

/* The gate the frame handler runs before every load. When the level archive is
   not reachable and the CD marker says it should be, it parks the screen: it
   records what was being waited for, raises the swap-disc dialog once, and
   reports 0 so the caller does not proceed. A reachable archive, or a disc the
   check clears, reports 1.

   The dialog is built once and kept; the three setters go through the vtable
   here because the receiver is a base pointer rather than a constructor's own
   object, which is what separates these call sites from the derived
   constructors that reach the same three addresses directly. */
// FUNCTION: WIZ8 0x00591620
unsigned char PleaseWaitScreenEnsureLevelArchive(int level)
{
    if (!FileExistsNoDB("Levels\\Levels.slf") && g_cd_marker_present_69b7d0) {
        if (Function42B6F0(level)) {
            g_load_descriptor_69b7c8->waiting = 1;
            g_load_descriptor_69b7c8->parameter = level;
            g_load_descriptor_69b7c8->entered_tick = GetTickCount();
            if (!g_swap_disc_dialog_69b7cc) {
                g_swap_disc_dialog_69b7cc = new W8ModalDialogBase;
                g_swap_disc_dialog_69b7cc->SetBackground(
                    "Data\\Dialogs\\DialogBackground.sti", 0);
                g_swap_disc_dialog_69b7cc->SetExtent(0xf0, 0xbe);
                g_swap_disc_dialog_69b7cc->SetOrigin(0xa0, 100);
            }
            wchar_t* message = FormatWideString(L"%s%d", gppStringList[0x1bb8 / 4],
                                                Function42B720(level));
            g_swap_disc_dialog_69b7cc->SetMessage(message, 1, 0x32, 1, 1, 1, 0, 0, 0);
            SetFlag603C60();
            return 0;
        }
        RefreshSlfArchives();
    }
    return 1;
}

/* The screen's one drawn frame: the level's own backdrop, the progress bar's
   two pieces and the caption. The frame handler emits it from two places - the
   parked path and the ordinary path - and the retail body carries both copies,
   which is why it is written out at each site rather than factored here. */
#define PLEASE_WAIT_SCREEN_DRAW()                                             \
    do {                                                                      \
        int backdrop = (unsigned int)g_load_descriptor_69b7c8->parameter < 0x2f \
            ? g_level_backdrops_64bf8c[g_load_descriptor_69b7c8->parameter]   \
            : 0xe4;                                                           \
        Function548F90(-14, backdrop, 0, 0, 0, 0, 2, 0);                      \
        Function548F90(-14, 0x1de, 0, 0, 0, 0x1be, 2, 0);                     \
        SetValue5FF5F0(g_level_load_font_69b7c0);                             \
        Function407650(0x6a, 0x1c7, "%",                                      \
                       (const wchar_t*)g_load_descriptor_69b7c8);             \
        Function548F90(-14, 0x1dd, 0, g_load_descriptor_69b7c8->caption_y,    \
                       0, 0x185, 2, 0);                                       \
        Function422F10();                                                     \
    } while (0)

/* The frame handler, and the body whose five assertions name this unit.
   Two passes in one: while the screen is parked on a missing disc it polls the
   swap-disc dialog and does nothing else, and otherwise it draws one frame,
   performs the load the record was entered for, and queues the transition out.

   Both passes fall through to the same trailing 0x00426790, which is why the
   parked path returns through it rather than around it. */
// FUNCTION: WIZ8 0x00590fa0
void PleaseWaitScreenFrame(void)
{
    if (g_load_descriptor_69b7c8->waiting) {
        if (g_swap_disc_dialog_69b7cc->is_open) {
            g_swap_disc_dialog_69b7cc->vslot3();
            if (!g_swap_disc_dialog_69b7cc->Close()) {
                if (!g_swap_disc_dialog_69b7cc->close_result) {
                    delete g_swap_disc_dialog_69b7cc;
                    g_swap_disc_dialog_69b7cc = 0;
                    RequestScreenTransition();
                    return;
                }
                PLEASE_WAIT_SCREEN_DRAW();
                Function426790();
                return;
            }
        }
        else if (GetTickCount() - g_load_descriptor_69b7c8->entered_tick > 200) {
            if (!Function42B6F0(g_load_descriptor_69b7c8->parameter)) {
                g_load_descriptor_69b7c8->waiting = 0;
                RefreshSlfArchives();
                g_swap_disc_dialog_69b7cc->is_open = 0;
            }
            else if (!g_swap_disc_dialog_69b7cc->is_open
                     && ++g_value_69b7c4 > 4) {
                g_swap_disc_dialog_69b7cc->is_open = 1;
                g_value_69b7c4 = 0;
            }
            g_load_descriptor_69b7c8->entered_tick = GetTickCount();
        }
        Function426790();
        return;
    }

    switch (g_load_descriptor_69b7c8->mode) {
    case 0:
        wcscpy(g_load_descriptor_69b7c8->caption, gppStringList[0x1bbc / 4]);
        break;
    case 1:
        {
            wchar_t** strings = gppStringList;
            wcscpy(g_load_descriptor_69b7c8->caption,
                   strncmp(g_load_descriptor_69b7c8->name, "Quick",
                           strlen("Quick")) == 0
                       ? strings[0x1bc4 / 4]
                       : strings[0x1bc0 / 4]);
        }
        break;
    case 2:
        wcscpy(g_load_descriptor_69b7c8->caption, gppStringList[0x1bc8 / 4]);
        break;
    case 3:
        if ((unsigned int)g_load_descriptor_69b7c8->parameter < 0x2f) {
            swprintf(g_load_descriptor_69b7c8->caption, L"%s %s...",
                     gppStringList[0x1bcc / 4],
                     gppStringList[g_level_name_indices_605820
                                       [g_load_descriptor_69b7c8->parameter]]);
        }
        else if (g_load_descriptor_69b7c8->parameter == 0x38) {
            wcscpy(g_load_descriptor_69b7c8->caption, L"Entering default level...");
        }
        else {
            swprintf(g_load_descriptor_69b7c8->caption, L"Entering test level %c..",
                     g_load_descriptor_69b7c8->parameter + 2);
        }
    }

    PLEASE_WAIT_SCREEN_DRAW();
    Function426790();
    Function426790();
    ClearFlag603C60();

    switch (g_load_descriptor_69b7c8->mode) {
    case 0:
        if (PleaseWaitScreenEnsureLevelArchive(8)) {
            Function512C40();
            int level;
            int entrance;
            Function5092F0(&level, &entrance);
            if (!LoadLevel(level, entrance, 0)) {
                srAssertFail("fVerify", PLEASE_WAIT_SCREEN_CPP, 279, 0);
            }
            Function5063E0();
        }
        break;
    case 1:
        if (PleaseWaitScreenEnsureLevelArchive(g_screen_state_0068ec78.parameter)) {
            if (!LoadGame(g_load_descriptor_69b7c8->name)) {
                srAssertFail("fVerify", PLEASE_WAIT_SCREEN_CPP, 293, 0);
            }
            if (!LoadLevel(g_current_level, -1, 1)) {
                srAssertFail("fVerify", PLEASE_WAIT_SCREEN_CPP, 296, 0);
            }
        }
        break;
    case 2: {
        unsigned char saved = SaveGame(g_load_descriptor_69b7c8->name,
                                       g_load_descriptor_69b7c8->save_payload);
        Function58AC00(0xc,
                       saved ? gppStringList[0x1bd0 / 4] : gppStringList[0x1bd8 / 4],
                       -1, -1, 0);
        if (g_load_descriptor_69b7c8->save_payload) {
            ::operator delete(g_load_descriptor_69b7c8->save_payload);
        }
        while (GetTickCount() - g_load_descriptor_69b7c8->entered_tick < 500) {
        }
        break;
    }
    case 3:
        if (PleaseWaitScreenEnsureLevelArchive(g_load_descriptor_69b7c8->parameter)) {
            if (!Function42AF60(g_load_descriptor_69b7c8->parameter,
                                g_load_descriptor_69b7c8->parameter_2)) {
                srAssertFail("fVerify", PLEASE_WAIT_SCREEN_CPP, 320, 0);
            }
            Function5159E0(1);
        }
    }

    if (!g_load_descriptor_69b7c8->waiting) {
        if (!GetWorld()) {
            srAssertFail("GetWorld()", PLEASE_WAIT_SCREEN_CPP, 332, 0);
        }
        delete g_swap_disc_dialog_69b7cc;
        g_swap_disc_dialog_69b7cc = 0;
        RequestScreenTransition();
        if (g_load_descriptor_69b7c8->mode == 1 && g_flag_689b2c) {
            SetValue64D8AC(4);
            SetPendingScreenState(0);
            return;
        }
        if (Function55EC10() != 7) {
            SetPendingScreenState(7);
        }
    }
}

/* The leave. The leaving pass releases the descriptor the entry handler
   allocated; the ordinary pass only tears the frame down. Both passes run the
   teardown, which is why the release sits under the flag rather than the whole
   body. */
// FUNCTION: WIZ8 0x00591560
unsigned char PleaseWaitScreenLeave(char leaving)
{
    if (leaving) {
        free(g_load_descriptor_69b7c8);
        g_load_descriptor_69b7c8 = 0;
    }
    NoOp();
    ShutdownDisplayList();
    ResetRegions();
    SetFlag603C60();
    return 1;
}

/* Texture loads performed while screen four is active keep audio and the
   animated please-wait glyph moving. The descriptor fields and the draw call
   are the same private state used by PleaseWaitScreenFrame above. */
// FUNCTION: WIZ8 0x005915A0
void UpdatePleaseWaitLoadFrame005915A0(void)
{
    unsigned long tick;

    SoundServiceStreams();
    Function48F9E0();
    tick = GetTickCount();
    if (tick - g_load_descriptor_69b7c8->entered_tick > 499) {
        g_load_descriptor_69b7c8->caption_y =
            (g_load_descriptor_69b7c8->caption_y + 1) % 0x18;
        g_load_descriptor_69b7c8->entered_tick = tick;
        Function549600(-0xe, 0x1dd, 0,
                       g_load_descriptor_69b7c8->caption_y,
                       0, 0x185, 2, 0);
        Function426790();
    }
}
