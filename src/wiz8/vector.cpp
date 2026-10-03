#include "wiz8/vector.h"

#include "wiz8/local_code/SpellEffect.h"

#include <stdlib.h>
#include <string.h>

/* Concrete names here are retained recomp pairing selectors unless supported
   by independent retail symbols or typed element flow. Folded storage helpers
   and construction tables alone do not establish exact template arguments. */

// VTABLE: WIZ8 0x005ebfe0
// class W8GrowableVector<int>

/* CastSpellFromSource installs 0x005EC280 after the base constructor. The
   derived destructor at 0x00451AE0 restores 0x005EBFE4 before deleting the
   array. */
// VTABLE: WIZ8 0x005ebfe4
// class W8GrowableVector<W8SpellVisual*>

// VTABLE: WIZ8 0x005ec280
// class W8Vector<W8SpellVisual*>

// VTABLE: WIZ8 0x005ee8c8
// class W8Vector<W8ChunkHead*>

// VTABLE: WIZ8 0x005ec15c
// class W8Vector<W8WorldItem*>

class W8Missile;

/* 0x005EBFE8 is the base W8GrowableVector table; the derived W8Vector
   table is 0x005EC27C, which CastSpellFromSource 0x004FB4C0 writes into
   W8SpellEffectEntry::missiles and ConstructWorldCollections writes into
   W8World::missiles. */
// VTABLE: WIZ8 0x005ebfe8
// class W8GrowableVector<W8Missile*>

// VTABLE: WIZ8 0x005ec27c
// class W8Vector<W8Missile*>

struct W8SpellDamageReport;

// VTABLE: WIZ8 0x005ebfec
// class W8GrowableVector<W8SpellDamageReport*>

/* The capacity-five ctor the W8SpellEffectResult::reports member calls:
   every construction site emits PUSH 5 before this out-of-line emission. */

/* The derived vftable every W8SpellDamageReport* reports member takes at
   construction: one slot, the derived scalar deleting destructor. */
// VTABLE: WIZ8 0x005ece4c
// class W8Vector<W8SpellDamageReport*>

// VTABLE: WIZ8 0x005ec51c
// class W8GrowableVector<unsigned char>

// VTABLE: WIZ8 0x005eca78
// class W8GrowableVector<unsigned short>

// VTABLE: WIZ8 0x005ee7e8
// class W8GrowableVector<W8TargetSource>

struct W8JournalEntry;

// VTABLE: WIZ8 0x005ee8a0
// class W8GrowableVector<W8JournalEntry>

/* Direct W8GrowableVector specialization identified by its vtable. */

struct W8NpcState;
struct W8MessageBoxLine;

// VTABLE: WIZ8 0x005ed890
// class W8GrowableVector<W8MessageBoxLine*>

// VTABLE: WIZ8 0x005ed894
// class W8GrowableVector<int*>

/* The NPC-state line queue at 0x0068C4BC is used by 0x00525FA0.
   Its recovered type and this emission selector are not independent proof
   of the original template argument or translation-unit placement. */

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

// VTABLE: WIZ8 0x005ed810
// class W8GrowableVector<W8NpcState*>

/* CreateAutomapMarkerSprites constructs the derived W8Vector<srClass*>.
   The base ctor helper at 0x00585460 stamps 0x005EBFB8 and allocates the
   array; the caller installs the derived 0x005EBFB4 table. The complete
   ctor at 0x0042A260 performs both stages. */
// VTABLE: WIZ8 0x005ebfb8
// class W8GrowableVector<srClass*>

// VTABLE: WIZ8 0x005ebfb4
// class W8Vector<srClass*>

/* The base deleting destructor restamps 0x005EBFB8 and frees the array. */

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

/* Local Screens\MGSRadarMap.cpp's g_radar_icon_pools emission: the
   static initializer constructs the eighteen-pool array through this ctor. */

class stModelInstance2D;

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

/* Direct W8GrowableVector specialization identified by its vtable. AutomapScreenEnter's
   excluded_textures is the lone capacity-constructed instance. */

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Trigger.cpp's g_timed_events. */
// VTABLE: WIZ8 0x005ec16c
// class W8GrowableVector<W8TriggerEvent*>

/* Emitted lifecycle bodies belong directly to the template specialization. */

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\3dapi.cpp's g_worlds. */
// VTABLE: WIZ8 0x005ec2b8
// class W8GrowableVector<W8World*>

/* Direct W8GrowableVector specialization identified by its vtable. */

class W8Navigator;

