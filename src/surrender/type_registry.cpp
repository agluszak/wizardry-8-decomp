#include "surrender/srTypeRegistry.h"

#include "surrender/srDebug.h"

#include <string.h>

/* The assert strings carry the original build tree's __FILE__ expansions,
   not this checkout's layout. */
#define SRRUNTIMECLASS_CPP "D:\\srsdk1x\\sources\\corelib\\srRuntimeClass.cpp"
#define SRCLASS_CPP "D:\\srsdk1x\\sources\\corelib\\srClass.cpp"

namespace {
unsigned long next_instance_id = 1;

inline unsigned long hashInteger(unsigned long value)
{
    return ((value >> 10) ^ value) >> 10 ^ value;
}

unsigned long hashName(const char* name)
{
    unsigned long hash = 0;
    for (unsigned long index = 0; name[index] != '\0'; ++index) {
        hash += (index + 0x4ad) * static_cast<signed char>(name[index]);
    }
    return hash;
}

template <class Key, class Value> class RegistryHash {
public:
    struct Entry {
        int next_00;
        Key key_04;
        Value value_08;
    };

    RegistryHash() : buckets_00(0), entries_04(0), free_08(-1), bucket_count_0c(0)
    {
        resize();
    }

    ~RegistryHash()
    {
        if (buckets_00 != 0) {
            operator delete(buckets_00);
        }
        if (entries_04 != 0) {
            operator delete(entries_04);
        }
    }

    Value find(Key key) const
    {
        int entry = buckets_00[hashInteger((unsigned long)key) & (bucket_count_0c - 1)];
        while (entry != -1) {
            if (entries_04[entry].key_04 == key) {
                return entries_04[entry].value_08;
            }
            entry = entries_04[entry].next_00;
        }
        return 0;
    }

    void insert(Key key, Value value)
    {
        int entry = allocateEntry();
        unsigned long bucket = hashInteger((unsigned long)key) & (bucket_count_0c - 1);
        entries_04[entry].key_04 = key;
        entries_04[entry].value_08 = value;
        entries_04[entry].next_00 = buckets_00[bucket];
        buckets_00[bucket] = entry;
    }

    Value erase(Key key)
    {
        unsigned long bucket = hashInteger((unsigned long)key) & (bucket_count_0c - 1);
        int* link = &buckets_00[bucket];
        while (*link != -1) {
            int entry = *link;
            if (entries_04[entry].key_04 == key) {
                Value value = entries_04[entry].value_08;
                *link = entries_04[entry].next_00;
                entries_04[entry].next_00 = free_08;
                free_08 = entry;
                return value;
            }
            link = &entries_04[entry].next_00;
        }
        return 0;
    }

    void clear()
    {
        if (bucket_count_0c != 0) {
            operator delete(buckets_00);
            operator delete(entries_04);
        }
        bucket_count_0c = 0;
        buckets_00 = 0;
        entries_04 = 0;
        free_08 = -1;
        resize();
    }

    int allocateEntry()
    {
        if (free_08 == -1) {
            resize();
        }
        int entry = free_08;
        free_08 = entries_04[entry].next_00;
        return entry;
    }

    void resize()
    {
        unsigned long count = bucket_count_0c * 2;
        if (count < 4) {
            count = 4;
        }
        Entry* entries = static_cast<Entry*>(operator new(count * sizeof(Entry)));
        int* buckets = static_cast<int*>(operator new(count * sizeof(int)));
        for (unsigned long index = 0; index < count; ++index) {
            buckets[index] = -1;
            entries[index].next_00 = -1;
        }

        unsigned long used = 0;
        if (bucket_count_0c != 0) {
            for (unsigned long bucket = 0; bucket < bucket_count_0c; ++bucket) {
                for (int old = buckets_00[bucket]; old != -1; old = entries_04[old].next_00) {
                    entries[used].key_04 = entries_04[old].key_04;
                    entries[used].value_08 = entries_04[old].value_08;
                    unsigned long next_bucket =
                        hashInteger((unsigned long)entries[used].key_04) & (count - 1);
                    entries[used].next_00 = buckets[next_bucket];
                    buckets[next_bucket] = used++;
                }
            }
            operator delete(buckets_00);
            operator delete(entries_04);
        }

        for (unsigned long free_index = used; free_index < count; ++free_index) {
            entries[free_index].next_00 = free_index + 1;
        }
        entries[count - 1].next_00 = -1;
        free_08 = used;
        buckets_00 = buckets;
        bucket_count_0c = count;
        entries_04 = entries;
    }

private:
    int* buckets_00;
    Entry* entries_04;
    int free_08;
    unsigned long bucket_count_0c;
};

static_assert(sizeof(RegistryHash<unsigned long, void*>) == 0x10, "RegistryHash_must_be_0x10");

class RegistryAccess {
public:
    explicit RegistryAccess(srCriticalSection* critical_section)
        : critical_section_(critical_section)
    {
        critical_section_->getAccess();
    }

    ~RegistryAccess()
    {
        critical_section_->releaseAccess();
    }

private:
    srCriticalSection* critical_section_;
};
} // namespace

struct srRegistry::ClassNode::NameIndex {
    struct NameEntry {
        NameEntry* next_00;
        NameEntry* previous_04;
        unsigned long bucket_08;
        const char* name_0c;
        srRuntimeClass* instance_10;
    };

    NameIndex()
        : entries_10(0), free_14(0), buckets_18(0), count_1c(0), bucket_count_20(0),
          case_sensitive_24(1)
    {
        resize(4);
    }

    ~NameIndex()
    {
        if (buckets_18 != 0) {
            ::operator delete(buckets_18);
        }
        if (entries_10 != 0) {
            ::operator delete(entries_10);
        }
        buckets_18 = 0;
        entries_10 = 0;
        free_14 = 0;
        count_1c = 0;
        bucket_count_20 = 0;
        by_instance_00.clear();
    }

    int namesEqual(const char* first, const char* second) const
    {
        return case_sensitive_24 != 0 ? strcmp(first, second) == 0 : _stricmp(first, second) == 0;
    }

