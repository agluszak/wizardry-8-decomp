#include "wiz8/vector.h"

#include "wiz8/local_code/SpellEffect.h"

#include <stdlib.h>
#include <string.h>

/* Concrete names here are retained recomp pairing selectors unless supported
   by independent retail symbols or typed element flow. Folded storage helpers
   and construction tables alone do not establish exact template arguments. */

// TEMPLATE: WIZ8 0x004addf0
// NAME: W8GrowableVector<T>::Grow (shared four-byte storage)
// RECOMP: W8GrowableVector::Grow (shared four-byte storage)

// TEMPLATE: WIZ8 0x004ed900
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte copy)
// RECOMP: ??0?$W8GrowableVector@H@@QAE@ABV0@@Z

// TEMPLATE: WIZ8 0x004ed950
// srMatrix3T<float>::TransformTransposed

// TEMPLATE: WIZ8 0x004d99e0
// NAME: W8GrowableVector<T>::RemoveAt (shared four-byte storage)
// RECOMP: W8GrowableVector::RemoveAt (shared four-byte storage)

// TEMPLATE: WIZ8 0x00445f70
// NAME: W8GrowableVector<T>::Add
// RECOMP: W8GrowableVector<T*>::Add

// TEMPLATE: WIZ8 0x00474c60
// NAME: W8GrowableVector<T>::W8GrowableVector (one-byte default)
// RECOMP: ??0?$W8GrowableVector@E@@QAE@XZ

// TEMPLATE: WIZ8 0x00474ca0
// NAME: W8GrowableVector<T>::Add
// RECOMP: W8GrowableVector<unsigned char>::Add

// TEMPLATE: WIZ8 0x005c3490
// NAME: W8GrowableVector<T>::SetAt
// RECOMP: W8GrowableVector<int>::SetAt

// TEMPLATE: WIZ8 0x005c3790
// NAME: W8GrowableVector<T>::GetAt
// RECOMP: W8GrowableVector<int>::GetAt

// VTABLE: WIZ8 0x005ebfe0
// class W8GrowableVector<int>

/* CastSpellFromSource installs 0x005EC280 after the base constructor. The
   derived destructor at 0x00451AE0 restores 0x005EBFE4 before deleting the
   array. */
// VTABLE: WIZ8 0x005ebfe4
// class W8GrowableVector<W8SpellVisual*>

// VTABLE: WIZ8 0x005ec280
// class W8Vector<W8SpellVisual*>

// SYNTHETIC: WIZ8 0x0042bba0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8SpellVisual*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451ac0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8SpellVisual*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0042bb70 SYMBOL
// NAME: W8GrowableVector<T>::retained emission
// RECOMP: ??_G?$W8GrowableVector@H@@UAEPAXI@Z

// TEMPLATE: WIZ8 0x00438c50
// NAME: W8GrowableVector<T>::~W8GrowableVector (Octree.cpp emission)
// RECOMP: W8GrowableVector::~W8GrowableVector (Octree.cpp emission)

// SYNTHETIC: WIZ8 0x00474e30
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<unsigned char>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048cea0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<unsigned short>::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ee8c8
// class W8Vector<W8ChunkHead*>

// SYNTHETIC: WIZ8 0x0055cbe0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8ChunkHead*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0055cb90
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// VTABLE: WIZ8 0x005ec15c
// class W8Vector<W8WorldItem*>

// SYNTHETIC: WIZ8 0x004461e0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8WorldItem*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00446070
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

class W8Missile;

/* 0x005EBFE8 is the base W8GrowableVector table; the derived W8Vector
   table is 0x005EC27C, which CastSpellFromSource 0x004FB4C0 writes into
   W8SpellEffectEntry::missiles and ConstructWorldCollections writes into
   W8World::missiles. */
// VTABLE: WIZ8 0x005ebfe8
// class W8GrowableVector<W8Missile*>

// VTABLE: WIZ8 0x005ec27c
// class W8Vector<W8Missile*>

// TEMPLATE: WIZ8 0x00501e50
// NAME: W8Vector<T>::W8Vector (four-byte capacity)
// RECOMP: ??0?$W8Vector@PAVW8Missile@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x00451b00
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8Missile*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0042bbd0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Missile*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451b20
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

struct W8SpellDamageReport;

// VTABLE: WIZ8 0x005ebfec
// class W8GrowableVector<W8SpellDamageReport*>

/* The capacity-five ctor the W8SpellEffectResult::reports member calls:
   every construction site emits PUSH 5 before this out-of-line emission. */
