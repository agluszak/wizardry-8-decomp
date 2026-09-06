#pragma once

#include "wiz8/text_types.h"

extern "C" {

unsigned char GetStringFromStringDatabase(
    const char* path,
    int index,
    W8WideChar* output,
    unsigned int* metadata_04,
    unsigned int* metadata_00);
void ShowString(W8WideChar* text);

}
