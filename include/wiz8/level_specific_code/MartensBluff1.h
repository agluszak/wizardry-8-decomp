#pragma once

class Trigger;

/* Level 5 (MartensBluff1) master function and activation callbacks,
   registered by the case-5 block of InitializeLevelMasterFunctions004D6C50. */
void MartensBluff1Setup004DEB40(void);
bool MartensBluff1FHandlock004DEDB0(Trigger* pTrigger);
bool MartensBluff1DialA004DEE10(Trigger* pTrigger);
bool MartensBluff1DialB004DEE50(Trigger* pTrigger);
bool MartensBluff1DialC004DEEA0(Trigger* pTrigger);
bool MartensBluff1GasSwitch004DEEF0(Trigger* pTrigger);
bool MartensBluff1JDoorController004DEFB0(Trigger* pTrigger);
bool MartensBluff1Controller004DF120(Trigger* pTrigger);
bool MartensBluff1TransportSpawn004DF160(void);
void MartensBluff1Transporter004DF260(int command);
bool MartensBluff1Teleporter004DF4A0(Trigger* pTrigger);
bool MartensBluff1ButtonGigas004DF540(Trigger* pTrigger);
bool MartensBluff1ButtonTrang004DF560(Trigger* pTrigger);
bool MartensBluff1ButtonRift004DF580(Trigger* pTrigger);
bool MartensBluff1ButtonMaten004DF5A0(Trigger* pTrigger);
bool MartensBluff1TeleportState004DF5C0(int new_state);
bool MartensBluff1WireTrigger004DF710(Trigger* pTrigger);
bool MartensBluff1MartenBook004DF7E0(Trigger* pTrigger);
