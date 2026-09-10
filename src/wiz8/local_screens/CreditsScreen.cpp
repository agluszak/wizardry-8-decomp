#include "wiz8/render_state.h"
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
// GLOBAL
int g_font_00683614;
extern int g_font_bold_0068368c;
// GLOBAL: WIZ8 0x0068368c
int g_font_bold_0068368c;
W8GrowableVector<W8CreditLine>* g_credit_lines_0069c4a8;
int g_credit_elapsed_steps_0069c494;
unsigned char g_credit_redraw_0069c498;
unsigned long g_credit_started_at_0069c49c;
int g_credit_y_0069c4a0;
int g_credit_line_0069c4a4;

/* Read one wide line, stopping at a newline, capacity, or the end of the
   stream. Answers whether the line ended at a newline; trailing carriage
   returns are stripped. */
// FUNCTION: WIZ8 0x004CEED0
unsigned char ReadWideTextLine004CEED0(
    int handle, wchar_t* destination, int capacity, unsigned char* more)
{
    wchar_t* write = destination;
    wchar_t current = 0;
    int count = 0;
    unsigned char ok;

    *destination = 0;
    *more = 1;
    for (;;) {
        unsigned int bytes_read;
        ok = FileRead(handle, &current, sizeof(current), &bytes_read);
        if (bytes_read == 0) {
            ok = 0;
            *more = 0;
        }
        else if (ok == 0) {
            *more = 0;
        }
        else if (current == 10) {
            break;
        }
        else {
            *write++ = current;
            ++count;
        }
        if (count >= capacity - 1) {
            ok = 0;
            break;
        }
        if (ok == 0) {
            break;
        }
    }
    destination[count] = 0;
    if (count != 0 && destination[count - 1] == 0xd) {
        destination[count - 1] = 0;
    }
    return ok;
}

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
        FileRead(handle, line, sizeof(wchar_t), 0);
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
                        entry.primary,
                        bold ? g_font_bold_0068368c : g_font_00683614);
                }
                entry.line_height = 0x14 + (bold ? 5 : 0);
                g_credit_lines_0069c4a8->Add(entry);
            }
        }
        FileClose(handle);
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
    if (IsCurrentMusicPlaylist("EndCredit.MPL")) {
        StopMusicPlaylist(1);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005bc530
void CreditsScreenFrame(void)
{
    POINT point;
    InputAtom input;

    SGPMouseGetPos(&point);
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
                gprintf((0x280 - entry->pixel_width) / 2, y,
                        (unsigned short*)L"%s", entry->primary);
            }
            else {
                gprintf(0x136 - entry->pixel_width, y,
                        (unsigned short*)L"%s", entry->primary);
                if (entry->secondary != 0) {
                    gprintf(0x14a, y, (unsigned short*)L"%s", entry->secondary);
                }
            }
        }
        y += entry->line_height;
    }
    ResetTransientRenderScenes();
    g_credit_redraw_0069c498 = 0;
    RenderFrame();
}
