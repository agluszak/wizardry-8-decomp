#include "surrender/srTimer.h"

#include <ctype.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

/* reset()'s persistence record: the registry round-trip pairs CPU identity
   with the measured tick frequency and the read hook.  calibrate() fills the
   CPUID side when the stored signature does not describe the running CPU. */
struct srTimerConfig {
    long use_stored;                            /* +0x00 */
    long unused_04;                             /* +0x04 */
    long save;                                  /* +0x08 */
    long cpuid_support;                         /* +0x0c */
    long cpu_count;                             /* +0x10 */
    char cpu_vendor[0x10];                      /* +0x14 */
    unsigned long cpu_max_id;                   /* +0x24 */
    unsigned long cpu_signature;                /* +0x28 */
    unsigned long cpu_features;                 /* +0x2c */
    char os_ident[0x400];                       /* +0x30 */
    char cpu_ident[0x400];                      /* +0x430 */
    srQuadWord frequency;                       /* +0x830 */
    int(__stdcall* read_tick)(srQuadWord* out); /* +0x838 */
};

namespace {
unsigned __int64 quadWord64(const srQuadWord& value)
{
    return ((unsigned __int64)value.hi << 32) | value.lo;
}

/* Empty lpClass/empty-string storage. Bounded by the flag-name cursor global
   srLight::dump reads at 0x100A49D0, so the retail object is at most 0x1C
   bytes. */
// GLOBAL: SURRENDER 0x100A49B4
char storage_class[0x1c];
} // namespace

// GLOBAL: SURRENDER 0x1009C710
unsigned short srTimer::cpuFreqVariancePct = 4;

// GLOBAL: SURRENDER 0x1009C712
short srTimer::osThreadState = -1;

// GLOBAL: SURRENDER 0x1009C714
const char* srTimer::default_storage = "hkcu:SOFTWARE/Hybrid";

// GLOBAL: SURRENDER 0x1009C718
const char* srTimer::RegRoot = "hrtimer";

// GLOBAL: SURRENDER 0x1009C71C
const char* srTimer::RegCpuFreq = "hrticks";

// GLOBAL: SURRENDER 0x1009C720
const char* srTimer::RegCpuSIG = "hrsignature";

// GLOBAL: SURRENDER 0x1009C724
const char* srTimer::RegCpuMaxID = "hrmid";

// GLOBAL: SURRENDER 0x1009C728
const char* srTimer::RegCpuVers = "hrvers";

// GLOBAL: SURRENDER 0x1009C72C
const char* srTimer::RegCpuFeatures = "hrfeat";

// GLOBAL: SURRENDER 0x1009C730
const char* srTimer::RegCpuVariance = "hrvariance";

// GLOBAL: SURRENDER 0x10077608
const unsigned long srTimer::CPU_Model_Mask = 0x3fff;

// GLOBAL: SURRENDER 0x1007760C
const unsigned long srTimer::CPU_Features_Mask = 0x800011;

// GLOBAL: SURRENDER 0x100A8A30
char srTimer::RegKeyName[0x400];

// GLOBAL: SURRENDER 0x100A8E30
char srTimer::osIdent[0x400];

// GLOBAL: SURRENDER 0x100A9230
void* srTimer::RegKeyBase = 0;

// FUNCTION: SURRENDER 0x100609F0
srTimer::srTimer(int argument_0, int argument_1, int argument_2)
{
    m_frequency.lo = 0;
    m_frequency.hi = 0;
    m_base.lo = 0;
    m_base.hi = 0;
    m_tick.lo = 0;
    m_tick.hi = 0;
    m_pause.lo = 0;
    m_pause.hi = 0;
    m_kernel32 = 0;
    m_read_tick = 0;
    setUnits(1000);
    reset(argument_0, argument_1, argument_2);
}

/* Retail copies the two 0x400 strings and the 13-byte CPU signature with
   byte-at-a-time loops, not memcpy, and reloads kernel32 instead of sharing
   the source's handle. */
// FUNCTION: SURRENDER 0x10060B90
srTimer::srTimer(const srTimer& other)
{
    int index;
    for (index = 0; index < 0x400; ++index) {
        m_ident[index] = other.m_ident[index];
    }
    for (index = 0; index < 0x400; ++index) {
        m_cpu_ident[index] = other.m_cpu_ident[index];
    }
    m_frequency = other.m_frequency;
    m_base = other.m_base;
    m_tick = other.m_tick;
    m_pause = other.m_pause;
    m_units_per_interval = other.m_units_per_interval;
    m_read_tick = other.m_read_tick;
    m_kernel32 = other.m_kernel32 == 0 ? 0 : (void*)LoadLibraryA("kernel32");
    for (index = 0; index < 0xd; ++index) {
        m_cpu_vendor[index] = other.m_cpu_vendor[index];
    }
    m_cpu_max_id = other.m_cpu_max_id;
    m_cpu_signature = other.m_cpu_signature;
    m_cpu_features = other.m_cpu_features;
    m_pause.lo = 0;
    m_pause.hi = 0;
}

