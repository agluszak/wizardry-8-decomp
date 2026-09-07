#pragma once

#include "wiz8/dialog_code/DialogBase.h"

struct W8Character;

extern "C" {
void ConfigureDialogFont(
    int font,
    unsigned char enabled,
    unsigned char foreground,
    unsigned char background);
W8DialogBase* Function5CF280(W8Character* character);
W8DialogBase* CreateDialogByKind(int kind);
unsigned char GetDialogResult(W8DialogBase* dialog);
void DrawDialog(W8DialogBase* dialog);
unsigned char ProcessDialogInput(W8DialogBase* dialog);
void SetDialogDestroyCallback(
    W8DialogBase* dialog, W8DialogDestroyCallback callback);
}
