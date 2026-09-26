#pragma once

#include <iostream>

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
// VTABLE: SURRENDER 0x10077620 srTimer
class SR_DLL_IMPORT srTimer {
public:
    enum e_timerReadControl { TIMER_READ_DEFAULT = 0 };
    /* CPUID signature processor-type field (EAX bits 12:13). */
    enum e_cpuTypeId {
        CPU_TYPE_OEM = 0,
        CPU_TYPE_OVERDRIVE = 1,
        CPU_TYPE_DUAL = 2,
        CPU_TYPE_RESERVED = 3
    };

    srTimer(int argument_0 = 0, int argument_1 = 0, int argument_2 = 1);
    srTimer(const srTimer& other);
    srTimer& operator=(const srTimer& other);

    virtual ~srTimer(); /* 0 */
    virtual char* getAscTime(char* buffer, e_timerReadControl control);
    char* getAscTime(char* buffer, srQuadWord value);
    virtual int pause();            /* 2 */
    virtual unsigned long resume(); /* 3 */
    virtual int reset(int argument_0, int argument_1, int argument_2);
    virtual unsigned long getMsTime(e_timerReadControl control); /* 5 */
    virtual double getTime(e_timerReadControl control);          /* 6 */
    /* Slots 7/9 take srQuadWord&, slots 8/10 take only the control. MSVC lays
       an adjacent virtual overload group out in reverse declaration order, so
       the source declares each pair reversed to land them as exported. */
    virtual unsigned long getUTime(e_timerReadControl control); /* 8 */
    virtual unsigned long getUTime(srQuadWord& out, e_timerReadControl control);
    virtual unsigned long getRawTime(e_timerReadControl control); /* 10 */
    virtual unsigned long getRawTime(srQuadWord& out, e_timerReadControl control);

    const char* getIdent() const;
    const char* getOsIdent() const;
    /* srCore::dump inlines the trivial field accessors. The member dllexport
       marks keep the header bodies for that folding while still emitting the
       exported standalone copies. */
    // FUNCTION: SURRENDER 0x10062340
    // ?getCPUIdent@srTimer@@QBEPBDXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    const char* getCPUIdent() const
    {
        return m_cpu_ident;
    }
    // FUNCTION: SURRENDER 0x10062330
    // ?getCPUCount@srTimer@@QBEKXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    unsigned long getCPUCount() const
    {
        return m_cpu_count;
    }
    e_cpuTypeId getCPUType() const;
    unsigned short getCPUFamily() const;
    unsigned short getCPUModel() const;
    unsigned short getCPUStepping() const;
    int getFeature(long feature) const;
    int getCPUIDSupport() const;
    // FUNCTION: SURRENDER 0x100623B0
    // ?getFPUSupport@srTimer@@QBEHXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int getFPUSupport() const
    {
        return m_cpu_features & 1;
    }
    // FUNCTION: SURRENDER 0x100623D0
    // ?getMMXSupport@srTimer@@QBEHXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int getMMXSupport() const
    {
        return m_cpu_features >> 0x17 & 1;
    }
    // FUNCTION: SURRENDER 0x100623C0
    // ?getRDTSCSupport@srTimer@@QBEHXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int getRDTSCSupport() const
    {
        return m_cpu_features >> 4 & 1;
    }
    void getFreq(srQuadWord& out) const;
    // FUNCTION: SURRENDER 0x100621E0
    // ?getFreqf@srTimer@@QBENXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    double getFreqf() const
    {
        return m_frequency.lo * 1e-06 + m_frequency.hi * 4294.967296;
    }
    unsigned long getUnits() const;
    void setUnits(unsigned long units);
    int isPaused() const;
    /* Header-visible like srCore::getRegistry: srInit expands the
       osThreadState/getOsIdent sequence inline rather than calling the
       out-of-line emission, so the original header carried this body. The
       member dllexport emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x10062750
    // ?fastThreads@srTimer@@QAEHXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    int fastThreads()
    {
        if (osThreadState == -1) {
            getOsIdent();
        }
        return osThreadState == 1;
    }
    const char* getCPUTypeIdString(e_cpuTypeId type) const;
    /* "<hive>:<path>" names the registry location store()/retrieve() use; a
       null storage selects default_storage. */
    static char* getStorage(char* buffer, unsigned long size);
    static void setStorage(char* const storage);

    unsigned char unknown_004_[0x4];
    char m_ident[0x400];     /* 0x008: module/OS identity text */
    char m_cpu_ident[0x400]; /* 0x408: processor identity text */
    /* Ticks per second, measured by reset(). */
    srQuadWord m_frequency;   /* 0x808 */
    srQuadWord m_base;        /* 0x810: tick offset subtracted from reads */
    srQuadWord m_tick;        /* 0x818: last-read tick snapshot */
    srQuadWord m_pause;       /* 0x820: tick at pause(), zero while running */
    int m_units_per_interval; /* 0x828: the game-timer unit writes 10000 */
    unsigned char unknown_82c_[0x4];
    double m_seconds_per_tick;                    /* 0x830: 1.0 / frequency */
    double m_units_per_tick;                      /* 0x838: units / frequency */
    unsigned long m_cpu_count;                    /* 0x840 */
    int(__stdcall* m_read_tick)(srQuadWord* out); /* 0x844: getTick or RDTSC */
    void* m_kernel32;                             /* 0x848: kernel32 handle when QPC is used */
    char m_cpu_vendor[0x10];                      /* 0x84c: CPUID vendor string */
    unsigned long m_cpu_max_id;                   /* 0x85c: max CPUID input */
    unsigned long m_cpu_signature;                /* 0x860: CPUID EAX */
    unsigned long m_cpu_features;                 /* 0x864: CPUID EDX */

protected:
    int retrieve();
    int store();
    /* Internal frequency calibration; the record is the registry persistence
       blob reset() fills. */
    static int calibrate(struct srTimerConfig* config);

    static int __stdcall getTick(srQuadWord* out);
    static int __stdcall RDTSC(srQuadWord* out);
    static short osThreadState;
    static char osIdent[0x400];
    static unsigned short cpuFreqVariancePct;
    static const char* default_storage;
    static const unsigned long CPU_Model_Mask;
    static const unsigned long CPU_Features_Mask;
    static void* RegKeyBase;
    static char RegKeyName[0x400];
    static const char* RegRoot;
    static const char* RegCpuFreq;
    static const char* RegCpuSIG;
    static const char* RegCpuMaxID;
    static const char* RegCpuVers;
    static const char* RegCpuFeatures;
    static const char* RegCpuVariance;
};
#pragma pack(pop)

SR_DLL_IMPORT std::ostream& operator<<(std::ostream& stream, const srTimer& timer);

static_assert((sizeof(srTimer) == 0x868), "srTimer_must_be_0x868");
