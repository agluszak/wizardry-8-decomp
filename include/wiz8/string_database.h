#pragma once


unsigned char GetStringFromStringDatabase(const char* path, int index, wchar_t* output,
                                          unsigned int* metadata_00, unsigned int* metadata_04);
void ShowString(wchar_t* text);
