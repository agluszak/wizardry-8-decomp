#ifndef WIZ8_INPUT_HOOKS_H
#define WIZ8_INPUT_HOOKS_H
#include "wiz8/wiz8_windows.h"

/* Wizardry-specific key translation retained beside released SGP input.c. */

void Function402270(unsigned int key, unsigned int flags, char pressed);
void Function4023B0(unsigned short key);
unsigned short TranslateKeyToCharacter(unsigned short key, unsigned char modifiers);
void Function427A70(RECT* rect);
#endif
