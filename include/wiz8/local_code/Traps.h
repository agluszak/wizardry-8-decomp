#pragma once

#include "input.h"
#include "surrender/srMath.h"

class Trigger;

/* The trap UI has fifteen known-trap rows and eight device buttons. The retail
   manual describes the same model explicitly: every trap is a combination of
   up to eight devices, selected from the Known Traps list. */
enum { W8_TRAP_TYPE_COUNT = 15, W8_TRAP_DEVICE_COUNT = 8 };

/* Local Code\Traps.cpp. The three bodies at 0x5E35F0-0x5E3730 sit in the
   attribution gap before the asserted Traps.cpp body at 0x5E3800. */

void ClearValue69DA68(void);
unsigned char GetFlag69DA6C(void);
/* Record-mode console line input and its per-key prompt/apply callbacks. */
char Function5E3610(const InputAtom* input, void (*prompt)(void)); /* 0x005E3610 */
void Function5E34B0(void);                                         /* 0x005E34B0 */
void Function5E35A0(void);                                         /* 0x005E35A0 */
unsigned char GetTable650434Entry(int trap, int device);
void Function5E3780(Trigger* trigger); /* 0x005E3780 */
void Function5E3AB0(Trigger* trigger); /* 0x005E3AB0 */
/* 0x005E3800: cast `spell_id` from a trap at `point`; place-targeted spells go
   to the camera position, party spells strike up to `target_count` random
   living members other than the selected character. */
void CastTrapSpellAtPoint005E3800(srVector3T<float> point, int spell_id, unsigned int power_level,
                                  int target_count);
