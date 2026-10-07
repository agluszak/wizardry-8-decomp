#pragma once
#include "wiz8/layouts/screen_state.h"
#include "wiz8/dialog_code/DialogBase.h"

void RefreshPartySlotDisplay(unsigned int party_slot);
void InitializeMainGameLevelBlock(void);
void SetTargetCursor(int cursor);
void ApplyCurrentCursor(void);
void RequestPartySlotRedraw(int bit);
unsigned char GetPartyOrderTextColor(signed char party_order);
unsigned char ScreenLifecycleSuccess(void); // bool-byte-ok: screen-table unsigned char (*)() slot
void NoOp(void);
W8ScreenId GetPendingScreenState(void);
void SetPendingScreenState(W8ScreenId value);
void RequestScreenTransition(void);
// bool-byte-ok: screen-table unsigned char (*)() slot
bool IsScreenTransitionPending(void);
void RequestExitScreen(void);
unsigned char ExitScreenEnter(void);
void ExitScreenFrame(void);
/* Dispatch one already-built line to the active screen. */
void ShowNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, bool confirmation,
                    bool cancel);
