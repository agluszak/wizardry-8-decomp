#pragma once

#include "surrender/srCore.h"
#include "surrender/srExporter.h"
#include "surrender/srPlugin.h"

#include "layout.h"

class srJPEGColorSurface : public srColorSurface {
public:
    static void* operator new(size_t size)
    {
        return srHeap.allocate(size);
    }

    static void operator delete(void* memory)
    {
        srHeap.free(memory);
    }

    srJPEGColorSurface(
        srPixelConvert::e_surfaceType type,
        unsigned long width,
        unsigned long height)
        : srColorSurface(type, width, height) {}

    virtual const char* getClassName() const;
    virtual unsigned long getClassID() const;
    virtual srRegistry::ClassNode* getClassNode() const;
    virtual srColorSurfaceIFace* clone();
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x100151D0
// srJPEGColorSurface::`scalar deleting destructor'

class srJPEGImporter :
    public srSurfaceIOManager::SurfaceImporter,
    public srSurfaceIOManager::SurfaceExporter {
public:
    srJPEGImporter();
    virtual ~srJPEGImporter();

    virtual const char* getTypeName() const;
    virtual int getSurfaceDesc(
        srColorSurfaceIFace::SurfaceDesc& description,
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(
        srBinIStream& stream,
        const srSurfaceIOManager::ImportInfo& options);
    virtual void exportSurface(
        srBinOStream& stream,
        srColorSurfaceIFace& surface,
        const srSurfaceIOManager::ExportInfo& options);

private:
    void initializeCodecOptions();
    bool readHeader(void* input_cookie);

    JpegCodecState32 codec_;
    JpegExportOptions32 export_options_;
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014DB0
// srJPEGImporter::`scalar deleting destructor'
// SYNTHETIC: SREXT_JPEGIMPORTER 0x100155E0
// ?getTypeName@srJPEGImporter@@W3BEPBDXZ
// SYNTHETIC: SREXT_JPEGIMPORTER 0x100155F0
// ??_EsrJPEGImporter@@W3AEPAXI@Z

class srJPEGPlugin : public srPlugin {
public:
    virtual ~srJPEGPlugin();
    virtual const char* getDescription() const;

private:
    srJPEGImporter jpeg_importer_;
};

// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014BB0
// srJPEGPlugin::`scalar deleting destructor'
// SYNTHETIC: SREXT_JPEGIMPORTER 0x10014BD0
// srJPEGPlugin::~srJPEGPlugin
