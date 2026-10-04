#include "surrender/srTypeRegistry.h"

#include "surrender/srArray.h"
#include "surrender/srDebug.h"
#include "surrender/srHash.h"

#include <ostream>
#include <string.h>

/* The assert strings carry the original build tree's __FILE__ expansions,
   not this checkout's layout. */
#define SRRUNTIMECLASS_CPP "D:\\srsdk1x\\sources\\corelib\\srRuntimeClass.cpp"
#define SRCLASS_CPP "D:\\srsdk1x\\sources\\corelib\\srClass.cpp"

namespace {
unsigned long next_instance_id = 1;

unsigned long hashName(const char* name)
{
    unsigned long hash = 0;
    for (unsigned long index = 0; name[index] != '\0'; ++index) {
        hash += (index + 0x4ad) * static_cast<signed char>(name[index]);
    }
    return hash;
}
} // namespace

struct srRegistry::ClassNode::NameIndex {
    struct NameEntry {
        NameEntry* next;
        NameEntry* previous;
        unsigned long bucket;
        const char* name;
        srRuntimeClass* instance;
    };

    NameIndex()
        : entries(0), free(0), buckets(0), count(0), bucket_count(0),
          case_sensitive(1)
    {
        resize(4);
    }

    ~NameIndex()
    {
        if (buckets != 0) {
            ::operator delete(buckets);
        }
        if (entries != 0) {
            ::operator delete(entries);
        }
        buckets = 0;
        entries = 0;
        free = 0;
        count = 0;
        bucket_count = 0;
        by_instance.Clear();
    }

    // FUNCTION: SURRENDER 0x10010E70
    int namesEqual(const char* first, const char* second) const
    {
        return case_sensitive != 0 ? strcmp(first, second) == 0 : _stricmp(first, second) == 0;
    }

    void add(srRuntimeClass* instance)
    {
        const char* name = instance->getName();
        if (name == 0) {
            return;
        }
        if (free == 0) {
            resize(bucket_count * 2);
        }

        NameEntry* entry = free;
        free = entry->next;
        /* Retail clears the popped entry's link before reconfiguring it
           (0x1000F97B), even though the bucket push below overwrites it. */
        entry->next = 0;
        unsigned long bucket = bucketIndex(name);
        entry->bucket = bucket;
        entry->name = name;
        entry->instance = instance;
        entry->previous = 0;
        entry->next = buckets[bucket];
        if (entry->next != 0) {
            entry->next->previous = entry;
        }
        buckets[bucket] = entry;
        by_instance.Insert(&instance, &entry);
        ++count;
    }

    void remove(srRuntimeClass* instance)
    {
        NameEntry* entry = by_instance.Lookup(&instance);
        if (entry == 0) {
            return;
        }
        by_instance.Remove(&entry->instance);
        if (entry->previous == 0) {
            buckets[entry->bucket] = entry->next;
        } else {
            entry->previous->next = entry->next;
        }
        if (entry->next != 0) {
            entry->next->previous = entry->previous;
        }
        entry->previous = 0;
        entry->name = 0;
        entry->instance = 0;
        entry->next = free;
        free = entry;
        --count;
        if (bucket_count > 7 && count <= bucket_count / 4) {
            resize(bucket_count / 2);
        }
    }

    /* Retail emits bucketIndex as a callable body (0x10010EF0), so its
       definition sits out-of-line below the struct. */
    unsigned long bucketIndex(const char* name) const;

