#pragma once

#include "srBinFStream.h"

/* Retail exports every declared srIStreamOpener member, including the private
   helpers (AAE mangling), but no copy constructor and no Opener vftable
   (??_7Opener@srIStreamOpener@@6B@ is absent). Consumers import the whole
   declared surface member by member, so the declaration itself stays unimported. */
class srIStreamOpener {
public:
    class __declspec(novtable) Opener {
    public:
        /* srInit inlines the trivial construction through srFStreamOpener.
           The member dllexport keeps the header body for that folding while
           still emitting the exported standalone copy consumers import. */
        // FUNCTION: SURRENDER 0x10032680
        // ??0Opener@srIStreamOpener@@QAE@XZ
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Opener()
        {
        }
        virtual SR_DLL_IMPORT ~Opener();
        SR_DLL_IMPORT Opener& operator=(const Opener& other);

        virtual srBinIStream* open(const char* path) = 0;
        virtual const char* getDescription() const = 0;
    };

    /* srInit inlines the constructor including its sentinel allocation.
       The member dllexport keeps the header body for that folding while
       still emitting the exported standalone copy consumers import. */
    // FUNCTION: SURRENDER 0x100326B0
    // ??0srIStreamOpener@@QAE@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srIStreamOpener()
    {
        first_04 = new StreamType;
        end_08 = first_04;
        first_04->next_08 = 0;
        first_04->previous_0c = 0;
        count_00 = 0;
    }
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

    static_assert(sizeof(StreamType) == 0x10, "srIStreamOpener_StreamType_must_be_0x10");

    SR_DLL_IMPORT Opener* findOpener(const char* extension);
    SR_DLL_IMPORT srBinIStream* open(const char* path, const char* extension);
    SR_DLL_IMPORT void parsePrefix(char** prefix, char** path, const char* input);

    long count_00;
    StreamType* first_04;
    StreamType* end_08;
};

static_assert(sizeof(srIStreamOpener::Opener) == 0x04, "srIStreamOpener_Opener_must_be_0x04");
static_assert(sizeof(srIStreamOpener) == 0x0c, "srIStreamOpener_must_be_0x0c");

/* SR's built-in file opener is provider-owned. Consumers use the imported
   srIStreamOpener surface; no known consumer imports srFStreamOpener itself.
   The exported vftable (??_7srFStreamOpener@@6B@) is emitted by the provider
   where the lifecycle bodies live, in stream.cpp (the destructor emission is
   at retail address 0x10016850, in the unit that instantiates the opener). */
// VTABLE: SURRENDER 0x10075520 srFStreamOpener
class srFStreamOpener : public srIStreamOpener::Opener {
public:
    /* srInit inlines the trivial construction. The member dllexport keeps the
       header body for that folding while still emitting the exported
       standalone copy. */
    // FUNCTION: SURRENDER 0x10032440
    // ??0srFStreamOpener@@QAE@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srFStreamOpener()
    {
    }
    srFStreamOpener& operator=(const srFStreamOpener& other);

    virtual srBinIStream* open(const char* path) override;
    virtual const char* getDescription() const override;
};

static_assert(sizeof(srFStreamOpener) == 0x04, "srFStreamOpener_must_be_0x04");
