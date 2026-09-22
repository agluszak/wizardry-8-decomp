#include <stdio.h>
#include <string.h>

#include "surrender/srCore.h"
#include "surrender/srBinIAsyncStream.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srMemoryPool.h"
#include "surrender/srMutex.h"
#include "surrender/srString.h"
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

// FUNCTION: SURRENDER 0x10015B10
srMemoryAllocator* srCore::getMemoryAllocator() const
{
    return memory_allocator_20;
}

// FUNCTION: SURRENDER 0x10015AC0
srIStreamOpener* srCore::getIStreamOpener() const
{
    return stream_opener_14;
}

// FUNCTION: SURRENDER 0x10015B00
srNode* srCore::getRootNode() const
{
    return root_node_164;
}

// FUNCTION: SURRENDER 0x10015B20
srFileManager* srCore::getFileManager() const
{
    return file_manager_24;
}

// FUNCTION: SURRENDER 0x10015A60
srTexture* srCore::getTexture() const
{
    return texture_174;
}

/* TU-local srInlineString copy constructor: like file_stream.cpp's copies,
   this unit's implicit stream copy machinery needs a callable emission. */
inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source);
    }
}

inline srInlineString& srInlineString::operator=(const srInlineString& source)
{
    init();
    if (source.data_ != 0 && *source.data_ != '\0') {
        size_ = strlen(source.data_) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, source.data_);
    }
    return *this;
}

/* The provider's process-wide core object; its constructor body is still
   unrecovered, so the definition intentionally leaves that reference
   unresolved like the other first-party gaps. */
// GLOBAL: SURRENDER 0x100A45F8
class srCore srCore;

// FUNCTION: SURRENDER 0x10015B60
srCore::srCore()
{
    sprintf(version_, "%d.%d.%d.%d", 1, 42, 2, 9);
    sprintf(copyright_, "SurRender %s ", version_);
    strcat(copyright_, "Release");
    strcat(copyright_, " Build (");
    strcat(copyright_, "MSVC 6.0");
    strcat(copyright_, ") (c) Hybrid Holding Ltd. 1994-1999");
    multi_thread_160 = 0;
    global_recycler_04 = 0;
    scheduler_00 = 0;
    texture_174 = 0;
    surface_0c = 0;
    surface_io_manager_10 = 0;
    video_manager_178 = 0;
    model_io_manager_168 = 0;
    hierarchy_io_manager_16c = 0;
    filter_1c = 0;
    root_node_164 = 0;
    memory_allocator_20 = 0;
    default_file_manager_30 = 0;
    file_manager_24 = 0;
    palette_34 = 0;
    timer_08 = 0;
    registry_ = 0;
    next_unique_id_38 = 0;
    debug_level_15c = 1;
}
