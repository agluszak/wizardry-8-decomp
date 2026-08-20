#pragma once

/* The open-chained hash table used by the octree builders. Entries double as
   the free list: next_index links a bucket chain while live and the next
   unused slot while free. */
inline unsigned int W8HashValue(unsigned short key)
{
    return (key >> 10) ^ key;
}

inline unsigned int W8HashValue(unsigned int key)
{
    unsigned int mixed = (key >> 10) ^ key;
    return (mixed >> 10) ^ mixed;
}

template <class Key, class Value>
struct W8HashEntry {
    int next_index;
    Key key;
    Value value;
};

template <class Key, class Value>
class W8HashTable {
public:
    W8HashTable()
        : bucket_heads(0), entries(0), free_head(-1), bucket_count(0)
    {
        Grow();
    }
    ~W8HashTable()
    {
        if (bucket_heads != 0) {
            ::operator delete(bucket_heads);
        }
        if (entries != 0) {
            ::operator delete(entries);
        }
    }

    Value Lookup(const Key* key) const;
    int FindNextEntry(const Key* key, int previous) const;
    void Insert(const Key* key, const Value* value);
    void Remove(const Key* key, const Value* value);
    void Grow();
    int AllocateEntry();

    int* bucket_heads;
    W8HashEntry<Key, Value>* entries;
    int free_head;
    unsigned int bucket_count;
};

template <class Key, class Value>
Value W8HashTable<Key, Value>::Lookup(const Key* key) const
{
    Key wanted = *key;
    int slot = bucket_heads[W8HashValue(wanted) & (bucket_count - 1)];
    while (slot != -1) {
        if (entries[slot].key == wanted) {
            return entries[slot].value;
        }
        slot = entries[slot].next_index;
    }
    return 0;
}

template <class Key, class Value>
int W8HashTable<Key, Value>::FindNextEntry(
    const Key* key, int previous) const
{
    int slot;
    if (previous == -1) {
        slot = bucket_heads[W8HashValue(*key) & (bucket_count - 1)];
    }
    else {
        slot = entries[previous].next_index;
    }
    while (slot != -1 && entries[slot].key != *key) {
        slot = entries[slot].next_index;
    }
    return slot;
}

template <class Key, class Value>
void W8HashTable<Key, Value>::Insert(const Key* key, const Value* value)
{
    int slot = AllocateEntry();
    Key stored = *key;
    unsigned int bucket = W8HashValue(stored) & (bucket_count - 1);

    entries[slot].key = stored;
    entries[slot].value = *value;
    entries[slot].next_index = bucket_heads[bucket];
    bucket_heads[bucket] = slot;
}

template <class Key, class Value>
void W8HashTable<Key, Value>::Remove(const Key* key, const Value* value)
{
    Key wanted = *key;
    int* bucket =
        bucket_heads + (W8HashValue(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        W8HashEntry<Key, Value>* entry = entries + slot;
        if (entry->key == wanted && entry->value == *value) {
            if (previous == -1) {
                *bucket = entry->next_index;
            }
            else {
                entries[previous].next_index = entry->next_index;
            }
            entry->next_index = free_head;
            free_head = slot;
            return;
        }
        previous = slot;
        slot = entry->next_index;
    }
}

template <class Key, class Value>
void W8HashTable<Key, Value>::Grow()
{
    unsigned int capacity = bucket_count << 1;
    if (capacity < 4) {
        capacity = 4;
    }

    W8HashEntry<Key, Value>* new_entries =
        static_cast<W8HashEntry<Key, Value>*>(
            ::operator new(capacity * sizeof(W8HashEntry<Key, Value>)));
    int* new_buckets =
        static_cast<int*>(::operator new(capacity * sizeof(int)));

    W8HashEntry<Key, Value>* fill_entry = new_entries;
    int* fill_bucket = new_buckets;
    unsigned int remaining = capacity;
    if ((int)capacity > 0) {
        do {
            fill_entry->next_index = -1;
            *fill_bucket = -1;
            --remaining;
            ++fill_entry;
            ++fill_bucket;
        } while (remaining != 0);
    }

    int used = 0;
    if (bucket_count != 0) {
        for (int bucket = 0; bucket < (int)bucket_count; ++bucket) {
            int slot = bucket_heads[bucket];
            while (slot != -1) {
                W8HashEntry<Key, Value>* source = entries + slot;
                new_entries[used].key = source->key;
                unsigned int new_bucket =
                    W8HashValue(source->key) & (capacity - 1);
                new_entries[used].value = source->value;
                new_entries[used].next_index = new_buckets[new_bucket];
                new_buckets[new_bucket] = used;
                slot = source->next_index;
                ++used;
            }
        }
        ::operator delete(bucket_heads);
        ::operator delete(entries);
    }

    if (used < (int)capacity) {
        for (int slot = used; slot < (int)capacity;) {
            ++slot;
            new_entries[slot - 1].next_index = slot;
        }
    }
    new_entries[capacity - 1].next_index = -1;

    bucket_count = capacity;
    entries = new_entries;
    free_head = used;
    bucket_heads = new_buckets;
}

template <class Key, class Value>
int W8HashTable<Key, Value>::AllocateEntry()
{
    if (free_head == -1) {
        Grow();
    }
    int slot = free_head;
    free_head = entries[slot].next_index;
    return slot;
}
