#include <ctype.h>
#include <string.h>

#include "surrender/srBinFStream.h"
#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srExporter.h"
#include "surrender/srHeap.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srImporter.h"
#include "surrender/srString.h"

// FUNCTION: SURRENDER 0x1002CB10
const char* srIOManager::Error::getDescription()
{
    return description;
}

// FUNCTION: SURRENDER 0x1002C890
srIOManager::srIOManager() {}

// FUNCTION: SURRENDER 0x1002C920
srIOManager::~srIOManager() {}

// FUNCTION: SURRENDER 0x1002D1C0
const char* srIOManager::getExtension(const char* path)
{
    for (long index = (long)strlen(path) - 1;
         index >= 0 && path[index] != '/' && path[index] != '\\'; --index) {
        if (path[index] == '.') {
            return path + index + 1;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1002C9D0
srIOManager::Importer* srIOManager::findImporter(const char* extension)
{
    if (extension == 0 || *extension == '\0') {
        return 0;
    }
    char* upper = new char[strlen(extension) + 1];
    strcpy(upper, extension);
    for (unsigned long index = 0; index < strlen(extension); ++index) {
        if (islower(upper[index]) != 0) {
            upper[index] = (char)toupper(upper[index]);
        }
    }
    Importer* result = importers.find(upper);
    delete[] upper;
    return result;
}

// FUNCTION: SURRENDER 0x1002CB20
srIOManager::Exporter* srIOManager::findExporter(const char* extension)
{
    if (extension == 0 || *extension == '\0') {
        return 0;
    }
    char* upper = new char[strlen(extension) + 1];
    strcpy(upper, extension);
    for (unsigned long index = 0; index < strlen(extension); ++index) {
        if (islower(upper[index]) != 0) {
            upper[index] = (char)toupper(upper[index]);
        }
    }
    Exporter* result = exporters.find(upper);
    delete[] upper;
    return result;
}

// FUNCTION: SURRENDER 0x1002CCC0
void srIOManager::addImporter(Importer* importer, const char* extension)
{
    if (importer != 0 && extension != 0 && *extension != '\0') {
        char* upper = new char[strlen(extension) + 1];
        strcpy(upper, extension);
        long length = strlen(upper);
        for (long index = 0; index < length; ++index) {
            upper[index] = (char)toupper(upper[index]);
        }
        importers.insert(importers.sentinel, upper, importer);
    }
}

// FUNCTION: SURRENDER 0x1002CE30
void srIOManager::addExporter(Exporter* exporter, const char* extension)
{
    if (exporter != 0 && extension != 0 && *extension != '\0') {
        char* upper = new char[strlen(extension) + 1];
        strcpy(upper, extension);
        long length = strlen(upper);
        for (long index = 0; index < length; ++index) {
            upper[index] = (char)toupper(upper[index]);
        }
        exporters.insert(exporters.sentinel, upper, exporter);
    }
}

// FUNCTION: SURRENDER 0x1002CFA0
void srIOManager::removeImporter(Importer* importer)
{
    if (importer == 0) {
        return;
    }
    ImporterRegistration* node;
    while ((node = importers.find(importer)) != 0) {
        delete[] node->extension;
        importers.erase(node);
    }
}

// FUNCTION: SURRENDER 0x1002D090
void srIOManager::removeExporter(Exporter* exporter)
{
    if (exporter == 0) {
        return;
    }
    ExporterRegistration* node;
    while ((node = exporters.find(exporter)) != 0) {
        delete[] node->extension;
        exporters.erase(node);
    }
}

// FUNCTION: SURRENDER 0x1002D100
void srIOManager::dump()
{
    srPrintf("extension   importer\n");
    srPrintf("-----------------------------------------------------------------\n");
    ImporterRegistration* node = importers.first;
    while (node != importers.sentinel) {
        srPrintf("%-8s    '%s'\n", node->extension, node->importer->getTypeName());
        node = node->next;
    }
    srPrintf("-----------------------------------------------------------------\n");
    srPrintf("total %d instances\n", importers.count);
    srPrintf("extension   exporter\n");
    srPrintf("-----------------------------------------------------------------\n");
    ExporterRegistration* exporter_node = exporters.first;
    while (exporter_node != exporters.sentinel) {
        srPrintf("%-8s    '%s'\n", exporter_node->extension,
                 exporter_node->exporter->getTypeName());
        exporter_node = exporter_node->next;
    }
    srPrintf("-----------------------------------------------------------------\n");
    srPrintf("total %d instances\n", exporters.count);
}

// FUNCTION: SURRENDER 0x1002D200
void srIOManager::Importer::addToImporters(srIOManager* manager, const char* extension)
{
    manager->addImporter(this, extension);
}

// FUNCTION: SURRENDER 0x1002D220
void srIOManager::Exporter::addToExporters(srIOManager* manager, const char* extension)
{
    manager->addExporter(this, extension);
}

// FUNCTION: SURRENDER 0x1002D240
void srIOManager::Importer::addToImporters(srIOManager* manager, srIOManager::Importer* importer,
                                           const char* extension)
{
    manager->addImporter(importer, extension);
}

// FUNCTION: SURRENDER 0x1002D260
void srIOManager::Exporter::addToExporters(srIOManager* manager, srIOManager::Exporter* exporter,
                                           const char* extension)
{
    manager->addExporter(exporter, extension);
}

// FUNCTION: SURRENDER 0x1002D280
void srIOManager::Importer::removeFromImporters(srIOManager* manager)
{
    manager->removeImporter(this);
}

// FUNCTION: SURRENDER 0x1002D290
void srIOManager::Exporter::removeFromExporters(srIOManager* manager)
{
    manager->removeExporter(this);
}

// FUNCTION: SURRENDER 0x1002D300
void srIOManager::ImporterList::insert(ImporterRegistration* position, char* extension,
                                       Importer* importer)
{
    ImporterRegistration* node = new ImporterRegistration();
    node->extension = extension;
    node->importer = importer;
    node->next = position;
    node->previous = position->previous;
    if (node->previous != 0) {
        node->previous->next = node;
    } else {
        first = node;
    }
    if (node->next != 0) {
        node->next->previous = node;
    }
    ++count;
}

// FUNCTION: SURRENDER 0x1002D360
void srIOManager::ExporterList::insert(ExporterRegistration* position, char* extension,
                                       Exporter* exporter)
{
    ExporterRegistration* node = new ExporterRegistration();
    node->extension = extension;
    node->exporter = exporter;
    node->next = position;
    node->previous = position->previous;
    if (node->previous != 0) {
        node->previous->next = node;
    } else {
        first = node;
    }
    if (node->next != 0) {
        node->next->previous = node;
    }
    ++count;
}

// FUNCTION: SURRENDER 0x1002D440
void srHierarchyIOManager::importHierarchy(const char* path, const ImportInfo& options)
{
    if (path != 0 && *path != '\0') {
        HierarchyImporter* importer =
            static_cast<HierarchyImporter*>(findImporter(getExtension(path)));
        if (importer == 0) {
            throw Error("srHierarchyIOManager::importHierarchy: Importer could not be found");
        }
        srBinIStream* stream = srCore.getIStreamOpener()->open(path);
        if (stream != 0 && stream->good()) {
            importer->importHierarchy(*stream, options);
            delete static_cast<srBinStream*>(stream);
            return;
        }
        throw Error("srHierarchyIOManager::importHierarchy: File could not be opened");
    }
    throw Error("srHierarchyIOManager::importHierarchy: Given filename is NULL or empty");
}

// FUNCTION: SURRENDER 0x1002D540
void srHierarchyIOManager::exportHierarchy(const char* path, const ExportInfo& options)
{
    if (path == 0 || *path == '\0') {
        throw Error("srHierarchyIOManager::exportHierarchy: Given filename is NULL or empty");
    }
    HierarchyExporter* exporter = static_cast<HierarchyExporter*>(findExporter(getExtension(path)));
    if (exporter == 0) {
        throw Error("srHierarchyIOManager::exportHierarchy: Exporter could not be found");
    }
    srBinOFStream stream(path);
    if (stream.good()) {
        exporter->exportHierarchy(stream, options);
        return;
    }
    throw Error("srHierarchyIOManager::exportHierarchy: File could not be opened");
}

// FUNCTION: SURRENDER 0x1002D6A0
srModel* srModelIOManager::importModel(const char* path, const ImportInfo& options)
{
    if (path != 0 && *path != '\0') {
        ModelImporter* importer = static_cast<ModelImporter*>(findImporter(getExtension(path)));
        if (importer == 0) {
            throw Error("srModelIOManager::importModel: Importer could not be found");
        }
        srBinIStream* stream = srCore.getIStreamOpener()->open(path);
        if (stream != 0 && stream->good()) {
            srModel* model = importer->importModel(*stream, options);
            delete static_cast<srBinStream*>(stream);
            return model;
        }
        throw Error("srModelIOManager::importModel: File could not be opened");
    }
    throw Error("srModelIOManager::import: Given filename is NULL or empty");
}

// FUNCTION: SURRENDER 0x1002D7A0
void srModelIOManager::exportModel(const char* path, srModel& model, const ExportInfo& options)
{
    if (path != 0 && *path != '\0') {
        ModelExporter* exporter = static_cast<ModelExporter*>(findExporter(getExtension(path)));
        if (exporter != 0) {
            srBinOFStream stream(path);
            if (stream.good()) {
                exporter->exportModel(stream, model, options);
                return;
            }
            throw Error("srModelIOManager::exportModel: File could not be opened");
        }
    }
    throw Error("srModelIOManager::exportModel: Exporter could not be found");
}

// FUNCTION: SURRENDER 0x1002D890
srSurfaceIOManager::SurfaceImporter* srSurfaceIOManager::getImporter(const char* path)
{
    if (path == 0) {
        return 0;
    }
    return static_cast<SurfaceImporter*>(findImporter(getExtension(path)));
}

// FUNCTION: SURRENDER 0x1002D8C0
srSurfaceIOManager::SurfaceExporter* srSurfaceIOManager::getExporter(const char* path)
{
    if (path == 0) {
        return 0;
    }
    return static_cast<SurfaceExporter*>(findExporter(getExtension(path)));
}

// FUNCTION: SURRENDER 0x1002D8F0
void srSurfaceIOManager::getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc& description,
                                        const char* path, const ImportInfo& options)
{
    if (path != 0 && *path != '\0') {
        SurfaceImporter* importer = static_cast<SurfaceImporter*>(findImporter(getExtension(path)));
        if (importer != 0) {
            srBinIStream* stream = srCore.getIStreamOpener()->open(path);
            if (stream != 0 && stream->good()) {
                if (importer->getSurfaceDesc(description, *stream, options) == 0) {
                    throw Error("srSurfaceIOManager::getSurfaceDesc() - Importer failed to "
                                "load surface!");
                }
                delete static_cast<srBinStream*>(stream);
                return;
            }
            throw Error("srSurfaceIOManager::getSurfaceDesc(): File could not be opened");
        }
    }
    throw Error("srSurfaceIOManager::getSurfaceDesc() - Importer for this file extension "
                "not found.");
}

// FUNCTION: SURRENDER 0x1002DD70
int srSurfaceIOManager::SurfaceImporter::getSurfaceDesc(
    srColorSurfaceIFace::SurfaceDesc& description, srBinIStream& stream,
    const srSurfaceIOManager::ImportInfo& options)
{
    srColorSurfaceIFace* surface = importSurface(stream, options);
    if (surface == 0) {
        return 0;
    }
    surface->getSurfaceDesc(description);
    surface->release();
    return 1;
}

// FUNCTION: SURRENDER 0x1002DB20
srColorSurfaceIFace* srSurfaceIOManager::importSurface(const char* path, srBinIStream& stream,
                                                       const ImportInfo& options)
{
    if (path != 0 && *path != '\0') {
        /* Retail tests the stream parameter's storage for null (TEST on the
           lowered reference pointer) before the vbase-adjusted good() call. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-undefined-compare"
        if (&stream != 0 && stream.good()) {
#pragma clang diagnostic pop
            Importer* importer = findImporter(getExtension(path));
            if (importer != 0) {
                return static_cast<SurfaceImporter*>(importer)->importSurface(stream, options);
            }
            throw Error("srSurfaceIOManager::importSurface() - Importer for this file "
                        "extension not found");
        }
        throw Error("srSurfaceIOManager::importSurface() - corrupt input stream");
    }
    throw Error("srSurfaceIOManager::importSurface() - given filename is NULL or empty");
}

// FUNCTION: SURRENDER 0x1002DA00
srColorSurfaceIFace* srSurfaceIOManager::importSurface(const char* path, const ImportInfo& options)
{
    if (path != 0 && *path != '\0') {
        const char* extension = getExtension(path);
        Importer* importer = findImporter(extension);
        if (importer == 0) {
            throw Error("srSurfaceIOManager::importSurface() - Importer for this file "
                        "extension not found");
        }
        srBinIStream* stream = srCore.getIStreamOpener()->open(path);
        if (stream != 0 && stream->good()) {
            srColorSurfaceIFace* surface =
                static_cast<SurfaceImporter*>(importer)->importSurface(*stream, options);
            if (surface == 0) {
                throw Error("srSurfaceIOManager::importSurface() - Importer failed to "
                            "load surface!");
            }
            delete static_cast<srBinStream*>(stream);
            return surface;
        }
        throw Error("srSurfaceIOManager::importSurface: File could not be opened");
    }
    throw Error("srSurfaceIOManager::importSurface: Given filename is NULL or empty");
}

// FUNCTION: SURRENDER 0x1002DBC0
void srSurfaceIOManager::exportSurface(const char* path, srColorSurfaceIFace& surface,
                                       const ExportInfo& options)
{
    if (path != 0 && *path != '\0') {
        const char* extension = getExtension(path);
        Exporter* exporter = findExporter(extension);
        if (exporter == 0) {
            throw Error("srSurfaceIOManager::exportSurface() - Exporter not found");
        }
        srBinOFStream stream(path);
        if (stream.good()) {
            static_cast<SurfaceExporter*>(exporter)->exportSurface(stream, surface, options);
            return;
        }
        throw Error("srSurfaceIOManager::exportSurface() - File could not be opened");
    }
    throw Error("srSurfaceIOManager::exportSurface() - Given filename is NULL or empty");
}

// FUNCTION: SURRENDER 0x1002DCD0
void srSurfaceIOManager::exportSurface(const char* path, srBinOStream& stream,
                                       srColorSurfaceIFace& surface, const ExportInfo& options)
{
    if (path != 0 && *path != '\0') {
        /* Retail tests the stream parameter's storage for null (TEST on the
           lowered reference pointer) before the vbase-adjusted good() call. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-undefined-compare"
        if (&stream != 0 && stream.good()) {
#pragma clang diagnostic pop
            Exporter* exporter = findExporter(getExtension(path));
            if (exporter != 0) {
                static_cast<SurfaceExporter*>(exporter)->exportSurface(stream, surface, options);
                return;
            }
            throw Error("srSurfaceIOManager::exportSurface() - exporter not found");
        }
        throw Error("srSurfaceIOManager::exportSurface() - output stream is corrupt");
    }
    throw Error("srSurfaceIOManager::exportSurface() - given filename is NULL or empty");
}

/* The IO-manager units emit the deleting-destructor wrappers and the
   material class-support registrations they reference. */
