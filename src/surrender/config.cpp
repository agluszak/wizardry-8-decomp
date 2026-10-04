#include "surrender/srConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surrender/srDebug.h"
#include "surrender/srHash.h"
#include "surrender/srString.h"

namespace {
/* The provider's name hash: identical to the registry index in
   type_registry.cpp. Retail inlines it in this unit. */
unsigned long hashName(const char* name)
{
    unsigned long hash = 0;
    for (unsigned long index = 0; name[index] != '\0'; ++index) {
        hash += (index + 0x4ad) * static_cast<signed char>(name[index]);
    }
    return hash;
}

} // namespace

/* srConfig's private index: a name-hash node table plus an Entry*-keyed side
   map, the same dense-record machinery as the provider's registry indices. */
struct srConfig::Index {
    struct NameEntry {
        NameEntry* next;
        NameEntry* previous;
        unsigned long bucket;
        char* name;
        Entry* entry;
    };

    /* Entry*-keyed side map (0x10 bytes): dense records with a free list,
       bucket heads of record indices. */
    class EntryMap {
    public:
        struct Record {
            int next;
            Entry* key;
            NameEntry* value_08;
        };

        EntryMap() : heads(0), records(0), free(-1), count(0)
        {
            resize();
        }

        ~EntryMap()
        {
            if (heads != 0) {
                delete[] heads;
            }
            if (records != 0) {
                delete[] records;
            }
        }

        void clear()
        {
            if (count != 0) {
                delete[] heads;
                delete[] records;
            }
            count = 0;
            heads = 0;
            records = 0;
            free = -1;
            resize();
        }

        /* Retail keeps callable copies of the record allocator and the
           keyed operations (Index::resize calls 0x100136D0, the name-table
           add calls 0x10013340, removeEntry calls 0x100134F0) while resize
           splits between call and inlined copies. */
        int allocRecord();
        void insert(Entry*& key, NameEntry*& value);
        void erase(Entry*& key);

        // FUNCTION: SURRENDER 0x10013580
        void resize()
        {
            long count = this->count * 2;
            if (static_cast<unsigned long>(count) < 4) {
                count = 4;
            }
            Record* records = new Record[count];
            int* heads = new int[count];
            for (long index = 0; index < count; ++index) {
                records[index].next = -1;
                heads[index] = -1;
            }

            int used = 0;
            if (this->count != 0) {
                for (long bucket = 0; bucket < this->count; ++bucket) {
                    for (int old = this->heads[bucket]; old != -1; old = this->records[old].next) {
                        records[used].key = this->records[old].key;
                        int next_bucket = srHashValue(records[used].key) & (count - 1);
                        records[used].value_08 = this->records[old].value_08;
                        records[used].next = heads[next_bucket];
                        heads[next_bucket] = used++;
                    }
                }
                delete[] this->heads;
                delete[] this->records;
            }

            for (long free_index = used; free_index < count; ++free_index) {
                records[free_index].next = free_index + 1;
            }
            records[count - 1].next = -1;
            free = used;
            this->heads = heads;
            this->count = count;
            this->records = records;
        }

        int* heads;
        Record* records;
        int free;
        int count;
    };

    Index()
    {
        case_sensitive = 1;
        this->entries = 0;
        this->buckets = 0;
        count = 0;
        bucket_count = 4;
        free = 0;
        by_entry.clear();

        NameEntry* entries = new NameEntry[4];
        NameEntry** buckets = new NameEntry*[4];
        for (int index = 0; index < 4; ++index) {
            buckets[index] = 0;
            entries[index].bucket = 0;
            entries[index].previous = 0;
            entries[index].next = &entries[index + 1];
            entries[index].name = 0;
            entries[index].entry = 0;
        }
        entries[3].next = 0;
        free = entries;
        if (this->buckets != 0) {
            delete[] this->buckets;
        }
        if (this->entries != 0) {
            delete[] this->entries;
        }
        this->entries = entries;
        this->buckets = buckets;
    }

