#pragma once

#include "wiz8/sr_api.h"

/* The fixed-capacity binary minimum heap used by OctPath.cpp. Callers own the
   element ordering; this template owns only storage and heap maintenance. */
template <class T> class stHeap {
public:
    ~stHeap()
    {
        if (external_storage == 0) {
            delete[] entries_00;
        }
    }

    T* entries_00;
    unsigned int external_storage;
    int capacity;
    int size_0c;

    void Insert(const T* entry);
    void SiftDown(int index);
    void SiftUp(int index);
    T Delete();
};

template <class T> void stHeap<T>::Insert(const T* entry)
{
    if (size_0c >= capacity) {
        srAssertFail("heapsize < maxheapsize", "..\\Engine Code\\Include\\stHeap.hpp", 0xe1,
                     "stHeap overflow");
    }
    entries_00[size_0c] = *entry;
    SiftUp(size_0c);
    ++size_0c;
}

template <class T> void stHeap<T>::SiftDown(int index)
{
    T entry = entries_00[index];
    int child = index * 2 + 1;
    while (child < size_0c) {
        if (child + 1 < size_0c && entries_00[child + 1] <= entries_00[child]) {
            ++child;
        }
        if (entry <= entries_00[child]) {
            break;
        }
        entries_00[index] = entries_00[child];
        index = child;
        child = child * 2 + 1;
    }
    entries_00[index] = entry;
}

template <class T> void stHeap<T>::SiftUp(int index)
{
    T entry = entries_00[index];
    while (index != 0) {
        int parent = (index - 1) >> 1;
        if (entries_00[parent] <= entry) {
            break;
        }
        entries_00[index] = entries_00[parent];
        index = parent;
    }
    entries_00[index] = entry;
}

template <class T> T stHeap<T>::Delete()
{
    if (size_0c < 1) {
        srAssertFail("heapsize > 0", "..\\Engine Code\\Include\\stHeap.hpp", 0xf2,
                     "Delete called on empty stHeap");
    }

    T result = entries_00[0];
    entries_00[0] = entries_00[--size_0c];
    SiftDown(0);
    return result;
}
