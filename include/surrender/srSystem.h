#pragma once

#if defined(SURRENDER_BUILD)
#define SR_SYSTEM_API __declspec(dllexport)
#elif defined(_MSC_VER) && !defined(WIZ8_CLANG_LINT)
#define SR_SYSTEM_API __declspec(dllimport)
#else
#define SR_SYSTEM_API
#endif

class srStringTable;

class srSystem {
public:
    static SR_SYSTEM_API w8_long chDir(const char* path);
    static SR_SYSTEM_API char* fullPath(char* absolute_path, const char* path, w8_ulong size);
    static SR_SYSTEM_API char* getCwd(char* path, w8_long size);
    static SR_SYSTEM_API void makePath(char* path, const char* drive, const char* directory,
                                       const char* filename, const char* extension);
    static SR_SYSTEM_API w8_long scanFiles(srStringTable& files, const char* path);
    static SR_SYSTEM_API w8_long scanFiles(srStringTable& files, const char* directory,
                                           const char* pattern);
    static SR_SYSTEM_API w8_long scanLibraries(srStringTable& libraries, const char* directory,
                                               const char* extension);
    static SR_SYSTEM_API void splitPath(const char* path, char* drive, char* directory,
                                        char* filename, char* extension);
};

static_assert(sizeof(srSystem) == 0x01, "srSystem_must_be_stateless");

#undef SR_SYSTEM_API
