#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"

#include <stdio.h>

class srBinFStream : public virtual srBinStream {
public:
    SR_DLL_IMPORT srBinFStream(const srBinFStream& stream);
    SR_DLL_IMPORT srBinFStream& operator=(const srBinFStream& stream);

    SR_DLL_IMPORT void close();
    SR_DLL_IMPORT const char* getPath() const;
    SR_DLL_IMPORT int isOpen();

protected:
    enum e_mode {
        SR_MODE_READ = 0,
        SR_MODE_WRITE = 1,
        SR_MODE_READ_WRITE = 2
    };

    SR_DLL_IMPORT srBinFStream();
    virtual SR_DLL_IMPORT ~srBinFStream() override;

    SR_DLL_IMPORT void mopen(const char* path, e_mode mode, int search_paths);
    virtual SR_DLL_IMPORT srBinStream& pseek(unsigned long position);
    virtual SR_DLL_IMPORT srBinStream& pseek(
        unsigned long position, srBinStream::e_seekDir direction);
    virtual SR_DLL_IMPORT unsigned long ptell();

private:
    SR_DLL_IMPORT void setPath(const char* path);

    FILE* file_08;
    char empty_path_0c;
    unsigned long path_size_10;
    char* path_14;
};

class srBinIFStream : public srBinFStream, public srBinIStream {
public:
    SR_DLL_IMPORT srBinIFStream();
    SR_DLL_IMPORT srBinIFStream(const char* path);
    SR_DLL_IMPORT srBinIFStream(const srBinIFStream& stream);
    virtual SR_DLL_IMPORT ~srBinIFStream() override;
    SR_DLL_IMPORT srBinIFStream& operator=(const srBinIFStream& stream);

    SR_DLL_IMPORT void open(const char* path);
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned short vget() override;
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size) override;
};

class srBinIOFStream : public srBinFStream,
                       public srBinIStream,
                       public srBinOStream {
public:
    SR_DLL_IMPORT srBinIOFStream();
    SR_DLL_IMPORT srBinIOFStream(const char* path);
    SR_DLL_IMPORT srBinIOFStream(const srBinIOFStream& stream);
    virtual SR_DLL_IMPORT ~srBinIOFStream() override;
    SR_DLL_IMPORT srBinIOFStream& operator=(const srBinIOFStream& stream);

    SR_DLL_IMPORT void open(const char* path);
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned short vget() override;
    virtual SR_DLL_IMPORT unsigned short vput(char value) override;
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size) override;
    virtual SR_DLL_IMPORT unsigned long vwrite(
        const void* source, unsigned long size) override;
};

class srBinOFStream : public virtual srBinOStream,
                      public virtual srBinFStream {
public:
    SR_DLL_IMPORT srBinOFStream();
    SR_DLL_IMPORT srBinOFStream(const char* path);
    SR_DLL_IMPORT srBinOFStream(const srBinOFStream& stream);
    virtual SR_DLL_IMPORT ~srBinOFStream() override;
    SR_DLL_IMPORT srBinOFStream& operator=(const srBinOFStream& stream);

    SR_DLL_IMPORT void open(const char* path);
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned short vput(char value) override;
    virtual SR_DLL_IMPORT unsigned long vwrite(
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
