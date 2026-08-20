#pragma once

#include <wchar.h>

/* Local Code\Strings.cpp owns the decoded game string table. */
extern "C" {
extern wchar_t** gppStringList;
extern int giStringListLen;
void FreeStringTable(void);
bool IsStringTableLoaded(void);
}
