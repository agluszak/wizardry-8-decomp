#pragma once

// Recovered SurRender3D 1.42.2.9 plug-in boundary.  The interface order is
// proven by every srEXT_* wrapper vtable: deleting destructor first, textual
// description second.

class __declspec(novtable) srPlugin {
public:
    virtual ~srPlugin() {}
    virtual const char* getDescription() const = 0;
};

// Undecorated exports at ordinals 1 and 2. These zero-argument x86 calls
// cannot distinguish authored cdecl from stdcall: both use plain RET.
// Keep both spellings explicit; JPEG uses cdecl, ZIP currently uses stdcall.
typedef unsigned long (__cdecl *srGetLibraryVersionCdeclFn)();
typedef unsigned long (__stdcall *srGetLibraryVersionStdcallFn)();
typedef srPlugin* (__cdecl *srInitPluginCdeclFn)();
typedef srPlugin* (__stdcall *srInitPluginStdcallFn)();