    srRuntimeClass* find(const char* name, const srRuntimeClass* relative_to) const
    {
        if (relative_to != 0) {
            srRuntimeClass* relative_key = const_cast<srRuntimeClass*>(relative_to);
            NameEntry* entry = by_instance.Lookup(&relative_key);
            if (entry == 0) {
                /* Retail returns the bucket head without namesEqual when the
                   relative instance is absent from the side index. */
                entry = buckets[bucketIndex(name)];
                return entry == 0 ? 0 : entry->instance;
            }
            for (entry = entry->next; entry != 0; entry = entry->next) {
                if (namesEqual(name, entry->name)) {
                    return entry->instance;
                }
            }
            return 0;
        }
        for (NameEntry* entry = buckets[bucketIndex(name)]; entry != 0; entry = entry->next) {
            if (namesEqual(name, entry->name)) {
                return entry->instance;
            }
        }
        return 0;
    }

private:
    // FUNCTION: SURRENDER 0x10010F30
    void resize(unsigned long bucket_count)
    {
        unsigned long old_bucket_count = this->bucket_count;
        this->bucket_count = bucket_count;
        free = 0;
        /* Retail clears by_instance rather than rehashing it: every live
           instance is re-inserted with its new NameEntry below, so preserving
           the old mappings would leave duplicate keys pointing into the freed
           entry array (0x10010F30 calls 0x10011380, srHashTable::Clear). */
        by_instance.Clear();
        NameEntry* entries = 0;
        NameEntry** buckets = 0;
        if (bucket_count != 0) {
            entries = static_cast<NameEntry*>(::operator new(bucket_count * sizeof(NameEntry)));
            buckets = static_cast<NameEntry**>(::operator new(bucket_count * sizeof(NameEntry*)));
            for (unsigned long index = 0; index < bucket_count; ++index) {
                buckets[index] = 0;
                entries[index].next = &entries[index + 1];
                entries[index].previous = 0;
                entries[index].bucket = 0;
                entries[index].name = 0;
                entries[index].instance = 0;
            }
            entries[bucket_count - 1].next = 0;
            free = entries;
            if (this->buckets != 0 && old_bucket_count != 0) {
                for (unsigned long bucket = 0; bucket < old_bucket_count; ++bucket) {
                    for (NameEntry* entry = this->buckets[bucket]; entry != 0;
                         entry = entry->next) {
                        const char* name = entry->name;
                        if (free == 0) {
                            resize(this->bucket_count * 2);
                        }
                        NameEntry* reused = free;
                        free = reused->next;
                        unsigned long new_bucket = bucketIndex(name);
                        reused->bucket = new_bucket;
                        reused->name = name;
                        reused->instance = entry->instance;
                        reused->previous = 0;
                        reused->next = buckets[new_bucket];
                        if (reused->next != 0) {
                            reused->next->previous = reused;
                        }
                        buckets[new_bucket] = reused;
                        by_instance.Insert(&entry->instance, &reused);
                    }
                }
            }
        }
        if (this->buckets != 0) {
            ::operator delete(this->buckets);
        }
        if (this->entries != 0) {
            ::operator delete(this->entries);
        }
        this->buckets = 0;
        this->entries = 0;
        if (bucket_count != 0) {
            this->buckets = buckets;
            this->entries = entries;
        }
    }

    srHashTable<srRuntimeClass*, NameEntry*> by_instance;
    NameEntry* entries;
    NameEntry* free;
    NameEntry** buckets;
    unsigned long count;
    unsigned long bucket_count;
    int case_sensitive;
};

static_assert(sizeof(srRegistry::ClassNode::NameIndex) == 0x28,
              "srRegistry_ClassNode_NameIndex_must_be_0x28");

// FUNCTION: SURRENDER 0x10010EF0
unsigned long srRegistry::ClassNode::NameIndex::bucketIndex(const char* name) const
{
    return hashName(name) & (bucket_count - 1);
}

struct srRegistry::ClassNode::IDIndex {
    struct InstanceLink {
        union {
            srRuntimeClass* instance;
            InstanceLink* free;
        };
        InstanceLink* next;
        InstanceLink* previous;
        unsigned long unused;
    };
    static_assert(sizeof(InstanceLink) == 0x10,
                  "srRegistry_ClassNode_IDIndex_InstanceLink_must_be_0x10");

    IDIndex()
        : active_count(0), free(0), block_count(0), first(0), last(0),
          list_count(0)
    {
    }

    /* Retail ~IDIndex (0x100109F0) is a callable body invoked by delete
       expressions, so its definition sits out-of-line below the struct. */
    ~IDIndex();

    /* Standalone full teardown (0x10010670): drains the link list, clears
       the block pool, then runs the destructor in place. Retail never
       calls it — ~ClassNode performs the same steps manually. */
    void destroy();

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
        unsigned long id = instance->getID();
        InstanceLink* link = insert(0, instance);
        by_id.Insert(&id, &link);
        return link;
    }

    void remove(unsigned long id)
    {
        InstanceLink* link = by_id.Lookup(&id);
        if (link == 0) {
            return;
        }
        by_id.Remove(&id);
        if (link->previous == 0) {
            first = link->next;
        } else {
            link->previous->next = link->next;
        }
        if (link->next == 0) {
            last = link->previous;
        } else {
            link->next->previous = link->previous;
        }
        --active_count;
        link->free = free;
        free = link;
        if (active_count == 0) {
            clearBlocks();
        }
        --list_count;
    }

    srRuntimeClass* find(unsigned long id) const
    {
        InstanceLink* link = by_id.Lookup(&id);
        return link == 0 ? 0 : link->instance;
    }

    InstanceLink* findRelative(const srRuntimeClass* relative_to) const
    {
        InstanceLink* link = first;
        if (relative_to != 0) {
            while (link != 0) {
                srRuntimeClass* instance = link->instance;
                link = link->next;
                if (instance == relative_to) {
                    break;
                }
            }
        }
        return link;
    }

    /* ~ClassNode tears an IDIndex down in place (by_id release,
       clearLinks, delete) rather than through a single call. */
    friend class srRegistry::ClassNode;

