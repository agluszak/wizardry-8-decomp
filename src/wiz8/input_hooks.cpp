#include "wiz8/bringup_gates.h"
#include "wiz8/input_hooks.h"
#include "english.h"
#include "wiz8/sgp_input_private.h"

#include "wiz8/cursor.h"
#include "wiz8/utility.h"
#include "wiz8/wiz8_windows.h"

extern HWND g_window_6596cc;

/* Retail 0x005FF51C: virtual-key remap for keys 0x21-0x2E without the
   extended flag, transcribed from the image. */
unsigned short g_key_remap_5ff51c[14] = {
    0x0069, 0x0063, 0x0061, 0x0067, 0x0064, 0x0068, 0x0066,
    0x0062, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x006e
};

// LIBRARY: WIZ8 0x00401f90
// ShutdownInputManager

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
            if (gfCurrentStringInputState == 0) {
                gfKeyState[code] = 1;
                QueueEvent(1, code, packed);
                return;
            }
        }
        else if (gfCurrentStringInputState == 0) {
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
    StringInput* editor = gpCurrentStringDescriptor;
    unsigned int code = key;
    unsigned short current;

    if (editor == 0) {
        return;
    }
    switch (code) {
    case 8:
        current = editor->usStringOffset;
        if (current == 0) {
            return;
        }
        if (current <= editor->usCurrentStringLength) {
            do {
                code = current;
                ++current;
                editor->pString[code - 1] = editor->pString[code];
            } while (current <= editor->usCurrentStringLength);
        }
        --editor->usStringOffset;
        --editor->usCurrentStringLength;
        return;
    case 9:
        if (gfShiftState == 0) {
            if (editor->pNextString == 0) {
                return;
            }
            editor->fFocus = 0;
            goto next_editor;
        }
        if (editor->pPreviousString == 0) {
            return;
        }
        editor->fFocus = 0;
        editor = editor->pPreviousString;
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
        if (editor->pFilter != 0) {
            unsigned int allowed = 1;
            current = *editor->pFilter;
            if (current == 0) {
                return;
            }
            while (key != editor->pFilter[allowed]) {
                ++allowed;
                if (current < allowed) {
                    return;
                }
            }
        }
        if (editor->fInsertMode == 1) {
            current = editor->usCurrentStringLength;
            if ((int)(editor->usMaxStringLength - 1) <=
                (int)(unsigned int)current) {
                break;
            }
            if (editor->usStringOffset < current) {
                do {
                    code = current;
                    --current;
                    editor->pString[code] = editor->pString[code - 1];
                } while (editor->usStringOffset < current);
            }
            editor->pString[current] = key;
            ++editor->usStringOffset;
        }
        else {
            if ((int)(unsigned int)editor->usStringOffset <
                (int)(editor->usMaxStringLength - 1)) {
                editor->pString[editor->usStringOffset] = key;
                ++editor->usStringOffset;
            }
            if (editor->usStringOffset <= editor->usCurrentStringLength) {
                break;
            }
            editor->pString[editor->usStringOffset] = 0;
        }
        ++editor->usCurrentStringLength;
        break;
    case 0x0d:
        editor->fFocus = 0;
        if (editor->pNextString == 0) {
            editor->usLastCharacter = key;
            gfCurrentStringInputState = 0;
            return;
        }
next_editor:
        editor = editor->pNextString;
        editor->fFocus = 1;
        editor->usLastCharacter = 0;
        return;
    case 0x1b:
        editor->fFocus = 0;
        editor->usLastCharacter = key;
        gfCurrentStringInputState = 0;
        return;
    case 0x23:
        editor->usStringOffset = editor->usCurrentStringLength;
        editor->usLastCharacter = key;
        return;
    case 0x24:
        editor->usStringOffset = 0;
        editor->usLastCharacter = key;
        return;
    case 0x25:
        if (editor->usStringOffset != 0) {
            --editor->usStringOffset;
            editor->usLastCharacter = key;
            return;
        }
        break;
    case 0x26:
        if (editor->pPreviousString == 0) {
            return;
        }
        editor->fFocus = 0;
        editor = editor->pPreviousString;
activate_editor:
        editor->fFocus = 1;
        editor->usLastCharacter = 0;
        return;
    case 0x27:
        if (editor->usStringOffset < editor->usCurrentStringLength) {
            ++editor->usStringOffset;
            editor->usLastCharacter = key;
            return;
        }
        break;
    case 0x28:
        if (editor->pNextString == 0) {
            return;
        }
        editor->fFocus = 0;
        editor = editor->pNextString;
        editor->fFocus = 1;
        editor->usLastCharacter = 0;
        return;
    case 0x2d:
        editor->fInsertMode = editor->fInsertMode != 1;
        editor->usLastCharacter = key;
        return;
    case 0x2e:
        current = editor->usStringOffset;
        if (current < editor->usCurrentStringLength) {
            do {
                code = current;
                ++current;
                editor->pString[code] = editor->pString[code + 1];
            } while (current < editor->usCurrentStringLength);
            --editor->usCurrentStringLength;
            editor->usLastCharacter = key;
            return;
        }
        break;
    }
    editor->usLastCharacter = key;
}

// FUNCTION: WIZ8 0x004027a70
void Function427A70(RECT* rect)
{
    GetClientRect(g_window_6596cc, rect);
    ClientToScreen(g_window_6596cc, (POINT*)rect);
    ClientToScreen(g_window_6596cc, (POINT*)&rect->right);
}
