#pragma once

#include <wchar.h>

void LoadLocalizedStrings(const char* path);

/* Local Code\Strings.cpp owns the decoded game string table. */
extern wchar_t** gppStringList;
extern int giStringListLen;
void FreeStringTable(void);

