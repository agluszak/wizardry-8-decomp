#pragma once

// Recovered SurRender3D 1.42.2.9 plug-in boundary.  The interface order is
// proven by every srEXT_* wrapper vtable: deleting destructor first, textual
// description second.

class srPlugin {
public:
    virtual ~srPlugin() {}
    virtual const char* getDescription() const = 0;
};
