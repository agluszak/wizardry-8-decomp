#include "wiz8/local_screens/CreditsScreen.h"

#include "wiz8/cursor.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/virtual_file.h"
#include "wiz8/wiz8_windows.h"

extern "C" {
#include "FileMan.h"
#include "Font.h"
#include "input.h"
}

#include <stdlib.h>
#include <wchar.h>

extern int g_font_00683614;
extern int g_font_bold_0068368c;
extern void SetViewport(int left, int top, int right, int bottom);
extern void ResetRegions(void);
extern unsigned char ClearFlag603C60(void);
extern unsigned char SetFlag603C60(void);
extern unsigned char ReadWideTextLine004CEED0(
    int handle, wchar_t* destination, int capacity, unsigned char* more);
extern void Function407650(int x, int y, const wchar_t* format, ...);
extern void ResetTransientRenderScenes(void);
extern void RenderFrame(void);
extern char Function490180(const char* playlist);

W8GrowableVector<W8CreditLine>* g_credit_lines_0069c4a8;
int g_credit_elapsed_steps_0069c494;
unsigned char g_credit_redraw_0069c498;
unsigned long g_credit_started_at_0069c49c;
int g_credit_y_0069c4a0;
int g_credit_line_0069c4a4;

// FUNCTION: WIZ8 0x005bc130
unsigned char CreditsScreenEnter(void)
{
    int handle;
    wchar_t line[128];

    SetViewport(0, 0, 0x280, 0x1e0);
    ResetRegions();
    RegionSetEnable(2);
    ClearFlag603C60();
    g_credit_lines_0069c4a8 = new W8GrowableVector<W8CreditLine>();

    handle = FileOpen((char*)"Data\\Options\\Credits.txt", FILE_ACCESS_READ, 0);
    if (handle != 0) {
        unsigned char more;
        ReadVirtualFile(handle, line, sizeof(wchar_t), 0);
        while (!FileCheckEndOfFile(handle)) {
            if (ReadWideTextLine004CEED0(handle, line, 128, &more) && line[0] != L'*') {
                W8CreditLine entry = { 0, 0, 0, 0, 0 };
                bool blank = false;
                bool bold = false;
                if (line[0] == L'!') {
                    bold = true;
                    entry.flags = 1;
                    entry.primary = _wcsdup(line + 1);
                }
                else {
                    wchar_t* separator = wcschr(line, L'&');
                    if (separator != 0) {
                        separator[-1] = L'\0';
                        entry.flags = 2;
                        entry.secondary = _wcsdup(separator + 2);
                        entry.primary = _wcsdup(line);
                    }
                    else if (line[0] != L'\0') {
                        entry.primary = _wcsdup(line);
                    }
                    else {
                        blank = true;
                        entry.flags = 4;
                    }
                }
                if (!blank) {
                    entry.pixel_width = StringPixLength(
                        reinterpret_cast<unsigned short*>(entry.primary),
                        bold ? g_font_bold_0068368c : g_font_00683614);
                }
                entry.line_height = 0x14 + (bold ? 5 : 0);
                g_credit_lines_0069c4a8->Add(entry);
            }
        }
        CloseVirtualFile(handle);
    }
    g_credit_line_0069c4a4 = 0;
    g_credit_elapsed_steps_0069c494 = 0;
    g_credit_y_0069c4a0 = 0x1df;
    g_credit_started_at_0069c49c = GetTickCount();
    g_credit_redraw_0069c498 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x005bc420
unsigned char CreditsScreenLeave(int)
{
    for (int index = 0; index < g_credit_lines_0069c4a8->count; ++index) {
        W8CreditLine* entry = g_credit_lines_0069c4a8->GetAt(index);
        if (entry->primary != 0) {
            free(entry->primary);
        }
        if (entry->secondary != 0) {
            free(entry->secondary);
        }
    }
    delete g_credit_lines_0069c4a8;
    ResetRegions();
    SetFlag603C60();
    if (Function490180("EndCredit.MPL")) {
        StopMusicPlaylist(1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005bc530
void CreditsScreenFrame(void)
{
    W8ScreenPoint point;
    InputAtom input;

    GetScreenPoint004284F0(&point);
    UpdateRegionMousePosition(point.x, point.y);
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
            RequestScreenTransition();
        }
    }

    int steps = (GetTickCount() - g_credit_started_at_0069c49c) / 35
                - g_credit_elapsed_steps_0069c494;
    if (steps >= 1) {
        g_credit_elapsed_steps_0069c494 += steps;
        g_credit_redraw_0069c498 = 1;
        W8CreditLine entry = *g_credit_lines_0069c4a8->GetAt(g_credit_line_0069c4a4);
        g_credit_y_0069c4a0 -= steps;
        while (g_credit_y_0069c4a0 < 0) {
            ++g_credit_line_0069c4a4;
            if (g_credit_line_0069c4a4 >= g_credit_lines_0069c4a8->count) {
                RequestScreenTransition();
                break;
            }
            entry = *g_credit_lines_0069c4a8->GetAt(g_credit_line_0069c4a4);
            g_credit_y_0069c4a0 += entry.line_height;
        }
    }
    if (!g_credit_redraw_0069c498) {
        return;
    }

    DrawCatalogImage(-14, 0xe9, 0, 0, 0, 0, 2, 0);
    int y = g_credit_y_0069c4a0;
    for (int index = g_credit_line_0069c4a4;
         index < g_credit_lines_0069c4a8->count && y <= 0x1df;
         ++index) {
        const W8CreditLine* entry = g_credit_lines_0069c4a8->GetAt(index);
        if ((entry->flags & 4) == 0) {
            SetFont((entry->flags & 1) ? g_font_bold_0068368c : g_font_00683614);
            if ((entry->flags & 2) == 0) {
                Function407650((0x280 - entry->pixel_width) / 2, y,
                               L"%s", entry->primary);
            }
            else {
                Function407650(0x136 - entry->pixel_width, y,
                               L"%s", entry->primary);
                if (entry->secondary != 0) {
                    Function407650(0x14a, y, L"%s", entry->secondary);
                }
            }
        }
        y += entry->line_height;
    }
    ResetTransientRenderScenes();
    g_credit_redraw_0069c498 = 0;
    RenderFrame();
}
