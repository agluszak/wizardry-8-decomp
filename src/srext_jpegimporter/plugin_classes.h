#pragma once

#include "surrender/srCore.h"
#include "surrender/srExporter.h"
#include "surrender/srPlugin.h"

#include "layout.h"

class srJPEGImporter : public srSurfaceIOManager::SurfaceImporter,
                       public srSurfaceIOManager::SurfaceExporter {
public:
    srJPEGImporter();
    virtual ~srJPEGImporter();

    virtual const char* getTypeName() const;
    virtual int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description, srBinIStream& stream,
                               const srSurfaceIOManager::ImportInfo& options);
    virtual srColorSurfaceIFace* importSurface(srBinIStream& stream,
                                               const srSurfaceIOManager::ImportInfo& options);
    virtual void exportSurface(srBinOStream& stream, srColorSurfaceIFace& surface,
                               const srSurfaceIOManager::ExportInfo& options);

private:
    void initializeCodecOptions();
    bool readHeader(void* input_cookie);

    JpegCodecState codec_;
    JpegExportOptions export_options_;
};

class srJPEGPlugin : public srPlugin {
public:
    virtual ~srJPEGPlugin();
    virtual const char* getDescription() const;

private:
    srJPEGImporter jpeg_importer_;
};

static_assert((sizeof(srJPEGImporter) == 0x44), "srJPEGImporter_must_be_0x44");
static_assert((sizeof(srJPEGPlugin) == 0x48), "srJPEGPlugin_must_be_0x48");
