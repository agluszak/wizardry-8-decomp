/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Reconstruct Wizardry physical-key mapping, raw-key string input, and character helpers.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Recover the wide-character predicate return width and key translation modulo.
   Remove the unused local configuration include and use the SDK mouse-wheel header, 2026-10-04.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include <windows.h>
#include <zmouse.h>
#include <stdio.h>
#include <memory.h>
#include "debug.h"
#include "input.h"
#include "memman.h"
#include "english.h"
#include "video2.h"

// Make sure to refer to the translation table which is within one of the following files (depending
// on the language used). ENGLISH.C, JAPANESE.C, FRENCH.C, GERMAN.C, SPANISH.C, etc...

#include "sgp.h"

#undef GetCursorPos
#define GetCursorPos SGPMouseGetPos

// The gfKeyState table is used to track which of the keys is up or down at any one time. This is used while polling
// the interface.

// GLOBAL: WIZ8 0x006f0520
BOOLEAN gfKeyState[256]; // TRUE = Pressed, FALSE = Not Pressed
// GLOBAL: WIZ8 0x00650db8
BOOLEAN fCursorWasClipped = FALSE;
RECT gCursorClipRect;

// The gsKeyTranslationTables basically translates scan codes to our own key value table. Please note that the table is 2 bytes
// wide per entry. This will be used since we will use 2 byte characters for translation purposes.

// GLOBAL: WIZ8 0x006f04ea
UINT16 gfShiftState; // TRUE = Pressed, FALSE = Not Pressed
// GLOBAL: WIZ8 0x006f051c
UINT16 gfAltState; // TRUE = Pressed, FALSE = Not Pressed
// GLOBAL: WIZ8 0x006f0508
UINT16 gfCtrlState; // TRUE = Pressed, FALSE = Not Pressed

// These data structure are used to track the mouse while polling

// GLOBAL: WIZ8 0x006f04f4
BOOLEAN gfTrackDblClick;
// GLOBAL: WIZ8 0x006f04e4
UINT32 guiDoubleClkDelay; // Current delay in milliseconds for a delay
// GLOBAL: WIZ8 0x006f0504
UINT32 guiSingleClickTimer;
UINT32 guiRecordedWParam;
UINT32 guiRecordedLParam;
// GLOBAL: WIZ8 0x006f0514
UINT16 gusRecordedKeyState;
// GLOBAL: WIZ8 0x006f04e9
BOOLEAN gfRecordedLeftButtonUp;

// GLOBAL: WIZ8 0x006f0518
UINT32 guiLeftButtonRepeatTimer;
// GLOBAL: WIZ8 0x006f04f0
UINT32 guiRightButtonRepeatTimer;

// GLOBAL: WIZ8 0x006f04ec
BOOLEAN gfTrackMousePos; // TRUE = queue mouse movement events, FALSE = don't
// GLOBAL: WIZ8 0x006f04ed
BOOLEAN gfLeftButtonState; // TRUE = Pressed, FALSE = Not Pressed
// GLOBAL: WIZ8 0x006f04e8
BOOLEAN gfRightButtonState; // TRUE = Pressed, FALSE = Not Pressed
// GLOBAL: WIZ8 0x006f050a
UINT16 gusMouseXPos; // X position of the mouse on screen
// GLOBAL: WIZ8 0x006f04f8
UINT16 gusMouseYPos; // y position of the mouse on screen

// The queue structures are used to track input events using queued events

// GLOBAL: WIZ8 0x006ef4e0
InputAtom gEventQueue[256];
// GLOBAL: WIZ8 0x006f04f6
UINT16 gusQueueCount;
// GLOBAL: WIZ8 0x006f04e2
UINT16 gusHeadIndex;
// GLOBAL: WIZ8 0x006f04e0
UINT16 gusTailIndex;

// ATE: Added to signal if we have had input this frame - cleared by the SGP main loop
// GLOBAL: WIZ8 0x00650db9
BOOLEAN gfSGPInputReceived = FALSE;

