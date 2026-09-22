#include "surrender/srConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surrender/srDebug.h"
#include "surrender/srString.h"

namespace {
/* The provider's name hash: identical to the registry index in
   type_registry.cpp. Retail inlines it in this unit. */
inline unsigned long hashName(const char* name)
{
    unsigned long hash = 0;
    for (unsigned long index = 0; name[index] != '\0'; ++index) {
        hash += (index + 0x4ad) * static_cast<signed char>(name[index]);
    }
    return hash;
}

inline unsigned long hashInteger(unsigned long value)
{
    return ((value >> 10) ^ value) >> 10 ^ value;
}
} // namespace

/* srConfig's private index: a name-hash node table plus an Entry*-keyed side
   map, the same dense-record machinery as the provider's registry indices. */
struct srConfig::Index {
    struct NameEntry {
        NameEntry* next_00;
        NameEntry* previous_04;
        unsigned long bucket_08;
        char* name_0c;
        Entry* entry_10;
    };

    /* Entry*-keyed side map (0x10 bytes): dense records with a free list,
       bucket heads of record indices. */
    class EntryMap {
    public:
        struct Record {
            int next_00;
            Entry* key_04;
            NameEntry* value_08;
        };

        EntryMap() : heads_00(0), records_04(0), free_08(-1), count_0c(0)
        {
            resize();
        }

        ~EntryMap()
        {
            if (heads_00 != 0) {
                delete[] heads_00;
            }
            if (records_04 != 0) {
                delete[] records_04;
            }
        }

        void clear()
        {
            if (count_0c != 0) {
                delete[] heads_00;
                delete[] records_04;
            }
            count_0c = 0;
            heads_00 = 0;
            records_04 = 0;
            free_08 = -1;
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
            long count = count_0c * 2;
            if (static_cast<unsigned long>(count) < 4) {
                count = 4;
            }
            Record* records = new Record[count];
            int* heads = new int[count];
            for (long index = 0; index < count; ++index) {
                records[index].next_00 = -1;
                heads[index] = -1;
            }

            int used = 0;
            if (count_0c != 0) {
                for (long bucket = 0; bucket < count_0c; ++bucket) {
                    for (int old = heads_00[bucket]; old != -1; old = records_04[old].next_00) {
                        records[used].key_04 = records_04[old].key_04;
                        int next_bucket =
                            // reinterpret-ok: the side map hashes the key's
                            // address bits.
                            hashInteger(reinterpret_cast<unsigned long>(records[used].key_04)) &
                            (count - 1);
                        records[used].value_08 = records_04[old].value_08;
                        records[used].next_00 = heads[next_bucket];
                        heads[next_bucket] = used++;
                    }
                }
                delete[] heads_00;
                delete[] records_04;
            }

            for (long free_index = used; free_index < count; ++free_index) {
                records[free_index].next_00 = free_index + 1;
            }
            records[count - 1].next_00 = -1;
            free_08 = used;
            heads_00 = heads;
            count_0c = count;
            records_04 = records;
        }

        int* heads_00;
        Record* records_04;
        int free_08;
        int count_0c;
    };

    Index()
    {
        case_sensitive_24 = 1;
        entries_10 = 0;
        buckets_18 = 0;
        count_1c = 0;
        bucket_count_20 = 4;
        free_14 = 0;
        by_entry_00.clear();

        NameEntry* entries = new NameEntry[4];
        NameEntry** buckets = new NameEntry*[4];
        for (int index = 0; index < 4; ++index) {
            buckets[index] = 0;
            entries[index].bucket_08 = 0;
            entries[index].previous_04 = 0;
            entries[index].next_00 = &entries[index + 1];
            entries[index].name_0c = 0;
            entries[index].entry_10 = 0;
        }
        entries[3].next_00 = 0;
        free_14 = entries;
        if (buckets_18 != 0) {
            delete[] buckets_18;
        }
        if (entries_10 != 0) {
            delete[] entries_10;
        }
        entries_10 = entries;
        buckets_18 = buckets;
    }

    ~Index();

    int namesEqual(const char* first, const char* second) const
    {
        return case_sensitive_24 != 0 ? strcmp(first, second) == 0 : _stricmp(first, second) == 0;
    }

