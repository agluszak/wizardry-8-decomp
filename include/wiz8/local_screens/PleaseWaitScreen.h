#pragma once

extern int g_level_load_font_69b7c0; /* 0x0069B7C0: font used for level-load and
                                        party-death/ending overlay text */

unsigned char PleaseWaitScreenInitialize(void);
unsigned char PleaseWaitScreenEnter(void);
void PleaseWaitScreenFrame(void);
unsigned char PleaseWaitScreenLeave(int leaving);
void UpdatePleaseWaitLoadFrame005915A0(void);