// This is the WIN95 hook specific data and defines used to handle the keyboard and
// mouse hook

// GLOBAL: WIZ8 0x006f04fc
HHOOK ghKeyboardHook;
// GLOBAL: WIZ8 0x006f050c
HHOOK ghMouseHook;

// If the following pointer is non NULL then input characters are redirected to
// the related string

// GLOBAL: WIZ8 0x006f0500
BOOLEAN gfCurrentStringInputState;
// GLOBAL: WIZ8 0x006f0510
StringInput* gpCurrentStringDescriptor;

// Local function headers

void QueueEvent(UINT16 ubInputEvent, UINT32 usParam, UINT32 uiParam);
void RedirectToString(UINT16 uiInputCharacter);
void HandleSingleClicksAndButtonRepeats(void);
void AdjustMouseForWindowOrigin(void);

// These are the hook functions for both keyboard and mouse

// FUNCTION: WIZ8 0x00401b30
LRESULT CALLBACK KeyboardHandler(int Code, WPARAM wParam, LPARAM lParam)
{
    if ((Code < 0) ||
        (!gfApplicationActive)) { // Do not handle this message, pass it on to another window
        return CallNextHookEx(ghKeyboardHook, Code, wParam, lParam);
    }

    if (lParam & TRANSITION_MASK) { // The key has been released
        KeyUp(wParam, lParam);
        //gfSGPInputReceived =  TRUE;
    } else { // Key was up
        KeyDown(wParam, lParam);
        gfSGPInputReceived = TRUE;
    }

    return TRUE;
}

// Wizardry mouse hander

// FUNCTION: WIZ8 0x00401c70
LRESULT CALLBACK MouseHandler(int Code, WPARAM wParam, LPARAM lParam)
{
    UINT32 uiParam;
    UINT32 uiXPos, uiYPos;
    RECT rcClient;
    BOOLEAN fOutsideClient = FALSE;
    // GLOBAL: WIZ8 0x00650dba
    static BOOLEAN fResizing = FALSE;
    LRESULT Result;

    uiXPos = (((MOUSEHOOKSTRUCT*)lParam)->pt).x;
    uiYPos = (((MOUSEHOOKSTRUCT*)lParam)->pt).y;

    if (!VideoIsFullScreen()) {
        if (wParam == WM_NCLBUTTONDOWN)
            fResizing = TRUE;

        VideoGetClientRect(&rcClient);
        if ((uiXPos < (UINT32)rcClient.left) || (uiXPos > (UINT32)rcClient.right) ||
            (uiYPos < (UINT32)rcClient.top) || (uiYPos > (UINT32)rcClient.bottom))
            fOutsideClient = TRUE;
    }

    if ((Code < 0) || (!gfApplicationActive) || fOutsideClient ||
        fResizing) { // Do not handle this message, pass it on to another window
        Result = CallNextHookEx(ghMouseHook, Code, wParam, lParam);

        if ((wParam == WM_LBUTTONUP) || (wParam == WM_NCLBUTTONUP))
            fResizing = FALSE;

        return (Result);
    }

    switch (wParam) {
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
        if (VideoIsFullScreen()) {
            gusMouseXPos = (UINT16)(uiXPos);
            gusMouseYPos = (UINT16)(uiYPos);
        } else {
            gusMouseXPos = (UINT16)(uiXPos - rcClient.left);
            gusMouseYPos = (UINT16)(uiYPos - rcClient.top);
        }
        uiParam = (UINT32)gusMouseYPos << 16 | (UINT32)gusMouseXPos;
        //Set that we have input
        gfSGPInputReceived = TRUE;
        break;
    }

    if (wParam == WM_MOUSEWHEEL) {
        return (FALSE);
    }

    switch (wParam) {
    case WM_LBUTTONDOWN:
        gfLeftButtonState = TRUE;
        QueueEvent(LEFT_BUTTON_DOWN, 0, uiParam);
        break;
    case WM_LBUTTONUP:
        gfLeftButtonState = FALSE;
        QueueEvent(LEFT_BUTTON_UP, 0, uiParam);
        break;
    case WM_RBUTTONDOWN:
        gfRightButtonState = TRUE;
        QueueEvent(RIGHT_BUTTON_DOWN, 0, uiParam);
        break;
    case WM_RBUTTONUP:
        gfRightButtonState = FALSE;
        QueueEvent(RIGHT_BUTTON_UP, 0, uiParam);
        break;
    case WM_MOUSEMOVE:
        if (gfTrackMousePos)
            QueueEvent(MOUSE_POS, 0, uiParam);
        break;
    }

    return (TRUE);
}

