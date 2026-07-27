#pragma once

#include "srHeap.h"
#include "srQuadWord.h"

/* The eleven virtual slots are the exported ??_7srTimer@@6B@ in slot order,
   from evidence/snapshots/surrender-abi/vftable-slots.csv: the destructor in
   slot 0, then the ten methods the library exports by name. Wizardry imports
   the (int,int,int) constructor and the destructor, so a derived class needs
   no body the DLL will not supply.

   The enum's values are not established; the one observed call site passes 0.

   The extent is asserted at 0x810, and that number is bounded rather than
   proven: the shared derived timer is allocated at 0x868, the 64-bit value at
   +0x808 is read before any first-party write, so it must be initialized by
   this constructor and lie inside the base, while the fields at +0x828 and
   +0x838 are first written by first-party code and are modelled as the derived
   class's. Any boundary in 0x810..0x828 produces the same bytes; this is the
   smallest one covering the frequency. */
class srTimer {
public:
    enum e_timerReadControl {
        TIMER_READ_DEFAULT = 0
    };

    SR_DLL_IMPORT srTimer(int argument_0, int argument_1, int argument_2);

    virtual SR_DLL_IMPORT ~srTimer();                                /* 0 */
    virtual SR_DLL_IMPORT char* getAscTime(char* buffer, e_timerReadControl control);
    virtual SR_DLL_IMPORT int pause();                                            /* 2 */
    virtual SR_DLL_IMPORT unsigned long resume();                                 /* 3 */
    virtual SR_DLL_IMPORT int reset(int argument_0, int argument_1, int argument_2);
    virtual SR_DLL_IMPORT unsigned long getMsTime(e_timerReadControl control);    /* 5 */
    virtual SR_DLL_IMPORT double getTime(e_timerReadControl control);             /* 6 */
    /* Slots 7/9 take srQuadWord&, slots 8/10 take only the control. MSVC lays
       an adjacent virtual overload group out in reverse declaration order, so
       the source declares each pair reversed to land them as exported. */
    virtual SR_DLL_IMPORT unsigned long getUTime(e_timerReadControl control);     /* 8 */
    virtual SR_DLL_IMPORT unsigned long getUTime(srQuadWord& out, e_timerReadControl control);
    virtual SR_DLL_IMPORT unsigned long getRawTime(e_timerReadControl control);   /* 10 */
    virtual SR_DLL_IMPORT unsigned long getRawTime(srQuadWord& out, e_timerReadControl control);

    unsigned char unknown_004_[0x804];
    /* Ticks per second, measured by the constructor. */
    srQuadWord m_frequency;              /* 0x808 */
};

typedef char srTimer_must_be_0x810[(sizeof(srTimer) == 0x810) ? 1 : -1];
