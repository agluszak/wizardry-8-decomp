#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/grcycle.h"
#include "wiz8/sr_api.h"
#include "surrender/srTimer.h"

extern srTimer* g_shared_timer_base;

#define MONSTER_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp"

// VTABLE: WIZ8 0x005ed200
// class W8Monster

// SYNTHETIC: WIZ8 0x004beba0
// W8Monster::`scalar deleting destructor'

struct W8Releasable {
    virtual ~W8Releasable();
};

struct W8Forwarded {
    void Method4C5290();
};

extern "C" {
extern void Function4C4EF0(void);
extern void Function4A7A70(int value);
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
}

class W8MonsterGrCycle005ED290 : public W8GrCycle {
public:
    unsigned char IsCycleSupported(signed char cycle);
    void SubmitCurrentAnimEntry004C3F00();

private:
    W8Monster* monster_1d8;
};

static_assert(
    sizeof(W8MonsterGrCycle005ED290) == 0x1dc,
    "W8MonsterGrCycle005ED290_size_must_be_0x1dc");

struct W8MonsterLinkedObject004C5870 {
    unsigned char unknown_00[0x28];
    int value_28;
};

struct W8MonsterLinkedObjectList004C5870 {
    void* unknown_00;
    int count_04;
    void* unknown_08;
    W8MonsterLinkedObject004C5870** entries_0c;

    W8MonsterLinkedObject004C5870* GetAt(int index)
    {
        W8MonsterLinkedObject004C5870** slot = entries_0c;
        if (index < count_04) {
            slot += index;
        }
        return *slot;
    }
};

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second subobject array begins. */
// FUNCTION: WIZ8 0x004bfab0
unsigned char W8Monster::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            MONSTER_CPP,
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = Subobject18().m_bCurrentCycle;
    }
    return m_cycles[bCycle].count.ubNumSubs;
}

/* The Monster vtable's slot-three method selects the active subcycle's
   AnimObj (falling back to entry zero) and submits the Monster's current
   animation index.  The assertion's `pao` spelling establishes the pointee's
   AnimObj identity without supplying a name for this Monster method. */
// FUNCTION: WIZ8 0x004bf970
void W8Monster::ApplyEmitterSetting(char cycle)
{
    W8MonsterCycle* selected_cycle = &m_cycles[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (unknown_0a5[0] <
        (int)selected_cycle->num_subs_04) {
        animation_slot = selected_cycle->animation_objects +
            unknown_0a5[0];
    } else {
        animation_slot = selected_cycle->animation_objects;
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

/* The concrete Monster GrCycle keeps its owning Monster immediately after the
   shared 0x1d8-byte GrCycle base. */
// FUNCTION: WIZ8 0x004c3740
unsigned char W8MonsterGrCycle005ED290::IsCycleSupported(signed char cycle)
{
    if (cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xafc,
            "IsCycleSupported() -> Invalid cycle num.");
    }
    return monster_1d8->m_cycles[cycle].num_subs_04 != 0;
}

/* Resolve the active cycle/subcycle AnimObj and submit entry zero using the
   Monster's animation index. */
// FUNCTION: WIZ8 0x004c3f00
void W8MonsterGrCycle005ED290::SubmitCurrentAnimEntry004C3F00()
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    int cycle_index =
        monster_1d8->Subobject18().m_bCurrentCycle;
    int subcycle_index =
        monster_1d8->Subobject18().current_subcycle_8d;
    W8MonsterCycle* cycle = &monster_1d8->m_cycles[cycle_index];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle_index < (int)cycle->num_subs_04) {
        animation_slot = cycle->animation_objects + subcycle_index;
    } else {
        animation_slot = cycle->animation_objects;
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
        monster_1d8->Subobject18().animation_index_80,
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
    monster->m_cycles[19].value_08 = value;
    *(short*)&monster->m_cycles[3].flags_00 = (short)value;
    if (monster->LinkedObjects010() != 0) {
        count = ((W8MonsterLinkedObjectList004C5870*)
            monster->LinkedObjects010())->count_04;
        index = 0;
        if (count > 0) {
            do {
                W8MonsterLinkedObjectList004C5870* list =
                    (W8MonsterLinkedObjectList004C5870*)
                    monster->LinkedObjects010();
                int propagated_value = monster->m_cycles[19].value_08;
                W8MonsterLinkedObject004C5870* object = list->GetAt(index);
                ++index;
                object->value_28 = propagated_value;
            } while (index < count);
        }
    }
}

// FUNCTION: WIZ8 0x004c5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->pending_cycle != -1;
}

/* Cycle 17's third state byte is preserved by ActivateMonster while the live
   engine object is rebuilt, then restored into the replacement. */
// FUNCTION: WIZ8 0x004c57f0
unsigned char MonsterGetCycle17State(W8Monster* monster)
{
    return monster->m_cycles[17].bytes.state_02;
}

// FUNCTION: WIZ8 0x004c5800
void MonsterSetCycle17State(W8Monster* monster, unsigned char state)
{
    monster->m_cycles[17].bytes.state_02 = state;
}

// FUNCTION: WIZ8 0x004c5820
unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->flag_5bc;
}

// FUNCTION: WIZ8 0x004c5840
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag)
{
    monster->m_cycles[18].runtime->flag_5bc = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004c5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->scale;
}

// FUNCTION: WIZ8 0x004c57a0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_cycles[18].runtime->scale = scale;
}

