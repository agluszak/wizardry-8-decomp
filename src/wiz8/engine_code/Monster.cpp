#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/grcycle.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector_005ec294.h"
#include "surrender/srTimer.h"

#include <string.h>

extern srTimer* g_shared_timer_base;

#define MONSTER_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp"

// VTABLE: WIZ8 0x005ed200
// class W8MonsterRep

// SYNTHETIC: WIZ8 0x004beba0
// W8MonsterRep::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004bea20
W8MonsterRep::W8MonsterRep()
    : flag_5bc(0),
      name_5c0(0),
      linked_objects_5e8(0),
      value_5ec(0),
      scale_5f0(1.0f),
      minimum_scale_5f4(0.0f),
      maximum_scale_5f8(0.0f),
      value_5fc(1.0f),
      flag_600(0),
      flag_601(0),
      value_604(10.0f),
      value_608(0),
      value_60c(0),
      value_610(0),
      monster_light_624(0)
{
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
}

// FUNCTION: WIZ8 0x004bebd0
W8MonsterRep::W8MonsterRep(const W8MonsterRep& other)
    : W8EmitterHost(other),
      flag_5bc(other.flag_5bc),
      linked_objects_5e8(0),
      value_5ec(other.value_5ec),
      scale_5f0(other.scale_5f0),
      minimum_scale_5f4(other.minimum_scale_5f4),
      maximum_scale_5f8(other.maximum_scale_5f8),
      value_5fc(other.value_5fc),
      flag_600(other.flag_600),
      flag_601(other.flag_601),
      value_604(other.value_604),
      value_608(other.value_608),
      value_60c(other.value_60c),
      value_610(other.value_610),
      monster_light_624(0)
{
    signed char cycle;
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        Method004BF0F0(cycle, &other, cycle);
    }
    if (other.monster_light_624 != 0) {
        monster_light_624 = new MonsterLight(*other.monster_light_624);
    }
    name_5c0 = 0;
    if (other.name_5c0 != 0) {
        name_5c0 = new char[strlen(other.name_5c0) + 1];
        if (name_5c0 != 0) {
            strcpy(name_5c0, other.name_5c0);
        }
    }
}

extern void DestroyAnimObj004A01E0(W8AnimObj* animation);

// FUNCTION: WIZ8 0x004bee50
W8MonsterRep::~W8MonsterRep()
{
    int cycle;
    int index;

    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = animations[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            W8AnimObj* animation = *animations[cycle].GetAt(index);
            if (animation != 0) {
                DestroyAnimObj004A01E0(animation);
            }
        }
    }
    for (index = 0; index < 8; ++index) {
        delete objects_5c8[index];
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = light_lists[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            DestroyLightVector(*light_lists[cycle].GetAt(index));
        }
        light_lists[cycle].Clear();
    }
    while (linked_runtime_objects_614.GetCount() != 0) {
        delete linked_runtime_objects_614.RemoveAt(0);
    }
    delete monster_light_624;
    delete[] name_5c0;
}

struct W8Forwarded {
    void Method4C5290();
};

extern "C" {
extern void Function4C4EF0(void);
extern void Function4A7A70(int value);
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
}

// VTABLE: WIZ8 0x005ed22c W8Monster
// VTABLE: WIZ8 0x005ed218 W8Navigator
// class W8Monster

// SYNTHETIC: WIZ8 0x004bfde0
// W8Monster::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004cae30
// W8Monster::`vector deleting destructor' adjustor{24}

extern int g_monster_cycle_registry_weight_0065ba4c;
extern void PrepareMonsterCycleForDestruction004ACF90(
    W8Monster* cycle);
extern void __fastcall RefreshMonsterCycleRegistry004C6B10(
    W8Monster* cycle);

