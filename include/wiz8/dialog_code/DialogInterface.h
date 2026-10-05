#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/engine_code/Video2.h"
#include "input.h"
#include "mousesystem_macros.h"

/* Standard dialog button events: held left clicks repeat the press. Numeric
   dialogs forward their distinct repeat events before using this dispatcher. */
inline bool DispatchDialogMouseInput(unsigned short event, unsigned short x, unsigned short y)
{
    switch (event) {
    case LEFT_BUTTON_REPEAT:
        event = LEFT_BUTTON_DOWN;
        break;
    case LEFT_BUTTON_DOWN:
    case LEFT_BUTTON_UP:
    case RIGHT_BUTTON_DOWN:
    case RIGHT_BUTTON_UP:
        break;
    default:
        return false;
    }
    MSYS_SGP_Mouse_Handler_Hook(event, x, y, gfLeftButtonState, gfRightButtonState);
    return true;
}

/* Numeric split dialogs release the active field before forwarding button-up,
   and forward repeat events unchanged. The field reference remains live across
   callbacks; keyboard handling stays with the concrete dialog. */
template <class Dialog, class Field>
void ProcessNumericDialogInputEvents(Dialog* dialog, Field*& active_field,
                                     bool (Dialog::*handle_event)(const InputAtom*))
{
    POINT mouse;
    InputAtom input;

    SGPMouseGetPos(&mouse);
    MSYS_SGP_Mouse_Handler_Hook(MOUSE_POS, mouse.x, mouse.y, gfLeftButtonState, gfRightButtonState);
    while (DequeueEvent(&input) == 1) {
        if (input.usEvent == LEFT_BUTTON_UP && active_field != 0) {
            active_field->SetActive(false);
        }
        if (input.usEvent == LEFT_BUTTON_REPEAT || input.usEvent == RIGHT_BUTTON_REPEAT) {
            MSYS_SGP_Mouse_Handler_Hook(input.usEvent, mouse.x, mouse.y, gfLeftButtonState,
                                        gfRightButtonState);
        } else if (!DispatchDialogMouseInput(input.usEvent, mouse.x, mouse.y)) {
            (dialog->*handle_event)(&input);
        }
    }
}

class W8MessageDialogBase;
struct W8Character;
class W8MessageDialogBase;

/* ConfigureDialogFont writes these four; every dialog draw/text path reads
   them. Declared here because this unit owns their definitions. */
extern int g_dialog_interface_font;
extern BOOLEAN g_dialog_font_enabled;
extern unsigned char g_dialog_font_foreground;
extern unsigned char g_dialog_font_background;
void ConfigureDialogFont(int font, BOOLEAN enabled, unsigned char foreground,
                         unsigned char background);
W8DialogBase* CreateCharacterSummaryDialog(W8Character* character);
W8DialogBase* CreateDialogByKind(W8DialogKind kind);
bool GetDialogResult(W8DialogBase* dialog);
void SetDialogPrompt(W8MessageDialogBase* dialog, wchar_t* text, int, int);
void DrawDialog(W8DialogBase* dialog);
bool ProcessDialogInput(W8DialogBase* dialog);
void SetDialogDestroyCallback(W8DialogBase* dialog, W8DialogDestroyCallback callback);
/* The shared empty wide string dialogs hand to SetText; defined in
   OptionsScreen.cpp, referenced across dialog and screen TUs. */
