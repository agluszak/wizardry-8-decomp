#pragma once

#include "srBinFStream.h"

class SR_DLL_EXPORT srIStreamOpener {
public:
    class __declspec(novtable) Opener {
    public:
        // FUNCTION: SURRENDER 0x10032680
        // RECOMP: ??0Opener@srIStreamOpener@@QAE@XZ
        SR_DLL_EXPORT Opener() {}
        // FUNCTION: SURRENDER 0x10032690
        // RECOMP: ??1Opener@srIStreamOpener@@UAE@XZ
        virtual SR_DLL_EXPORT ~Opener() {}
        SR_DLL_IMPORT Opener& operator=(const Opener& other);

        virtual srBinIStream* open(const char* path) = 0;
        virtual const char* getDescription() const = 0;
    };

    // FUNCTION: SURRENDER 0x100326B0
    // RECOMP: ??0srIStreamOpener@@QAE@XZ
    srIStreamOpener()
    {
        first = new StreamType;
        end = first;
        first->next = 0;
        first->previous = 0;
        count = 0;
    }
    SR_DLL_IMPORT ~srIStreamOpener();
#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srIStreamOpener& operator=(const srIStreamOpener& other);
#endif

    SR_DLL_IMPORT void addStreamType(Opener* opener, const char* extension);
    SR_DLL_IMPORT srBinIStream* open(const char* path);

private:
    struct StreamType {
        Opener* opener;
        char* extension;
        StreamType* next;
        StreamType* previous;
    };

    static_assert(sizeof(StreamType) == 0x10, "srIStreamOpener_StreamType_must_be_0x10");

    SR_DLL_IMPORT Opener* findOpener(const char* extension);
    SR_DLL_IMPORT srBinIStream* open(const char* path, const char* extension);
    SR_DLL_IMPORT void parsePrefix(char** prefix, char** path, const char* input);

    long count;
    StreamType* first;
    StreamType* end;
};

static_assert(sizeof(srIStreamOpener::Opener) == 0x04, "srIStreamOpener_Opener_must_be_0x04");
static_assert(sizeof(srIStreamOpener) == 0x0c, "srIStreamOpener_must_be_0x0c");

/* SR's built-in file opener. */
// VTABLE: SURRENDER 0x10075520 srFStreamOpener
class SR_DLL_EXPORT srFStreamOpener : public srIStreamOpener::Opener {
public:
    // FUNCTION: SURRENDER 0x10032440
    // RECOMP: ??0srFStreamOpener@@QAE@XZ
    srFStreamOpener() {}

    virtual srBinIStream* open(const char* path) override;
    virtual const char* getDescription() const override;
};

static_assert(sizeof(srFStreamOpener) == 0x04, "srFStreamOpener_must_be_0x04");
