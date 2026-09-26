#include "surrender/srCore.h"
#include "surrender/srDebug.h"
#include "surrender/srDynamicLibrary.h"
#include "surrender/srPlugin.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "surrender/srString.h"

/* TU-local srInlineString expansions: the constructors and destructor expand
   through the sibling units' callable emissions (init at 0x10004150,
   operator= at 0x100040D0, reset at 0x10012C80, operator+ at 0x10012CB0)
   while this unit owns find's callable emission at 0x100467E0. */

inline srInlineString::srInlineString(const char* source)
{
    init();
    if (source != 0) {
        operator=(source);
    }
}

inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source.data_);
    }
}

inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

// FUNCTION: SURRENDER 0x100467E0
long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}

namespace {

/* Retail returns an srInlineString by value: the name is canonicalized by
   finding the last '.' after the last '/' or '\\' separator; a trailing '.'
   gets the extension appended, an existing extension is kept, and a missing
   one becomes ".<extension>". Callers pass "dll". */
// FUNCTION: SURRENDER 0x10045F80
srInlineString libraryName(const char* name, const char* extension)
{
    srInlineString filename(name);
    srInlineString needle;
    needle = ".";
    long pos = filename.find(needle, 0);
    long dot = pos;
    while (pos != -1 && pos < (long)filename.size() - 1) {
        dot = pos;
        pos = filename.find(needle, pos + 1);
    }
    needle = "/";
    pos = filename.find(needle, 0);
    long slash = pos;
    while (pos != -1 && pos < (long)filename.size() - 1) {
        slash = pos;
        pos = filename.find(needle, pos + 1);
    }
    needle = "\\";
    pos = filename.find(needle, 0);
    long backslash = pos;
    while (pos != -1 && pos < (long)filename.size() - 1) {
        backslash = pos;
        pos = filename.find(needle, pos + 1);
    }
    long separator = slash;
    if (slash < backslash) {
        separator = backslash;
    }
    if (separator < dot) {
        if (dot == (long)filename.size() - 2) {
            srInlineString suffix(extension);
            return filename + suffix;
        }
        return filename;
    }
    srInlineString suffix(extension);
    needle = ".";
    return filename + needle + suffix;
}

inline char* versionString(
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
    *translation = (*translation >> 16) | ((*translation & 0xffff) << 16);

    char query[256];
    wsprintfA(query, "\\StringFileInfo\\%08lx\\%s",
              *translation, key);

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

// FUNCTION: SURRENDER 0x10045990
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        _srLibraryInit();
        break;
    case DLL_PROCESS_DETACH:
        _srLibraryExit();
        break;
    }
    return TRUE;
}

// FUNCTION: SURRENDER 0x10045E70
void* srDynamicLibrary::load(const char* name)
{
    if (name == 0) {
        return 0;
    }

    const srInlineString filename(libraryName(name, "dll"));
    if (testDependencies(filename.data()) == 0) {
        return 0;
    }

    HMODULE library = LoadLibraryA(filename.data());
    if (library == 0) {
        char message[512];
        sprintf(message, "srDynamicLibrary::load () -- failed to load DLL file '%s'\n",
                filename.data());
        srDebugPrintf(0, message);
    }
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
    const srInlineString filename(libraryName(name, "dll"));
    if (filename.data() == 0) {
        return 0;
    }

    int available = 1;
    unsigned char* version_info = 0;
    char* dependencies = versionString(
        filename.data(), "srDependencies", version_info);
    if (dependencies != 0) {
        char* dependency_list = new char[strlen(dependencies) + 1];
        strcpy(dependency_list, dependencies);

        char* end = dependency_list + strlen(dependency_list);
        char* token = dependency_list;
        do {
            char* split = strpbrk(token, ", ");
            char* next = end;
            if (split != 0) {
                *split = '\0';
                next = split;
            }
            HMODULE library = LoadLibraryExA(
                token, 0, LOAD_LIBRARY_AS_DATAFILE);
            if (library == 0) {
                char message[512];
                sprintf(message,
                        "srDynamicLibrary::testDependencies () -- '%s' failed because dependent file '%s' could not be loaded\n",
                        name, token);
                srDebugPrintf(0, message);
            } else {
                FreeLibrary(library);
            }
            available = library != 0;
            token = next + 1;
        } while (available && token < end);

        delete[] dependency_list;
    }
    delete[] version_info;
    return available;
}

// FUNCTION: SURRENDER 0x10046500
unsigned long srDynamicLibrary::getVersion(const char* name)
{
    void* library = load(name);
    if (library != 0) {
        srGetLibraryVersionCdeclFn get_library_version =
            reinterpret_cast<srGetLibraryVersionCdeclFn>(
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
