#include "surrender/srTypeRegistry.h"

#include "surrender/srDebug.h"

#include <string.h>

namespace {
unsigned long next_instance_id = 1;

unsigned long hashInteger(unsigned long value)
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

template <class Key, class Value>
class RegistryHash {
public:
    struct Entry {
        int next_00;
        Key key_04;
        Value value_08;
    };

    RegistryHash()
        : buckets_00(0), entries_04(0), free_08(-1), bucket_count_0c(0)
    {
        resize(4);
    }

    ~RegistryHash()
    {
        delete[] buckets_00;
        delete[] entries_04;
    }

    Value find(Key key) const
    {
        if (bucket_count_0c == 0) {
            return 0;
        }
        int entry = buckets_00[hashInteger(
            (unsigned long)key) & (bucket_count_0c - 1)];
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
        if (free_08 == -1) {
            resize(bucket_count_0c * 2);
        }
        int entry = free_08;
        free_08 = entries_04[entry].next_00;
        unsigned long bucket = hashInteger(
            (unsigned long)key) & (bucket_count_0c - 1);
        entries_04[entry].key_04 = key;
        entries_04[entry].value_08 = value;
        entries_04[entry].next_00 = buckets_00[bucket];
        buckets_00[bucket] = entry;
    }

    Value erase(Key key)
    {
        if (bucket_count_0c == 0) {
            return 0;
        }
        unsigned long bucket = hashInteger(
            (unsigned long)key) & (bucket_count_0c - 1);
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
        delete[] buckets_00;
        delete[] entries_04;
        buckets_00 = 0;
        entries_04 = 0;
        free_08 = -1;
        bucket_count_0c = 0;
        resize(4);
    }

private:
    void resize(unsigned long count)
    {
        if (count < 4) {
            count = 4;
        }
        int* buckets = new int[count];
        Entry* entries = new Entry[count];
        for (unsigned long index = 0; index < count; ++index) {
            buckets[index] = -1;
            entries[index].next_00 = -1;
        }

        unsigned long used = 0;
        if (bucket_count_0c != 0) {
            for (unsigned long bucket = 0; bucket < bucket_count_0c; ++bucket) {
                for (int old = buckets_00[bucket]; old != -1;
                     old = entries_04[old].next_00) {
                    entries[used].key_04 = entries_04[old].key_04;
                    entries[used].value_08 = entries_04[old].value_08;
                    unsigned long next_bucket = hashInteger(
                        (unsigned long)entries[used].key_04) &
                        (count - 1);
                    entries[used].next_00 = buckets[next_bucket];
                    buckets[next_bucket] = used++;
                }
            }
            delete[] buckets_00;
            delete[] entries_04;
        }

        for (unsigned long free_index = used; free_index + 1 < count;
             ++free_index) {
            entries[free_index].next_00 = free_index + 1;
        }
        entries[count - 1].next_00 = -1;
        buckets_00 = buckets;
        entries_04 = entries;
        free_08 = used;
        bucket_count_0c = count;
    }

    int* buckets_00;
    Entry* entries_04;
    int free_08;
    unsigned long bucket_count_0c;
};

static_assert(sizeof(RegistryHash<unsigned long, void*>) == 0x10,
              "RegistryHash_must_be_0x10");

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
}

struct srRegistry::ClassNode::NameIndex {
    struct NameEntry {
        NameEntry* next_00;
        NameEntry* previous_04;
        unsigned long bucket_08;
        const char* name_0c;
        srRuntimeClass* instance_10;
    };

    NameIndex()
        : entries_10(0),
          free_14(0),
          buckets_18(0),
          count_1c(0),
          bucket_count_20(0),
          case_sensitive_24(1)
    {
        resize(4);
    }

    ~NameIndex()
    {
        delete[] buckets_18;
        delete[] entries_10;
    }

