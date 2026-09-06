#include "surrender/srVectorProcessor.h"

#include "surrender/srDebug.h"
#include "surrender/srDynamicLibrary.h"
#include <stdio.h>

srVP* srVectorProcessor::vp = 0;
srVP* srVectorProcessor::base_100a9240 = 0;
srVP* srVectorProcessor::debug_100a9244 = 0;
unsigned long srVectorProcessor::state_100a9248 = 0;
void* srVectorProcessor::module_100a924c = 0;

// FUNCTION: SURRENDER 0x10064370
const char* srVectorProcessor::getName()
{
    return vp != 0 ? vp->getName() : 0;
}

// FUNCTION: SURRENDER 0x10064390
void srVectorProcessor::install10064390(srVP* processor)
{
    if (vp != 0) {
        release();
    }
    vp = processor;
    base_100a9240 = processor;
    debug_100a9244 = 0;
    state_100a9248 = 0;
}

// FUNCTION: SURRENDER 0x10064920
int srVectorProcessor::load(const char* filename)
{
    if (filename == 0) {
        return 0;
    }
    void* module = srDynamicLibrary::load(filename);
    if (module == 0) {
        return 0;
    }
    srGetVectorProcessorAPIFn get_api = reinterpret_cast<srGetVectorProcessorAPIFn>(
        srDynamicLibrary::getFunction(module, "srGetVectorProcessorAPI"));
    srInitVectorProcessorFn init_processor = reinterpret_cast<srInitVectorProcessorFn>(
        srDynamicLibrary::getFunction(module, "srInitVectorProcessor"));
    if (get_api == 0) {
        srDebugPrintf(0, "srVectorProcessor::load() -- cannot locate function 'srGetVectorProcessorAPI' for VP file '%s'\n", filename);
    }
    else if (init_processor == 0) {
        srDebugPrintf(0, "srVectorProcessor::load() -- cannot locate function 'srInitVectorProcessor' for VP file '%s'\n", filename);
    }
    else if (get_api() >= SR_VP_MIN_API_VERSION) {
        srVP* processor = init_processor();
        if (processor != 0) {
            install10064390(processor);
            module_100a924c = module;
            return 1;
        }
    }
    srDynamicLibrary::free(module);
    return 0;
}

// FUNCTION: SURRENDER 0x10064A40
long srVectorProcessor::getID(const char* filename)
{
    if (filename == 0) {
        return -1;
    }
    void* module = srDynamicLibrary::load(filename);
    if (module == 0) {
        return -1;
    }
    srGetVectorProcessorIDFn get_id = reinterpret_cast<srGetVectorProcessorIDFn>(
        srDynamicLibrary::getFunction(module, "srGetVectorProcessorID"));
    if (get_id == 0) {
        srDebugPrintf(0, "srVectorProcessor::getID() -- cannot locate function 'srGetVectorProcessorID' for VP file '%s'\n", filename);
        srDynamicLibrary::free(module);
        return -1;
    }
    long id = get_id();
    srDynamicLibrary::free(module);
    return id;
}

// FUNCTION: SURRENDER 0x10064BB0
void srVectorProcessor::release()
{
    if (vp != 0) {
        delete base_100a9240;
        delete debug_100a9244;
        if (module_100a924c != 0 && !srDynamicLibrary::free(module_100a924c)) {
            char message[512];
            sprintf(message, "srVectorProcessor::release () -- call to srDynamicLibrary::free() failed!\n");
            srDebugPrintf(0, message);
        }
        vp = 0;
        base_100a9240 = 0;
        debug_100a9244 = 0;
        module_100a924c = 0;
    }
}
