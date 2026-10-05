#pragma once

#include <iosfwd>

#include "srHeap.h"

// VTABLE: SURRENDER 0x100755C8
// class srFileManager
class SR_DLL_EXPORT srFileManager {
public:
    class SR_DLL_EXPORT Path {
    public:
#if !defined(SURRENDER_BUILD)
        SR_DLL_IMPORT Path& operator=(const Path& other);
#endif

        SR_DLL_IMPORT const char* getName() const;
        SR_DLL_IMPORT Path* getNext() const;

    protected:
        SR_DLL_IMPORT Path(const char* name);
        SR_DLL_IMPORT ~Path();

    private:
        friend class srFileManager;

        char* name0;
        Path* next;
        Path* previous;
    };

    SR_DLL_IMPORT srFileManager();
    /* Provider copy/assignment perform a shallow first_path pointer copy.
       Consumers retain the imported standalone declarations. */

#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srFileManager(const srFileManager& other);
    SR_DLL_IMPORT srFileManager& operator=(const srFileManager& other);
#endif
    virtual SR_DLL_IMPORT ~srFileManager();

    SR_DLL_IMPORT void addPath(const char* path);
    SR_DLL_IMPORT void dump(std::ostream& stream);
    SR_DLL_IMPORT Path* getFirstPath() const;
    SR_DLL_IMPORT void removePath(const char* path);
    SR_DLL_IMPORT void setPath(const char* path);

    virtual SR_DLL_IMPORT void* allocate(const char* path);
    virtual SR_DLL_IMPORT void free(void* allocation);
    virtual SR_DLL_IMPORT void load(const char* path, void* destination, unsigned long size);
    virtual SR_DLL_IMPORT void save(const char* path, void* source, unsigned long size);
    virtual SR_DLL_IMPORT long getSize(const char* path);

private:
    Path* first_path;
};

static_assert(sizeof(srFileManager::Path) == 0x0c, "srFileManager_Path_must_be_0x0c");
static_assert(sizeof(srFileManager) == 0x08, "srFileManager_must_be_0x08");
