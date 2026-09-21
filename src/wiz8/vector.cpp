#include "wiz8/vector.h"

#include "wiz8/local_code/SpellEffect.h"

#include <stdlib.h>
#include <string.h>

// TEMPLATE: WIZ8 0x004addf0
// W8GrowableVector<int>::Grow

// TEMPLATE: WIZ8 0x004ed900
// W8GrowableVector<int>::W8GrowableVector

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

// TEMPLATE: WIZ8 0x00438c50
// W8GrowableVector<int>::~W8GrowableVector<int> (Octree.cpp emission)

// SYNTHETIC: WIZ8 0x00474e30
// W8GrowableVector<unsigned char>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048cea0
// W8GrowableVector<unsigned short>::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ee8c8
// class W8GrowableVector<W8ChunkHead*>

// SYNTHETIC: WIZ8 0x0055cbe0
// W8GrowableVector<W8ChunkHead*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0055cb90
// W8GrowableVector<W8ChunkHead*>::~W8GrowableVector<W8ChunkHead*>

// VTABLE: WIZ8 0x005ec15c
// class W8GrowableVector<W8WorldItem*>

// SYNTHETIC: WIZ8 0x004461e0
// W8GrowableVector<W8WorldItem*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00446070
// W8GrowableVector<W8WorldItem*>::~W8GrowableVector<W8WorldItem*>

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

struct W8SpellDamageReport;

// VTABLE: WIZ8 0x005ebfec
// class W8GrowableVector<W8SpellDamageReport*>

/* The capacity-five ctor the W8SpellEffectResult::reports member calls:
   every construction site emits PUSH 5 before this out-of-line emission. */
// TEMPLATE: WIZ8 0x00516950
// W8GrowableVector<W8SpellDamageReport*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0042bb40
// W8GrowableVector<W8SpellDamageReport*>::`scalar deleting destructor'

/* The derived vftable every W8SpellDamageReport* reports member takes at
   construction: one slot, the derived scalar deleting destructor. */
// VTABLE: WIZ8 0x005ece4c
// class W8Vector<W8SpellDamageReport*>

// SYNTHETIC: WIZ8 0x004a5cb0
// W8Vector<W8SpellDamageReport*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a5cd0
// W8Vector<W8SpellDamageReport*>::~W8Vector<W8SpellDamageReport*>

// VTABLE: WIZ8 0x005ec51c
// class W8GrowableVector<unsigned char>

// VTABLE: WIZ8 0x005eca78
// class W8GrowableVector<unsigned short>

// VTABLE: WIZ8 0x005ee7e8
// class W8GrowableVector<W8TargetSource>

// SYNTHETIC: WIZ8 0x00546dc0
// W8GrowableVector<W8TargetSource>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00546da0
// W8GrowableVector<W8TargetSource>::~W8GrowableVector<W8TargetSource>

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

// TEMPLATE: WIZ8 0x0052a1d0
// W8GrowableVector<W8MessageBoxLine*>::~W8GrowableVector<W8MessageBoxLine*>

// VTABLE: WIZ8 0x005ed894
// class W8GrowableVector<int*>

/* Emitted for the NPC-state line queue at 0x0068C4BC (Ghidra types it
   W8GrowableVector<int*>); the only caller is 0x00525FA0 in the NPC
   Scripting.cpp span, which is where this InsertAt was emitted. */
// TEMPLATE: WIZ8 0x0052a1f0
// W8GrowableVector<int*>::InsertAt

// SYNTHETIC: WIZ8 0x0052a290
// W8GrowableVector<W8MessageBoxLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0052a2c0
// W8GrowableVector<int*>::`scalar deleting destructor'

/* Local Code\Health Stamina Mana.cpp's W8CharacterEventQueue members: the
   queue constructor writes each vector's table in two stages, first
   0x005EE74C - this specialization's construction-phase table - then the
   final 0x005EE748. The deleting destructor at 0x0052E570 chains to the
   destructor body at 0x0052E520, which reinstalls the construction-phase
   table before freeing data. */
class W8CharacterEvent;

// VTABLE: WIZ8 0x005ee748
// class W8GrowableVector<W8CharacterEvent*>

// SYNTHETIC: WIZ8 0x0052e570
// W8GrowableVector<W8CharacterEvent*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0052e540
// W8GrowableVector<W8CharacterEvent*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x0052e520
// W8GrowableVector<W8CharacterEvent*>::~W8GrowableVector<W8CharacterEvent*>

// TEMPLATE: WIZ8 0x0052e4d0
// W8GrowableVector<W8CharacterEvent*>::Remove

