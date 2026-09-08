#ifndef WIZ8_INPUT_HOOKS_H
#define WIZ8_INPUT_HOOKS_H
#include "wiz8/wiz8_windows.h"

/* Wizardry's Windows input-hook layer: the thread keyboard/mouse hooks that
   feed the key-event buffer and text line editor below, plus the state they
   share. The SGP queue in input.c stays untouched; this unit owns the
   retail 0x00401xxx hook addresses the startup spine installs. */

unsigned char InitializeInputManager00401EA0(void);

/* Retail 0x006F04E8/0x006F04ED: the mouse hook's button latches, set on
   button-down and cleared on button-up. The exit screen reads them. */
extern unsigned char g_flag_6f04e8;
extern unsigned char g_flag_6f04ed;

void Function401F90(short kind, unsigned int param_08, unsigned int param_0c);
void Function402270(unsigned int key, unsigned int flags, char pressed);
void Function4023B0(unsigned short key);
void Function427A70(RECT* rect);
long __stdcall Function401B30(int code, unsigned int key, long flags);
long __stdcall Function401C70(int code, unsigned int button, long info);

#endif
