#include "surrender/srExtension.h"

#include "surrender/srConfig.h"
#include "surrender/srDebug.h"
#include "surrender/srDynamicLibrary.h"

#include <stdio.h>
#include <string.h>

long srExtension::count = 0;
srExtension* srExtension::firstExt = 0;

// FUNCTION: SURRENDER 0x10013840
void srExtension::dumpAll(std::ostream& stream)
{
    srStreamPrintf(stream, "Handle      Extension Name        Description\n");
    srStreamPrintf(stream, "------------------------------------------------------------------\n");
    for (srExtension* extension = firstExt; extension != 0; extension = extension->next_10) {
        srStreamPrintf(stream, "%08x    %-20s  %s\n", extension->module_08,
                       extension->getName(), extension->getDescription());
    }
}

// FUNCTION: SURRENDER 0x100138A0
const char* srExtension::getDescription()
{
    return plugin_00 != 0 ? plugin_00->getDescription() : "Something funny going on";
}

// FUNCTION: SURRENDER 0x100138C0
srExtension* srExtension::getFirst()
{
    return firstExt;
}

// FUNCTION: SURRENDER 0x100138D0
srExtension* srExtension::getNext()
{
    return next_10;
}

// FUNCTION: SURRENDER 0x100138E0
long srExtension::getCount()
{
    return count;
}

// FUNCTION: SURRENDER 0x100138F0
srExtension* srExtension::find(const char* name)
{
    for (srExtension* extension = firstExt; extension != 0; extension = extension->next_10) {
        const char* extension_name = extension->getName();
        if (extension_name != 0 && strcmp(extension_name, name) == 0) {
            return extension;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10013960
void srExtension::releaseAll()
{
    srExtension* extension = firstExt;
    while (extension != 0) {
        srExtension* next = extension->next_10;
        delete extension;
        extension = next;
    }
}

// FUNCTION: SURRENDER 0x10013990
srExtension::srExtension(const char* name)
    : module_08(0), previous_0c(0), next_10(firstExt)
{
    if (firstExt != 0) {
        firstExt->previous_0c = this;
    }
    firstExt = this;
    ++count;
    if (name != 0 && *name != '\0') {
        name_04 = new char[strlen(name) + 1];
        strcpy(name_04, name);
    }
    else {
        name_04 = 0;
    }
    plugin_00 = 0;
}

// FUNCTION: SURRENDER 0x10013A40
srExtension::~srExtension()
{
    if (previous_0c != 0) {
        previous_0c->next_10 = next_10;
    }
    if (next_10 != 0) {
        next_10->previous_0c = previous_0c;
    }
    if (firstExt == this) {
        firstExt = next_10;
    }
    --count;
    delete plugin_00;
    if (module_08 != 0 && !srDynamicLibrary::free(module_08)) {
        char message[512];
        sprintf(message, "srExtension::~srExtension() -- call to srDynamicLibrary::free() failed for extension '%s'!!\n", getName());
        srDebugPrintf(0, message);
    }
    if (name_04 != 0) {
        delete[] name_04;
    }
}

// FUNCTION: SURRENDER 0x10013AE0
const char* srExtension::getName()
{
    return name_04 != 0 ? name_04 : "anonymous extension";
}

// FUNCTION: SURRENDER 0x10013AF0
srExtension* srExtension::load(const char* name, const char* path)
{
    if (name == 0 || *name == '\0') {
        return 0;
    }
    srExtension* extension = find(name);
    if (extension != 0) {
        srDebugPrintf(0, "srExtension::load() -- multiple attempts to load extension '%s'\n", name);
        return extension;
    }
    if (path == 0) {
        path = srConfig.get("DLL_PATH");
    }
    char filename[512];
    if (path != 0) {
        sprintf(filename, "%s/srEXT_%s", path, name);
    }
    else {
        sprintf(filename, "srEXT_%s", name);
    }
    srDynamicLibrary::Compatibility compatibility = srDynamicLibrary::checkCompatibility(filename);
    if (compatibility == srDynamicLibrary::COMPATIBILITY_0 ||
        compatibility == srDynamicLibrary::COMPATIBILITY_1) {
        char message[512];
        sprintf(message, compatibility == srDynamicLibrary::COMPATIBILITY_0
                    ? "srExtension::load() -- cannot load extension file '%s'\n"
                    : "srExtension::load() -- incompatible version number for extension file '%s'\n",
                filename);
        srDebugPrintf(0, message);
        return 0;
    }
    void* module = srDynamicLibrary::load(filename);
    if (module == 0) {
        char message[512];
        sprintf(message, "srExtension::load() -- srDynamicLibrary::load() failed for file '%s'\n", filename);
        srDebugPrintf(0, message);
        return 0;
    }
    srInitPluginCdeclFn init_plugin = reinterpret_cast<srInitPluginCdeclFn>(
        srDynamicLibrary::getFunction(module, "srInitPlugin"));
    if (init_plugin == 0) {
        srDebugPrintf(0, "srExtension::load() -- getProcAddress('srInitPlugin') failed for file '%s' -- not a valid SurRender plugin DLL!!\n", filename);
        srDynamicLibrary::free(module);
        return 0;
    }
    srPlugin* plugin = init_plugin();
    if (plugin == 0) {
        srDebugPrintf(0, "srExtension::load() -  extension initialization failed for file '%s'\n", filename);
        srDynamicLibrary::free(module);
        return 0;
    }
    extension = new srExtension(name);
    extension->plugin_00 = plugin;
    extension->module_08 = static_cast<HMODULE>(module);
    srDebugPrintf(5, "srExtension::load() -- SurRender extension '%s' initialized.\n", extension->getName());
    return extension;
}
