#include "surrender/srDebug.h"

#include <ostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "surrender/srCore.h"

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

/* With no handler installed the process exits with code 0xdeadbabe. The buffer is terminated even
   when message is 0. */
// FUNCTION: SURRENDER 0x10001020
void __cdecl srAssertFail(const char* expression, const char* source_path, w8_long line,
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

// FUNCTION: SURRENDER 0x10033310
static w8_long __cdecl srVsnprintf(char* buffer, w8_ulong size, const char* format, va_list args)
{
    return _vsnprintf(buffer, size, format, args);
}

// FUNCTION: SURRENDER 0x10033330
w8_long __cdecl srDebugPrintf(w8_ulong level, const char* format, ...)
{
    if (level >= (srCore.debug_level & 0xff)) {
        return 0;
    }
    if (format == 0) {
        return 0;
    }
    char buffer[0x404];
    va_list args;
    va_start(args, format);
    w8_long result = srVsnprintf(buffer, 0x400, format, args);
    va_end(args);
    srOut << buffer;
    return result;
}

// FUNCTION: SURRENDER 0x100333A0
w8_long __cdecl srPrintf(const char* format, ...)
{
    if (format == 0) {
        return 0;
    }
    char buffer[0x404];
    va_list args;
    va_start(args, format);
    w8_long result = srVsnprintf(buffer, 0x400, format, args);
    va_end(args);
    srOut << buffer;
    return result;
}

// FUNCTION: SURRENDER 0x100333F0
w8_long __cdecl srStreamPrintf(std::ostream& stream, const char* format, ...)
{
    if (format != 0 && static_cast<void*>(&stream) != 0) {
        char buffer[0x404];
        va_list args;
        va_start(args, format);
        w8_long result = srVsnprintf(buffer, 0x400, format, args);
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

/* The stock handler SurRender ships for hosts that want a dialog. */
// FUNCTION: SURRENDER 0x100466A0
void __cdecl srDefaultAssertFailFunc(const char* expression, const char* source_path, w8_long line,
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

/* One shared dummy buffer backs all four provider streams. */
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