// FUNCTION: SURRENDER 0x10062480
srTimer& srTimer::operator=(const srTimer& other)
{
    int index;
    for (index = 0; index < 0x400; ++index) {
        m_ident[index] = other.m_ident[index];
    }
    for (index = 0; index < 0x400; ++index) {
        m_cpu_ident[index] = other.m_cpu_ident[index];
    }
    m_frequency = other.m_frequency;
    m_base = other.m_base;
    m_tick = other.m_tick;
    m_pause = other.m_pause;
    m_units_per_interval = other.m_units_per_interval;
    m_read_tick = other.m_read_tick;
    m_kernel32 = other.m_kernel32 == 0 ? 0 : (void*)LoadLibraryA("kernel32");
    for (index = 0; index < 0xd; ++index) {
        m_cpu_vendor[index] = other.m_cpu_vendor[index];
    }
    m_cpu_max_id = other.m_cpu_max_id;
    m_cpu_signature = other.m_cpu_signature;
    m_cpu_features = other.m_cpu_features;
    return *this;
}

// FUNCTION: SURRENDER 0x10060F80
srTimer::~srTimer() {}

// FUNCTION: SURRENDER 0x10061060
int __stdcall srTimer::getTick(srQuadWord* out)
{
    out->lo = GetTickCount();
    out->hi = 0;
    return 1;
}

// FUNCTION: SURRENDER 0x10061080
int __stdcall srTimer::RDTSC(srQuadWord* out)
{
    __asm {
        pushad
        mov ecx, out
        rdtsc
        mov [ecx], eax
        mov [ecx + 4], edx
        popad
    }
    return 1;
}

/* The classic CPUID probe: the ID flag (bit 21) is toggled through
   PUSHFD/POPFD; if it reads back changed, CPUID is available. */
// FUNCTION: SURRENDER 0x100610A0
int srTimer::getCPUIDSupport() const
{
    int supported = 0;
    __asm {
        push ebx
        pushfd
        pop eax
        mov ebx, eax
        xor eax, 0x00200000
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ebx
        jz unsupported
        mov supported, 1
        jmp done
    unsupported:
        mov supported, 0
    done:
        push ebx
        popfd
        pop ebx
    }
    return supported == 1;
}

