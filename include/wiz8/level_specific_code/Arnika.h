#pragma once

class Trigger;

/* Level 0 (Arnika) elevator masters and activation callbacks, registered by
   the case-0 block of InitializeLevelMasterFunctions004D6C50. */
void ArnikaLevelSetup004E06D0(void);
bool ArnikaLazerScanner004E0880(Trigger* pTrigger);
void ArnikaLaserScanMaster004E0960(int command);
bool ArnikaScannerDoor004E0A80(Trigger* pTrigger);
void ArnikaWarningSound004E0AC0(int command);
bool ArnikaMookholo004E0DC0(Trigger* pTrigger);
void ArnikaMookholoWatch004E0F70(int command);
bool ArnikaMookFrontDoor004E1040(Trigger* pTrigger);
bool ArnikaYellowButton004E10A0(Trigger* pTrigger);
bool ArnikaVaultAlarmDoor004E1120(Trigger* pTrigger);
bool ArnikaExitButton004E1180(Trigger* pTrigger);
void ArnikaTeleportWatch004E1300(int command);
bool ArnikaGenVaultDoor004E1340(Trigger* pTrigger);
void ArnikaElevator1Setup004E13B0(void);
bool ArnikaRedButton004E1740(Trigger* pTrigger);
void ArnikaEl1Button004E17C0(int command);
bool ArnikaEl1TopButtons004E1930(Trigger* pTrigger);
bool ArnikaEl1BottomButtons004E1970(Trigger* pTrigger);
void ArnikaEl1Moving004E19B0(int command);
void ArnikaElevator2Setup004E1A10(void);
bool ArnikaGreenButton004E1DC0(Trigger* pTrigger);
void ArnikaEl2Button004E1E10(int command);
bool ArnikaElevator02Trigger004E1F80(Trigger* pTrigger);
void ArnikaEl2Moving004E1FB0(int command);
void ArnikaElevatorAdvance004E2010(int which);
bool ArnikaChaosMolori004E2340(Trigger* pTrigger);
bool ArnikaMaddmook004E2360(Trigger* pTrigger);
bool ArnikaAstralDominae004E23C0(Trigger* pTrigger);
bool ArnikaCMbox004E2420(Trigger* pTrigger);
int ArnikaPedestalItem004E24E0(int* previous_item);
bool ArnikaBallSlot004E26F0(Trigger* pTrigger);
bool ArnikaFlightRecorder004E2760(Trigger* pTrigger);
