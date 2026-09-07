#pragma once

#include "wiz8/dialog_code/DialogBase.h"

struct W8Character;

extern "C" {
void ConfigureDialogFont(
    int font,
    unsigned char enabled,
    unsigned char foreground,
    unsigned char background);
W8DialogBase005DC7A0* Function5CF280(W8Character* character);
W8DialogBase005DC7A0* CreateDialogByKind(int kind);
unsigned char GetDialogResult(W8DialogBase005DC7A0* dialog);
void DrawDialog(W8DialogBase005DC7A0* dialog);
unsigned char ProcessDialogInput(W8DialogBase005DC7A0* dialog);
void SetDialogDestroyCallback(
    W8DialogBase005DC7A0* dialog, W8DialogDestroyCallback callback);
}