// VTABLE: WIZ8 0x005ed810
// class W8GrowableVector<W8NpcState*>

// SYNTHETIC: WIZ8 0x0050e510
// W8GrowableVector<W8NpcState*>::`scalar deleting destructor'

/* Direct W8GrowableVector specialization identified by its vtable. */

/* AutomapScreen.cpp's CreateAutomapMarkerSprites005822C0 constructs
   g_releasable_68f1f4 with `new W8GrowableVector<srClass*>(5)`. That use is
   what emits this specialization. Retail splits the capacity ctor there: a
   helper at 0x00585460 writes the construction-phase table 0x005EBFB8 and
   allocates the array, then the final-vtable store at 0x00582310 (inside
   CreateAutomapMarkerSprites, not a separate function) installs 0x005EBFB4.
   The complete TEMPLATE ctor below performs both stages in one body. Like the
   other construction-phase tables in this file, 0x005EBFB8 carries no marker
   of its own. */
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

/* 0x005EC004 is the construction-phase table of this specialization: the
   complete-object ctor 0x004CAD80 writes it during setup and 0x005EC018 when
   the object is finished, and the member-construction emission 0x004390F0
   leaves it in place for the enclosing ctor to overwrite. Like the other
   construction-phase tables in this file it carries no marker of its own. */
// VTABLE: WIZ8 0x005ec018
// class W8GrowableVector<stModelInstance*>

// TEMPLATE: WIZ8 0x004cad80
// W8GrowableVector<stModelInstance*>::W8GrowableVector

// TEMPLATE: WIZ8 0x004390f0
// W8GrowableVector<stModelInstance*>::W8GrowableVector (member-construction emission)

// SYNTHETIC: WIZ8 0x00438f70
// W8GrowableVector<stModelInstance*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00438f40
// W8GrowableVector<stModelInstance*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x00438c70
// W8GrowableVector<stModelInstance*>::~W8GrowableVector<stModelInstance*>

/* Local Screens\MGSRadarMap.cpp's g_radar_icon_pools_0069bf68 emission: the
   static initializer constructs the eighteen-pool array through this ctor. */

class stModelInstance2D;

// TEMPLATE: WIZ8 0x005a20d0
// W8GrowableVector<stModelInstance2D*>::W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8EncounterScriptName;

/* W8Vector<W8EncounterScriptName*>: the derived table 0x005ec164 rides over
   the base W8GrowableVector table 0x005ec168. AutomapScreenEnter constructs
   a five-element local through the shared base ctor 0x00474FB0 and stamps the
   derived table itself; W8EncounterTableRuntime::script_names at +0x40 calls
   the emitted derived ctor 0x00445FF0. The derived dtor 0x00446050 inlines the
   base teardown (base table store plus delete[]). */
// VTABLE: WIZ8 0x005ec164
// class W8Vector<W8EncounterScriptName*>

// VTABLE: WIZ8 0x005ec168
// class W8GrowableVector<W8EncounterScriptName*>

// TEMPLATE: WIZ8 0x00445ff0
// W8Vector<W8EncounterScriptName*>::W8Vector<W8EncounterScriptName*>

// TEMPLATE: WIZ8 0x00474fb0
// W8GrowableVector<W8EncounterScriptName*>::W8GrowableVector<W8EncounterScriptName*>

// SYNTHETIC: WIZ8 0x00446190
// W8Vector<W8EncounterScriptName*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00446050
// W8Vector<W8EncounterScriptName*>::~W8Vector<W8EncounterScriptName*>

/* Direct W8GrowableVector specialization identified by its vtable. AutomapScreenEnter's
   excluded_textures is the lone capacity-constructed instance. */

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