    ~Index();

    int namesEqual(const char* first, const char* second) const
    {
        return case_sensitive != 0 ? strcmp(first, second) == 0 : _stricmp(first, second) == 0;
    }

    NameEntry* find(const char* name) const
    {
        unsigned long bucket = hashName(name) & (bucket_count - 1);
        for (NameEntry* node = buckets[bucket]; node != 0; node = node->next) {
            if (namesEqual(name, node->name)) {
                return node;
            }
        }
        return 0;
    }

    /* Shared free-slot allocation expanded by add and resize. */
    NameEntry* allocateEntry()
    {
        if (free == 0) {
            resize(bucket_count * 2);
        }
        NameEntry* entry = free;
        free = entry->next;
        return entry;
    }

    void add(Entry* entry)
    {
        NameEntry* node = allocateEntry();
        node->next = 0;
        unsigned long bucket = hashName(entry->name) & (bucket_count - 1);
        node->bucket = bucket;
        node->name = entry->name;
        node->entry = entry;
        node->previous = 0;
        node->next = buckets[bucket];
        if (node->next != 0) {
            node->next->previous = node;
        }
        buckets[bucket] = node;
        by_entry.insert(entry, node);
        ++count;
    }

    /* Name-based removal unlinks every matching bucket entry before
       applying the existing quarter-full shrink rule. */
    void remove(const char* name)
    {
        if (name != 0) {
            unsigned long bucket = hashName(name) & (bucket_count - 1);
            NameEntry* node = buckets[bucket];
            while (node != 0) {
                NameEntry* next = node->next;
                if (namesEqual(name, node->name) && node != 0) {
                    by_entry.erase(node->entry);
                    if (node->previous == 0) {
                        buckets[node->bucket] = node->next;
                    } else {
                        node->previous->next = node->next;
                    }
                    if (node->next != 0) {
                        node->next->previous = node->previous;
                    }
                    node->next = free;
                    node->previous = 0;
                    node->name = 0;
                    node->entry = 0;
                    free = node;
                    --count;
                }
                node = next;
            }
            if (bucket_count > 7 && count <= bucket_count / 4) {
                resize(bucket_count / 2);
            }
        }
    }

    void resize(long bucket_count);

    EntryMap by_entry;
    NameEntry* entries;
    NameEntry* free;
    NameEntry** buckets;
    long count;
    long bucket_count;
    int case_sensitive;
};

static_assert(sizeof(srConfig::Index) == 0x28, "srConfig_Index_must_be_0x28");

// FUNCTION: SURRENDER 0x10011DB0
srConfig::Index* srConfig::getIndex() const
{
    if (index != 0) {
        return index;
    }
    index = new Index;
    return index;
}

// FUNCTION: SURRENDER 0x10011F10
void srConfig::dump(std::ostream& stream)
{
    for (Entry* entry = first_entry; entry != 0; entry = entry->next) {
        srStreamPrintf(stream, "%s = %s\n", entry->name, entry->value);
    }
}

// FUNCTION: SURRENDER 0x10011F40
srConfig::srConfig() : first_entry(0), entry_pool(), index(0) {}

// FUNCTION: SURRENDER 0x10011F60
srConfig::~srConfig()
{
    removeAll();
}

// FUNCTION: SURRENDER 0x10012010
void srConfig::removeAll()
{
    while (first_entry != 0) {
        removeEntry(first_entry);
    }
    if (index != 0) {
        delete index;
    }
    index = 0;
    entry_pool.release();
}

// FUNCTION: SURRENDER 0x100120F0
void srConfig::setBool(const char* name, int value)
{
    if (value != 0) {
        setLong(name, 1);
    } else {
        setLong(name, 0);
    }
}

// FUNCTION: SURRENDER 0x10012120
void srConfig::setLong(const char* name, long value)
{
    char text[128];
    sprintf(text, "%d", static_cast<int>(value));
    set(name, text);
}

