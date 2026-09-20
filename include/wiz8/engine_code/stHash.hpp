#pragma once

/* The open-chained hash table used by the octree builders. Its layout and
   operations agree with SurRender's srHashTable; the original shared template
   spelling remains unknown. Entries double as the free list: next_index
   links a bucket chain while live and the next unused slot while free. */
inline unsigned int W8HashValue(unsigned int key)
{
    unsigned int mixed = (key >> 10) ^ key;
    return (mixed >> 10) ^ key;
}

inline unsigned int W8HashValue(unsigned short key)
{
    return W8HashValue(static_cast<unsigned int>(key));
}

/* Single-array sibling of the QuickSortByKey idiom: partition while the range
   is wide, then finish with an insertion pass. Call sites inline the outer
   frame, so the Octree.cpp emission only ever serves the recursion. */
inline void InsertionSort(unsigned long* values, int first, int last)
{
    for (int index = first + 1; index < last; ++index) {
        unsigned long value = values[index];
        int position = index;
        while (value < values[position - 1]) {
            values[position] = values[position - 1];
            --position;
            if (position == first) {
                break;
            }
        }
        values[position] = value;
    }
}

// FUNCTION: WIZ8 0x00438e60
inline void QuickSort(unsigned long* values, int first, int last)
{
    if (last - first <= 8) {
        InsertionSort(values, first, last + 1);
        return;
    }
    unsigned long pivot = values[last];
    int low = first - 1;
    int high = last;
    unsigned long value;
    do {
        do {
            ++low;
        } while (low < last && values[low] < pivot);
        do {
            --high;
        } while (0 < high && pivot < values[high]);
        value = values[low];
        values[low] = values[high];
        values[high] = value;
    } while (low < high);
    values[high] = values[low];
    values[low] = values[last];
    values[last] = value;
    if (first < low - 1) {
        QuickSort(values, first, low - 1);
    }
    if (low + 1 < last) {
        QuickSort(values, low + 1, last);
    }
}

/* The paired-array sibling of the sorts above: items and their unsigned long
   keys move together.  Retail emits these instantiations in several TUs; the
   insertion pass only appears inside QuickSortByKey's tail or a SortByKey
   inline. */
template <class T> void InsertionSortByKey(T* items, unsigned long* keys, int first, int last)
{
    for (int index = first + 1; index < last; ++index) {
        T item = items[index];
        unsigned long key = keys[index];
        int position = index;
        while (key < keys[position - 1]) {
            keys[position] = keys[position - 1];
            items[position] = items[position - 1];
            --position;
            if (position == first) {
                break;
            }
        }
        keys[position] = key;
        items[position] = item;
    }
}

template <class T> void QuickSortByKey(T* items, unsigned long* keys, int first, int last)
{
    while (last - first > 8) {
        unsigned long pivot = keys[last];
        int low = first - 1;
        int high = last;
        T item;
        unsigned long key;
        do {
            do {
                ++low;
            } while (low < last && keys[low] < pivot);
            do {
                --high;
            } while (high > 0 && pivot < keys[high]);
            item = items[low];
            items[low] = items[high];
            items[high] = item;
            key = keys[low];
            keys[low] = keys[high];
            keys[high] = key;
        } while (low < high);
        items[high] = items[low];
        items[low] = items[last];
        items[last] = item;
        keys[high] = keys[low];
        keys[low] = keys[last];
        keys[last] = key;
        if (first < low - 1) {
            QuickSortByKey(items, keys, first, low - 1);
        }
        first = low + 1;
        if (last <= first) {
            return;
        }
    }
    InsertionSortByKey(items, keys, first, last + 1);
}

template <class T> void SortByKey(T* items, unsigned long* keys, int count)
{
    if (count > 1) {
        int ordered_pairs = 0;
        for (int index = 0; index < count - 1; ++index) {
            if (keys[index] <= keys[index + 1]) {
                ++ordered_pairs;
            }
        }
        if (ordered_pairs + 1 == count) {
            return;
        }
        if (ordered_pairs < count / 3) {
            for (int index = 0; index < count / 2; ++index) {
                T item = items[index];
                items[index] = items[count - 1 - index];
                items[count - 1 - index] = item;
                unsigned long key = keys[index];
                keys[index] = keys[count - 1 - index];
                keys[count - 1 - index] = key;
            }
            if (ordered_pairs == 0) {
                return;
            }
            InsertionSortByKey(items, keys, 0, count);
        } else if (ordered_pairs < 50) {
            InsertionSortByKey(items, keys, 0, count);
        } else {
            QuickSortByKey(items, keys, 0, count - 1);
        }
    }
}

template <class Key, class Value> struct W8HashEntry {
    int next_index;
    Key key;
    Value value;
};

template <class Key, class Value> class W8HashTable {
public:
    W8HashTable() : bucket_heads(0), entries(0), free_head(-1), bucket_count(0)
    {
        Grow();
    }
    ~W8HashTable()
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
    W8HashEntry<Key, Value>* entries;
    int free_head;
    unsigned int bucket_count;
};

template <class Key, class Value> Value W8HashTable<Key, Value>::Lookup(const Key* key) const
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

template <class Key, class Value> void W8HashTable<Key, Value>::Remove(const Key* key)
{
    Key wanted = *key;
    int* bucket = bucket_heads + (W8HashValue(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        W8HashEntry<Key, Value>* entry = entries + slot;
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
int W8HashTable<Key, Value>::FindNextEntry(const Key* key, int previous) const
{
    int slot;
    if (previous == -1) {
        slot = bucket_heads[W8HashValue(*key) & (bucket_count - 1)];
    } else {
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
    int* bucket = bucket_heads + (W8HashValue(wanted) & (bucket_count - 1));
    int slot = *bucket;
    int previous = -1;

    while (slot != -1) {
        W8HashEntry<Key, Value>* entry = entries + slot;
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

/* Remove the entry at a known slot, as 0x0042E650/0x0042E880 do after
   FindNextEntry. */
// TEMPLATE: WIZ8 0x00438dd0
// W8HashTable<unsigned int,int>::RemoveAt
template <class Key, class Value> void W8HashTable<Key, Value>::RemoveAt(int slot)
{
    Key wanted = entries[slot].key;
    int* bucket = bucket_heads + (W8HashValue(wanted) & (bucket_count - 1));
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

template <class Key, class Value> void W8HashTable<Key, Value>::Grow()
{
    unsigned int capacity = bucket_count << 1;
    if (capacity < 4) {
        capacity = 4;
    }

    W8HashEntry<Key, Value>* new_entries = new W8HashEntry<Key, Value>[capacity];
    int* new_buckets = new int[capacity];

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
                unsigned int new_bucket = W8HashValue(source->key) & (capacity - 1);
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

template <class Key, class Value> int W8HashTable<Key, Value>::AllocateEntry()
{
    if (free_head == -1) {
        Grow();
    }
    int slot = free_head;
    free_head = entries[slot].next_index;
    return slot;
}
