#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/CreditsScreen.h"
#include "wiz8/local_screens/Screens.h"

#include "wiz8/cursor.h"
#include "wiz8/music_playlist.h"
#include "wiz8/regions.h"
#include "wiz8/utility.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/fonts.h"
#include "wiz8/virtual_file.h"
#include "wiz8/wiz8_windows.h"

#include "FileMan.h"
#include "Font.h"
#include "input.h"

#include <stdlib.h>
#include <wchar.h>

// GLOBAL: WIZ8 0x0069C4A8
W8GrowableVector<W8CreditLine>* g_credit_lines;
// GLOBAL: WIZ8 0x0069C494
int g_credit_elapsed_steps;
// GLOBAL: WIZ8 0x0069C498
bool g_credit_redraw;
// GLOBAL: WIZ8 0x0069C49C
unsigned long g_credit_started_at;
// GLOBAL: WIZ8 0x0069C4A0
int g_credit_y;
// GLOBAL: WIZ8 0x0069C4A4
int g_credit_line;

/* Read one wide line, stopping at a newline, capacity, or the end of the
   stream. Answers whether the line ended at a newline; trailing carriage
   returns are stripped. */
// FUNCTION: WIZ8 0x004CEED0
unsigned char ReadWideTextLine(int handle, wchar_t* destination, int capacity, unsigned char* more)
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
        } else if (ok == 0) {
            *more = 0;
        } else if (current == 10) {
            break;
        } else {
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
    DisableCursorScene();
    g_credit_lines = new W8GrowableVector<W8CreditLine>();

    handle = FileOpen((char*)"Data\\Options\\Credits.txt", FILE_ACCESS_READ, 0);
    if (handle != 0) {
        unsigned char more;
        FileRead(handle, line, sizeof(wchar_t), 0);
        while (!FileCheckEndOfFile(handle)) {
            if (ReadWideTextLine(handle, line, 128, &more) && line[0] != L'*') {
                W8CreditLine entry = {0, 0, 0, 0, 0};
                bool blank = false;
                bool bold = false;
                if (line[0] == L'!') {
                    bold = true;
                    entry.flags = 1;
                    entry.primary = _wcsdup(line + 1);
                } else {
                    wchar_t* separator = wcschr(line, L'&');
                    if (separator != 0) {
                        separator[-1] = L'\0';
                        entry.flags = 2;
                        entry.secondary = _wcsdup(separator + 2);
                        entry.primary = _wcsdup(line);
                    } else if (line[0] != L'\0') {
                        entry.primary = _wcsdup(line);
                    } else {
                        blank = true;
                        entry.flags = 4;
                    }
                }
                if (!blank) {
                    entry.pixel_width = StringPixLength(
                        entry.primary, bold ? g_options_title_font : g_options_detail_font);
                }
                entry.line_height = 0x14 + (bold ? 5 : 0);
                g_credit_lines->Add(entry);
            }
        }
        FileClose(handle);
    }
    g_credit_line = 0;
    g_credit_elapsed_steps = 0;
    g_credit_y = 0x1df;
    g_credit_started_at = GetTickCount();
    g_credit_redraw = true;
    return 1;
}

// FUNCTION: WIZ8 0x005bc420
unsigned char CreditsScreenLeave(int)
{
    for (int index = 0; index < g_credit_lines->count; ++index) {
        W8CreditLine* entry = g_credit_lines->GetAt(index);
        if (entry->primary != 0) {
            free(entry->primary);
        }
        if (entry->secondary != 0) {
            free(entry->secondary);
        }
    }
    delete g_credit_lines;
    ResetRegions();
    EnableCursorScene();
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

    int steps = (GetTickCount() - g_credit_started_at) / 35 - g_credit_elapsed_steps;
    if (steps >= 1) {
        g_credit_elapsed_steps += steps;
        g_credit_redraw = true;
        W8CreditLine entry = *g_credit_lines->GetAt(g_credit_line);
        g_credit_y -= steps;
        while (g_credit_y < 0) {
            ++g_credit_line;
            if (g_credit_line >= g_credit_lines->count) {
                RequestScreenTransition();
                break;
            }
            entry = *g_credit_lines->GetAt(g_credit_line);
            g_credit_y += entry.line_height;
        }
    }
    if (!g_credit_redraw) {
        return;
    }

    DrawCatalogImage(-14, 0xe9, 0, 0, 0, 0, 2, 0);
    int y = g_credit_y;
    for (int index = g_credit_line; index < g_credit_lines->count && y <= 0x1df; ++index) {
        const W8CreditLine* entry = g_credit_lines->GetAt(index);
        if ((entry->flags & 4) == 0) {
            SetFont((entry->flags & 1) ? g_options_title_font : g_options_detail_font);
            if ((entry->flags & 2) == 0) {
                gprintf((0x280 - entry->pixel_width) / 2, y, L"%s", entry->primary);
            } else {
                gprintf(0x136 - entry->pixel_width, y, L"%s", entry->primary);
                if (entry->secondary != 0) {
                    gprintf(0x14a, y, L"%s", entry->secondary);
                }
            }
        }
        y += entry->line_height;
    }
    ResetTransientRenderScenes();
    g_credit_redraw = false;
    RenderFrame();
}

/* Full-screen credits background: left-up or right-up leaves the screen. */
// FUNCTION: WIZ8 0x005BC7A0
unsigned char CreditsBackgroundRegionEvent(const InputAtom* event, W8Region*)
{
    int us_event = event->usEvent;
    if (us_event != LEFT_BUTTON_UP && us_event != RIGHT_BUTTON_UP) {
        return 0;
    }
    RequestScreenTransition();
    return 1;
}
