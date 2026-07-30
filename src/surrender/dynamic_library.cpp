#include "surrender/srDynamicLibrary.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

namespace {

char* libraryName(const char* name)
{
    if (name == 0) {
        return 0;
    }

    const size_t length = strlen(name);
    const char* dot = strrchr(name, '.');
    const char* slash = strrchr(name, '/');
    const char* backslash = strrchr(name, '\\');
    const char* separator = slash;
    if (separator == 0 ||
        (backslash != 0 && backslash > separator)) {
        separator = backslash;
    }

    size_t suffix_length = 0;
    if (dot == 0 || (separator != 0 && dot < separator)) {
        suffix_length = 4;
    } else if (dot == name + length - 1) {
        suffix_length = 3;
    }

    char* result = new char[length + suffix_length + 1];
    strcpy(result, name);
    if (suffix_length == 4) {
        strcat(result, ".dll");
    } else if (suffix_length == 3) {
        strcat(result, "dll");
    }
    return result;
}

char* versionString(
    const char* name, const char* key, unsigned char*& version_info)
{
    char* mutable_name = const_cast<char*>(name);
    DWORD ignored;
    const DWORD size = GetFileVersionInfoSizeA(mutable_name, &ignored);
    if (size == 0) {
        return 0;
    }

    version_info = new unsigned char[size + 1];
    if (GetFileVersionInfoA(mutable_name, 0, size, version_info) == 0) {
        delete[] version_info;
        version_info = 0;
        return 0;
    }

    unsigned long* translation;
    unsigned int translation_size;
    VerQueryValueA(
        version_info,
        "\\VarFileInfo\\Translation",
        reinterpret_cast<void**>(&translation),
        &translation_size);
    const unsigned long language_and_codepage =
        (*translation >> 16) | ((*translation & 0xffff) << 16);

    char query[256];
    wsprintfA(query, "\\StringFileInfo\\%08lx\\%s",
              language_and_codepage, key);

    char* value;
    unsigned int value_size;
    if (VerQueryValueA(
            version_info,
            query,
            reinterpret_cast<void**>(&value),
            &value_size) == 0) {
        return 0;
    }
    return value;
}

} // namespace

// FUNCTION: SURRENDER 0x10045780
srDynamicLibrary::Compatibility
srDynamicLibrary::checkCompatibility(const char* name)
{
    if (name == 0) {
        return COMPATIBILITY_0;
    }

    const unsigned long version = getVersion(name);
    if (version == 0) {
        return COMPATIBILITY_0;
    }
    return (version & 0xffffff00) == 0x012a0200
               ? COMPATIBILITY_2
               : COMPATIBILITY_1;
}

// FUNCTION: SURRENDER 0x10045E70
void* srDynamicLibrary::load(const char* name)
{
    if (name == 0) {
        return 0;
    }

    char* filename = libraryName(name);
    if (!testDependencies(filename)) {
        delete[] filename;
        return 0;
    }

    HMODULE library = LoadLibraryA(filename);
    delete[] filename;
    return library;
}

// FUNCTION: SURRENDER 0x10046250
int srDynamicLibrary::free(void* library)
{
    if (library == 0) {
        return 0;
    }
    return FreeLibrary(static_cast<HMODULE>(library)) != 0;
}

// FUNCTION: SURRENDER 0x10046270
void* srDynamicLibrary::getFunction(void* library, const char* function_name)
{
    if (library == 0 || function_name == 0) {
        return 0;
    }
    return GetProcAddress(static_cast<HMODULE>(library), function_name);
}

// FUNCTION: SURRENDER 0x10046290
int srDynamicLibrary::testDependencies(const char* name)
{
    char* filename = libraryName(name);
    if (filename == 0) {
        return 0;
    }

    unsigned char* version_info = 0;
    char* dependencies = versionString(
        filename, "srDependencies", version_info);
    if (dependencies == 0) {
        delete[] version_info;
        delete[] filename;
        return 1;
    }

    const size_t length = strlen(dependencies);
    char* dependency_list = new char[length + 1];
    strcpy(dependency_list, dependencies);

    int available = 1;
    char* dependency = strtok(dependency_list, ", ");
    while (available && dependency != 0) {
        HMODULE library = LoadLibraryExA(
            dependency, 0, LOAD_LIBRARY_AS_DATAFILE);
        available = library != 0;
        if (library != 0) {
            FreeLibrary(library);
        }
        dependency = strtok(0, ", ");
    }

    delete[] dependency_list;
    delete[] version_info;
    delete[] filename;
    return available;
}

// FUNCTION: SURRENDER 0x10046500
unsigned long srDynamicLibrary::getVersion(const char* name)
{
    typedef unsigned long(__cdecl* GetLibraryVersion)();

    void* library = load(name);
    if (library != 0) {
        GetLibraryVersion get_library_version =
            reinterpret_cast<GetLibraryVersion>(
                getFunction(library, "srGetLibraryVersion"));
        if (get_library_version != 0) {
            const unsigned long version = get_library_version();
            free(library);
            return version;
        }
        free(library);
    }

    if (name == 0) {
        return 0;
    }

    unsigned char* version_info = 0;
    char* version = versionString(name, "FileVersion", version_info);
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;
    if (version != 0) {
        sscanf(version, "%d, %d, %d, %d",
               &major, &minor, &patch, &build);
    }
    delete[] version_info;
    return ((major << 8 | minor) << 8 | patch) << 8 | build;
}
