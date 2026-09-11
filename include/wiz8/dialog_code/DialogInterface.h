#pragma once

#include "wiz8/dialog_code/DialogBase.h"

struct W8Character;

/* ConfigureDialogFont writes these four; every dialog draw/text path reads
   them. Declared here because this unit owns their definitions. */
extern int g_dialog_font_64fde8;
extern BOOLEAN g_dialog_font_enabled_69ca32;
extern unsigned char g_dialog_font_foreground_64fdec;
extern unsigned char g_dialog_font_background_64fded;
void ConfigureDialogFont(
    int font,
    BOOLEAN enabled,
    unsigned char foreground,
    unsigned char background);
W8DialogBase* Function5CF280(W8Character* character);
W8DialogBase* CreateDialogByKind(int kind);
unsigned char GetDialogResult(W8DialogBase* dialog);
void DrawDialog(W8DialogBase* dialog);
unsigned char ProcessDialogInput(W8DialogBase* dialog);
void SetDialogDestroyCallback(
    W8DialogBase* dialog, W8DialogDestroyCallback callback);
