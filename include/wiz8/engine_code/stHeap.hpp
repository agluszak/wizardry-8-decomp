#pragma once

#include "wiz8/sr_api.h"

/* The fixed-capacity binary minimum heap used by OctPath.cpp. Callers own the
   element ordering; this template owns only storage and heap maintenance. */
template <class T> class stHeap {
public:
    ~stHeap()
    {
        if (external_storage == 0) {
            delete[] entries;
        }
    }

    T* entries;
    unsigned int external_storage;
    int capacity;
    int size;

    void Insert(const T* entry);
    void SiftDown(int index);
    void SiftUp(int index);
    T Delete();
};

template <class T> void stHeap<T>::Insert(const T* entry)
{
    if (size >= capacity) {
        srAssertFail("heapsize < maxheapsize", "..\\Engine Code\\Include\\stHeap.hpp", 0xe1,
                     "stHeap overflow");
    }
    entries[size] = *entry;
    SiftUp(size);
    ++size;
}

template <class T> void stHeap<T>::SiftDown(int index)
{
    T entry = entries[index];
    int child = index * 2 + 1;
    while (child < size) {
        if (child + 1 < size && entries[child + 1] <= entries[child]) {
            ++child;
        }
        if (entry <= entries[child]) {
            break;
        }
        entries[index] = entries[child];
        index = child;
        child = child * 2 + 1;
    }
    entries[index] = entry;
}

template <class T> void stHeap<T>::SiftUp(int index)
{
    T entry = entries[index];
    while (index != 0) {
        int parent = (index - 1) >> 1;
        if (entries[parent] <= entry) {
            break;
        }
        entries[index] = entries[parent];
        index = parent;
    }
    entries[index] = entry;
}

template <class T> T stHeap<T>::Delete()
{
    if (size < 1) {
        srAssertFail("heapsize > 0", "..\\Engine Code\\Include\\stHeap.hpp", 0xf2,
                     "Delete called on empty stHeap");
    }

    T result = entries[0];
    entries[0] = entries[--size];
    SiftDown(0);
    return result;
}
