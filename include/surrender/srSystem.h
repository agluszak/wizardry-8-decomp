#pragma once

#if defined(SURRENDER_BUILD)
#define SR_SYSTEM_API __declspec(dllexport)
#elif defined(_MSC_VER) && !defined(WIZ8_CLANG_LINT)
#define SR_SYSTEM_API __declspec(dllimport)
#else
#define SR_SYSTEM_API
#endif

class srStringTable;

class SR_SYSTEM_API srSystem {
public:
    static long chDir(const char* path);
    static char* fullPath(
        char* absolute_path, const char* path, unsigned long size);
    static char* getCwd(char* path, long size);
    static void makePath(
        char* path,
        const char* drive,
        const char* directory,
        const char* filename,
        const char* extension);
    static long scanFiles(srStringTable& files, const char* path);
    static long scanFiles(
        srStringTable& files, const char* directory, const char* pattern);
    static long scanLibraries(
        srStringTable& libraries,
        const char* directory,
        const char* extension);
    static void splitPath(
        const char* path,
        char* drive,
        char* directory,
        char* filename,
        char* extension);
};

static_assert(sizeof(srSystem) == 0x01, "srSystem_must_be_stateless");

#undef SR_SYSTEM_API