// TEMPLATE: WIZ8 0x00516950
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte capacity)
// RECOMP: ??0?$W8GrowableVector@PAUW8SpellDamageReport@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x0042bb40
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8SpellDamageReport*>::`scalar deleting destructor'

/* The derived vftable every W8SpellDamageReport* reports member takes at
   construction: one slot, the derived scalar deleting destructor. */
// VTABLE: WIZ8 0x005ece4c
// class W8Vector<W8SpellDamageReport*>

// SYNTHETIC: WIZ8 0x004a5cb0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8SpellDamageReport*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a5cd0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// VTABLE: WIZ8 0x005ec51c
// class W8GrowableVector<unsigned char>

// TEMPLATE: WIZ8 0x0048CD60
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// VTABLE: WIZ8 0x005eca78
// class W8GrowableVector<unsigned short>

// TEMPLATE: WIZ8 0x0048CD80
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// VTABLE: WIZ8 0x005ee7e8
// class W8GrowableVector<W8TargetSource>

// SYNTHETIC: WIZ8 0x00546dc0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8TargetSource>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00546da0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// TEMPLATE: WIZ8 0x00546df0
// NAME: W8GrowableVector<T>::Grow
// RECOMP: W8GrowableVector<W8TargetSource>::Grow

struct W8JournalEntry;

// VTABLE: WIZ8 0x005ee8a0
// class W8GrowableVector<W8JournalEntry>

// SYNTHETIC: WIZ8 0x00558c10
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8JournalEntry>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005be1f0
// NAME: W8GrowableVector<T>::Grow
// RECOMP: W8GrowableVector<W8JournalEntry>::Grow

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8NpcState;
struct W8MessageBoxLine;

// VTABLE: WIZ8 0x005ed890
// class W8GrowableVector<W8MessageBoxLine*>

// TEMPLATE: WIZ8 0x0052a1d0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// VTABLE: WIZ8 0x005ed894
// class W8GrowableVector<int*>

/* The NPC-state line queue at 0x0068C4BC is used by 0x00525FA0.
   Its recovered type and this emission selector are not independent proof
   of the original template argument or translation-unit placement. */
// TEMPLATE: WIZ8 0x0052a1f0
// NAME: W8GrowableVector<T>::InsertAt
// RECOMP: W8GrowableVector<int*>::InsertAt

// SYNTHETIC: WIZ8 0x0052a290
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8MessageBoxLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0052a2c0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<int*>::`scalar deleting destructor'

/* Local Code\Health Stamina Mana.cpp's W8CharacterEventQueue members: the
   queue constructor writes each vector's table in two stages, first
   0x005EE74C - the base table - then the derived 0x005EE748. The deleting
   destructor at 0x0052E570 chains to the destructor body at 0x0052E520,
   which reinstalls the base
   table before freeing data. */
class W8CharacterEvent;

// VTABLE: WIZ8 0x005ee74c
// class W8GrowableVector<W8CharacterEvent*>

// VTABLE: WIZ8 0x005ee748
// class W8Vector<W8CharacterEvent*>

// SYNTHETIC: WIZ8 0x0052e570
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8CharacterEvent*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0052e540
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8CharacterEvent*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0052e520
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// TEMPLATE: WIZ8 0x0052e4d0
// NAME: W8GrowableVector<T>::Remove
// RECOMP: W8GrowableVector<W8CharacterEvent*>::Remove

// VTABLE: WIZ8 0x005ed810
// class W8GrowableVector<W8NpcState*>

// SYNTHETIC: WIZ8 0x0050e510
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8NpcState*>::`scalar deleting destructor'

/* CreateAutomapMarkerSprites constructs the derived W8Vector<srClass*>.
   The base ctor helper at 0x00585460 stamps 0x005EBFB8 and allocates the
   array; the caller installs the derived 0x005EBFB4 table. The complete
   ctor at 0x0042A260 performs both stages. */
// VTABLE: WIZ8 0x005ebfb8
// class W8GrowableVector<srClass*>

// VTABLE: WIZ8 0x005ebfb4
// class W8Vector<srClass*>

// TEMPLATE: WIZ8 0x0042a260
// NAME: W8Vector<T>::W8Vector
// RECOMP: W8Vector<srClass*>::W8Vector

// SYNTHETIC: WIZ8 0x0042a310
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<srClass*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0042a2c0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

/* The base deleting destructor restamps 0x005EBFB8 and frees the array. */
// SYNTHETIC: WIZ8 0x0042a2e0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<srClass*>::`scalar deleting destructor'

/* Direct W8GrowableVector specialization identified by its vtable. */

class stModelInstance;

/* 0x005EC004 is the base W8GrowableVector table: the
   complete-object ctor 0x004CAD80 writes it during setup and 0x005EC018 when
   the object is finished, and the member-construction emission 0x004390F0
   leaves it in place for the enclosing ctor to overwrite. */
