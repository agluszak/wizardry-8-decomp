#pragma once

#include <iostream>

#include "srHeap.h"

class srFileManager {
public:
    class Path {
    public:
        SR_DLL_IMPORT Path& operator=(const Path& other);

        SR_DLL_IMPORT const char* getName() const;
        SR_DLL_IMPORT Path* getNext() const;

    protected:
        SR_DLL_IMPORT Path(const char* name);
        SR_DLL_IMPORT ~Path();

    private:
        friend class srFileManager;

        char* name_00;
        Path* next_04;
        Path* previous_08;
    };

    SR_DLL_IMPORT srFileManager();
    SR_DLL_IMPORT srFileManager(const srFileManager& other);
    virtual SR_DLL_IMPORT ~srFileManager();
    SR_DLL_IMPORT srFileManager& operator=(const srFileManager& other);

    SR_DLL_IMPORT void addPath(const char* path);
    SR_DLL_IMPORT void dump(std::ostream& stream);
    SR_DLL_IMPORT Path* getFirstPath() const;
    SR_DLL_IMPORT void removePath(const char* path);
    SR_DLL_IMPORT void setPath(const char* path);

    virtual SR_DLL_IMPORT void* allocate(const char* path);
    virtual SR_DLL_IMPORT void free(void* allocation);
    virtual SR_DLL_IMPORT void load(
        const char* path, void* destination, unsigned long size);
    virtual SR_DLL_IMPORT void save(
        const char* path, void* source, unsigned long size);
    virtual SR_DLL_IMPORT long getSize(const char* path);

private:
    Path* first_path_04;
};

static_assert(sizeof(srFileManager::Path) == 0x0c,
              "srFileManager_Path_must_be_0x0c");
static_assert(sizeof(srFileManager) == 0x08,
              "srFileManager_must_be_0x08");