private:
    /* Retail emits insert as a callable body (0x100107E0), so its definition
       sits out-of-line below the struct. */
    InstanceLink* insert(InstanceLink* after, srRuntimeClass*& instance);

    void allocateBlock()
    {
        /* Retail compares signed (0xff < (int)count) and clamps counts
           at 0x100, not above it. */
        int count = active_count < 2 ? 1 : active_count;
        if (count > 0xff) {
            count = 0x100;
        }
        InstanceLink* block =
            static_cast<InstanceLink*>(srHeap.allocate(count * sizeof(InstanceLink)));
        free = block;
        unsigned long index = block_count;
        block_count = index + 1;
        blocks[index] = block;
        for (int i = 0; i < count; ++i) {
            block[i].free = &block[i + 1];
        }
        block[count - 1].free = 0;
    }

    /* Retail emits clearLinks as a callable body (0x100108E0), so its
       definition sits out-of-line below the struct. */
    void clearLinks();

    /* Retail clearBlocks (0x10010A90) resets only the block bookkeeping and
       active_count; it deliberately does not touch first, last or
       list_count. Callers update the list head/tail before the count hits
       zero, and remove() decrements list_count after this call, so the
       counters stay consistent — zeroing them here would underflow the
       post-call decrement to 0xffffffff. */
    void clearBlocks();

    unsigned long active_count;
    InstanceLink* free;
    srArray<InstanceLink*> blocks;
    unsigned long block_count;
    InstanceLink* first;
    InstanceLink* last;
    unsigned long list_count;
    /* Dtorless: ~IDIndex (0x100109F0) runs no hash teardown, and ~ClassNode
       (0x1000F73C-0x1000F759) releases the two arrays before the link/block
       teardown. */
    srHashTableBase<unsigned long, InstanceLink*> by_id;
};

static_assert(sizeof(srRegistry::ClassNode::IDIndex) == 0x30,
              "srRegistry_ClassNode_IDIndex_must_be_0x30");

// FUNCTION: SURRENDER 0x100107E0
srRegistry::ClassNode::IDIndex::InstanceLink*
srRegistry::ClassNode::IDIndex::insert(InstanceLink* after, srRuntimeClass*& instance)
{
    if (free == 0) {
        allocateBlock();
    }
    InstanceLink* link = free;
    free = link->free;
    ++active_count;
    link->instance = instance;
    if (after == 0) {
        link->previous = 0;
        link->next = first;
    } else {
        link->previous = after;
        link->next = after->next;
        after->next = link;
    }
    if (link->next != 0) {
        link->next->previous = link;
    }
    if (link->previous == 0) {
        first = link;
    }
    if (link->next == 0) {
        last = link;
    }
    ++list_count;
    return link;
}

// FUNCTION: SURRENDER 0x10010A90
void srRegistry::ClassNode::IDIndex::clearBlocks()
{
    for (unsigned long index = 0; index < block_count; ++index) {
        srHeap.free(blocks[index]);
    }
    blocks.release();
    free = 0;
    block_count = 0;
    active_count = 0;
}

// FUNCTION: SURRENDER 0x100108E0
void srRegistry::ClassNode::IDIndex::clearLinks()
{
    while (first != 0) {
        InstanceLink* link = first;
        if (link->previous == 0) {
            first = link->next;
        } else {
            link->previous->next = link->next;
        }
        if (link->next == 0) {
            last = link->previous;
        } else {
            link->next->previous = link->previous;
        }
        /* Retail guards the unlink bookkeeping with link != 0 even though
           link was just taken from the non-null first. */
        if (link != 0) {
            --active_count;
            link->free = free;
            free = link;
            if (active_count == 0) {
                clearBlocks();
            }
        }
        --list_count;
    }
    clearBlocks();
}

