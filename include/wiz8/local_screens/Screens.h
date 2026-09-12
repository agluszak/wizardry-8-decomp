#pragma once

void RefreshPartySlotDisplay(unsigned int party_slot); /* 0x0055EC90 */
void InitializeMainGameLevelBlock(void);               /* 0x0055F2C0 */
void SetTargetCursor(int cursor);                      /* 0x0055EE70 */
void ApplyCurrentCursor(void);                         /* 0x0055F080 */
void RequestPartySlotRedraw(int bit);                  /* 0x0055EE30 */
unsigned char ScreenLifecycleSuccess(void);
void NoOp(void);
int GetPendingScreenState(void);
void SetPendingScreenState(int value);
void RequestScreenTransition(void);
unsigned char IsScreenTransitionPending(void);
void RequestExitScreen(void);
unsigned char ExitScreenEnter(void);
void ExitScreenFrame(void);