/* Engine Code\ReadMesh.cpp's g_retained_materials_65b9d0: the static
   initializer at 0x00485AF0 constructs it with capacity five; the stores
   feed it srMaterialIFace* entries out of the mesh material arrays.
   0x005ECA60 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005eca5c
// class W8GrowableVector<srMaterialIFace*>

// TEMPLATE: WIZ8 0x00489ed0
// W8GrowableVector<srMaterialIFace*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048a140
// W8GrowableVector<srMaterialIFace*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048a110
// W8GrowableVector<srMaterialIFace*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x00489f30
// W8GrowableVector<srMaterialIFace*>::~W8GrowableVector<srMaterialIFace*>

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

/* Engine Code\stCube.cpp's g_world_cursor_nodes_65ba58: the static
   initializer at 0x0048D020 constructs it with capacity five; the table
   holds the world's W8WorldCursorNode* cursor nodes.
   0x005ECAD4 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ecad0
// class W8GrowableVector<W8WorldCursorNode*>

// TEMPLATE: WIZ8 0x0048f190
// W8GrowableVector<W8WorldCursorNode*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048f240
// W8GrowableVector<W8WorldCursorNode*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048f210
// W8GrowableVector<W8WorldCursorNode*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x0048f1f0
// W8GrowableVector<W8WorldCursorNode*>::~W8GrowableVector<W8WorldCursorNode*>

/* The W8MasterFunction (void (*)(int)) pointer-vector specialization emitted
   by MasterFunctionList.cpp. InitializeLevelMasterFunctions004D6C50's
   five-element construction calls the base ctor 0x004D9A70 and then installs
   the derived vtable 0x005ED438 itself, so the retail new-expression is
   `new W8Vector<W8MasterFunction>(5)` and g_master_functions_006834d8 is the
   thin derived type. The base vtable 0x005ED43C also tags the
   DialogFactoryDialogs.cpp member embedded at +0x64, which that unit only
   constructs, clears and destroys. No other specialization shares either
   vtable. */

// VTABLE: WIZ8 0x005ed438
// class W8Vector<W8MasterFunction>

// VTABLE: WIZ8 0x005ed43c
// class W8GrowableVector<W8MasterFunction>

// TEMPLATE: WIZ8 0x004d9a70
// W8GrowableVector<W8MasterFunction>::W8GrowableVector<W8MasterFunction> (MasterFunctionList.cpp emission)

// SYNTHETIC: WIZ8 0x004d9a20
// W8Vector<W8MasterFunction>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004d9a40
// W8GrowableVector<W8MasterFunction>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005cd6e0
// W8GrowableVector<W8MasterFunction>::~W8GrowableVector<W8MasterFunction>

/* Emitted inside the DialogFactoryDialogs.cpp span by the listbox dialog's
   destruction path; the specialization's table is 0x005ECA78. */
// TEMPLATE: WIZ8 0x005cd6c0
// W8GrowableVector<unsigned short>::~W8GrowableVector<unsigned short>

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

/* Engine Code\Monster.cpp's W8GrowableVector<W8AnimObj*> emission: the 0x1B
   elements at W8MonsterRep+0xAC (the per-cycle animations array) are built
   through ??_L with the capacity-five ctor thunk at 0x004BEBC0.
   0x005ED2CC is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ed2c8
// class W8GrowableVector<W8AnimObj*>

// TEMPLATE: WIZ8 0x004cad00
// W8GrowableVector<W8AnimObj*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004cae10
// W8GrowableVector<W8AnimObj*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004cade0
// W8GrowableVector<W8AnimObj*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x004cad60
// W8GrowableVector<W8AnimObj*>::~W8GrowableVector<W8AnimObj*>

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

/* Local Screens\OptionsScreen.cpp's W8Vector<W8OptionsSaveRow*> emission:
   W8OptionsSaveLoadPanel::m_rows at +0x90. The derived ctor emission at
   0x005ACFC0 stamps 0x005EF08C over the 0x005EF190 construction-phase table;
   the two deleting destructors are the derived and base copies. */
// VTABLE: WIZ8 0x005ef190
// class W8GrowableVector<W8OptionsSaveRow*>

// VTABLE: WIZ8 0x005ef08c
// class W8Vector<W8OptionsSaveRow*>

// TEMPLATE: WIZ8 0x005ad220
// W8GrowableVector<W8OptionsSaveRow*>::W8GrowableVector (OptionsScreen.cpp emission)

// TEMPLATE: WIZ8 0x005acfc0
// W8Vector<W8OptionsSaveRow*>::W8Vector

// TEMPLATE: WIZ8 0x005ad020
// W8GrowableVector<W8OptionsSaveRow*>::~W8GrowableVector<W8OptionsSaveRow*>

// SYNTHETIC: WIZ8 0x005ad180
// W8GrowableVector<W8OptionsSaveRow*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad1b0
// W8Vector<W8OptionsSaveRow*>::`scalar deleting destructor'

/* W8OptionsPanelSet::m_panels_010 is a W8Vector<W8OptionsPanel*>: the derived
   0x005EF01C table over base 0x005EEFE0. */
// VTABLE: WIZ8 0x005eefe0
// class W8GrowableVector<W8OptionsPanel*>

// VTABLE: WIZ8 0x005ef01c
// class W8Vector<W8OptionsPanel*>

