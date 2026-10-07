#pragma once

#include "input.h"
#include "surrender/srMath.h"

class Trigger;

/* Every trap is a combination of up to eight devices, selected from the
   fifteen-row Known Traps list. */
enum { W8_TRAP_TYPE_COUNT = 15, W8_TRAP_DEVICE_COUNT = 8 };

/* Spell-id-per-trap-type at index 11+; the opening entries are unrelated
   chance values used by the lock interaction. */
extern int g_table2[];
void ClearRecordModeValue(void);
bool IsRecordModeActive(void);
/* Record-mode console line input and its per-key prompt/apply callbacks. */
void WriteRecordModeEntry(void);
char HandleRecordModeKey(const InputAtom* input, void (*prompt)(void));
void ApplyRecordModeLine(void);
void PromptRecordModeEntry(void);
unsigned char GetTable650434Entry(int trap, int device);
/* Picks the trigger's trap type (device_id) by rejection-rolling a table row
   whose difficulty sits within four of the trigger's grade (difficulty). */
void SelectTrapType(Trigger* trigger);
/* Finishes a successful disarm: completes the item interaction, rolls the
   learn chance, prints the "<trap> disarmed" line and runs the trigger. */
void CompleteTrapDisarm(Trigger* trigger);
/* Resolves a sprung trap: prints the outcome line, derives target count and
   power from the device count versus the type's difficulty, then discharges
   the trap's spell. */
void ResolveSprungTrap(Trigger* trigger);
