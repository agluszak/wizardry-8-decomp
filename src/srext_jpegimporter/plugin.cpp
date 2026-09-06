#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <new>

#include "codec_adapter.h"
#include "plugin_classes.h"

srJPEGPlugin::~srJPEGPlugin()
{
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014D40
srJPEGImporter::srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    if (manager != 0) {
        addToImporters(manager, "jpg");
        addToImporters(manager, "jpeg");
        addToExporters(manager, "jpg");
        addToExporters(manager, "jpeg");
    }
    initializeCodecOptions();
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014DD0
srJPEGImporter::~srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    if (manager != 0) {
        removeFromImporters(manager);
        removeFromExporters(manager);
    }
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014E10
void srJPEGImporter::initializeCodecOptions()
{
    export_options_.limit_200 = 200;
    export_options_.quality = 75;
    export_options_.smoothing_factor = 0;
    export_options_.pointer_08 = 0;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10015420
const char* srJPEGImporter::getTypeName() const
{
    return "JPEG";
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014E30
bool srJPEGImporter::readHeader(void* input_cookie)
{
    memset(&codec_, 0, sizeof(codec_));
    codec_.input_stdio_cookie = input_cookie;
    srJPEG_read_header_adapter(&codec_);
    return codec_.failed == 0;
}

// TEMPLATE: SREXT_JPEGIMPORTER 0x10015450
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassID

// TEMPLATE: SREXT_JPEGIMPORTER 0x10015460
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassName

// TEMPLATE: SREXT_JPEGIMPORTER 0x10015470
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassNode

// TEMPLATE: SREXT_JPEGIMPORTER 0x100154E0
// srClassSupport<srColorSurface,srColorSurface,0,12560>::clone

// FUNCTION: SREXT_JPEGIMPORTER 0x10014BA0
const char* srJPEGPlugin::getDescription() const
{
    return "SurRender JPEG-importer/exporter plug-in";
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100155B0
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x100155D0
extern "C" unsigned long __cdecl srGetLibraryVersion()
{
    return 0x012A0209UL;
}

// FUNCTION: SREXT_JPEGIMPORTER 0x10014B70
extern "C" srPlugin* __cdecl srInitPlugin()
{
    return new srJPEGPlugin;
}

static_assert((sizeof(srJPEGImporter) == 0x44), "srJPEGImporter_must_be_0x44");
static_assert((sizeof(srJPEGPlugin) == 0x48), "srJPEGPlugin_must_be_0x48");