// FUNCTION: WIZ8 0x004bfb00
W8Monster::W8Monster()
{
    memset(&value_1e0, 0, 0x58);
    memset(&state_28c, 0, sizeof(state_28c));
    memset(&state_2ac, 0, sizeof(state_2ac));
    memset(&state_2fc, 0, sizeof(state_2fc));
    memset(&flags_330, 0, sizeof(flags_330));

    flags_1dc = 0;
    value_1e0 = -1;
    propagated_value_1e4 = -1;
    value_1e8 = 1.0f;
    value_1ec = 1.0f;
    value_1f0 = 1.0f;
    value_210 = -1;
    object_238 = 0;
    value_23c = 0;
    value_240 = -1;
    value_278 = 0;
    registry_weight_27c = 0;
    formation.value_00 = 0;
    formation.value_04 = 0;
    formation.value_08 = 0;
    object_334 = 0;

    m_pRep = new W8MonsterRep;
    m_pRep->linked_objects_5e8 = PListCreate();
}

// FUNCTION: WIZ8 0x004bfe00
W8Monster::W8Monster(const W8Monster& rhs)
    : W8GrCycle(rhs),
      flags_1dc(rhs.flags_1dc),
      value_1e0(rhs.value_1e0),
      propagated_value_1e4(rhs.propagated_value_1e4),
      value_1e8(1.0f),
      value_1ec(1.0f),
      value_1f0(1.0f),
      value_1f4(rhs.value_1f4),
      value_1f8(rhs.value_1f8),
      flag_1fc(0),
      flag_1fd(0),
      value_200(0),
      value_204(0),
      value_208(0),
      value_210(-1),
      flag_215(rhs.flag_215),
      flag_216(1),
      flag_217(0),
      flag_218(0),
      value_21c(rhs.value_21c),
      value_220(rhs.value_220),
      value_224(rhs.value_224),
      value_228(rhs.value_228),
      flag_22c(0),
      flag_22d(0),
      object_238(0),
      value_240(-1),
      value_278(0),
      registry_weight_27c(rhs.registry_weight_27c),
      object_334(0)
{
    formation.value_00 = 0;
    formation.value_04 = 0;
    formation.value_08 = 0;
    flags_330.flag_00 = 0;
    flags_330.flag_01 = 0;
    flags_330.copied_flag_02 = rhs.flags_330.copied_flag_02;

    if (rhs.m_pRep == 0) {
        srAssertFail(
            "rhs.m_pRep",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            1099,
            0);
    }
    m_pRep = static_cast<W8MonsterRep*>(rhs.m_pRep->Clone());
    m_pRep->linked_objects_5e8 = PListCreate();

    state_28c.flag_00 = 0;
    state_28c.flag_01 = 0;
    state_28c.value_02 = -1;
    state_28c.flag_03 = 0;
    state_28c.flag_04 = 0;
    state_28c.flag_05 = 0;
    state_2ac.flag_00 = 0;
    state_2ac.value_04 = 0;
    state_2ac.value_08 = 0;
    state_2ac.value_0c = 0;
    state_2ac.value_1c = 0;
    state_2ac.value_20 = 0;
    state_2ac.value_24 = -1;
    state_2ac.flag_28 = 1;
    state_2fc.scale_00 = 1.0f;
    state_2fc.scale_04 = 1.0f;
    flags_1dc &= 0xfffffcb6;
    state_22e = 0;
    unknown_230[0] = 0;
    if (flags_330.copied_flag_02 != 0) {
        fields.state_088 = 0;
    }
}

// FUNCTION: WIZ8 0x004c0170
W8Monster::~W8Monster()
{
    SetLights(0);
    if (m_pRep->linked_objects_5e8 != 0) {
        PrepareMonsterCycleForDestruction004ACF90(this);
        PListDestroy(m_pRep->linked_objects_5e8);
    }
    if (IsSoleGrCycleForName(this)) {
        g_monster_cycle_registry_weight_0065ba4c -= registry_weight_27c;
        RefreshMonsterCycleRegistry004C6B10(this);
    }
    UnregisterGrCycle(this);
    delete m_pRep;
    if (object_238 != 0) {
        object_238->release();
        object_238 = 0;
    }
    flags_1dc &= ~0x20;
    value_23c = 0;
    value_240 = -1;
    if (object_334 != 0) {
        object_334->release();
        object_334 = 0;
    }
}

