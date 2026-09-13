#include "wiz8/vector.h"

#include <stdlib.h>
#include <string.h>

// TEMPLATE: WIZ8 0x004addf0
// W8GrowableVector<int>::Grow

// TEMPLATE: WIZ8 0x004d99e0
// W8GrowableVector<T*>::RemoveAt

// TEMPLATE: WIZ8 0x00445f70
// W8GrowableVector<T*>::Add

// TEMPLATE: WIZ8 0x00474c60
// W8GrowableVector<unsigned char>::W8GrowableVector

// TEMPLATE: WIZ8 0x00474ca0
// W8GrowableVector<unsigned char>::Add

// TEMPLATE: WIZ8 0x005c3490
// W8GrowableVector<int>::SetAt

// TEMPLATE: WIZ8 0x005c3790
// W8GrowableVector<int>::GetAt

// VTABLE: WIZ8 0x005ebfe0
// class W8GrowableVector<int>

/* 0x005EBFE4 is W8SpellEffectEntry::effects' final table; CastSpellFromSource
   0x004FB4C0 writes 0x005EC280 while the entry is under construction and
   0x005EBFE4 once it is complete, so 0x005EC280 is that same specialization's
   construction-phase table, not a second W8SpellVisual vector. */
// VTABLE: WIZ8 0x005ebfe4
// class W8GrowableVector<W8SpellVisual*>

// SYNTHETIC: WIZ8 0x0042bba0
// W8GrowableVector<W8SpellVisual*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451ac0
// W8GrowableVector<W8SpellVisual*>::`scalar deleting destructor' (construction-phase copy)

// SYNTHETIC: WIZ8 0x0042bb70
// W8GrowableVector<int>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00474e30
// W8GrowableVector<unsigned char>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048cea0
// W8GrowableVector<unsigned short>::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ee8c8
// class W8GrowableVector<W8ChunkHead*>

// VTABLE: WIZ8 0x005ec15c
// class W8GrowableVector<W8WorldItem*>

class W8Missile;

/* 0x005EBFE8 is the construction-phase table of this specialization; the
   final table is 0x005EC27C, which CastSpellFromSource 0x004FB4C0 writes into
   W8SpellEffectEntry::missiles and ConstructWorldCollections writes into
   W8World::missiles. */
// VTABLE: WIZ8 0x005ec27c
// class W8GrowableVector<W8Missile*>

// TEMPLATE: WIZ8 0x00501e50
// W8GrowableVector<W8Missile*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00451b00
// W8GrowableVector<W8Missile*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0042bbd0
// W8GrowableVector<W8Missile*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x00451b20
// W8GrowableVector<W8Missile*>::~W8GrowableVector<W8Missile*>

class W8SpellDamageReport;

// VTABLE: WIZ8 0x005ebfec
// class W8GrowableVector<W8SpellDamageReport*>

// SYNTHETIC: WIZ8 0x0042bb40
// W8GrowableVector<W8SpellDamageReport*>::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ec51c
// class W8GrowableVector<unsigned char>

// VTABLE: WIZ8 0x005eca78
// class W8GrowableVector<unsigned short>

// VTABLE: WIZ8 0x005ee7e8
// class W8GrowableVector<W8TargetSource>

// SYNTHETIC: WIZ8 0x00546dc0
// W8GrowableVector<W8TargetSource>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00546df0
// W8GrowableVector<W8TargetSource>::Grow

struct W8JournalEntry;

// VTABLE: WIZ8 0x005ee8a0
// class W8GrowableVector<W8JournalEntry>

// SYNTHETIC: WIZ8 0x00558c10
// W8GrowableVector<W8JournalEntry>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005be1f0
// W8GrowableVector<W8JournalEntry>::Grow

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8NpcState;
struct W8MessageBoxLine;

// VTABLE: WIZ8 0x005ed890
// class W8GrowableVector<W8MessageBoxLine*>

// VTABLE: WIZ8 0x005ed894
// class W8GrowableVector<int*>

// SYNTHETIC: WIZ8 0x0052a290
// W8GrowableVector<W8MessageBoxLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0052a2c0
// W8GrowableVector<int*>::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ed810
// class W8GrowableVector<W8NpcState*>

// SYNTHETIC: WIZ8 0x0050e510
// W8GrowableVector<W8NpcState*>::`scalar deleting destructor'

/* Direct W8GrowableVector specialization identified by its vtable. */

/* 0x005EBFB4 is AutomapScreen.cpp's g_releasable_68f1f4, declared
   W8GrowableVector<srClass*>*; its initializer at 0x00582310 installs this
   final table. The vtable is not emitted by recovered source yet because the
   constructing body is still unrecovered. */
// VTABLE: WIZ8 0x005ebfb4
// class W8GrowableVector<srClass*>

