#pragma once

/* Local Code\Gameplay Mods.cpp. */

void RebuildPartyEffectBlock0050E700(void);

/* The unit's unrecovered block helpers; the party effect block is their
   shared target. */
void Function50EDC0(
    const unsigned char* source, unsigned char* target); /* 0x0050EDC0 */
void Function50EF50(
    const unsigned char* source, unsigned char* target); /* 0x0050EF50 */
void Function50F090(void* target, const void* source); /* 0x0050F090 */
