#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srColorSurface.h"
#include "srIOManager.h"
#include "srOptionList.h"

class srBinOStream;
class srModel;

// VTABLE: SURRENDER 0x10075418
// class srSurfaceIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srSurfaceIOManager : public srIOManager {
public:
    struct ImportInfo {
        unsigned long unknown_00;
    };

    struct ExportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    class SurfaceImporter;
    class SurfaceExporter;

    /* The default-constructor body contains base construction and a derived
       vftable store, consistent with implicit construction. */

    void exportSurface(const char* path, srBinOStream& stream, srColorSurfaceIFace& surface,
                       const ExportInfo& options);
    SR_DLL_IMPORT void exportSurface(const char* path, srColorSurfaceIFace& surface,
                                     const ExportInfo& options);
    srColorSurfaceIFace* importSurface(const char* path, srBinIStream& stream,
                                       const ImportInfo& options);
    srColorSurfaceIFace* importSurface(const char* path, const ImportInfo& options);
    SurfaceImporter* getImporter(const char* path);
    SurfaceExporter* getExporter(const char* path);
    void getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, const char* path,
                        const ImportInfo& options);
};

static_assert((sizeof(srSurfaceIOManager::ImportInfo) == 0x04), "srSurfaceImportInfo_must_be_0x04");
static_assert((sizeof(srSurfaceIOManager::ExportInfo) == 0x0c), "srSurfaceExportInfo_must_be_0x0c");

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    virtual int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, srBinIStream& stream,
                               const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(srBinIStream& stream,
                                               const srSurfaceIOManager::ImportInfo& options) = 0;
};

// VTABLE: SURRENDER 0x10075530 srHierarchyIOManager
// class srHierarchyIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srHierarchyIOManager : public srIOManager {
public:
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class HierarchyImporter;
    class HierarchyExporter;

    void importHierarchy(const char* path, const ImportInfo& options);
    void exportHierarchy(const char* path, const ExportInfo& options);
};

static_assert((sizeof(srHierarchyIOManager) == 0x1c), "srHierarchyIOManager_must_be_0x1c");
static_assert((sizeof(srHierarchyIOManager::ImportInfo) == 0x01),
              "srHierarchyImportInfo_must_be_0x01");
static_assert((sizeof(srHierarchyIOManager::ExportInfo) == 0x01),
              "srHierarchyExportInfo_must_be_0x01");

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srHierarchyIOManager::HierarchyImporter : public srIOManager::Importer {
public:
    virtual void importHierarchy(srBinIStream& stream, const ImportInfo& options) = 0;
};

// VTABLE: SURRENDER 0x10075534 srModelIOManager
// class srModelIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srModelIOManager : public srIOManager {
public:
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class ModelImporter;
    class ModelExporter;

    srModel* importModel(const char* path, const ImportInfo& options);
    void exportModel(const char* path, srModel& model, const ExportInfo& options);
};

static_assert((sizeof(srModelIOManager) == 0x1c), "srModelIOManager_must_be_0x1c");
static_assert((sizeof(srModelIOManager::ImportInfo) == 0x01), "srModelImportInfo_must_be_0x01");
static_assert((sizeof(srModelIOManager::ExportInfo) == 0x01), "srModelExportInfo_must_be_0x01");

class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srModelIOManager::ModelImporter : public srIOManager::Importer {
public:
    virtual srModel* importModel(srBinIStream& stream, const ImportInfo& options) = 0;
};