// VTABLE: WIZ8 0x005ec004
// class W8GrowableVector<stModelInstance*>

// VTABLE: WIZ8 0x005ec018
// class W8Vector<stModelInstance*>

// TEMPLATE: WIZ8 0x004cad80
// NAME: W8Vector<T>::W8Vector (four-byte capacity)
// RECOMP: ??0?$W8Vector@PAVstModelInstance@@@@QAE@H@Z

// TEMPLATE: WIZ8 0x004390f0
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte capacity)
// RECOMP: ??0?$W8GrowableVector@PAVstModelInstance@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x00438f70
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<stModelInstance*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00438f40
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<stModelInstance*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00438c70
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

/* Local Screens\MGSRadarMap.cpp's g_radar_icon_pools emission: the
   static initializer constructs the eighteen-pool array through this ctor. */

class stModelInstance2D;

// TEMPLATE: WIZ8 0x005a20d0
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<stModelInstance2D*>::W8GrowableVector

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
// NAME: W8Vector<T>::W8Vector<T>
// RECOMP: W8Vector<W8EncounterScriptName*>::W8Vector<W8EncounterScriptName*>

// TEMPLATE: WIZ8 0x00474fb0
// NAME: W8GrowableVector<T>::W8GrowableVector<T>
// RECOMP: W8GrowableVector<W8EncounterScriptName*>::W8GrowableVector<W8EncounterScriptName*>

// SYNTHETIC: WIZ8 0x00446190
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8EncounterScriptName*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00446050
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

/* Direct W8GrowableVector specialization identified by its vtable. AutomapScreenEnter's
   excluded_textures is the lone capacity-constructed instance. */

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Trigger.cpp's g_timed_events. */
// VTABLE: WIZ8 0x005ec16c
// class W8GrowableVector<W8TriggerEvent*>

// TEMPLATE: WIZ8 0x00446090
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8TriggerEvent*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00446230
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8TriggerEvent*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004460f0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Emitted lifecycle bodies belong directly to the template specialization. */

// TEMPLATE: WIZ8 0x00484870
// NAME: W8Vector<T>::W8Vector (four-byte capacity)
// RECOMP: ??0?$W8Vector@PAVstLight@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x00451d10
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<stLight*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451d30
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\3dapi.cpp's g_worlds. */
// VTABLE: WIZ8 0x005ec2b8
// class W8GrowableVector<W8World*>

// TEMPLATE: WIZ8 0x00451a40
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8World*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00451b70
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8World*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451aa0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

class W8Navigator;

// VTABLE: WIZ8 0x005ec324
// class W8GrowableVector<W8Navigator*>

// TEMPLATE: WIZ8 0x00456140
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8Navigator*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004561f0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Navigator*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004561a0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x004561c0
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005EC328)
// RECOMP: W8GrowableVector<W8Navigator*>::`scalar deleting destructor' (companion table 0x005EC328)

/* ConstructWorldCollections builds the W8World collection vectors through
   W8Vector-derived construction tables; each specialization carries a second
   lifecycle table whose scalar deleting destructors and inline
   teardowns are emitted in this unit. */
// SYNTHETIC: WIZ8 0x00451b90
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<Trigger*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451bc0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<Trigger*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451be0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// SYNTHETIC: WIZ8 0x00451c00
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<stParticle*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451c30
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<stParticle*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451c50
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// SYNTHETIC: WIZ8 0x00451c70
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8NamedPosition*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451ca0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8NamedPosition*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451cc0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// SYNTHETIC: WIZ8 0x00451ce0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<stLight*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451d50
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Prop*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451d80
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8Prop*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451da0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// SYNTHETIC: WIZ8 0x00451dc0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<MonGen*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00451df0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<MonGen*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00451e10
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// TEMPLATE: WIZ8 0x00451ae0
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// SYNTHETIC: WIZ8 0x00451b40
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005EC2BC)
// RECOMP: W8GrowableVector<W8World*>::`scalar deleting destructor' (companion table 0x005EC2BC)

// SYNTHETIC: WIZ8 0x00446130
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (Trigger.cpp table 0x005EC0E0)
// RECOMP: W8GrowableVector<int>::`scalar deleting destructor' (Trigger.cpp table 0x005EC0E0)

// SYNTHETIC: WIZ8 0x00446160
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8EncounterScriptName*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004461b0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8WorldItem*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x00446200
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005EC170)
// RECOMP: W8GrowableVector<W8TriggerEvent*>::`scalar deleting destructor' (companion table 0x005EC170)

/* stMeshModel.cpp's mesh-model registry at 0x00659CB8: the destructor at
   0x00470ED0 walks count 0x659CBC / data 0x659CC4 and unlinks the model. */
