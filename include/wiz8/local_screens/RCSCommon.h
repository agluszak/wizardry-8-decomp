#pragma once

/* Local Screens\RCSCommon.cpp's cross-TU panel lifecycle: release the three
   level-runtime dialogue owners and tear the camp panel down. */
void ReleaseRuntimeDialogOwners(void);
void Function5B2200(void);

void Function5B4EB0(void);
void Function5B55F0(void);
void Function5B59B0(int page);
void Function5B6B30(unsigned int slot);

void Function5B1C80(void);
void Function5B1E70(void);

void RedrawRcsLevelUpPanel(void); /* 0x005B6590 */
void RedrawRcsDismissPanel(void); /* 0x005B68D0 */
