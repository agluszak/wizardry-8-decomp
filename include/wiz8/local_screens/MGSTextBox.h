#pragma once

#include "wiz8/3d_code/PList.h"

/* Local Screens\MGSTextBox.cpp owns the four message runs at 0x0068F2D8. */
struct W8MessageStorageRecord {
    void* allocation_00;
    unsigned char unknown_04[0x14];
    W8PList* entries_18;
    unsigned char unknown_1c[8];
};

static_assert(sizeof(W8MessageStorageRecord) == 0x24,
              "W8MessageStorageRecord_must_be_0x24");

extern "C" {
extern W8MessageStorageRecord g_message_storage_68f2d8[4][0x15e];
}
