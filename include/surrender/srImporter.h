#pragma once

#include "srBinIStream.h"
#include "srColorSurface.h"
#include "srIOManager.h"
#include "srOptionList.h"

class srModel;

class srSurfaceIOManager : public srIOManager {
public:
    struct ImportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    struct ExportInfo {
        unsigned long unknown_00;
        unsigned long unknown_04;
        const char* option_string;
    };

    class SurfaceImporter;
    class SurfaceExporter;

    SR_DLL_IMPORT void exportSurface(const char* path, srColorSurfaceIFace& surface,
                                     const ExportInfo& options);
};

static_assert((sizeof(srSurfaceIOManager::ImportInfo) == 0x0c), "srSurfaceImportInfo_must_be_0x0c");
static_assert((sizeof(srSurfaceIOManager::ExportInfo) == 0x0c), "srSurfaceExportInfo_must_be_0x0c");

class __declspec(novtable) srSurfaceIOManager::SurfaceImporter : public srIOManager::Importer {
public:
    virtual SR_DLL_IMPORT int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description,
                                             srBinIStream& stream,
                                             const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(srBinIStream& stream,
                                               const srSurfaceIOManager::ImportInfo& options) = 0;
};

class srHierarchyIOManager : public srIOManager {
public:
    class ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class HierarchyImporter;
    class HierarchyExporter;

    srHierarchyIOManager();
    srHierarchyIOManager(const srHierarchyIOManager& other);
    srHierarchyIOManager& operator=(const srHierarchyIOManager& other);
    virtual ~srHierarchyIOManager();

    SR_DLL_IMPORT void importHierarchy(const char* path, const ImportInfo& options);
    SR_DLL_IMPORT void exportHierarchy(const char* path, const ExportInfo& options);
};

static_assert((sizeof(srHierarchyIOManager) == 0x1c), "srHierarchyIOManager_must_be_0x1c");
static_assert((sizeof(srHierarchyIOManager::ImportInfo) == 0x01),
              "srHierarchyImportInfo_must_be_0x01");
static_assert((sizeof(srHierarchyIOManager::ExportInfo) == 0x01),
              "srHierarchyExportInfo_must_be_0x01");

class __declspec(novtable) srHierarchyIOManager::HierarchyImporter : public srIOManager::Importer {
public:
    HierarchyImporter() {}
};

class srModelIOManager : public srIOManager {
public:
    class ImportInfo {
    public:
        unsigned char unknown_00;
    };
    class ExportInfo {
    public:
        unsigned char unknown_00;
    };

    class ModelImporter;
    class ModelExporter;

    srModelIOManager();
    srModelIOManager(const srModelIOManager& other);
    srModelIOManager& operator=(const srModelIOManager& other);
    virtual ~srModelIOManager();

    SR_DLL_IMPORT srModel* importModel(const char* path, const ImportInfo& options);
    SR_DLL_IMPORT void exportModel(const char* path, srModel& model, const ExportInfo& options);
};

static_assert((sizeof(srModelIOManager) == 0x1c), "srModelIOManager_must_be_0x1c");
static_assert((sizeof(srModelIOManager::ImportInfo) == 0x01), "srModelImportInfo_must_be_0x01");
static_assert((sizeof(srModelIOManager::ExportInfo) == 0x01), "srModelExportInfo_must_be_0x01");

class __declspec(novtable) srModelIOManager::ModelImporter : public srIOManager::Importer {
public:
    ModelImporter() {}
};
