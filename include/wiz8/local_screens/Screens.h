#pragma once
#include "wiz8/layouts/screen_state.h"
#include "wiz8/dialog_code/DialogBase.h"

void RefreshPartySlotDisplay(unsigned int party_slot);         /* 0x0055EC90 */
void InitializeMainGameLevelBlock(void);                       /* 0x0055F2C0 */
void SetTargetCursor(int cursor);                              /* 0x0055EE70 */
void ApplyCurrentCursor(void);                                 /* 0x0055F080 */
void RequestPartySlotRedraw(int bit);                          /* 0x0055EE30 */
unsigned char GetPartyOrderTextColor(signed char party_order); /* 0x0055F2B0 */
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
/* 0x0055F260 dispatches one already-built line to the active screen. */
void ShowNoticeLine(wchar_t* text, W8DialogDestroyCallback callback, bool confirmation,
                    bool cancel);
