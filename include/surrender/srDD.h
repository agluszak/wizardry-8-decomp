#pragma once

class srDD;

// Dynamic driver entrypoints, proven by DirectX7 and the srGERD loader.
// This is a reconstructed header grouping, not a recovered original filename.
// Index/count/API values occupy one 32-bit word. The loader compares them
// unsigned; surviving exports do not distinguish original int/long spelling.
enum { SR_DD_MIN_API_VERSION = 0x128 };
typedef unsigned long (__cdecl *srDDGetDriverApiVersionFn)();
typedef const char* (__cdecl *srDDGetDriverNameFn)();
typedef void (__cdecl *srDDConfigureDriverFn)(const char* configuration);
typedef unsigned long (__cdecl *srDDGetDeviceCountFn)();
typedef const char* (__cdecl *srDDGetDeviceNameFn)(unsigned long index);
typedef srDD* (__cdecl *srDDInitDeviceFn)(unsigned long index);

// Argument-bearing DirectX7 entrypoints and srGERD call sites prove caller
// stack cleanup. No-argument functions alone cannot prove cdecl vs stdcall.
// The srDD object layout and its full virtual interface remain unrecovered.