    void add(srRuntimeClass* instance)
    {
        const char* name = instance->getName();
        if (name == 0) {
            return;
        }
        if (free_14 == 0) {
            resize(bucket_count_20 * 2);
        }

        NameEntry* entry = free_14;
        free_14 = entry->next_00;
        unsigned long bucket = hashName(name) & (bucket_count_20 - 1);
        entry->bucket_08 = bucket;
        entry->name_0c = name;
        entry->instance_10 = instance;
        entry->previous_04 = 0;
        entry->next_00 = buckets_18[bucket];
        if (entry->next_00 != 0) {
            entry->next_00->previous_04 = entry;
        }
        buckets_18[bucket] = entry;
        by_instance_00.insert(instance, entry);
        ++count_1c;
    }

    void remove(srRuntimeClass* instance)
    {
        NameEntry* entry = by_instance_00.erase(instance);
        if (entry == 0) {
            return;
        }
        if (entry->previous_04 == 0) {
            buckets_18[entry->bucket_08] = entry->next_00;
        } else {
            entry->previous_04->next_00 = entry->next_00;
        }
        if (entry->next_00 != 0) {
            entry->next_00->previous_04 = entry->previous_04;
        }
        entry->previous_04 = 0;
        entry->name_0c = 0;
        entry->instance_10 = 0;
        entry->next_00 = free_14;
        free_14 = entry;
        --count_1c;
        if (bucket_count_20 > 7 && count_1c <= bucket_count_20 / 4) {
            resize(bucket_count_20 / 2);
        }
    }

    unsigned long bucketIndex(const char* name) const
    {
        return hashName(name) & (bucket_count_20 - 1);
    }

    srRuntimeClass* find(const char* name, const srRuntimeClass* relative_to) const
    {
        if (relative_to != 0) {
            NameEntry* entry = by_instance_00.find(const_cast<srRuntimeClass*>(relative_to));
            if (entry == 0) {
                /* Retail (inlined at 0x10010156 in findByName 0x100100D0)
                   returns the hash-bucket head unverified — no namesEqual —
                   when the relative instance is absent from the side index.
                   A collision can therefore return a differently-named
                   instance; genuine retail behavior, preserved. */
                entry = buckets_18[bucketIndex(name)];
                return entry == 0 ? 0 : entry->instance_10;
            }
            for (entry = entry->next_00; entry != 0; entry = entry->next_00) {
                if (namesEqual(name, entry->name_0c)) {
                    return entry->instance_10;
                }
            }
            return 0;
        }
        for (NameEntry* entry = buckets_18[bucketIndex(name)]; entry != 0; entry = entry->next_00) {
            if (namesEqual(name, entry->name_0c)) {
                return entry->instance_10;
            }
        }
        return 0;
    }

private:
    void resize(unsigned long bucket_count)
    {
        unsigned long old_bucket_count = bucket_count_20;
        bucket_count_20 = bucket_count;
        free_14 = 0;
        /* Retail clears by_instance_00 rather than rehashing it: every live
           instance is re-inserted with its new NameEntry below, so preserving
           the old mappings would leave duplicate keys pointing into the freed
           entry array (0x10010F30 calls 0x10011380, RegistryHash::clear). */
        by_instance_00.clear();
        NameEntry* entries = 0;
        NameEntry** buckets = 0;
        if (bucket_count != 0) {
            entries = static_cast<NameEntry*>(::operator new(bucket_count * sizeof(NameEntry)));
            buckets = static_cast<NameEntry**>(::operator new(bucket_count * sizeof(NameEntry*)));
            for (unsigned long index = 0; index < bucket_count; ++index) {
                buckets[index] = 0;
                entries[index].next_00 = &entries[index + 1];
                entries[index].previous_04 = 0;
                entries[index].bucket_08 = 0;
                entries[index].name_0c = 0;
                entries[index].instance_10 = 0;
            }
            entries[bucket_count - 1].next_00 = 0;
            free_14 = entries;
            if (buckets_18 != 0 && old_bucket_count != 0) {
                for (unsigned long bucket = 0; bucket < old_bucket_count; ++bucket) {
                    for (NameEntry* entry = buckets_18[bucket]; entry != 0;
                         entry = entry->next_00) {
                        const char* name = entry->name_0c;
                        NameEntry* reused = free_14;
                        free_14 = reused->next_00;
                        unsigned long new_bucket = hashName(name) & (bucket_count_20 - 1);
                        reused->bucket_08 = new_bucket;
                        reused->name_0c = name;
                        reused->instance_10 = entry->instance_10;
                        reused->previous_04 = 0;
                        reused->next_00 = buckets[new_bucket];
                        if (reused->next_00 != 0) {
                            reused->next_00->previous_04 = reused;
                        }
                        buckets[new_bucket] = reused;
                        by_instance_00.insert(entry->instance_10, reused);
                    }
                }
            }
        }
        if (buckets_18 != 0) {
            ::operator delete(buckets_18);
        }
        if (entries_10 != 0) {
            ::operator delete(entries_10);
        }
        buckets_18 = 0;
        entries_10 = 0;
        if (bucket_count != 0) {
            buckets_18 = buckets;
            entries_10 = entries;
        }
    }

    RegistryHash<srRuntimeClass*, NameEntry*> by_instance_00;
    NameEntry* entries_10;
    NameEntry* free_14;
    NameEntry** buckets_18;
    unsigned long count_1c;
    unsigned long bucket_count_20;
    int case_sensitive_24;
};

static_assert(sizeof(srRegistry::ClassNode::NameIndex) == 0x28,
              "srRegistry_ClassNode_NameIndex_must_be_0x28");

struct srRegistry::ClassNode::IDIndex {
    struct InstanceLink {
        union {
            srRuntimeClass* instance_00;
            InstanceLink* free_00;
        };
        InstanceLink* next_04;
        InstanceLink* previous_08;
        unsigned long unused_0c;
    };

    IDIndex()
        : active_count_00(0), free_04(0), blocks_08(0), block_capacity_0c(0), block_count_10(0),
          first_14(0), last_18(0), list_count_1c(0)
    {
    }

