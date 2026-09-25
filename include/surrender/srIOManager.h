#pragma once

#include "srHeap.h"

/* The provider exports the full member surface including the vftable and the
   implicit lifecycle sweep (copy ctor and assignment emit as memberwise
   copies), so the declaration is dllexport under SURRENDER_BUILD. Consumers
   keep the member-level imports below. */
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

    SR_DLL_IMPORT srIOManager(const srIOManager& manager);
    SR_DLL_IMPORT srIOManager& operator=(const srIOManager& manager);

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
            extension_00 = 0;
        }

        char* extension_00;
        union {
            Importer* importer_04;
            Exporter* exporter_04;
        };
        Registration* next_08;
        Registration* previous_0c;
    };

    static_assert(sizeof(Registration) == 0x10, "srIOManager_Registration_must_be_0x10");

    /* addImporter/addExporter call two identical insert bodies (0x1002D300/
       0x1002D360) as __thiscall on the {count, first, sentinel} triple at
       +0x04/+0x10: two distinct typed list objects, not flat fields. */
    struct ImporterList {
        unsigned long count_00;
        Registration* first_04;
        Registration* sentinel_08;
        void insert(Registration* position, char* extension, Importer* importer);
    };

    struct ExporterList {
        unsigned long count_00;
        Registration* first_04;
        Registration* sentinel_08;
        void insert(Registration* position, char* extension, Exporter* exporter);
    };

    ImporterList importers_04_;
    ExporterList exporters_10_;
};

/* Retail exports the implicit Error assignment (0x1002CC70, a single field
   copy), so the class is dllexport under SURRENDER_BUILD. */
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
    Error(const char* description) { description_00 = description; }
    SR_DLL_IMPORT const char* getDescription();

private:
    const char* description_00;
};

/* Retail exports the implicit lifecycle sweep (0x1002CC80-0x1002CCA0, all
   trivial bodies), so the class is dllexport under SURRENDER_BUILD. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srIOManager::Importer {
public:
    Importer() {}

    virtual const char* getTypeName() const = 0;
    virtual ~Importer() {}

protected:
    SR_DLL_IMPORT void addToImporters(srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToImporters(srIOManager* manager, srIOManager::Importer* importer,
                                      const char* extension);
    SR_DLL_IMPORT void removeFromImporters(srIOManager* manager);
};

/* Retail exports the implicit lifecycle sweep (0x1002CCB0-0x1002CD80, all
   trivial bodies), so the class is dllexport under SURRENDER_BUILD. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    __declspec(novtable) srIOManager::Exporter {
public:
    Exporter() {}

    virtual const char* getTypeName() const = 0;
    virtual ~Exporter() {}

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
