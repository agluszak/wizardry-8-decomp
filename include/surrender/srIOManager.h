#pragma once

#include "srHeap.h"

/* Provider exports include copy construction, assignment and the vftable. The
   reconstruction uses class-level export and memberwise copying; original
   declaration spelling is unresolved. */
// VTABLE: SURRENDER 0x10076960
// class srIOManager
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srIOManager {
public:
    class Error;
    class Importer;
    class Exporter;

    /* The nested Importer/Exporter registration forwarders call the
       protected add/remove paths; MSVC6 grants nested classes no implicit
       enclosing-class access, so the classes are friends. */
    friend class Importer;
    friend class Exporter;

    /* Provider copy/assignment contain memberwise copies of the two typed
       registration lists. Consumers retain the imported declarations. */

#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srIOManager(const srIOManager& manager);
    SR_DLL_IMPORT srIOManager& operator=(const srIOManager& manager);
#endif

    SR_DLL_IMPORT void dump();
    SR_DLL_IMPORT const char* getExtension(const char* path);

protected:
    SR_DLL_IMPORT srIOManager();
    virtual SR_DLL_IMPORT ~srIOManager();

    SR_DLL_IMPORT void addImporter(Importer* importer, const char* extension);
    SR_DLL_IMPORT void addExporter(Exporter* exporter, const char* extension);
    SR_DLL_IMPORT Importer* findImporter(const char* extension);
    SR_DLL_IMPORT Exporter* findExporter(const char* extension);
    SR_DLL_IMPORT void removeImporter(Importer* importer);
    SR_DLL_IMPORT void removeExporter(Exporter* exporter);

private:
    struct Registration {
        Registration()
        {
            extension = 0;
        }

        char* extension;
        union {
            Importer* importer;
            Exporter* exporter;
        };
        Registration* next;
        Registration* previous;
    };

    static_assert(sizeof(Registration) == 0x10, "srIOManager_Registration_must_be_0x10");

    /* addImporter/addExporter call two identical insert bodies (0x1002D300/
       0x1002D360) as __thiscall on the {count, first, sentinel} triple at
       +0x04/+0x10: two distinct typed list objects, not flat fields. */
    struct ImporterList {
        unsigned long count;
        Registration* first;
        Registration* sentinel;
        void insert(Registration* position, char* extension, Importer* importer);
    };

    struct ExporterList {
        unsigned long count;
        Registration* first;
        Registration* sentinel;
        void insert(Registration* position, char* extension, Exporter* exporter);
    };

    ImporterList importers;
    ExporterList exporters;
};

/* Retail exports Error assignment at 0x1002CC70 as a single-field copy. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srIOManager::Error {
public:
    /* Retail exports the standalone copy while folding the single store into
       every throw site. */
    // FUNCTION: SURRENDER 0x1002CB00
    // ??0Error@srIOManager@@QAE@PBD@Z
    Error(const char* description) { this->description = description; }
    SR_DLL_IMPORT const char* getDescription();

    /* The emitted body is consistent with memberwise assignment. */

private:
    const char* description;
};

/* Retail exports trivial lifecycle bodies at 0x1002CC80-0x1002CCA0, consistent
   with implicit lifecycle. Original declaration spelling is unresolved. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srIOManager::Importer {
public:

    virtual const char* getTypeName() const = 0;

    // FUNCTION: SURRENDER 0x1002CC50
    // ??1Importer@srIOManager@@UAE@XZ
    virtual ~Importer() {}

    /* The emitted bodies are consistent with ordinary memberwise copying. */

protected:
    SR_DLL_IMPORT void addToImporters(srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToImporters(srIOManager* manager, srIOManager::Importer* importer,
                                      const char* extension);
    SR_DLL_IMPORT void removeFromImporters(srIOManager* manager);
};

/* Retail exports trivial lifecycle bodies at 0x1002CCB0-0x1002CD80, consistent
   with implicit lifecycle. Original declaration spelling is unresolved. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srIOManager::Exporter {
public:

    virtual const char* getTypeName() const = 0;

    // FUNCTION: SURRENDER 0x1002CC60
    // ??1Exporter@srIOManager@@UAE@XZ
    virtual ~Exporter() {}

    /* The emitted bodies are consistent with ordinary memberwise copying. */

protected:
    SR_DLL_IMPORT void addToExporters(srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToExporters(srIOManager* manager, srIOManager::Exporter* exporter,
                                      const char* extension);
    SR_DLL_IMPORT void removeFromExporters(srIOManager* manager);
};

static_assert(sizeof(srIOManager) == 0x1c, "srIOManager_must_be_0x1c");
static_assert(sizeof(srIOManager::Error) == 0x04, "srIOManager_Error_must_be_0x04");
static_assert(sizeof(srIOManager::Importer) == 0x04, "srIOManager_Importer_must_be_0x04");
static_assert(sizeof(srIOManager::Exporter) == 0x04, "srIOManager_Exporter_must_be_0x04");
