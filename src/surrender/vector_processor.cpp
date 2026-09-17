#include "surrender/srVectorProcessor.h"

#include <stdio.h>

#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srDebugVP.h"
#include "surrender/srDynamicLibrary.h"
#include "surrender/srStringTable.h"
#include "surrender/srSystem.h"
#include "surrender/srVP_generic.h"

#if defined(WIZ8_CLANG_LINT)
/* The lint lane's stub <ostream> declares only the operator<< overloads the
   recovered ABI references. dump calls std::endl - the real VC6 header
   resolves it to the _CRTIMP char overload imported from MSVCP60 - so the
   compile-only lane needs this declaration to parse. */
namespace std {
ostream& endl(ostream& stream);
}
#endif

srVP* srVectorProcessor::vp = 0;
srVP* srVectorProcessor::base = 0;
srDebugVP* srVectorProcessor::debug = 0;
unsigned long srVectorProcessor::debug_active = 0;
void* srVectorProcessor::module = 0;

// FUNCTION: SURRENDER 0x10064370
const char* srVectorProcessor::getName()
{
    if (vp == 0) {
        return 0;
    }
    return vp->getName();
}

// FUNCTION: SURRENDER 0x10064390
void srVectorProcessor::install(srVP* processor)
{
    if (vp != 0) {
        release();
    }
    vp = processor;
    base = processor;
    debug = 0;
    debug_active = 0;
}

// FUNCTION: SURRENDER 0x100643D0
void srVectorProcessor::startDebug(int check_misalignments)
{
    if (debug_active == 0) {
        debug = new srDebugVP(vp);
        vp = debug;
        debug_active = 1;
        debug->check_misalignments_440 = check_misalignments;
    }
}

// FUNCTION: SURRENDER 0x10064450
void srVectorProcessor::endDebug()
{
    if (debug_active != 0) {
        vp = base;
        debug_active = 0;
        if (debug != 0) {
            delete debug;
        }
        debug = 0;
    }
}

// FUNCTION: SURRENDER 0x10064490
void srVectorProcessor::dump(std::ostream& stream)
{
    int command;
    int index;
    int entry;

    if (debug_active == 0) {
        srStreamPrintf(stream, "VP statistics only available with srDebugVP, use "
                               "srVectorProcessor::startDebug()\n");
        return;
    }
    srStreamPrintf(stream, "\n\n");
    srStreamPrintf(stream,
                   "------------------ srDebugVP statistics ---------------------------\n\n");
    srStreamPrintf(stream, "[%% of VP][cycles][calls][av elements][mis 8/16]\n\n");
    double frequency = srCore.getTimer()->m_frequency;
    double total = 0.0;
    int used = 0;
    for (command = 0; command < 0xa6; ++command) {
        if (debug->call_counts_eb0[command] != 0) {
            double time = debug->call_times_450[command] -
                          debug->call_counts_eb0[command] * debug->call_overhead_448;
            if (time <= 0.0) {
                time = 0.0;
            }
            total += time;
            ++used;
        }
    }
    if (used == 0) {
        return;
    }
    if (total == 0.0) {
        return;
    }
    SRDWORD* scores = static_cast<SRDWORD*>(operator new(used * 4));
    int* order = static_cast<int*>(operator new(used * 4));
    index = 0;
    for (command = 0; command < 0xa6; ++command) {
        if (debug->call_counts_eb0[command] != 0) {
            double time = debug->call_times_450[command] -
                          debug->call_counts_eb0[command] * debug->call_overhead_448;
            if (time <= 0.0) {
                time = 0.0;
            }
            scores[index] = -1 - static_cast<int>(time / total * 4294967295.0);
            order[index] = command;
            ++index;
        }
    }
    for (entry = 1; entry < used; ++entry) {
        SRDWORD score = scores[entry];
        int selected = order[entry];
        index = entry - 1;
        while (index >= 0 && scores[index] > score) {
            scores[index + 1] = scores[index];
            order[index + 1] = order[index];
            --index;
        }
        scores[index + 1] = score;
        order[index + 1] = selected;
    }
    for (index = 0; index < used; ++index) {
        command = order[index];
        double calls = debug->call_counts_eb0[command];
        double elements = debug->element_counts_980[command];
        double time = debug->call_times_450[command] - calls * debug->call_overhead_448;
        if (time <= 0.0) {
            time = 0.0;
        }
        char percent[0x40];
        char cycles[0x40];
        char call_text[0x40];
        char element_text[0x40];
        char misalignments[0x40];
        sprintf(percent, "%.2f%%", time * 100.0 / total);
        sprintf(cycles, "%.2f", time * frequency / calls);
        sprintf(call_text, "%d", static_cast<int>(calls));
        sprintf(element_text, "%d", static_cast<int>(elements / calls));
        sprintf(misalignments, "%d/%d", static_cast<int>(debug->misaligned8_1148[command]),
                static_cast<int>(debug->misaligned16_13e0[command]));
        srStreamPrintf(stream, "%-8s %-8s %-8s %-9s %-12s %s\n", percent, cycles, call_text,
                       element_text, misalignments, srDebugVP::command_names[command]);
    }
    srStreamPrintf(stream, "\n\n");
    if (debug->check_misalignments_440 == 0) {
        srStreamPrintf(stream, "misAlignments not checked\n");
    }
    srStreamPrintf(stream, "\n");
    std::endl(stream);
    operator delete(order);
    operator delete(scores);
}

