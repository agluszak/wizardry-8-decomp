#pragma once

#include <iosfwd>

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

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srCore {
public:
    SR_DLL_IMPORT srCore();
    /* The exported assignment is consistent with whole-object memberwise copying. */

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
       direct [srCore + 0x170] read rather than an import thunk. The class
       dllexport still emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x10015730
    // RECOMP: ?getMaterial@srCore@@QBEPAVsrMaterial@@XZ
    srMaterial* getMaterial() const
    {
        return material;
    }
    SR_DLL_IMPORT srMemoryAllocator* getMemoryAllocator() const;
    SR_DLL_IMPORT srModelIOManager* getModelIOManager() const;
    SR_DLL_IMPORT srPalette* getPalette() const;
    SR_DLL_IMPORT srNode* getRootNode() const;
    /* Header-visible like getRegistry: the srBinIAsyncStream constructor
       queues its job through a direct [srCore + 0x00] read rather than an
       out-of-line accessor call. The class dllexport still emits the
       exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156A0
    // RECOMP: ?getScheduler@srCore@@QBEPAVsrScheduler@@XZ
    srScheduler* getScheduler() const
    {
        return scheduler;
    }
    /* Header-visible in the triangle pipeline: its statistics updates load
       the manager directly from srCore +0x28. The class dllexport still
       emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156B0
    // RECOMP: ?getStatisticsManager@srCore@@QBEPAVsrStatisticsManager@@XZ
    srStatisticsManager* getStatisticsManager() const
    {
        return statistics_manager;
    }
    SR_DLL_IMPORT srColorSurfaceIFace* getSurface() const;
    SR_DLL_IMPORT srTexture* getTexture() const;
    /* Header-visible like getRegistry/getMaterial: timer users in both Wiz8
       and recovered SR code read the pointer directly from srCore +0x08. The
       class dllexport still emits the exported standalone copy. */
    // FUNCTION: SURRENDER 0x100156C0
    // RECOMP: ?getTimer@srCore@@QBEPAVsrVariableTimer@@XZ
    srVariableTimer* getTimer() const
    {
        return timer;
    }
    SR_DLL_IMPORT unsigned long getUniqueID();
    SR_DLL_IMPORT srVideoManager* getVideoManager() const;
    SR_DLL_IMPORT int isInitialized() const;
    SR_DLL_IMPORT void setDebugLevel(unsigned char level);
    SR_DLL_IMPORT void setFileManager(srFileManager* manager);
    SR_DLL_IMPORT void setFilter(srFilter* filter);
    SR_DLL_IMPORT int supportMultiThread();
    SR_DLL_IMPORT void supportMultiThread(int enabled);

    /* Consumer expansions read the registry field directly; the provider also
       exports a standalone copy. This is consistent with a header-visible
       definition; exact original annotation spelling is unresolved. */
    // FUNCTION: SURRENDER 0x10015760
    // RECOMP: ?getRegistry@srCore@@QBEPAVsrRegistry@@XZ
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

    srScheduler* scheduler;
    srGlobalRecycler* global_recycler;
    srVariableTimer* timer;
    srColorSurfaceIFace* surface;
    srSurfaceIOManager* surface_io_manager;
    srIStreamOpener* stream_opener;
    srFStreamOpener* file_stream_opener;
    srFilter* filter;
    srMemoryAllocator* memory_allocator;
    srFileManager* file_manager;
    srStatisticsManager* statistics_manager;
    srRegistry* registry_;
    srFileManager* default_file_manager;
    srPalette* palette;
    unsigned long next_unique_id;
    char version_[0x20];
    char copyright_[0x100];
    /* Full dword member: setDebugLevel stores MOVZX+4-byte write and
       srDebugPrintf reads the dword before masking with 0xff. */
    unsigned long debug_level;
    int multi_thread;
    srNode* root_node;
    srModelIOManager* model_io_manager;
    srHierarchyIOManager* hierarchy_io_manager;
    srMaterial* material;
    srTexture* texture;
    srVideoManager* video_manager;
};

static_assert(sizeof(srCore) == 0x17c, "srCore_must_be_0x17c");

extern SR_DLL_IMPORT class srCore srCore;

/* DLL attach/detach hooks called by the library entry wrapper; the exported
   0x1000-byte default-surface image lives in core.cpp. */
void __cdecl _srLibraryInit(void);
void __cdecl _srLibraryExit(void);

extern const unsigned char srLogo[0x1000];
