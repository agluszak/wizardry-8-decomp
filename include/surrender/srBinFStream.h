#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srString.h"

#include <stdio.h>

/* SR-owned file-stream family. The path member is the provider's
   srInlineString (the ctor/dtor/setPath/mopen bodies show its inline empty
   state, srHeap-backed growth and release). Its classes and virtual tables
   are visible in the provider ABI, but no known Wizardry/JPEG/ZIP consumer
   imports a srBinFStream/srBinIFStream/srBinIOFStream/srBinOFStream symbol;
   the provider export reproduces the retail emissions. */
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

    /* Implicit copy constructor/assignment: retail emits them via the
       class-level dllexport as memberwise copies. file_08 aliases the
       source's FILE* — two live copies fclose the same stream and
       assignment leaks the destination's open handle — while path_0c
       deep-copies, so copying an open stream is unsafe. Genuine but
       unreachable retail behavior: no consumer imports the family. */
    // SYNTHETIC: SURRENDER 0x1002F430
    // srBinFStream::srBinFStream
    // SYNTHETIC: SURRENDER 0x1002F530
    // srBinFStream::operator=
    // SYNTHETIC: SURRENDER 0x1002F5F0
    // srBinFStream::`vbase destructor'

protected:
    enum e_mode { SR_MODE_READ = 0, SR_MODE_WRITE = 1, SR_MODE_READ_WRITE = 2 };

    srBinFStream();
    virtual ~srBinFStream() override;

    void mopen(const char* path, e_mode mode, int search_paths);
    virtual srBinStream& pseek(unsigned long position, srBinStream::e_seekDir direction);
    virtual srBinStream& pseek(unsigned long position);
    virtual unsigned long ptell();

    /* The directional file streams' vget/vput/vread/vwrite bodies all touch
       the file handle directly, so the member sits at protected access. */
    FILE* file_08;

private:
    void setPath(const char* path);

    srInlineString path_0c;
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
    virtual ~srBinIFStream() override;

    /* The retail emission is a bare ret while the vbase destructor owns the
       table stores; the body is defined in stream.cpp. */

    void open(const char* path);
    virtual srBinStream& seek(unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    /* Implicit copy constructor/assignment, vtordisp thunk and vbase
       destructor: emitted via the class-level dllexport as memberwise
       copies. They inherit srBinFStream's unsafe file_08 FILE* aliasing;
       copying an open stream is not safe value semantics. */
    // SYNTHETIC: SURRENDER 0x1002F8D0
    // srBinIFStream::srBinIFStream
    // SYNTHETIC: SURRENDER 0x1002FA20
    // srBinIFStream::operator=
    // SYNTHETIC: SURRENDER 0x1002E480
    // srBinIFStream::~srBinIFStream (vtordisp adjustor thunk)
    // SYNTHETIC: SURRENDER 0x1002E460
    // srBinIFStream::`vbase destructor'

    virtual unsigned short vget() override;
    virtual unsigned long vread(void* destination, unsigned long size) override;
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
    virtual ~srBinIOFStream() override;

    /* The retail emission is a bare ret while the vbase destructor owns the
       table stores; the body is defined in stream.cpp. */

    void open(const char* path);
    virtual srBinStream& seek(unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    /* Implicit copy constructor/assignment, vtordisp thunk and vbase
       destructor: emitted via the class-level dllexport as memberwise
       copies. They inherit srBinFStream's unsafe file_08 FILE* aliasing;
       copying an open stream is not safe value semantics. */
    // SYNTHETIC: SURRENDER 0x1002FF00
    // srBinIOFStream::srBinIOFStream
    // SYNTHETIC: SURRENDER 0x10030070
    // srBinIOFStream::operator=
    // SYNTHETIC: SURRENDER 0x100301E0
    // srBinIOFStream::~srBinIOFStream (vtordisp adjustor thunk)
    // SYNTHETIC: SURRENDER 0x100301F0
    // srBinIOFStream::`vbase destructor'

    virtual unsigned short vget() override;
    virtual unsigned short vput(char value) override;
    virtual unsigned long vread(void* destination, unsigned long size) override;
    virtual unsigned long vwrite(const void* source, unsigned long size) override;
};

/* Unlike the input family, srBinOFStream virtually inherits both directional
   interfaces: the retail object carries its own vbptr at +0, the shared
   srBinStream vbase at +8, the srBinOStream subobject at +0x1c and the
   srBinFStream subobject at +0x24. */
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
    virtual ~srBinOFStream() override;

    /* The retail emission is a bare ret while the vbase destructor owns the
       table stores; the body is defined in stream.cpp. */

    void open(const char* path);
    virtual srBinStream& seek(unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    /* Implicit copy constructor/assignment and vbase destructor: emitted via
       the class-level dllexport as memberwise copies. They inherit
       srBinFStream's unsafe file_08 FILE* aliasing; copying an open stream
       is not safe value semantics. */
    // SYNTHETIC: SURRENDER 0x100305D0
    // srBinOFStream::srBinOFStream
    // SYNTHETIC: SURRENDER 0x10030760
    // srBinOFStream::operator=
    // SYNTHETIC: SURRENDER 0x1002D640
    // srBinOFStream::`vbase destructor'

    virtual unsigned short vput(char value) override;
    virtual unsigned long vwrite(const void* source, unsigned long size) override;
};

static_assert(sizeof(srBinFStream) == 0x28, "srBinFStream_must_be_0x28");
static_assert(sizeof(srBinIFStream) == 0x34, "srBinIFStream_must_be_0x34");
static_assert(sizeof(srBinIOFStream) == 0x3c, "srBinIOFStream_must_be_0x3c");
static_assert(sizeof(srBinOFStream) == 0x3c, "srBinOFStream_must_be_0x3c");