    int namesEqual(const char* first, const char* second) const
    {
        return case_sensitive_24 != 0 ? strcmp(first, second) == 0
                                      : _stricmp(first, second) == 0;
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
        }
        else {
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

    srRuntimeClass* find(
        const char* name, const srRuntimeClass* relative_to) const
    {
        NameEntry* entry = 0;
        if (relative_to == 0) {
            entry = buckets_18[hashName(name) & (bucket_count_20 - 1)];
        }
        else {
            entry = by_instance_00.find(
                const_cast<srRuntimeClass*>(relative_to));
            entry = entry == 0 ? 0 : entry->next_00;
        }
        while (entry != 0) {
            if (namesEqual(name, entry->name_0c)) {
                return entry->instance_10;
            }
            entry = entry->next_00;
        }
        return 0;
    }

private:
    void resize(unsigned long bucket_count)
    {
        NameEntry* old_entries = entries_10;
        NameEntry** old_buckets = buckets_18;
        unsigned long old_bucket_count = bucket_count_20;

        entries_10 = new NameEntry[bucket_count];
        buckets_18 = new NameEntry*[bucket_count];
        bucket_count_20 = bucket_count;
        free_14 = entries_10;
        count_1c = 0;
        by_instance_00.clear();
        for (unsigned long index = 0; index < bucket_count; ++index) {
            buckets_18[index] = 0;
            entries_10[index].next_00 =
                index + 1 < bucket_count ? &entries_10[index + 1] : 0;
            entries_10[index].previous_04 = 0;
            entries_10[index].name_0c = 0;
            entries_10[index].instance_10 = 0;
        }

        if (old_buckets != 0) {
            for (unsigned long bucket = 0; bucket < old_bucket_count;
                 ++bucket) {
                for (NameEntry* entry = old_buckets[bucket]; entry != 0;
                     entry = entry->next_00) {
                    add(entry->instance_10);
                }
            }
        }
        delete[] old_buckets;
        delete[] old_entries;
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
        srRuntimeClass* instance_00;
        InstanceLink* next_04;
        InstanceLink* previous_08;
        unsigned long unused_0c;
    };

    IDIndex()
        : active_count_00(0),
          free_04(0),
          blocks_08(0),
          block_capacity_0c(0),
          block_count_10(0),
          first_14(0),
          last_18(0),
          list_count_1c(0)
    {
    }

    ~IDIndex()
    {
        clearBlocks();
    }

    void* operator new(unsigned int size) { return srHeap.allocate(size); }
    void operator delete(void* index) { srHeap.free(index); }

    InstanceLink* add(srRuntimeClass* instance)
    {
        if (free_04 == 0) {
            allocateBlock();
        }
        InstanceLink* link = free_04;
        free_04 = free_04->next_04;
        ++active_count_00;
        link->instance_00 = instance;
        link->previous_08 = 0;
        link->next_04 = first_14;
        if (first_14 != 0) {
            first_14->previous_08 = link;
        }
        else {
            last_18 = link;
        }
        first_14 = link;
        ++list_count_1c;
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
        }
        else {
            link->previous_08->next_04 = link->next_04;
        }
        if (link->next_04 == 0) {
            last_18 = link->previous_08;
        }
        else {
            link->next_04->previous_08 = link->previous_08;
        }
        link->instance_00 = 0;
        link->next_04 = free_04;
        free_04 = link;
        --active_count_00;
        --list_count_1c;
        if (active_count_00 == 0) {
            clearBlocks();
        }
    }

    srRuntimeClass* find(unsigned long id) const
    {
        InstanceLink* link = by_id_20.find(id);
        return link == 0 ? 0 : link->instance_00;
    }

    srRuntimeClass* findRelative(const srRuntimeClass* relative_to) const
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
        return link == 0 ? 0 : link->instance_00;
    }

