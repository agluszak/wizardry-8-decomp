#pragma once

#include "srHeap.h"

class SR_DLL_IMPORT srBinStream {
public:
    enum e_state {
        SR_STREAM_OK = 0,
        SR_STREAM_ERROR = 1
    };
    enum e_seekDir {
        SR_SEEK_BEGIN = 0,
        SR_SEEK_CURRENT = 1,
        SR_SEEK_END = 2
    };

    // Slot order and pure-ness are read off the exported vftables in
    // evidence/snapshots/surrender-abi/vftable-slots.csv: ??_7srBinStream@@6B@
    // has five slots, of which 2 to 4 share one internal target - the
    // pure-virtual stub - while slot 1 holds a real srBinStream::getSize.
    virtual ~srBinStream() {}
    virtual unsigned long getSize();
    virtual srBinStream& seek(unsigned long position, e_seekDir direction) = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual unsigned long tell() = 0;

    void setState(e_state state);

    bool good() const;

protected:
    srBinStream();

private:
    // The ZIP extension's owned memory-stream subclass proves that the
    // virtual srBinStream subobject is 0x10 bytes: a vptr followed by 0x0c
    // bytes of SR.DLL-owned stream state.
    unsigned char unknown_04_[0x0c];
};

typedef char srBinStream_must_be_0x10[
    (sizeof(srBinStream) == 0x10) ? 1 : -1];

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
// ??_7srBinIStream@@6B0@@ has two slots: vget, which the library implements,
// and one holding the pure-virtual stub. Both are introduced here rather than
// inherited, because the destructor override lands in the srBinStream subobject
// table instead. The pure slot is what every reader supplies - srBinIMStream
// with its own vread, and Wizardry's virtual-file adapter with its.
class __declspec(novtable) srBinIStream : public virtual srBinStream {
public:
    virtual ~srBinIStream() override {}
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);

protected:
    virtual SR_DLL_IMPORT unsigned short vget();

private:
    virtual unsigned long vread(void* destination, unsigned long size) = 0;
};

class __declspec(novtable) srBinIMStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIMStream(const void* data, unsigned long size);
    virtual unsigned long getSize() override;
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size) override;
    // The imported implementation owns the prefix through +0x13. The ZIP
    // extension's zero-data wrapper adds its malloc owner at +0x14 and places
    // the virtual srBinStream base at +0x1c.
    unsigned char unknown_08_[0x0c];
};
