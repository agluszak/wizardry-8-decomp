#include "surrender/srCore.h"
#include "surrender/srBinIAsyncStream.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srMemoryPool.h"
#include "surrender/srMutex.h"
#include "surrender/srThread.h"

// FUNCTION: SURRENDER 0x10015010
const char* srCore::getCopyright() const
{
    return copyright_;
}

// FUNCTION: SURRENDER 0x10015030
const char* srCore::getVersion() const
{
    return version_;
}

// FUNCTION: SURRENDER 0x10015a80
srSurfaceIOManager* srCore::getSurfaceIOManager() const
{
    return surface_io_manager_10;
}

// FUNCTION: SURRENDER 0x10015a90
srVideoManager* srCore::getVideoManager() const
{
    return video_manager_178;
}

// FUNCTION: SURRENDER 0x10015aa0
srModelIOManager* srCore::getModelIOManager() const
{
    return model_io_manager_168;
}

// FUNCTION: SURRENDER 0x10015ab0
srHierarchyIOManager* srCore::getHierarchyIOManager() const
{
    return hierarchy_io_manager_16c;
}
