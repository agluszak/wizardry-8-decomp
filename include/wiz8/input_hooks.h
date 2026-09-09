#ifndef WIZ8_INPUT_HOOKS_H
#define WIZ8_INPUT_HOOKS_H
#include "wiz8/wiz8_windows.h"

/* Wizardry's Windows input hooks feed the released SGP input queue consumed
   by the screens. Shared input state belongs to input.c. */

unsigned char InitializeInputManager00401EA0(void);

void Function402270(unsigned int key, unsigned int flags, char pressed);
void Function4023B0(unsigned short key);
unsigned short TranslateKeyToCharacter(unsigned short key, unsigned char modifiers);
void Function427A70(RECT* rect);
long __stdcall Function401B30(int code, unsigned int key, long flags);
long __stdcall Function401C70(int code, unsigned int button, long info);

#endif
