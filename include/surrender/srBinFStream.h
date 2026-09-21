#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"

#include <stdio.h>

/* SR-owned file-stream family. Its classes and virtual tables are visible in
   the provider ABI, but no known Wizardry/JPEG/ZIP consumer imports a
   srBinFStream/srBinIFStream/srBinIOFStream/srBinOFStream symbol. */
class srBinFStream : public virtual srBinStream {
public:
    srBinFStream(const srBinFStream& stream);
    srBinFStream& operator=(const srBinFStream& stream);

    void close();
    const char* getPath() const;
    int isOpen();

protected:
    enum e_mode {
        SR_MODE_READ = 0,
        SR_MODE_WRITE = 1,
        SR_MODE_READ_WRITE = 2
    };

    srBinFStream();
    virtual ~srBinFStream() override;

    void mopen(const char* path, e_mode mode, int search_paths);
    virtual srBinStream& pseek(unsigned long position);
    virtual srBinStream& pseek(
        unsigned long position, srBinStream::e_seekDir direction);
    virtual unsigned long ptell();

private:
    void setPath(const char* path);

    FILE* file_08;
    char empty_path_0c;
    unsigned long path_size_10;
    char* path_14;
};

class srBinIFStream : public srBinFStream, public srBinIStream {
public:
    srBinIFStream();
    srBinIFStream(const char* path);
    srBinIFStream(const srBinIFStream& stream);
    virtual ~srBinIFStream() override;
    srBinIFStream& operator=(const srBinIFStream& stream);

    void open(const char* path);
    virtual srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    virtual unsigned short vget() override;
    virtual unsigned long vread(
        void* destination, unsigned long size) override;
};

class srBinIOFStream : public srBinFStream,
                       public srBinIStream,
                       public srBinOStream {
public:
    srBinIOFStream();
    srBinIOFStream(const char* path);
    srBinIOFStream(const srBinIOFStream& stream);
    virtual ~srBinIOFStream() override;
    srBinIOFStream& operator=(const srBinIOFStream& stream);

    void open(const char* path);
    virtual srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    virtual unsigned short vget() override;
    virtual unsigned short vput(char value) override;
    virtual unsigned long vread(
        void* destination, unsigned long size) override;
    virtual unsigned long vwrite(
        const void* source, unsigned long size) override;
};

class srBinOFStream : public virtual srBinOStream,
                      public virtual srBinFStream {
public:
    srBinOFStream();
    srBinOFStream(const char* path);
    srBinOFStream(const srBinOFStream& stream);
    virtual ~srBinOFStream() override;
    srBinOFStream& operator=(const srBinOFStream& stream);

    void open(const char* path);
    virtual srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    virtual unsigned short vput(char value) override;
    virtual unsigned long vwrite(
        const void* source, unsigned long size) override;
};

static_assert(sizeof(srBinFStream) == 0x28,
              "srBinFStream_must_be_0x28");
static_assert(sizeof(srBinIFStream) == 0x34,
              "srBinIFStream_must_be_0x34");
static_assert(sizeof(srBinIOFStream) == 0x3c,
              "srBinIOFStream_must_be_0x3c");
static_assert(sizeof(srBinOFStream) == 0x3c,
              "srBinOFStream_must_be_0x3c");
