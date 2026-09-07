#include "wiz8/wiz8_windows.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/text_input.h"

#include "Font.h"
#include "himage.h"
#include "input.h"
#include "mousesystem.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* Wizardry's product fork of Sir-Tech's released Utils/Text_Input.c. */

typedef void (*INPUT_CALLBACK)(unsigned char index, int active);

/* The allocation at 0x005D39B0 is 0x6c bytes. Repeated traversal fixes the
   scalar fields and links, while calls into the pinned SGP oracle fix
   0x10-0x5b as one ordinary MOUSE_REGION. */
struct TEXTINPUTNODE {
    unsigned char ubID;
    unsigned char _padding01;
    short usInputType;
    unsigned char ubMaxChars;
    unsigned char _padding05[3];
    wchar_t* szString;
    unsigned char ubStrLen;
    unsigned char fEnabled;
    unsigned char fUserField;
    unsigned char _padding0f;
    MOUSE_REGION region;
    INPUT_CALLBACK InputCallback;
    unsigned char flag_60;
    unsigned char blocks_mouse_callback;
    unsigned char value_62[2];
    TEXTINPUTNODE* next;
    TEXTINPUTNODE* prev;
};

/* 0x005D3520 allocates this record once per active input session, and
   0x005D35E0 fills the same fixed layout for its three presentation modes. */
struct TextInputColors {
    short usFont;
    unsigned short usTextFieldColor;
    unsigned char ubForeColor;
    unsigned char ubShadowColor;
    unsigned char ubHiForeColor;
    unsigned char ubHiShadowColor;
    unsigned char ubHiBackColor;
    unsigned char fBevelling;
    unsigned short usBrighterColor;
    unsigned short usDarkerColor;
    unsigned short usCursorColor;
    unsigned char fUseDisabledAutoShade;
    unsigned char ubDisabledForeColor;
    unsigned char ubDisabledShadowColor;
    unsigned char _padding13;
    unsigned short usDisabledTextFieldColor;
    unsigned short usWizardryInactiveColor;
};

struct STACKTEXTINPUTNODE {
    TEXTINPUTNODE* head;
    TextInputColors* pColors;
    STACKTEXTINPUTNODE* next;
};

static_assert(sizeof(TEXTINPUTNODE) == 0x6c,
              "text input field must match the retail allocation");
static_assert(sizeof(TextInputColors) == 0x18,
              "text input style must match the retail allocation");
static_assert(sizeof(STACKTEXTINPUTNODE) == 0x0c,
              "text input session must match the retail allocation");

extern "C" {
extern int g_wiz_text_mono_font_683630;
extern int g_font12point1_683648;
extern unsigned char g_flag_6f04ed;
unsigned char gfEditingText;
}
extern wchar_t g_no_target_text[];
extern unsigned char FillSurfaceRect(int surface_id, int left, int top,
                                     int right, int bottom, int colour);
extern void* Function402B90(int surface_id, unsigned int* pitch);
extern void Function402C30(int surface_id);
extern int Function4124A0(void* pixels, unsigned int pitch, int* rectangle);
extern int MSYS_GrabMouse(MOUSE_REGION* region);
extern bool IsModalOpen(void);
extern int Function55EF80(void);
extern void Function55EE70(int value);
void MouseMovedInTextRegionCallback(MOUSE_REGION* region, int reason);
extern void MouseClickedInTextRegionCallback(MOUSE_REGION* region, int reason);

static TextInputColors* pColors;
static STACKTEXTINPUTNODE* pInputStack;
static TEXTINPUTNODE* gpTextInputHead;
static TEXTINPUTNODE* gpTextInputTail;
static TEXTINPUTNODE* gpActive;
static unsigned char gfTextInputMode;
static unsigned char gfHiliteMode;
static unsigned char gubCursorPos;
static unsigned char gubStartHilite;
static unsigned char gubEndHilite;
static unsigned char gubParkingPos;
static unsigned char gubVisibleStart;
static unsigned char gfHorizontalKey;
static unsigned char gubMouseDownPos;
static int gsCursorX;
static size_t guiVisibleCount;

void SetTextInputScheme(int mode);
unsigned int CalculateCursorPos(int width, int cursor, const wchar_t* text,
                           int* cursor_width, size_t* visible_count);
void RenderBackgroundField(TEXTINPUTNODE* field);
void RenderInactiveTextFieldNode(TEXTINPUTNODE* field);
void SelectAllText(void);
void HandleExclusiveInput(unsigned short character);
void AddChar(unsigned short character);
void SetTextInputCursor(unsigned char cursor);
unsigned short Function402780(unsigned short key, unsigned char modifiers);
unsigned short Function402800(unsigned short character);
unsigned short Function402820(unsigned short character);
unsigned short Function402840(unsigned short character);
int Function402880(int character);
int Function4028A0(int character);

// FUNCTION: WIZ8 0x005D3520
void InitTextInputModeWithScheme(int mode)
{
    if (gpTextInputHead != 0) {
        STACKTEXTINPUTNODE* session =
            (STACKTEXTINPUTNODE*)malloc(sizeof(STACKTEXTINPUTNODE));
        session->head = gpTextInputHead;
        session->pColors = pColors;
        session->next = pInputStack;
        pInputStack = session;
        for (TEXTINPUTNODE* field = gpTextInputHead;
             field != 0; field = field->next) {
            if (field->fEnabled != 0) {
                MSYS_DisableRegion(&field->region);
                field->fEnabled = 0;
            }
        }
        gpActive = 0;
    }
    gpTextInputHead = 0;
    pColors =
        (TextInputColors*)malloc(sizeof(TextInputColors));
    gfTextInputMode = 1;
    gfEditingText = 0;
    pColors->fBevelling = 0;
    pColors->fUseDisabledAutoShade = 1;
    pColors->usCursorColor = Get16BPPColor(0x0a0a0a);
    gubVisibleStart = 0;
    SetTextInputScheme(mode);
}

