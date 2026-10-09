#include "surrender/srSystem.h"

#include "surrender/srStringTable.h"

#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// FUNCTION: SURRENDER 0x100457B0
w8_long srSystem::scanFiles(srStringTable& files, const char* path)
{
    if (path == 0 || *path == '\0') {
        return 0;
    }

    const w8_long last = strlen(path) - 1;
    w8_long slash;
    for (slash = last; slash >= 0; --slash) {
        if (path[slash] == '/') {
            break;
        }
    }
    if (slash == last) {
        return 0;
    }
    if (slash > 0) {
        /* The directory keeps its trailing separator. */
        char directory[MAX_PATH];
        memcpy(directory, path, slash + 1);
        directory[slash + 1] = '\0';
        return scanFiles(files, directory, path + slash + 1);
    }
    if (*path == '/') {
        return scanFiles(files, "/", path + 1);
    }
    return scanFiles(files, 0, path);
}

// FUNCTION: SURRENDER 0x10045B70
char* srSystem::getCwd(char* path, w8_long size)
{
    return _getcwd(path, size);
}

// FUNCTION: SURRENDER 0x10045B90
w8_long srSystem::chDir(const char* path)
{
    return _chdir(path);
}

// FUNCTION: SURRENDER 0x10045BA0
w8_long srSystem::scanLibraries(srStringTable& libraries, const char* directory,
                                const char* extension)
{
    char pattern[MAX_PATH + 4];
    strcpy(pattern, extension);
    strcat(pattern, ".dll");

    srStringTable files;
    scanFiles(files, directory, pattern);
    for (w8_long index = 0; index < files.getCount(); ++index) {
        char* path = files.getString(index);
        for (w8_long position = strlen(path) - 1; position > 1; --position) {
            if (path[position] == '.') {
                path[position] = '\0';
                break;
            }
        }
        libraries.addString(path);
    }
    return files.getCount();
}

// FUNCTION: SURRENDER 0x10045CD0
w8_long srSystem::scanFiles(srStringTable& files, const char* directory, const char* pattern)
{
    if (pattern == 0) {
        return 0;
    }

    char previous_directory[MAX_PATH];
    if (directory != 0) {
        getCwd(previous_directory, MAX_PATH);
        if (chDir(directory) != 0) {
            return 0;
        }
    }

    w8_long count = 0;
    WIN32_FIND_DATAA file;
    HANDLE search = FindFirstFileA(pattern, &file);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                char absolute_path[MAX_PATH * 2];
                absolute_path[0] = '\0';
                fullPath(absolute_path, file.cFileName, MAX_PATH);
                files.addString(absolute_path);
                ++count;
            }
        } while (FindNextFileA(search, &file));
        FindClose(search);
    }

    if (directory != 0) {
        chDir(previous_directory);
    }
    return count;
}

// FUNCTION: SURRENDER 0x10045DE0
void srSystem::makePath(char* path, const char* drive, const char* directory, const char* filename,
                        const char* extension)
{
    _makepath(path, drive, directory, filename, extension);
}

// FUNCTION: SURRENDER 0x10045E10
char* srSystem::fullPath(char* absolute_path, const char* path, w8_ulong size)
{
    if (absolute_path == 0) {
        absolute_path = new char[MAX_PATH];
    }
    return _fullpath(absolute_path, path, size);
}

// FUNCTION: SURRENDER 0x10045E40
void srSystem::splitPath(const char* path, char* drive, char* directory, char* filename,
                         char* extension)
{
    _splitpath(path, drive, directory, filename, extension);
}
