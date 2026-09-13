#pragma once

#include "srHeap.h"

/* SurRender's ordinary two-word growable-array boundary. Repeated
   instantiations prove the {data, capacity} layout, indexed growth rule,
   element construction, assignment, and teardown. `srArray` is a provisional
   spelling because the closed SDK's identifier did not survive; the primary
   template and its operations are compiler- and retail-proved. */
template <class T> class srArray {
public:
    inline srArray() : data(0), capacity(0) {}

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

/* The separately proved srHeap-backed family has both preserving exact-size
   storage and a scratch-buffer operation that discards old contents when it
   grows. `srHeapArray` is a provisional spelling, not a per-element wrapper or
   specialization. Its constructor invokes ensure(0); retail retains that call
   even for an empty request. */
template <class T> class srHeapArray {
public:
    inline srHeapArray() : data(0), capacity(0)
    {
        ensure(0);
    }

    inline ~srHeapArray()
    {
        release();
    }

    inline void release()
    {
        if (data != 0) {
            srHeap.free(data);
        }
        data = 0;
        capacity = 0;
    }

    static inline T* allocate(unsigned long count)
    {
        return static_cast<T*>(srHeap.allocate(count * sizeof(T)));
    }

    inline srHeapArray& operator=(const srHeapArray& other)
    {
        if (this != &other) {
            release();
            if (other.capacity != 0) {
                setCapacity(other.capacity);
                for (unsigned long index = 0; index < capacity; ++index) {
                    data[index] = other.data[index];
                }
            }
        }
        return *this;
    }

    /* `preserve` copies the overlapping prefix of the old contents into the
       new storage; callers that refill the whole array pass 0. */
    inline void setCapacity(unsigned long new_capacity, int preserve = 1)
    {
        if (capacity != new_capacity) {
            if (new_capacity > 0) {
                T* replacement = allocate(new_capacity);
                if (data != 0 && capacity > 0 && preserve) {
                    unsigned long copy_count = capacity;
                    if (copy_count >= new_capacity) {
                        copy_count = new_capacity;
                    }
                    for (unsigned long index = 0; index < copy_count; ++index) {
                        replacement[index] = data[index];
                    }
                }
                release();
                data = replacement;
                capacity = new_capacity;
            } else {
                release();
            }
        }
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

static_assert(sizeof(srArray<unsigned long>) == 0x08, "srArray_must_be_0x08");
static_assert(sizeof(srHeapArray<unsigned long>) == 0x08, "srHeapArray_must_be_0x08");
