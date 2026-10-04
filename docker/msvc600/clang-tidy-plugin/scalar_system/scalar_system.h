#pragma once
// System-header API contracts for the scalar collector fixture.
typedef unsigned int size_t;
typedef unsigned short wchar_t_fixture;
typedef wchar_t_fixture WCHAR;
extern "C" char* strcpy(char* destination, const char* source);
extern "C" void* memcpy(void* destination, const void* source, size_t count);
extern "C" WCHAR* lstrcpyW(WCHAR* destination, const WCHAR* source);
