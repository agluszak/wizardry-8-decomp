#pragma once

class Trigger;

/* Level Specific Code\Ascension.cpp helpers (Ascension Peak). */
/* How many of the three Ascension Peak relic items (0x242-0x244) the party
   carries; the type-5 world-cursor handler gates "AP_SeenBodies" on it. */
int CountAscensionPeakItems004DF810(void);                /* 0x004DF810 */
unsigned char AscensionPeakInit004DF870(void);            /* 0x004DF870 */
unsigned char SpawnAlfieChaos004DFAE0(int unused);        /* 0x004DFAE0 */
unsigned char SpawnAlfieLife004DFB40(int unused);         /* 0x004DFB40 */
unsigned char SpawnAlfieKnow004DFB80(int unused);         /* 0x004DFB80 */
void AscensionAvalanche004DFBC0(unsigned char command);   /* 0x004DFBC0 */
void AscensionLandShaker004DFD70(int command);            /* 0x004DFD70 */
bool AscensionRampUp004DFE60(Trigger* pTrigger);          /* 0x004DFE60 */
unsigned char SpawnAscensionAmbush004DFEA0(void);         /* 0x004DFEA0 */
bool AscensionChaosBTrigger004DFF90(Trigger* pTrigger);   /* 0x004DFF90 */
bool AscensionChaosATrigger004DFFF0(Trigger* pTrigger);   /* 0x004DFFF0 */
bool AscensionLifeBTrigger004E00B0(Trigger* pTrigger);    /* 0x004E00B0 */
bool AscensionLifeATrigger004E0110(Trigger* pTrigger);    /* 0x004E0110 */
bool AscensionKnowBTrigger004E01D0(Trigger* pTrigger);    /* 0x004E01D0 */
bool AscensionKnowATrigger004E0230(Trigger* pTrigger);    /* 0x004E0230 */
bool AscensionDarkSavantSpawn004E02F0(Trigger* pTrigger); /* 0x004E02F0 */
bool AscensionPath1Camera004E0390(Trigger* pTrigger);     /* 0x004E0390 */
void RemoveAletheides(void);                              /* 0x004E0430 */
bool AscensionShaker004E04F0(Trigger* pTrigger);          /* 0x004E04F0 */
