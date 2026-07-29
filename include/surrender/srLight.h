#pragma once

#include "srNode.h"
#include "srVertexProcessor.h"

/*
 * The imported vtable pair and MonsterLight's copy constructor establish this
 * exact multiple-inheritance prefix.  srNode contributes 0x138 bytes and the
 * srVertexProcessor secondary subobject contributes 0xf0 bytes.
 */
class srLight : public srIlluminator, public srVertexProcessor {
public:
    enum e_preset {
        PRESET_POSITIONAL_0 = 0
    };

    SR_DLL_IMPORT srLight(srNode* parent, e_preset preset);
    SR_DLL_IMPORT srLight& operator=(const srLight& other);

    virtual const char* getClassName() const override;
    virtual unsigned long getClassID() const override;
    virtual srRegistry::ClassNode* getClassNode() const override;
    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;

protected:
    virtual SR_DLL_IMPORT ~srLight() override;

public:
    virtual srNode* vslot7() override;
    virtual SR_DLL_IMPORT void traverse(srNode::TraverseInfo& info) override;
    virtual SR_DLL_IMPORT void process(
        const srNode::ProcessInfo& info,
        srNode::e_processType type) override;
    virtual SR_DLL_IMPORT int isActive(srVertexPipe& pipe) override;
    virtual SR_DLL_IMPORT void process(srVertexPipe& pipe) override;
};

static_assert(sizeof(srLight) == 0x228, "srLight_must_be_0x228");
