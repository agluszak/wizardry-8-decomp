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

extern int g_table_6504e8[];
void ClearValue69DA68(void);
unsigned char GetFlag69DA6C(void);
/* Record-mode console line input and its per-key prompt/apply callbacks. */
void WriteRecordModeEntry005E3280(void);                                        /* 0x005E3280 */
char HandleRecordModeKey005E3610(const InputAtom* input, void (*prompt)(void)); /* 0x005E3610 */
void ApplyRecordModeLine005E34B0(void);                                         /* 0x005E34B0 */
void PromptRecordModeEntry005E35A0(void);                                       /* 0x005E35A0 */
unsigned char GetTable650434Entry(int trap, int device);
/* 0x005E3740: roll the embedded lock state's pin count against
   g_tumbler_count_table_006504ac until it lands within four of the trigger's
   difficulty count. */
void RandomizeTriggerTumblerCount005E3740(Trigger* trigger);
void CompleteTrapInteraction005E3780(Trigger* trigger); /* 0x005E3780 */
void CastTrapSpell005E3800(srVector3T<float> point, int spell_id, int power,
                           int num_targets);      /* 0x005E3800 */
void TriggerTrapDevice005E3AB0(Trigger* trigger); /* 0x005E3AB0 */
