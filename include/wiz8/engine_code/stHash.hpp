#pragma once

/* The open-chained hash table emitted by OctBuildPreTree.cpp for 16-bit
   region keys. Entries double as the free list: next_index links a bucket
   chain while live and the next unused slot while free. */
template <class Value>
struct W8UShortHashEntry {
    int next_index;
    unsigned short key;
    Value value;
};

template <class Value>
class W8UShortHashTable {
public:
    W8UShortHashTable();

    Value Lookup(const unsigned short* key) const;
    void Insert(const unsigned short* key, const Value* value);
    void Remove(const unsigned short* key, const Value* value);
    void Grow();
    int AllocateEntry();

    int* bucket_heads;
    W8UShortHashEntry<Value>* entries;
    int free_head;
    unsigned int bucket_count;

private:
    static unsigned int Hash(unsigned short key)
    {
        return (key >> 10) ^ key;
    }
};

template <class Value>
W8UShortHashTable<Value>::W8UShortHashTable()
    : bucket_heads(0), entries(0), free_head(-1), bucket_count(0)
{
    Grow();
}

template <class Value>
Value W8UShortHashTable<Value>::Lookup(const unsigned short* key) const
{
    unsigned short wanted = *key;
    int slot = bucket_heads[Hash(wanted) & (bucket_count - 1)];
    while (slot != -1) {
        if (entries[slot].key == wanted) {
            return entries[slot].value;
        }
        slot = entries[slot].next_index;
    }
    return 0;
}

template <class Value>
void W8UShortHashTable<Value>::Insert(
    const unsigned short* key, const Value* value)
{
    int slot = AllocateEntry();
    unsigned short stored = *key;
    unsigned int bucket = Hash(stored) & (bucket_count - 1);

    entries[slot].key = stored;
    entries[slot].value = *value;
    entries[slot].next_index = bucket_heads[bucket];
    bucket_heads[bucket] = slot;
}

template <class Value>
void W8UShortHashTable<Value>::Remove(
    const unsigned short* key, const Value* value)
{
    unsigned short wanted = *key;
    int* bucket = bucket_heads + (Hash(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        W8UShortHashEntry<Value>* entry = entries + slot;
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

template <class Value>
void W8UShortHashTable<Value>::Grow()
{
    unsigned int capacity = bucket_count << 1;
    if (capacity < 4) {
        capacity = 4;
    }

    W8UShortHashEntry<Value>* new_entries =
        static_cast<W8UShortHashEntry<Value>*>(
            ::operator new(capacity * sizeof(W8UShortHashEntry<Value>)));
    int* new_buckets =
        static_cast<int*>(::operator new(capacity * sizeof(int)));

    W8UShortHashEntry<Value>* fill_entry = new_entries;
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
                W8UShortHashEntry<Value>* source = entries + slot;
                new_entries[used].key = source->key;
                unsigned int new_bucket =
                    Hash(source->key) & (capacity - 1);
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

template <class Value>
int W8UShortHashTable<Value>::AllocateEntry()
{
    if (free_head == -1) {
        Grow();
    }
    int slot = free_head;
    free_head = entries[slot].next_index;
    return slot;
}