// VTABLE: WIZ8 0x005ec514
// class W8GrowableVector<stMeshModel*>

// TEMPLATE: WIZ8 0x00474be0
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<stMeshModel*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00474e10
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<stMeshModel*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00474c40
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Engine Code\ReadMesh.cpp's g_retained_materials: the static
   initializer at 0x00485AF0 constructs it with capacity five; the stores
   feed it srMaterialIFace* entries out of the mesh material arrays.
   0x005ECA60 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005eca5c
// class W8GrowableVector<srMaterialIFace*>

// TEMPLATE: WIZ8 0x00489ed0
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<srMaterialIFace*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048a140
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<srMaterialIFace*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048a110
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (construction-phase copy)
// RECOMP: W8GrowableVector<srMaterialIFace*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x00489f30
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* MonGen.cpp's active monster-group list at 0x0065BA10. GenerateEncounter at
   0x0048AD20 stores W8MonsterGroup* elements through g_active_groups. */
// VTABLE: WIZ8 0x005eca98
// class W8GrowableVector<W8MonsterGroup*>

// TEMPLATE: WIZ8 0x0048cda0
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8MonsterGroup*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048cf00
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8MonsterGroup*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048ced0
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005ECA9C)
// RECOMP: W8GrowableVector<W8MonsterGroup*>::`scalar deleting destructor' (companion table 0x005ECA9C)

// TEMPLATE: WIZ8 0x0048ce00
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\MonGen.cpp's g_encounter_tables. */
// VTABLE: WIZ8 0x005ecaa0
// class W8GrowableVector<W8EncounterTableRuntime*>

// TEMPLATE: WIZ8 0x0048ce20
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8EncounterTableRuntime*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048cf50
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8EncounterTableRuntime*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048cf20
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005ECAA4)
// RECOMP: W8GrowableVector<W8EncounterTableRuntime*>::`scalar deleting destructor' (companion table 0x005ECAA4)

// TEMPLATE: WIZ8 0x0048CF70
// NAME: W8GrowableVector<T>::Grow (shared two-byte storage; signedness unresolved)
// RECOMP: W8GrowableVector::Grow (shared two-byte storage; signedness unresolved)

// TEMPLATE: WIZ8 0x0048CFD0
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte copy)
// RECOMP: ??0?$W8GrowableVector@PAUMonGen@@@@QAE@ABV0@@Z

// TEMPLATE: WIZ8 0x0048ce80
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Engine Code\stCube.cpp's g_world_cursor_nodes: the static
   initializer at 0x0048D020 constructs it with capacity five; the table
   holds the world's W8WorldCursorNode* cursor nodes.
   0x005ECAD4 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ecad0
// class W8GrowableVector<W8WorldCursorNode*>

// TEMPLATE: WIZ8 0x0048f190
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8WorldCursorNode*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x0048f240
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8WorldCursorNode*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0048f210
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (construction-phase copy)
// RECOMP: W8GrowableVector<W8WorldCursorNode*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x0048f1f0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* The W8MasterFunction (void (*)(int)) pointer-vector specialization emitted
   by MasterFunctionList.cpp. InitializeLevelMasterFunctions's
   five-element construction calls the base ctor 0x004D9A70 and then installs
   the derived vtable 0x005ED438 itself, so the retail new-expression is
   `new W8Vector<W8MasterFunction>(5)` and g_master_functions is the
   thin derived type. The base vtable 0x005ED43C also tags the
   DialogFactoryDialogs.cpp member embedded at +0x64, which that unit only
   constructs, clears and destroys. No other specialization shares either
   vtable. */

// VTABLE: WIZ8 0x005ed438
// class W8Vector<W8MasterFunction>

// VTABLE: WIZ8 0x005ed43c
// class W8GrowableVector<W8MasterFunction>

// TEMPLATE: WIZ8 0x004d9a70
// NAME: W8GrowableVector<T>::W8GrowableVector<T> (MasterFunctionList.cpp emission)
// RECOMP: W8GrowableVector<W8MasterFunction>::W8GrowableVector<W8MasterFunction> (MasterFunctionList.cpp emission)

// SYNTHETIC: WIZ8 0x004d9a20
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8MasterFunction>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004d9a40
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8MasterFunction>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005cd6e0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Emitted inside the DialogFactoryDialogs.cpp span by the listbox dialog's
   destruction path; stores table 0x005EE9FC, the W8GrowableVector<unsigned short*>
   (wchar_t* keyword list) specialization's table. */
// TEMPLATE: WIZ8 0x005cd6c0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* The emitted lifecycle and both reviewed vtables belong directly to the
   ordinary light-vector template specializations. */

