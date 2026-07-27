#pragma once

#include "srHeap.h"

class srClass;
class srVertexPipe;

/* The thirteen virtual slots are the exported ??_7srMaterial@@6B@ in slot
   order, from evidence/snapshots/surrender-abi/vftable-slots.csv. Eight of them
   the library exports by name and a derived class reaches through an import
   thunk; the other five it implements without exporting, and every first-party
   subclass overrides all five, so nothing here needs a body the DLL will not
   supply.

   Two simplifications are deliberate and neither moves a slot. `dump` really
   takes `std::basic_ostream<char, std::char_traits<char> >&` and `verify` an
   `srRuntimeClass::e_verify`; modelling MSVCP60's stream types is a separate
   problem, and a parameter type cannot change a vtable's layout. `getMaterialInfo`
   likewise takes `srVertexProcessor::MaterialInfo&`.

   The size is the extent the first-party subclass leaves for it:
   Engine Code\materials.cpp's stMaterial is 0x7C bytes with its own field at
   0x78, and the srMaterial construction inlined into its constructor writes
   0x68 and 0x6C. Nothing here proves what fills those bytes. */
class SR_DLL_IMPORT srMaterial {
public:
    virtual const char* vslot0();                               /* 0 */
    virtual unsigned long vslot1();                             /* 1 */
    virtual void* vslot2();                                     /* 2 */
    virtual void dump(void* stream);                            /* 3 */
    virtual void verify(int check);                             /* 4 */
    virtual void vslot5();                                      /* 5 */
    virtual srClass* vInstance();                               /* 6 */
    virtual srMaterial* vslot7();                               /* 7 */
    virtual void getMaterialInfo(void* info);                   /* 8 */
    virtual void preProcess(srVertexPipe& pipe);                /* 9 */
    virtual void postProcess(srVertexPipe& pipe);               /* 10 */

protected:
    virtual void updateParms();                                 /* 11 */
    virtual void reset();                                       /* 12 */

public:
    srMaterial& operator=(const srMaterial& other);

private:
    unsigned char unknown_04_[0x74];
};

typedef char srMaterial_must_be_0x78[(sizeof(srMaterial) == 0x78) ? 1 : -1];
