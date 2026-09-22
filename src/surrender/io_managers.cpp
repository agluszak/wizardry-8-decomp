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

/* TU-local srInlineString copies like file_stream.cpp's: this unit's stream
   copy machinery references them and sibling units own the callable
   emissions. */
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

// FUNCTION: SURRENDER 0x1002CB00
srIOManager::Error::Error(const char* description)
{
    description_00 = description;
}

// FUNCTION: SURRENDER 0x1002CB10
const char* srIOManager::Error::getDescription()
{
    return description_00;
}

// FUNCTION: SURRENDER 0x1002C890
srIOManager::srIOManager()
{
    Registration* importers = new Registration();
    importers_04_.first_04 = importers;
    importers_04_.sentinel_08 = importers;
    importers->next_08 = 0;
    importers->previous_0c = 0;
    importers_04_.count_00 = 0;
    Registration* exporters = new Registration();
    exporters_10_.first_04 = exporters;
    exporters_10_.sentinel_08 = exporters;
    exporters->next_08 = 0;
    exporters->previous_0c = 0;
    exporters_10_.count_00 = 0;
}

// FUNCTION: SURRENDER 0x1002C920
srIOManager::~srIOManager()
{
    Registration* node = exporters_10_.first_04;
    while (node != exporters_10_.sentinel_08) {
        exporters_10_.first_04 = node->next_08;
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        node = exporters_10_.first_04;
        --exporters_10_.count_00;
    }
    delete exporters_10_.first_04;
    node = importers_04_.first_04;
    while (node != importers_04_.sentinel_08) {
        importers_04_.first_04 = node->next_08;
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        node = importers_04_.first_04;
        --importers_04_.count_00;
    }
    delete importers_04_.first_04;
}

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
    Importer* result = 0;
    for (Registration* node = importers_04_.first_04; node != importers_04_.sentinel_08;
         node = node->next_08) {
        if (strcmp(node->extension_00, upper) == 0) {
            result = node->importer_04;
            break;
        }
    }
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
    Exporter* result = 0;
    for (Registration* node = exporters_10_.first_04; node != exporters_10_.sentinel_08;
         node = node->next_08) {
        if (strcmp(node->extension_00, upper) == 0) {
            result = node->exporter_04;
            break;
        }
    }
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
        importers_04_.insert(importers_04_.sentinel_08, upper, importer);
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
        exporters_10_.insert(exporters_10_.sentinel_08, upper, exporter);
    }
}

// FUNCTION: SURRENDER 0x1002CFA0
void srIOManager::removeImporter(Importer* importer)
{
    if (importer == 0) {
        return;
    }
    Registration* node;
    do {
        node = importers_04_.first_04;
        while (true) {
            if (node == importers_04_.sentinel_08) {
                return;
            }
            if (node->importer_04 == importer) {
                break;
            }
            node = node->next_08;
        }
        delete[] node->extension_00;
        if (node == importers_04_.first_04) {
            importers_04_.first_04 = node->next_08;
        }
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        --importers_04_.count_00;
    } while (true);
}

// FUNCTION: SURRENDER 0x1002D090
void srIOManager::removeExporter(Exporter* exporter)
{
    if (exporter == 0) {
        return;
    }
    Registration* node;
    do {
        node = exporters_10_.first_04;
        while (true) {
            if (node == exporters_10_.sentinel_08) {
                return;
            }
            if (node->exporter_04 == exporter) {
                break;
            }
            node = node->next_08;
        }
        delete[] node->extension_00;
        if (node == exporters_10_.first_04) {
            exporters_10_.first_04 = node->next_08;
        }
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        --exporters_10_.count_00;
    } while (true);
}

// FUNCTION: SURRENDER 0x1002D100
void srIOManager::dump()
{
    srPrintf("extension   importer\n");
    srPrintf("-----------------------------------------------------------------\n");
    Registration* node = importers_04_.first_04;
    while (node != importers_04_.sentinel_08) {
        srPrintf("%-8s    '%s'\n", node->extension_00, node->importer_04->getTypeName());
        node = node->next_08;
    }
    srPrintf("-----------------------------------------------------------------\n");
    srPrintf("total %d instances\n", importers_04_.count_00);
    srPrintf("extension   exporter\n");
    srPrintf("-----------------------------------------------------------------\n");
    node = exporters_10_.first_04;
    while (node != exporters_10_.sentinel_08) {
        srPrintf("%-8s    '%s'\n", node->extension_00, node->exporter_04->getTypeName());
        node = node->next_08;
    }
    srPrintf("-----------------------------------------------------------------\n");
    srPrintf("total %d instances\n", exporters_10_.count_00);
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
void srIOManager::ImporterList::insert(Registration* position, char* extension, Importer* importer)
{
    Registration* node = new Registration();
    node->extension_00 = extension;
    node->importer_04 = importer;
    node->next_08 = position;
    node->previous_0c = position->previous_0c;
    if (node->previous_0c != 0) {
        node->previous_0c->next_08 = node;
    } else {
        first_04 = node;
    }
    if (node->next_08 != 0) {
        node->next_08->previous_0c = node;
    }
    ++count_00;
}

// FUNCTION: SURRENDER 0x1002D360
void srIOManager::ExporterList::insert(Registration* position, char* extension, Exporter* exporter)
{
    Registration* node = new Registration();
    node->extension_00 = extension;
    node->exporter_04 = exporter;
    node->next_08 = position;
    node->previous_0c = position->previous_0c;
    if (node->previous_0c != 0) {
        node->previous_0c->next_08 = node;
    } else {
        first_04 = node;
    }
    if (node->next_08 != 0) {
        node->next_08->previous_0c = node;
    }
    ++count_00;
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
    HierarchyExporter* exporter =
        static_cast<HierarchyExporter*>(findExporter(getExtension(path)));
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
        SurfaceImporter* importer =
            static_cast<SurfaceImporter*>(findImporter(getExtension(path)));
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

// FUNCTION: SURRENDER 0x10016470
srHierarchyIOManager::srHierarchyIOManager() {}

// FUNCTION: SURRENDER 0x100165B0
srHierarchyIOManager::~srHierarchyIOManager() {}

// FUNCTION: SURRENDER 0x10016980
srModelIOManager::srModelIOManager() {}

// FUNCTION: SURRENDER 0x10016AC0
srModelIOManager::~srModelIOManager() {}

/* The IO-manager units emit the deleting-destructor wrappers and the
   material class-support registrations they reference. */
// SYNTHETIC: SURRENDER 0x100165C0
// srHierarchyIOManager scalar deleting destructor

// SYNTHETIC: SURRENDER 0x10016640
// srHierarchyIOManager::HierarchyImporter scalar deleting destructor

// SYNTHETIC: SURRENDER 0x100166C0
// srHierarchyIOManager::HierarchyExporter scalar deleting destructor

// TEMPLATE: SURRENDER 0x10016740
// srClassSupport<srMaterialIFace, srClass, true, 0x2200>::srClassSupport

// TEMPLATE: SURRENDER 0x100167D0
// srClassSupport<srMaterial, srMaterialIFace, false, 0x2210>::sGetClassNode

// SYNTHETIC: SURRENDER 0x10016830
// srFStreamOpener scalar deleting destructor

// SYNTHETIC: SURRENDER 0x10016860
// srMaterial scalar deleting destructor
