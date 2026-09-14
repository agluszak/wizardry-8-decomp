#pragma once

struct Controls;
class W8TextControl;

/* Local Screens\RCSCommon.cpp's cross-TU panel lifecycle: release the three
   level-runtime dialogue owners and tear the camp panel down. */
void ReleaseRuntimeDialogOwners(void);
void ReleaseReviewCommonPanels(void);
void CloseReviewCommonUi(void);

/* The formation panel and its controls. MGSFormation.cpp builds and drives
   them; the shared review teardown releases them. */
extern Controls* g_panel_69c2ec;
extern W8TextControl* g_panel_controls_69c344[15];
extern W8TextControl* g_panel_controls_69c384[15];
extern W8TextControl* g_panel_controls_69c2f8[3];

void Function5B4EB0(void);
void Function5B55F0(void);
void SetCampItemActionMode005B59B0(char mode);
void SelectCampCharacter005B6B30(int slot);

void RedrawRcsLevelUpPanel(void); /* 0x005B6590 */
void RedrawRcsDismissPanel(void); /* 0x005B68D0 */