    NameEntry* find(const char* name) const
    {
        unsigned long bucket = hashName(name) & (bucket_count_20 - 1);
        for (NameEntry* node = buckets_18[bucket]; node != 0; node = node->next_00) {
            if (namesEqual(name, node->name_0c)) {
                return node;
            }
        }
        return 0;
    }

    void add(Entry* entry)
    {
        if (free_14 == 0) {
            resize(bucket_count_20 * 2);
        }
        NameEntry* node = free_14;
        free_14 = node->next_00;
        node->next_00 = 0;
        unsigned long bucket = hashName(entry->name) & (bucket_count_20 - 1);
        node->bucket_08 = bucket;
        node->name_0c = entry->name;
        node->entry_10 = entry;
        node->previous_04 = 0;
        node->next_00 = buckets_18[bucket];
        if (node->next_00 != 0) {
            node->next_00->previous_04 = node;
        }
        buckets_18[bucket] = node;
        by_entry_00.insert(entry, node);
        ++count_1c;
    }

    void resize(long bucket_count);

    EntryMap by_entry_00;
    NameEntry* entries_10;
    NameEntry* free_14;
    NameEntry** buckets_18;
    long count_1c;
    long bucket_count_20;
    int case_sensitive_24;
};

static_assert(sizeof(srConfig::Index) == 0x28, "srConfig_Index_must_be_0x28");

// FUNCTION: SURRENDER 0x10011DB0
srConfig::Index* srConfig::getIndex() const
{
    if (index_18 != 0) {
        return index_18;
    }
    index_18 = new Index;
    return index_18;
}

// FUNCTION: SURRENDER 0x10011F10
void srConfig::dump(std::ostream& stream)
{
    for (Entry* entry = first_entry_00; entry != 0; entry = entry->next) {
        srStreamPrintf(stream, "%s = %s\n", entry->name, entry->value);
    }
}

// FUNCTION: SURRENDER 0x10011F40
srConfig::srConfig() : first_entry_00(0), entry_pool_04(), index_18(0) {}

// FUNCTION: SURRENDER 0x10011F60
srConfig::~srConfig()
{
    removeAll();
}

// FUNCTION: SURRENDER 0x10012010
void srConfig::removeAll()
{
    while (first_entry_00 != 0) {
        removeEntry(first_entry_00);
    }
    if (index_18 != 0) {
        delete index_18;
        index_18 = 0;
    }
    entry_pool_04.release();
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
    if (node != 0 && node->entry_10 != 0) {
        removeEntry(node->entry_10);
    }

    if (entry_pool_04.free_entries == 0) {
        long count = entry_pool_04.entry_count < 2 ? 1 : entry_pool_04.entry_count;
        if (count > 0xff) {
            count = 0x100;
        }
        Entry* block = static_cast<Entry*>(srHeap.allocate(count * sizeof(Entry)));
        unsigned long block_index = entry_pool_04.entry_block_count;
        entry_pool_04.free_entries = block;
        entry_pool_04.entry_block_count = block_index + 1;
        entry_pool_04.entry_blocks[block_index] = block;
        Entry* free_entry = block;
        for (unsigned long index_ = count; index_ != 0; --index_) {
            // reinterpret-ok: free pool entries thread the next-free pointer
            // through the name field.
            free_entry->name = reinterpret_cast<char*>(free_entry + 1);
            ++free_entry;
        }
        block[count - 1].name = 0;
    }

    Entry* entry = entry_pool_04.free_entries;
    // reinterpret-ok: the free list link lives in the name pointer.
    entry_pool_04.free_entries = reinterpret_cast<Entry*>(entry->name);
    ++entry_pool_04.entry_count;
    entry->previous = 0;
    entry->next = first_entry_00;
    if (first_entry_00 != 0) {
        first_entry_00->previous = entry;
    }
    first_entry_00 = entry;

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
    if (entry == first_entry_00) {
        first_entry_00 = entry->next;
    }
    char* name = entry->name;
    Index* index = getIndex();
    if (name != 0) {
        unsigned long bucket = hashName(name) & (index->bucket_count_20 - 1);
        Index::NameEntry* node = index->buckets_18[bucket];
        while (node != 0) {
            Index::NameEntry* next = node->next_00;
            if (index->namesEqual(name, node->name_0c) && node != 0) {
                index->by_entry_00.erase(node->entry_10);
                if (node->previous_04 == 0) {
                    index->buckets_18[node->bucket_08] = node->next_00;
                } else {
                    node->previous_04->next_00 = node->next_00;
                }
                if (node->next_00 != 0) {
                    node->next_00->previous_04 = node->previous_04;
                }
                node->next_00 = index->free_14;
                node->previous_04 = 0;
                node->name_0c = 0;
                node->entry_10 = 0;
                index->free_14 = node;
                --index->count_1c;
            }
            node = next;
        }
        if (index->bucket_count_20 > 7 && index->count_1c <= index->bucket_count_20 / 4) {
            index->resize(index->bucket_count_20 / 2);
        }
    }
    delete[] entry->name;
    delete[] entry->value;
    --entry_pool_04.entry_count;
    // reinterpret-ok: the free list link lives in the name pointer.
    entry->name = reinterpret_cast<char*>(entry_pool_04.free_entries);
    entry_pool_04.free_entries = entry;
    if (entry_pool_04.entry_count == 0) {
        entry_pool_04.release();
    }
}