// TEMPLATE: WIZ8 0x005ad1d0
// W8GrowableVector<W8OptionsPanel*>::W8GrowableVector (OptionsScreen.cpp emission)

// TEMPLATE: WIZ8 0x005acf80
// W8GrowableVector<W8OptionsPanel*>::~W8GrowableVector<W8OptionsPanel*>

// SYNTHETIC: WIZ8 0x005ad0e0
// W8GrowableVector<W8OptionsPanel*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad110
// W8Vector<W8OptionsPanel*>::`scalar deleting destructor'

/* W8OptionsPanel::m_text_buffers_058 is a W8Vector<W8TextBuffer*>: the derived
   0x005EEFCC table over base 0x005EEFD0. */
// VTABLE: WIZ8 0x005eefd0
// class W8GrowableVector<W8TextBuffer*>

// VTABLE: WIZ8 0x005eefcc
// class W8Vector<W8TextBuffer*>

// TEMPLATE: WIZ8 0x005acf40
// W8GrowableVector<W8TextBuffer*>::~W8GrowableVector<W8TextBuffer*>

// SYNTHETIC: WIZ8 0x005ad040
// W8GrowableVector<W8TextBuffer*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad070
// W8Vector<W8TextBuffer*>::`scalar deleting destructor'

/* W8OptionsPanel::m_option_selections is a W8Vector<W8OptionsSelection*>: the
   derived 0x005EEFC4 table over base 0x005EEFC8. */
// VTABLE: WIZ8 0x005eefc8
// class W8GrowableVector<W8OptionsSelection*>

// VTABLE: WIZ8 0x005eefc4
// class W8Vector<W8OptionsSelection*>

// TEMPLATE: WIZ8 0x005acf60
// W8GrowableVector<W8OptionsSelection*>::~W8GrowableVector<W8OptionsSelection*>

// SYNTHETIC: WIZ8 0x005ad090
// W8GrowableVector<W8OptionsSelection*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad0c0
// W8Vector<W8OptionsSelection*>::`scalar deleting destructor'

/* W8OptionsScreen::m_save_slots is a W8Vector<W8SaveSlot*>: the derived
   0x005EF00C table over base 0x005EF010. */
// VTABLE: WIZ8 0x005ef010
// class W8GrowableVector<W8SaveSlot*>

// VTABLE: WIZ8 0x005ef00c
// class W8Vector<W8SaveSlot*>

// TEMPLATE: WIZ8 0x005acfa0
// W8GrowableVector<W8SaveSlot*>::~W8GrowableVector<W8SaveSlot*>

// SYNTHETIC: WIZ8 0x005ad130
// W8GrowableVector<W8SaveSlot*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad160
// W8Vector<W8SaveSlot*>::`scalar deleting destructor'

/* Local Screens\PartySelectionScreen.cpp's
   W8PartySelectionCharacterCollection::characters: PartySelectionScreenEnter
   constructs the W8Vector member through this base-ctor emission at
   0x005C2E78, then stamps the derived 0x005EF4F0 table over the base
   0x005EF360; the collection destructor restores 0x005EF360 while tearing the
   member down. */
// VTABLE: WIZ8 0x005ef360
// class W8GrowableVector<W8Character*>

// VTABLE: WIZ8 0x005ef4f0
// class W8Vector<W8Character*>

// TEMPLATE: WIZ8 0x005c37b0
// W8GrowableVector<W8Character*>::W8GrowableVector (PartySelectionScreen.cpp emission)

// TEMPLATE: WIZ8 0x005c34b0
// W8GrowableVector<W8Character*>::~W8GrowableVector<W8Character*>

// SYNTHETIC: WIZ8 0x005c3740
// W8GrowableVector<W8Character*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005c3770
// W8Vector<W8Character*>::`scalar deleting destructor'

/* W8CharacterPage's member vector; its constructor stamps 0x005EF214 at
   0x005AFDFC. */
// VTABLE: WIZ8 0x005ef214
// class W8GrowableVector<W8CharacterPageEntry*>

// SYNTHETIC: WIZ8 0x005b1bc0
// W8GrowableVector<W8CharacterPageEntry*>::`scalar deleting destructor'

/* Local Screens\CreditsScreen.cpp's g_credit_lines_0069c4a8: the enter path
   news the vector and the element constructor allocates five 0x14-byte
   W8CreditLine slots. */
// VTABLE: WIZ8 0x005ef310
// class W8GrowableVector<W8CreditLine>

// SYNTHETIC: WIZ8 0x005bc7d0
// W8GrowableVector<W8CreditLine>::`scalar deleting destructor'