// VTABLE: WIZ8 0x005ec298
// class W8GrowableVector<stLight*>

// VTABLE: WIZ8 0x005ec294
// class W8Vector<stLight*>

// VTABLE: WIZ8 0x005ece60
// class W8GrowableVector<W8GrowableVector<stLight*>*>

// TEMPLATE: WIZ8 0x004a5c30
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8GrowableVector<stLight*>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004a5d20
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8GrowableVector<stLight*>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a5c90
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\GrCycle.cpp's g_grcycles_by_name. */
// VTABLE: WIZ8 0x005ecee4
// class W8GrowableVector<W8GrowableVector<W8GrCycle*>*>

// SYNTHETIC: WIZ8 0x004a8ef0
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (companion table 0x005ECEE8)
// RECOMP: W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::`scalar deleting destructor' (companion table 0x005ECEE8)

// TEMPLATE: WIZ8 0x004a8e70
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004a8f20
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8GrowableVector<W8GrCycle*>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a8ed0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Emitted W8GrowableVector instantiation identified by its vtable. */

// VTABLE: WIZ8 0x005ecf04
// class W8GrowableVector<srVector3T<float>*>

// VTABLE: WIZ8 0x005ecf00
// class W8Vector<srVector3T<float>*>

// SYNTHETIC: WIZ8 0x004aaaf0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<srVector3T<float>*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004aab10
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// TEMPLATE: WIZ8 0x004aab30
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<srVector3T<float>*>::W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Spells.cpp's g_sound3d_instances. */
// VTABLE: WIZ8 0x005ed018
// class W8GrowableVector<stSound3D*>

// TEMPLATE: WIZ8 0x004af690
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<stSound3D*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004af740
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<stSound3D*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004af6f0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

class srClipPlane;

// VTABLE: WIZ8 0x005ed1b8
// class W8GrowableVector<srClientSupport<srClipPlane,5376>*>

// TEMPLATE: WIZ8 0x00585340
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<srClientSupport<srClipPlane,5376>*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004be030
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<srClientSupport<srClipPlane,5376>*>::`scalar deleting destructor'

/* Second emission of the same specialization's ctor (0x004BE050 sits beside
   this unit's other clip-plane vector emissions). */
// TEMPLATE: WIZ8 0x004be050
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<srClientSupport<srClipPlane,5376>*>::W8GrowableVector

// TEMPLATE: WIZ8 0x004bdfe0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Engine Code\Monster.cpp's W8GrowableVector<W8AnimObj*> emission: the 0x1B
   elements at W8MonsterRep+0xAC (the per-cycle animations array) are built
   through ??_L with the capacity-five ctor thunk at 0x004BEBC0.
   0x005ED2CC is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ed2c8
// class W8GrowableVector<W8AnimObj*>

// TEMPLATE: WIZ8 0x004cad00
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8AnimObj*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x004cae10
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8AnimObj*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004cade0
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (construction-phase copy)
// RECOMP: W8GrowableVector<W8AnimObj*>::`scalar deleting destructor' (construction-phase copy)

// TEMPLATE: WIZ8 0x004cad60
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Magic.cpp's g_spell_effects. */
// VTABLE: WIZ8 0x005ed7d4
// class W8GrowableVector<W8SpellEffectEntry*>

// TEMPLATE: WIZ8 0x00501eb0
// NAME: W8GrowableVector<T>::W8GrowableVector
// RECOMP: W8GrowableVector<W8SpellEffectEntry*>::W8GrowableVector

// SYNTHETIC: WIZ8 0x00501f60
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8SpellEffectEntry*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00501f10
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// TEMPLATE: WIZ8 0x00501f80
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte capacity)
// RECOMP: ??0?$W8GrowableVector@PAVW8SpellVisual@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x00501fd0
// Record constructor (reports vector at +0x5A; enclosing record type unresolved)

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Search.cpp's g_searchables. */
// VTABLE: WIZ8 0x005ed844
// class W8GrowableVector<W8Searchable*>

// VTABLE: WIZ8 0x005ed840
// class W8Vector<W8Searchable*>

// TEMPLATE: WIZ8 0x00517810
// NAME: W8Vector<T>::W8Vector (four-byte capacity)
// RECOMP: ??0?$W8Vector@PAUW8Searchable@@@@QAE@H@Z

// SYNTHETIC: WIZ8 0x005178c0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8Searchable*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00517870
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

/* Engine Code\3dapi.cpp's ConstructWorldCollections00450B10 emission span:
   each `new W8GrowableVector<T*>` member there instantiates the
   specialization's destructor bodies in this neighbourhood, and each dtor
   restamps the construction-phase table the member's ctor wrote first. */

// VTABLE: WIZ8 0x005ec278
// class W8GrowableVector<Trigger*>