// FUNCTION: SURRENDER 0x10012850
void srConfig::remove(const char* name)
{
    if (name == 0) {
        return;
    }
    Index::NameEntry* node = getIndex()->find(name);
    if (node != 0 && node->entry_10 != 0) {
        removeEntry(node->entry_10);
    }
}

// FUNCTION: SURRENDER 0x10012930
const char* srConfig::get(const char* name) const
{
    if (name == 0) {
        return 0;
    }
    Index::NameEntry* node = getIndex()->find(name);
    if (node == 0 || node->entry_10 == 0) {
        return 0;
    }
    return node->entry_10->value;
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
    return node != 0 && node->entry_10 != 0;
}

/* The provider TU's own copies of the srInlineString methods append needs.
   Retail shows VC6's per-site inline lottery plainly: append's
   `previous = existing` expands operator= inline but keeps a call to this
   unit's reset emission (0x10012C80). The sibling unit's callable emissions -
   operator= at 0x100040D0 and init at 0x10004150 - belong to that unit's
   recovery, not this file. */

/* The provider's callable init emission - retail expansions call it from
   destructor and copy-constructor tails, so it is deliberately not inline. */
void srInlineString::init()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

/* The provider's reset releases non-inline storage, unlike the bare
   reinitialize Wiz8's unit emits. Retail emits this copy out of line and
   append's inlined operator= calls it, so it is deliberately not inline. */
// FUNCTION: SURRENDER 0x10012C80
void srInlineString::reset()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

inline srInlineString::srInlineString()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

inline srInlineString::srInlineString(const char* source)
{
    init();
    operator=(source);
}

inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source);
    }
}

inline srInlineString::~srInlineString()
{
    reset();
}

inline srInlineString& srInlineString::operator=(const char* source)
{
    reset();
    if (source == 0 || *source == '\0') {
        return *this;
    }
    size_ = strlen(source) + 1;
    data_ = static_cast<char*>(srHeap.allocate(size_));
    strcpy(data_, source);
    return *this;
}

/* The provider's copy-assign reinitializes instead of destroying first;
   retail's operator+ emission shows the init call inside the copy
   constructor expansion. */
inline srInlineString& srInlineString::operator=(const srInlineString& source)
{
    init();
    if (source.data_ != 0 && *source.data_ != '\0') {
        size_ = strlen(source.data_) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, source.data_);
    }
    return *this;
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
        if (result.data_ != result.inline_) {
            srHeap.free(result.data_);
        }
        result.inline_[0] = '\0';
        result.size_ = combined_size;
        result.data_ = combined;
    }
    return result;
}

// FUNCTION: SURRENDER 0x10012F10
srConfig::Index::~Index()
{
    if (buckets_18 != 0) {
        delete[] buckets_18;
    }
    if (entries_10 != 0) {
        delete[] entries_10;
    }
    buckets_18 = 0;
    entries_10 = 0;
    free_14 = 0;
    count_1c = 0;
    bucket_count_20 = 0;
    by_entry_00.clear();
}

// FUNCTION: SURRENDER 0x100136D0
int srConfig::Index::EntryMap::allocRecord()
{
    if (free_08 == -1) {
        resize();
    }
    int record = free_08;
    free_08 = records_04[record].next_00;
    return record;
}