// FUNCTION: WIZ8 0x005D35E0
void SetTextInputScheme(int mode)
{
    if (mode == 0) {
        pColors->usFont = (short)g_font12point1_683648;
        pColors->usTextFieldColor = Get16BPPColor(0x00c8c8);
        pColors->usWizardryInactiveColor = Get16BPPColor(0xffffff);
        pColors->usDarkerColor = Get16BPPColor(0x513d18);
        pColors->usBrighterColor = Get16BPPColor(0x878a88);
        pColors->fBevelling = 1;
        pColors->ubForeColor = 0x8d;
        pColors->ubShadowColor = 0;
        pColors->ubHiForeColor = 0xd0;
    }
    else if (mode == 1) {
        pColors->usFont = (short)g_wiz_text_mono_font_683630;
        pColors->usTextFieldColor = Get16BPPColor(0x632a1e);
        pColors->usWizardryInactiveColor = Get16BPPColor(0x0a0a0a);
        pColors->usDarkerColor = Get16BPPColor(0);
        pColors->usBrighterColor = Get16BPPColor(0);
        pColors->fBevelling = 1;
        pColors->ubForeColor = 4;
        pColors->ubShadowColor = 5;
        pColors->ubHiForeColor = 4;
        pColors->ubHiShadowColor = 5;
        pColors->ubHiBackColor = 3;
        pColors->usCursorColor = Get16BPPColor(0xffffff);
        return;
    }
    else if (mode == 2) {
        pColors->usFont = (short)g_wiz_text_mono_font_683630;
        pColors->usTextFieldColor = Get16BPPColor(0xffffff);
        pColors->usWizardryInactiveColor = Get16BPPColor(0xffffff);
        pColors->usDarkerColor = Get16BPPColor(0);
        pColors->usBrighterColor = Get16BPPColor(0);
        pColors->fBevelling = 1;
        pColors->ubForeColor = 0x8d;
        pColors->ubShadowColor = 0;
        pColors->ubHiForeColor = 0xd0;
    }
    else {
        return;
    }
    pColors->ubHiShadowColor = 0xcd;
    pColors->ubHiBackColor = 0xcd;
    pColors->usCursorColor = Get16BPPColor(0x0a0a0a);
}

// FUNCTION: WIZ8 0x005D3800
void KillTextInputMode(void)
{
    TEXTINPUTNODE* field = gpTextInputHead;
    if (field == 0) return;
    do {
        gpTextInputHead = field->next;
        if (field->szString != 0) {
            free(field->szString);
            field->szString = 0;
            MSYS_RemoveRegion(&field->region);
        }
        free(field);
        field = gpTextInputHead;
    } while (field != 0);

    free(pColors);
    STACKTEXTINPUTNODE* session = pInputStack;
    pColors = 0;
    gpTextInputHead = 0;
    if (session == 0) {
        gfTextInputMode = 0;
        gfEditingText = 0;
        gpActive = 0;
        return;
    }
    gpTextInputHead = session->head;
    pColors = session->pColors;
    pInputStack = session->next;
    free(session);
    for (field = gpTextInputHead; field != 0; field = field->next) {
        if (field->fEnabled == 0) {
            MSYS_EnableRegion(&field->region);
            field->fEnabled = 1;
        }
    }
    field = gpTextInputHead;
    if (gpActive == 0)
        gpActive = gpTextInputHead;
    for (; field != 0; field = field->next) {
        if (field != gpActive || field->ubID != 0 ||
            field->fEnabled == 0)
            continue;
        gpActive = field;
        if (field->szString == 0) {
            gfHiliteMode = 0;
            gfEditingText = 0;
            if (field->InputCallback != 0) field->InputCallback(field->ubID, 1);
        }
        else {
            gubStartHilite = 0;
            gubEndHilite = field->ubStrLen;
            gubCursorPos = field->ubStrLen;
            gubParkingPos = CalculateCursorPos(
                field->region.RegionBottomRightX - field->region.RegionTopLeftX - 10,
                gubCursorPos, field->szString,
                &gsCursorX,
                &guiVisibleCount);
            gubCursorPos = gpActive->ubStrLen;
            gfHiliteMode = 1;
            gfEditingText = 1;
        }
        break;
    }
    if (gpTextInputHead == 0) gpActive = 0;
}

// FUNCTION: WIZ8 0x005D39B0
char AddTextInputField(int left, int top, int width, int height, int priority,
                    const wchar_t* text, unsigned char capacity,
                    short input_type, unsigned char flag)
{
    TEXTINPUTNODE* field =
        (TEXTINPUTNODE*)malloc(sizeof(TEXTINPUTNODE));
    memset(field, 0, sizeof(TEXTINPUTNODE));
    if (gpTextInputHead == 0) {
        gpTextInputHead = field;
        gpTextInputTail = field;
        field->ubID = 0;
    }
    else {
        gpTextInputTail->next = field;
        field->prev = gpTextInputTail;
        field->ubID = gpTextInputTail->ubID + 1;
        gpTextInputTail = field;
    }
    field->usInputType = input_type;
    if (input_type == 0x1002) capacity = 6;
    field->szString = (wchar_t*)malloc((capacity + 1) * sizeof(wchar_t));
    if (text == 0) {
        field->ubStrLen = 0;
        swprintf(field->szString, g_no_target_text);
    }
    else {
        field->ubStrLen = (unsigned char)wcslen(text);
        swprintf(field->szString, text);
    }
    field->ubMaxChars = capacity;
    if (gpTextInputHead == field) {
        gubStartHilite = 0;
        gubEndHilite = field->ubStrLen;
        gubCursorPos = field->ubStrLen;
        if (gpActive != 0) {
            gubParkingPos = CalculateCursorPos(
                gpActive->region.RegionBottomRightX -
                    gpActive->region.RegionTopLeftX - 10,
                gubCursorPos, gpActive->szString,
                &gsCursorX,
                &guiVisibleCount);
        }
        gfHiliteMode = 1;
    }
    field->fEnabled = 1;
    MSYS_DefineRegion(&field->region, (unsigned short)left, (unsigned short)top,
                      (unsigned short)(left + width), (unsigned short)(top + height),
                      (signed char)priority, MSYS_NO_CURSOR,
                      MouseMovedInTextRegionCallback, MouseClickedInTextRegionCallback);
    MSYS_SetRegionUserData(&field->region, 0, field->ubID);
    field->flag_60 = flag;
    return field->ubID;
}

