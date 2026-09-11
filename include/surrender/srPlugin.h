#pragma once

// Recovered SurRender3D 1.42.2.9 plug-in boundary.  The interface order is
// proven by every srEXT_* wrapper vtable: deleting destructor first, textual
// description second.

class __declspec(novtable) srPlugin {
public:
    virtual ~srPlugin() {}
    virtual const char* getDescription() const = 0;
};

// Undecorated exports at ordinals 1 and 2. Retail exports these names
// undecorated in every srEXT_* DLL
// (evidence/snapshots/surrender-abi/exports.csv), which an extern "C"
// __stdcall definition could not produce (it decorates as _name@0), and the
// host below calls them through the cdecl pointers. Both extension
// recoveries and the host therefore use __cdecl. These zero-argument x86
// calls use plain RET under either convention, so the convention is
// invisible in the emitted bodies and only the export/host agreement pins it.
typedef unsigned long (__cdecl *srGetLibraryVersionCdeclFn)();
typedef srPlugin* (__cdecl *srInitPluginCdeclFn)();