/* Navigator is W8Monster's second base at +0x18. VC6 places this override in
   that secondary table and emits the adjusted entry form at 0x004CA840. */
// FUNCTION: WIZ8 0x004ca840
void W8Monster::SetPosition(const W8Position* position)
{
    vslot9()->SetLocation004B8850(position);
    m_pRep->SetLocation004B8850(position);
    SetPositionInternal00453590(position);
    fields.position_dirty_09c = 1;
}

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second vector array begins. */
// FUNCTION: WIZ8 0x004bfab0
unsigned char W8MonsterRep::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            MONSTER_CPP,
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = selection.monster.current_cycle;
    }
    return (unsigned char)animations[bCycle].GetCount();
}

/* Select the active AnimObj for a cycle and dispatch the requested LOD/frame.
   This is the concrete implementation behind AnimRep's third vtable slot. */
// FUNCTION: WIZ8 0x004bf8c0
void W8MonsterRep::SetCycleFrameLod(
    signed char cycle, int frame, int lod)
{
    typedef int (__cdecl *DispatchCall)(W8AnimObj*, int, int);

    int subcycle = selection.monster.current_subcycle;
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data + subcycle;
    }
    else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation->flag_05 == 0) {
        ((DispatchCall)AnimObjDispatch004A14D0)(animation, lod, frame);
    }
    else {
        ((DispatchCall)AnimObjDispatchList004A1560)(animation, lod, 0);
    }
}

/* The Monster vtable's slot-three method selects the active subcycle's
   AnimObj (falling back to entry zero) and submits the Monster's current
   animation index.  The assertion's `pao` spelling establishes the pointee's
   AnimObj identity without supplying a name for this Monster method. */
// FUNCTION: WIZ8 0x004bf970
void W8MonsterRep::ApplyEmitterSetting(char cycle)
{
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (selection.monster.current_subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data +
            selection.monster.current_subcycle;
    } else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x2de,
            0);
    }
    AnimObjValue004A15D0(
        animation, setting_98);
}

/* W8Monster stores its animation object immediately after the shared
   0x1d8-byte GrCycle base. */
// FUNCTION: WIZ8 0x004c3740
unsigned char W8Monster::IsCycleSupported(signed char cycle)
{
    if (cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xafc,
            "IsCycleSupported() -> Invalid cycle num.");
    }
    return m_pRep->animations[cycle].GetCount() != 0;
}

/* Resolve the active cycle/subcycle AnimObj and submit entry zero using the
   Monster's animation index. */
// FUNCTION: WIZ8 0x004c3f00
void W8Monster::SubmitCurrentAnimEntry004C3F00()
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    int cycle_index =
        m_pRep->selection.monster.current_cycle;
    int subcycle_index =
        m_pRep->selection.monster.current_subcycle;
    W8MonsterAnimationVector* cycle = &m_pRep->animations[cycle_index];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle_index < cycle->GetCount()) {
        animation_slot = cycle->data + subcycle_index;
    } else {
        animation_slot = cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xc4e,
            0);
    }
    /* This canonical caller passes the legacy three-argument call shape to
       0x004A1660 even though that callee's own body has a four-slot prototype.
       Preserve the observed caller ABI locally rather than weakening the
       callee's reviewed declaration. */
    ((LegacyAnimObjEntryCall)AnimObjEntry004A1660)(
        animation,
        fields.animation_index_080,
        0);
}

/* Store one value in the two cycle records used as its compact mirrors, then
   propagate it to every attached object's +0x28 field.  The body consumes two
   cdecl arguments; callers that reserve another stack slot clean it themselves. */
