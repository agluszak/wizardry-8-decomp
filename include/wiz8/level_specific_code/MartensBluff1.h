#pragma once

class Trigger;

/* Level 5 (MartensBluff1) master function and activation callbacks,
   registered by the case-5 block of InitializeLevelMasterFunctions. */
void MartensBluff1Setup(void);
bool MartensBluff1FHandlock(Trigger* pTrigger);
bool MartensBluff1DialA(Trigger* pTrigger);
bool MartensBluff1DialB(Trigger* pTrigger);
bool MartensBluff1DialC(Trigger* pTrigger);
bool MartensBluff1GasSwitch(Trigger* pTrigger);
bool MartensBluff1JDoorController(Trigger* pTrigger);
bool MartensBluff1Controller(Trigger* pTrigger);
bool MartensBluff1TransportSpawn(void);
void MartensBluff1Transporter(int command);
bool MartensBluff1Teleporter(Trigger* pTrigger);
bool MartensBluff1ButtonGigas(Trigger* pTrigger);
bool MartensBluff1ButtonTrang(Trigger* pTrigger);
bool MartensBluff1ButtonRift(Trigger* pTrigger);
bool MartensBluff1ButtonMaten(Trigger* pTrigger);
bool MartensBluff1TeleportState(int new_state);
bool MartensBluff1WireTrigger(Trigger* pTrigger);
bool MartensBluff1MartenBook(Trigger* pTrigger);
