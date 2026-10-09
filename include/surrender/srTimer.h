#pragma once

#include <iosfwd>

#include "srHeap.h"
#include "srQuadWord.h"

#pragma pack(push, 4)
// VTABLE: SURRENDER 0x10077620 srTimer
class SR_DLL_IMPORT SR_DLL_EXPORT srTimer {
public:
    typedef int(__stdcall* TickReader)(srQuadWord* out);

    enum e_timerReadControl { TIMER_READ_DEFAULT = 0 };
    /* CPUID signature processor-type field (EAX bits 12:13). */
    enum e_cpuTypeId {
        CPU_TYPE_OEM = 0,
        CPU_TYPE_OVERDRIVE = 1,
        CPU_TYPE_DUAL = 2,
        CPU_TYPE_RESERVED = 3
    };

    srTimer(int force_system_timer = 0, int unused = 0, int save_calibration = 1);
    srTimer(const srTimer& other);
    srTimer& operator=(const srTimer& other);

    virtual ~srTimer(); /* 0 */
    virtual char* getAscTime(char* buffer, e_timerReadControl control);
    char* getAscTime(char* buffer, srQuadWord value);
    virtual int pause();       /* 2 */
    virtual w8_ulong resume(); /* 3 */
    virtual int reset(int force_system_timer, int unused, int save_calibration);
    virtual w8_ulong getMsTime(e_timerReadControl control); /* 5 */
    virtual double getTime(e_timerReadControl control);     /* 6 */
    /* Each overload pair is declared in reverse slot order. */
    virtual w8_ulong getUTime(e_timerReadControl control); /* 8 */
    virtual w8_ulong getUTime(srQuadWord& out, e_timerReadControl control);
    virtual w8_ulong getRawTime(e_timerReadControl control); /* 10 */
    virtual w8_ulong getRawTime(srQuadWord& out, e_timerReadControl control);

    const char* getIdent() const;
    const char* getOsIdent() const;
    // FUNCTION: SURRENDER 0x10062340
    // RECOMP: ?getCPUIdent@srTimer@@QBEPBDXZ
    const char* getCPUIdent() const
    {
        return m_cpu_ident;
    }
    // FUNCTION: SURRENDER 0x10062330
    // RECOMP: ?getCPUCount@srTimer@@QBEKXZ
    w8_ulong getCPUCount() const
    {
        return m_cpu_count;
    }
    e_cpuTypeId getCPUType() const;
    unsigned short getCPUFamily() const;
    unsigned short getCPUModel() const;
    unsigned short getCPUStepping() const;
    enum { CPU_FEATURE_FPU = 0, CPU_FEATURE_RDTSC = 4, CPU_FEATURE_MMX = 23 };
    int getFeature(w8_long feature) const;
    int getCPUIDSupport() const;
    // FUNCTION: SURRENDER 0x100623B0
    // RECOMP: ?getFPUSupport@srTimer@@QBEHXZ
    int getFPUSupport() const
    {
        return m_cpu_features & (1UL << CPU_FEATURE_FPU);
    }
    // FUNCTION: SURRENDER 0x100623D0
    // RECOMP: ?getMMXSupport@srTimer@@QBEHXZ
    int getMMXSupport() const
    {
        return m_cpu_features >> CPU_FEATURE_MMX & 1;
    }
    // FUNCTION: SURRENDER 0x100623C0
    // RECOMP: ?getRDTSCSupport@srTimer@@QBEHXZ
    int getRDTSCSupport() const
    {
        return m_cpu_features >> CPU_FEATURE_RDTSC & 1;
    }
    void getFreq(srQuadWord& out) const;
    // FUNCTION: SURRENDER 0x100621E0
    // RECOMP: ?getFreqf@srTimer@@QBENXZ
    double getFreqf() const
    {
        return m_frequency.lo * 1e-06 + m_frequency.hi * 4294.967296;
    }
    w8_ulong getUnits() const;
    void setUnits(w8_ulong units);
    int isPaused() const;
    // FUNCTION: SURRENDER 0x10062750
    // RECOMP: ?fastThreads@srTimer@@QAEHXZ
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
    static char* getStorage(char* buffer, w8_ulong size);
    static void setStorage(char* const storage);

    unsigned char unknown_004_[0x4];
    char m_ident[0x400];     /* 0x008: module/OS identity text */
    char m_cpu_ident[0x400]; /* 0x408: processor identity text */
    /* Ticks per second, measured by reset(). */
    srQuadWord m_frequency;            /* 0x808 */
    srQuadWord m_base;                 /* 0x810: tick offset subtracted from reads */
    srQuadWord m_tick;                 /* 0x818: last-read tick snapshot */
    srQuadWord m_pause;                /* 0x820: tick at pause(), zero while running */
    unsigned int m_units_per_interval; /* 0x828: timer units per second */
    unsigned char unknown_82c_[0x4];
    double m_seconds_per_tick; /* 0x830: 1.0 / frequency */
    double m_units_per_tick;   /* 0x838: units / frequency */
    w8_ulong m_cpu_count;      /* 0x840 */
    TickReader m_read_tick;    /* 0x844: getTick or RDTSC */
    HMODULE m_kernel32;        /* 0x848: kernel32 handle when QPC is used */
    char m_cpu_vendor[0x10];   /* 0x84c: CPUID vendor string */
    w8_ulong m_cpu_max_id;     /* 0x85c: max CPUID input */
    w8_ulong m_cpu_signature;  /* 0x860: CPUID EAX */
    w8_ulong m_cpu_features;   /* 0x864: CPUID EDX */

protected:
    int retrieve();
    int store();
    /* Frequency calibration over the registry persistence blob reset() fills; installs the
       protected RDTSC reader. */
    friend int calibrate(struct srTimerConfig* config);

    static int __stdcall getTick(srQuadWord* out);
    static int __stdcall RDTSC(srQuadWord* out);
    static short osThreadState;
    static char osIdent[0x400];
    static unsigned short cpuFreqVariancePct;
    static const char* default_storage;
    static const w8_ulong CPU_Model_Mask;
    static const w8_ulong CPU_Features_Mask;
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

W8_ABI_ASSERT((sizeof(srTimer) == 0x868), "srTimer_must_be_0x868");