// VTABLE: WIZ8 0x005ec274
// class W8Vector<Trigger*>

// VTABLE: WIZ8 0x005ec270
// class W8GrowableVector<stParticle*>

// VTABLE: WIZ8 0x005ec26c
// class W8Vector<stParticle*>

// VTABLE: WIZ8 0x005ec268
// class W8GrowableVector<W8NamedPosition*>

// VTABLE: WIZ8 0x005ec264
// class W8Vector<W8NamedPosition*>

// VTABLE: WIZ8 0x005ec290
// class W8GrowableVector<W8Prop*>

// VTABLE: WIZ8 0x005ec28c
// class W8Vector<W8Prop*>

// VTABLE: WIZ8 0x005ec288
// class W8GrowableVector<MonGen*>

// VTABLE: WIZ8 0x005ec284
// class W8Vector<MonGen*>

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8AutomapNote;

// VTABLE: WIZ8 0x005eea2c
// class W8GrowableVector<W8AutomapNote*>

// VTABLE: WIZ8 0x005eea28
// class W8Vector<W8AutomapNote*>

// TEMPLATE: WIZ8 0x005853A0
// NAME: W8HashTable<Key,Value>::Lookup
// RECOMP: W8HashTable<unsigned int,int>::Lookup

// SYNTHETIC: WIZ8 0x00585420
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8AutomapNote*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00585440
// NAME: W8Vector<T>::~W8Vector
// RECOMP: W8Vector::~W8Vector

// TEMPLATE: WIZ8 0x00585460
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte capacity)
// RECOMP: ??0?$W8GrowableVector@PAVsrClass@@@@QAE@H@Z

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
// NAME: W8GrowableVector<T>::W8GrowableVector (OptionsScreen.cpp emission)
// RECOMP: W8GrowableVector<W8OptionsSaveRow*>::W8GrowableVector (OptionsScreen.cpp emission)

// TEMPLATE: WIZ8 0x005acfc0
// NAME: W8Vector<T>::W8Vector
// RECOMP: W8Vector<W8OptionsSaveRow*>::W8Vector

// TEMPLATE: WIZ8 0x005ad020
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005ad180
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8OptionsSaveRow*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad1b0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8OptionsSaveRow*>::`scalar deleting destructor'

/* W8OptionsPanelSet::m_panels_010 is a W8Vector<W8OptionsPanel*>: the derived
   0x005EF01C table over base 0x005EEFE0. */
// VTABLE: WIZ8 0x005eefe0
// class W8GrowableVector<W8OptionsPanel*>

// VTABLE: WIZ8 0x005ef01c
// class W8Vector<W8OptionsPanel*>

// TEMPLATE: WIZ8 0x005ad1d0
// NAME: W8GrowableVector<T>::W8GrowableVector (OptionsScreen.cpp emission)
// RECOMP: W8GrowableVector<W8OptionsPanel*>::W8GrowableVector (OptionsScreen.cpp emission)

// TEMPLATE: WIZ8 0x005acf80
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005ad0e0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8OptionsPanel*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad110
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8OptionsPanel*>::`scalar deleting destructor'

/* W8OptionsPanel::m_text_buffers_058 is a W8Vector<W8TextBuffer*>: the derived
   0x005EEFCC table over base 0x005EEFD0. */
// VTABLE: WIZ8 0x005eefd0
// class W8GrowableVector<W8TextBuffer*>

// VTABLE: WIZ8 0x005eefcc
// class W8Vector<W8TextBuffer*>

// TEMPLATE: WIZ8 0x005acf40
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005ad040
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8TextBuffer*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad070
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8TextBuffer*>::`scalar deleting destructor'

/* W8OptionsPanel::m_option_selections is a W8Vector<W8OptionsSelection*>: the
   derived 0x005EEFC4 table over base 0x005EEFC8. */
// VTABLE: WIZ8 0x005eefc8
// class W8GrowableVector<W8OptionsSelection*>

// VTABLE: WIZ8 0x005eefc4
// class W8Vector<W8OptionsSelection*>

// TEMPLATE: WIZ8 0x005acf60
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005ad090
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8OptionsSelection*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad0c0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8OptionsSelection*>::`scalar deleting destructor'

/* W8OptionsScreen::m_save_slots is a W8Vector<W8SaveSlot*>: the derived
   0x005EF00C table over base 0x005EF010. */
// VTABLE: WIZ8 0x005ef010
// class W8GrowableVector<W8SaveSlot*>

// VTABLE: WIZ8 0x005ef00c
// class W8Vector<W8SaveSlot*>

