#pragma once

#include <ostream>

#include "srHeap.h"

class srRuntimeClass {
public:
    enum e_verify {
        VERIFY_DEFAULT = 0
    };
};

class srRegistry {
public:
    class ClassNode;

    SR_DLL_IMPORT ClassNode* getClassNode(unsigned long class_id);
    SR_DLL_IMPORT ClassNode* registerClass(
        const char* class_name,
        ClassNode* parent,
        unsigned long class_id,
        int concrete);
};

class srClass {
public:
    static SR_DLL_IMPORT srRegistry::ClassNode* sGetClassNode();

    virtual const char* getClassName() const = 0;
    virtual unsigned long getClassID() const = 0;
    virtual srRegistry::ClassNode* getClassNode() const = 0;
    virtual void dump(std::ostream& stream) = 0;
    virtual SR_DLL_IMPORT void verify(srRuntimeClass::e_verify mode);
    virtual ~srClass() {}
    virtual srClass* vInstance() = 0;

    SR_DLL_IMPORT int release() const;
};