// FUNCTION: WIZ8 0x004c57c0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterCycleRuntime* runtime = monster->m_cycles[18].runtime;

    *minimum = runtime->minimum_scale;
    *maximum = runtime->maximum_scale;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004c5a00
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating)
{
    if (monster != 0) {
        W8MonsterCycleRuntime* runtime = monster->m_cycles[18].runtime;
        unsigned char previous = runtime->animating;

        runtime->animating = animating;
        runtime->animation_timestamp =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c59e0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_cycles[18].runtime->animating;
    }
    return 0;
}

/* Cycle 19 bit 5 blocks pending-cycle changes. Otherwise the request is stored
   as the signed low byte in cycle 18's runtime record. */
// FUNCTION: WIZ8 0x004c5aa0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && ((monster->m_cycles[19].flags_00 >> 5) & 1) == 0) {
        monster->m_cycles[18].runtime->pending_cycle = (signed char)cycle;
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
        monster->m_cycles[18].runtime->behaviour = behaviour;
    }
}

// FUNCTION: WIZ8 0x004c5ee0
unsigned char MonsterHasCycle19Flag3(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->m_cycles[19].flags_00 >> 3) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6160
void MonsterSetStateA0(W8Monster* monster, unsigned char state)
{
    if (monster != 0) {
        monster->Subobject18().state_a0 = state;
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
                          m_cycles[18].runtime->pending_cycle == 0x15;

    return dying;
}

/* Six thin bodies over the live engine object. Each is a null check and a
   forward, or a single member read; nothing here says what the members and
   slots are for, so each is named for what it reaches. */

extern void Function4C4DE0(int arg_1, int arg_2, int arg_3);
extern void Function4C0300(int arg_1, int arg_2, int arg_3, int arg_4, int arg_5);
/* Neither takes an argument nor reads ECX: both work entirely over the pair of
   globals at 0x00659B34 and 0x00659B3C, which is what makes them free
   functions rather than the subobject methods their neighbours in the same
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
extern void MonsterPolymorphicSubobject18Update(W8MonsterPolymorphicSubobject18* member);            /* 0x00453970 */
extern void Function4A84A0(W8Monster* monster);
/* Spelled the way MonsterManager.cpp already declares it: the callee takes its
   receiver in ECX, which __fastcall is how a no-argument member call is
   reachable from a free declaration. The receiver is the monster's subobject at
   0x18, not the monster - the forwarder derives it with a `lea`. */
extern void __fastcall Function4537E0(W8MonsterPolymorphicSubobject18* member);
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
    m_cycles[18].runtime->value_0a6 = value;
    if (m_cycles[18].runtime->pending_cycle == -1) {
        m_cycles[18].runtime->pending_cycle = m_cycles[18].runtime->value_0a4;
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
        monster->m_cycles[18].runtime->block_04c = block;
    }
}


/* The engine object a monster holds at 0x0c, or nothing when there is no
   monster to ask. */
// FUNCTION: WIZ8 0x004c5b30
void* MonsterGetObject0C(W8Monster* monster)
{
    if (monster != 0) {
        return *(void**)((char*)monster + 0xc);
    }
    return 0;
}

/* Run the member update with no monster of its own to name. */
// FUNCTION: WIZ8 0x004c5770
void MonsterUpdatePolymorphicSubobject18(void)
{
    MonsterPolymorphicSubobject18Update(0);
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
        Function4537E0(&monster->Subobject18());
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
 * Six more null-guarded forwarders onto the subobject at 0x18, the same shape
 * MonsterForward4537E0 has: the guard tests the monster, the receiver is
 * derived from it with a `lea`, and a monster that is not there is simply not
 * acted on. What each one answers on the null path is the evidence for its
 * return type - a cleared AL for the byte-sized ones, a loaded 0.0f for the
 * float, and a bare return for the four that hand nothing back.
 */
// FUNCTION: WIZ8 0x004c5f50
void MonsterSetSubobjectValue120(W8Monster* monster, float value)
{
    if (monster != 0) {
        monster->Subobject18().SetValue120(value);
    }
}

// FUNCTION: WIZ8 0x004c5f70
float MonsterGetSubobjectValue120(W8Monster* monster)
{
    if (monster != 0) {
        float value = monster->Subobject18().GetValue120();
        return value;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x004c5f90
unsigned char MonsterForward452630(W8Monster* monster, const W8Position* position)
{
    if (monster != 0) {
        return monster->Subobject18().Function452630(position);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5fb0
void MonsterForward453690(W8Monster* monster, void* argument)
{
    if (monster != 0) {
        monster->Subobject18().Function453690(argument);
    }
}

// FUNCTION: WIZ8 0x004c5fd0
void MonsterSetSubobjectObject68Flag38(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->Subobject18().SetObject68Flag38(value);
    }
}

// FUNCTION: WIZ8 0x004c6200
void MonsterSetSubobjectFlag25(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->Subobject18().SetFlag25(value);
    }
}

/* Hands the shared reference position to one of the subobject's two position
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
            0x14b3, MONSTER_CPP, monster->m_cycles[19].location_id_08, 1));
        if (monster_info->control_state != 1) {
            GetPosition421070(&position);
            if (alternate != 0) {
                monster->Subobject18().Function454040(&position);
            } else {
                monster->Subobject18().Function453F30(&position);
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
void Function4C5860(W8Releasable* object)
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
