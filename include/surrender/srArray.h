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

    /* Retail's canonical emissions (srArray<srNode*>::setCapacity at
       0x0049E290, the folded scalar-delete destructor at 0x004701B0) use the
       scalar global operators on raw bytes, never new[]/delete[]: every
       instantiated element type is POD, so construction and element teardown
       do not exist. */
    inline ~srArray()
    {
        release();
    }

    /* The shared teardown sr.dll emits out of line at 0x100027D0 for the
       srHuffman::Sampler pair array: scalar operator delete plus zeroing both
       words. setCapacity reaches it on the deep-inline paths. */
    inline void release()
    {
        ::operator delete(data);
        data = 0;
        capacity = 0;
    }

    /* Deep copy proved by srHuffman::Sampler's exported copy operations:
       self-check, release the old storage, allocate the source capacity, copy
       that many elements. */
    inline srArray& operator=(const srArray& other)
    {
        if (this != &other) {
            unsigned long new_capacity = other.capacity;
            release();
            if (new_capacity > 0) {
                capacity = new_capacity;
                data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
            }
            for (unsigned long index = 0; index < other.capacity; ++index) {
                data[index] = other.data[index];
            }
        }
        return *this;
    }

    inline void setCapacity(unsigned long new_capacity)
    {
        if (capacity != new_capacity) {
            T* replacement = 0;
            if (new_capacity > 0) {
                replacement = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
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

    /* The canonical emission at 0x004701D0 frees unconditionally; srHeap.free
       accepts a null pointer. */
    inline void release()
    {
        srHeap.free(data);
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

    /* The preserving single-argument grow `operator[]` reaches; retail emits
       it out of line at 0x004700D0 for the automap scratch array and inlines
       the same shape at 0x00580C76: element construction comes from `new T[]`
       through the element type's class operator new[], then the old storage
       is released unconditionally. */
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
            release();
            data = replacement;
            capacity = new_capacity;
        }
    }

    /* `preserve` copies the overlapping prefix of the old contents into the
       new storage; callers that refill the whole array pass 0. Retail's
       GetVertexLights inlines this overload (0x0047211A): it allocates raw
       storage and null-checks the old buffer before freeing, so the new
       elements are never constructed here. */
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
                if (data != 0) {
                    srHeap.free(data);
                }
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

    /* Lazy indexed access grows by eight slots like srArray's. */
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

static_assert(sizeof(srArray<unsigned long>) == 0x08, "srArray_must_be_0x08");
static_assert(sizeof(srHeapArray<unsigned long>) == 0x08, "srHeapArray_must_be_0x08");