// FUNCTION: WIZ8 0x005D3B40
void RemoveTextInputField(int index)
{
    TEXTINPUTNODE* field = gpTextInputHead;
    while (field != 0 && field->ubID != index) field = field->next;
    if (field == 0) return;
    if (field == gpTextInputHead) gpTextInputHead = field->next;
    if (field == gpTextInputTail) gpTextInputTail = field->prev;
    if (field->next != 0) field->next->prev = field->prev;
    if (field->prev != 0) field->prev->next = field->next;
    if (field->szString != 0) {
        free(field->szString);
        field->szString = 0;
        MSYS_RemoveRegion(&field->region);
    }
    if (field == gpActive) gpActive = 0;
    free(field);
    if (gpTextInputHead == 0) {
        gfTextInputMode = 0;
        gfEditingText = 0;
    }
}

// FUNCTION: WIZ8 0x005D3D00
unsigned char GetTextInputFieldLength(int index)
{
    for (TEXTINPUTNODE* field = gpTextInputHead;
         field != 0; field = field->next) {
        if (field->ubID == index) return field->ubStrLen;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005D3D20
void SetActiveField(char index)
{
    TEXTINPUTNODE* field = gpTextInputHead;
    while (field != 0 &&
           (field == gpActive || field->ubID != index ||
            field->fEnabled == 0)) {
        field = field->next;
    }
    if (field == 0) return;
    gpActive = field;
    if (field->szString != 0) {
        gubStartHilite = 0;
        gubEndHilite = field->ubStrLen;
        gubCursorPos = field->ubStrLen;
        gubParkingPos = CalculateCursorPos(
            field->region.RegionBottomRightX - field->region.RegionTopLeftX - 10,
            gubCursorPos, field->szString,
            &gsCursorX,
            &guiVisibleCount);
        gubCursorPos = gpActive->ubStrLen;
        gfHiliteMode = 1;
        gfEditingText = 1;
        return;
    }
    gfHiliteMode = 0;
    gfEditingText = 0;
    if (field->InputCallback != 0) field->InputCallback(field->ubID, 1);
}

// FUNCTION: WIZ8 0x005D3DF0
void SelectNextField(void)
{
    if (gpActive == 0) return;
    TEXTINPUTNODE* previous = gpActive;
    if (previous->szString == 0) {
        if (previous->InputCallback != 0) previous->InputCallback(previous->ubID, 0);
    }
    else {
        RenderInactiveTextFieldNode(previous);
    }

    bool found = false;
    do {
        gpActive = gpActive->next;
        if (gpActive == 0)
            gpActive = gpTextInputHead;
        if (gpActive->fEnabled != 0) {
            found = true;
            if (gpActive->szString == 0) {
                gfHiliteMode = 0;
                gfEditingText = 0;
                if (gpActive->InputCallback != 0)
                    gpActive->InputCallback(
                        gpActive->ubID, 1);
            }
            else {
                gubStartHilite = 0;
                gubEndHilite = gpActive->ubStrLen;
                gubCursorPos = gpActive->ubStrLen;
                gubParkingPos = CalculateCursorPos(
                    gpActive->region.RegionBottomRightX -
                        gpActive->region.RegionTopLeftX - 10,
                    gubCursorPos, gpActive->szString,
                    &gsCursorX,
                    &guiVisibleCount);
                gfHiliteMode = 1;
                gfEditingText = 1;
            }
        }
        if (gpActive == previous) break;
        if (found) return;
    } while (true);
    gfEditingText = 0;
}

// FUNCTION: WIZ8 0x005D3F00
void ClearActiveField(void)
{
    if (gpActive == 0) return;
    if (gpActive->szString != 0) {
        RenderInactiveTextFieldNode(gpActive);
    }
    else if (gpActive->InputCallback != 0) {
        gpActive->InputCallback(
            gpActive->ubID, 0);
    }
    gfEditingText = 0;
    gpActive = 0;
}

// FUNCTION: WIZ8 0x005D3F50
unsigned int HandleTextInput(const InputAtom* input)
{
    gfHorizontalKey = 0;
    if (gfTextInputMode == 0 || gfEditingText == 0 ||
        gpActive == 0 ||
        (input->usEvent != KEY_DOWN && input->usEvent != KEY_REPEAT) ||
        input->usParam == 0x1b || input->usParam == 0x0d || input->usParam == 9 ||
        (input->usKeyState & ALT_DOWN) != 0 ||
        ((input->usKeyState & CTRL_DOWN) != 0 && input->usParam != 0x2e &&
         input->usParam != 0x27 && input->usParam != 0x25) ||
        (input->usParam > 0x6f && input->usParam < 0x7c)) {
        return 0;
    }

    unsigned char selection_end = gubEndHilite;
    switch (input->usParam) {
    case 8:
        if (gfHiliteMode == 0) {
            if (gubCursorPos != 0) {
                --gubCursorPos;
                gubParkingPos = CalculateCursorPos(
                    gpActive->region.RegionBottomRightX -
                        gpActive->region.RegionTopLeftX - 10,
                    gubCursorPos, gpActive->szString,
                    &gsCursorX,
                    &guiVisibleCount);
                memmove(gpActive->szString + gubCursorPos,
                        gpActive->szString + gubCursorPos + 1,
                        (gpActive->ubStrLen -
                         gubCursorPos) * sizeof(wchar_t));
                --gpActive->ubStrLen;
                return 1;
            }
        }
        else if (gubStartHilite != selection_end) {
            unsigned char first = gubStartHilite;
            unsigned char last = selection_end;
            if (last < first) {
                unsigned char swap = first;
                first = last;
                last = swap;
            }
            memmove(gpActive->szString + first,
                    gpActive->szString + last,
                    (gpActive->ubStrLen - last + 1) * sizeof(wchar_t));
            gpActive->ubStrLen -= last - first;
            gubCursorPos = first;
            gubStartHilite = 0;
            gubParkingPos = CalculateCursorPos(
                gpActive->region.RegionBottomRightX -
                    gpActive->region.RegionTopLeftX - 10,
                first, gpActive->szString,
                &gsCursorX,
                &guiVisibleCount);
            gfHiliteMode = 0;
            return 1;
        }
        break;

    case 0x23: /* End */
        if ((input->usKeyState & SHIFT_DOWN) == 0) {
            gfHiliteMode = 0;
        }
        else if (gfHiliteMode == 0) {
            gfHiliteMode = 1;
            gubStartHilite = gubCursorPos;
        }
        gubCursorPos = gpActive->ubStrLen;
        gubEndHilite = gubCursorPos;
        gubParkingPos = CalculateCursorPos(
            gpActive->region.RegionBottomRightX -
                gpActive->region.RegionTopLeftX - 10,
            gubCursorPos, gpActive->szString,
            &gsCursorX,
            &guiVisibleCount);
        return 1;

    case 0x24: /* Home */
        if ((input->usKeyState & SHIFT_DOWN) == 0) {
            gfHiliteMode = 0;
        }
        else if (gfHiliteMode == 0) {
            gfHiliteMode = 1;
            gubStartHilite = gubCursorPos;
        }
        gubCursorPos = 0;
        gubEndHilite = 0;
        gubParkingPos = CalculateCursorPos(
            gpActive->region.RegionBottomRightX -
                gpActive->region.RegionTopLeftX - 10,
            0, gpActive->szString,
            &gsCursorX,
            &guiVisibleCount);
        return 1;

    case 0x25: /* Left */
        gfHorizontalKey = 1;
        if ((input->usKeyState & SHIFT_DOWN) != 0) {
            if (gfHiliteMode == 0) {
                gfHiliteMode = 1;
                gubStartHilite = gubCursorPos;
            }
            if (gubCursorPos != 0) --gubCursorPos;
            gubEndHilite = gubCursorPos;
        }
        else if (gfHiliteMode != 0) {
            gubCursorPos = gubStartHilite;
            gfHiliteMode = 0;
        }
        else if (gubCursorPos != 0) {
            --gubCursorPos;
        }
        gubParkingPos = CalculateCursorPos(
            gpActive->region.RegionBottomRightX -
                gpActive->region.RegionTopLeftX - 10,
            gubCursorPos, gpActive->szString,
            &gsCursorX,
            &guiVisibleCount);
        return 1;

    case 0x27: /* Right */
        gfHorizontalKey = 1;
        if ((input->usKeyState & SHIFT_DOWN) != 0) {
            if (gfHiliteMode == 0) {
                gfHiliteMode = 1;
                gubStartHilite = gubCursorPos;
            }
            if (gubCursorPos < gpActive->ubStrLen)
                ++gubCursorPos;
            gubEndHilite = gubCursorPos;
        }
        else if (gfHiliteMode != 0) {
            gubCursorPos = selection_end;
            gfHiliteMode = 0;
        }
        else if (gubCursorPos < gpActive->ubStrLen) {
            ++gubCursorPos;
        }
        gubParkingPos = CalculateCursorPos(
            gpActive->region.RegionBottomRightX -
                gpActive->region.RegionTopLeftX - 10,
            gubCursorPos, gpActive->szString,
            &gsCursorX,
            &guiVisibleCount);
        return 1;

    case 0x2e: /* Delete */
        if ((input->usKeyState & CTRL_DOWN) != 0) {
            gpActive->szString[0] = L'\0';
            gpActive->ubStrLen = 0;
            gubCursorPos = 0;
            gubStartHilite = 0;
            gubEndHilite = 0;
            gfHiliteMode = 0;
            SetTextInputCursor(0);
            return 1;
        }
        if (gfHiliteMode != 0 &&
            gubStartHilite != selection_end) {
            unsigned char first = gubStartHilite;
            unsigned char last = selection_end;
            if (last < first) {
                unsigned char swap = first;
                first = last;
                last = swap;
            }
            memmove(gpActive->szString + first,
                    gpActive->szString + last,
                    (gpActive->ubStrLen - last + 1) * sizeof(wchar_t));
            gpActive->ubStrLen -= last - first;
            gubCursorPos = first;
            gfHiliteMode = 0;
            SetTextInputCursor(first);
            return 1;
        }
        if (gubCursorPos < gpActive->ubStrLen) {
            memmove(gpActive->szString + gubCursorPos,
                    gpActive->szString + gubCursorPos + 1,
                    (gpActive->ubStrLen -
                     gubCursorPos) * sizeof(wchar_t));
            --gpActive->ubStrLen;
        }
        SetTextInputCursor(gubCursorPos);
        return 1;

    default:
        break;
    }

    unsigned int character = Function402780((unsigned short)input->usParam,
                                            input->usKeyState);
    if (character == 0) return 1;
    if (character == 0x25 || character == 0x5c) return 0;

    if (gfHiliteMode != 0 &&
        gubStartHilite != gubEndHilite) {
        unsigned char first = gubStartHilite;
        unsigned char last = gubEndHilite;
        if (last < first) {
            unsigned char swap = first;
            first = last;
            last = swap;
        }
        memmove(gpActive->szString + first,
                gpActive->szString + last,
                (gpActive->ubStrLen - last + 1) * sizeof(wchar_t));
        gpActive->ubStrLen -= last - first;
        gubCursorPos = first;
        gfHiliteMode = 0;
    }

    unsigned short input_type = (unsigned short)gpActive->usInputType;
    if (input_type > 0x0fff) {
        HandleExclusiveInput((unsigned short)character);
        return 1;
    }
    if (character == L' ' && (input_type & 4) != 0) {
        AddChar(L' ');
        return 1;
    }
    if (character == L'-' && (input_type & 2) != 0 && gubCursorPos == 0) {
        AddChar(L'-');
        return 1;
    }
    if (character >= L'0' && character <= L'9' && (input_type & 1) != 0) {
        AddChar((unsigned short)character);
        return 1;
    }
    if ((input_type & 2) != 0) {
        if (Function402800((unsigned short)character) != 0) {
            if ((input_type & 0x20) != 0) character = Function4028A0(character);
            AddChar((unsigned short)character);
            return 1;
        }
        if (Function402820((unsigned short)character) != 0) {
            if ((input_type & 0x10) != 0) character = Function402880(character);
            AddChar((unsigned short)character);
            return 1;
        }
    }
    if ((input_type & 8) != 0 && Function402840((unsigned short)character) != 0) {
        AddChar((unsigned short)character);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005D4970
void HandleExclusiveInput(unsigned short character)
{
    short input_type = gpActive->usInputType;
    if (input_type == 0x1000) {
        if (Function402800(character) == 0 && Function402820(character) == 0 &&
            (character < L'0' || character > L'9') && character != L'_' &&
            character != L'.') {
            return;
        }
        if (gubCursorPos == 0 && character >= L'0' && character <= L'9')
            return;
        AddChar(character);
        return;
    }
    if (input_type == 0x1001) {
        if (gubCursorPos == 0) {
            if (Function402820(character) != 0) {
                AddChar(character);
                return;
            }
            if (Function402800(character) == 0) return;
            AddChar((unsigned short)Function4028A0(character));
            return;
        }
        if (character >= L'0' && character <= L'9') AddChar(character);
        return;
    }
    if (input_type != 0x1002) return;
    if (gubCursorPos == 0) {
        if (character >= L'0' && character <= L'2') AddChar(character);
        return;
    }
    if (gubCursorPos == 1) {
        if (character >= L'0' && character <= L'9') {
            if (gpActive->szString[0] != L'2' || character <= L'3')
                AddChar(character);
        }
        if (gpActive->szString[2] == L'\0') {
            AddChar(L':');
            return;
        }
        ++gubCursorPos;
        SetTextInputCursor(gubCursorPos);
        return;
    }
    if (gubCursorPos == 2) {
        if (character == L':') {
            AddChar(L':');
            return;
        }
        if (character < L'0' || character > L'9') return;
        AddChar(L':');
        AddChar(character);
        return;
    }
    if (gubCursorPos == 3) {
        if (character >= L'0' && character <= L'5') AddChar(character);
        return;
    }
    if (gubCursorPos == 4 &&
        character >= L'0' && character <= L'9') {
        AddChar(character);
    }
}

// FUNCTION: WIZ8 0x005D4B70
void AddChar(unsigned short character)
{
    unsigned char length = gpActive->ubStrLen;
    if (gpActive->ubMaxChars <= length) {
        gpActive->ubStrLen = gpActive->ubMaxChars;
        gpActive->szString[
            gpActive->ubStrLen - 1] = character;
        gpActive->szString[
            gpActive->ubStrLen] = L'\0';
        return;
    }
    if (gubCursorPos == length) {
        gpActive->szString[length] = character;
        gpActive->szString[length + 1] = L'\0';
        ++gpActive->ubStrLen;
        gubCursorPos = gpActive->ubStrLen;
    }
    else {
        for (int position = length;
             position >= gubCursorPos; --position) {
            gpActive->szString[position + 1] =
                gpActive->szString[position];
        }
        gpActive->szString[gubCursorPos] = character;
        ++gpActive->ubStrLen;
        ++gubCursorPos;
    }
    gubParkingPos = CalculateCursorPos(
        gpActive->region.RegionBottomRightX -
            gpActive->region.RegionTopLeftX - 10,
        gubCursorPos, gpActive->szString,
        &gsCursorX,
        &guiVisibleCount);
}

// FUNCTION: WIZ8 0x005D4CB0
void MouseMovedInTextRegionCallback(MOUSE_REGION* region, int reason)
{
    if (IsModalOpen()) return;

    int field_index = MSYS_GetRegionUserData(region, 0);
    for (TEXTINPUTNODE* field = gpTextInputHead;
         field != 0; field = field->next) {
        if (field->ubID == field_index && field->blocks_mouse_callback != 0)
            return;
    }

    if ((reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) != 0)
        Function55EE70(Function55EF80());
    if ((reason & MSYS_CALLBACK_REASON_LOST_MOUSE) != 0)
        Function55EE70(-1);

    if (g_flag_6f04ed == 0 || gpActive == 0 ||
        (reason & MSYS_CALLBACK_REASON_MOVE) == 0) {
        return;
    }

    field_index = MSYS_GetRegionUserData(region, 0);
    if (field_index != gpActive->ubID) {
        RenderInactiveTextFieldNode(gpActive);
        for (TEXTINPUTNODE* field = gpTextInputHead;
             field != 0; field = field->next) {
            if (field->ubID == field_index) {
                gubMouseDownPos = 0;
                gubCursorPos = 0;
                gpActive = field;
                gubParkingPos = CalculateCursorPos(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    0, field->szString, &gsCursorX,
                    &guiVisibleCount);
                gfHiliteMode = 0;
                gubStartHilite = 0;
                gubEndHilite = 0;
                break;
            }
        }
    }

    TEXTINPUTNODE* current_field = gpActive;
    if (current_field->szString == 0 || g_flag_6f04ed == 0) return;

    unsigned char position = gubParkingPos;
    int mouse_offset = gusMouseXPos - current_field->region.RegionTopLeftX;
    unsigned int start = gubParkingPos;
    short width = StringPixLengthArg(
        pColors->usFont, 1,
        (unsigned short*)(current_field->szString + start));
    if ((width / 2) / 2 < mouse_offset) {
        int count = 1;
        int previous_width = width / 2;
        do {
            if (current_field->ubStrLen <= position) break;
            ++position;
            ++count;
            width = StringPixLengthArg(
                pColors->usFont, count,
                (unsigned short*)(current_field->szString + start));
            int midpoint = (width - previous_width) / 2 + previous_width;
            previous_width = width;
            if (mouse_offset <= midpoint) break;
        } while (true);
    }

    if (position == gubMouseDownPos) {
        gfHiliteMode = 0;
        return;
    }
    if (gubMouseDownPos < position) {
        gubStartHilite = gubMouseDownPos;
        gubEndHilite = position;
    }
    else {
        gubEndHilite = gubMouseDownPos;
        gubStartHilite = position;
    }
    gfHiliteMode = 1;
    gubCursorPos = position;
    gubParkingPos = CalculateCursorPos(
        current_field->region.RegionBottomRightX -
            current_field->region.RegionTopLeftX - 10,
        position, current_field->szString, &gsCursorX,
        &guiVisibleCount);
}

// FUNCTION: WIZ8 0x005D4F10
void MouseClickedInTextRegionCallback(MOUSE_REGION* region, int reason)
{
    int field_index = MSYS_GetRegionUserData(region, 0);
    if (IsModalOpen()) return;

    for (TEXTINPUTNODE* field = gpTextInputHead;
         field != 0; field = field->next) {
        if (field->ubID == field_index && field->blocks_mouse_callback != 0)
            return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DOUBLECLICK) != 0) {
        if (gpActive != 0) SelectAllText();
        return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) != 0) {
        TEXTINPUTNODE* field = gpActive;
        if (field == 0 || field_index != field->ubID) return;

        unsigned char position = gubParkingPos;
        if (field->szString == 0) {
            position = 0;
        }
        else {
            int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
            unsigned int start = gubParkingPos;
            short width = StringPixLengthArg(
                pColors->usFont, 1,
                (unsigned short*)(field->szString + start));
            if ((width / 2) / 2 < mouse_offset) {
                int count = 1;
                int previous_width = width / 2;
                do {
                    position = (unsigned char)(position + 1);
                    ++count;
                    width = StringPixLengthArg(
                        pColors->usFont, count,
                        (unsigned short*)(field->szString + start));
                    int midpoint = (width - previous_width) / 2 + previous_width;
                    previous_width = width;
                    if (field->ubStrLen <= position || mouse_offset <= midpoint)
                        break;
                } while (true);
            }
        }
        SetTextInputCursor(position);
        gubMouseDownPos = gubCursorPos;
        MSYS_GrabMouse(region);
        return;
    }

    if ((reason & MSYS_CALLBACK_REASON_LBUTTON_UP) == 0) return;
    MSYS_ReleaseMouse(region);

    TEXTINPUTNODE* clicked = gpTextInputHead;
    if (gpActive != 0) {
        if (field_index != gpActive->ubID)
            RenderInactiveTextFieldNode(gpActive);
        clicked = gpTextInputHead;
        if (field_index == gpActive->ubID)
            clicked = gpActive;
    }

    if (clicked != gpActive) {
        while (clicked != 0 && clicked->ubID != field_index)
            clicked = clicked->next;
        if (clicked == 0) return;

        TEXTINPUTNODE* candidate = gpTextInputHead;
        while (candidate != 0 &&
               (candidate == gpActive ||
                candidate->ubID != clicked->ubID ||
                candidate->fEnabled == 0)) {
            candidate = candidate->next;
        }
        if (candidate == 0) return;

        gpActive = candidate;
        if (candidate->szString == 0) {
            gfHiliteMode = 0;
            gfEditingText = 0;
            if (candidate->InputCallback != 0)
                candidate->InputCallback(candidate->ubID, 1);
            return;
        }
        gubStartHilite = 0;
        gubEndHilite = candidate->ubStrLen;
        gubCursorPos = candidate->ubStrLen;
        SetTextInputCursor(gubCursorPos);
        gubCursorPos = candidate->ubStrLen;
        gfHiliteMode = 1;
        gfEditingText = 1;
        return;
    }

    unsigned char position = gubParkingPos;
    if (g_flag_6f04ed != 0) {
        if (gpActive->szString == 0) {
            position = 0;
        }
        else {
            TEXTINPUTNODE* field = gpActive;
            int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
            unsigned int start = gubParkingPos;
            short width = StringPixLengthArg(
                pColors->usFont, 1,
                (unsigned short*)(field->szString + start));
            if ((width / 2) / 2 < mouse_offset) {
                int count = 1;
                int previous_width = width / 2;
                do {
                    if (field->ubStrLen <= position) break;
                    position = (unsigned char)(position + 1);
                    ++count;
                    width = StringPixLengthArg(
                        pColors->usFont, count,
                        (unsigned short*)(field->szString + start));
                    int midpoint = (width - previous_width) / 2 + previous_width;
                    previous_width = width;
                    if (mouse_offset <= midpoint) break;
                } while (true);
            }
        }
        if (position == gubMouseDownPos)
            gfHiliteMode = 0;
        SetTextInputCursor(position);
    }
}

// FUNCTION: WIZ8 0x005D52C0
void RenderBackgroundField(TEXTINPUTNODE* field)
{
    TextInputColors* style = pColors;
    int left = field->region.RegionTopLeftX;
    int top = field->region.RegionTopLeftY;
    int right = field->region.RegionBottomRightX;
    int bottom = field->region.RegionBottomRightY;

    if (style->fBevelling != 0) {
        FillSurfaceRect(-14, left, top, right, bottom, style->usDarkerColor);
        FillSurfaceRect(-14, left + 1, top + 1, right, bottom, style->usBrighterColor);
    }

    unsigned short colour;
    if (field->fEnabled == 0 && style->fUseDisabledAutoShade == 0)
        colour = style->usDisabledTextFieldColor;
    else
        colour = style->usTextFieldColor;
    if (field->flag_60 != 0 && field != gpActive)
        colour = style->usWizardryInactiveColor;

    FillSurfaceRect(-14, left, top, right, bottom, colour);
    MarkScreenRectDirty(left, top, right, bottom, 0);
}

// FUNCTION: WIZ8 0x005D5390
void RenderActiveTextField(void)
{
    TEXTINPUTNODE* field = gpActive;
    if (field == 0 || field->szString == 0) return;

    if (g_flag_6f04ed != 0) {
        if ((int)gusMouseXPos < field->region.RegionTopLeftX) {
            if (gubCursorPos != 0) {
                --gubCursorPos;
                gubParkingPos = CalculateCursorPos(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    gubCursorPos, field->szString,
                    &gsCursorX,
                    &guiVisibleCount);
            }
            if (gfHiliteMode != 0)
                gubStartHilite =
                    gubVisibleStart;
        }
        else if (field->region.RegionBottomRightX < (int)gusMouseXPos) {
            if (gubCursorPos < field->ubStrLen) {
                ++gubCursorPos;
                gubParkingPos = CalculateCursorPos(
                    field->region.RegionBottomRightX -
                        field->region.RegionTopLeftX - 10,
                    gubCursorPos, field->szString,
                    &gsCursorX,
                    &guiVisibleCount);
            }
            if (gfHiliteMode != 0)
                gubEndHilite =
                    (unsigned char)(guiVisibleCount +
                                    gubVisibleStart);
        }
    }

    SaveFontSettings();
    SetFont(pColors->usFont);
    unsigned short font_height =
        GetFontHeight(pColors->usFont);
    unsigned int vertical_offset =
        (field->region.RegionBottomRightY - field->region.RegionTopLeftY -
         font_height) / 2;
    RenderBackgroundField(field);

    wchar_t escaped[256];
    wchar_t visible[512];
    int escaped_length = 0;
    for (const wchar_t* source = field->szString; *source != L'\0'; ++source) {
        if (*source == L'%') escaped[escaped_length++] = L'%';
        escaped[escaped_length++] = *source;
    }
    escaped[escaped_length] = L'\0';
    wcscpy(visible, escaped + gubParkingPos);

    bool has_selection =
        gfHiliteMode != 0 &&
        gubStartHilite != gubEndHilite;
    unsigned char selection_first = gubEndHilite;
    unsigned char selection_last = gubStartHilite;
    if (gubStartHilite < gubEndHilite) {
        selection_first = gubStartHilite;
        selection_last = gubEndHilite;
    }

    for (size_t index = 0; index < guiVisibleCount;
         ++index) {
        short prefix = StringPixLengthArg(
            pColors->usFont, index,
            (unsigned short*)visible);
        if (has_selection &&
            (int)(selection_first - gubParkingPos) <=
                (int)index &&
            (int)index <
                (int)(selection_last - gubParkingPos)) {
            SetFontForeground(pColors->ubHiForeColor);
            SetFontBackground(pColors->ubHiShadowColor);
            SetFontShadow(pColors->ubHiBackColor);
        }
        else {
            SetFontForeground(pColors->ubForeColor);
            SetFontBackground(pColors->ubShadowColor);
            SetFontShadow(0);
        }
        if (visible[index] == L'%') {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%%");
        }
        else {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%c",
                    visible[index]);
        }
    }

    if (gfEditingText != 0 && field->szString != 0 && g_flag_6f04ed == 0 &&
        GetTickCount() % 1000 < 500) {
        int left = field->region.RegionTopLeftX +
                   gsCursorX;
        int top = field->region.RegionTopLeftY + vertical_offset;
        FillSurfaceRect(-14, left, top, left + 1, top + font_height,
                        pColors->usCursorColor);
    }
    RestoreFontSettings();
}

// FUNCTION: WIZ8 0x005D5770
void RenderInactiveTextFieldNode(TEXTINPUTNODE* field)
{
    if (field == 0 || field->szString == 0) return;

    SaveFontSettings();
    SetFont(pColors->usFont);
    bool disabled = field->fEnabled == 0 &&
                    pColors->fUseDisabledAutoShade != 0;
    if (disabled) {
        SetFontForeground(pColors->ubDisabledForeColor);
        SetFontBackground(pColors->ubDisabledShadowColor);
    }
    else {
        SetFontForeground(pColors->ubForeColor);
        SetFontBackground(pColors->ubShadowColor);
    }
    unsigned short font_height =
        GetFontHeight(pColors->usFont);
    unsigned int vertical_offset =
        (field->region.RegionBottomRightY - field->region.RegionTopLeftY -
         font_height) / 2;
    SetFontShadow(0);
    RenderBackgroundField(field);

    wchar_t escaped[256];
    int escaped_length = 0;
    for (const wchar_t* source = field->szString; *source != L'\0'; ++source) {
        if (*source == L'%') escaped[escaped_length++] = L'%';
        escaped[escaped_length++] = *source;
    }
    escaped[escaped_length] = L'\0';

    for (size_t index = 0; index < wcslen(escaped); ++index) {
        short prefix = StringPixLengthArg(
            pColors->usFont, index,
            (unsigned short*)escaped);
        if (field->region.RegionBottomRightX -
                field->region.RegionTopLeftX - 10 < prefix + 3) {
            break;
        }
        if (escaped[index] == L'%') {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%%");
        }
        else {
            mprintf(field->region.RegionTopLeftX + prefix + 3,
                    field->region.RegionTopLeftY + vertical_offset, L"%c",
                    escaped[index]);
        }
    }
    RestoreFontSettings();

    if (disabled) {
        int rectangle[4] = {
            field->region.RegionTopLeftX,
            field->region.RegionTopLeftY,
            field->region.RegionBottomRightX,
            field->region.RegionBottomRightY,
        };
        unsigned int pitch;
        void* pixels = Function402B90(-14, &pitch);
        Function4124A0(pixels, pitch, rectangle);
        Function402C30(-14);
    }
}

// FUNCTION: WIZ8 0x005D59A0
void RenderAllTextFields(void)
{
    for (STACKTEXTINPUTNODE* session =
             pInputStack;
         session != 0; session = session->next) {
        for (TEXTINPUTNODE* field = session->head;
             field != 0; field = field->next) {
            RenderInactiveTextFieldNode(field);
        }
    }
    for (TEXTINPUTNODE* field = gpTextInputHead;
         field != 0; field = field->next) {
        if (field == gpActive)
            RenderActiveTextField();
        else
            RenderInactiveTextFieldNode(field);
    }
}

// FUNCTION: WIZ8 0x005D5A00
unsigned char EditingText(void)
{
    return gfEditingText;
}

// FUNCTION: WIZ8 0x005D5A10
unsigned int CalculateCursorPos(int width, int cursor, const wchar_t* text,
                           int* cursor_width, size_t* visible_count)
{
    wchar_t buffer[512];
    if (cursor < gubVisibleStart)
        gubVisibleStart = (unsigned char)cursor;

    unsigned int start = gubVisibleStart;
    wcscpy(buffer, text + start);
    buffer[cursor - start] = L'\0';
    int measured = StringPixLength((unsigned short*)buffer,
                                   pColors->usFont);
    size_t count = wcslen(buffer);
    unsigned char retained_start;

    if (width < measured) {
        wchar_t* suffix = buffer;
        do {
            ++suffix;
            ++start;
            measured = StringPixLength((unsigned short*)suffix,
                                       pColors->usFont);
        } while (width < measured);
        retained_start = (unsigned char)start;

        if (gubVisibleStart < start) {
            wcscpy(buffer, text + start);
            size_t length = wcslen(buffer);
            count = length;
            for (size_t index = 0; index < wcslen(buffer); ++index) {
                short prefix = StringPixLengthArg(pColors->usFont,
                                                  index, (unsigned short*)buffer);
                count = index;
                if (width < prefix + 3) break;
                count = length;
            }
        }
    }
    else {
        wcscpy(buffer, text + start);
        size_t length = wcslen(buffer);
        count = length;
        retained_start = gubVisibleStart;
        for (size_t index = 0; index < wcslen(buffer); ++index) {
            short prefix = StringPixLengthArg(pColors->usFont,
                                              index, (unsigned short*)buffer);
            retained_start = gubVisibleStart;
            count = index;
            if (width < prefix + 3) break;
            count = length;
        }
    }

    gubVisibleStart = retained_start;
    *cursor_width = measured + 2;
    *visible_count = count;
    return start;
}

// FUNCTION: WIZ8 0x005D5BF0
void SetTextInputCursor(unsigned char cursor)
{
    gubCursorPos = cursor;
    if (gpActive != 0) {
        gubParkingPos = CalculateCursorPos(
            gpActive->region.RegionBottomRightX -
                gpActive->region.RegionTopLeftX - 10,
            cursor, gpActive->szString,
            &gsCursorX,
            &guiVisibleCount);
    }
}

// FUNCTION: WIZ8 0x005D5C40
void SelectAllText(void)
{
    TEXTINPUTNODE* field = gpActive;
    unsigned char position = gubParkingPos;
    if (field->szString == 0) {
        position = 0;
    }
    else {
        int mouse_offset = gusMouseXPos - field->region.RegionTopLeftX;
        unsigned int start = gubParkingPos;
        short width = StringPixLengthArg(
            pColors->usFont, 1,
            (unsigned short*)(field->szString + start));
        if ((width / 2) / 2 < mouse_offset) {
            int count = 1;
            int previous_width = width / 2;
            do {
                if (field->ubStrLen <= position) break;
                ++position;
                ++count;
                width = StringPixLengthArg(
                    pColors->usFont, count,
                    (unsigned short*)(field->szString + start));
                int midpoint = (width - previous_width) / 2 + previous_width;
                previous_width = width;
                if (mouse_offset <= midpoint) break;
            } while (true);
        }
    }

    if (field->szString[position] == L' ') return;
    unsigned char first = 0;
    if (position != 0) {
        unsigned int scan = position;
        const wchar_t* character = field->szString + position;
        do {
            if (*character == L' ') {
                first = (unsigned char)(scan + 1);
                break;
            }
            --scan;
            --character;
        } while (scan != 0);
    }

    unsigned char last = (unsigned char)wcslen(field->szString);
    for (unsigned int scan = position + 1; scan < wcslen(field->szString); ++scan) {
        if (field->szString[scan] == L' ') {
            last = (unsigned char)scan;
            break;
        }
    }
    gubStartHilite = first;
    gubEndHilite = last;
    gfHiliteMode = 1;
}