// VTABLE: WIZ8 0x005ec324
// class W8GrowableVector<W8Navigator*>

/* ConstructWorldCollections builds the W8World collection vectors through
   W8Vector-derived construction tables; each specialization carries a second
   lifecycle table whose scalar deleting destructors and inline
   teardowns are emitted in this unit. */

/* stMeshModel.cpp's mesh-model registry at 0x00659CB8: the destructor at
   0x00470ED0 walks count 0x659CBC / data 0x659CC4 and unlinks the model. */
// VTABLE: WIZ8 0x005ec514
// class W8GrowableVector<stMeshModel*>

/* Engine Code\ReadMesh.cpp's g_retained_materials: the static
   initializer at 0x00485AF0 constructs it with capacity five; the stores
   feed it srMaterialIFace* entries out of the mesh material arrays.
   0x005ECA60 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005eca5c
// class W8GrowableVector<srMaterialIFace*>

/* MonGen.cpp's active monster-group list at 0x0065BA10. GenerateEncounter at
   0x0048AD20 stores W8MonsterGroup* elements through g_active_groups. */
// VTABLE: WIZ8 0x005eca98
// class W8GrowableVector<W8MonsterGroup*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\MonGen.cpp's g_encounter_tables. */
// VTABLE: WIZ8 0x005ecaa0
// class W8GrowableVector<W8EncounterTableRuntime*>

/* Engine Code\stCube.cpp's g_world_cursor_nodes: the static
   initializer at 0x0048D020 constructs it with capacity five; the table
   holds the world's W8WorldCursorNode* cursor nodes.
   0x005ECAD4 is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ecad0
// class W8GrowableVector<W8WorldCursorNode*>

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

/* Emitted inside the DialogFactoryDialogs.cpp span by the listbox dialog's
   destruction path; stores table 0x005EE9FC, the W8GrowableVector<unsigned short*>
   (wchar_t* keyword list) specialization's table. */

/* The emitted lifecycle and both reviewed vtables belong directly to the
   ordinary light-vector template specializations. */

// VTABLE: WIZ8 0x005ec298
// class W8GrowableVector<stLight*>

// VTABLE: WIZ8 0x005ec294
// class W8Vector<stLight*>

// VTABLE: WIZ8 0x005ece60
// class W8GrowableVector<W8GrowableVector<stLight*>*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\GrCycle.cpp's g_grcycles_by_name. */
// VTABLE: WIZ8 0x005ecee4
// class W8GrowableVector<W8GrowableVector<W8GrCycle*>*>

/* Emitted W8GrowableVector instantiation identified by its vtable. */

// VTABLE: WIZ8 0x005ecf04
// class W8GrowableVector<srVector3T<float>*>

// VTABLE: WIZ8 0x005ecf00
// class W8Vector<srVector3T<float>*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Engine Code\Spells.cpp's g_sound3d_instances. */
// VTABLE: WIZ8 0x005ed018
// class W8GrowableVector<stSound3D*>

/* Direct W8GrowableVector specialization identified by its vtable. */

class srClipPlane;

// VTABLE: WIZ8 0x005ed1b8
// class W8GrowableVector<srClientSupport<srClipPlane,5376>*>

/* Second emission of the same specialization's ctor (0x004BE050 sits beside
   this unit's other clip-plane vector emissions). */

/* Engine Code\Monster.cpp's W8GrowableVector<W8AnimObj*> emission: the 0x1B
   elements at W8MonsterRep+0xAC (the per-cycle animations array) are built
   through ??_L with the capacity-five ctor thunk at 0x004BEBC0.
   0x005ED2CC is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ed2c8
// class W8GrowableVector<W8AnimObj*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Magic.cpp's g_spell_effects. */
// VTABLE: WIZ8 0x005ed7d4
// class W8GrowableVector<W8SpellEffectEntry*>

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Code\Search.cpp's g_searchables. */
// VTABLE: WIZ8 0x005ed844
// class W8GrowableVector<W8Searchable*>

// VTABLE: WIZ8 0x005ed840
// class W8Vector<W8Searchable*>

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

/* Direct W8GrowableVector specialization identified by its vtable. */

/* Local Screens\OptionsScreen.cpp's W8Vector<W8OptionsSaveRow*> emission:
   W8OptionsSaveLoadPanel::m_rows at +0x90. The derived ctor emission at
   0x005ACFC0 stamps 0x005EF08C over the 0x005EF190 construction-phase table;
   the two deleting destructors are the derived and base copies. */