// FUNCTION: SURRENDER 0x100610F0
int srTimer::reset(int detect, int argument_1, int save)
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    m_cpu_count = system_info.dwNumberOfProcessors;
    switch (system_info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        if (system_info.dwProcessorType == PROCESSOR_INTEL_386) {
            strcpy(m_cpu_ident, "i386");
        } else if (system_info.dwProcessorType == PROCESSOR_INTEL_486) {
            strcpy(m_cpu_ident, "i486");
        } else if (system_info.dwProcessorType == PROCESSOR_INTEL_PENTIUM) {
            strcpy(m_cpu_ident, "Pentium");
        } else {
            sprintf(m_cpu_ident, "x86 (%04X)", system_info.wProcessorLevel);
        }
        break;
    case PROCESSOR_ARCHITECTURE_MIPS:
        sprintf(m_cpu_ident, "MIPS R%02x%02x", system_info.wProcessorLevel & 0xff,
                system_info.wProcessorRevision & 0xff);
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        sprintf(m_cpu_ident, "Alpha %u", system_info.wProcessorLevel);
        break;
    case PROCESSOR_ARCHITECTURE_PPC:
        strcpy(m_cpu_ident, "PowerPC ");
        switch (system_info.wProcessorLevel) {
        case 1:
            strcat(m_cpu_ident, "601");
            break;
        case 3:
            strcat(m_cpu_ident, "603");
            break;
        case 4:
            strcat(m_cpu_ident, "604");
            break;
        case 6:
            strcat(m_cpu_ident, "603+");
            break;
        case 9:
            strcat(m_cpu_ident, "604+");
            break;
        case 20:
            strcat(m_cpu_ident, "620");
            break;
        }
        break;
    }
    if (system_info.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL ||
        system_info.dwProcessorType == PROCESSOR_INTEL_386) {
        detect = 1;
    }
    if (getCPUIDSupport() == 0) {
        detect = 1;
    }
    srTimerConfig config;
    config.cpuid_support = getCPUIDSupport();
    config.save = save;
    config.unused_04 = 0;
    config.cpu_count = m_cpu_count;
    strcpy(config.cpu_vendor, storage_class);
    strcpy(config.os_ident, storage_class);
    strcpy(config.cpu_ident, storage_class);
    config.cpu_max_id = 0;
    config.cpu_signature = 0;
    config.cpu_features = 0;
    config.frequency.lo = 0;
    config.frequency.hi = 0;
    config.read_tick = 0;
    config.use_stored = detect;
    if (retrieve()) {
        config.frequency = m_frequency;
        config.cpu_max_id = m_cpu_max_id;
        config.cpu_signature = m_cpu_signature;
        config.cpu_features = m_cpu_features;
        strcpy(config.cpu_vendor, m_cpu_vendor);
        config.read_tick = m_read_tick;
    } else {
        m_frequency.lo = 0;
        config.frequency.lo = 0;
        m_frequency.hi = 0;
        config.frequency.hi = 0;
    }
    calibrate(&config);
    strcpy(m_cpu_vendor, config.cpu_vendor);
    strcpy(m_ident, config.os_ident);
    strcpy(m_cpu_ident, config.cpu_ident);
    m_cpu_max_id = config.cpu_max_id;
    m_cpu_signature = config.cpu_signature;
    m_cpu_features = config.cpu_features;
    m_frequency = config.frequency;
    m_read_tick = config.read_tick;
    double frequency;
    if (m_frequency == 0.0) {
        frequency = 1.0;
    } else {
        frequency = m_frequency;
    }
    m_seconds_per_tick = 1.0 / frequency;
    m_units_per_tick = m_units_per_interval * (1.0 / frequency);
    if (m_read_tick == RDTSC) {
        strcpy(m_ident, "CPU Time Stamp Counter Register");
        strcpy(m_cpu_ident, m_cpu_vendor);
        strcat(m_cpu_ident, " ");
        if (memcmp(m_cpu_vendor, "CyrixInstead", 0xc) == 0) {
            unsigned long model = m_cpu_signature & 0xfff0;
            if (model == 0x540) {
                strcat(m_cpu_ident, "(MediaGX/MMX)");
            } else if (model == 0x600) {
                strcat(m_cpu_ident, "(6x86MX)");
            } else if (model == 0x440) {
                strcat(m_cpu_ident, "(MediaGX)");
            } else if (model == 0x520) {
                if ((m_cpu_features & 0x104) == 0x104) {
                    strcat(m_cpu_ident, "(6x86L)");
                } else {
                    strcat(m_cpu_ident, "(6x86)");
                }
            } else {
                strcat(m_cpu_ident, "(unknown version)");
            }
        } else if (memcmp(m_cpu_vendor, "CentaurHauls", 0xc) == 0) {
            unsigned long model = m_cpu_signature & 0xfff0;
            if (model == 0x590) {
                strcat(m_cpu_ident, "WinChip 3");
            } else if (model == 0x580) {
                strcat(m_cpu_ident, "WinChip 2");
            } else if (model == 0x540) {
                strcat(m_cpu_ident, "WinChip C6");
            } else {
                strcat(m_cpu_ident, "WinChip");
            }
        } else if (memcmp(m_cpu_vendor, "RiseRiseRise", 0xc) == 0) {
            strcat(m_cpu_ident, "mP6");
        } else if (memcmp(m_cpu_vendor, "AuthenticAMD", 0xc) == 0) {
            unsigned long model = m_cpu_signature & 0xfff0;
            if (model == 0x610) {
                strcat(m_cpu_ident, "K7(tm)");
            } else if ((m_cpu_signature & 0xf00) == 0x600) {
                strcat(m_cpu_ident, "Athlon");
            } else if (model == 0x590) {
                strcat(m_cpu_ident, "K6-3");
            } else if (model == 0x580) {
                strcat(m_cpu_ident, "K6-2");
            } else if (model == 0x570) {
                strcat(m_cpu_ident, "(K6 Model 7)");
            } else if (model == 0x560) {
                strcat(m_cpu_ident, "(K6 Model 6)");
            } else if (model == 0x550) {
                strcat(m_cpu_ident, "(K6 Model 5)");
            } else if (model == 0x540) {
                strcat(m_cpu_ident, "(K6 Model 4)");
            } else if (model == 0x530) {
                strcat(m_cpu_ident, "(K5 Model 3)");
            } else if (model == 0x520) {
                strcat(m_cpu_ident, "(K5 Model 2)");
            } else if (model == 0x510) {
                strcat(m_cpu_ident, "(K5 Model 1)");
            } else if (model == 0x500) {
                strcat(m_cpu_ident, "(K5)");
            } else {
                strcat(m_cpu_ident, "Am486/Am5x86");
            }
        } else {
            unsigned short family = (unsigned short)(m_cpu_signature >> 8) & 0xf;
            if (family < 5) {
                sprintf(m_cpu_ident + strlen(m_cpu_ident), "i%d86", family);
            } else if (family == 5) {
                sprintf(m_cpu_ident + strlen(m_cpu_ident), "Pentium%s",
                        (m_cpu_features & 0x800000) != 0 ? " MMX" : storage_class);
            } else if (family == 6) {
                const char* name;
                switch ((m_cpu_signature >> 4) & 0xf) {
                case 0:
                case 1:
                    name = "Pentium Pro";
                    break;
                case 2:
                    if ((m_cpu_features & 0x800000) == 0) {
                        name = "Pentium Pro";
                        break;
                    }
                    goto pentium_ii;
                case 3:
                case 4:
                case 5:
                pentium_ii:
                    name = "Pentium II";
                    break;
                case 6:
                    name = "Celeron";
                    break;
                default:
                    name = "Pentium III";
                    break;
                }
                strcat(m_cpu_ident, name);
            } else if (family > 7) {
                sprintf(m_cpu_ident + strlen(m_cpu_ident), " x86 Family %d", family);
                if ((m_cpu_features & 0x800000) != 0) {
                    strcat(m_cpu_ident, "/MMX");
                }
            }
            switch ((m_cpu_signature >> 0xc) & 3) {
            case 1:
                strcat(m_cpu_ident, "/OverDrive");
                break;
            case 2:
                strcat(m_cpu_ident, "/SMP");
                break;
            }
            sprintf(m_cpu_ident + strlen(m_cpu_ident), " Model %u Step %u",
                    (unsigned int)((m_cpu_signature >> 4) & 0xf),
                    (unsigned int)(m_cpu_signature & 0xf));
        }
    }
    if (m_read_tick == 0) {
        m_kernel32 = GetModuleHandleA("kernel32");
        if (m_kernel32 != 0) {
            // reinterpret-ok: Win32 GetProcAddress returns untyped FARPROC; retail calls the
            // result through the QueryPerformanceFrequency prototype
            BOOL(__stdcall * query_frequency)(LARGE_INTEGER*) =
                reinterpret_cast<BOOL(__stdcall*)(LARGE_INTEGER*)>(reinterpret_cast<void*>(
                    GetProcAddress((HMODULE)m_kernel32, "QueryPerformanceFrequency")));
            if (query_frequency != 0 && query_frequency((LARGE_INTEGER*)&m_frequency) != 0) {
                // reinterpret-ok: Win32 FARPROC has no parameter typing
                m_read_tick =
                    reinterpret_cast<int(__stdcall*)(srQuadWord*)>(reinterpret_cast<void*>(
                        GetProcAddress((HMODULE)m_kernel32, "QueryPerformanceCounter")));
            }
            if (m_read_tick == 0) {
                m_kernel32 = 0;
            } else {
                strcpy(m_ident, "Win32 QueryPerformanceCounter() API");
            }
        }
        if (m_read_tick == 0) {
            m_read_tick = getTick;
            m_frequency.lo = 1000;
            m_frequency.hi = 0;
            strcpy(m_ident, "Win32 GetTickCount() API");
        }
    }
    if (save) {
        store();
    }
    if (m_read_tick != 0) {
        return m_read_tick(&m_base);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10061EC0
int srTimer::calibrate(srTimerConfig* config)
{
    config->save = 0;
    if (config->read_tick != 0) {
        return 0;
    }
    unsigned long max_id = 0;
    unsigned long signature = 0;
    unsigned long features = 0;
    char vendor[0x10];
    if (config->use_stored == 0) {
        __asm {
            pushad
            mov eax, 0
            cpuid
            mov max_id, eax
            mov dword ptr [vendor], ebx
            mov dword ptr [vendor + 4], edx
            mov dword ptr [vendor + 8], ecx
            xor eax, eax
            mov vendor[0xc], al
            mov eax, 1
            cpuid
            mov signature, eax
            mov features, edx
            popad
        }
    } else {
        config->cpuid_support = 0;
    }
    if ((features & 0x10) == 0) {
        return 0;
    }
    config->read_tick = RDTSC;
    if (config->cpu_count > 1) {
        signature = (signature & 0xffffefff) | 0x2000;
    }
    if (strncmp(config->cpu_vendor, vendor, 0xc) != 0 ||
        ((config->cpu_signature ^ signature) & CPU_Model_Mask) != 0 ||
        ((config->cpu_features ^ features) & CPU_Features_Mask) != 0) {
        config->frequency.lo = 0;
        config->frequency.hi = 0;
    }
    if ((config->frequency.lo | config->frequency.hi) != 0) {
        __int64 tolerance =
            (__int64)((double)config->frequency * (cpuFreqVariancePct & 0xffff) * 0.01);
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        unsigned long edge = timeGetTime();
        unsigned long now;
        do {
            now = timeGetTime();
        } while (now == edge);
        srQuadWord start;
        srQuadWord end;
        config->read_tick(&start);
        now += 0x7d;
        if (timeGetTime() < now) {
            do {
            } while (timeGetTime() < now);
        }
        config->read_tick(&end);
        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        srQuadWord measured = (end - start) * 8u;
        srQuadWord drift;
        if (config->frequency.hi < measured.hi ||
            (config->frequency.hi == measured.hi && config->frequency.lo <= measured.lo)) {
            drift = measured - config->frequency;
        } else {
            drift = config->frequency - measured;
        }
        if ((__int64)quadWord64(drift) > tolerance) {
            config->frequency.lo = 0;
            config->frequency.hi = 0;
        }
    }
    if ((config->frequency.lo | config->frequency.hi) == 0) {
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        unsigned long edge = timeGetTime();
        unsigned long now;
        do {
            now = timeGetTime();
        } while (now == edge);
        srQuadWord start;
        srQuadWord end;
        config->read_tick(&start);
        now += 4000;
        if (timeGetTime() < now) {
            do {
            } while (timeGetTime() < now);
        }
        config->read_tick(&end);
        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        unsigned __int64 measured = quadWord64(end - start) >> 2;
        config->frequency.lo = (unsigned int)measured;
        config->frequency.hi = (unsigned int)(measured >> 0x20);
        config->cpu_max_id = max_id;
        config->cpu_signature = signature;
        config->cpu_features = features;
        memcpy(config->cpu_vendor, vendor, 0xc);
        config->cpu_vendor[0xc] = 0;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100621E0
double srTimer::getFreqf() const
{
    return m_frequency.lo * 1e-06 + m_frequency.hi * 4294.967296;
}

// FUNCTION: SURRENDER 0x10062230
void srTimer::getFreq(srQuadWord& out) const
{
    out.lo = m_frequency.lo;
    out.hi = m_frequency.hi;
}

// FUNCTION: SURRENDER 0x10062250
unsigned long srTimer::getUnits() const
{
    return m_units_per_interval;
}

// FUNCTION: SURRENDER 0x10062260
void srTimer::setUnits(unsigned long units)
{
    m_units_per_interval = units;
    if (m_frequency == 0.0) {
        m_units_per_tick = units / 1.0;
    } else {
        m_units_per_tick = units / (double)m_frequency;
    }
}

// FUNCTION: SURRENDER 0x10062320
const char* srTimer::getIdent() const
{
    return m_ident;
}

// FUNCTION: SURRENDER 0x10062330
unsigned long srTimer::getCPUCount() const
{
    return m_cpu_count;
}

// FUNCTION: SURRENDER 0x10062340
const char* srTimer::getCPUIdent() const
{
    return m_cpu_ident;
}

// FUNCTION: SURRENDER 0x10062350
srTimer::e_cpuTypeId srTimer::getCPUType() const
{
    return (e_cpuTypeId)(m_cpu_signature >> 0xc & 3);
}

// FUNCTION: SURRENDER 0x10062360
unsigned short srTimer::getCPUFamily() const
{
    return (unsigned short)(m_cpu_signature >> 8) & 0xf;
}

// FUNCTION: SURRENDER 0x10062370
unsigned short srTimer::getCPUModel() const
{
    return (unsigned short)(m_cpu_signature >> 4) & 0xf;
}

// FUNCTION: SURRENDER 0x10062380
unsigned short srTimer::getCPUStepping() const
{
    return (unsigned short)(m_cpu_signature & 0xf);
}

// FUNCTION: SURRENDER 0x10062390
int srTimer::getFeature(long feature) const
{
    return (m_cpu_features & (1 << feature)) != 0;
}

// FUNCTION: SURRENDER 0x100623B0
int srTimer::getFPUSupport() const
{
    return m_cpu_features & 1;
}

// FUNCTION: SURRENDER 0x100623C0
int srTimer::getRDTSCSupport() const
{
    return m_cpu_features >> 4 & 1;
}

// FUNCTION: SURRENDER 0x100623D0
int srTimer::getMMXSupport() const
{
    return m_cpu_features >> 0x17 & 1;
}

// FUNCTION: SURRENDER 0x100623E0
unsigned long srTimer::getRawTime(e_timerReadControl control)
{
    if (control == TIMER_READ_DEFAULT) {
        m_read_tick(&m_tick);
    }
    return m_tick.lo - m_base.lo;
}

// FUNCTION: SURRENDER 0x10062430
unsigned long srTimer::getRawTime(srQuadWord& out, e_timerReadControl control)
{
    if (control == TIMER_READ_DEFAULT) {
        m_read_tick(&m_tick);
    }
    out = m_tick - m_base;
    return out.lo;
}

// FUNCTION: SURRENDER 0x100625A0
char* srTimer::getStorage(char* buffer, unsigned long size)
{
    memset(buffer, 0, size);
    if (RegKeyBase == (void*)0x80000001) {
        strcpy(buffer, "hkcu");
    } else if (RegKeyBase == (void*)0x80000002) {
        strcpy(buffer, "hklm");
    } else if (RegKeyBase == (void*)0x80000005) {
        strcpy(buffer, "hkcc");
    } else if (RegKeyBase == (void*)0x80000000) {
        strcpy(buffer, "hkcr");
    } else if (RegKeyBase == (void*)0x80000003) {
        strcpy(buffer, "hkus");
    } else if (RegKeyBase == (void*)0x80000004) {
        strcpy(buffer, "hkpd");
    } else {
        sprintf(buffer, "0x%08X", (unsigned int)RegKeyBase);
    }
    strcat(buffer, ":");
    strncat(buffer, RegKeyName, size - 5);
    char* slash = strchr(buffer, '\\');
    while (slash != 0) {
        *slash = '/';
        slash = strchr(buffer, '\\');
    }
    return buffer;
}

// FUNCTION: SURRENDER 0x10062750
int srTimer::fastThreads()
{
    if (osThreadState == -1) {
        getOsIdent();
    }
    return osThreadState == 1;
}

// FUNCTION: SURRENDER 0x10062770
int srTimer::isPaused() const
{
    if ((m_pause.lo | m_pause.hi) != 0) {
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10062960
int srTimer::store()
{
    char key_name[256];
    if (RegKeyBase == 0) {
        setStorage(0);
    }
    sprintf(key_name, "%s\\%s", RegKeyName, RegRoot);
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA((HKEY)RegKeyBase, key_name, 0, storage_class, 0, KEY_ALL_ACCESS, 0, &key,
                        &disposition) == 0) {
        if (RegSetValueExA(key, RegCpuFreq, 0, REG_BINARY, (BYTE*)&m_frequency, 8) == 0) {
            if (RegSetValueExA(key, RegCpuSIG, 0, REG_BINARY, (BYTE*)m_cpu_vendor, 0xd) == 0) {
                if (RegSetValueExA(key, RegCpuMaxID, 0, REG_BINARY, (BYTE*)&m_cpu_max_id, 4) == 0) {
                    if (RegSetValueExA(key, RegCpuVers, 0, REG_BINARY, (BYTE*)&m_cpu_signature,
                                       4) == 0) {
                        if (RegSetValueExA(key, RegCpuFeatures, 0, REG_BINARY,
                                           (BYTE*)&m_cpu_features, 4) == 0) {
                            if (RegFlushKey(key) == 0) {
                                if (RegCloseKey(key) == 0) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10062AB0
int srTimer::retrieve()
{
    char key_name[256];
    if (RegKeyBase == 0) {
        setStorage(0);
    }
    sprintf(key_name, "%s\\%s", RegKeyName, RegRoot);
    int ok = 0;
    HKEY key;
    if (RegOpenKeyExA((HKEY)RegKeyBase, key_name, 0, KEY_ALL_ACCESS, &key) == 0) {
        m_frequency.lo = 0;
        m_frequency.hi = 0;
        m_cpu_features = 0;
        m_cpu_signature = 0;
        m_cpu_max_id = 0;
        memset(m_cpu_vendor, 0, 0xd);
        DWORD type = REG_BINARY;
        DWORD size = 8;
        ok = RegQueryValueExA(key, RegCpuFreq, 0, &type, (BYTE*)&m_frequency, &size) == 0;
        type = REG_BINARY;
        size = 0xd;
        if (ok != 0 &&
            RegQueryValueExA(key, RegCpuSIG, 0, &type, (BYTE*)m_cpu_vendor, &size) != 0) {
            ok = 0;
        }
        type = REG_BINARY;
        size = 4;
        if (ok != 0 &&
            RegQueryValueExA(key, RegCpuMaxID, 0, &type, (BYTE*)&m_cpu_max_id, &size) != 0) {
            ok = 0;
        }
        type = REG_BINARY;
        size = 4;
        if (ok != 0 &&
            RegQueryValueExA(key, RegCpuVers, 0, &type, (BYTE*)&m_cpu_signature, &size) != 0) {
            ok = 0;
        }
        type = REG_BINARY;
        size = 4;
        if (ok != 0 &&
            RegQueryValueExA(key, RegCpuFeatures, 0, &type, (BYTE*)&m_cpu_features, &size) != 0) {
            ok = 0;
        }
        type = REG_BINARY;
        size = 4;
        float variance;
        if (RegQueryValueExA(key, RegCpuVariance, 0, &type, (BYTE*)&variance, &size) == 0) {
            cpuFreqVariancePct = (unsigned short)variance;
        }
        RegCloseKey(key);
    }
    return ok;
}

// FUNCTION: SURRENDER 0x10062D20
int srTimer::pause()
{
    if ((m_pause.lo | m_pause.hi) != 0) {
        return 0;
    }
    m_read_tick(&m_pause);
    return 1;
}

// FUNCTION: SURRENDER 0x10062D50
unsigned long srTimer::resume()
{
    srQuadWord delta = {0, 0};
    if ((m_pause.lo | m_pause.hi) != 0) {
        srQuadWord now;
        m_read_tick(&now);
        delta.lo = now.lo - m_pause.lo;
        delta.hi = now.hi - m_pause.hi - (now.lo < m_pause.lo);
        unsigned int carry = m_base.lo;
        m_base.lo += delta.lo;
        m_base.hi += delta.hi + (m_base.lo < carry);
        m_pause.lo = 0;
        m_pause.hi = 0;
    }
    return (unsigned long)(quadWord64(delta) * (unsigned long)m_units_per_interval /
                           quadWord64(m_frequency));
}

// FUNCTION: SURRENDER 0x10062DF0
unsigned long srTimer::getMsTime(e_timerReadControl control)
{
    getUTime(control);
    return (unsigned long)(quadWord64(m_tick - m_base) * 1000 / quadWord64(m_frequency));
}

// FUNCTION: SURRENDER 0x10062E50
double srTimer::getTime(e_timerReadControl control)
{
    if (control == TIMER_READ_DEFAULT) {
        m_read_tick(&m_tick);
    }
    return (m_tick - m_base) * m_seconds_per_tick;
}

// FUNCTION: SURRENDER 0x10062EC0
unsigned long srTimer::getUTime(e_timerReadControl control)
{
    if (control == TIMER_READ_DEFAULT) {
        m_read_tick(&m_tick);
    }
    return (unsigned long)((m_tick - m_base) * m_units_per_tick);
}

// FUNCTION: SURRENDER 0x10062F40
unsigned long srTimer::getUTime(srQuadWord& out, e_timerReadControl control)
{
    if (control == TIMER_READ_DEFAULT) {
        m_read_tick(&m_tick);
    }
    unsigned __int64 units = (unsigned __int64)((m_tick - m_base) * m_units_per_tick);
    out.lo = (unsigned long)units;
    out.hi = (unsigned long)(units >> 0x20);
    return out.lo;
}

// FUNCTION: SURRENDER 0x10062FC0
char* srTimer::getAscTime(char* buffer, e_timerReadControl control)
{
    getUTime(control);
    unsigned __int64 units =
        quadWord64(m_tick - m_base) * (unsigned long)m_units_per_interval / quadWord64(m_frequency);
    srQuadWord ticks;
    ticks.lo = (unsigned long)units;
    ticks.hi = (unsigned long)(units >> 0x20);
    return getAscTime(buffer, ticks);
}

/* Writes "HHH:MM:SS.mmm"; the hour field is 3-wide until it overflows into
   "###" and the seconds carry the fraction. */
// FUNCTION: SURRENDER 0x10063030
char* srTimer::getAscTime(char* buffer, srQuadWord ticks)
{
    *buffer = '\0';
    float seconds = (float)((ticks.lo + ticks.hi * 4294967296.0) / m_units_per_interval);
    if (seconds >= 3600.0f) {
        long hours = (long)(seconds / 3600.0f);
        seconds -= (float)(hours * 0xe10);
        if (hours < 1000) {
            sprintf(buffer + strlen(buffer), "%03lu:", hours);
        } else {
            strcat(buffer, "###:");
        }
    } else {
        strcat(buffer, "000:");
    }
    if (seconds >= 60.0f) {
        long minutes = (long)(seconds / 60.0f);
        seconds -= (float)(minutes * 0x3c);
        sprintf(buffer + strlen(buffer), "%02lu:", minutes);
    } else {
        strcat(buffer, "00:");
    }
    sprintf(buffer + strlen(buffer), "%06.3f", seconds);
    return buffer;
}

// FUNCTION: SURRENDER 0x10063200
const char* srTimer::getCPUTypeIdString(e_cpuTypeId type) const
{
    const char* names[] = {"OEM", "Overdrive", "SMP", "Unknown"};
    if (4 <= (unsigned int)type) {
        type = CPU_TYPE_RESERVED;
    }
    return names[type];
}

// FUNCTION: SURRENDER 0x10062790
void srTimer::setStorage(char* const storage)
{
    char* value = storage;
    if (value == 0) {
        value = (char*)default_storage;
    }
    char* colon = strchr(value, ':');
    if (colon == 0 || (unsigned short)(colon - value) > 10) {
        return;
    }
    char root[11];
    memset(root, 0, sizeof(root));
    strncpy(root, value, colon - value);
    if (_strnicmp(root, "hkcu", 4) == 0) {
        value = (char*)0x80000001;
    } else if (_strnicmp(root, "hklm", 4) == 0) {
        value = (char*)0x80000002;
    } else if (_strnicmp(root, "hkcr", 4) == 0) {
        value = (char*)0x80000000;
    } else if (_strnicmp(root, "hkus", 4) == 0) {
        value = (char*)0x80000003;
    } else if (_strnicmp(root, "hkpd", 4) == 0) {
        value = (char*)0x80000004;
    } else if (_strnicmp(root, "hkcc", 4) == 0) {
        value = (char*)0x80000005;
    } else {
        if (_strnicmp(root, "0x", 2) != 0 || strlen(root) != 0xa) {
            return;
        }
        sscanf("%d", root, &value);
        if (value == 0) {
            return;
        }
    }
    RegKeyBase = value;
    strcpy(RegKeyName, colon + 1);
    char* slash = strchr(RegKeyName, '/');
    while (slash != 0) {
        *slash = '\\';
        slash = strchr(RegKeyName, '/');
    }
}

// FUNCTION: SURRENDER 0x10060CC0
const char* srTimer::getOsIdent() const
{
    if (osThreadState != -1) {
        return osIdent;
    }
    osThreadState = 1;
    OSVERSIONINFOA info;
    info.dwOSVersionInfoSize = 0x94;
    GetVersionExA(&info);
    char csd[256];
    memset(csd, 0, sizeof(csd));
    strncpy(csd, info.szCSDVersion, 0xff);
    char* front = csd;
    while (isspace(*front) && *front != '\0') {
        ++front;
    }
    strcpy(info.szCSDVersion, front);
    /* Retail walks from the NUL terminator (not the last character) and tests
       isspace before decrementing, so the trim is inert: isspace('\0') stops
       the loop at entry and *end = 0 rewrites the existing terminator. No
       trailing whitespace is ever removed, but no out-of-bounds access is
       possible either. Preserved as recovered retail behavior. */
    char* end = info.szCSDVersion + strlen(info.szCSDVersion);
    while (end != info.szCSDVersion && isspace(*end)) {
        --end;
    }
    *end = '\0';
    if (info.dwPlatformId == 2) {
        sprintf(osIdent, "WindowsNT %d.%d build %d", static_cast<int>(info.dwMajorVersion),
                static_cast<int>(info.dwMinorVersion), static_cast<int>(info.dwBuildNumber));
        if (info.szCSDVersion[0] != '\0') {
            sprintf(osIdent + strlen(osIdent), " (%s)", info.szCSDVersion);
        }
        return osIdent;
    }
    const char* name;
    if (info.dwMajorVersion < 4) {
        name = "Win32s on Windows";
    } else if (info.dwMinorVersion > 9) {
        name = "Windows98";
    } else {
        name = "Windows95";
    }
    strcpy(osIdent, name);
    if (info.szCSDVersion[0] != '\0') {
        if ((info.dwBuildNumber & 0xffff) == 0x457) {
            strcat(osIdent, " OSR2");
        } else if (info.szCSDVersion[0] == ' ') {
            strcat(osIdent, info.szCSDVersion);
        } else {
            sprintf(osIdent + strlen(osIdent), " %s", info.szCSDVersion);
        }
    }
    sprintf(osIdent + strlen(osIdent), " (Version %d.%02d.%u)",
            static_cast<int>(info.dwMajorVersion), static_cast<int>(info.dwMinorVersion),
            static_cast<unsigned int>(info.dwBuildNumber & 0xffff));
    osThreadState = 0;
    return osIdent;
}

/* The stream's width field doubles as the print-mode selector: 1 prints the
   CPU identity and frequency, anything else the OS identity; the mode is
   restored afterwards. */
// FUNCTION: SURRENDER 0x10060F90
std::ostream& operator<<(std::ostream& stream, const srTimer& timer)
{
    int mode = stream.width();
    if (mode == 1) {
        stream << timer.m_cpu_ident << " @ " << timer.m_frequency * 1e-6 << " Mhz";
    } else {
        stream << timer.m_ident;
    }
    stream.width(mode);
    return stream;
}
