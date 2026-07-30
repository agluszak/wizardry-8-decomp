#pragma once

#include "srBinIStream.h"

class srIStreamOpener {
public:
    class __declspec(novtable) Opener {
    public:
        virtual SR_DLL_IMPORT ~Opener() {}
        virtual srBinIStream* open(const char* path) = 0;
        virtual const char* getDescription() const = 0;
    };

    SR_DLL_IMPORT void addStreamType(Opener* opener, const char* extension);
};
