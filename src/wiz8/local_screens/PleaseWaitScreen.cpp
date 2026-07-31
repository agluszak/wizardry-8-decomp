#include "wiz8/wiz8_windows.h"
#include "wiz8/screen_state.h"

#include "Font.h"
#include "FileMan.h"

#include <stdlib.h>
#include <string.h>

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
// GLOBAL: WIZ8 0x0069B7D0
unsigned char g_cd_marker_present_69b7d0;

extern int g_dword_686a70;

}

void ResetRegions(void);
unsigned char SetFlag603C60(void);
unsigned char ClearFlag603C60(void);

extern "C" {

/* gameplay_boundaries.h owns this declaration and gives it C linkage; the
   spelling is repeated rather than the header included, because that header is
   at its includer ceiling. Linkage has to agree with it or the call resolves
   nowhere. */
extern void InitializeFactState(void);
extern void NoOp(void);
extern void ShutdownDisplayList(void);
extern int Function40B290(void);
extern void ConfigurePresentation00413FD0(int a, int b, int c, int d, int e);
extern void SetRenderClip00407220(int target, int left, int top, int right,
                                  int bottom, int flags);
extern void Function425570(int value);
extern int Function509750(void);
extern void Function58FD30(void);

}

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
        g_load_descriptor_69b7c8->mode = g_screen_state_0068ec78.state.mode;
        switch (g_load_descriptor_69b7c8->mode) {
        case 0:
            InitializeFactState();
            g_load_descriptor_69b7c8->parameter = Function509750();
            Function58FD30();
            DeleteFileA("Saves\\CurrentGame.SAV");
            break;
        case 1:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.state.parameter;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.state.name);
            break;
        case 2:
            g_load_descriptor_69b7c8->parameter = g_dword_686a70;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.state.name);
            g_load_descriptor_69b7c8->save_payload = g_screen_state_0068ec78.state.parameter_3;
            break;
        case 3:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.state.parameter;
            g_load_descriptor_69b7c8->parameter_2 = g_screen_state_0068ec78.state.parameter_2;
            break;
        }
    }
    Function40B290();
    ResetRegions();
    ConfigurePresentation00413FD0(0x500, 0, 0, 0x280, 0x1e0);
    SetRenderClip00407220(-14, 0, 0, 0x280, 0x1e0, 0);
    Function425570(0);
    ClearFlag603C60();
    g_load_descriptor_69b7c8->caption_y = 0;
    g_load_descriptor_69b7c8->entered_tick = GetTickCount();
    g_value_69b7c4 = 0;
    return 1;
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
