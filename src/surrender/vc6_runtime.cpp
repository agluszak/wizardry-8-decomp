/*
 * Linker- and CRT-owned functions in the canonical sr.dll. As in
 * src/wiz8/vc6_runtime.cpp, LIBRARY markers give reccmp address ownership
 * without pretending these bodies are first-party recovered source.
 */

// LIBRARY: SURRENDER 0x1006FF1A
// ??_M@YGXPAXIHP6EX0@Z@Z

// LIBRARY: SURRENDER 0x10070050
// __aullshr

// LIBRARY: SURRENDER 0x1007007C
// ??_L@YGXPAXIHP6EX0@Z1@Z

// LIBRARY: SURRENDER 0x100700E6
// ??_L@YGXPAXIHP6EX0@Z1@Z unwind funclet

// LIBRARY: SURRENDER 0x100701E0
// __allmul

// LIBRARY: SURRENDER 0x10070220
// __aulldiv
// SYNTHETIC: SURRENDER 0x1006FE6E
// VerQueryValueA import thunk

// SYNTHETIC: SURRENDER 0x1006FE74
// GetFileVersionInfoA import thunk

// SYNTHETIC: SURRENDER 0x1006FE7A
// GetFileVersionInfoSizeA import thunk

// SYNTHETIC: SURRENDER 0x1006FE80
// GetSaveFileNameA import thunk

// SYNTHETIC: SURRENDER 0x1006FEB6
// operator_new import thunk

// SYNTHETIC: SURRENDER 0x1006FEBC
// operator_delete import thunk

// SYNTHETIC: SURRENDER 0x1006FED0
// _CIpow import thunk

// LIBRARY: SURRENDER 0x1006FED6
// atexit

// LIBRARY: SURRENDER 0x1006FF02
// _onexit

// LIBRARY: SURRENDER 0x1006FF82
// CRT init/exit array guarded caller (checks the unwind flag before
// walking the function-pointer array)

// LIBRARY: SURRENDER 0x1006FF9A
// _initterm-style CRT init/exit array walker

// LIBRARY: SURRENDER 0x10070020
// _ftol

// LIBRARY: SURRENDER 0x1007003E
// _CxxThrowException

// LIBRARY: SURRENDER 0x1007010F
// type_info scalar deleting destructor

// LIBRARY: SURRENDER 0x10070172
// _CIacos

// LIBRARY: SURRENDER 0x10070288
// _CRT_INIT per-reason CRT init/term dispatcher (onexit-array malloc,
// initterm on PROCESS_ATTACH, onexit walk on DETACH)

// LIBRARY: SURRENDER 0x10070333
// _DllMainCRTStartup (the linker entry point calling DllMain at 0x10045990)

// LIBRARY: SURRENDER 0x100703D0
// __dllonexit

// LIBRARY: SURRENDER 0x100703E2
// type_info::~type_info

// SYNTHETIC: SURRENDER 0x100703E8
// initterm import thunk