    ~IDIndex()
    {
        by_id_20.~RegistryHash();
        clearLinks();
        clearBlocks();
    }

    void* operator new(unsigned int size)
    {
        return srHeap.allocate(size);
    }
    void operator delete(void* index)
    {
        srHeap.free(index);
    }

    InstanceLink* add(srRuntimeClass* instance)
    {
        InstanceLink* link = insert(0, instance);
        by_id_20.insert(instance->getID(), link);
        return link;
    }

    void remove(unsigned long id)
    {
        InstanceLink* link = by_id_20.erase(id);
        if (link == 0) {
            return;
        }
        if (link->previous_08 == 0) {
            first_14 = link->next_04;
        } else {
            link->previous_08->next_04 = link->next_04;
        }
        if (link->next_04 == 0) {
            last_18 = link->previous_08;
        } else {
            link->next_04->previous_08 = link->previous_08;
        }
        --active_count_00;
        link->free_00 = free_04;
        free_04 = link;
        if (active_count_00 == 0) {
            clearBlocks();
        }
        --list_count_1c;
    }

    srRuntimeClass* find(unsigned long id) const
    {
        InstanceLink* link = by_id_20.find(id);
        return link == 0 ? 0 : link->instance_00;
    }

    InstanceLink* findRelative(const srRuntimeClass* relative_to) const
    {
        InstanceLink* link = first_14;
        if (relative_to != 0) {
            while (link != 0 && link->instance_00 != relative_to) {
                link = link->next_04;
            }
            if (link != 0) {
                link = link->next_04;
            }
        }
        return link;
    }

private:
    InstanceLink* insert(InstanceLink* after, srRuntimeClass*& instance)
    {
        if (free_04 == 0) {
            allocateBlock();
        }
        InstanceLink* link = free_04;
        free_04 = link->free_00;
        ++active_count_00;
        link->instance_00 = instance;
        if (after == 0) {
            link->previous_08 = 0;
            link->next_04 = first_14;
        } else {
            link->previous_08 = after;
            link->next_04 = after->next_04;
            after->next_04 = link;
        }
        if (link->next_04 != 0) {
            link->next_04->previous_08 = link;
        }
        if (link->previous_08 == 0) {
            first_14 = link;
        }
        if (link->next_04 == 0) {
            last_18 = link;
        }
        ++list_count_1c;
        return link;
    }

    void allocateBlock()
    {
        unsigned long count = active_count_00 < 2 ? 1 : active_count_00;
        if (count > 0x100) {
            count = 0x100;
        }
        InstanceLink* block =
            static_cast<InstanceLink*>(srHeap.allocate(count * sizeof(InstanceLink)));
        free_04 = block;
        unsigned long index = block_count_10;
        block_count_10 = index + 1;
        if (block_capacity_0c <= index) {
            setBlockCapacity(block_capacity_0c + 8 + index);
        }
        blocks_08[index] = block;
        for (unsigned long i = 0; i < count; ++i) {
            block[i].free_00 = &block[i + 1];
        }
        block[count - 1].free_00 = 0;
    }

    void setBlockCapacity(unsigned long capacity)
    {
        if (block_capacity_0c != capacity) {
            InstanceLink** blocks = 0;
            if (capacity != 0) {
                blocks =
                    static_cast<InstanceLink**>(::operator new(capacity * sizeof(InstanceLink*)));
                if (blocks_08 != 0 && block_capacity_0c != 0) {
                    unsigned long copy =
                        capacity <= block_capacity_0c ? capacity : block_capacity_0c;
                    for (unsigned long i = 0; i < copy; ++i) {
                        blocks[i] = blocks_08[i];
                    }
                }
            }
            ::operator delete(blocks_08);
            blocks_08 = blocks;
            block_capacity_0c = capacity;
        }
    }

    void clearLinks()
    {
        while (first_14 != 0) {
            InstanceLink* link = first_14;
            if (link->previous_08 == 0) {
                first_14 = link->next_04;
            } else {
                link->previous_08->next_04 = link->next_04;
            }
            if (link->next_04 == 0) {
                last_18 = link->previous_08;
            } else {
                link->next_04->previous_08 = link->previous_08;
            }
            --active_count_00;
            link->free_00 = free_04;
            free_04 = link;
            if (active_count_00 == 0) {
                clearBlocks();
            }
        }
    }

    /* Retail clearBlocks (0x10010A90) resets only the block bookkeeping and
       active_count_00; it deliberately does not touch first_14, last_18 or
       list_count_1c. Callers update the list head/tail before the count hits
       zero, and remove() decrements list_count_1c after this call, so the
       counters stay consistent — zeroing them here would underflow the
       post-call decrement to 0xffffffff. */
    void clearBlocks()
    {
        for (unsigned long index = 0; index < block_count_10; ++index) {
            if (block_capacity_0c <= index) {
                setBlockCapacity(block_capacity_0c + 8 + index);
            }
            srHeap.free(blocks_08[index]);
        }
        ::operator delete(blocks_08);
        blocks_08 = 0;
        block_capacity_0c = 0;
        free_04 = 0;
        block_count_10 = 0;
        active_count_00 = 0;
    }

    unsigned long active_count_00;
    InstanceLink* free_04;
    InstanceLink** blocks_08;
    unsigned long block_capacity_0c;
    unsigned long block_count_10;
    InstanceLink* first_14;
    InstanceLink* last_18;
    unsigned long list_count_1c;
    RegistryHash<unsigned long, InstanceLink*> by_id_20;
};

static_assert(sizeof(srRegistry::ClassNode::IDIndex) == 0x30,
              "srRegistry_ClassNode_IDIndex_must_be_0x30");

struct srRegistry::ClassIndex : RegistryHash<unsigned long, srRegistry::ClassNode*> {};

// GLOBAL: SURRENDER 0x100A45AC
unsigned long srClass::_timestampCtr;

// GLOBAL: SURRENDER 0x100A45B0
srClass::Update* srClass::_firstUpdate;

// GLOBAL: SURRENDER 0x100A45B8
double srClass::_lastUpdateTime;

