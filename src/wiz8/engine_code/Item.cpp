#include "wiz8/engine_code/Item.h"

#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* VC6 emits the scalar-deleting wrapper at 0x0049F420 from this ordinary
   virtual destructor. */
// SYNTHETIC: WIZ8 0x0049F420
// W8Item::`scalar deleting destructor'
W8Item::~W8Item()
{
    delete m_pRep;
}

/* Detach the item's mesh from the world graph. The world parameter is asserted
   even though the body needs no field from it. */
// FUNCTION: WIZ8 0x0049fa30
void W8Item::DetachMesh0049FA30(W8World* world)
{
    srNode* mesh;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x24a, 0);
    }
    mesh = m_pRep->m_psrMesh;
    if (mesh == 0) {
        srAssertFail("psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x24f, 0);
    }
    mesh->setFlag(srNode::FLAG_POSITIONAL_0);
    mesh->setParent(0, 0);
}

/* Copy the representation's float transform onto its SurRender node. The
   location is widened because the exported node setter takes doubles. */
// FUNCTION: WIZ8 0x0049faa0
void W8Item::ApplyRepTransform0049FAA0()
{
    srVector3T<float> location;
    srVector3T<double> widened;
    srMatrix3T<float> rotation;
    srNode* mesh;

    if (m_pRep->m_psrMesh == 0) {
        srAssertFail("m_pRep->m_psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x266, 0);
    }
    mesh = m_pRep->m_psrMesh;
    m_pRep->GetLocation004B8890(&location);
    widened.x = location.x;
    widened.y = location.y;
    widened.z = location.z;
    mesh->setLocation(widened);
    m_pRep->GetRotation004B88F0(&rotation);
    mesh->setRotation(rotation);
}

// FUNCTION: WIZ8 0x0049fb20
srNode* W8Item::GetMesh()
{
    return m_pRep->m_psrMesh;
}

/* Raise or clear the selected representation flags and return the resulting
   flag word. */
// FUNCTION: WIZ8 0x0049F310
unsigned int W8ItemRep::SetFlags(unsigned int mask, unsigned char enabled)
{
    if (enabled != 0) {
        flags |= mask;
    }
    else {
        flags &= ~mask;
    }
    return flags;
}

/* Forward a new item location to the representation owned at +0x14. */
// FUNCTION: WIZ8 0x0049F720
void W8Item::Function49F720(const W8Position* location)
{
    m_pRep->SetLocation004B8850(location);
}
