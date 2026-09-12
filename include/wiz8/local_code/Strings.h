#pragma once

#include <wchar.h>

void LoadLocalizedStrings(const char* path);

/* Local Code\Strings.cpp owns the decoded game string table. */
extern wchar_t** gppStringList;
extern int giStringListLen;
extern const wchar_t g_dash_0064789c[];
void FreeStringTable(void);