private:
    void allocateBlock()
    {
        unsigned long count = active_count_00 < 2 ? 1 : active_count_00;
        if (count > 0x100) {
            count = 0x100;
        }
        InstanceLink* block = static_cast<InstanceLink*>(
            srHeap.allocate(count * sizeof(InstanceLink)));
        if (block_count_10 == block_capacity_0c) {
            unsigned long capacity = block_capacity_0c + 8;
            InstanceLink** blocks = new InstanceLink*[capacity];
            for (unsigned long index = 0; index < block_count_10; ++index) {
                blocks[index] = blocks_08[index];
            }
            delete[] blocks_08;
            blocks_08 = blocks;
            block_capacity_0c = capacity;
        }
        blocks_08[block_count_10++] = block;
        for (unsigned long index = 0; index < count; ++index) {
            block[index].next_04 =
                index + 1 < count ? &block[index + 1] : 0;
        }
        free_04 = block;
    }

    void clearBlocks()
    {
        for (unsigned long index = 0; index < block_count_10; ++index) {
            srHeap.free(blocks_08[index]);
        }
        delete[] blocks_08;
        active_count_00 = 0;
        free_04 = 0;
        blocks_08 = 0;
        block_capacity_0c = 0;
        block_count_10 = 0;
        first_14 = 0;
        last_18 = 0;
        list_count_1c = 0;
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

struct srRegistry::ClassIndex
    : RegistryHash<unsigned long, srRegistry::ClassNode*> {
};



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
    return srCore.getRegistry()->getNumberOfInstances(
        sGetClassNode(), exact);
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
    delete[] name_04;
    if (name == 0 || *name == '\0') {
        name_04 = 0;
    }
    else {
        name_04 = new char[strlen(name) + 1];
        strcpy(name_04, name);
    }
    srCore.getRegistry()->refreshInstance(getClassNode(), this);
}

// FUNCTION: SURRENDER 0x100119D0
srRuntimeClass::srRuntimeClass()
    : name_04(0), id_08(srCore.getRegistry()->allocateID())
{
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
}

// FUNCTION: SURRENDER 0x10011A10
srRuntimeClass::srRuntimeClass(const srRuntimeClass& other)
    : name_04(other.name_04), id_08(other.id_08)
{
}

// FUNCTION: SURRENDER 0x10011A40
srRuntimeClass::~srRuntimeClass()
{
    srCore.getRegistry()->unregisterInstance(sGetClassNode(), this);
    delete[] name_04;
}

