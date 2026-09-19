#include "wiz8/engine_code/Item.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "DEBUG.H"
#include "FileMan.h"
#include "Random.h"
#include "sgp.h"
#include "surrender/srCamera.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/ReadMesh.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "timer.h"

#define ITEM_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp"

// VTABLE: WIZ8 0x005ECD78 W8Item
// class W8Item

/* VC6 emits the scalar-deleting wrapper at 0x0049F420 from this ordinary
   virtual destructor. */
// SYNTHETIC: WIZ8 0x0049F420
// W8Item::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0049F440
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
    mesh = static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
    if (mesh == 0) {
        srAssertFail("psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x24f, 0);
    }
    mesh->setFlag(srNode::FLAG_DISABLE);
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

    if (static_cast<W8ItemRep*>(m_pRep)->m_psrMesh == 0) {
        srAssertFail("m_pRep->m_psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x266,
                     0);
    }
    mesh = static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
    m_pRep->GetLocation004B8890(&location);
    widened.SetFromFloat(&location);
    mesh->setLocation(widened);
    m_pRep->GetRotation004B88F0(&rotation);
    mesh->setRotation(rotation);
}

// FUNCTION: WIZ8 0x0049F900
void W8Item::AttachMesh0049F900(W8World* world)
{
    srNode* mesh;
    srNode* child;
    srVector3T<float> location;
    srVector3T<double> widened;
    srMatrix3T<float> rotation;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x211, 0);
    }
    if ((static_cast<W8ItemRep*>(m_pRep)->flags & 0x40) == 0) {
        return;
    }
    mesh = static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
    if (mesh == 0) {
        srAssertFail("psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\Item.cpp", 0x219, 0);
    }
    if (mesh->firstChild() == 0) {
        mesh->clearFlag(srNode::FLAG_DISABLE);
    }
    mesh->setParent(world->dynamic_scene, 0);
    m_pRep->GetLocation004B8890(&location);
    m_pRep->GetRotation004B88F0(&rotation);
    child = mesh->firstChild();
    if (child == 0) {
        widened.SetFromFloat(&location);
        mesh->setLocation(widened);
        mesh->setRotation(rotation);
        return;
    }
    do {
        widened.SetFromFloat(&location);
        child->setLocation(widened);
        child->setRotation(rotation);
        child = child->nextSibling();
    } while (child != 0);
}

// FUNCTION: WIZ8 0x0049fb20
srNode* W8Item::GetMesh()
{
    return static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
}

/* Raise or clear the selected representation flags and return the resulting
   flag word. */
// FUNCTION: WIZ8 0x0049F310
unsigned int W8ItemRep::SetFlags(unsigned int mask, bool enabled)
{
    if (enabled != 0) {
        flags |= mask;
    } else {
        flags &= ~mask;
    }
    return flags;
}

/* Forward a new item location to the representation owned at +0x14. */
// FUNCTION: WIZ8 0x0049F720
void W8Item::SetLocation0049F720(const srVector3T<float>* location)
{
    m_pRep->SetLocation004B8850(location);
}

/* Whether the item's radar-blip timer is still ticking. */
// FUNCTION: WIZ8 0x004A0050
unsigned char W8Item::IsRadarBlipLit()
{
    return ClockIsTicking(value_01c) != 0;
}

// VTABLE: WIZ8 0x005ECD70 W8ItemRep
// class W8ItemRep

// SYNTHETIC: WIZ8 0x0049F100
// W8ItemRep::`scalar deleting destructor'

/* Inlined into both item construction paths. Retail leaves the mesh pointer
   and bounds untouched until ReadFromFile; do not initialize them here. */
W8ItemRep::W8ItemRep()
{
    flags = 0;
    pulse_level = static_cast<float>(Random(20) * 0.05);
    flags |= 0x20;
}

// FUNCTION: WIZ8 0x0049F120
W8ItemRep::~W8ItemRep()
{
    if (m_psrMesh != 0) {
        m_psrMesh->release();
    }
}

// FUNCTION: WIZ8 0x0049F170
bool W8ItemRep::ReadFromFile(W8ReadLevelInfo* info, W8Item* item, bool anonymous_mesh)
{
    const char* bitmap_folder = info->bitmap_folder;
    srModelInstance* mesh = 0;
    if (info == 0 || info->hFile == 0 || item == 0) {
        srAssertFail("pInfo && pInfo->hFile && pItem", ITEM_CPP, 0x7e, 0);
    }

    info->bitmap_folder = "Data\\Items3D\\Bitmaps";
    bool success = ReadSingleLevelMesh00485B20(info, &mesh, 0, 0,
                                               anonymous_mesh ? 0 : info->mesh_filename, 1) != 0;
    if (!success || mesh == 0) {
        srAssertFail("fSuccess && psrMesh", ITEM_CPP, 0x8c, 0);
    }
    mesh->setName("ItemRep::ReadFromFile");
    static_cast<stModelInstance*>(mesh)->state_178 |= 8;
    SetChainValue15C(
        reinterpret_cast< // reinterpret-ok: existing unresolved node-tail API in 3d.cpp
            char*>(mesh),
        4);
    info->bitmap_folder = bitmap_folder;
    flags |= 0x40;
    m_psrMesh = mesh;
    RefreshBounds();
    return success;
}

