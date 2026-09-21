#pragma once

#include "srHeap.h"

class SR_DLL_IMPORT srBinStream {
public:
    enum e_state {
        SR_STREAM_OK = 0,
        SR_STREAM_ERROR = 1,
        SR_STREAM_STATE_2 = 2
    };

    enum e_seekDir {
        SR_SEEK_BEGIN = 0,
        SR_SEEK_CURRENT = 1,
        SR_SEEK_END = 2
    };

    enum e_byteOrder {
        SR_BYTE_ORDER_0 = 0,
        SR_BYTE_ORDER_1 = 1
    };

    srBinStream(const srBinStream& stream);
    virtual ~srBinStream() {}
    srBinStream& operator=(const srBinStream& stream);

    virtual unsigned long getSize();
    virtual srBinStream& seek(
        unsigned long position, e_seekDir direction) = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual unsigned long tell() = 0;

    void clear();
    bool exceptions(bool enabled);
    e_byteOrder getByteOrder() const;
    bool good() const;
    operator void*() const;
    void setByteOrder(e_byteOrder byte_order);
    void setState(e_state state);

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

static_assert(sizeof(srBinStream) == 0x10,
              "srBinStream_must_be_0x10");
