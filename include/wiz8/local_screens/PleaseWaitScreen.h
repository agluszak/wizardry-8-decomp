#pragma once

extern int g_level_load_font; /* Font used for level-load and
                                        party-death/ending overlay text */

unsigned char PleaseWaitScreenInitialize(void);
unsigned char PleaseWaitScreenEnter(void);
void PleaseWaitScreenFrame(void);
unsigned char PleaseWaitScreenLeave(int leaving);
void UpdatePleaseWaitLoadFrame(void);
