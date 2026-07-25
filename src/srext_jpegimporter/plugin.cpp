#include "surrender/srCore.h"
#include "surrender/srExporter.h"
#include "surrender/srPlugin.h"

#include "layout.h"

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
    JpegCodecState32 codec_;
    JpegExportOptions32 export_options_;
};

class srJPEGPlugin : public srPlugin {
public:
    virtual ~srJPEGPlugin() {}
    virtual const char* getDescription() const;

private:
    srJPEGImporter jpeg_importer_;
};

srJPEGImporter::srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    addToImporters(manager, "jpeg");
    addToImporters(manager, "jpg");
    addToExporters(manager, "jpeg");
    addToExporters(manager, "jpg");
}

srJPEGImporter::~srJPEGImporter()
{
    srSurfaceIOManager* manager = srCore.getSurfaceIOManager();
    removeFromImporters(manager);
    removeFromExporters(manager);
}

const char* srJPEGImporter::getTypeName() const
{
    return "JPEG";
}

int srJPEGImporter::getSurfaceDesc(
    srColorSurfaceIFace::SurfaceDesc&,
    srBinIStream&,
    const srSurfaceIOManager::ImportInfo&)
{
    return 0;
}

srColorSurfaceIFace* srJPEGImporter::importSurface(
    srBinIStream&,
    const srSurfaceIOManager::ImportInfo&)
{
    return 0;
}

void srJPEGImporter::exportSurface(
    srBinOStream&,
    srColorSurfaceIFace&,
    const srSurfaceIOManager::ExportInfo&)
{
}

const char* srJPEGPlugin::getDescription() const
{
    return "SurRender JPEG-importer/exporter plug-in";
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

typedef char srJPEGImporter_must_be_0x44[(sizeof(srJPEGImporter) == 0x44) ? 1 : -1];
typedef char srJPEGPlugin_must_be_0x48[(sizeof(srJPEGPlugin) == 0x48) ? 1 : -1];