// FUNCTION: SURRENDER 0x10011790
const char* srRuntimeClass::getName() const
{
    return name_04 != 0 ? name_04 : "anonymous";
}

// FUNCTION: SURRENDER 0x100117A0
void srRuntimeClass::verify(e_verify)
{
    if (getName() == 0) {
        srAssertFail("getName()", SRRUNTIMECLASS_CPP, 0xb0, 0);
    }
    if (getID() == 0) {
        srAssertFail("getID()", SRRUNTIMECLASS_CPP, 0xb1, 0);
    }
    if (getClassName() == 0) {
        srAssertFail("getClassName()", SRRUNTIMECLASS_CPP, 0xb2, 0);
    }
    if (getClassID() == 0) {
        srAssertFail("getClassID()", SRRUNTIMECLASS_CPP, 0xb3, 0);
    }
    if (getClassNode() == 0) {
        srAssertFail("getClassNode()", SRRUNTIMECLASS_CPP, 0xb4, 0);
    }
    if (matchClassID(getClassID()) == 0) {
        srAssertFail("matchClassID(getClassID())", SRRUNTIMECLASS_CPP, 0xb5, 0);
    }
}

// FUNCTION: SURRENDER 0x100118D0
int srRuntimeClass::matchClassID(unsigned long class_id) const
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* instance_class = getClassNode();
    srRegistry::ClassNode* requested_class = registry->getClassNode(class_id);
    return registry->isDerivedOrSame(requested_class, instance_class);
}

// FUNCTION: SURRENDER 0x10011900
int srRuntimeClass::isNamed() const
{
    return name_04 != 0;
}

// FUNCTION: SURRENDER 0x10011910
long srRuntimeClass::getTotalInstances(int exact)
{
    return srCore.getRegistry()->getNumberOfInstances(sGetClassNode(), exact);
}

// FUNCTION: SURRENDER 0x10011930
void srRuntimeClass::dumpNames(std::ostream& stream, int indent)
{
    srCore.getRegistry()->dumpInstanceNames(sGetClassNode(), stream, indent);
}

// FUNCTION: SURRENDER 0x10011880
void srRuntimeClass::getUniqueName(std::ostream& stream) const
{
    stream << getName() << '[' << getID() << ']';
}

// FUNCTION: SURRENDER 0x10011950
void srRuntimeClass::setName(const char* name)
{
    if (name_04 != 0) {
        delete[] name_04;
    }
    if (name == 0 || *name == '\0') {
        name_04 = 0;
    } else {
        name_04 = new char[strlen(name) + 1];
        strcpy(name_04, name);
    }
    srCore.getRegistry()->refreshInstance(getClassNode(), this);
}

/* Retail stores the vptr before the member writes, so the body assigns the
   members rather than running a member-initializer list, and the registry
   pointer is fetched once into a named local. */
// FUNCTION: SURRENDER 0x100119D0
srRuntimeClass::srRuntimeClass()
{
    name_04 = 0;
    srRegistry* registry = srCore.getRegistry();
    id_08 = registry->allocateID();
    registry->registerInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x10011A40
srRuntimeClass::~srRuntimeClass()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
    if (name_04 != 0) {
        delete[] name_04;
    }
}

/* The dump prints through the void* overload for both IDs and addresses:
   getClassID/getID results reach operator<<(const void*), not the unsigned
   long overload. */
// FUNCTION: SURRENDER 0x10011AB0
void srRuntimeClass::dump(std::ostream& stream)
{
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    // c-style-cast-ok: retail prints the id through the void* overload
    stream << "Class Id: " << (void*)getClassID() << '\n';
    stream.width(0x20);
    stream << "Class name: " << getClassName() << '\n';
    stream.width(0x20);
    // c-style-cast-ok: retail prints the id through the void* overload
    stream << "Instance Id code: " << (void*)getID() << '\n';
    stream.width(0x20);
    stream << "Instance name: " << getName() << '\n';
    stream.width(0x20);
    stream << "Memory address: " << this << '\n';
    stream.flags(flags);
}

// FUNCTION: SURRENDER 0x10011CB0
unsigned long srRuntimeClass::getID() const
{
    return id_08;
}

// FUNCTION: SURRENDER 0x10011C50
srRegistry::ClassNode* srRuntimeClass::sGetClassNode()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(1);
    if (node == 0) {
        node = registry->registerClass("srRuntimeClass", registry->getRootNode(), 1, 0);
    }
    return node;
}

// FUNCTION: SURRENDER 0x10011C90
unsigned long srRuntimeClass::sGetClassID()
{
    srRegistry* registry = srCore.getRegistry();
    return registry->getClassID(sGetClassNode());
}

// FUNCTION: SURRENDER 0x10011CC0
const char* srRuntimeClass::getClassName() const
{
    return "srRuntimeClass";
}

// FUNCTION: SURRENDER 0x10011CD0
unsigned long srRuntimeClass::getClassID() const
{
    return sGetClassID();
}

// FUNCTION: SURRENDER 0x10011CE0
srRegistry::ClassNode* srRuntimeClass::getClassNode() const
{
    return sGetClassNode();
}

// FUNCTION: SURRENDER 0x1000E050
void srClass::verify(srRuntimeClass::e_verify mode)
{
    srRuntimeClass::verify(mode);
    if (reference_count_0c < 0) {
        srAssertFail("_refCount >= 0", SRCLASS_CPP, 0x3f, 0);
    }
}

// FUNCTION: SURRENDER 0x1000E080
srClass* srClass::find(const char* name, const srClass* relative_to)
{
    return static_cast<srClass*>(srCore.getRegistry()->find(sGetClassNode(), name, relative_to));
}

// FUNCTION: SURRENDER 0x1000E0A0
srClass* srClass::find(const char* name, unsigned long class_id, const srRuntimeClass* relative_to)
{
    srRegistry* registry = srCore.getRegistry();
    return static_cast<srClass*>(
        registry->find(registry->getClassNode(class_id), name, relative_to));
}

