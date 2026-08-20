#pragma once

#include <iostream>

#include "srFileManager.h"
#include "srGlobalRecycler.h"
#include "srHeap.h"
#include "srMemoryAllocator.h"
#include "srScheduler.h"
#include "srStatisticsManager.h"
#include "srVariableTimer.h"

class srColorSurfaceIFace;
class srFilter;
class srFStreamOpener;
class srHierarchyIOManager;
class srIStreamOpener;
class srMaterial;
class srModelIOManager;
class srNode;
class srPalette;
class srRegistry;
class srSurfaceIOManager;
class srTexture;
class srVideoManager;

class srCore {
public:
    SR_DLL_IMPORT srCore();
    SR_DLL_IMPORT srCore& operator=(const srCore& other);

    SR_DLL_IMPORT void dump(std::ostream& stream);
    SR_DLL_IMPORT const char* getBuildTime() const;
    SR_DLL_IMPORT srSurfaceIOManager* getSurfaceIOManager() const;
    SR_DLL_IMPORT srIStreamOpener* getIStreamOpener() const;
    SR_DLL_IMPORT const char* getCopyright() const;
    SR_DLL_IMPORT const char* getVersion() const;
    SR_DLL_IMPORT unsigned char getDebugLevel() const;
    SR_DLL_IMPORT srFileManager* getFileManager() const;
    SR_DLL_IMPORT srFilter* getFilter() const;
    SR_DLL_IMPORT srGlobalRecycler* getGlobalRecycler() const;
    SR_DLL_IMPORT srHierarchyIOManager* getHierarchyIOManager() const;
    /* Inline like getRegistry: pipeline reset/get paths load material as a
       direct [srCore + 0x170] read rather than an import thunk. */
    srMaterial* getMaterial() const { return material_170; }
    SR_DLL_IMPORT srMemoryAllocator* getMemoryAllocator() const;
    SR_DLL_IMPORT srModelIOManager* getModelIOManager() const;
    SR_DLL_IMPORT srPalette* getPalette() const;
    SR_DLL_IMPORT srNode* getRootNode() const;
    SR_DLL_IMPORT srScheduler* getScheduler() const;
    /* Header-visible in the triangle pipeline: its statistics updates load
       the manager directly from srCore +0x28. */
    srStatisticsManager* getStatisticsManager() const {
        return statistics_manager_28;
    }
    SR_DLL_IMPORT srColorSurfaceIFace* getSurface() const;
    SR_DLL_IMPORT srTexture* getTexture() const;
    SR_DLL_IMPORT srVariableTimer* getTimer() const;
    SR_DLL_IMPORT unsigned long getUniqueID();
    SR_DLL_IMPORT srVideoManager* getVideoManager() const;
    SR_DLL_IMPORT int isInitialized() const;
    SR_DLL_IMPORT void setDebugLevel(unsigned char level);
    SR_DLL_IMPORT void setFileManager(srFileManager* manager);
    SR_DLL_IMPORT void setFilter(srFilter* filter);
    SR_DLL_IMPORT int supportMultiThread();
    SR_DLL_IMPORT void supportMultiThread(int enabled);

    /* Defined inline because Wiz8 inlines it. Every srClassSupport
       sGetClassNode emission loads the registry as a direct [srCore + 0x2c]
       field read rather than calling an import thunk, so the original header
       carried this body even though SR.DLL also exports an out-of-line copy.
       Declaring it SR_DLL_IMPORT instead costs every getClassNode body its
       exact match. */
    srRegistry* getRegistry() const { return registry_; }

private:
    SR_DLL_IMPORT void reset();

    static SR_DLL_IMPORT int initialized;

    srScheduler* scheduler_00;
    srGlobalRecycler* global_recycler_04;
    srVariableTimer* timer_08;
    srColorSurfaceIFace* surface_0c;
    srSurfaceIOManager* surface_io_manager_10;
    srIStreamOpener* stream_opener_14;
    srFStreamOpener* file_stream_opener_18;
    srFilter* filter_1c;
    srMemoryAllocator* memory_allocator_20;
    srFileManager* file_manager_24;
    srStatisticsManager* statistics_manager_28;
    srRegistry* registry_;
    srFileManager* default_file_manager_30;
    srPalette* palette_34;
    unsigned long next_unique_id_38;
    char version_[0x20];
    char copyright_[0x100];
    unsigned char debug_level_15c;
    unsigned char unknown_15d[3];
    int multi_thread_160;
    srNode* root_node_164;
    srModelIOManager* model_io_manager_168;
    srHierarchyIOManager* hierarchy_io_manager_16c;
    srMaterial* material_170;
    srTexture* texture_174;
    srVideoManager* video_manager_178;
};

static_assert(sizeof(srCore) == 0x17c, "srCore_must_be_0x17c");

extern SR_DLL_IMPORT class srCore srCore;
