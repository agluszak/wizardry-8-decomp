#pragma once

#include "srHeap.h"

/* SurRender's ordinary two-word growable-array boundary. Repeated
   instantiations prove the {data, capacity} layout, indexed growth rule,
   element construction, assignment, and teardown. `srArray` is a provisional
   spelling because the closed SDK's identifier did not survive; the primary
   template and its operations are compiler- and retail-proved. */
template <class T>
class srArray {
public:
    inline srArray()
        : data(0), capacity(0)
    {
    }

    inline ~srArray()
    {
        delete[] data;
        data = 0;
        capacity = 0;
    }

    inline void setCapacity(unsigned long new_capacity)
    {
        if (capacity != new_capacity) {
            T* replacement = 0;
            if (new_capacity > 0) {
                replacement = new T[new_capacity];
                if (data != 0 && capacity > 0) {
                    unsigned long copy_count = capacity;
                    if (copy_count >= new_capacity) {
                        copy_count = new_capacity;
                    }
                    for (unsigned long index = 0; index < copy_count; ++index) {
                        replacement[index] = data[index];
                    }
                }
            }
            delete[] data;
            data = replacement;
            capacity = new_capacity;
        }
    }

    inline T& operator[](unsigned long index)
    {
        if (index >= capacity) {
            setCapacity(capacity + 8 + index);
        }
        return data[index];
    }

    T* data;
    unsigned long capacity;
};

/* The separately proved scratch-buffer family uses srHeap and intentionally
   discards old contents when it grows. `srHeapArray` is likewise a provisional
   spelling, not a per-element wrapper or specialization. Its constructor
   invokes ensure(0); retail retains that call even for an empty request. */
template <class T>
class srHeapArray {
public:
    inline srHeapArray()
        : data(0), capacity(0)
    {
        ensure(0);
    }

    inline ~srHeapArray()
    {
        if (data != 0) {
            srHeap.free(data);
        }
        data = 0;
        capacity = 0;
    }

    inline T* ensure(unsigned long needed)
    {
        T* result = data;
        if (needed > capacity) {
            if (result != 0) {
                srHeap.free(result);
            }
            data = 0;
            capacity = 0;

            if (needed != 0) {
                needed = static_cast<unsigned long>((needed + 4) * 1.1);
            }
            capacity = needed;
            if (needed > 0) {
                data = static_cast<T*>(srHeap.allocate(needed * sizeof(T)));
            }
            result = data;
        }
        return result;
    }

    T* data;
    unsigned long capacity;
};

static_assert(sizeof(srArray<unsigned long>) == 0x08,
              "srArray_must_be_0x08");
static_assert(sizeof(srHeapArray<unsigned long>) == 0x08,
              "srHeapArray_must_be_0x08");