// FUNCTION: SURRENDER 0x1000E0D0
srClass* srClass::find(unsigned long id)
{
    return static_cast<srClass*>(srCore.getRegistry()->find(sGetClassNode(), id));
}

// FUNCTION: SURRENDER 0x1000E0F0
srClass* srClass::find(const srClass* relative_to)
{
    return static_cast<srClass*>(srCore.getRegistry()->find(sGetClassNode(), relative_to));
}

/* Retail assigns only the instance name: the base operator= is not invoked
   and the reference count, timestamp and update link are left alone. */
// FUNCTION: SURRENDER 0x1000E110
srClass& srClass::operator=(const srClass& other)
{
    if (this != &other) {
        setName(other.getName());
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1000E130
srClass::srClass() : reference_count_0c(1), update_14(0)
{
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    touch();
}

/* Retail ~srClass is exactly unregisterInstance + base teardown; it does
   NOT unlink update_14. A class destroyed while updates remain enabled
   leaves a leaked Update node on the global _firstUpdate list with a
   dangling instance_14 that performUpdates later calls into — genuine
   retail lifetime bug. The ownership contract is that derived classes (or
   their owners) call setUpdate(0, 0) before destruction; the destructor is
   kept faithful rather than "fixed". */
// FUNCTION: SURRENDER 0x1000E1A0
srClass::~srClass()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x1000E200
srRegistry::ClassNode* srClass::sGetClassNode()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x100);
    if (node == 0) {
        node = registry->registerClass(sGetClassName(), srRuntimeClass::sGetClassNode(), 0x100, 0);
    }
    return node;
}

// FUNCTION: SURRENDER 0x1000E2F0
srClass::UpdateCallBack srClass::getUpdateCallBack()
{
    return update_14 == 0 ? 0 : update_14->callback_10;
}

// FUNCTION: SURRENDER 0x1000E360
double srClass::getUpdateInterval()
{
    return update_14 == 0 ? 0.0 : update_14->interval_08;
}

// FUNCTION: SURRENDER 0x1000E380
void srClass::setUpdate(UpdateCallBack callback, double interval)
{
    if (update_14 != 0 || callback != 0) {
        if (update_14 != 0) {
            if (callback != 0) {
                update_14->callback_10 = callback;
                update_14->interval_08 = interval;
                return;
            }

            if (update_14->previous_18 != 0) {
                update_14->previous_18->next_1c = update_14->next_1c;
            }
            if (update_14->next_1c != 0) {
                update_14->next_1c->previous_18 = update_14->previous_18;
            }
            if (update_14 == _firstUpdate) {
                _firstUpdate = update_14->next_1c;
            }
            delete update_14;
            update_14 = 0;
        }

        if (callback != 0) {
            update_14 = new Update;
            update_14->instance_14 = this;
            update_14->callback_10 = callback;
            update_14->interval_08 = interval;
            update_14->last_update_time_00 = _lastUpdateTime;
            update_14->next_1c = _firstUpdate;
            update_14->previous_18 = 0;
            if (update_14->next_1c != 0) {
                update_14->next_1c->previous_18 = update_14;
            }
            _firstUpdate = update_14;
        }
    }
}

// FUNCTION: SURRENDER 0x1000E490
void srClass::setUpdatesTime(double time)
{
    _lastUpdateTime = time;
    for (Update* update = _firstUpdate; update != 0; update = update->next_1c) {
        update->last_update_time_00 = time;
    }
}

// FUNCTION: SURRENDER 0x1000E4D0
void srClass::performUpdates(double time)
{
    if (time > _lastUpdateTime) {
        Update* update = _firstUpdate;
        if (_lastUpdateTime == 0.0) {
            _lastUpdateTime = time;
        }
        while (update != 0) {
            Update* next = update->next_1c;
            if (update->interval_08 <= 0.0) {
                update->callback_10(update->instance_14, time, time - _lastUpdateTime);
                update->last_update_time_00 = time;
            } else {
                for (double update_time = update->last_update_time_00 + update->interval_08;
                     update_time <= time; update_time += update->interval_08) {
                    update->callback_10(update->instance_14, update_time,
                                        update_time - update->last_update_time_00);
                    update->last_update_time_00 = update_time;
                }
            }
            update = next;
        }
    }
    _lastUpdateTime = time;
}

// FUNCTION: SURRENDER 0x1000E5E0
void srClass::touch()
{
    timestamp_10 = ++_timestampCtr;
}

// FUNCTION: SURRENDER 0x1000E5F0
unsigned long srClass::getTimestamp() const
{
    return timestamp_10;
}

// FUNCTION: SURRENDER 0x1000E600
unsigned long srClass::allocateTimeStamps(unsigned long count) const
{
    unsigned long first = _timestampCtr;
    _timestampCtr += count;
    return first + 1;
}

/* The update block prints through the void* overload for the callback and
   the intrusive list links; the interval prints "every frame" at zero and
   the bare "secs" suffix otherwise. */
// FUNCTION: SURRENDER 0x1000E620
void srClass::dump(std::ostream& stream)
{
    srRuntimeClass::dump(stream);
    std::ios::fmtflags flags = stream.flags();
    stream.setf(std::ios::left, std::ios::adjustfield);
    stream.width(0x20);
    stream << "  Timestamp: " << getTimestamp() << '\n';
    stream.width(0x20);
    stream << "  Reference count: " << getReferenceCount() << '\n';
    stream.width(0x20);
    if (update_14 != 0) {
        if (update_14->interval_08 == 0.0) {
            stream << "  Update interval: " << "every frame" << '\n';
        } else {
            stream << "  Update interval: " << update_14->interval_08 << "secs" << '\n';
        }
        stream.width(0x20);
        // c-style-cast-ok: retail prints the callback through the void* overload
        stream << "    Update callback: " << (void*)update_14->callback_10 << '\n';
        if (update_14->instance_14 != 0) {
            stream.width(0x20);
            stream << "    Update owner: ";
            update_14->instance_14->getUniqueName(stream);
            stream << '\n';
        }
        if (update_14->previous_18 != 0) {
            stream.width(0x20);
            stream << "    Update previous: " << update_14->previous_18 << '\n';
        }
        if (update_14->next_1c != 0) {
            stream.width(0x20);
            stream << "    Update next: " << update_14->next_1c << '\n';
        }
    } else {
        stream << "  Update interval: " << "disabled" << '\n';
    }
    stream.flags(flags);
}