// FUNCTION: SURRENDER 0x10012160
void srConfig::setFloat(const char* name, float value)
{
    char text[128];
    sprintf(text, "%f", value);
    set(name, text);
}

// FUNCTION: SURRENDER 0x100121B0
void srConfig::set(const char* name, const char* value)
{
    if (name == 0 || value == 0) {
        return;
    }

    Index* index = getIndex();
    Index::NameEntry* node = index->find(name);
    if (node != 0 && node->entry != 0) {
        removeEntry(node->entry);
    }

    Entry* entry = entry_pool.allocate();
    entry->previous = 0;
    entry->next = first_entry;
    if (first_entry != 0) {
        first_entry->previous = entry;
    }
    first_entry = entry;

    entry->name = new char[strlen(name) + 1];
    entry->value = new char[strlen(value) + 1];
    strcpy(entry->name, name);
    strcpy(entry->value, value);

    index = getIndex();
    if (entry->name != 0) {
        index->add(entry);
    }
}

// FUNCTION: SURRENDER 0x100124B0
void srConfig::append(const char* name, const char* value)
{
    if (name != 0 && value != 0) {
        const char* existing = get(name);
        srInlineString previous;
        if (existing != 0) {
            previous = existing;
        }
        srInlineString suffix(value);
        set(name, (previous + suffix).data());
    }
}

// FUNCTION: SURRENDER 0x10012640
void srConfig::removeEntry(Entry* entry)
{
    if (entry == 0) {
        return;
    }
    if (entry->previous != 0) {
        entry->previous->next = entry->next;
    }
    if (entry->next != 0) {
        entry->next->previous = entry->previous;
    }
    if (entry == first_entry) {
        first_entry = entry->next;
    }
    char* name = entry->name;
    Index* index = getIndex();
    index->remove(name);
    delete[] entry->name;
    delete[] entry->value;
    entry_pool.free(entry);
}

// FUNCTION: SURRENDER 0x10012850
void srConfig::remove(const char* name)
{
    if (name == 0) {
        return;
    }
    Index::NameEntry* node = getIndex()->find(name);
    if (node != 0 && node->entry != 0) {
        removeEntry(node->entry);
    }
}

// FUNCTION: SURRENDER 0x10012930
const char* srConfig::get(const char* name) const
{
    if (name == 0) {
        return 0;
    }
    Index::NameEntry* node = getIndex()->find(name);
    if (node == 0 || node->entry == 0) {
        return 0;
    }
    return node->entry->value;
}

// FUNCTION: SURRENDER 0x10012A00
long srConfig::getLong(const char* name) const
{
    const char* text = get(name);
    if (text == 0) {
        return 0;
    }
    return atoi(text);
}

// FUNCTION: SURRENDER 0x10012A20
int srConfig::getBool(const char* name) const
{
    return getLong(name);
}

// FUNCTION: SURRENDER 0x10012A30
float srConfig::getFloat(const char* name) const
{
    const char* text = get(name);
    if (text == 0) {
        return 0.0f;
    }
    return static_cast<float>(atof(text));
}

// FUNCTION: SURRENDER 0x10012A60
int srConfig::exists(const char* name) const
{
    if (name == 0) {
        return 0;
    }
    Index::NameEntry* node = getIndex()->find(name);
    return node != 0 && node->entry != 0;
}

/* Provider empty-state initialization, shared by the header-defined methods. */
// FUNCTION: SURRENDER 0x10004150
void srInlineString::init()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

/* The provider's reset releases non-inline storage, unlike the bare
   reinitialization at 0x0047D290 in Wiz8. */
// FUNCTION: SURRENDER 0x10012C80
void srInlineString::reset()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    init();
}

