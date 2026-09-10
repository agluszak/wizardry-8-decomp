#pragma once

#include "Font.h"

/* Wizardry uses SGP's font-manager representation: FontObjs holds the
   0xFC-byte objects 0x00406180 builds, FontDefault selects the print font,
   and FontDestBuffer/FontDestRegion/FontDestWrap select the print target.
   Retail's accessors, palette setter, loader, printers, and string metrics are
   relocation-equivalent to the released oracle (see
   build/reports/sgp/harness.csv) and link from Font.c. Their declarations
   remain in the pinned Font.h. */
