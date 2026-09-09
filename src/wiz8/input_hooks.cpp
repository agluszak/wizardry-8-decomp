#include "wiz8/screen_state.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/input_hooks.h"
#include "english.h"
#include "wiz8/sgp_input_private.h"

#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"

#include <string.h>

extern HWND g_window_6596cc;

/* The text line editor descriptor the hooks edit through. Only the fields
   Function4023B0 touches are named; the rest stay positional. */
struct W8TextEditState {
    unsigned short* buffer_00;
    int unknown_04;
    unsigned short* allowed_08;
    unsigned short max_0c;
    unsigned short end_0e;
    unsigned short cursor_10;
    unsigned short last_key_12;
    unsigned char insert_14;
    unsigned char active_15;
    unsigned char unknown_16[2];
    W8TextEditState* prev_18;
    W8TextEditState* next_1c;
};

/* Retail 0x006F0500: text-entry active flag. */
unsigned char g_text_entry_active_6f0500;
/* Retail 0x006F0510: the active text line editor, or 0. */
struct W8TextEditState* g_text_editor_6f0510;
/* Retail 0x00650DBA: non-client mouse flag the mouse hook maintains. */
unsigned char g_input_flag_650dba;

/* Retail 0x005FF51C: virtual-key remap for keys 0x21-0x2E without the
   extended flag, transcribed from the image. */
unsigned short g_key_remap_5ff51c[14] = {
    0x0069, 0x0063, 0x0061, 0x0067, 0x0064, 0x0068, 0x0066,
    0x0062, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x006e
};

// FUNCTION: WIZ8 0x00401ea0
unsigned char InitializeInputManager00401EA0(void)
{
    DWORD thread;

    memset(gfKeyState, 0, sizeof(gfKeyState));
    gusQueueCount = 0;
    gusHeadIndex = 0;
    gusTailIndex = 0;
    gfTrackMousePos = 0;
    gfShiftState = 0;
    gfAltState = 0;
    gfCtrlState = 0;
    gfTrackDblClick = 1;
    guiDoubleClkDelay = 300;
    guiSingleClickTimer = 0;
    gfRecordedLeftButtonUp = 0;
    gfLeftButtonState = 0;
    gfRightButtonState = 0;
    guiLeftButtonRepeatTimer = 0;
    guiRightButtonRepeatTimer = 0;
    gusMouseXPos = 0x140;
    gusMouseYPos = 0xf0;
    g_text_entry_active_6f0500 = 0;
    g_text_editor_6f0510 = 0;
    thread = GetCurrentThreadId();
    ghKeyboardHook = SetWindowsHookExA(
        WH_KEYBOARD, Function401B30, NULL, thread);
    thread = GetCurrentThreadId();
    ghMouseHook = SetWindowsHookExA(
        WH_MOUSE, Function401C70, NULL, thread);
    return 1;
}

// FUNCTION: WIZ8 0x00401b30
long __stdcall Function401B30(int code, unsigned int key, long flags)
{
    if (code < 0 || g_application_active == 0) {
        return CallNextHookEx(ghKeyboardHook, code, key, flags);
    }
    if ((flags & 0x80000000) == 0) {
        if (key == 0x10) {
            gfShiftState = 1;
            gfKeyState[0x10] = 1;
            gfSGPInputReceived = 1;
            return 1;
        }
        if (key == 0x11) {
            gfKeyState[0x11] = 1;
            gfSGPInputReceived = 1;
            gfCtrlState = 2;
            return 1;
        }
        if (key == 0x12) {
            gfKeyState[0x12] = 1;
            gfSGPInputReceived = 1;
            gfAltState = 4;
            return 1;
        }
        if (key != 0x2c) {
            Function402270(key, flags, 1);
        }
        gfSGPInputReceived = 1;
        return 1;
    }
    if (key == 0x10) {
        gfShiftState = 0;
        gfKeyState[0x10] = 0;
        return 1;
    }
    if (key == 0x11) {
        gfCtrlState = 0;
        gfKeyState[0x11] = 0;
        return 1;
    }
    if (key == 0x12) {
        gfAltState = 0;
        gfKeyState[0x12] = 0;
        return 1;
    }
    if (key == 0x2c) {
        if (gfKeyState[0x11] != 0) {
            NoOp();
            return 1;
        }
        Function4229D0();
        return 1;
    }
    Function402270(key, flags, 0);
    return 1;
}

