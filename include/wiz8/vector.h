#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

#include <new>

/* Retained vector vtables. These identify binary tables; folded tables and
   emitted lifecycle bodies alone do not establish original template arguments.
   The template declarations below remain the canonical source owner. */
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

/* 0x005EBFE8 is the base W8GrowableVector table; the derived W8Vector
   table is 0x005EC27C, which CastSpellFromSource 0x004FB4C0 writes into
   W8SpellEffectEntry::missiles and ConstructWorldCollections writes into
   W8World::missiles. */
// VTABLE: WIZ8 0x005ebfe8
// class W8GrowableVector<W8Missile*>

// VTABLE: WIZ8 0x005ec27c
// class W8Vector<W8Missile*>

// VTABLE: WIZ8 0x005ebfec
// class W8GrowableVector<W8SpellDamageReport*>

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

// VTABLE: WIZ8 0x005ee8a0
// class W8GrowableVector<W8JournalEntry>

// VTABLE: WIZ8 0x005ed890
// class W8GrowableVector<W8MessageBoxLine*>

// VTABLE: WIZ8 0x005ed894
// class W8GrowableVector<int*>

/* Local Code\Health Stamina Mana.cpp's W8CharacterEventQueue members: the
   queue constructor writes each vector's table in two stages, first
   0x005EE74C - the base table - then the derived 0x005EE748. The deleting
   destructor at 0x0052E570 chains to the destructor body at 0x0052E520,
   which reinstalls the base
   table before freeing data. */
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

/* 0x005EC004 is the base W8GrowableVector table: the
   complete-object ctor 0x004CAD80 writes it during setup and 0x005EC018 when
   the object is finished, and the member-construction emission 0x004390F0
   leaves it in place for the enclosing ctor to overwrite. */
// VTABLE: WIZ8 0x005ec004
// class W8GrowableVector<stModelInstance*>

// VTABLE: WIZ8 0x005ec018
// class W8Vector<stModelInstance*>

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

// VTABLE: WIZ8 0x005ec16c
// class W8GrowableVector<W8TriggerEvent*>

// VTABLE: WIZ8 0x005ec2b8
// class W8GrowableVector<W8World*>

// VTABLE: WIZ8 0x005ec324
// class W8GrowableVector<W8Navigator*>

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

// VTABLE: WIZ8 0x005ec298
// class W8GrowableVector<stLight*>

// VTABLE: WIZ8 0x005ec294
// class W8Vector<stLight*>

// VTABLE: WIZ8 0x005ece60
// class W8GrowableVector<W8GrowableVector<stLight*>*>

// VTABLE: WIZ8 0x005ecee4
// class W8GrowableVector<W8GrowableVector<W8GrCycle*>*>

// VTABLE: WIZ8 0x005ecf04
// class W8GrowableVector<srVector3T<float>*>

// VTABLE: WIZ8 0x005ecf00
// class W8Vector<srVector3T<float>*>

// VTABLE: WIZ8 0x005ed018
// class W8GrowableVector<stSound3D*>

// VTABLE: WIZ8 0x005ed1b8
// class W8GrowableVector<srClientSupport<srClipPlane,5376>*>

/* Engine Code\Monster.cpp's W8GrowableVector<W8AnimObj*> emission: the 0x1B
   elements at W8MonsterRep+0xAC (the per-cycle animations array) are built
   through ??_L with the capacity-five ctor thunk at 0x004BEBC0.
   0x005ED2CC is this specialization's construction-phase table. */
// VTABLE: WIZ8 0x005ed2c8
// class W8GrowableVector<W8AnimObj*>

// VTABLE: WIZ8 0x005ed7d4
// class W8GrowableVector<W8SpellEffectEntry*>

// VTABLE: WIZ8 0x005ed844
// class W8GrowableVector<W8Searchable*>

// VTABLE: WIZ8 0x005ed840
// class W8Vector<W8Searchable*>

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

// VTABLE: WIZ8 0x005eea2c
// class W8GrowableVector<W8AutomapNote*>

// VTABLE: WIZ8 0x005eea28
// class W8Vector<W8AutomapNote*>

