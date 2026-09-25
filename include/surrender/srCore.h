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
       direct [srCore + 0x170] read rather than an import thunk. The member
       dllexport still emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x10015730
    // ?getMaterial@srCore@@QBEPAVsrMaterial@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srMaterial* getMaterial() const
    {
        return material_170;
    }
    SR_DLL_IMPORT srMemoryAllocator* getMemoryAllocator() const;
    SR_DLL_IMPORT srModelIOManager* getModelIOManager() const;
    SR_DLL_IMPORT srPalette* getPalette() const;
    SR_DLL_IMPORT srNode* getRootNode() const;
    /* Header-visible like getRegistry: the srBinIAsyncStream constructor
       queues its job through a direct [srCore + 0x00] read rather than an
       out-of-line accessor call. The member dllexport still emits the
       exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156A0
    // ?getScheduler@srCore@@QBEPAVsrScheduler@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srScheduler* getScheduler() const
    {
        return scheduler_00;
    }
    /* Header-visible in the triangle pipeline: its statistics updates load
       the manager directly from srCore +0x28. The member dllexport still
       emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156B0
    // ?getStatisticsManager@srCore@@QBEPAVsrStatisticsManager@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srStatisticsManager* getStatisticsManager() const
    {
        return statistics_manager_28;
    }
    SR_DLL_IMPORT srColorSurfaceIFace* getSurface() const;
    SR_DLL_IMPORT srTexture* getTexture() const;
    /* Header-visible like getRegistry/getMaterial: timer users in both Wiz8
       and recovered SR code read the pointer directly from srCore +0x08. The
       member dllexport still emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156C0
    // ?getTimer@srCore@@QBEPAVsrVariableTimer@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srVariableTimer* getTimer() const
    {
        return timer_08;
    }
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
       exact match. The member dllexport emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x10015760
    // ?getRegistry@srCore@@QBEPAVsrRegistry@@XZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srRegistry* getRegistry() const
    {
        return registry_;
    }

private:
    /* srDebugPrintf reads the dword level and masks 0xff directly rather than
       calling the byte-loading exported accessor. */
    friend long __cdecl srDebugPrintf(unsigned long level, const char* format, ...);
    /* srInit/srExit drive library lifecycle: they write the private field
       block and run the private reset() directly. */
    friend SR_DLL_IMPORT int __cdecl srInit(void);
    friend SR_DLL_IMPORT int __cdecl srExit(void);

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
    /* Full dword member: setDebugLevel stores MOVZX+4-byte write and
       srDebugPrintf reads the dword before masking with 0xff. */
    unsigned long debug_level_15c;
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

/* DLL attach/detach hooks called by the library entry wrapper; the exported
   0x1000-byte default-surface image lives in core.cpp. */
void __cdecl _srLibraryInit(void);
void __cdecl _srLibraryExit(void);

extern const unsigned char srLogo[0x1000];