// FUNCTION: WIZ8 0x00401c70
long __stdcall Function401C70(int code, unsigned int button, long info)
{
    MOUSEHOOKSTRUCT* state = (MOUSEHOOKSTRUCT*)info;
    unsigned int x = state->pt.x;
    unsigned int y = state->pt.y;
    unsigned int packed;
    RECT client;
    unsigned char outside;

    outside = 0;
    if (Function4229B0() == 0) {
        if (button == 0xa1) {
            g_input_flag_650dba = 1;
        }
        Function427A70(&client);
        if (x < (unsigned int)client.left || (unsigned int)client.right < x ||
            y < (unsigned int)client.top || (unsigned int)client.bottom < y) {
            outside = 1;
        }
    }
    if (code >= 0 && g_application_active != 0 && !outside &&
        g_input_flag_650dba == 0) {
        switch (button - 0x200) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            if (Function4229B0() == 0) {
                x -= client.left;
                y -= client.top;
                gusMouseXPos = (unsigned short)x;
            }
            else {
                gusMouseXPos = (unsigned short)x;
            }
            gusMouseYPos = (unsigned short)y;
            packed = (y << 0x10) | (x & 0xffff);
            gfSGPInputReceived = 1;
            break;
        default:
            packed = button;
            break;
        }
        if (button == 0x20a) {
            return 0;
        }
        switch (button - 0x200) {
        case 0:
            if (gfTrackMousePos != 0) {
                QueueEvent(0x400, 0, packed);
            }
            break;
        case 1:
            gfLeftButtonState = 1;
            QueueEvent(8, 0, packed);
            return 1;
        case 2:
            gfLeftButtonState = 0;
            QueueEvent(0x10, 0, packed);
            return 1;
        case 4:
            gfRightButtonState = 1;
            QueueEvent(0x80, 0, packed);
            return 1;
        case 5:
            gfRightButtonState = 0;
            QueueEvent(0x100, 0, packed);
            return 1;
        }
        return 1;
    }
    LRESULT next = CallNextHookEx(ghMouseHook, code, button, info);
    if (button == 0x202 || button == 0xa2) {
        g_input_flag_650dba = 0;
    }
    return next;
}

// LIBRARY: WIZ8 0x00401f90

// FUNCTION: WIZ8 0x00402270
void Function402270(unsigned int key, unsigned int flags, char pressed)
{
    W8ScreenPoint point;
    unsigned int packed;
    unsigned int code;

    if (key == 0x0c) {
        key = 0x65;
    }
    else if (key < 0x2f && key > 0x20 && (flags & 0x1000000) == 0) {
        key = g_key_remap_5ff51c[key - 0x21];
    }
    else if (key == 0x0d && (flags & 0x1000000) != 0) {
        key = 0x6c;
    }
    GetScreenPoint004284F0(&point);
    packed = ((unsigned int)point.y << 0x10) | ((unsigned int)point.x & 0xffff);
    if (pressed == 1) {
        code = key & 0xffff;
        if (gfKeyState[code] == 0) {
            if (g_text_entry_active_6f0500 == 0) {
                gfKeyState[code] = 1;
                QueueEvent(1, code, packed);
                return;
            }
        }
        else if (g_text_entry_active_6f0500 == 0) {
            QueueEvent(4, code, packed);
            return;
        }
        Function4023B0((unsigned short)key);
        return;
    }
    code = key & 0xffff;
    if (gfKeyState[code] == 1) {
        gfKeyState[code] = 0;
        QueueEvent(2, code, packed);
        return;
    }
    if ((short)key == 9 && gfAltState != 0) {
        ShowWindow(g_window_6596cc, 6);
        gfKeyState[0x12] = 0;
        gfAltState = 0;
    }
}