/* Local Screens\OptionsScreen.cpp's W8Vector<W8OptionsSaveRow*> emission:
   W8OptionsSaveLoadPanel::m_rows at +0x90. The derived ctor emission at
   0x005ACFC0 stamps 0x005EF08C over the 0x005EF190 construction-phase table;
   the two deleting destructors are the derived and base copies. */
// VTABLE: WIZ8 0x005ef190
// class W8GrowableVector<W8OptionsSaveRow*>

// VTABLE: WIZ8 0x005ef08c
// class W8Vector<W8OptionsSaveRow*>

/* W8OptionsPanelSet::m_panels is a W8Vector<W8OptionsPanel*>: the derived
   0x005EF01C table over base 0x005EEFE0. */
// VTABLE: WIZ8 0x005eefe0
// class W8GrowableVector<W8OptionsPanel*>

// VTABLE: WIZ8 0x005ef01c
// class W8Vector<W8OptionsPanel*>

/* W8OptionsPanel::m_text_buffers is a W8Vector<W8TextBuffer*>: the derived
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

/* GrCycle.cpp span. 0x005ECED0 is the particle-attachment base table;
   0x005ECED4 is the derived shake-effect table. */
// VTABLE: WIZ8 0x005eced0
// class W8GrowableVector<W8GrCycleParticleAttachment*>

// VTABLE: WIZ8 0x005ecee0
// class W8Vector<W8GrowableVector<W8GrCycle*>*>

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

// VTABLE: WIZ8 0x005ed5b4
// class W8GrowableVector<W8Widget*>

// VTABLE: WIZ8 0x005ed660
// class W8GrowableVector<W8TextControl*>

// VTABLE: WIZ8 0x005ee8cc
// class W8GrowableVector<W8ChunkHead*>

/* MGSKeyboard's binding vector stamps the base table at 0x0055CFF4,
   then the derived table at 0x0055D01B. */
// VTABLE: WIZ8 0x005ee8f8 W8GrowableVector<MGSKeyBinding*>
// class W8GrowableVector<MGSKeyBinding*>

// VTABLE: WIZ8 0x005ee8f4 W8Vector<MGSKeyBinding*>
// class W8Vector<MGSKeyBinding*>

/* One hand-rolled growable-array template. Retained storage/lifecycle emissions
   may be shared by different element types; concrete pairing names alone do not
   establish their original template arguments. */