// FUNCTION: SURRENDER 0x1000E850
srClass* srClass::instance()
{
    return vInstance();
}

// FUNCTION: SURRENDER 0x1000E870
const char* srClass::sGetClassName()
{
    return "srClass";
}

// FUNCTION: SURRENDER 0x1000E880
srRegistry::ClassNode* srClass::getClassNode() const
{
    return sGetClassNode();
}

// FUNCTION: SURRENDER 0x1000E910
srRegistry::srRegistry()
    : root_00(0), class_index_04(0), valid_08(0), critical_section_0c(new srCriticalSection)
{
    RegistryAccess access(critical_section_0c);
    class_index_04 = new ClassIndex;
    root_00 = new ClassNode(0, "root", 0);
    class_index_04->insert(0, root_00);
    valid_08 = 1;
}

// FUNCTION: SURRENDER 0x1000EA40
unsigned long srRegistry::getClassID(ClassNode* node)
{
    RegistryAccess access(critical_section_0c);
    return node->getClassID();
}

// FUNCTION: SURRENDER 0x1000EAA0
const char* srRegistry::getClassName(ClassNode* node)
{
    RegistryAccess access(critical_section_0c);
    return node->class_name_14;
}

// FUNCTION: SURRENDER 0x1000EAD0
srRegistry::~srRegistry()
{
    {
        RegistryAccess access(critical_section_0c);
        delete root_00;
        root_00 = 0;
        delete class_index_04;
        class_index_04 = 0;
        valid_08 = 0;
    }
    delete critical_section_0c;
}

/* Retail emits a verbatim memberwise copy of all four owning fields
   (root_00, class_index_04, valid_08, critical_section_0c): this is the
   compiler-generated operator= the dllexport class requires, not safe
   value semantics. Assigning a live registry aliases the source's entire
   ownership graph and leaks the destination's root, index and critical
   section; destructing either then double-frees them. No copy ctor is
   emitted for srRegistry and no consumer imports this export (absent
   from the Wiz8.exe sr.dll import table), so the shared-ownership hazard
   is genuine but unreachable retail behavior. */
// FUNCTION: SURRENDER 0x1000EBA0
srRegistry& srRegistry::operator=(const srRegistry& other)
{
    root_00 = other.root_00;
    class_index_04 = other.class_index_04;
    valid_08 = other.valid_08;
    critical_section_0c = other.critical_section_0c;
    return *this;
}

// FUNCTION: SURRENDER 0x1000EBD0
srRegistry::ClassNode* srRegistry::getClassNode(unsigned long class_id)
{
    RegistryAccess access(critical_section_0c);
    return class_id == 0 ? 0 : class_index_04->find(class_id);
}

// FUNCTION: SURRENDER 0x1000EC60
srRegistry::ClassNode* srRegistry::registerClass(const char* class_name, ClassNode* parent,
                                                 unsigned long class_id, int register_instances)
{
    RegistryAccess access(critical_section_0c);
    ClassNode* node = class_index_04->find(class_id);
    if (node == 0) {
        srDebugPrintf(0xfe, "srRegistry::registerClass() - registering %s (ID 0x%x)\n", class_name,
                      class_id);
        node = addToTree(parent, class_name, class_id);
        if (register_instances != 0) {
            node->enableInstanceLookup();
        }
    }
    return node;
}

// FUNCTION: SURRENDER 0x1000EDB0
srRegistry::ClassNode* srRegistry::addToTree(ClassNode* parent, const char* class_name,
                                             unsigned long class_id)
{
    RegistryAccess access(critical_section_0c);
    ClassNode* node = new ClassNode(parent, class_name, class_id);
    class_index_04->insert(class_id, node);
    return node;
}

// FUNCTION: SURRENDER 0x1000EEA0
void srRegistry::dumpInstanceNames(ClassNode* node, std::ostream& stream, int indent)
{
    RegistryAccess access(critical_section_0c);
    // c-style-cast-ok: find(node, 0) is ambiguous between the id and
    // relative_to overloads; the original spelled the null pointer cast
    for (srRuntimeClass* instance = find(node, (srRuntimeClass*)0); instance != 0;
         instance = find(node, instance)) {
        if (indent != 0 || instance->isNamed()) {
            stream << "Name: " << instance->getName();
            stream << " (class: " << instance->getClassName() << ", address: " << instance
                   << ", instanceId: " << instance->getID() << ")\n";
        }
    }
}

