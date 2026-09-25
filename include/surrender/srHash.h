#pragma once

/* The open-chained hash table the SurRender SDK carries for its own code.
   Its layout and operations agree with Wiz8's W8HashTable, but no accepted
   source oracle establishes one original template spelling across the two
   binaries. The Huffman sampler/compressor instantiate it with unsigned long
   keys, so the primary hash overload here takes unsigned long directly.
   Entries double as the free list: next_index links a bucket chain while live
   and the next unused slot while free. */
inline unsigned int srHashValue(unsigned long key)
{
    unsigned long mixed = (key >> 10) ^ key;
    return (mixed >> 10) ^ key;
}

inline unsigned int srHashValue(unsigned short key)
{
    return srHashValue(static_cast<unsigned long>(key));
}

/* srRegistry's by_instance_00 index hashes srRuntimeClass* keys with the same
   mixing as the integer-keyed tables (retail srTypeRegistry unregisterInstance
   at 0x1000FCD0 applies the (k>>10 ^ k)>>10 ^ k sequence to the pointer). */
inline unsigned int srHashValue(const void* key)
{
    // reinterpret-ok: pointer-keyed hashing mixes the pointer's integer value
    return srHashValue(reinterpret_cast<unsigned long>(key));
}

template <class Key, class Value> struct srHashEntry {
    int next_index;
    Key key;
    Value value;
};

template <class Key, class Value> class srHashTable {
public:
    // TEMPLATE: SURRENDER 0x10027840
    // srHashTable<srGERD::Renderer::TextureSetKey, unsigned long>::srHashTable
    srHashTable() : bucket_heads(0), entries(0), free_head(-1), bucket_count(0)
    {
        Grow();
    }
    // TEMPLATE: SURRENDER 0x10027860
    // srHashTable<srGERD::Renderer::TextureSetKey, unsigned long>::~srHashTable
    ~srHashTable()
    {
        if (bucket_heads != 0) {
            delete[] bucket_heads;
        }
        if (entries != 0) {
            delete[] entries;
        }
    }

    Value Lookup(const Key* key) const;
    int FindNextEntry(const Key* key, int previous) const;
    void Insert(const Key* key, const Value* value);
    void Remove(const Key* key, const Value* value);
    void Remove(const Key* key);
    void RemoveAt(int slot);
    void Grow();
    int AllocateEntry();

    // TEMPLATE: SURRENDER 0x10027890
    // srHashTable<srGERD::Renderer::TextureSetKey, unsigned long>::Clear
    void Clear()
    {
        if (bucket_count != 0) {
            delete[] bucket_heads;
            delete[] entries;
        }
        bucket_heads = 0;
        entries = 0;
        free_head = -1;
        bucket_count = 0;
        Grow();
    }

    int* bucket_heads;
    srHashEntry<Key, Value>* entries;
    int free_head;
    unsigned int bucket_count;
};

template <class Key, class Value> Value srHashTable<Key, Value>::Lookup(const Key* key) const
{
    Key wanted = *key;
    int slot = bucket_heads[srHashValue(wanted) & (bucket_count - 1)];
    while (slot != -1) {
        if (entries[slot].key == wanted) {
            return entries[slot].value;
        }
        slot = entries[slot].next_index;
    }
    return 0;
}

template <class Key, class Value>
int srHashTable<Key, Value>::FindNextEntry(const Key* key, int previous) const
{
    int slot;
    if (previous == -1) {
        slot = bucket_heads[srHashValue(*key) & (bucket_count - 1)];
    } else {
        slot = entries[previous].next_index;
    }
    while (slot != -1 && entries[slot].key != *key) {
        slot = entries[slot].next_index;
    }
    return slot;
}

template <class Key, class Value>
void srHashTable<Key, Value>::Insert(const Key* key, const Value* value)
{
    int slot = AllocateEntry();
    Key stored = *key;
    unsigned int bucket = srHashValue(stored) & (bucket_count - 1);

    entries[slot].key = stored;
    entries[slot].value = *value;
    entries[slot].next_index = bucket_heads[bucket];
    bucket_heads[bucket] = slot;
}

template <class Key, class Value> void srHashTable<Key, Value>::Remove(const Key* key)
{
    Key wanted = *key;
    int* bucket = bucket_heads + (srHashValue(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        srHashEntry<Key, Value>* entry = entries + slot;
        if (entry->key == wanted) {
            if (previous == -1) {
                *bucket = entry->next_index;
            } else {
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
void srHashTable<Key, Value>::Remove(const Key* key, const Value* value)
{
    Key wanted = *key;
    int* bucket = bucket_heads + (srHashValue(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        srHashEntry<Key, Value>* entry = entries + slot;
        if (entry->key == wanted && entry->value == *value) {
            if (previous == -1) {
                *bucket = entry->next_index;
            } else {
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

template <class Key, class Value> void srHashTable<Key, Value>::RemoveAt(int slot)
{
    Key wanted = entries[slot].key;
    int* bucket = bucket_heads + (srHashValue(wanted) & (bucket_count - 1));
    int current = *bucket;
    int previous = -1;

    while (current != -1) {
        if (current == slot) {
            if (previous == -1) {
                *bucket = entries[current].next_index;
            } else {
                entries[previous].next_index = entries[current].next_index;
            }
            entries[current].next_index = free_head;
            free_head = current;
            return;
        }
        previous = current;
        current = entries[current].next_index;
    }
}

// TEMPLATE: SURRENDER 0x100279E0
// srHashTable<srGERD::Renderer::TextureSetKey, unsigned long>::Grow
template <class Key, class Value> void srHashTable<Key, Value>::Grow()
{
    unsigned int capacity = bucket_count << 1;
    if (capacity < 4) {
        capacity = 4;
    }

    srHashEntry<Key, Value>* new_entries = new srHashEntry<Key, Value>[capacity];
    int* new_buckets = new int[capacity];

    srHashEntry<Key, Value>* fill_entry = new_entries;
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
                srHashEntry<Key, Value>* source = entries + slot;
                new_entries[used].key = source->key;
                unsigned int new_bucket = srHashValue(source->key) & (capacity - 1);
                new_entries[used].value = source->value;
                new_entries[used].next_index = new_buckets[new_bucket];
                new_buckets[new_bucket] = used;
                slot = source->next_index;
                ++used;
            }
        }
        delete[] bucket_heads;
        delete[] entries;
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

template <class Key, class Value> int srHashTable<Key, Value>::AllocateEntry()
{
    if (free_head == -1) {
        Grow();
    }
    int slot = free_head;
    free_head = entries[slot].next_index;
    return slot;
}