template <class T> class W8GrowableVector {
public:
    /* Retail emits both a no-argument constructor with capacity five and a
       capacity-taking constructor. Observed no-argument allocations include
       one-, four- and twelve-byte elements; width alone does not name T. */
    W8GrowableVector()
    {
        data = new T[5];
        count = 0;
        if (data != 0) {
            capacity = 5;
        } else {
            capacity = 0;
        }
    }

    explicit W8GrowableVector(int initial_capacity)
    {
        if (initial_capacity < 1) {
            initial_capacity = 1;
        }
        data = new T[initial_capacity];
        count = 0;
        if (data != 0) {
            capacity = initial_capacity;
        } else {
            capacity = 0;
        }
    }

    /* The copy is sized to the source's live count rather than its capacity.
       Retail 0x004ed900 copies four-byte elements without distinguishing their
       scalar/pointer interpretation. */
    W8GrowableVector(const W8GrowableVector& other)
    {
        data = new T[other.count];
        count = other.count;
        capacity = other.count;
        for (int index = 0; index < count; ++index) {
            data[index] = other.data[index];
        }
    }

    virtual ~W8GrowableVector()
    {
        delete[] data;
    }
    int operator=(const W8GrowableVector& other);

    int Grow(int minimum_capacity)
    {
        int index;
        T* previous_data;
        T* replacement;

        if (minimum_capacity > capacity) {
            previous_data = data;
            replacement = new T[minimum_capacity];
            data = replacement;
            if (replacement == 0) {
                data = previous_data;
                return 0;
            }
            capacity = minimum_capacity;
            for (index = 0; index < count; ++index) {
                data[index] = previous_data[index];
            }
            delete[] previous_data;
        }
        return 1;
    }

    int GetCount() const
    {
        return count;
    }

    T* GetAt(int position)
    {
        if (position < count) {
            return data + position;
        }
        return data;
    }

    const T* GetAt(int position) const
    {
        if (position < count) {
            return data + position;
        }
        return data;
    }

    /* Unchecked element access. Retail indexes the backing store directly at
       many sites (for example the chunk reader's top-of-stack reads), which
       GetAt's bounds test cannot produce. */
    T& operator[](int position)
    {
        return data[position];
    }

    const T& operator[](int position) const
    {
        return data[position];
    }

    T SetAt(int position, T value)
    {
        T previous;

        if (position >= count) {
            return 0;
        }
        previous = data[position];
        data[position] = value;
        return previous;
    }

    int Add(T value)
    {
        int position = count + 1;

        if (position > capacity && !Grow(position)) {
            return -1;
        }
        data[count] = value;
        return count++;
    }

    unsigned char InsertAt(int position, T value)
    {
        int index;

        /* Retail insertion grows by five, unlike Add's minimum-sized growth
           (005D21F0 pointer entries and 004C80E0 script-condition bytes). */
        if (count + 1 > capacity && !Grow(capacity + 5)) {
            return 0;
        }
        index = position;
        if (position < count) {
            for (index = count; index > position && index >= 0; --index) {
                data[index] = data[index - 1];
            }
        }
        data[index] = value;
        ++count;
        return 1;
    }

    /* Returns the element it unlinked. GenerateItemsFromTable discards that
       value, while callers such as the dialog destructor delete it. */
    T RemoveAt(int position);

    /* Removes the entry before deleting its object. UI ownership teardown
       repeats this operation in reverse order; spell-effect teardown also
       calls the out-of-line emission at 0x00516A00. */
    void RemoveAtAndDelete(int position);

    /* Capture the original extent, unlinking each entry before destruction.
       Destructors may change the vector, so RemoveAt retains its bounds check. */
    void RemoveAllAndDelete()
    {
        int index = GetCount();
        if (index > 0) {
            while (--index, index >= 0) {
                RemoveAtAndDelete(index);
            }
        }
    }

    /* Removes the first matching entry, if any, and reports whether one was
       there. The startup entry queues reach it through QueueEntry. */
    unsigned char Remove(T entry);

    /* The image walks the array from a pointer loaded once rather than
       indexing through GetAt, which bounds-checks. Controls.cpp:2718 asserts on
       the -1 this returns, so the not-found value is the source's own. */
    int IndexOf(T value)
    {
        T* scan = data;
        int index;

        for (index = 0; index < count; ++index) {
            if (*scan == value) {
                return index;
            }
            ++scan;
        }
        return -1;
    }

    void Clear()
    {
        count = 0;
    }

    int count;    /* 0x04 */
    int capacity; /* 0x08 */
    T* data;      /* 0x0c */
}; /* 0x10 in the 32-bit target */

template <class T> int W8GrowableVector<T>::operator=(const W8GrowableVector<T>& other)
{
    int index;

    count = 0;
    if (other.count > capacity && !Grow(other.count)) {
        return 0;
    }
    for (index = 0; index < other.count; ++index) {
        data[index] = other.data[index];
    }
    if (count <= other.count) {
        count = other.count;
    }
    return 1;
}

template <class T> T W8GrowableVector<T>::RemoveAt(int position)
{
    int index;
    T result;

    if (position >= count || position < 0) {
        return 0;
    }
    result = data[position];
    for (index = position; index < count - 1; ++index) {
        data[index] = data[index + 1];
    }
    --count;
    return result;
}

template <class T> void W8GrowableVector<T>::RemoveAtAndDelete(int position)
{
    delete RemoveAt(position);
}

template <class T> unsigned char W8GrowableVector<T>::Remove(T entry)
{
    int index = 0;

    while (index < count) {
        if (data[index] == entry) {
            RemoveAt(index);
            return 1;
        }
        ++index;
    }
    return 0;
}

/* Thin derived collection: identical layout and inherited behavior, but the
   retail image gives it its own vtable and deleting destructor (0x005EC018
   over the base 0x005EC004 for the stModelInstance* instantiation, with the
   base constructor emitted at 0x004390F0 and the derived deleting destructor
   at 0x00438F70). The octree model-instance queries take the base pointer. */
template <class T> class W8Vector : public W8GrowableVector<T> {
public:
    /* Retail emits only the capacity form (0x00445FF0 takes the count); the
       default argument carries unadorned declarations. */
    explicit W8Vector(int initial_capacity = 5) : W8GrowableVector<T>(initial_capacity) {}
};

#endif