// FUNCTION: WIZ8 0x004023b0
void Function4023B0(unsigned short key)
{
    W8TextEditState* editor = g_text_editor_6f0510;
    unsigned int code = key;
    unsigned short current;

    if (editor == 0) {
        return;
    }
    switch (code) {
    case 8:
        current = editor->cursor_10;
        if (current == 0) {
            return;
        }
        if (current <= editor->end_0e) {
            do {
                code = current;
                ++current;
                editor->buffer_00[code - 1] = editor->buffer_00[code];
            } while (current <= editor->end_0e);
        }
        --editor->cursor_10;
        --editor->end_0e;
        return;
    case 9:
        if (gfShiftState == 0) {
            if (editor->next_1c == 0) {
                return;
            }
            editor->active_15 = 0;
            goto next_editor;
        }
        if (editor->prev_18 == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->prev_18;
        goto activate_editor;
    default:
        current = (unsigned short)(
            gfAltState | gfCtrlState | gfShiftState);
        if ((current & 6) != 0) {
            return;
        }
        if ((current & 1) == 0) {
            key = gsKeyTranslationTable[code];
        }
        else {
            key = gsKeyTranslationTable[code + 256];
        }
        if (key == 0) {
            return;
        }
        if (editor->allowed_08 != 0) {
            unsigned int allowed = 1;
            current = *editor->allowed_08;
            if (current == 0) {
                return;
            }
            while (key != editor->allowed_08[allowed]) {
                ++allowed;
                if (current < allowed) {
                    return;
                }
            }
        }
        if (editor->insert_14 == 1) {
            current = editor->end_0e;
            if ((int)(editor->max_0c - 1) <= (int)(unsigned int)current) {
                break;
            }
            if (editor->cursor_10 < current) {
                do {
                    code = current;
                    --current;
                    editor->buffer_00[code] = editor->buffer_00[code - 1];
                } while (editor->cursor_10 < current);
            }
            editor->buffer_00[current] = key;
            ++editor->cursor_10;
        }
        else {
            if ((int)(unsigned int)editor->cursor_10 <
                (int)(editor->max_0c - 1)) {
                editor->buffer_00[editor->cursor_10] = key;
                ++editor->cursor_10;
            }
            if (editor->cursor_10 <= editor->end_0e) {
                break;
            }
            editor->buffer_00[editor->cursor_10] = 0;
        }
        ++editor->end_0e;
        break;
    case 0x0d:
        editor->active_15 = 0;
        if (editor->next_1c == 0) {
            editor->last_key_12 = key;
            g_text_entry_active_6f0500 = 0;
            return;
        }
next_editor:
        editor = editor->next_1c;
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x1b:
        editor->active_15 = 0;
        editor->last_key_12 = key;
        g_text_entry_active_6f0500 = 0;
        return;
    case 0x23:
        editor->cursor_10 = editor->end_0e;
        editor->last_key_12 = key;
        return;
    case 0x24:
        editor->cursor_10 = 0;
        editor->last_key_12 = key;
        return;
    case 0x25:
        if (editor->cursor_10 != 0) {
            --editor->cursor_10;
            editor->last_key_12 = key;
            return;
        }
        break;
    case 0x26:
        if (editor->prev_18 == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->prev_18;
activate_editor:
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x27:
        if (editor->cursor_10 < editor->end_0e) {
            ++editor->cursor_10;
            editor->last_key_12 = key;
            return;
        }
        break;
    case 0x28:
        if (editor->next_1c == 0) {
            return;
        }
        editor->active_15 = 0;
        editor = editor->next_1c;
        editor->active_15 = 1;
        editor->last_key_12 = 0;
        return;
    case 0x2d:
        editor->insert_14 = editor->insert_14 != 1;
        editor->last_key_12 = key;
        return;
    case 0x2e:
        current = editor->cursor_10;
        if (current < editor->end_0e) {
            do {
                code = current;
                ++current;
                editor->buffer_00[code] = editor->buffer_00[code + 1];
            } while (current < editor->end_0e);
            --editor->end_0e;
            editor->last_key_12 = key;
            return;
        }
        break;
    }
    editor->last_key_12 = key;
}

// FUNCTION: WIZ8 0x004027a70
void Function427A70(RECT* rect)
{
    GetClientRect(g_window_6596cc, rect);
    ClientToScreen(g_window_6596cc, (POINT*)rect);
    ClientToScreen(g_window_6596cc, (POINT*)&rect->right);
}
