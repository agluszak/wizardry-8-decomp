#pragma once

#include "wiz8/dialog_code/DialogBase.h"

extern "C" {
void Function5CF250(
    int font,
    unsigned char enabled,
    unsigned char foreground,
    unsigned char background);
W8DialogBase005DC7A0* Function5CF300(int kind);
void Function5CF520(W8DialogBase005DC7A0* dialog);
unsigned char Function5CF550(W8DialogBase005DC7A0* dialog);
void Function5CF580(
    W8DialogBase005DC7A0* dialog, W8DialogDestroyCallback callback);
}
