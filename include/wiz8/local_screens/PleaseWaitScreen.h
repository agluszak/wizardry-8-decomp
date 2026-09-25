#pragma once

extern int g_level_load_font; /* 0x0069B7C0: font used for level-load and
                                        party-death/ending overlay text */

unsigned char PleaseWaitScreenInitialize(void);
unsigned char PleaseWaitScreenEnter(void);
void PleaseWaitScreenFrame(void);
unsigned char PleaseWaitScreenLeave(int leaving);
void UpdatePleaseWaitLoadFrame(void);
