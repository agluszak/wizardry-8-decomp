#include "surrender/srDebug.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "surrender/srCore.h"

/* The installed assert handler, zero-initialized in retail .data: nothing
   references it outside this translation unit. */
// GLOBAL: SURRENDER 0x100A0280
static srAssertHandler srAssertFunc = 0;

// FUNCTION: SURRENDER 0x10001000
void __cdecl srAssertSetFunc(srAssertHandler handler)
{
    srAssertFunc = handler;
}

// FUNCTION: SURRENDER 0x10001010
srAssertHandler __cdecl srAssertGetFunc()
{
    return srAssertFunc;
}

/* With no handler installed retail exits on the 0xdeadbabe code rather than
   reporting. The buffer clears its first byte even when message is 0 so the
   handler always receives a terminated string. */
// FUNCTION: SURRENDER 0x10001020
void __cdecl srAssertFail(const char* expression, const char* source_path, long line,
                          const char* message, ...)
{
    char buffer[0x800];
    if (srAssertFunc != 0) {
        buffer[0] = '\0';
        if (message != 0) {
            va_list args;
            va_start(args, message);
            _vsnprintf(buffer, 0x400, message, args);
            va_end(args);
        }
        srAssertFunc(expression, source_path, line, buffer);
        return;
    }
    exit(0xdeadbabe);
}

// FUNCTION: SURRENDER 0x10032EF0
srDummyStreamBuf::srDummyStreamBuf() {}

// FUNCTION: SURRENDER 0x10032FA0
int srDummyStreamBuf::overflow(int)
{
    return 0;
}

// FUNCTION: SURRENDER 0x10032FB0
int srDummyStreamBuf::underflow()
{
    return 0;
}

// FUNCTION: SURRENDER 0x10032FC0
srDummyStreamBuf& srDummyStreamBuf::operator=(const srDummyStreamBuf&)
{
    return *this;
}

/* The copy constructor reinitializes a fresh stream buffer rather than
   copying get/put state. */
// FUNCTION: SURRENDER 0x10032FD0
srDummyStreamBuf::srDummyStreamBuf(const srDummyStreamBuf&)
    : std::basic_streambuf<char, std::char_traits<char> >()
{
}

// FUNCTION: SURRENDER 0x100334D0
srOStream_withassign::srOStream_withassign(std::streambuf* buffer)
    : std::basic_ostream<char, std::char_traits<char> >(buffer)
{
}

/* Shared formatting sink the printf family routes through: a plain
   _vsnprintf passthrough with the 0x400-byte limit. */
// FUNCTION: SURRENDER 0x10033310
static long __cdecl srVsnprintf(char* buffer, unsigned long size, const char* format, va_list args)
{
    return _vsnprintf(buffer, size, format, args);
}

// FUNCTION: SURRENDER 0x10033330
long __cdecl srDebugPrintf(unsigned long level, const char* format, ...)
{
    if (level >= (srCore.debug_level_15c & 0xff)) {
        return 0;
    }
    if (format == 0) {
        return 0;
    }
    /* Retail frames are 0x404 bytes: the buffer carries four slack bytes
       past the 0x400 write cap. */
    char buffer[0x404];
    va_list args;
    va_start(args, format);
    long result = srVsnprintf(buffer, 0x400, format, args);
    va_end(args);
    srOut << buffer;
    return result;
}

// FUNCTION: SURRENDER 0x100333A0
long __cdecl srPrintf(const char* format, ...)
{
    if (format == 0) {
        return 0;
    }
    char buffer[0x404];
    va_list args;
    va_start(args, format);
    long result = srVsnprintf(buffer, 0x400, format, args);
    va_end(args);
    srOut << buffer;
    return result;
}

// FUNCTION: SURRENDER 0x100333F0
long __cdecl srStreamPrintf(std::ostream& stream, const char* format, ...)
{
    if (format != 0 && static_cast<void*>(&stream) != 0) {
        char buffer[0x404];
        va_list args;
        va_start(args, format);
        long result = srVsnprintf(buffer, 0x400, format, args);
        va_end(args);
        stream << buffer;
        return result;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10002E40
const char* __cdecl srBoolToString(int value)
{
    return value != 0 ? "true" : "false";
}

/* The stock handler SurRender ships for hosts that want a dialog. Retail
   links it in its own TU far from the assert setter/getter/fail cluster. */
// FUNCTION: SURRENDER 0x100466A0
void __cdecl srDefaultAssertFailFunc(const char* expression, const char* source_path, long line,
                                     const char* message)
{
    char buffer[0x800];
    if (message != 0 && *message != '\0') {
        _snprintf(buffer, 0x79b,
                  "Debug assertion in module %s line %d failed:\n\nExpression [ %s ] evaluates to "
                  "false.\n\n%s\n",
                  source_path, line, expression, message);
    } else {
        _snprintf(buffer, 0x79b,
                  "Debug assertion in module %s line %d failed:\n\nExpression [ %s ] evaluates to "
                  "false.\n",
                  source_path, line, expression);
    }
    strcat(buffer, "\nSelect 'yes' to trigger the debugger or 'no' to resume program execution.\n");
    if (MessageBoxA(GetActiveWindow(), buffer, "FATAL: SR Assertion Failed",
                    MB_YESNO | MB_ICONWARNING) == IDYES) {
        __asm int 3
    }
}

/* One shared dummy buffer backs all four provider streams; the retail CRT
   init order is buffer, srOut, srLog, srErr, srDummyStream. */
// GLOBAL: SURRENDER 0x100A47E0
static srDummyStreamBuf srDummyBuf;

// GLOBAL: SURRENDER 0x100A4888
// srOut
srOStream_withassign srOut(&srDummyBuf);

// GLOBAL: SURRENDER 0x100A4850
// srLog
srOStream_withassign srLog(&srDummyBuf);

// GLOBAL: SURRENDER 0x100A4818
// srErr
srOStream_withassign srErr(&srDummyBuf);

// GLOBAL: SURRENDER 0x100A47A0
// srDummyStream
srOStream_withassign srDummyStream(&srDummyBuf);

// SYNTHETIC: SURRENDER 0X100330D0
// srDummyStreamBuf scalar deleting destructor

// SYNTHETIC: SURRENDER 0X10033160
// srDummyStreamBuf global static-init block

// SYNTHETIC: SURRENDER 0X10033170
// srDummyStreamBuf global atexit registrar

// SYNTHETIC: SURRENDER 0X100331A0
// srOStream_withassign global static-init block

// SYNTHETIC: SURRENDER 0X100331C0
// srOStream_withassign global atexit registrar

// SYNTHETIC: SURRENDER 0X10033200
// srOStream_withassign global static-init block

// SYNTHETIC: SURRENDER 0X10033220
// srOStream_withassign global atexit registrar

// SYNTHETIC: SURRENDER 0X10033260
// srOStream_withassign global static-init block

// SYNTHETIC: SURRENDER 0X10033280
// srOStream_withassign global atexit registrar

// SYNTHETIC: SURRENDER 0X100332C0
// srOStream_withassign global static-init block

// SYNTHETIC: SURRENDER 0X100332E0
// srOStream_withassign global atexit registrar

// SYNTHETIC: SURRENDER 0X10033460
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10033470
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X100334A0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X100334B0
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X10046770
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X10046780
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X100467B0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X100467C0
// std::_Winit global atexit registrar
