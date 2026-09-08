#pragma once

/* Wizardry's font objects live in SGP's font manager: FontObjs holds the
   0xFC-byte objects 0x00406180 builds, FontDefault selects the print font,
   and FontDestBuffer/FontDestRegion/FontDestWrap select the print target.
   Retail's stateless accessors (StringPixLength, GetFontHeight,
   GetFontObject, GetFontObjectPalette16BPP, SetFont, SetFontDestBuffer,
   BltIsClipped) are byte-identical to the oracle (see
   build/reports/sgp/harness.csv) and link from Font.c; only the modified
   palette setter, the file-backed loader, and the two surface-aware
   printers stay first-party. The entry points' declarations stay in the
   pinned Font.h, whose signatures match. */