// FUNCTION: WIZ8 0x00401ea0
BOOLEAN InitializeInputManager(void)
{
    // Link to debugger
    RegisterDebugTopic(TOPIC_INPUT, "Input Manager");
    // Initialize the gfKeyState table to FALSE everywhere
    memset(gfKeyState, FALSE, 256);
    // Initialize the Event Queue
    gusQueueCount = 0;
    gusHeadIndex = 0;
    gusTailIndex = 0;
    // By default, we will not queue mousemove events
    gfTrackMousePos = FALSE;
    // Initialize other variables
    gfShiftState = FALSE;
    gfAltState = FALSE;
    gfCtrlState = FALSE;
    // Initialize variables pertaining to DOUBLE CLIK stuff
    gfTrackDblClick = TRUE;
    guiDoubleClkDelay = DBL_CLK_TIME;
    guiSingleClickTimer = 0;
    gfRecordedLeftButtonUp = FALSE;
    // Initialize variables pertaining to the button states
    gfLeftButtonState = FALSE;
    gfRightButtonState = FALSE;
    // Initialize variables pertaining to the repeat mechanism
    guiLeftButtonRepeatTimer = 0;
    guiRightButtonRepeatTimer = 0;
    // Set the mouse to the center of the screen
    gusMouseXPos = 320;
    gusMouseYPos = 240;
    // Initialize the string input mechanism
    gfCurrentStringInputState = FALSE;
    gpCurrentStringDescriptor = NULL;
    // Activate the hook functions for both keyboard and Mouse
    ghKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardHandler, (HINSTANCE)0,
                                      GetCurrentThreadId());
    DbgMessage(TOPIC_INPUT, DBG_LEVEL_2, String("Set keyboard hook returned %d", ghKeyboardHook));

    ghMouseHook =
        SetWindowsHookEx(WH_MOUSE, (HOOKPROC)MouseHandler, (HINSTANCE)0, GetCurrentThreadId());
    DbgMessage(TOPIC_INPUT, DBG_LEVEL_2, String("Set mouse hook returned %d", ghMouseHook));
    return TRUE;
}

// FUNCTION: WIZ8 0x00401f70
void ShutdownInputManager(void)
{ // There's very little to do when shutting down the input manager. In the future, this is where the keyboard and
    // mouse hooks will be destroyed
    UnRegisterDebugTopic(TOPIC_INPUT, "Input Manager");
    UnhookWindowsHookEx(ghKeyboardHook);
    UnhookWindowsHookEx(ghMouseHook);
}

