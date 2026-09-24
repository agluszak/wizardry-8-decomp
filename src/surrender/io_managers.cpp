#include <ctype.h>
#include <string.h>

#include "surrender/srBinFStream.h"
#include "surrender/srCore.h"
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
    for (Registration* node = first_exporter_14; node != exporter_sentinel_18;
         node = node->next_08) {
        if (strcmp(node->extension_00, upper) == 0) {
            result = node->exporter_04;
            break;
        }
    }
    delete[] upper;
    return result;
}

// FUNCTION: SURRENDER 0x1002C890
srIOManager::srIOManager()
{
    Registration* importers = new Registration();
    first_importer_08 = importers;
    importer_sentinel_0c = importers;
    importers->next_08 = 0;
    importers->previous_0c = 0;
    importer_count_04 = 0;
    Registration* exporters = new Registration();
    first_exporter_14 = exporters;
    exporter_sentinel_18 = exporters;
    exporters->next_08 = 0;
    exporters->previous_0c = 0;
    exporter_count_10 = 0;
}

// FUNCTION: SURRENDER 0x1002C920
srIOManager::~srIOManager()
{
    Registration* node = first_exporter_14;
    while (node != exporter_sentinel_18) {
        first_exporter_14 = node->next_08;
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        node = first_exporter_14;
        --exporter_count_10;
    }
    delete first_exporter_14;
    node = first_importer_08;
    while (node != importer_sentinel_0c) {
        first_importer_08 = node->next_08;
        if (node->previous_0c != 0) {
            node->previous_0c->next_08 = node->next_08;
        }
        if (node->next_08 != 0) {
            node->next_08->previous_0c = node->previous_0c;
        }
        delete node;
        node = first_importer_08;
        --importer_count_04;
    }
    delete first_importer_08;
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
    for (Registration* node = first_importer_08; node != importer_sentinel_0c;
         node = node->next_08) {
        if (strcmp(node->extension_00, upper) == 0) {
            result = node->importer_04;
            break;
        }
    }
    delete[] upper;
    return result;
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
