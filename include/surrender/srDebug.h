#pragma once

#include <ostream>
#include "srHeap.h"

SR_DLL_IMPORT w8_long __cdecl srDebugPrintf(w8_ulong level, const char* format, ...);
SR_DLL_IMPORT w8_long __cdecl srPrintf(const char* format, ...);
SR_DLL_IMPORT w8_long __cdecl srStreamPrintf(std::ostream& stream, const char* format, ...);
SR_DLL_IMPORT const char* __cdecl srBoolToString(int value);

/* Consumers see the fixed-arity form declared in wiz8's sr_api.h. */
SR_DLL_IMPORT void __cdecl srAssertFail(const char* expression, const char* source_path,
                                        w8_long line, const char* message, ...);

typedef void(__cdecl* srAssertHandler)(const char* expression, const char* source_path,
                                       w8_long line, const char* message);

SR_DLL_IMPORT srAssertHandler __cdecl srAssertGetFunc();
SR_DLL_IMPORT void __cdecl srAssertSetFunc(srAssertHandler handler);
SR_DLL_IMPORT void __cdecl srDefaultAssertFailFunc(const char* expression, const char* source_path,
                                                   w8_long line, const char* message);

/* Sink that discards every insertion. */
// VTABLE: SURRENDER 0x10076C00 srDummyStreamBuf
// class srDummyStreamBuf
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srDummyStreamBuf : public std::streambuf {
public:
    srDummyStreamBuf();

private:
    virtual int overflow(int ch);
    virtual int underflow();

    /* The copy constructor reinitializes a fresh stream buffer rather than copying get/put state. */
    srDummyStreamBuf(const srDummyStreamBuf& other);
    srDummyStreamBuf& operator=(const srDummyStreamBuf& other);
};

// VTABLE: SURRENDER 0x10076C34 srOStream_withassign
class srOStream_withassign : public std::ostream {
public:
    srOStream_withassign(std::streambuf* buffer);
};

extern SR_DLL_IMPORT class srOStream_withassign srDummyStream;
extern SR_DLL_IMPORT class srOStream_withassign srErr;
extern SR_DLL_IMPORT class srOStream_withassign srLog;
extern SR_DLL_IMPORT class srOStream_withassign srOut;