// FUNCTION: WIZ8 0x004c5870
void MonsterPropagateValue004C5870(W8Monster* monster, int value)
{
    int index;
    int count;

    if (monster == 0) {
        srAssertFail(
            "pMonster",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x125f,
            0);
    }
    monster->propagated_value_1e4 = value;
    monster->fields.location_id_0c4 = (unsigned short)value;
    if (monster->m_plsSoundEvents != 0) {
        count = monster->m_plsSoundEvents->GetCount();
        index = 0;
        if (count > 0) {
            do {
                int propagated_value = monster->propagated_value_1e4;
                W8VectorElement005ED094* object =
                    *monster->m_plsSoundEvents->GetAt(index);
                ++index;
                object->value_028 = propagated_value;
            } while (index < count);
        }
    }
}

// FUNCTION: WIZ8 0x004c5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_pRep->selection.monster.pending_cycle != -1;
}

/* Cycle 17's third state byte is preserved by ActivateMonster while the live
   engine object is rebuilt, then restored into the replacement. */
// FUNCTION: WIZ8 0x004c57f0
unsigned char MonsterGetCycle17State(W8Monster* monster)
{
    return monster->unknown_1be;
}

// FUNCTION: WIZ8 0x004c5800
void MonsterSetCycle17State(W8Monster* monster, unsigned char state)
{
    monster->unknown_1be = state;
}

// FUNCTION: WIZ8 0x004c5820
unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster)
{
    return monster->m_pRep->flag_5bc;
}

// FUNCTION: WIZ8 0x004c5840
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag)
{
    monster->m_pRep->flag_5bc = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004c5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_pRep->scale_5f0;
}

// FUNCTION: WIZ8 0x004c57a0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_pRep->scale_5f0 = scale;
}

// FUNCTION: WIZ8 0x004c57c0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterRep* runtime = monster->m_pRep;

    *minimum = runtime->minimum_scale_5f4;
    *maximum = runtime->maximum_scale_5f8;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004c5a00
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating)
{
    if (monster != 0) {
        W8MonsterRep* runtime = monster->m_pRep;
        unsigned char previous = runtime->flag_06d;

        runtime->flag_06d = animating;
        runtime->timer_068 =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c59e0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pRep->flag_06d;
    }
    return 0;
}

/* Cycle 19 bit 5 blocks pending-cycle changes. Otherwise the request is stored
   as the signed low byte in cycle 18's runtime record. */
// FUNCTION: WIZ8 0x004c5aa0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && ((monster->flags_1dc >> 5) & 1) == 0) {
        monster->m_pRep->selection.monster.pending_cycle = (signed char)cycle;
    }
}

// FUNCTION: WIZ8 0x004c5e40
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour)
{
    if (monster != 0) {
        if (behaviour < 1 || behaviour > 3) {
            srAssertFail(
                "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                "..\\Engine Code\\Include\\AnimRep.hpp",
                0x87,
                0);
        }
        monster->m_pRep->behaviour_071 = behaviour;
    }
}

// FUNCTION: WIZ8 0x004c5ee0
unsigned char MonsterHasCycle19Flag3(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->flags_1dc >> 3) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6160
void MonsterSetStateA0(W8Monster* monster, unsigned char state)
{
    if (monster != 0) {
        monster->fields.state_088 = state;
    }
}

/* Named by the MonsterManager assertions. A null monster answers -1 rather than
   forwarding, which is how the callers tell "no monster" from a real result. */
// FUNCTION: WIZ8 0x004c5b40
int MonsterQuery(W8Monster* monster, int query)
{
    if (monster != NULL) {
        return monster->Query(query);
    }
    return -1;
}

// FUNCTION: WIZ8 0x004ca4c0
unsigned char W8Monster::IsDying()
{
    unsigned char dying = Query(6) == 0x15 ||
                          m_pRep->selection.monster.pending_cycle == 0x15;

    return dying;
}

/* Six thin bodies over the live animation object. Each is a null check and a
   forward, or a single member read; nothing here says what the members and
   slots are for, so each is named for what it reaches. */

