#pragma once

/* <new> supplies VC6's throw() operator delete declaration. */
#include <new>

#include "srHeap.h"

template <class T> class srArray {
public:
    inline srArray() : data(0), capacity(0) {}

    inline explicit srArray(unsigned long reserve_capacity) : data(0), capacity(0)
    {
        if (reserve_capacity != 0) {
            reserve(reserve_capacity);
        }
    }

    inline srArray(const srArray& other) : data(0), capacity(0)
    {
        *this = other;
    }

    /* Discards contents and allocates exactly count elements. */
    void reserve(unsigned long count);

    inline ~srArray()
    {
        release();
    }

    inline void release()
    {
        delete[] data;
        data = 0;
        capacity = 0;
    }

    inline srArray& operator=(const srArray& other)
    {
        if (this != &other) {
            reserve(other.capacity);
            for (unsigned long index = 0; index < other.capacity; ++index) {
                data[index] = other.data[index];
            }
        }
        return *this;
    }

    void setCapacity(unsigned long new_capacity);

    /* Grows to include index, preserving contents. */
    void ensureIndex(unsigned long index);

    T& operator[](unsigned long index);

    T* data;
    unsigned long capacity;
};

template <class T> void srArray<T>::reserve(unsigned long count)
{
    release();
    if (count > 0) {
        capacity = count;
        data = new T[count];
    }
}

template <class T> inline void srArray<T>::setCapacity(unsigned long new_capacity)
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
        release();
        data = replacement;
        capacity = new_capacity;
    }
}

template <class T> void srArray<T>::ensureIndex(unsigned long index)
{
    if (index >= capacity) {
        setCapacity(capacity + 8 + index);
    }
}

template <class T> T& srArray<T>::operator[](unsigned long index)
{
    ensureIndex(index);
    return data[index];
}

/* Raw srHeap storage; does not construct or destroy elements. */
template <class T> class srHeapBuffer {
public:
    inline srHeapBuffer() : data(0), capacity(0)
    {
        ensure(0);
    }

    inline ~srHeapBuffer()
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

    /* Discards contents when growing. */
    inline T* ensure(unsigned long needed)
    {
        T* result = data;
        if (needed > capacity) {
            release();
            if (needed != 0) {
                needed = static_cast<unsigned long>((needed + 4) * 1.1);
            }
            capacity = needed;
            if (needed > 0) {
                data = allocate(needed);
            }
            result = data;
        }
        return result;
    }

    inline void setCapacity(unsigned long new_capacity, int preserve)
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

    inline srHeapBuffer& operator=(const srHeapBuffer& other)
    {
        if (this != &other) {
            release();
            if (other.capacity != 0) {
                setCapacity(other.capacity, 0);
                for (unsigned long index = 0; index < other.capacity; ++index) {
                    data[index] = other.data[index];
                }
            }
        }
        return *this;
    }

    T* data;
    unsigned long capacity;
};

static_assert(sizeof(srArray<unsigned long>) == 0x08, "srArray_must_be_0x08");
static_assert(sizeof(srHeapBuffer<unsigned long>) == 0x08, "srHeapBuffer_must_be_0x08");