// FUNCTION: SURRENDER 0x1000EFB0
void srRegistry::registerInstance(ClassNode* node, srRuntimeClass* instance)
{
    RegistryAccess access(critical_section_0c);
    node->registerInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F010
void srRegistry::refreshInstance(ClassNode* node, srRuntimeClass* instance)
{
    RegistryAccess access(critical_section_0c);
    node->refreshInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F070
srRuntimeClass* srRegistry::find(ClassNode* node, const char* name,
                                 const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findByName(node, name, 0, relative_to);
}

// FUNCTION: SURRENDER 0x1000F0E0
srRuntimeClass* srRegistry::findExact(ClassNode* node, const char* name,
                                      const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findByName(node, name, 1, relative_to);
}

// FUNCTION: SURRENDER 0x1000F150
srRuntimeClass* srRegistry::find(ClassNode* node, unsigned long id)
{
    RegistryAccess access(critical_section_0c);
    return node->findByID(node, id, 0);
}

// FUNCTION: SURRENDER 0x1000F1B0
srRuntimeClass* srRegistry::findExact(ClassNode* node, unsigned long id)
{
    RegistryAccess access(critical_section_0c);
    return node->findByID(node, id, 1);
}

// FUNCTION: SURRENDER 0x1000F210
void srRegistry::unregisterInstance(ClassNode* node, srRuntimeClass* instance)
{
    RegistryAccess access(critical_section_0c);
    node->unregisterInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F270
int srRegistry::isDerivedOrSame(ClassNode* base, ClassNode* derived)
{
    RegistryAccess access(critical_section_0c);
    int result = 0;
    if (base != 0 && derived != 0) {
        result = base->isDerivedOrSame(derived);
    }
    return result;
}

// FUNCTION: SURRENDER 0x1000F2F0
srRuntimeClass* srRegistry::findExact(ClassNode* node, const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findRelative(node, 1, relative_to);
}

// FUNCTION: SURRENDER 0x1000F350
srRuntimeClass* srRegistry::find(ClassNode* node, const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findRelative(node, 0, relative_to);
}

// FUNCTION: SURRENDER 0x1000F3B0
srRegistry::ClassNode* srRegistry::getRootClass()
{
    RegistryAccess access(critical_section_0c);
    return root_00->first_child_04->node_00;
}

// FUNCTION: SURRENDER 0x1000F3E0
srRegistry::ClassNode* srRegistry::getChildClass(ClassNode* parent, ClassNode* child)
{
    RegistryAccess access(critical_section_0c);
    ClassNode::ChildLink* link = parent->first_child_04;
    ClassNode* result = 0;
    if (child == 0) {
        if (link != parent->child_end_08) {
            result = link->node_00;
        }
    } else if (link != parent->child_end_08) {
        while (link->node_00 != child || link->next_04 == parent->child_end_08) {
            link = link->next_04;
            if (link == parent->child_end_08) {
                return 0;
            }
        }
        result = link->next_04->node_00;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1000F450
int srRegistry::checkValidity()
{
    return valid_08 != 0;
}

// FUNCTION: SURRENDER 0x1000F460
long srRegistry::getNumberOfInstances(ClassNode* node, int exact)
{
    RegistryAccess access(critical_section_0c);
    return node->getNumberOfInstances(exact);
}

// FUNCTION: SURRENDER 0x1000F4C0
unsigned long srRegistry::allocateID()
{
    RegistryAccess access(critical_section_0c);
    return next_instance_id++;
}

// FUNCTION: SURRENDER 0x100105C0
srRegistry::ClassNode* srRegistry::getRootNode()
{
    RegistryAccess access(critical_section_0c);
    return root_00;
}

// FUNCTION: SURRENDER 0x1000F580
srRegistry::ClassNode::ClassNode(ClassNode* parent, const char* class_name, unsigned long class_id)
    : child_count_00(0), first_child_04(new ChildLink), child_end_08(first_child_04)
{
    first_child_04->next_04 = 0;
    first_child_04->previous_08 = 0;
    initialize(parent, class_name, class_id);
}

// FUNCTION: SURRENDER 0x1000F5F0
void srRegistry::ClassNode::initialize(ClassNode* parent, const char* class_name,
                                       unsigned long class_id)
{
    class_id_10 = class_id;
    parent_0c = parent;
    class_name_14 = class_name;
    named_instances_18 = 0;
    inherited_named_instances_1c = 0;
    instances_by_id_20 = 0;
    inherited_instances_by_id_24 = 0;
    instance_count_28 = 0;
    if (parent != 0) {
        ChildLink* link = new ChildLink;
        link->next_04 = parent->first_child_04;
        link->node_00 = this;
        link->previous_08 = parent->first_child_04->previous_08;
        if (link->previous_08 == 0) {
            parent->first_child_04 = link;
        } else {
            link->previous_08->next_04 = link;
        }
        if (link->next_04 != 0) {
            link->next_04->previous_08 = link;
        }
        ++parent->child_count_00;
        inherited_named_instances_1c = parent->getNameIndex();
        inherited_instances_by_id_24 = parent->getIDIndex();
    }
}

// FUNCTION: SURRENDER 0x1000F670
srRegistry::ClassNode::~ClassNode()
{
    for (ChildLink* link = first_child_04; link != child_end_08; link = link->next_04) {
        delete link->node_00;
    }
    delete named_instances_18;
    delete instances_by_id_20;
    while (first_child_04 != child_end_08) {
        ChildLink* link = first_child_04;
        first_child_04 = link->next_04;
        if (link->previous_08 != 0) {
            link->previous_08->next_04 = link->next_04;
        }
        if (link->next_04 != 0) {
            link->next_04->previous_08 = link->previous_08;
        }
        delete link;
        --child_count_00;
    }
    delete child_end_08;
}

// FUNCTION: SURRENDER 0x1000F7E0
void srRegistry::ClassNode::enableInstanceLookup()
{
    if (named_instances_18 == 0) {
        named_instances_18 = new NameIndex;
    }
    if (instances_by_id_20 == 0) {
        instances_by_id_20 = new IDIndex;
    }
}

// FUNCTION: SURRENDER 0x1000FED0
unsigned long srRegistry::ClassNode::getClassID() const
{
    return class_id_10;
}

// FUNCTION: SURRENDER 0x10010060
srRegistry::ClassNode* srRegistry::ClassNode::getParent() const
{
    return parent_0c;
}

// FUNCTION: SURRENDER 0x10010070
srRegistry::ClassNode::NameIndex* srRegistry::ClassNode::getNameIndex() const
{
    return named_instances_18 != 0 ? named_instances_18 : inherited_named_instances_1c;
}

// FUNCTION: SURRENDER 0x10010080
srRegistry::ClassNode::IDIndex* srRegistry::ClassNode::getIDIndex() const
{
    return instances_by_id_20 != 0 ? instances_by_id_20 : inherited_instances_by_id_24;
}

// FUNCTION: SURRENDER 0x1000F930
void srRegistry::ClassNode::registerInstance(srRuntimeClass* instance)
{
    if (named_instances_18 != 0 && instance->isNamed()) {
        named_instances_18->add(instance);
    }
    if (instances_by_id_20 != 0) {
        instances_by_id_20->add(instance);
    }
    ++instance_count_28;
}

// FUNCTION: SURRENDER 0x1000FAD0
void srRegistry::ClassNode::refreshInstance(srRuntimeClass* instance)
{
    for (ClassNode* node = this; node != 0; node = node->parent_0c) {
        if (node->named_instances_18 != 0) {
            node->named_instances_18->remove(instance);
            if (instance->isNamed()) {
                node->named_instances_18->add(instance);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1000FCD0
void srRegistry::ClassNode::unregisterInstance(srRuntimeClass* instance)
{
    if (named_instances_18 != 0) {
        named_instances_18->remove(instance);
    }
    if (instances_by_id_20 != 0) {
        instances_by_id_20->remove(instance->getID());
    }
    --instance_count_28;
}

// FUNCTION: SURRENDER 0x100100D0
srRuntimeClass* srRegistry::ClassNode::findByName(ClassNode* requested_class, const char* name,
                                                  int exact, const srRuntimeClass* relative_to)
{
    if (name == 0) {
        return 0;
    }

    NameIndex* index = getNameIndex();
    srRuntimeClass* found = const_cast<srRuntimeClass*>(relative_to);
    if (index == 0) {
        if (exact != 0) {
            return 0;
        }
        ChildLink* child = first_child_04;
        if (relative_to != 0 && child != child_end_08) {
            do {
                srRuntimeClass* hit = child->node_00->findByName(requested_class, name, 0, 0);
                while (hit != 0 && hit != relative_to) {
                    hit = child->node_00->findByName(requested_class, name, 0, hit);
                }
                if (hit != 0) {
                    found = child->node_00->findByName(requested_class, name, 0, relative_to);
                    if (found != 0) {
                        return found;
                    }
                    child = child->next_04;
                    break;
                }
                child = child->next_04;
            } while (child != child_end_08);
        }
        if (child == child_end_08) {
            return 0;
        }
        do {
            found = child->node_00->findByName(requested_class, name, 0, 0);
            if (found != 0) {
                return found;
            }
            child = child->next_04;
        } while (child != child_end_08);
        return 0;
    }

    found = index->find(name, relative_to);
    if (exact == 0) {
        while (found != 0) {
            if (requested_class->isDerivedOrSame(found->getClassNode())) {
                return found;
            }
            found = index->find(name, found);
        }
        return 0;
    }
    while (found != 0) {
        if (requested_class->isSame(found->getClassNode())) {
            return found;
        }
        found = index->find(name, found);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100103B0
srRuntimeClass* srRegistry::ClassNode::findRelative(ClassNode* requested_class, int exact,
                                                    const srRuntimeClass* relative_to)
{
    IDIndex* index = getIDIndex();
    if (index == 0) {
        if (exact == 0) {
            ChildLink* child = first_child_04;
            if (relative_to != 0) {
                if (child == child_end_08) {
                    return 0;
                }
                do {
                    srRuntimeClass* hit = child->node_00->findRelative(requested_class, 0, 0);
                    while (hit != 0 && hit != relative_to) {
                        hit = child->node_00->findRelative(requested_class, 0, hit);
                    }
                    if (hit != 0) {
                        srRuntimeClass* found =
                            child->node_00->findRelative(requested_class, 0, relative_to);
                        if (found != 0) {
                            return found;
                        }
                        child = child->next_04;
                        break;
                    }
                    child = child->next_04;
                } while (child != child_end_08);
            }
            while (child != child_end_08) {
                srRuntimeClass* found = child->node_00->findRelative(requested_class, 0, 0);
                if (found != 0) {
                    return found;
                }
                child = child->next_04;
            }
        }
        return 0;
    }

    IDIndex::InstanceLink* link = index->findRelative(relative_to);
    if (exact == 0) {
        while (link != 0) {
            if (requested_class->isDerivedOrSame(link->instance_00->getClassNode())) {
                return link->instance_00;
            }
            link = link->next_04;
        }
        return 0;
    }
    while (link != 0) {
        if (requested_class->isSame(link->instance_00->getClassNode())) {
            return link->instance_00;
        }
        link = link->next_04;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100104D0
srRuntimeClass* srRegistry::ClassNode::findByID(ClassNode* requested_class, unsigned long id,
                                                int exact)
{
    IDIndex* index = getIDIndex();
    if (index == 0) {
        if (exact == 0) {
            for (ChildLink* child = first_child_04; child != child_end_08; child = child->next_04) {
                srRuntimeClass* found = child->node_00->findByID(requested_class, id, 0);
                if (found != 0) {
                    return found;
                }
            }
        }
        return 0;
    }

    srRuntimeClass* found = index->find(id);
    if (found == 0) {
        return 0;
    }
    ClassNode* found_class = found->getClassNode();
    if (exact != 0 ? requested_class->isSame(found_class)
                   : requested_class->isDerivedOrSame(found_class)) {
        return found;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1000F920
int srRegistry::ClassNode::isSame(ClassNode* other) const
{
    return this == other;
}

// FUNCTION: SURRENDER 0x1000F8E0
int srRegistry::ClassNode::isDerivedOrSame(ClassNode* derived) const
{
    while (true) {
        if (this == derived) {
            return 1;
        }
        if (derived->getParent() == 0) {
            break;
        }
        derived = derived->getParent();
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10010090
long srRegistry::ClassNode::getNumberOfInstances(int exact) const
{
    if (exact == 0) {
        return instance_count_28;
    }

    long children = 0;
    for (ChildLink* child = first_child_04; child != child_end_08; child = child->next_04) {
        children += child->node_00->getNumberOfInstances(0);
    }
    return instance_count_28 - children;
}

// FUNCTION: SURRENDER 0x1000E240
void srClass::addReference() const
{
    ++reference_count_0c;
}

// FUNCTION: SURRENDER 0x1000E250
void srClass::autoRelease()
{
    if (reference_count_0c == 1) {
        reference_count_0c = 0;
    }
}

// FUNCTION: SURRENDER 0x1000E260
int srClass::release() const
{
    /* VC6 member functions can be invoked with a null this. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
    if (this == 0) {
#pragma clang diagnostic pop
        return 1;
    }

    if (--reference_count_0c <= 0) {
        delete this;
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1000E840
long srClass::getReferenceCount() const
{
    return reference_count_0c;
}