/* member-dtor-ok: the body is clearBlocks and the implicit ~srArray member
   teardown releases blocks again; by_id is a dtorless srHashTableBase so no
   hash teardown follows; ~ClassNode (0x1000F772) and the delete-expression
   unwind funclets call it. */
// FUNCTION: SURRENDER 0x100109F0
srRegistry::ClassNode::IDIndex::~IDIndex()
{
    clearBlocks();
}

// FUNCTION: SURRENDER 0x10010670
void srRegistry::ClassNode::IDIndex::destroy()
{
    clearLinks();
    clearBlocks();
    // member-dtor-ok: retail 0x10010670 runs clearBlocks and then repeats the
    // ~IDIndex body plus its member teardown inline — the authored op is an
    // in-place destructor call on a live index.
    this->~IDIndex();
}

struct srRegistry::ClassIndex : srHashTable<unsigned long, srRegistry::ClassNode*> {};

// GLOBAL: SURRENDER 0x100A45AC
unsigned long srClass::_timestampCtr;

// GLOBAL: SURRENDER 0x100A45B0
srClass::Update* srClass::_firstUpdate;

// GLOBAL: SURRENDER 0x100A45B8
double srClass::_lastUpdateTime;

// FUNCTION: SURRENDER 0x10011790
const char* srRuntimeClass::getName() const
{
    return name != 0 ? name : "anonymous";
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
    return name != 0;
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
    if (this->name != 0) {
        delete[] this->name;
    }
    if (name == 0 || *name == '\0') {
        this->name = 0;
    } else {
        this->name = new char[strlen(name) + 1];
        strcpy(this->name, name);
    }
    srCore.getRegistry()->refreshInstance(getClassNode(), this);
}

/* Retail writes the vptr before the member values and reuses the registry
   pointer. Constructor spelling and the presence of a named local are unresolved. */