// FUNCTION: WIZ8 0x00401f90
void QueueEvent(UINT16 ubInputEvent, UINT32 usParam, UINT32 uiParam)
{
    UINT32 uiTimer;
    UINT16 usKeyState;

    uiTimer = GetTickCount();
    usKeyState = gfShiftState | gfCtrlState | gfAltState;

    // Can we queue up one more event, if not, the event is lost forever
    if (gusQueueCount == 256) { // No more queue space
        return;
    }

    if (ubInputEvent == LEFT_BUTTON_DOWN) {
        guiLeftButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIMEOUT;
    }

    if (ubInputEvent == RIGHT_BUTTON_DOWN) {
        guiRightButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIMEOUT;
    }

    if (ubInputEvent == LEFT_BUTTON_UP) {
        guiLeftButtonRepeatTimer = 0;
    }

    if (ubInputEvent == RIGHT_BUTTON_UP) {
        guiRightButtonRepeatTimer = 0;
    }

    if ((ubInputEvent == LEFT_BUTTON_UP)) {
        // Do we have a double click
        if ((uiTimer - guiSingleClickTimer) < DBL_CLK_TIME) {
            guiSingleClickTimer = 0;

            // Add a button up first...
            gEventQueue[gusTailIndex].uiTimeStamp = uiTimer;
            gEventQueue[gusTailIndex].usKeyState = gusRecordedKeyState;
            gEventQueue[gusTailIndex].usEvent = LEFT_BUTTON_UP;
            gEventQueue[gusTailIndex].usParam = usParam;
            gEventQueue[gusTailIndex].uiParam = uiParam;

            // Increment the number of items on the input queue
            gusQueueCount++;

            // Increment the gusTailIndex pointer
            if (gusTailIndex == 255) { // The gusTailIndex is about to wrap around the queue ring
                gusTailIndex = 0;
            } else { // We simply increment the gusTailIndex
                gusTailIndex++;
            }

            // Now do double click
            gEventQueue[gusTailIndex].uiTimeStamp = uiTimer;
            gEventQueue[gusTailIndex].usKeyState = gusRecordedKeyState;
            gEventQueue[gusTailIndex].usEvent = LEFT_BUTTON_DBL_CLK;
            gEventQueue[gusTailIndex].usParam = usParam;
            gEventQueue[gusTailIndex].uiParam = uiParam;

            // Increment the number of items on the input queue
            gusQueueCount++;

            // Increment the gusTailIndex pointer
            if (gusTailIndex == 255) { // The gusTailIndex is about to wrap around the queue ring
                gusTailIndex = 0;
            } else { // We simply increment the gusTailIndex
                gusTailIndex++;
            }

            return;
        } else {
            // Save time
            guiSingleClickTimer = uiTimer;
        }
    }

    // Okey Dokey, we can queue up the event, so we do it
    gEventQueue[gusTailIndex].uiTimeStamp = uiTimer;
    gEventQueue[gusTailIndex].usKeyState = usKeyState;
    gEventQueue[gusTailIndex].usEvent = ubInputEvent;
    gEventQueue[gusTailIndex].usParam = usParam;
    gEventQueue[gusTailIndex].uiParam = uiParam;

    // Increment the number of items on the input queue
    gusQueueCount++;

    // Increment the gusTailIndex pointer
    if (gusTailIndex == 255) { // The gusTailIndex is about to wrap around the queue ring
        gusTailIndex = 0;
    } else { // We simply increment the gusTailIndex
        gusTailIndex++;
    }
}

// FUNCTION: WIZ8 0x00402140
BOOLEAN DequeueEvent(InputAtom* Event)
{
    HandleSingleClicksAndButtonRepeats();

    // Is there an event to dequeue
    if (gusQueueCount > 0) {
        // We have an event, so we dequeue it
        memcpy(Event, &(gEventQueue[gusHeadIndex]), sizeof(InputAtom));

        if (gusHeadIndex == 255) {
            gusHeadIndex = 0;
        } else {
            gusHeadIndex++;
        }

        // Decrement the number of items on the input queue
        gusQueueCount--;

        // dequeued an event, return TRUE
        return TRUE;
    } else {
        // No events to dequeue, return FALSE
        return FALSE;
    }
}

// GLOBAL: WIZ8 0x005ff51c
unsigned short g_key_remap_5ff51c[14] = {0x0069, 0x0063, 0x0061, 0x0067, 0x0064, 0x0068, 0x0066,
                                         0x0062, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x006e};

