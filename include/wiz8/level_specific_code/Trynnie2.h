#pragma once

/* Level 0x1a (Trynnie2) helpers, run by InitializeLevelMasterFunctions
   under case 0x1a. */
void EnsureTrynnie2KilledVar(void);
struct W8ItemInstance;
/* 0x004D9F60: consume the summon-quest items (0x1b3/0x1c3) when used. */
bool Trynnie2UseItem(W8ItemInstance* item);
