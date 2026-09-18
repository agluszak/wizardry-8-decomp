#pragma once

/* Level 0x1a (Trynnie2) helpers, run by InitializeLevelMasterFunctions004D6C50
   under case 0x1a. */
void EnsureTrynnie2KilledVar004D9D30(void);

struct W8ItemInstance;
/* 0x004D9F60: consume the summon-quest items (0x1b3/0x1c3) when used. */
char Function4D9F60(W8ItemInstance* item);