// FUNCTION: WIZ8 0x00402270
void KeyChange(UINT32 key, UINT32 flags, UINT8 pressed)
{
    POINT point;
    unsigned int packed;
    unsigned int code;

    if (key == 0x0c) {
        key = 0x65;
    } else if (key < 0x2f && key > 0x20 && (flags & 0x1000000) == 0) {
        key = g_key_remap_5ff51c[key - 0x21];
    } else if (key == 0x0d && (flags & 0x1000000) != 0) {
        key = 0x6c;
    }
    SGPMouseGetPos(&point);
    packed = ((unsigned int)point.y << 0x10) | ((unsigned int)point.x & 0xffff);
    if (pressed == 1) {
        code = key & 0xffff;
        if (gfKeyState[code] == 0) {
            if (gfCurrentStringInputState == 0) {
                gfKeyState[code] = 1;
                QueueEvent(1, code, packed);
                return;
            }
        } else if (gfCurrentStringInputState == 0) {
            QueueEvent(4, code, packed);
            return;
        }
        RedirectToString((unsigned short)key);
        return;
    }
    code = key & 0xffff;
    if (gfKeyState[code] == 1) {
        gfKeyState[code] = 0;
        QueueEvent(2, code, packed);
        return;
    }
    if ((short)key == 9 && gfAltState != 0) {
        ShowWindow(ghWindow, 6);
        gfKeyState[0x12] = 0;
        gfAltState = 0;
    }
}

void KeyDown(UINT32 usParam, UINT32 uiParam)
{                        // Are we PRESSING down one of SHIFT, ALT or CTRL ???
    if (usParam == 16) { // SHIFT key is PRESSED
        gfShiftState = SHIFT_DOWN;
        gfKeyState[16] = TRUE;
    } else {
        if (usParam == 17) { // CTRL key is PRESSED
            gfCtrlState = CTRL_DOWN;
            gfKeyState[17] = TRUE;
        } else {
            if (usParam == 18) { // ALT key is pressed
                gfAltState = ALT_DOWN;
                gfKeyState[18] = TRUE;
            } else {
                if (usParam == SNAPSHOT) {
                    //PrintScreen();
                    // DB Done in the KeyUp function
                    // this used to be keyed to SCRL_LOCK
                    // which I believe Luis gave the wrong value
                } else {
                    // No special keys have been pressed
                    // Call KeyChange() and pass TRUE to indicate key has been PRESSED and not RELEASED
                    KeyChange(usParam, uiParam, TRUE);
                }
            }
        }
    }
}

void KeyUp(UINT32 usParam, UINT32 uiParam)
{                        // Are we RELEASING one of SHIFT, ALT or CTRL ???
    if (usParam == 16) { // SHIFT key is RELEASED
        gfShiftState = FALSE;
        gfKeyState[16] = FALSE;
    } else {
        if (usParam == 17) { // CTRL key is RELEASED
            gfCtrlState = FALSE;
            gfKeyState[17] = FALSE;
        } else {
            if (usParam == 18) { // ALT key is RELEASED
                gfAltState = FALSE;
                gfKeyState[18] = FALSE;
            } else {
                if (usParam == SNAPSHOT) {
                    // DB this used to be keyed to SCRL_LOCK
                    // which I believe Luis gave the wrong value
                    //#ifndef JA2
                    if (_KeyDown(CTRL))
                        VideoCaptureToggle();
                    else
                        //#endif
                        PrintScreen();
                } else {
                    // No special keys have been pressed
                    // Call KeyChange() and pass FALSE to indicate key has been PRESSED and not RELEASED
                    KeyChange(usParam, uiParam, FALSE);
                }
            }
        }
    }
}

// These functions will be used for string input

// Since all string input will have to be handle by reentrant capable functions (since we must attend
// to windows messaging as well as network traffic related issues), whenever there is ongoing string input
// going on, we must use InitStringInput() and HandleStringInput() to get the job done. HandleStringInput()
// will return TRUE as long as the string input is going on, and FALSE when its done
//
// During string input, all keyboard are rerouted to the string and hence are not queued up on the
// event queue or registered in the state table. Also note that several string inputs can occur
// at the same time. Use the SetStringFocus() function to manager the focus for multiple
// string inputs

