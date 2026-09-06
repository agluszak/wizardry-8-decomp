#pragma once

#include "srHeap.h"

class srIOManager {
public:
    class Error;
    class Importer;
    class Exporter;

    SR_DLL_IMPORT srIOManager(const srIOManager& manager);
    SR_DLL_IMPORT srIOManager& operator=(const srIOManager& manager);

    SR_DLL_IMPORT void dump();
    SR_DLL_IMPORT const char* getExtension(const char* path);

protected:
    SR_DLL_IMPORT srIOManager();
    virtual SR_DLL_IMPORT ~srIOManager();

    SR_DLL_IMPORT void addImporter(
        Importer* importer, const char* extension);
    SR_DLL_IMPORT void addExporter(
        Exporter* exporter, const char* extension);
    SR_DLL_IMPORT Importer* findImporter(const char* extension);
    SR_DLL_IMPORT Exporter* findExporter(const char* extension);
    SR_DLL_IMPORT void removeImporter(Importer* importer);
    SR_DLL_IMPORT void removeExporter(Exporter* exporter);

private:
    struct Registration {
        char* extension_00;
        union {
            Importer* importer_04;
            Exporter* exporter_04;
        };
        Registration* next_08;
        Registration* previous_0c;
    };

    static_assert(sizeof(Registration) == 0x10,
                  "srIOManager_Registration_must_be_0x10");

    unsigned long importer_count_04;
    Registration* first_importer_08;
    Registration* importer_sentinel_0c;
    unsigned long exporter_count_10;
    Registration* first_exporter_14;
    Registration* exporter_sentinel_18;
};

class srIOManager::Error {
public:
    SR_DLL_IMPORT Error(const char* description);
    SR_DLL_IMPORT Error& operator=(const Error& error);
    SR_DLL_IMPORT const char* getDescription();

private:
    const char* description_00;
};

class __declspec(novtable) srIOManager::Importer {
public:
    Importer() {}
    SR_DLL_IMPORT Importer(const Importer& importer);
    SR_DLL_IMPORT Importer& operator=(const Importer& importer);

    virtual const char* getTypeName() const = 0;
    virtual ~Importer() {}

protected:
    SR_DLL_IMPORT void addToImporters(
        srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToImporters(
        srIOManager* manager,
        srIOManager::Importer* importer,
        const char* extension);
    SR_DLL_IMPORT void removeFromImporters(srIOManager* manager);
};

class __declspec(novtable) srIOManager::Exporter {
public:
    Exporter() {}
    SR_DLL_IMPORT Exporter(const Exporter& exporter);
    SR_DLL_IMPORT Exporter& operator=(const Exporter& exporter);

    virtual const char* getTypeName() const = 0;
    virtual ~Exporter() {}

protected:
    SR_DLL_IMPORT void addToExporters(
        srIOManager* manager, const char* extension);
    SR_DLL_IMPORT void addToExporters(
        srIOManager* manager,
        srIOManager::Exporter* exporter,
        const char* extension);
    SR_DLL_IMPORT void removeFromExporters(srIOManager* manager);
};

static_assert(sizeof(srIOManager) == 0x1c, "srIOManager_must_be_0x1c");
static_assert(sizeof(srIOManager::Error) == 0x04,
              "srIOManager_Error_must_be_0x04");
static_assert(sizeof(srIOManager::Importer) == 0x04,
              "srIOManager_Importer_must_be_0x04");
static_assert(sizeof(srIOManager::Exporter) == 0x04,
              "srIOManager_Exporter_must_be_0x04");