// FUNCTION: SURRENDER 0x10013340
void srConfig::Index::EntryMap::insert(Entry*& key, NameEntry*& value)
{
    if (free_08 == -1) {
        resize();
    }
    int record = free_08;
    free_08 = records_04[record].next_00;
    records_04[record].key_04 = key;
    records_04[record].value_08 = value;
    unsigned long bucket =
        // reinterpret-ok: the side map hashes the key's address bits.
        hashInteger(reinterpret_cast<unsigned long>(key)) & (count_0c - 1);
    records_04[record].next_00 = heads_00[bucket];
    heads_00[bucket] = record;
}

// FUNCTION: SURRENDER 0x100134F0
void srConfig::Index::EntryMap::erase(Entry*& key)
{
    unsigned long bucket =
        // reinterpret-ok: the side map hashes the key's address bits.
        hashInteger(reinterpret_cast<unsigned long>(key)) & (count_0c - 1);
    int* link = &heads_00[bucket];
    int record = *link;
    if (record != -1) {
        int previous = -1;
        while (records_04[record].key_04 != key) {
            previous = record;
            record = records_04[record].next_00;
            if (record == -1) {
                return;
            }
        }
        if (previous != -1) {
            records_04[previous].next_00 = records_04[record].next_00;
        } else {
            *link = records_04[record].next_00;
        }
        records_04[record].next_00 = free_08;
        free_08 = record;
    }
}

// FUNCTION: SURRENDER 0x10012FD0
void srConfig::Index::resize(long bucket_count)
{
    long old_bucket_count = bucket_count_20;
    NameEntry* entries = 0;
    NameEntry** buckets = 0;
    bucket_count_20 = bucket_count;
    free_14 = 0;
    by_entry_00.clear();

    if (bucket_count != 0) {
        entries = new NameEntry[bucket_count];
        buckets = new NameEntry*[bucket_count];
        for (long entry_index = 0; entry_index < bucket_count; ++entry_index) {
            buckets[entry_index] = 0;
            entries[entry_index].bucket_08 = 0;
            entries[entry_index].previous_04 = 0;
            entries[entry_index].next_00 = &entries[entry_index + 1];
            entries[entry_index].name_0c = 0;
            entries[entry_index].entry_10 = 0;
        }
        entries[bucket_count - 1].next_00 = 0;
        free_14 = entries;

        if (buckets_18 != 0 && old_bucket_count != 0) {
            for (long index = 0; index < old_bucket_count; ++index) {
                for (NameEntry* old_node = buckets_18[index]; old_node != 0;
                     old_node = old_node->next_00) {
                    char* name = old_node->name_0c;
                    if (free_14 == 0) {
                        resize(bucket_count_20 * 2);
                    }
                    NameEntry* node = free_14;
                    free_14 = node->next_00;
                    node->next_00 = 0;
                    unsigned long bucket = hashName(name) & (bucket_count_20 - 1);
                    node->bucket_08 = bucket;
                    node->name_0c = name;
                    node->entry_10 = old_node->entry_10;
                    node->previous_04 = 0;
                    node->next_00 = buckets[bucket];
                    if (node->next_00 != 0) {
                        node->next_00->previous_04 = node;
                    }
                    buckets[bucket] = node;

                    int record = by_entry_00.allocRecord();
                    unsigned long sub_bucket =
                        // reinterpret-ok: the side map hashes the entry's
                        // address bits.
                        hashInteger(reinterpret_cast<unsigned long>(old_node->entry_10)) &
                        (by_entry_00.count_0c - 1);
                    by_entry_00.records_04[record].key_04 = old_node->entry_10;
                    by_entry_00.records_04[record].value_08 = node;
                    by_entry_00.records_04[record].next_00 = by_entry_00.heads_00[sub_bucket];
                    by_entry_00.heads_00[sub_bucket] = record;
                }
            }
        }
    }

    if (buckets_18 != 0) {
        delete[] buckets_18;
    }
    if (entries_10 != 0) {
        delete[] entries_10;
    }
    buckets_18 = 0;
    entries_10 = 0;
    if (bucket_count != 0) {
        buckets_18 = buckets;
        entries_10 = entries;
    }
}

// TEMPLATE: SURRENDER 0x10012C60
// srArray<srConfig::Entry*>::~srArray<srConfig::Entry*>

// TEMPLATE: SURRENDER 0x10012EA0
// srArray<srConfig::Entry*>::setCapacity

// GLOBAL: SURRENDER 0x100A45C8
class srConfig srConfig;
