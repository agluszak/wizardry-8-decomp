#pragma once

void Function59B940(void);
void Function59BDB0(void);
void Function598AB0(void);
void Function598AE0(void);
void Function59B270(void);
void Function59BAD0(void); /* 0x0059BAD0 */
void Function59BF70(void); /* 0x0059BF70 */
void Function59C930(int slot);
void Function59C9C0(void);
void EnablePortraitAdvanceRegions0059BB70(void);                 /* 0x0059BB70 */
void InvalidatePortraitControl0059BBD0(unsigned int party_slot); /* 0x0059BBD0 */

/* Main-game portrait overlay helpers used when a party slot refreshes. */
unsigned char PreparePartyPortraitOverlay(unsigned int party_slot, unsigned int flags,
                                          unsigned int top); /* 0x005993A0 */
void RedrawPartyPortraitOverlay(unsigned int party_slot, char highlighted, char overlay_ready,
                                char slot_enabled); /* 0x005994C0 */
