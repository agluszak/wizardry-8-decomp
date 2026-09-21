#pragma once

#include "input.h"

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
char HandleRecordModeInput005E3610(const InputAtom* input, void (*prompt)(void)); /* 0x005E3610 */
void HandleRecordModeCommand005E34B0(void);                                       /* 0x005E34B0 */
void ShowRecordModePrompt005E35A0(void);                                          /* 0x005E35A0 */
unsigned char GetTable650434Entry(int trap, int device);
/* Finishes a successful disarm: completes the item interaction, rolls the
   learn chance, prints the "<trap> disarmed" line and runs the trigger. */
void CompleteTrapDisarm005E3780(Trigger* trigger); /* 0x005E3780 */
/* Resolves a sprung trap: prints the outcome line, derives target count and
   power from the device count versus the type's difficulty, then discharges
   the trap's spell. */
void ResolveSprungTrap005E3AB0(Trigger* trigger); /* 0x005E3AB0 */