// VTABLE: WIZ8 0x005ef190
// class W8GrowableVector<W8OptionsSaveRow*>

// VTABLE: WIZ8 0x005ef08c
// class W8Vector<W8OptionsSaveRow*>

/* W8OptionsPanelSet::m_panels_010 is a W8Vector<W8OptionsPanel*>: the derived
   0x005EF01C table over base 0x005EEFE0. */
// VTABLE: WIZ8 0x005eefe0
// class W8GrowableVector<W8OptionsPanel*>

// VTABLE: WIZ8 0x005ef01c
// class W8Vector<W8OptionsPanel*>

/* W8OptionsPanel::m_text_buffers_058 is a W8Vector<W8TextBuffer*>: the derived
   0x005EEFCC table over base 0x005EEFD0. */
// VTABLE: WIZ8 0x005eefd0
// class W8GrowableVector<W8TextBuffer*>

// VTABLE: WIZ8 0x005eefcc
// class W8Vector<W8TextBuffer*>

/* W8OptionsPanel::m_option_selections is a W8Vector<W8OptionsSelection*>: the
   derived 0x005EEFC4 table over base 0x005EEFC8. */
// VTABLE: WIZ8 0x005eefc8
// class W8GrowableVector<W8OptionsSelection*>

// VTABLE: WIZ8 0x005eefc4
// class W8Vector<W8OptionsSelection*>

/* W8OptionsScreen::m_save_slots is a W8Vector<W8SaveSlot*>: the derived
   0x005EF00C table over base 0x005EF010. */
// VTABLE: WIZ8 0x005ef010
// class W8GrowableVector<W8SaveSlot*>

// VTABLE: WIZ8 0x005ef00c
// class W8Vector<W8SaveSlot*>

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

/* W8CharacterPage's member vector stamps the base table at 0x005AFDD3,
   then this derived table at 0x005AFDFC. */
// VTABLE: WIZ8 0x005ef214 W8Vector<W8CharacterPageEntry*>
// class W8Vector<W8CharacterPageEntry*>

/* Local Screens\CreditsScreen.cpp's g_credit_lines: the enter path
   news the vector and the element constructor allocates five 0x14-byte
   W8CreditLine slots. */
// VTABLE: WIZ8 0x005ef310
// class W8GrowableVector<W8CreditLine>

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

/* GrCycle.cpp span. 0x005ECED0 is the particle-attachment base table;
   0x005ECED4 is the derived shake-effect table. */
// VTABLE: WIZ8 0x005eced0
// class W8GrowableVector<W8GrCycleParticleAttachment*>

// VTABLE: WIZ8 0x005ecee0
// class W8Vector<W8GrowableVector<W8GrCycle*>*>

/* PathAI.CPP -> Spells.cpp gap. */

/* Spells.cpp span. */

/* AnimObj.cpp span. */

/* Monster.cpp span. */

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

/* Controls.cpp -> ItemManager.cpp gap. */
// VTABLE: WIZ8 0x005ed5b4
// class W8GrowableVector<W8Widget*>

// VTABLE: WIZ8 0x005ed660
// class W8GrowableVector<W8TextControl*>

/* ItemManager.cpp span. */

/* Magic.cpp span. */

/* NPC Manager.cpp span. */

/* LoadSaveGame.cpp span: removes the indexed element, shift-fills the slot
   and destroys the removed entry. */

/* search.cpp span. */

/* chunk.cpp span. */
// VTABLE: WIZ8 0x005ee8cc
// class W8GrowableVector<W8ChunkHead*>

/* MGSKeyboard's binding vector stamps the base table at 0x0055CFF4,
   then the derived table at 0x0055D01B. */
// VTABLE: WIZ8 0x005ee8f8 W8GrowableVector<MGSKeyBinding*>
// class W8GrowableVector<MGSKeyBinding*>

// VTABLE: WIZ8 0x005ee8f4 W8Vector<MGSKeyBinding*>
// class W8Vector<MGSKeyBinding*>

/* AutomapScreen.cpp span. */

/* Cursor3d.cpp -> stParticle.cpp gap. */

/* AnimObj.cpp -> Missile.cpp gap. */

/* ReadLevel.cpp -> quad.cpp gap. */

/* Monster.cpp -> OctPrePath.cpp gap. */

/* Combat.cpp -> GameplayCode.cpp gap. */

/* InputMapper.cpp -> Screens.cpp gap. */

/* AutomapScreen.cpp -> MGSTextBox.cpp gap. */
