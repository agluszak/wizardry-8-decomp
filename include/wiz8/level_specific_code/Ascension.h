#pragma once

class Trigger;

/* Level Specific Code\Ascension.cpp helpers (Ascension Peak). */
/* How many of the three Ascension Peak relic items (0x242-0x244) the party
   carries; the type-5 world-cursor handler gates "AP_SeenBodies" on it. */
int CountAscensionPeakItems(void);                /* 0x004DF810 */
unsigned char AscensionPeakInit(void);            /* 0x004DF870 */
unsigned char SpawnAlfieChaos(int unused);        /* 0x004DFAE0 */
unsigned char SpawnAlfieLife(int unused);         /* 0x004DFB40 */
unsigned char SpawnAlfieKnow(int unused);         /* 0x004DFB80 */
void AscensionAvalanche(unsigned char command);   /* 0x004DFBC0 */
void AscensionLandShaker(int command);            /* 0x004DFD70 */
bool AscensionRampUp(Trigger* pTrigger);          /* 0x004DFE60 */
unsigned char SpawnAscensionAmbush(void);         /* 0x004DFEA0 */
bool AscensionChaosBTrigger(Trigger* pTrigger);   /* 0x004DFF90 */
bool AscensionChaosATrigger(Trigger* pTrigger);   /* 0x004DFFF0 */
bool AscensionLifeBTrigger(Trigger* pTrigger);    /* 0x004E00B0 */
bool AscensionLifeATrigger(Trigger* pTrigger);    /* 0x004E0110 */
bool AscensionKnowBTrigger(Trigger* pTrigger);    /* 0x004E01D0 */
bool AscensionKnowATrigger(Trigger* pTrigger);    /* 0x004E0230 */
bool AscensionDarkSavantSpawn(Trigger* pTrigger); /* 0x004E02F0 */
bool AscensionPath1Camera(Trigger* pTrigger);     /* 0x004E0390 */
void RemoveAletheides(void);                      /* 0x004E0430 */
bool AscensionShaker(Trigger* pTrigger);          /* 0x004E04F0 */