// FUNCTION: WIZ8 0x0049F2B0
void W8ItemRep::RefreshBounds()
{
    m_psrMesh->model()->getBoundingBox(bounds_minimum, bounds_maximum);
    bounds_radius = static_cast<float>((bounds_maximum - bounds_minimum).Length() * 0.5);
}

// FUNCTION: WIZ8 0x0049F350
W8Item::W8Item()
{
    trigger_018 = 0;
    kind_004 = 2;
    m_pRep = new W8ItemRep;
    id_008 = IncrementValue60DFAC();
    value_01c = SetCountdownClock(0);
}

// FUNCTION: WIZ8 0x0049F4A0
bool LoadItemFromFile(const W8ReadLevelInfo* context, const char* name, W8Item** output,
                      bool anonymous_mesh)
{
    W8Item* item = 0;
    if (name == 0 || output == 0) {
        srAssertFail("pacName && ppItem", ITEM_CPP, 0x11a, 0);
    }
    char filename[52];
    sprintf(filename, "%s\\%s.itm", "Data\\Items3D", name);
    HWFILE file = FileOpen(filename, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (file == 0) {
        ShutdownWithErrorBox(
            reinterpret_cast< // reinterpret-ok: SGP String returns unsigned text bytes
                const char*>(String("Couldn't load %s", filename)));
        return false;
    }

    W8ReadLevelInfo info;
    info.world = context->world;
    info.hFile = file;
    info.bitmap_folder = context->bitmap_folder;
    char* mesh_name = new char[1024];
    strcpy(mesh_name, name);
    info.mesh_filename = mesh_name;
    unsigned char version;
    bool success = false;
    if (FileRead(file, &version, 1, 0) && version == 2 &&
        ReadItemFromFile(&info, &item, anonymous_mesh)) {
        *output = item;
        success = true;
    }
    FileClose(file);
    delete[] mesh_name;
    return success;
}

/* On failure retail leaves the output alone and does not delete the allocated
   item. This is not an owning smart-pointer factory. */
// FUNCTION: WIZ8 0x0049F5D0
bool ReadItemFromFile(W8ReadLevelInfo* info, W8Item** output, bool anonymous_mesh)
{
    if (info == 0) {
        srAssertFail("pInfo", ITEM_CPP, 0x15d, 0);
    }
    W8Item* item = new W8Item;
    if (item == 0) {
        srAssertFail("pItem", ITEM_CPP, 0x161, 0);
    }
    bool success = static_cast<W8ItemRep*>(item->m_pRep)->ReadFromFile(info, item, anonymous_mesh);
    if (success) {
        *output = item;
    }
    return success;
}

// FUNCTION: WIZ8 0x0049F730
void W8Item::UpdateAnimation0049F730()
{
    if (m_pRep == 0) {
        srAssertFail("m_pRep", ITEM_CPP, 0x1ca, 0);
    }
    W8ItemRep* rep = static_cast<W8ItemRep*>(m_pRep);
    if ((rep->flags & 0x40) == 0) {
        return;
    }
    if ((rep->flags & 2) != 0 && g_monster_combat_timer_enabled_006f0531 == 0) {
        srMatrix3T<float> rotation;
        m_pRep->GetRotation004B88F0(&rotation);
        double cosine = cos(-0.1963495375);
        double sine = sin(-0.1963495375);
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        first.Set(cosine, 0.0, sine);
        second.Set(0.0, 1.0, 0.0);
        third.Set(-sine, 0.0, cosine);
        srMatrix3T<float> step;
        step.SetRows(first, second, third);
        rotation.MultiplyBy(step);
        srNode* mesh = rep->m_psrMesh;
        m_pRep->SetRotation004B88D0(&rotation);
        if (mesh->firstChild() == 0) {
            mesh->setRotation(rotation);
        } else {
            for (srNode* child = mesh->firstChild(); child != 0; child = child->nextSibling()) {
                child->setRotation(rotation);
            }
        }
    }

    rep = static_cast<W8ItemRep*>(m_pRep);
    if ((rep->flags & 1) != 0) {
        if ((rep->flags & 0x20) != 0) {
            rep->pulse_level += 0.05f;
            if (rep->pulse_level >= 1.0f) {
                rep->pulse_level = 1.0f;
                rep->flags &= ~0x20u;
            }
        } else {
            rep->pulse_level -= 0.05f;
            if (rep->pulse_level <= 0.25f) {
                rep->pulse_level = 0.25f;
                rep->flags |= 0x20;
            }
        }
        SetHighlight(true);
    }
}

// FUNCTION: WIZ8 0x0049FB30
unsigned char W8Item::GetCachedLocalBounds(srVector3T<float>* lower, srVector3T<float>* upper)
{
    W8ItemRep* rep = static_cast<W8ItemRep*>(m_pRep);
    lower->x = rep->bounds_minimum.x;
    lower->y = rep->bounds_minimum.y;
    lower->z = rep->bounds_minimum.z;
    upper->x = rep->bounds_maximum.x;
    upper->y = rep->bounds_maximum.y;
    upper->z = rep->bounds_maximum.z;
    return true;
}

// FUNCTION: WIZ8 0x0049FB80
bool W8Item::GetBoundsRadius(float* radius)
{
    *radius = static_cast<W8ItemRep*>(m_pRep)->bounds_radius;
    return true;
}

/* Raise the representation location by two thirds of the cached local height.
   Searchables, the automap, and the inlined selection test all use this point. */
static const float g_item_bounds_vertical_factor_005ecd88 = 0.66f;

// FUNCTION: WIZ8 0x0049FBA0
unsigned char W8Item::GetSearchPosition(srVector3T<float>* location)
{
    srVector3T<float> lower;
    srVector3T<float> upper;

    if (GetCachedLocalBounds(&lower, &upper) == 0) {
        return 0;
    }
    m_pRep->GetLocation004B8890(location);
    location->y += (upper.y - lower.y) * g_item_bounds_vertical_factor_005ecd88;
    return 1;
}

// FUNCTION: WIZ8 0x0049FBF0
void W8Item::SetYaw(float angle)
{
    srMatrix3T<float> rotation;
    rotation.SetIdentity();
    double yaw = NormalizeAngle(angle);
    if (yaw != 0.0) {
        double cosine = cos(yaw);
        double sine = sin(yaw);
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        first.Set(cosine, 0.0, sine);
        second.Set(0.0, 1.0, 0.0);
        third.Set(-sine, 0.0, cosine);
        srMatrix3T<float> step;
        step.SetRows(first, second, third);
        rotation.MultiplyBy(step);
    }
    srNode* mesh = static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
    m_pRep->SetRotation004B88D0(&rotation);
    if (mesh->firstChild() == 0) {
        mesh->setRotation(rotation);
    } else {
        for (srNode* child = mesh->firstChild(); child != 0; child = child->nextSibling()) {
            child->setRotation(rotation);
        }
    }
}

/* The receiver is the item, not its representation. Blue takes precedence
   over green, and retail does not multiply either colour by pulse_level. */
// FUNCTION: WIZ8 0x0049FDB0
void W8Item::SetHighlight(bool enabled)
{
    W8ItemRep* rep = static_cast<W8ItemRep*>(m_pRep);
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;
    if (enabled) {
        if ((rep->flags & 0x80) != 0) {
            blue = 1.0f;
            alpha = 1.0f;
        } else if ((rep->flags & 0x10) != 0) {
            green = 1.0f;
            alpha = 1.0f;
        }
    }
    rep->render_state_04c.highlight_red = 0.0f;
    rep->render_state_04c.highlight_green = green;
    rep->render_state_04c.highlight_blue = blue;
    rep->render_state_04c.highlight_alpha = alpha;
}

// FUNCTION: WIZ8 0x0049FF40
bool W8Item::IsSelected()
{
    srModelInstance* selected = GetValue65962C();
    srVector3T<float> location;
    /* The selection-point helper is inlined here even though its result is
       not used by the final mesh-identity test. */
    GetSearchPosition(&location);
    return selected == static_cast<W8ItemRep*>(m_pRep)->m_psrMesh;
}

// FUNCTION: WIZ8 0x0049FFA0
float W8Item::DistanceToCamera(W8World* world)
{
    srVector3T<float> lower;
    srVector3T<float> upper;
    if (!GetCachedLocalBounds(&lower, &upper)) {
        return -1.0f;
    }
    srVector3T<double> camera = world->camera->getLocation();
    srVector3T<float> camera_location;
    camera_location.SetFromDouble(&camera);
    srVector3T<float> center = m_pRep->location_004;
    /* Unlike the selection-point helper, retail offsets Z by half the Z
       extent here, not Y by 0.66 of the Y extent. */
    center.z += (upper.z - lower.z) * 0.5f;
    return (camera_location - center).Length();
}

// FUNCTION: WIZ8 0x004A0030
void W8Item::LightRadarBlip()
{
    value_01c = SetCountdownClock(10000);
}

/* Fire the item's trigger with no source and report its action state. States
   1 and 4 propagate; a finished trigger, a missing trigger or a missing item
   collapse to 1/0. */
// FUNCTION: WIZ8 0x004A0070
unsigned char RunItemTrigger004A0070(W8Item* item)
{
    Trigger* trigger;

    if (item == 0) {
        return 0;
    }
    trigger = item->trigger_018;
    if (trigger == 0) {
        return 1;
    }
    trigger->Run(-1);
    if (trigger->action_state_232 == 1 || trigger->action_state_232 == 4) {
        return trigger->action_state_232;
    }
    return 1;
}

// FUNCTION: WIZ8 0x004A00C0
bool GetItemWorldBounds(W8Item* item, srVector3T<float>* lower, srVector3T<float>* upper)
{
    if (item == 0) {
        return false;
    }
    item->GetCachedLocalBounds(lower, upper);
    srVector3T<float> location;
    item->m_pRep->GetLocation004B8890(&location);
    *lower += location;
    *upper += location;
    return true;
}
