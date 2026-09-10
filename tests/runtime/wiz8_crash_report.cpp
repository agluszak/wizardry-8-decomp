/*
 * Shared in-process crash diagnostics for the runnable Wizardry 8 images.
 *
 * The matching comparison image does not link this unit. Both runnable
 * products link /FORCE:UNRESOLVED, and link.exe redirects an unresolved call
 * to the image base. The bytes there are the PE DOS header, so the processor
 * executes "MZ" as code: "dec ebp" destroys the frame chain and the following
 * "pop edx" consumes the return address before the first faulting access.
 * That is why a generic post-mortem frame walk cannot recover the caller.
 *
 * This filter records the register file, scans registers as well as stack
 * words for addresses inside the main image, and names EDX when EIP is inside
 * the mapped PE headers. The host-side MAP symbolizer then turns the consumed
 * return address and the owning object's unresolved externals into a report.
 */

#include "wiz8_crash_report.h"

#include <windows.h>

namespace {

const unsigned int kMaxCandidates = 32;

W8CrashContextWriter g_context_writer = 0;
unsigned long g_main_image_base = 0;
unsigned int g_candidate_count = 0;

unsigned long ImageHeadersSize(unsigned long base)
{
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
    const IMAGE_NT_HEADERS* nt;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }
    if (dos->e_lfanew < (long)sizeof(IMAGE_DOS_HEADER) || dos->e_lfanew > 0x1000) {
        return 0;
    }
    nt = (const IMAGE_NT_HEADERS*)(base + (unsigned long)dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }
    return nt->OptionalHeader.SizeOfHeaders;
}

int IsMainImageAddress(unsigned long address, unsigned long* offset)
{
    MEMORY_BASIC_INFORMATION memory;

    if (g_main_image_base == 0) {
        return 0;
    }
    if (VirtualQuery((const void*)address, &memory, sizeof(memory)) == 0) {
        return 0;
    }
    if (memory.State != MEM_COMMIT || memory.Type != MEM_IMAGE) {
        return 0;
    }
    if ((unsigned long)memory.AllocationBase != g_main_image_base) {
        return 0;
    }
    *offset = address - g_main_image_base;
    return 1;
}

void ReportCandidate(const char* source, unsigned long address)
{
    unsigned long offset;

    if (g_candidate_count >= kMaxCandidates) {
        return;
    }
    if (!IsMainImageAddress(address, &offset)) {
        return;
    }
    fprintf(stderr,
            "WIZ8_RUNTIME_CANDIDATE source=%s address=%08lx offset=%08lx\n",
            source, address, offset);
    ++g_candidate_count;
}

void ReportStackCandidates(unsigned long esp)
{
    const unsigned long* stack = (const unsigned long*)esp;
    MEMORY_BASIC_INFORMATION memory;
    unsigned long available;
    unsigned int words;
    unsigned int index;
    char source[24];

    if (VirtualQuery((const void*)esp, &memory, sizeof(memory)) == 0 ||
        memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
        return;
    }
    available = (unsigned long)memory.BaseAddress + memory.RegionSize - esp;
    words = available / sizeof(unsigned long);
    if (words > 256) {
        words = 256;
    }
    for (index = 0; index < words; ++index) {
        _snprintf(source, sizeof(source), "stack+%x",
                  index * sizeof(unsigned long));
        source[sizeof(source) - 1] = 0;
        ReportCandidate(source, stack[index]);
    }
}

LONG WINAPI ReportUnhandledException(EXCEPTION_POINTERS* exception)
{
    EXCEPTION_RECORD* record = exception->ExceptionRecord;
    CONTEXT* context = exception->ContextRecord;
    const char* operation = "unknown";
    unsigned long access_address = 0;
    unsigned long headers_size;
    int image_base_fault;
    int mz_stub = 0;
    int consumed_return = 0;

    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        record->NumberParameters >= 2) {
        switch (record->ExceptionInformation[0]) {
        case 0:
            operation = "read";
            break;
        case 1:
            operation = "write";
            break;
        case 8:
            operation = "execute";
            break;
        default:
            operation = "access";
            break;
        }
        access_address = (unsigned long)record->ExceptionInformation[1];
    }

    g_candidate_count = 0;
    g_main_image_base = (unsigned long)GetModuleHandleA(0);
    headers_size = g_main_image_base != 0 ? ImageHeadersSize(g_main_image_base) : 0;
    image_base_fault =
        g_main_image_base != 0 &&
        (unsigned long)context->Eip >= g_main_image_base &&
        (unsigned long)context->Eip < g_main_image_base + headers_size;
    if (image_base_fault) {
        mz_stub = *(const unsigned short*)g_main_image_base == IMAGE_DOS_SIGNATURE;
        /* The DOS stub begins "4d 5a": dec ebp, then pop edx. Once EIP is
           past those two bytes, EDX holds the return address the bogus stub
           stole from the stack. */
        consumed_return = mz_stub && (unsigned long)context->Eip >= g_main_image_base + 2;
    }

    fprintf(stderr,
            "WIZ8_RUNTIME_CRASH code=%08lx thread=%08lx operation=%s "
            "access=%08lx eip=%08lx esp=%08lx ebp=%08lx eax=%08lx ebx=%08lx "
            "ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            (unsigned long)record->ExceptionCode,
            (unsigned long)GetCurrentThreadId(), operation, access_address,
            (unsigned long)context->Eip, (unsigned long)context->Esp,
            (unsigned long)context->Ebp, (unsigned long)context->Eax,
            (unsigned long)context->Ebx, (unsigned long)context->Ecx,
            (unsigned long)context->Edx, (unsigned long)context->Esi,
            (unsigned long)context->Edi);

    if (image_base_fault) {
        fprintf(stderr,
                "WIZ8_RUNTIME_IMAGE_BASE_FAULT base=%08lx eip=%08lx mz=%u "
                "forced-unresolved=1",
                g_main_image_base, (unsigned long)context->Eip, mz_stub ? 1u : 0u);
        if (consumed_return) {
            fprintf(stderr, " consumed=edx:%08lx", (unsigned long)context->Edx);
        }
        fprintf(stderr, "\n");
        if (consumed_return) {
            ReportCandidate("return:edx", (unsigned long)context->Edx);
        }
    }

    ReportCandidate("reg:eax", (unsigned long)context->Eax);
    ReportCandidate("reg:ebx", (unsigned long)context->Ebx);
    ReportCandidate("reg:ecx", (unsigned long)context->Ecx);
    ReportCandidate("reg:edx", (unsigned long)context->Edx);
    ReportCandidate("reg:esi", (unsigned long)context->Esi);
    ReportCandidate("reg:edi", (unsigned long)context->Edi);
    ReportCandidate("reg:ebp", (unsigned long)context->Ebp);
    ReportStackCandidates((unsigned long)context->Esp);

    if (g_context_writer != 0) {
        g_context_writer(stderr);
    }
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

struct CrashReportInstaller {
    CrashReportInstaller()
    {
        SetUnhandledExceptionFilter(ReportUnhandledException);
    }
};

CrashReportInstaller g_crash_report_installer;

}  // namespace

void W8SetCrashContextWriter(W8CrashContextWriter writer)
{
    g_context_writer = writer;
}
