#pragma once

#include "srHeap.h"

class srQuadWord;

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
    virtual SR_DLL_IMPORT unsigned long getUTime(srQuadWord& out, e_timerReadControl control);
    virtual SR_DLL_IMPORT unsigned long getUTime(e_timerReadControl control);     /* 8 */
    virtual SR_DLL_IMPORT unsigned long getRawTime(srQuadWord& out, e_timerReadControl control);
    virtual SR_DLL_IMPORT unsigned long getRawTime(e_timerReadControl control);   /* 10 */

    unsigned char unknown_004_[0x804];
    /* Ticks per second, measured by the constructor; one unsigned 64-bit
       value, spelled as two words so the class keeps 4-byte alignment - an
       __int64 member makes VC6 pad the extent past what the image allocates. */
    unsigned int m_frequency_low;        /* 0x808 */
    unsigned int m_frequency_high;       /* 0x80c */
};

typedef char srTimer_must_be_0x810[(sizeof(srTimer) == 0x810) ? 1 : -1];
