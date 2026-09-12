#pragma once

/* Local Code\Traps.cpp. The three bodies at 0x5E35F0-0x5E3730 sit in the
   attribution gap before the asserted Traps.cpp body at 0x5E3800. */

void ClearValue69DA68(void);
unsigned char GetFlag69DA6C(void);
unsigned char GetTable650434Entry(int row, int column);