BOOLEAN CharacterIsValid(UINT16 usCharacter, UINT16* pFilter)
{
    UINT32 uiIndex, uiEndIndex;

    if (pFilter != NULL) {
        uiEndIndex = *pFilter;
        for (uiIndex = 1; uiIndex <= *pFilter; uiIndex++) {
            if (usCharacter == *(pFilter + uiIndex)) {
                return TRUE;
            }
        }
        return FALSE;
    }

    return TRUE;
}

// FUNCTION: WIZ8 0x004023b0
void RedirectToString(UINT16 usInputCharacter)
{
    UINT16 usIndex;

    if (gpCurrentStringDescriptor != NULL) {
        // Handle the new character input
        switch (usInputCharacter) {
        case ENTER: // ENTER is pressed, the last character field should be set to ENTER
            if (gpCurrentStringDescriptor->pNextString != NULL) {
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor = gpCurrentStringDescriptor->pNextString;
                gpCurrentStringDescriptor->fFocus = TRUE;
                gpCurrentStringDescriptor->usLastCharacter = 0;
            } else {
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
                gfCurrentStringInputState = FALSE;
            }
            break;
        case ESC: // ESC was pressed, the last character field should be set to ESC
            gpCurrentStringDescriptor->fFocus = FALSE;
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            gfCurrentStringInputState = FALSE;
            break;
        case TAB:
            if (gfShiftState) {
                if (gpCurrentStringDescriptor->pPreviousString == NULL)
                    return;
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor = gpCurrentStringDescriptor->pPreviousString;
            } else {
                if (gpCurrentStringDescriptor->pNextString == NULL)
                    return;
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor = gpCurrentStringDescriptor->pNextString;
            }
            gpCurrentStringDescriptor->fFocus = TRUE;
            gpCurrentStringDescriptor->usLastCharacter = 0;
            break;
        case 0x26: // The UPARROW was pressed, the last character field should be set to UPARROW
            if (gpCurrentStringDescriptor->pPreviousString != NULL) {
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor = gpCurrentStringDescriptor->pPreviousString;
                gpCurrentStringDescriptor->fFocus = TRUE;
                gpCurrentStringDescriptor->usLastCharacter = 0;
            }
            break;
        case 0x28: // The DNARROW was pressed, the last character field should be set to DNARROW
            if (gpCurrentStringDescriptor->pNextString != NULL) {
                gpCurrentStringDescriptor->fFocus = FALSE;
                gpCurrentStringDescriptor = gpCurrentStringDescriptor->pNextString;
                gpCurrentStringDescriptor->fFocus = TRUE;
                gpCurrentStringDescriptor->usLastCharacter = 0;
            }
            break;
        case 0x25: // The LEFTARROW was pressed, move one character to the left
            if (gpCurrentStringDescriptor->usStringOffset > 0) { // Decrement the offset
                gpCurrentStringDescriptor->usStringOffset--;
            }
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
        case 0x27: // The RIGHTARROW was pressed, move one character to the right
            if (gpCurrentStringDescriptor->usStringOffset <
                gpCurrentStringDescriptor
                    ->usCurrentStringLength) { // Ok we can move the cursor one up without going past the end of string
                gpCurrentStringDescriptor->usStringOffset++;
            }
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
        case BACKSPACE: // Delete the character preceding the cursor
            if (gpCurrentStringDescriptor->usStringOffset >
                0) { // Ok, we are not at the beginning of the string, so we may proceed
                for (usIndex = gpCurrentStringDescriptor->usStringOffset;
                     usIndex <= gpCurrentStringDescriptor->usCurrentStringLength;
                     usIndex++) { // Shift the characters one at a time
                    *(gpCurrentStringDescriptor->pString + usIndex - 1) =
                        *(gpCurrentStringDescriptor->pString + usIndex);
                }
                gpCurrentStringDescriptor->usStringOffset--;
                gpCurrentStringDescriptor->usCurrentStringLength--;
            }

            break;
        case 0x2e: // Delete the character which follows the cursor
            if (gpCurrentStringDescriptor->usStringOffset <
                gpCurrentStringDescriptor
                    ->usCurrentStringLength) { // Ok we are not at the end of the string, so we may proceed
                for (usIndex = gpCurrentStringDescriptor->usStringOffset;
                     usIndex < gpCurrentStringDescriptor->usCurrentStringLength;
                     usIndex++) { // Shift the characters one at a time
                    *(gpCurrentStringDescriptor->pString + usIndex) =
                        *(gpCurrentStringDescriptor->pString + usIndex + 1);
                }
                gpCurrentStringDescriptor->usCurrentStringLength--;
            }
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
        case 0x2d: // Toggle insert mode
            if (gpCurrentStringDescriptor->fInsertMode == TRUE) {
                gpCurrentStringDescriptor->fInsertMode = FALSE;
            } else {
                gpCurrentStringDescriptor->fInsertMode = TRUE;
            }
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
        case 0x24: // Go to the beginning of the input string
            gpCurrentStringDescriptor->usStringOffset = 0;
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
            // Stupid definition causes problems with headers that use the keyword END -- DB
        case 0x23: // Go to the end of the input string
            gpCurrentStringDescriptor->usStringOffset =
                gpCurrentStringDescriptor->usCurrentStringLength;
            gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            break;
        default: //
            // normal input
            //
            usInputCharacter = TranslateKeyToCharacter(
                usInputCharacter, (UINT8)(gfAltState | gfCtrlState | gfShiftState));
            if (usInputCharacter == 0)
                return;
            if (CharacterIsValid(usInputCharacter, gpCurrentStringDescriptor->pFilter) == TRUE) {
                if (gpCurrentStringDescriptor->fInsertMode ==
                    TRUE) { // Before we can shift characters for the insert, we must make sure we have the space
                    if (gpCurrentStringDescriptor->usCurrentStringLength <
                        (gpCurrentStringDescriptor->usMaxStringLength -
                         1)) { // Before we can add a new character we must shift existing ones to for the insert
                        for (usIndex = gpCurrentStringDescriptor->usCurrentStringLength;
                             usIndex > gpCurrentStringDescriptor->usStringOffset;
                             usIndex--) { // Shift the characters one at a time
                            *(gpCurrentStringDescriptor->pString + usIndex) =
                                *(gpCurrentStringDescriptor->pString + usIndex - 1);
                        }
                        // Ok now we introduce the new character
                        *(gpCurrentStringDescriptor->pString + usIndex) = usInputCharacter;
                        gpCurrentStringDescriptor->usStringOffset++;
                        gpCurrentStringDescriptor->usCurrentStringLength++;
                    }
                } else {
                    // Ok, add character to string (by overwriting)
                    if (gpCurrentStringDescriptor->usStringOffset <
                        (gpCurrentStringDescriptor->usMaxStringLength -
                         1)) { // Ok, we have not exceeded the maximum number of characters yet
                        *(gpCurrentStringDescriptor->pString +
                          gpCurrentStringDescriptor->usStringOffset) = usInputCharacter;
                        gpCurrentStringDescriptor->usStringOffset++;
                    }
                    // Did we push back the current string length (i.e. add character to end of string)
                    if (gpCurrentStringDescriptor->usStringOffset >
                        gpCurrentStringDescriptor->usCurrentStringLength) { // Add a NULL character
                        *(gpCurrentStringDescriptor->pString +
                          gpCurrentStringDescriptor->usStringOffset) = 0;
                        gpCurrentStringDescriptor->usCurrentStringLength++;
                    }
                }
                gpCurrentStringDescriptor->usLastCharacter = usInputCharacter;
            }
            break;
        }
    }
}

