#pragma once

#include "srHeap.h"

/* The provider exports the full member surface including the implicit copy
   constructor/assignment and the vftable, so the declaration is dllexport
   under SURRENDER_BUILD; consumers keep the class-wide import. */
// VTABLE: SURRENDER 0x10076970 srBinStream
// class srBinStream
#if defined(SURRENDER_BUILD)
class __declspec(dllexport) srBinStream {
#else
class SR_DLL_IMPORT srBinStream {
#endif
public:
    enum e_state { SR_STREAM_OK = 0, SR_STREAM_ERROR = 1, SR_STREAM_STATE_2 = 2 };

    enum e_seekDir { SR_SEEK_BEGIN = 0, SR_SEEK_CURRENT = 1, SR_SEEK_END = 2 };

    enum e_byteOrder { SR_BYTE_ORDER_0 = 0, SR_BYTE_ORDER_1 = 1 };

    /* Thrown by setState when exceptions are enabled and the stream enters the
       error state. Retail RTTI shows a one-byte type carrying the state. */
    class Failure {
    public:
        Failure(e_state state) : state_00(static_cast<char>(state)) {}
        char state_00;
    };

    virtual ~srBinStream();
    virtual unsigned long getSize();
    virtual srBinStream& seek(unsigned long position, e_seekDir direction) = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual unsigned long tell() = 0;

    void clear();
    bool exceptions(bool enabled);
    e_byteOrder getByteOrder() const;
    bool good() const;
    bool operator!() const;
    operator void*() const;
    void setByteOrder(e_byteOrder byte_order);
    void setState(e_state state);

    /* Implicit copy constructor/assignment: retail emits them via the
       class-level dllexport; the bodies are plain memberwise copies. */
    // SYNTHETIC: SURRENDER 0x10032240
    // srBinStream::srBinStream
    // SYNTHETIC: SURRENDER 0x10032280
    // srBinStream::operator=
    // SYNTHETIC: SURRENDER 0x100322E0
    // srBinStream::`vector deleting destructor'

protected:
    srBinStream();

    bool byteOrderMatch() const;
    static void byteSwap(unsigned char* data, int size);

private:
    e_state state_04;
    bool exceptions_08;
    unsigned char padding_09_[3];
    e_byteOrder byte_order_0c;
};

static_assert(sizeof(srBinStream) == 0x10, "srBinStream_must_be_0x10");
