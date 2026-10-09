#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srString.h"

#include <stdio.h>

// VTABLE: SURRENDER 0x10076A40 srBinStream
// VTABLE: SURRENDER 0x10076A54 srBinFStream
// class srBinFStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srBinFStream : public virtual srBinStream {
public:
    void close();
    const char* getPath() const;
    int isOpen();

protected:
    enum e_mode { SR_MODE_READ = 0, SR_MODE_WRITE = 1, SR_MODE_READ_WRITE = 2 };

    srBinFStream();
    virtual ~srBinFStream() override;

    void mopen(const char* path, e_mode mode, int search_paths);
    virtual srBinStream& pseek(w8_ulong position, srBinStream::e_seekDir direction);
    virtual srBinStream& pseek(w8_ulong position);
    virtual w8_ulong ptell();

    /* The directional file streams' vget/vput/vread/vwrite bodies all touch
       the file handle directly, so the member sits at protected access. */
    FILE* file;

private:
    void setPath(const char* path);

    srInlineString path;
};

// VTABLE: SURRENDER 0x10076A70 srBinStream
// VTABLE: SURRENDER 0x10076A84 srBinIStream
// VTABLE: SURRENDER 0x10076A8C srBinFStream
// class srBinIFStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srBinIFStream : public srBinFStream,
                    public srBinIStream {
public:
    srBinIFStream();
    srBinIFStream(const char* path);

    void open(const char* path);
    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;

private:
    virtual unsigned short vget() override;
    virtual w8_ulong vread(void* destination, w8_ulong size) override;
};

// VTABLE: SURRENDER 0x10076AB8 srBinStream
// VTABLE: SURRENDER 0x10076ACC srBinOStream
// VTABLE: SURRENDER 0x10076AD4 srBinIStream
// VTABLE: SURRENDER 0x10076ADC srBinFStream
// class srBinIOFStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srBinIOFStream : public srBinFStream,
                     public srBinIStream,
                     public srBinOStream {
public:
    srBinIOFStream();
    srBinIOFStream(const char* path);

    void open(const char* path);
    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;

private:
    virtual unsigned short vget() override;
    virtual unsigned short vput(char value) override;
    virtual w8_ulong vread(void* destination, w8_ulong size) override;
    virtual w8_ulong vwrite(const void* source, w8_ulong size) override;
};

// VTABLE: SURRENDER 0x10076B30 srBinFStream
// VTABLE: SURRENDER 0x10076B3C srBinOStream
// VTABLE: SURRENDER 0x10076B44 srBinStream
// class srBinOFStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srBinOFStream : public virtual srBinOStream,
                    public virtual srBinFStream {
public:
    srBinOFStream();
    srBinOFStream(const char* path);

    void open(const char* path);
    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;

private:
    virtual unsigned short vput(char value) override;
    virtual w8_ulong vwrite(const void* source, w8_ulong size) override;
};

W8_ABI_ASSERT(sizeof(srBinFStream) == 0x28, "srBinFStream_must_be_0x28");
W8_ABI_ASSERT(sizeof(srBinIFStream) == 0x34, "srBinIFStream_must_be_0x34");
W8_ABI_ASSERT(sizeof(srBinIOFStream) == 0x3c, "srBinIOFStream_must_be_0x3c");
W8_ABI_ASSERT(sizeof(srBinOFStream) == 0x3c, "srBinOFStream_must_be_0x3c");