//
// Miscellaneous input-related utility functions:
//

// FUNCTION: WIZ8 0x00402750
void FreeMouseCursor(void)
{
    ClipCursor(NULL);
    fCursorWasClipped = FALSE;
}

void HandleSingleClicksAndButtonRepeats(void)
{
    UINT32 uiTimer;

    uiTimer = GetTickCount();

    // Is there a LEFT mouse button repeat
    if (gfLeftButtonState) {
        if ((guiLeftButtonRepeatTimer > 0) && (guiLeftButtonRepeatTimer <= uiTimer)) {
            UINT32 uiTmpLParam;
            POINT MousePos;

            GetCursorPos(&MousePos);
            uiTmpLParam = ((MousePos.y << 16) & 0xffff0000) | (MousePos.x & 0x0000ffff);
            QueueEvent(LEFT_BUTTON_REPEAT, 0, uiTmpLParam);
            guiLeftButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIME;
        }
    } else {
        guiLeftButtonRepeatTimer = 0;
    }

    // Is there a RIGHT mouse button repeat
    if (gfRightButtonState) {
        if ((guiRightButtonRepeatTimer > 0) && (guiRightButtonRepeatTimer <= uiTimer)) {
            UINT32 uiTmpLParam;
            POINT MousePos;

            GetCursorPos(&MousePos);
            uiTmpLParam = ((MousePos.y << 16) & 0xffff0000) | (MousePos.x & 0x0000ffff);
            QueueEvent(RIGHT_BUTTON_REPEAT, 0, uiTmpLParam);
            guiRightButtonRepeatTimer = uiTimer + BUTTON_REPEAT_TIME;
        }
    } else {
        guiRightButtonRepeatTimer = 0;
    }
}