// TEMPLATE: WIZ8 0x005acfa0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005ad130
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8SaveSlot*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005ad160
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8SaveSlot*>::`scalar deleting destructor'

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
// NAME: W8GrowableVector<T>::W8GrowableVector (four-byte capacity)
// RECOMP: ??0?$W8GrowableVector@PAUW8Character@@@@QAE@H@Z

// TEMPLATE: WIZ8 0x005c34b0
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x005c3740
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Character*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x005c3770
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8Character*>::`scalar deleting destructor'

/* W8CharacterPage's member vector stamps the base table at 0x005AFDD3,
   then this derived table at 0x005AFDFC. */
// VTABLE: WIZ8 0x005ef214 W8Vector<W8CharacterPageEntry*>
// class W8Vector<W8CharacterPageEntry*>

// SYNTHETIC: WIZ8 0x005b1bc0
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<W8CharacterPageEntry*>::`scalar deleting destructor'

/* Local Screens\CreditsScreen.cpp's g_credit_lines: the enter path
   news the vector and the element constructor allocates five 0x14-byte
   W8CreditLine slots. */
// VTABLE: WIZ8 0x005ef310
// class W8GrowableVector<W8CreditLine>

// SYNTHETIC: WIZ8 0x005bc7d0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8CreditLine>::`scalar deleting destructor'

/* Trigger.cpp span (Trigger.cpp -> OctBuildTree.cpp gap). The 0x005EC160
   table's element type is unresolved; the other emissions re-instantiate
   already-reviewed specializations. */
// VTABLE: WIZ8 0x005ec160
// class W8GrowableVector<W8WorldItem*>

/* Prop.cpp span. */
// VTABLE: WIZ8 0x005ec1d4
// class W8GrowableVector<W8PropAnimationSegment*>

/* stTextureAnim's constructor installs this base table at 0x00484D0C,
   then the derived table at 0x00484D2B. AddTexture stores srTextureIFace
   objects whose virtual texture operations the frame accessors invoke. */
// VTABLE: WIZ8 0x005ec9bc
// class W8GrowableVector<srTextureIFace*>

// VTABLE: WIZ8 0x005ec9b8
// class W8Vector<srTextureIFace*>

/* ReadMesh.cpp span. 0x005ECA58 is used by BuildSingleLevelMesh's local
   W8GrowableVector<srShader> and by the derived material pointer vector. The
   identical one-slot tables are folded; the address does not identify one
   source specialization. 0x005ECA5C is the material vector's base table. */
// VTABLE: WIZ8 0x005eca58
// class W8Vector<srMaterialIFace*>

/* Missile.cpp span. */
// SYNTHETIC: WIZ8 0x004A5CF0
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (Missile.cpp emission)
// RECOMP: W8GrowableVector<W8GrowableVector<stLight*>*>::`scalar deleting destructor' (Missile.cpp emission)

// SYNTHETIC: WIZ8 0x004A5DB0
// W8SpellEffectResult::~W8SpellEffectResult (reports member at +0x58)

/* GrCycle.cpp span. 0x005ECED0 is the particle-attachment base table;
   0x005ECED4 is the derived shake-effect table. */
// VTABLE: WIZ8 0x005eced0
// class W8GrowableVector<W8GrCycleParticleAttachment*>

// VTABLE: WIZ8 0x005ecee0
// class W8Vector<W8GrowableVector<W8GrCycle*>*>

/* PathAI.CPP -> Spells.cpp gap. */
// SYNTHETIC: WIZ8 0x004AAAC0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<srVector3T<float>*>::`scalar deleting destructor'

/* Spells.cpp span. */
// SYNTHETIC: WIZ8 0x004AF710
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (Spells.cpp emission)
// RECOMP: W8GrowableVector<stSound3D*>::`scalar deleting destructor' (Spells.cpp emission)

/* AnimObj.cpp span. */
// TEMPLATE: WIZ8 0x004A2060
// NAME: W8GrowableVector<T>::~W8GrowableVector (array free)
// RECOMP: ??1?$W8GrowableVector@M@@UAE@XZ

// SYNTHETIC: WIZ8 0x004A20C0 SYMBOL
// NAME: W8GrowableVector<T>::retained emission
// RECOMP: ??_G?$W8GrowableVector@M@@UAEPAXI@Z

/* Monster.cpp span. */
// TEMPLATE: WIZ8 0x004CACE0
// NAME: W8GrowableVector<T>::~W8GrowableVector (Monster +0x29C owner; element spelling unresolved)
// RECOMP: W8GrowableVector::~W8GrowableVector (Monster +0x29C owner; element spelling unresolved)

/* stScript's constructor installs the base and derived tables at +0x18
   for lines and +0x28 for labels. Load's record allocations and member
   accesses distinguish the element types independently of these tables. */