// TEMPLATE: WIZ8 0x0042a260
// W8GrowableVector<srClass*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0042a310
// W8GrowableVector<srClass*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0042a2c0
// W8GrowableVector<srClass*>::~W8GrowableVector<srClass*>

/* Direct W8GrowableVector specialization identified by its vtable. */

class stModelInstance;

// VTABLE: WIZ8 0x005ec018
// class W8GrowableVector<stModelInstance*>

// TEMPLATE: WIZ8 0x004cad80
// W8GrowableVector<stModelInstance*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00438f70
// W8GrowableVector<stModelInstance*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00438c70
// W8GrowableVector<stModelInstance*>::~W8GrowableVector<stModelInstance*>

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8EncounterScriptName;

// VTABLE: WIZ8 0x005ec164
// class W8GrowableVector<W8EncounterScriptName*>

// TEMPLATE: WIZ8 0x00445ff0
// W8GrowableVector<W8EncounterScriptName*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00446190
// W8GrowableVector<W8EncounterScriptName*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00446050
// W8GrowableVector<W8EncounterScriptName*>::~W8GrowableVector<W8EncounterScriptName*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Trigger.cpp's g_timed_events_006599b8. */
// VTABLE: WIZ8 0x005ec16c
// class W8GrowableVector<W8TriggerEvent*>

// TEMPLATE: WIZ8 0x00446090
// W8GrowableVector<W8TriggerEvent*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00446230
// W8GrowableVector<W8TriggerEvent*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004460f0
// W8GrowableVector<W8TriggerEvent*>::~W8GrowableVector<W8TriggerEvent*>

/* Emitted lifecycle bodies belong directly to the template specialization. */

// TEMPLATE: WIZ8 0x00484870
// W8GrowableVector<stLight*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00451d10
// W8GrowableVector<stLight*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451d30
// W8GrowableVector<stLight*>::~W8GrowableVector<stLight*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\3dapi.cpp's g_worlds_00659a80. */
// VTABLE: WIZ8 0x005ec2b8
// class W8GrowableVector<W8World*>

// TEMPLATE: WIZ8 0x00451a40
// W8GrowableVector<W8World*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00451b70
// W8GrowableVector<W8World*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451aa0
// W8GrowableVector<W8World*>::~W8GrowableVector<W8World*>

/* Direct W8GrowableVector specialization identified by its vtable. */

class W8Navigator;

// VTABLE: WIZ8 0x005ec324
// class W8GrowableVector<W8Navigator*>

// TEMPLATE: WIZ8 0x00456140
// W8GrowableVector<W8Navigator*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004561f0
// W8GrowableVector<W8Navigator*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004561a0
// W8GrowableVector<W8Navigator*>::~W8GrowableVector<W8Navigator*>

/* stMeshModel.cpp's mesh-model registry at 0x00659CB8: the destructor at
   0x00470ED0 walks count 0x659CBC / data 0x659CC4 and unlinks the model. */
// VTABLE: WIZ8 0x005ec514
// class W8GrowableVector<stMeshModel*>

// TEMPLATE: WIZ8 0x00474be0
// W8GrowableVector<stMeshModel*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00474e10
// W8GrowableVector<stMeshModel*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00474c40
// W8GrowableVector<stMeshModel*>::~W8GrowableVector<stMeshModel*>

/* 0x005ECA5C is an unresolved pointer-vector specialization (ctor 0x00489ED0,
   deleting destructor 0x0048A140, complete destructor 0x00489F30). */

/* MonGen.cpp's active monster-group list at 0x0065BA10. GenerateEncounter at
   0x0048AD20 stores W8MonsterGroup* elements through g_active_groups. */
// VTABLE: WIZ8 0x005eca98
// class W8GrowableVector<W8MonsterGroup*>

// TEMPLATE: WIZ8 0x0048cda0
// W8GrowableVector<W8MonsterGroup*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048cf00
// W8GrowableVector<W8MonsterGroup*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0048ce00
// W8GrowableVector<W8MonsterGroup*>::~W8GrowableVector<W8MonsterGroup*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\MonGen.cpp's g_encounter_tables. */
// VTABLE: WIZ8 0x005ecaa0
// class W8GrowableVector<W8EncounterTableRuntime*>

// TEMPLATE: WIZ8 0x0048ce20
// W8GrowableVector<W8EncounterTableRuntime*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048cf50
// W8GrowableVector<W8EncounterTableRuntime*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0048ce80
// W8GrowableVector<W8EncounterTableRuntime*>::~W8GrowableVector<W8EncounterTableRuntime*>

/* 0x005ECAD0 is another unresolved specialization that ctor 0x0048F190 builds
   (deleting destructor 0x0048F240, complete destructor 0x0048F1F0). */