// FUNCTION: SURRENDER 0x10064900
void srVectorProcessor::resetStatistics()
{
    if (debug_active != 0 && debug != 0) {
        debug->resetInternalStatistics();
    }
}

// FUNCTION: SURRENDER 0x10064920
int srVectorProcessor::load(const char* filename)
{
    if (filename == 0) {
        return 0;
    }
    void* new_module = srDynamicLibrary::load(filename);
    if (new_module == 0) {
        return 0;
    }
    srGetVectorProcessorAPIFn get_api = reinterpret_cast<srGetVectorProcessorAPIFn>(
        srDynamicLibrary::getFunction(new_module, "srGetVectorProcessorAPI"));
    srInitVectorProcessorFn init_processor = reinterpret_cast<srInitVectorProcessorFn>(
        srDynamicLibrary::getFunction(new_module, "srInitVectorProcessor"));
    if (init_processor == 0 || get_api == 0) {
        if (get_api == 0) {
            srDebugPrintf(0,
                          "srVectorProcessor::load() -- cannot locate function "
                          "'srGetVectorProcessorAPI' for VP file '%s'\n",
                          filename);
        } else {
            srDebugPrintf(0,
                          "srVectorProcessor::load() -- cannot locate function "
                          "'srInitVectorProcessor' for VP file '%s'\n",
                          filename);
        }
    } else if (get_api() >= SR_VP_MIN_API_VERSION) {
        srVP* processor = init_processor();
        if (processor != 0) {
            install(processor);
            module = new_module;
            return 1;
        }
    }
    srDynamicLibrary::free(new_module);
    return 0;
}

// FUNCTION: SURRENDER 0x100649C0
void srVectorProcessor::initBaseVP()
{
    install(new srVP_generic);
    module = 0;
}

// FUNCTION: SURRENDER 0x10064A40
long srVectorProcessor::getID(const char* filename)
{
    if (filename == 0) {
        return -1;
    }
    void* probe_module = srDynamicLibrary::load(filename);
    if (probe_module == 0) {
        return -1;
    }
    srGetVectorProcessorIDFn get_id = reinterpret_cast<srGetVectorProcessorIDFn>(
        srDynamicLibrary::getFunction(probe_module, "srGetVectorProcessorID"));
    if (get_id == 0) {
        srDebugPrintf(0,
                      "srVectorProcessor::getID() -- cannot locate function "
                      "'srGetVectorProcessorID' for VP file '%s'\n",
                      filename);
        srDynamicLibrary::free(probe_module);
        return -1;
    }
    long id = get_id();
    srDynamicLibrary::free(probe_module);
    return id;
}

// FUNCTION: SURRENDER 0x10064AB0
int srVectorProcessor::loadBest(const char* path)
{
    srStringTable libraries;
    long best = 0;
    long count;
    int index;

    install(0);
    srSystem::scanLibraries(libraries, path, "srVP_*");
    count = libraries.getCount();
    for (index = 0; index < count; ++index) {
        long id = getID(libraries.getString(index));
        if (best <= id) {
            if (load(libraries.getString(index)) != 0) {
                best = id;
            }
        }
    }
    if (vp == 0) {
        return 0;
    }
    srDebugPrintf(0, "srVectorProcessor::load() -- vector processor '%s' loaded.\n", vp->getName());
    return 1;
}

// FUNCTION: SURRENDER 0x10064BB0
void srVectorProcessor::release()
{
    if (vp != 0) {
        delete base;
        delete debug;
        if (module != 0 && !srDynamicLibrary::free(module)) {
            char message[512];
            sprintf(message,
                    "srVectorProcessor::release () -- call to srDynamicLibrary::free() failed!\n");
            srDebugPrintf(0, message);
        }
        vp = 0;
        base = 0;
        debug = 0;
        module = 0;
    }
}
