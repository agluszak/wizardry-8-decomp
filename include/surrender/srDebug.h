#pragma once

#include <ostream>
#include "srHeap.h"

// Export-proven declarations; this grouping's original header name is unknown.
SR_DLL_IMPORT long __cdecl srDebugPrintf(unsigned long level, const char* format, ...);
SR_DLL_IMPORT long __cdecl srPrintf(const char* format, ...);
SR_DLL_IMPORT long __cdecl srStreamPrintf(std::ostream& stream, const char* format, ...);
/* Exported diagnostic helper; srMaterial::dump prints the dirty flag through
   it. The owning TU is unknown. */
SR_DLL_IMPORT const char* __cdecl srBoolToString(int value);

/* The provider-facing declaration keeps the variadic export's ZZ mangling;
   consumers see the fixed-arity form declared in wiz8's sr_api.h. */
SR_DLL_IMPORT void __cdecl srAssertFail(const char* expression, const char* source_path, long line,
                                        const char* message, ...);

typedef void(__cdecl* srAssertHandler)(const char* expression, const char* source_path, long line,
                                       const char* message);

SR_DLL_IMPORT srAssertHandler __cdecl srAssertGetFunc();
SR_DLL_IMPORT void __cdecl srAssertSetFunc(srAssertHandler handler);
SR_DLL_IMPORT void __cdecl srDefaultAssertFailFunc(const char* expression, const char* source_path,
                                                   long line, const char* message);

/* Sink that discards every insertion: overflow/underflow are the only
   provider-owned virtuals on the retail vtable. The class-level export is
   retail-proven: the export table carries the implicit destructor
   (??_1srDummyStreamBuf@@UAE@XZ) and the vftable slot 0 is a real vector
   deleting destructor (??_E), which VC6 only emits for a class declared
   __declspec(dllexport). No known consumer imports it. */
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

    /* Exported as private members (AAE/EAE mangling): the copy constructor
       reinitializes a fresh stream buffer rather than copying get/put state. */
    srDummyStreamBuf(const srDummyStreamBuf& other);
    srDummyStreamBuf& operator=(const srDummyStreamBuf& other);
};

/* No declared destructor: the retail emission stores the imported
   basic_streambuf vftable directly, the implicit-destroyer shape. */
// SYNTHETIC: SURRENDER 0x10033080
// srDummyStreamBuf::~srDummyStreamBuf
// SYNTHETIC: SURRENDER 0x100330F0
// srDummyStreamBuf::`vector deleting destructor'

/* basic_ostream<char>-shaped provider stream (0x38 bytes: vbptr +
   basic_ios<char>). No own virtuals; the retail vtable carries only the
   deleting destructor slot. */
// VTABLE: SURRENDER 0x10076C34 srOStream_withassign
class srOStream_withassign : public std::ostream {
public:
    srOStream_withassign(std::streambuf* buffer);
};

/* Implicit destructor: the retail emission stores only the imported
   basic_ostream vftable into the virtual base. */
// SYNTHETIC: SURRENDER 0x100335C0
// srOStream_withassign::~srOStream_withassign
// SYNTHETIC: SURRENDER 0x10033590
// srOStream_withassign::`scalar deleting destructor'

extern SR_DLL_IMPORT class srOStream_withassign srDummyStream;
extern SR_DLL_IMPORT class srOStream_withassign srErr;
extern SR_DLL_IMPORT class srOStream_withassign srLog;
extern SR_DLL_IMPORT class srOStream_withassign srOut;