// FUNCTION: WIZ8 0x00402760
INT16 GetMouseWheelDeltaValue(UINT32 wParam)
{
    INT16 sDelta = HIWORD(wParam);

    return (sDelta / WHEEL_DELTA);
}

// FUNCTION: WIZ8 0x00402780
unsigned short TranslateKeyToCharacter(unsigned short key, unsigned char modifiers)
{
    if ((modifiers & (CTRL_DOWN | ALT_DOWN)) != 0)
        return 0;
    if ((modifiers & SHIFT_DOWN) != 0)
        return gsKeyTranslationTable[key + 256];
    return gsKeyTranslationTable[key];
}

// FUNCTION: WIZ8 0x004027C0
unsigned short TranslateCharacterToKey(unsigned short character)
{
    UINT16 key;
    for (key = 0; key < 0x200; ++key) {
        if (gsKeyTranslationTable[key] == character) {
            return key % 256;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x00402800
BOOLEAN IsUppercaseWideChar(unsigned short character)
{
    if (character >= L'A' && character <= L'Z')
        return TRUE;
    return FALSE;
}

// FUNCTION: WIZ8 0x00402820
BOOLEAN IsLowercaseWideChar(unsigned short character)
{
    if (character >= L'a' && character <= L'z')
        return TRUE;
    return FALSE;
}

// FUNCTION: WIZ8 0x00402840
BOOLEAN IsPunctuationWideChar(unsigned short character)
{
    if ((character >= L'!' && character <= L'/') || (character >= L':' && character <= L'@') ||
        (character >= L'[' && character <= L'_') || (character >= L'{' && character <= L'}'))
        return TRUE;
    return FALSE;
}

// FUNCTION: WIZ8 0x00402880
int ToUppercaseWideChar(int character)
{
    if ((unsigned short)character > L'`' && (unsigned short)character < L'{') {
        character -= L'a' - L'A';
    }
    return character;
}

// FUNCTION: WIZ8 0x004028A0
int ToLowercaseWideChar(int character)
{
    if ((unsigned short)character > L'@' && (unsigned short)character < L'[') {
        character += L'a' - L'A';
    }
    return character;
}

/* Unlike the pinned VC6 _wcsicmp, retail has no locale branch. The adjacent
   character helpers provide the same ASCII-only case conversion. */
// FUNCTION: WIZ8 0x00402920
int CompareWideTextIgnoreAsciiCase(const wchar_t* first, const wchar_t* second)
{
    unsigned short left;
    unsigned short right;
    do {
        left = ToLowercaseWideChar(*first++);
        right = ToLowercaseWideChar(*second++);
    } while (left != 0 && left == right);
    return (UINT32)left - (UINT32)right;
}
