#pragma once

#include <wchar.h>

void LoadLocalizedStrings(const char* path);

/* Local Code\Strings.cpp owns the decoded game string table. */
extern "C" {
extern wchar_t** gppStringList;
extern int giStringListLen;
void FreeStringTable(void);
}

void StartBreathCycle(int party_slot, int arg_2);                 /* 0x0052FE80 */

