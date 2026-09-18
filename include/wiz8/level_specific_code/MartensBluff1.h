#pragma once

class Trigger;

/* Level 5 (MartensBluff1) master function and activation callbacks,
   registered by the case-5 block of InitializeLevelMasterFunctions004D6C50. */
bool MartensBluff1TransportSpawn004DF160(void);
void MartensBluff1Transporter004DF260(int command);
bool MartensBluff1Teleporter004DF4A0(Trigger* pTrigger);
bool MartensBluff1ButtonGigas004DF540(Trigger* pTrigger);
bool MartensBluff1ButtonTrang004DF560(Trigger* pTrigger);
bool MartensBluff1ButtonRift004DF580(Trigger* pTrigger);
bool MartensBluff1ButtonMaten004DF5A0(Trigger* pTrigger);
bool MartensBluff1TeleportState004DF5C0(int new_state);
