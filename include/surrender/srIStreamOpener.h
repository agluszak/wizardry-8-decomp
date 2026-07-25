#pragma once

#include "srBinIStream.h"

class srIStreamOpener {
public:
    class Opener {
    public:
        virtual SR_DLL_IMPORT ~Opener();
        virtual srBinIStream* open(char* path) = 0;
        virtual const char* getDescription() const = 0;
    };

    SR_DLL_IMPORT void addStreamType(Opener* opener, const char* extension);
};
