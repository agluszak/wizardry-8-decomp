#pragma once

/* Level Specific Code\Ascension.cpp helpers (Ascension Peak, incl. the
   Footsteps level 0x24 block). */
/* How many of the three Ascension Peak relic items (0x242-0x244) the party
   carries; the type-5 world-cursor handler gates "AP_SeenBodies" on it. */
int CountAscensionPeakItems004DF810(void);          /* 0x004DF810 */
unsigned char SpawnAlfieLife004DFB40(int unused); /* 0x004DFB40 */
unsigned char SpawnAlfieKnow004DFB80(int unused); /* 0x004DFB80 */
void SyncButtonBlockerFromFact004E0510(void);     /* 0x004E0510 */