// FUNCTION: SURRENDER 0x10012CB0
srInlineString operator+(const srInlineString& left, const srInlineString& right)
{
    srInlineString result(left);
    if (right.data_ != 0 && *right.data_ != '\0') {
        unsigned long combined_size = result.size_ + strlen(right.data_);
        char* combined = static_cast<char*>(srHeap.allocate(combined_size));
        strcpy(combined, result.data_);
        strcpy(combined + result.size_ - 1, right.data_);
        result.reset();
        result.size_ = combined_size;
        result.data_ = combined;
    }
    return result;
}

// FUNCTION: SURRENDER 0x10012F10
srConfig::Index::~Index()
{
    if (buckets != 0) {
        delete[] buckets;
    }
    if (entries != 0) {
        delete[] entries;
    }
    buckets = 0;
    entries = 0;
    free = 0;
    count = 0;
    bucket_count = 0;
    by_entry.clear();
}

// FUNCTION: SURRENDER 0x100136D0
int srConfig::Index::EntryMap::allocRecord()
{
    if (free == -1) {
        resize();
    }
    int record = free;
    free = records[record].next;
    return record;
}

// FUNCTION: SURRENDER 0x10013340
void srConfig::Index::EntryMap::insert(Entry*& key, NameEntry*& value)
{
    int record = allocRecord();
    records[record].key = key;
    records[record].value_08 = value;
    unsigned long bucket = srHashValue(key) & (count - 1);
    records[record].next = heads[bucket];
    heads[bucket] = record;
}

// FUNCTION: SURRENDER 0x100134F0
void srConfig::Index::EntryMap::erase(Entry*& key)
{
    unsigned long bucket = srHashValue(key) & (count - 1);
    int* link = &heads[bucket];
    int record = *link;
    if (record != -1) {
        int previous = -1;
        while (records[record].key != key) {
            previous = record;
            record = records[record].next;
            if (record == -1) {
                return;
            }
        }
        if (previous != -1) {
            records[previous].next = records[record].next;
        } else {
            *link = records[record].next;
        }
        records[record].next = free;
        free = record;
    }
}

// FUNCTION: SURRENDER 0x10012FD0
void srConfig::Index::resize(long bucket_count)
{
    long old_bucket_count = this->bucket_count;
    NameEntry* entries = 0;
    NameEntry** buckets = 0;
    this->bucket_count = bucket_count;
    free = 0;
    by_entry.clear();

    if (bucket_count != 0) {
        entries = new NameEntry[bucket_count];
        buckets = new NameEntry*[bucket_count];
        for (long entry_index = 0; entry_index < bucket_count; ++entry_index) {
            buckets[entry_index] = 0;
            entries[entry_index].bucket = 0;
            entries[entry_index].previous = 0;
            entries[entry_index].next = &entries[entry_index + 1];
            entries[entry_index].name = 0;
            entries[entry_index].entry = 0;
        }
        entries[bucket_count - 1].next = 0;
        free = entries;

        if (this->buckets != 0 && old_bucket_count != 0) {
            for (long index = 0; index < old_bucket_count; ++index) {
                for (NameEntry* old_node = buckets[index]; old_node != 0;
                     old_node = old_node->next) {
                    char* name = old_node->name;
                    NameEntry* node = allocateEntry();
                    node->next = 0;
                    unsigned long bucket = hashName(name) & (bucket_count - 1);
                    node->bucket = bucket;
                    node->name = name;
                    node->entry = old_node->entry;
                    node->previous = 0;
                    node->next = buckets[bucket];
                    if (node->next != 0) {
                        node->next->previous = node;
                    }
                    buckets[bucket] = node;

                    by_entry.insert(old_node->entry, node);
                }
            }
        }
    }

    if (this->buckets != 0) {
        delete[] this->buckets;
    }
    if (this->entries != 0) {
        delete[] this->entries;
    }
    this->buckets = 0;
    this->entries = 0;
    if (bucket_count != 0) {
        this->buckets = buckets;
        this->entries = entries;
    }
}

// GLOBAL: SURRENDER 0x100A45C8
class srConfig srConfig;
