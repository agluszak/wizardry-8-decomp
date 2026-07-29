#pragma once

#include "srHeap.h"
#include "srQuadWord.h"

/* The eleven virtual slots are the exported ??_7srTimer@@6B@ in slot order,
   from evidence/snapshots/surrender-abi/vftable-slots.csv: the destructor in
   slot 0, then the ten methods the library exports by name. Wizardry imports
   the (int,int,int) constructor and the destructor, so a derived class needs
   no body the DLL will not supply.

   The enum's values are not established; the one observed call site passes 0.

   The extent 0x868 is what `new srTimer` allocates at 0x00439590. There is no
   first-party subclass: vtable 0x005EC078 is the local copy VC6 materializes
   for a dllimport class it instantiates - every slot an import thunk and the
   deleting destructor generated locally - and the constructor re-stores it
   over the vptr the imported constructor installed. The three named fields are
   srTimer's own, placed by the game-timer unit's byte-exact constructor. */
/* Packed at 4 the way the era's SDK headers ship: with the class's natural
   alignment of 8 (the double member) MSVC pads the vfptr slot to the class
   alignment and every field lands four bytes late; pack(4) is what puts the
   frequency at +0x808 and the tick quotient at +0x838, where the byte-exact
   constructor addresses them. */
#pragma pack(push, 4)
class SR_DLL_IMPORT srTimer {
public:
    enum e_timerReadControl {
        TIMER_READ_DEFAULT = 0
    };

    srTimer(int argument_0, int argument_1, int argument_2);

    virtual ~srTimer();                                /* 0 */
    virtual char* getAscTime(char* buffer, e_timerReadControl control);
    virtual int pause();                                            /* 2 */
    virtual unsigned long resume();                                 /* 3 */
    virtual int reset(int argument_0, int argument_1, int argument_2);
    virtual unsigned long getMsTime(e_timerReadControl control);    /* 5 */
    virtual double getTime(e_timerReadControl control);             /* 6 */
    /* Slots 7/9 take srQuadWord&, slots 8/10 take only the control. MSVC lays
       an adjacent virtual overload group out in reverse declaration order, so
       the source declares each pair reversed to land them as exported. */
    virtual unsigned long getUTime(e_timerReadControl control);     /* 8 */
    virtual unsigned long getUTime(srQuadWord& out, e_timerReadControl control);
    virtual unsigned long getRawTime(e_timerReadControl control);   /* 10 */
    virtual unsigned long getRawTime(srQuadWord& out, e_timerReadControl control);

    unsigned char unknown_004_[0x804];
    /* Ticks per second, measured by the constructor. */
    srQuadWord m_frequency;              /* 0x808 */
    unsigned char unknown_810_[0x18];
    int m_units_per_interval;            /* 0x828: the game-timer unit writes 10000 */
    unsigned char unknown_82c_[0xc];
    double m_units_per_tick;             /* 0x838: 10000 / frequency, written by the same unit */
    unsigned char unknown_840_[0x28];
};
#pragma pack(pop)


static_assert((sizeof(srTimer) == 0x868), "srTimer_must_be_0x868");