extern void Function4C4DE0(int arg_1, int arg_2, int arg_3);
extern void Function4C0300(int arg_1, int arg_2, int arg_3, int arg_4, int arg_5);
/* Neither takes an argument nor reads ECX: both work entirely over the pair of
   globals at 0x00659B34 and 0x00659B3C, which is what makes them free
   functions rather than the Navigator methods their neighbours in the same
   address range are. */
extern void Function453160(void);
extern void Function4531A0(void);
/* Cleans its own argument - the caller at 0x004C5A40 pushes and never adjusts
   afterwards - so it is __stdcall and not the cdecl the decompiler assumes. */
extern void __stdcall Function4A7BE0(const float* position);

/* The location-id lookup pair, spelled as Magic Effects.cpp already declares
   it: the index comes first and the script part is fetched from it. */
extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id, unsigned char assert_on_failure);
extern W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int index);

/* 0x00421070, owned by the 0041F261-0042403F quarantine: the shared reference
   position every consumer of the object at 0x0065A0F8 reads. */
extern void GetPosition421070(W8Position* position);
extern void MonsterNavigatorUpdate(W8Navigator* navigator); /* 0x00453970 */
extern void Function4A84A0(W8GrCycle* monster);
/* Spelled the way MonsterManager.cpp already declares it: the callee takes its
   receiver in ECX, which __fastcall is how a no-argument member call is
   reachable from a free declaration. The receiver is the monster's Navigator
   base at +0x18. */
extern void __fastcall Function4537E0(W8Navigator* navigator);
extern void* g_monster_vtable_005ed290;

/* Copies a position into a local and hands the local on. The monster argument
   is dead beyond its own null check - the callee never receives it - which is
   the same shape the other guarded forwarders here take, except that what
   survives the guard is the copy rather than the object.
   The copy goes through the FPU one component at a time - `fld dword` then
   `fstp dword` per component - rather than as the three integer moves VC6
   emits for a plain three-float assignment, which is what this body still gets
   and the whole of its remaining difference. That shape is the signature of
   srVector3T<float>::method_00421680 expanded inline: its parameters are
   doubles, so each float round-trips through the FPU instead of being copied
   as bits. The image carries both an out-of-line COMDAT copy of that setter at
   0x00421680 and this inlined expansion, which is the multiple-translation-unit
   visibility the inlining policy asks for before a body moves into a header.

   That was measured rather than argued. Defining the setter in srMath.h and
   calling it here reproduces the copy exactly - the three fld/fstp pairs land
   instruction for instruction, leaving only a register choice and one
   scheduling swap - and takes this body from 0.375 to 0.8125. It also stops
   VC6 emitting the out-of-line copy at all, because this is the only call site
   in the tree and it inlines: 0x00421680 goes from exact to missing. The
   inlining policy requires the bundle to improve without regressing an exact
   boundary, so the trade is refused and the out-of-line definition stays.
   Hand-spelling the conversion does not work either - `(float)(double)f` is
   value-preserving, so VC6 folds it straight back to the integer copy.
   Reproducing both emissions needs a second call site that does not inline,
   which is not decidable from this one; the filed bead tracks it. */
// FUNCTION: WIZ8 0x004c5a40
void MonsterForward4A7BE0(W8Monster* monster, const W8Position* position)
{
    srVector3T<float> local;

    if (monster != 0) {
        local.x = position->x;
        local.y = position->y;
        local.z = position->z;
        Function4A7BE0(&local.x);
    }
}

/* Records a value on the cycle runtime and, when nothing is pending, seeds the
   pending cycle from the runtime's own fallback at 0x0a4 rather than leaving it
   at -1. The runtime pointer is fetched twice rather than held in a local -
   the second `mov` reloads it from the cycle - which is what says the original
   spelled the two reaches out separately instead of naming the record once. */