// FUNCTION: SURRENDER 0x10011A80
srRuntimeClass& srRuntimeClass::operator=(const srRuntimeClass& other)
{
    name_04 = other.name_04;
    id_08 = other.id_08;
    return *this;
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
        node = registry->registerClass(
            "srRuntimeClass", registry->getRootNode(), 1, 0);
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

// FUNCTION: SURRENDER 0x1000E080
srClass* srClass::find(const char* name, const srClass* relative_to)
{
    return static_cast<srClass*>(srCore.getRegistry()->find(
        sGetClassNode(), name, relative_to));
}

// FUNCTION: SURRENDER 0x1000E0A0
srClass* srClass::find(
    const char* name,
    unsigned long class_id,
    const srRuntimeClass* relative_to)
{
    srRegistry* registry = srCore.getRegistry();
    return static_cast<srClass*>(registry->find(
        registry->getClassNode(class_id), name, relative_to));
}

// FUNCTION: SURRENDER 0x1000E0D0
srClass* srClass::find(unsigned long id)
{
    return static_cast<srClass*>(
        srCore.getRegistry()->find(sGetClassNode(), id));
}

// FUNCTION: SURRENDER 0x1000E0F0
srClass* srClass::find(const srClass* relative_to)
{
    return static_cast<srClass*>(
        srCore.getRegistry()->find(sGetClassNode(), relative_to));
}

// FUNCTION: SURRENDER 0x1000E110
srClass& srClass::operator=(const srClass& other)
{
    if (this != &other) {
        setName(other.getName());
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1000E130
srClass::srClass()
    : reference_count_0c(1), update_14(0)
{
    srCore.getRegistry()->registerInstance(sGetClassNode(), this);
    touch();
}

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
        node = registry->registerClass(
            sGetClassName(), srRuntimeClass::sGetClassNode(), 0x100, 0);
    }
    return node;
}

// FUNCTION: SURRENDER 0x1000E290
srClass::srClass(const srClass& other)
    : srRuntimeClass(other),
      reference_count_0c(other.reference_count_0c),
      timestamp_10(other.timestamp_10),
      update_14(other.update_14)
{
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
    for (Update* update = _firstUpdate; update != 0;
         update = update->next_1c) {
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
                update->callback_10(
                    update->instance_14, time, time - _lastUpdateTime);
                update->last_update_time_00 = time;
            }
            else {
                for (double update_time =
                         update->last_update_time_00 + update->interval_08;
                     update_time <= time;
                     update_time += update->interval_08) {
                    update->callback_10(
                        update->instance_14, update_time,
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
    unsigned long first = _timestampCtr + 1;
    _timestampCtr += count;
    return first;
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
    : root_00(0), class_index_04(0), valid_08(0),
      critical_section_0c(new srCriticalSection)
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
srRegistry::ClassNode* srRegistry::registerClass(
    const char* class_name,
    ClassNode* parent,
    unsigned long class_id,
    int register_instances)
{
    RegistryAccess access(critical_section_0c);
    ClassNode* node = class_index_04->find(class_id);
    if (node == 0) {
        srDebugPrintf(
            0xfe,
            "srRegistry::registerClass() - registering %s (ID 0x%x)\n",
            class_name, class_id);
        node = addToTree(parent, class_name, class_id);
        if (register_instances != 0) {
            node->enableInstanceLookup();
        }
    }
    return node;
}

// FUNCTION: SURRENDER 0x1000EDB0
srRegistry::ClassNode* srRegistry::addToTree(
    ClassNode* parent, const char* class_name, unsigned long class_id)
{
    RegistryAccess access(critical_section_0c);
    ClassNode* node = new ClassNode(parent, class_name, class_id);
    class_index_04->insert(class_id, node);
    return node;
}

// FUNCTION: SURRENDER 0x1000EFB0
void srRegistry::registerInstance(
    ClassNode* node, srRuntimeClass* instance)
{
    RegistryAccess access(critical_section_0c);
    node->registerInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F010
void srRegistry::refreshInstance(
    ClassNode* node, srRuntimeClass* instance)
{
    RegistryAccess access(critical_section_0c);
    node->refreshInstance(instance);
}

// FUNCTION: SURRENDER 0x1000F070
srRuntimeClass* srRegistry::find(
    ClassNode* node,
    const char* name,
    const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findByName(node, name, 0, relative_to);
}

// FUNCTION: SURRENDER 0x1000F0E0
srRuntimeClass* srRegistry::findExact(
    ClassNode* node,
    const char* name,
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
void srRegistry::unregisterInstance(
    ClassNode* node, srRuntimeClass* instance)
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
srRuntimeClass* srRegistry::findExact(
    ClassNode* node, const srRuntimeClass* relative_to)
{
    RegistryAccess access(critical_section_0c);
    return node->findRelative(node, 1, relative_to);
}

// FUNCTION: SURRENDER 0x1000F350
srRuntimeClass* srRegistry::find(
    ClassNode* node, const srRuntimeClass* relative_to)
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
srRegistry::ClassNode* srRegistry::getChildClass(
    ClassNode* parent, ClassNode* child)
{
    RegistryAccess access(critical_section_0c);
    ClassNode::ChildLink* link = parent->first_child_04;
    ClassNode* result = 0;
    if (child == 0) {
        if (link != parent->child_end_08) {
            result = link->node_00;
        }
    }
    else {
        while (link != parent->child_end_08) {
            if (link->node_00 == child) {
                link = link->next_04;
                if (link != parent->child_end_08) {
                    result = link->node_00;
                }
                break;
            }
            link = link->next_04;
        }
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
srRegistry::ClassNode::ClassNode(
    ClassNode* parent, const char* class_name, unsigned long class_id)
    : child_count_00(0),
      first_child_04(new ChildLink),
      child_end_08(first_child_04),
      parent_0c(parent),
      class_id_10(class_id),
      class_name_14(class_name),
      named_instances_18(0),
      inherited_named_instances_1c(0),
      instances_by_id_20(0),
      inherited_instances_by_id_24(0),
      instance_count_28(0)
{
    first_child_04->next_04 = 0;
    first_child_04->previous_08 = 0;
    if (parent_0c != 0) {
        ChildLink* link = new ChildLink;
        link->node_00 = this;
        link->next_04 = parent_0c->first_child_04;
        link->previous_08 = parent_0c->first_child_04->previous_08;
        if (link->previous_08 == 0) {
            parent_0c->first_child_04 = link;
        }
        else {
            link->previous_08->next_04 = link;
        }
        if (link->next_04 != 0) {
            link->next_04->previous_08 = link;
        }
        ++parent_0c->child_count_00;
        inherited_named_instances_1c = parent_0c->getNameIndex();
        inherited_instances_by_id_24 = parent_0c->getIDIndex();
    }
}

// FUNCTION: SURRENDER 0x1000F670
srRegistry::ClassNode::~ClassNode()
{
    while (first_child_04 != child_end_08) {
        delete first_child_04->node_00;
        ChildLink* link = first_child_04;
        first_child_04 = link->next_04;
        delete link;
        --child_count_00;
    }
    delete named_instances_18;
    delete instances_by_id_20;
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
srRegistry::ClassNode::NameIndex*
srRegistry::ClassNode::getNameIndex() const
{
    return named_instances_18 != 0 ? named_instances_18
                                   : inherited_named_instances_1c;
}

// FUNCTION: SURRENDER 0x10010080
srRegistry::ClassNode::IDIndex* srRegistry::ClassNode::getIDIndex() const
{
    return instances_by_id_20 != 0 ? instances_by_id_20
                                   : inherited_instances_by_id_24;
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
srRuntimeClass* srRegistry::ClassNode::findByName(
    ClassNode* requested_class,
    const char* name,
    int exact,
    const srRuntimeClass* relative_to)
{
    if (name == 0) {
        return 0;
    }

    NameIndex* index = getNameIndex();
    if (index == 0) {
        for (ChildLink* child = first_child_04; child != child_end_08;
             child = child->next_04) {
            srRuntimeClass* found = child->node_00->findByName(
                requested_class, name, exact, relative_to);
            if (found != 0) {
                return found;
            }
        }
        return 0;
    }

    srRuntimeClass* found = index->find(name, relative_to);
    while (found != 0) {
        ClassNode* found_class = found->getClassNode();
        if (exact != 0 ? requested_class == found_class
                       : requested_class->isDerivedOrSame(found_class)) {
            return found;
        }
        found = index->find(name, found);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100103B0
srRuntimeClass* srRegistry::ClassNode::findRelative(
    ClassNode* requested_class,
    int exact,
    const srRuntimeClass* relative_to)
{
    IDIndex* index = getIDIndex();
    if (index == 0) {
        for (ChildLink* child = first_child_04; child != child_end_08;
             child = child->next_04) {
            srRuntimeClass* found = child->node_00->findRelative(
                requested_class, exact, relative_to);
            if (found != 0) {
                return found;
            }
        }
        return 0;
    }

    srRuntimeClass* found = index->findRelative(relative_to);
    while (found != 0) {
        ClassNode* found_class = found->getClassNode();
        if (exact != 0 ? requested_class == found_class
                       : requested_class->isDerivedOrSame(found_class)) {
            return found;
        }
        found = index->findRelative(found);
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100104D0
srRuntimeClass* srRegistry::ClassNode::findByID(
    ClassNode* requested_class, unsigned long id, int exact)
{
    IDIndex* index = getIDIndex();
    if (index == 0) {
        if (exact == 0) {
            for (ChildLink* child = first_child_04; child != child_end_08;
                 child = child->next_04) {
                srRuntimeClass* found = child->node_00->findByID(
                    requested_class, id, 0);
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
    if (exact != 0 ? requested_class == found_class
                   : requested_class->isDerivedOrSame(found_class)) {
        return found;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1000F8E0
int srRegistry::ClassNode::isDerivedOrSame(ClassNode* derived) const
{
    while (derived != 0) {
        if (this == derived) {
            return 1;
        }
        derived = derived->parent_0c;
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
    for (ChildLink* child = first_child_04; child != child_end_08;
         child = child->next_04) {
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
    if (this == 0) {
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
