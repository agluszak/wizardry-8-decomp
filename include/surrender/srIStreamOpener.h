#pragma once

#include "srBinIStream.h"

class srIStreamOpener {
public:
    class __declspec(novtable) Opener {
    public:
        SR_DLL_IMPORT Opener();
        virtual SR_DLL_IMPORT ~Opener();
        SR_DLL_IMPORT Opener& operator=(const Opener& other);

        virtual srBinIStream* open(const char* path) = 0;
        virtual const char* getDescription() const = 0;
    };

    SR_DLL_IMPORT srIStreamOpener();
    SR_DLL_IMPORT ~srIStreamOpener();
    SR_DLL_IMPORT srIStreamOpener& operator=(const srIStreamOpener& other);

    SR_DLL_IMPORT void addStreamType(Opener* opener, const char* extension);
    SR_DLL_IMPORT srBinIStream* open(const char* path);

private:
    struct StreamType {
        Opener* opener_00;
        char* extension_04;
        StreamType* next_08;
        StreamType* previous_0c;
    };

    static_assert(sizeof(StreamType) == 0x10,
                  "srIStreamOpener_StreamType_must_be_0x10");

    SR_DLL_IMPORT Opener* findOpener(const char* extension);
    SR_DLL_IMPORT srBinIStream* open(
        const char* path, const char* extension);
    SR_DLL_IMPORT void parsePrefix(
        char** prefix, char** path, const char* input);

    long count_00;
    StreamType* first_04;
    StreamType* end_08;
};

static_assert(sizeof(srIStreamOpener::Opener) == 0x04,
              "srIStreamOpener_Opener_must_be_0x04");
static_assert(sizeof(srIStreamOpener) == 0x0c,
              "srIStreamOpener_must_be_0x0c");