// FUNCTION: WIZ8 0x004c6c00
void W8Monster::SetRuntimeValueA6(unsigned char value)
{
    m_pRep->selection.monster.runtime_value_a6 = value;
    if (m_pRep->selection.monster.pending_cycle == -1) {
        m_pRep->selection.monster.pending_cycle =
            m_pRep->selection.monster.current_cycle;
    }
}

/* Writes the sixteen-byte block the cycle runtime carries at 0x4c, but only for
   a monster that is neither absent nor already answering the dying cycle to
   query six - the same 0x15 the death test compares against, reached the same
   way. Both guards leave through one shared exit, which is why the body has a
   single epilogue despite testing two things. The block arrives by value and is
   stored as one assignment. */
// FUNCTION: WIZ8 0x004c5ad0
void MonsterSetRuntimeBlock4C(W8Monster* monster, W8MonsterRuntimeBlock4C block)
{
    if (monster != 0 && monster->Query(6) != 0x15) {
        monster->m_pRep->value_04c = block;
    }
}


/* The engine object a monster holds at 0x0c, or nothing when there is no
   monster to ask. */
// FUNCTION: WIZ8 0x004c5b30
void* MonsterGetObject0C(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pAI;
    }
    return 0;
}

/* Run the member update with no monster of its own to name. */
// FUNCTION: WIZ8 0x004c5770
void UpdateMonsterNavigator(void)
{
    MonsterNavigatorUpdate(0);
}

/* Two null-checked forwards that share one shape: a monster that is not there
   is simply not acted on. */
// FUNCTION: WIZ8 0x004c5ea0
void MonsterForward4A84A0(W8Monster* monster)
{
    if (monster != 0) {
        Function4A84A0(monster);
    }
}

// FUNCTION: WIZ8 0x004c6140
void MonsterForward4537E0(W8Monster* monster)
{
    if (monster != 0) {
        Function4537E0(monster);
    }
}

/* Two more of the same null-checked shape, except that what they forward to is
   already recovered: both callees are W8GrCycle setters GrCycle.cpp owns, and
   both are reached as methods rather than as free functions - the receiver
   stays in ECX across the guard and only the value is pushed. That is what
   types the parameter as the cycle rather than as the opaque pointer the
   neighbouring forwarders take. */
// FUNCTION: WIZ8 0x004c61a0
void MonsterSetCycleBehaviour(W8GrCycle* cycle, signed char bBehaviour)
{
    if (cycle != 0) {
        cycle->SetBehaviour(bBehaviour);
    }
}

// FUNCTION: WIZ8 0x004c61c0
void MonsterSetCycleSubCycle(W8GrCycle* cycle, unsigned char subcycle)
{
    if (cycle != 0) {
        cycle->SetSubCycle(subcycle);
    }
}

/* An unguarded three-argument forward. The arguments are pushed back to front
   and handed straight on, so nothing here says what any of them mean. */
// FUNCTION: WIZ8 0x004c5eb0
void MonsterForward4C4DE0(int arg_1, int arg_2, int arg_3)
{
    Function4C4DE0(arg_1, arg_2, arg_3);
}

/* The same unguarded cdecl pass-through with five arguments. Like its
   three-argument sibling it re-pushes its own stack slots and makes a real
   call rather than jumping: caller-cleanup cdecl cannot tail-jump when there
   are stack arguments to account for. */
// FUNCTION: WIZ8 0x004c58e0
void MonsterForward4C0300(int arg_1, int arg_2, int arg_3, int arg_4, int arg_5)
{
    Function4C0300(arg_1, arg_2, arg_3, arg_4, arg_5);
}

/* Two whole-body tail calls. Neither wrapper takes an argument and neither
   callee touches ECX - both read only the pair of globals at 0x00659B34 and
   0x00659B3C - so the wrappers pass nothing on and VC6 lowers each to a bare
   jump. That is the whole difference from the cdecl pass-throughs above: with
   no stack arguments there is nothing left to clean up. */
// FUNCTION: WIZ8 0x004c61e0
void MonsterForward453160(void)
{
    Function453160();
}

