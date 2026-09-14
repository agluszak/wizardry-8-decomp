#pragma once

#include <stddef.h>

struct Controls;
class W8TextControl;

void Function5B4EB0(void);
void Function5B55F0(void);
void SetCampItemActionMode005B59B0(char mode);
void SelectCampCharacter005B6B30(int slot);

void RedrawRcsLevelUpPanel(void); /* 0x005B6590 */
void RedrawRcsDismissPanel(void); /* 0x005B68D0 */

extern Controls* g_level_up_panel_0069c3c4;
extern Controls* g_dismiss_panel_0069c3c8;
extern W8TextControl* g_level_up_button_0069c3c0;
extern W8TextControl* g_dismiss_button_0069c400;

void CreateRcsLevelUpPanel(void);
void DestroyRcsLevelUpPanel(void);
void UpdateRcsLevelUpPanel(void);
void CreateRcsDismissPanel(void);
void DestroyRcsDismissPanel(void);
void UpdateRcsDismissPanel(void);
void DrawRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawRcsBoldText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
void DrawTallRcsText(const wchar_t* text, int left, int top, int width, unsigned int layout_mode);
/* 0x005B6FD0: like DrawRcsText but the box height is caller-provided and the
   text is rendered through mprintf with the current font. */
void DrawRcsTextJustified(const wchar_t* text, int left, int top, int width, int height,
                          unsigned int layout_mode);