// FUNCTION: SURRENDER 0x100119D0
srRuntimeClass::srRuntimeClass()
{
    name = 0;
    srRegistry* registry = srCore.getRegistry();
    id = registry->allocateID();
    registry->registerInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x10011A40
srRuntimeClass::~srRuntimeClass()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
    if (name != 0) {
        delete[] name;
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
    return id;
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
    if (reference_count < 0) {
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
srClass::srClass() : reference_count(1), update(0)
{
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    touch();
}

/* Retail ~srClass unregisters the instance and proceeds to base teardown;
   no update unlink or deletion is emitted here. */
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
    return update == 0 ? 0 : update->callback;
}

// FUNCTION: SURRENDER 0x1000E360
double srClass::getUpdateInterval()
{
    return update == 0 ? 0.0 : update->interval;
}

// FUNCTION: SURRENDER 0x1000E380
void srClass::setUpdate(UpdateCallBack callback, double interval)
{
    if (update != 0 || callback != 0) {
        if (update != 0) {
            if (callback != 0) {
                update->callback = callback;
                update->interval = interval;
                return;
            }

            /* Retail re-tests update after the reuse-early-return and runs
               the unlink plus delete only inside that second guard. */
            if (update != 0) {
                if (update->previous != 0) {
                    update->previous->next = update->next;
                }
                if (update->next != 0) {
                    update->next->previous = update->previous;
                }
                if (update == _firstUpdate) {
                    _firstUpdate = update->next;
                }
                delete update;
                update = 0;
            }
        }

        if (callback != 0) {
            update = new Update;
            update->instance = this;
            update->callback = callback;
            update->interval = interval;
            update->last_update_time = _lastUpdateTime;
            update->next = _firstUpdate;
            update->previous = 0;
            if (update->next != 0) {
                update->next->previous = update;
            }
            _firstUpdate = update;
        }
    }
}

// FUNCTION: SURRENDER 0x1000E490
void srClass::setUpdatesTime(double time)
{
    _lastUpdateTime = time;
    for (Update* update = _firstUpdate; update != 0; update = update->next) {
        update->last_update_time = time;
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
            Update* next = update->next;
            if (update->interval <= 0.0) {
                update->callback(update->instance, time, time - _lastUpdateTime);
                update->last_update_time = time;
            } else {
                for (double update_time = update->last_update_time + update->interval;
                     update_time <= time; update_time += update->interval) {
                    update->callback(update->instance, update_time,
                                        update_time - update->last_update_time);
                    update->last_update_time = update_time;
                }
            }
            update = next;
        }
        _lastUpdateTime = time;
    }
}

// FUNCTION: SURRENDER 0x1000E5E0
void srClass::touch()
{
    timestamp = ++_timestampCtr;
}

// FUNCTION: SURRENDER 0x1000E5F0
unsigned long srClass::getTimestamp() const
{
    return timestamp;
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
    if (update != 0) {
        if (update->interval == 0.0) {
            stream << "  Update interval: " << "every frame" << '\n';
        } else {
            stream << "  Update interval: " << update->interval << "secs" << '\n';
        }
        stream.width(0x20);
        // c-style-cast-ok: retail prints the callback through the void* overload
        stream << "    Update callback: " << (void*)update->callback << '\n';
        if (update->instance != 0) {
            stream.width(0x20);
            stream << "    Update owner: ";
            update->instance->getUniqueName(stream);
            stream << '\n';
        }
        if (update->previous != 0) {
            stream.width(0x20);
            stream << "    Update previous: " << update->previous << '\n';
        }
        if (update->next != 0) {
            stream.width(0x20);
            stream << "    Update next: " << update->next << '\n';
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

// FUNCTION: SURRENDER 0x1000E860
srClass* srClass::clone()
{
    return vClone();
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

/* Retail 0x1000E910 assigns every member in the body: the critical section
   new runs before the access guard is taken, and root/class_index/
   valid are never zero-initialized ahead of it. */
// FUNCTION: SURRENDER 0x1000E910
srRegistry::srRegistry()
{
    critical_section = new srCriticalSection;
    srCriticalSectionAccess access(critical_section);
    class_index = new ClassIndex;
    root = new ClassNode(0, "root", 0);
    unsigned long root_id = 0;
    class_index->Insert(&root_id, &root);
    valid = 1;
}

// FUNCTION: SURRENDER 0x1000EA40
unsigned long srRegistry::getClassID(ClassNode* node)
{
    srCriticalSectionAccess access(critical_section);
    return node->getClassID();
}

// FUNCTION: SURRENDER 0x1000EAA0
const char* srRegistry::getClassName(ClassNode* node)
{
    srCriticalSectionAccess access(critical_section);
    return node->class_name;
}

// FUNCTION: SURRENDER 0x1000EAD0
srRegistry::~srRegistry()
{
    {
        srCriticalSectionAccess access(critical_section);
        delete root;
        root = 0;
        delete class_index;
        class_index = 0;
        valid = 0;
    }
    /* Retail drains the lock once more before destroying the section, all
       under one null check (0x1000EB66-0x1000EB7E): getAccess/releaseAccess
       complete before DeleteCriticalSection runs. */
    if (critical_section != 0) {
        critical_section->getAccess();
        critical_section->releaseAccess();
        delete critical_section;
    }
}

// FUNCTION: SURRENDER 0x1000EBD0
srRegistry::ClassNode* srRegistry::getClassNode(unsigned long class_id)
{
    srCriticalSectionAccess access(critical_section);
    if (class_id == 0) {
        return 0;
    }
    return class_index->Lookup(&class_id);
}

// FUNCTION: SURRENDER 0x1000EC60
srRegistry::ClassNode* srRegistry::registerClass(const char* class_name, ClassNode* parent,
                                                 unsigned long class_id, int register_instances)
{
    srCriticalSectionAccess access(critical_section);
    ClassNode* node = class_index->Lookup(&class_id);
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

// FUNCTION: SURRENDER 0x1000ED40
void srRegistry::dumpClassHierarchy(std::ostream& stream)
{
    srCriticalSectionAccess access(critical_section);
    for (ClassNode::ChildLink* link = root->children.first;
         link != root->children.last; link = link->next) {
        link->node->dump(stream, 0);
    }
}

// FUNCTION: SURRENDER 0x1000EDB0
srRegistry::ClassNode* srRegistry::addToTree(ClassNode* parent, const char* class_name,
                                             unsigned long class_id)
{
    srCriticalSectionAccess access(critical_section);
    ClassNode* node = new ClassNode(parent, class_name, class_id);
    class_index->Insert(&class_id, &node);
    return node;
}

// FUNCTION: SURRENDER 0x1000EEA0
void srRegistry::dumpInstanceNames(ClassNode* node, std::ostream& stream, int indent)
{
    srCriticalSectionAccess access(critical_section);
    // c-style-cast-ok: the recovered overload set requires an explicit null-pointer type
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
    srCriticalSectionAccess access(critical_section);
    node->registerInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F010
void srRegistry::refreshInstance(ClassNode* node, srRuntimeClass* instance)
{
    srCriticalSectionAccess access(critical_section);
    node->refreshInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F070
srRuntimeClass* srRegistry::find(ClassNode* node, const char* name,
                                 const srRuntimeClass* relative_to)
{
    srCriticalSectionAccess access(critical_section);
    return node->findByName(node, name, 0, relative_to);
}

// FUNCTION: SURRENDER 0x1000F0E0
srRuntimeClass* srRegistry::findExact(ClassNode* node, const char* name,
                                      const srRuntimeClass* relative_to)
{
    srCriticalSectionAccess access(critical_section);
    return node->findByName(node, name, 1, relative_to);
}

// FUNCTION: SURRENDER 0x1000F150
srRuntimeClass* srRegistry::find(ClassNode* node, unsigned long id)
{
    srCriticalSectionAccess access(critical_section);
    return node->findByID(node, id, 0);
}

// FUNCTION: SURRENDER 0x1000F1B0
srRuntimeClass* srRegistry::findExact(ClassNode* node, unsigned long id)
{
    srCriticalSectionAccess access(critical_section);
    return node->findByID(node, id, 1);
}

// FUNCTION: SURRENDER 0x1000F210
void srRegistry::unregisterInstance(ClassNode* node, srRuntimeClass* instance)
{
    srCriticalSectionAccess access(critical_section);
    node->unregisterInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F270
int srRegistry::isDerivedOrSame(ClassNode* base, ClassNode* derived)
{
    srCriticalSectionAccess access(critical_section);
    if (base != 0 && derived != 0) {
        return base->isDerivedOrSame(derived);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1000F2F0
srRuntimeClass* srRegistry::findExact(ClassNode* node, const srRuntimeClass* relative_to)
{
    srCriticalSectionAccess access(critical_section);
    return node->findRelative(node, 1, relative_to);
}

// FUNCTION: SURRENDER 0x1000F350
srRuntimeClass* srRegistry::find(ClassNode* node, const srRuntimeClass* relative_to)
{
    srCriticalSectionAccess access(critical_section);
    return node->findRelative(node, 0, relative_to);
}

// FUNCTION: SURRENDER 0x1000F3B0
srRegistry::ClassNode* srRegistry::getRootClass()
{
    srCriticalSectionAccess access(critical_section);
    return root->children.first->node;
}

// FUNCTION: SURRENDER 0x1000F3E0
srRegistry::ClassNode* srRegistry::getChildClass(ClassNode* parent, ClassNode* child)
{
    srCriticalSectionAccess access(critical_section);
    ClassNode::ChildLink* link = parent->children.first;
    ClassNode* result = 0;
    if (child == 0) {
        if (link != parent->children.last) {
            result = link->node;
        }
    } else if (link != parent->children.last) {
        while (link->node != child || link->next == parent->children.last) {
            link = link->next;
            if (link == parent->children.last) {
                return 0;
            }
        }
        result = link->next->node;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1000F450
int srRegistry::checkValidity()
{
    return valid != 0;
}

// FUNCTION: SURRENDER 0x1000F460
long srRegistry::getNumberOfInstances(ClassNode* node, int exact)
{
    srCriticalSectionAccess access(critical_section);
    return node->getNumberOfInstances(exact);
}

// FUNCTION: SURRENDER 0x1000F4C0
unsigned long srRegistry::allocateID()
{
    srCriticalSectionAccess access(critical_section);
    return next_instance_id++;
}

// FUNCTION: SURRENDER 0x100105C0
srRegistry::ClassNode* srRegistry::getRootNode()
{
    srCriticalSectionAccess access(critical_section);
    return root;
}

// FUNCTION: SURRENDER 0x1000F580
srRegistry::ClassNode::ClassNode(ClassNode* parent, const char* class_name, unsigned long class_id)
    : children()
{
    initialize(parent, class_name, class_id);
}

// FUNCTION: SURRENDER 0x1000F5F0
void srRegistry::ClassNode::initialize(ClassNode* parent, const char* class_name,
                                       unsigned long class_id)
{
    this->class_id = class_id;
    this->parent = parent;
    this->class_name = class_name;
    named_instances = 0;
    inherited_named_instances = 0;
    instances_by_id = 0;
    inherited_instances_by_id = 0;
    instance_count = 0;
    if (parent != 0) {
        ChildLink* link = new ChildLink;
        link->next = parent->children.first;
        link->node = this;
        link->previous = parent->children.first->previous;
        if (link->previous == 0) {
            parent->children.first = link;
        } else {
            link->previous->next = link;
        }
        if (link->next != 0) {
            link->next->previous = link;
        }
        ++parent->children.count;
        inherited_named_instances = parent->getNameIndex();
        inherited_instances_by_id = parent->getIDIndex();
    }
}

// FUNCTION: SURRENDER 0x1000F670
srRegistry::ClassNode::~ClassNode()
{
    for (ChildLink* link = children.first; link != children.last;
         link = link->next) {
        delete link->node;
    }
    delete named_instances;
    /* Retail releases by_id's storage manually — srHashTableBase has no
       destructor — then unlinks and deletes the index object. */
    IDIndex* index = instances_by_id;
    if (index != 0) {
        index->by_id.Release();
        index->clearLinks();
        delete index;
    }
}

// FUNCTION: SURRENDER 0x1000F7E0
void srRegistry::ClassNode::enableInstanceLookup()
{
    if (named_instances == 0) {
        named_instances = new NameIndex;
    }
    if (instances_by_id == 0) {
        instances_by_id = new IDIndex;
    }
}

// FUNCTION: SURRENDER 0x1000FEE0
void srRegistry::ClassNode::dump(std::ostream& stream, int indent)
{
    int i;
    for (i = indent; i != 0; i--) {
        stream << ' ';
    }
    stream << "Class name: " << class_name << '\n';
    for (i = indent; i != 0; i--) {
        stream << ' ';
    }
    /* reinterpret-ok: retail prints the numeric class id through
       operator<<(const void*). */
    stream << "Class Id: " << reinterpret_cast<const void*>(class_id) << '\n';
    for (i = indent; i != 0; i--) {
        stream << ' ';
    }
    stream << "Num children: " << children.count << '\n';
    for (i = indent; i != 0; i--) {
        stream << ' ';
    }
    stream << "Hash: " << named_instances << '\n';
    for (i = indent; i != 0; i--) {
        stream << ' ';
    }
    stream << "Nearest parent hash: " << inherited_named_instances << '\n';
    for (ChildLink* link = children.first; link != children.last;
         link = link->next) {
        link->node->dump(stream, indent + 2);
    }
}

// FUNCTION: SURRENDER 0x1000FED0
unsigned long srRegistry::ClassNode::getClassID() const
{
    return class_id;
}

// FUNCTION: SURRENDER 0x10010060
srRegistry::ClassNode* srRegistry::ClassNode::getParent() const
{
    return parent;
}

// FUNCTION: SURRENDER 0x10010070
srRegistry::ClassNode::NameIndex* srRegistry::ClassNode::getNameIndex() const
{
    return named_instances != 0 ? named_instances : inherited_named_instances;
}

// FUNCTION: SURRENDER 0x10010080
srRegistry::ClassNode::IDIndex* srRegistry::ClassNode::getIDIndex() const
{
    return instances_by_id != 0 ? instances_by_id : inherited_instances_by_id;
}

// FUNCTION: SURRENDER 0x1000F930
void srRegistry::ClassNode::registerInstance(srRuntimeClass* instance)
{
    if (named_instances != 0 && instance->isNamed()) {
        named_instances->add(instance);
    }
    if (instances_by_id != 0) {
        instances_by_id->add(instance);
    }
    ++instance_count;
}

// FUNCTION: SURRENDER 0x1000FAD0
void srRegistry::ClassNode::refreshInstance(srRuntimeClass* instance)
{
    for (ClassNode* node = this; node != 0; node = node->parent) {
        if (node->named_instances != 0) {
            node->named_instances->remove(instance);
            if (instance->isNamed()) {
                node->named_instances->add(instance);
            }
        }
    }
}

// FUNCTION: SURRENDER 0x1000FCD0
void srRegistry::ClassNode::unregisterInstance(srRuntimeClass* instance)
{
    if (named_instances != 0) {
        named_instances->remove(instance);
    }
    if (instances_by_id != 0) {
        instances_by_id->remove(instance->getID());
    }
    --instance_count;
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
        ChildLink* child = children.first;
        if (relative_to != 0) {
            while (child != children.last) {
                srRuntimeClass* hit = child->node->findByName(requested_class, name, 0, 0);
                while (hit != 0) {
                    if (hit == relative_to) {
                        break;
                    }
                    hit = child->node->findByName(requested_class, name, 0, hit);
                }
                if (hit == relative_to) {
                    found = child->node->findByName(requested_class, name, 0, relative_to);
                    if (found != 0) {
                        return found;
                    }
                    child = child->next;
                    break;
                }
                child = child->next;
            }
        }
        while (child != children.last) {
            found = child->node->findByName(requested_class, name, 0, 0);
            if (found != 0) {
                return found;
            }
            child = child->next;
        }
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
            ChildLink* child = children.first;
            if (relative_to != 0) {
                if (child == children.last) {
                    return 0;
                }
                do {
                    srRuntimeClass* hit = child->node->findRelative(requested_class, 0, 0);
                    while (hit != 0) {
                        if (hit == relative_to) {
                            break;
                        }
                        hit = child->node->findRelative(requested_class, 0, hit);
                    }
                    if (hit == relative_to) {
                        srRuntimeClass* found =
                            child->node->findRelative(requested_class, 0, relative_to);
                        if (found != 0) {
                            return found;
                        }
                        child = child->next;
                        break;
                    }
                    child = child->next;
                } while (child != children.last);
            }
            while (child != children.last) {
                srRuntimeClass* found = child->node->findRelative(requested_class, 0, 0);
                if (found != 0) {
                    return found;
                }
                child = child->next;
            }
        }
        return 0;
    }

    IDIndex::InstanceLink* link = index->findRelative(relative_to);
    if (exact == 0) {
        while (link != 0) {
            if (requested_class->isDerivedOrSame(link->instance->getClassNode())) {
                return link->instance;
            }
            link = link->next;
        }
        return 0;
    }
    while (link != 0) {
        if (requested_class->isSame(link->instance->getClassNode())) {
            return link->instance;
        }
        link = link->next;
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
            for (ChildLink* child = children.first; child != children.last;
                 child = child->next) {
                srRuntimeClass* found = child->node->findByID(requested_class, id, 0);
                if (found != 0) {
                    return found;
                }
            }
        }
        return 0;
    }

    IDIndex::InstanceLink* link = index->by_id.Lookup(&id);
    if (link != 0) {
        srRuntimeClass* found = link->instance;
        if (exact == 0) {
            if (requested_class->isDerivedOrSame(found->getClassNode())) {
                return found;
            }
        } else {
            if (requested_class->isSame(found->getClassNode())) {
                return found;
            }
        }
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
        return instance_count;
    }

    long children = 0;
    for (ChildLink* child = this->children.first; child != this->children.last;
         child = child->next) {
        children += child->node->getNumberOfInstances(0);
    }
    return instance_count - children;
}

// FUNCTION: SURRENDER 0x1000E240
void srClass::addReference() const
{
    ++reference_count;
}

// FUNCTION: SURRENDER 0x1000E250
void srClass::autoRelease()
{
    if (reference_count == 1) {
        reference_count = 0;
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

    if (--reference_count <= 0) {
        delete this;
        return 1;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1000E840
long srClass::getReferenceCount() const
{
    return reference_count;
}

/* Retail 0x10010780 drains the {count, first, last} sentinel list
   embedded at ClassNode+0x00, freeing each ChildLink through operator delete;
   exception-unwind funclets call this ClassNode::ChildList destructor. */

/* member-dtor-ok: ~IDIndex (retail 0x100109F0) — the body is clearBlocks and
   the implicit ~srArray member teardown releases blocks again; by_id
   is a dtorless srHashTableBase so no hash teardown follows; ~ClassNode (0x1000F772)
   and the delete-expression unwind funclets call it. */

/* Funclet-invoked on this+8 during the IDIndex constructor unwind: the
   blocks member destructor. */

/* Retail calls this Remove emission for by_instance from the unregister
   and refresh paths; the by_id Remove is inlined at its call sites. */

/* Called on the fresh NameIndex's by_instance from the instance-index
   setup path (0x1000F82D) and from the inlined AllocateEntry inside the
   register path (0x1000FC5E). */

/* AllocateEntry emits standalone for by_instance: its body is the
   free_head == -1 guard, the inlined Grow, then the free-slot pop. Called
   from the inherited-instance population loop (0x1000F9F6) and resize's
   reinsert path (0x100111B4). */