// FUNCTION: WIZ8 0x004c61f0
void MonsterForward4531A0(void)
{
    Function4531A0();
}

/*
 * Six more null-guarded forwarders onto the Navigator base at +0x18, the same shape
 * MonsterForward4537E0 has: the guard tests the monster, the receiver is
 * derived from it with a `lea`, and a monster that is not there is simply not
 * acted on. What each one answers on the null path is the evidence for its
 * return type - a cleared AL for the byte-sized ones, a loaded 0.0f for the
 * float, and a bare return for the four that hand nothing back.
 */
// FUNCTION: WIZ8 0x004c5f50
void MonsterSetNavigatorValue120(W8Monster* monster, float value)
{
    if (monster != 0) {
        monster->SetValue120(value);
    }
}

// FUNCTION: WIZ8 0x004c5f70
float MonsterGetNavigatorValue120(W8Monster* monster)
{
    if (monster != 0) {
        float value = monster->GetValue120();
        return value;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x004c5f90
unsigned char MonsterForward452630(W8Monster* monster, const W8Position* position)
{
    if (monster != 0) {
        return monster->Function452630(position);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5fb0
void MonsterForward453690(W8Monster* monster, void* argument)
{
    if (monster != 0) {
        monster->Function453690(argument);
    }
}

// FUNCTION: WIZ8 0x004c5fd0
void MonsterSetNavigatorObjectFlag38(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetObject68Flag38(value);
    }
}

// FUNCTION: WIZ8 0x004c6200
void MonsterSetNavigatorFlag25(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetFlag25(value);
    }
}

/* Hands the shared reference position to one of Navigator's two position
   sinks. The monster's script part is found the long way round - the location
   id lives in the cycle array at 0x1e4, and the lookup pair turns it into the
   W8MonsterInfo whose control state gates the whole body - which is what puts
   this in Monster.cpp rather than in the engine: the assertion path the lookup
   carries names this file and line 5299.
   Control state one is the only value that suppresses the update; every other
   value falls through. The position is fetched before the flag is read, so it
   is read even on the path that turns out not to need one sink over the other,
   and the flag decides only which sink receives it. */
// FUNCTION: WIZ8 0x004c6240
void MonsterForwardReferencePosition(W8Monster* monster, char alternate)
{
    W8MonsterInfo* monster_info;
    W8Position position;

    if (monster != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x14b3, MONSTER_CPP, monster->propagated_value_1e4, 1));
        if (monster_info->control_state != 1) {
            GetPosition421070(&position);
            if (alternate != 0) {
                monster->Function454040(&position);
            } else {
                monster->Function453F30(&position);
            }
        }
    }
}

/* Install the vtable at 0x005ED290 and nothing else - the whole of a base
   constructor whose own members are all left as they were. */
// FUNCTION: WIZ8 0x004c3730
void MonsterInstallVtable5ED290(void** object)
{
    *object = &g_monster_vtable_005ed290;
}

/* Forward to the object's own vtable slot four. */
// FUNCTION: WIZ8 0x004c59b0
void MonsterCallSlot10(void* object, int argument)
{
    (*(void(**)(void*, int))(*(void***)object + 4))(object, argument);
}

extern "C" {
// FUNCTION: WIZ8 0x004C5810
void Function4C5810(W8Forwarded* target)
{
    target->Method4C5290();
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5860
void Function4C5860(W8MonsterReleasable005C8* object)
{
    if (object != NULL) {
        delete object;
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C59C0
void Function4C59C0(int enabled, int value)
{
    if (enabled != 0 && value != 0) {
        Function4A7A70(value);
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5ED0
void Function4C5ED0(int enabled)
{
    if (enabled != 0) {
        Function4C4EF0();
    }
}
}
// FUNCTION: WIZ8 0x004C6220
void SetFlag6081E4(unsigned char value)
{
    g_flag_6081e4 = value;
    g_value_659c14 = 0;
}