/* 0x005ED43C is the W8MasterFunction (void (*)(int)) pointer-vector
   specialization emitted by MasterFunctionList.cpp: ctor 0x004D9A70 (its only
   caller is InitializeLevelMasterFunctions004D6C50's five-element
   construction) and scalar deleting destructor 0x004D9A40. No other
   specialization shares the vtable, which is what identifies the
   DialogFactoryDialogs.cpp member embedded at +0x64. */

/* The emitted lifecycle and both reviewed vtables belong directly to the
   ordinary light-vector template specializations. */

// VTABLE: WIZ8 0x005ec294
// class W8GrowableVector<stLight*>

// VTABLE: WIZ8 0x005ece60
// class W8GrowableVector<W8GrowableVector<stLight*>*>

// TEMPLATE: WIZ8 0x004a5c30
// W8GrowableVector<W8GrowableVector<stLight*>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004a5d20
// W8GrowableVector<W8GrowableVector<stLight*>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a5c90
// W8GrowableVector<W8GrowableVector<stLight*>*>::~W8GrowableVector<W8GrowableVector<stLight*>*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\GrCycle.cpp's g_grcycles_by_name. */
// VTABLE: WIZ8 0x005ecee4
// class W8GrowableVector<W8GrowableVector<W8GrCycle*>*>

// TEMPLATE: WIZ8 0x004a8e70
// W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004a8f20
// W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a8ed0
// W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::~W8GrowableVector<W8GrowableVector<W8GrCycle*>*>

/* Emitted W8GrowableVector instantiation identified by its vtable. */

// VTABLE: WIZ8 0x005ecf00
// class W8GrowableVector<srVector3T<float>*>

// SYNTHETIC: WIZ8 0x004aaaf0
// W8GrowableVector<srVector3T<float>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004aab10
// W8GrowableVector<srVector3T<float>*>::~W8GrowableVector<srVector3T<float>*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Spells.cpp's g_sound3d_instances_65be40. */
// VTABLE: WIZ8 0x005ed018
// class W8GrowableVector<stSound3D*>

// TEMPLATE: WIZ8 0x004af690
// W8GrowableVector<stSound3D*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004af740
// W8GrowableVector<stSound3D*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004af6f0
// W8GrowableVector<stSound3D*>::~W8GrowableVector<stSound3D*>

/* Direct W8GrowableVector specialization identified by its vtable. */

class srClipPlane;

// VTABLE: WIZ8 0x005ed1b8
// class W8GrowableVector<srClassSupport<srClipPlane,srClipPlane,0,5376>*>

// TEMPLATE: WIZ8 0x00585340
// W8GrowableVector<srClassSupport<srClipPlane,srClipPlane,0,5376>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004be030
// W8GrowableVector<srClassSupport<srClipPlane,srClipPlane,0,5376>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004bdfe0
// W8GrowableVector<srClassSupport<srClipPlane,srClipPlane,0,5376>*>::~W8GrowableVector<srClassSupport<srClipPlane,srClipPlane,0,5376>*>

/* 0x005ED2C8 is the embedded vector W8MonsterRep's constructor at 0x004BEA20
   initialises at +0xAC (ctor 0x004CAD00, deleting destructor 0x004CAE10,
   complete destructor 0x004CAD60); its element type stays unresolved. */

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Magic.cpp's g_spell_effects. */
// VTABLE: WIZ8 0x005ed7d4
// class W8GrowableVector<W8SpellEffectEntry*>

// TEMPLATE: WIZ8 0x00501eb0
// W8GrowableVector<W8SpellEffectEntry*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00501f60
// W8GrowableVector<W8SpellEffectEntry*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00501f10
// W8GrowableVector<W8SpellEffectEntry*>::~W8GrowableVector<W8SpellEffectEntry*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Search.cpp's g_searchables_00689fa8. */
// VTABLE: WIZ8 0x005ed840
// class W8GrowableVector<W8Searchable*>

// TEMPLATE: WIZ8 0x00517810
// W8GrowableVector<W8Searchable*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x005178c0
// W8GrowableVector<W8Searchable*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00517870
// W8GrowableVector<W8Searchable*>::~W8GrowableVector<W8Searchable*>

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8AutomapNote;

// VTABLE: WIZ8 0x005eea28
// class W8GrowableVector<W8AutomapNote*>

// SYNTHETIC: WIZ8 0x00585420
// W8GrowableVector<W8AutomapNote*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00585440
// W8GrowableVector<W8AutomapNote*>::~W8GrowableVector<W8AutomapNote*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* 0x005EF08C is the final table of a vector constructed inside the
   OptionsScreen factory at 0x005ABFC0 (the 0x005AD020/0x005AD1B0 destructor
   pair belongs to it); its element type is unresolved, so the emission stays
   in the pointer-vector ledger rather than naming an element class. */