// VTABLE: WIZ8 0x005ed354
// class W8GrowableVector<stScriptLine*>

// VTABLE: WIZ8 0x005ed350
// class W8Vector<stScriptLine*>

// VTABLE: WIZ8 0x005ed34c
// class W8GrowableVector<stScriptLabel*>

// VTABLE: WIZ8 0x005ed348
// class W8Vector<stScriptLabel*>

// TEMPLATE: WIZ8 0x004CFA00
// NAME: W8GrowableVector<T>::~W8GrowableVector for the 0x005ED354 table (stScript.cpp emission)
// RECOMP: W8GrowableVector<stScriptLine*>::~W8GrowableVector (stScript.cpp emission)

// TEMPLATE: WIZ8 0x004CFA20
// NAME: W8GrowableVector<T>::~W8GrowableVector for the 0x005ED34C table (stScript.cpp emission)
// RECOMP: W8GrowableVector<stScriptLabel*>::~W8GrowableVector (stScript.cpp emission)

// SYNTHETIC: WIZ8 0x004CFA40
// W8GrowableVector<stScriptLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004CFA70
// W8Vector<stScriptLine*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004CFA90
// W8GrowableVector<stScriptLabel*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004CFAC0
// W8Vector<stScriptLabel*>::`scalar deleting destructor'

/* Controls.cpp -> ItemManager.cpp gap. */
// VTABLE: WIZ8 0x005ed5b4
// class W8GrowableVector<W8Widget*>

// VTABLE: WIZ8 0x005ed660
// class W8GrowableVector<W8TextControl*>

// SYNTHETIC: WIZ8 0x004F6870
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Widget*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004F68E0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8TextControl*>::`scalar deleting destructor'

/* ItemManager.cpp span. */

/* Magic.cpp span. */
// SYNTHETIC: WIZ8 0x00501F30
// NAME: W8GrowableVector<T>::`scalar deleting destructor' (Magic.cpp emission)
// RECOMP: W8GrowableVector<W8SpellEffectEntry*>::`scalar deleting destructor' (Magic.cpp emission)

/* NPC Manager.cpp span. */

/* LoadSaveGame.cpp span: removes the indexed element, shift-fills the slot
   and destroys the removed entry. */
// TEMPLATE: WIZ8 0x00516A00
// NAME: W8GrowableVector<T> erase-and-delete emission (LoadSaveGame.cpp span)
// RECOMP: W8GrowableVector<W8SpellEffectEntry*> erase-and-delete emission (LoadSaveGame.cpp span)

/* search.cpp span. */
// SYNTHETIC: WIZ8 0x00517890
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8Searchable*>::`scalar deleting destructor'

/* chunk.cpp span. */
// VTABLE: WIZ8 0x005ee8cc
// class W8GrowableVector<W8ChunkHead*>

// SYNTHETIC: WIZ8 0x0055CBB0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8ChunkHead*>::`scalar deleting destructor'

/* MGSKeyboard's binding vector stamps the base table at 0x0055CFF4,
   then the derived table at 0x0055D01B. */
// VTABLE: WIZ8 0x005ee8f8 W8GrowableVector<MGSKeyBinding*>
// class W8GrowableVector<MGSKeyBinding*>

// VTABLE: WIZ8 0x005ee8f4 W8Vector<MGSKeyBinding*>
// class W8Vector<MGSKeyBinding*>

// TEMPLATE: WIZ8 0x0055DB60
// NAME: W8GrowableVector<T>::~W8GrowableVector
// RECOMP: W8GrowableVector::~W8GrowableVector

// SYNTHETIC: WIZ8 0x0055DDF0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<MGSKeyBinding*>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0055DE20
// NAME: W8Vector<T>::`scalar deleting destructor'
// RECOMP: W8Vector<MGSKeyBinding*>::`scalar deleting destructor'

/* AutomapScreen.cpp span. */
// SYNTHETIC: WIZ8 0x005853F0
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8AutomapNote*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x0047D290
// intrusive-list head init emission

/* Cursor3d.cpp -> stParticle.cpp gap. */

/* AnimObj.cpp -> Missile.cpp gap. */

/* ReadLevel.cpp -> quad.cpp gap. */
// SYNTHETIC: WIZ8 0x004BE000
// NAME: W8GrowableVector<T>::`scalar deleting destructor'
// RECOMP: W8GrowableVector<W8VectorElement005ED1B8*>::`scalar deleting destructor'

/* Monster.cpp -> OctPrePath.cpp gap. */

/* Combat.cpp -> GameplayCode.cpp gap. */

/* InputMapper.cpp -> Screens.cpp gap. */

/* AutomapScreen.cpp -> MGSTextBox.cpp gap. */
