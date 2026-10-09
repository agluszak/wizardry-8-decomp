#pragma once

// SurRender3D 1.42.2.9 plug-in boundary.

class __declspec(novtable) srPlugin {
public:
    virtual ~srPlugin() {}
    virtual const char* getDescription() const = 0;
};

// Undecorated exports at ordinals 1 and 2.
typedef w8_ulong(__cdecl* srGetLibraryVersionCdeclFn)();
typedef srPlugin*(__cdecl* srInitPluginCdeclFn)();
