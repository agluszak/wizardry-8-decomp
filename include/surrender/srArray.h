#pragma once

#include "srHeap.h"

/* SurRender's ordinary two-word growable-array boundary. Repeated
   instantiations prove the {data, capacity} layout, indexed growth rule,
   element construction, assignment, and teardown. `srArray` is a provisional
   spelling because the closed SDK's identifier did not survive; the primary
   template and its operations are compiler- and retail-proved. */
template <class T> class srArray {
public:
    // TEMPLATE: SURRENDER 0x10027CE0
    // srArray<float>::srArray
    inline srArray() : data(0), capacity(0) {}

    /* The reserve form retail emits out of line for the srGERD::Renderer
       index-batch members (srArray<unsigned long> at 0x10027090, the
       unsigned-char form at 0x10026FD0): conditional exact-size storage
       through reserve(), never the preserving grow. */
    // TEMPLATE: SURRENDER 0x10026FD0
    // srArray<unsigned char>::srArray
    // TEMPLATE: SURRENDER 0x10027090
    // srArray<unsigned long>::srArray
    inline explicit srArray(unsigned long reserve_capacity) : data(0), capacity(0)
    {
        if (reserve_capacity != 0) {
            reserve(reserve_capacity);
        }
    }

    /* Default-init then assign: srHuffman::Sampler's exported copy
       constructor (0x100014F0) zeroes its srArray<Symbol> member and inlines
       the full operator= body, and srModeler's copy constructor (0x10037DE0)
       does the same for srArray<Triangle>. */
    inline srArray(const srArray& other) : data(0), capacity(0)
    {
        *this = other;
    }

    /* Free-then-allocate exact storage without preserving contents; the
       reserve constructor reaches it. srArray<Triangle>'s emission
       (0x10038120) constructs every element: new T[] scalar-allocates for
       these trivially destructible element types while still running each
       element constructor, matching retail. */
    // TEMPLATE: SURRENDER 0x10038120
    // srArray<srModeler::Triangle>::reserve
    void reserve(unsigned long count);

    /* Retail's canonical emissions (srArray<srNode*>::setCapacity at
       0x0049E290, the folded scalar-delete destructor at 0x004701B0) use the
       scalar global operators on raw bytes. delete[] lowers to the same
       scalar operator call for these trivially destructible element types;
       sr.dll imports no vector delete emission at all. */
    // TEMPLATE: SURRENDER 0x10026F10
    // srArray<float>::~srArray
    inline ~srArray()
    {
        release();
    }

    /* The shared teardown sr.dll emits out of line at 0x100027D0 for the
       srHuffman::Sampler pair array. Wiz8 folds this operation at 0x004701B0
       across arrays of different element types. Both delete with the scalar
       operator and zero the pointer and capacity. */
    // TEMPLATE: SURRENDER 0x10026DE0
    // srArray<srGERD::Renderer::TextureSet>::release
    // TEMPLATE: SURRENDER 0x10026F30
    // srArray<float>::release
    // TEMPLATE: SURRENDER 0x10027020
    // srArray<unsigned char>::release
    // TEMPLATE: SURRENDER 0x10027100
    // srArray<unsigned long>::release
    inline void release()
    {
        delete[] data;
        data = 0;
        capacity = 0;
    }

    /* Deep copy proved by srHuffman::Sampler's exported copy operations and
       srModeler's implicit assignment (0x10037F90): self-check, reserve the
       source capacity (release + exact-size allocate), copy that many
       elements. */
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

    /* srArray<Triangle>'s emission (0x1003BCF0) constructs every element of
       the new storage before copy-assigning the preserved prefix; new T[]
       emits that construction for the non-trivial element type. */
    // TEMPLATE: SURRENDER 0x100274E0
    // srArray<float>::setCapacity
    // TEMPLATE: SURRENDER 0x10027550
    // srArray<unsigned char>::setCapacity
    // TEMPLATE: SURRENDER 0x10027650
    // srArray<unsigned long>::setCapacity
    // TEMPLATE: SURRENDER 0x10027720
    // srArray<srGERD::Renderer::TextureSet>::setCapacity
    // TEMPLATE: SURRENDER 0x1003BCF0
    // srArray<srModeler::Triangle>::setCapacity
    // TEMPLATE: SURRENDER 0x10044EE0
    // srArray<srTriMeshPipeline::Record>::setCapacity
    // TEMPLATE: SURRENDER 0x10044E60
    // srArray<srVertexArray>::setCapacity
    // TEMPLATE: SURRENDER 0x10045030
    // srArray<srTriMeshPipeline::Pass>::setCapacity
    void setCapacity(unsigned long new_capacity);

    // TEMPLATE: SURRENDER 0x10026F50
    // srArray<float>::operator[]
    // TEMPLATE: SURRENDER 0x10027120
    // srArray<unsigned long>::operator[]
    // TEMPLATE: SURRENDER 0x10044E30
    // srArray<srTriMeshPipeline::Record>::operator[]
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

/* Out-of-class with auto-inlining disabled so the instantiations emit as
   standalone calls the way retail callers reach them: srModeler's accessors
   inline operator[]'s grow check then CALL 0x1003BCF0 rather than inlining
   the allocation loop. */
#pragma auto_inline(off)
template <class T> void srArray<T>::reserve(unsigned long count)
{
    release();
    if (count > 0) {
        capacity = count;
        data = new T[count];
    }
}

template <class T> void srArray<T>::setCapacity(unsigned long new_capacity)
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
#pragma auto_inline(on)

/* The separately proved srHeap-backed family has both preserving exact-size
   storage and a scratch-buffer operation that discards old contents when it
   grows. `srHeapArray` is a provisional spelling, not a per-element wrapper or
   specialization. Its constructor invokes ensure(0); retail retains that call
   even for an empty request. */
template <class T> class srHeapArray {
public:
    // TEMPLATE: SURRENDER 0x10027CD0
    // srHeapArray<srVector2T<float> >::srHeapArray
    inline srHeapArray() : data(0), capacity(0)
    {
        ensure(0);
    }

    /* The reserve form retail emits out of line for the srGERD::Renderer
       vertex streams (srHeapArray<srVector4T<float>> at 0x10026E00):
       conditional exact-size storage through reserve(), never the scaled
       ensure or preserving setCapacity. */
    // TEMPLATE: SURRENDER 0x10026E00
    // srHeapArray<srVector4T<float> >::srHeapArray
    inline explicit srHeapArray(unsigned long reserve_count) : data(0), capacity(0)
    {
        if (reserve_count != 0) {
            reserve(reserve_count);
        }
    }

    /* Free-then-allocate exact storage without preserving contents; the
       reserve constructor reaches it (retail 0x10027330 for the vec4
       stream). */
    // TEMPLATE: SURRENDER 0x10027330
    // srHeapArray<srVector4T<float> >::reserve
    inline void reserve(unsigned long count)
    {
        release();
        if (count > 0) {
            capacity = count;
            data = allocate(count);
        }
    }

    // TEMPLATE: SURRENDER 0x10026EA0
    // srHeapArray<srVector2T<float> >::~srHeapArray
    inline ~srHeapArray()
    {
        release();
    }

    /* The canonical emission at 0x004701D0 frees unconditionally; srHeap.free
       accepts a null pointer. */
    // TEMPLATE: SURRENDER 0x10026E50
    // srHeapArray<srVector4T<float> >::release
    // TEMPLATE: SURRENDER 0x10026EC0
    // srHeapArray<srVector2T<float> >::release
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
       the same shape at 0x00580C76: it allocates raw storage through
       srHeap.allocate like release() and the other members, then the old
       storage is released unconditionally — no element construction. */
    // TEMPLATE: SURRENDER 0x10027390
    // srHeapArray<srVector4T<float> >::setCapacity
    // TEMPLATE: SURRENDER 0x10027440
    // srHeapArray<srVector2T<float> >::setCapacity
    // TEMPLATE: SURRENDER 0x100275B0
    // srHeapArray<srVector3i>::setCapacity
    inline void setCapacity(unsigned long new_capacity)
    {
        if (capacity != new_capacity) {
            T* replacement = 0;
            if (new_capacity > 0) {
                replacement = allocate(new_capacity);
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
    // TEMPLATE: SURRENDER 0x10026E70
    // srHeapArray<srVector4T<float> >::operator[]
    // TEMPLATE: SURRENDER 0x10026EE0
    // srHeapArray<srVector2T<float> >::operator[]
    // TEMPLATE: SURRENDER 0x10027060
    // srHeapArray<srVector3i>::operator[]
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

/* The Renderer's scratch buffers share the {data, capacity} pair, srHeap
   storage and the 1.1-slack ensure grow, but teardown null-checks the
   pointer: ~Renderer inlines `if (data != 0) srHeap.free(data); {0,0}` for
   the byte, dword and stq streams while the vertex streams use srHeapArray's
   unconditional release, and the eharray destructor at 0x10027300 emits the
   same check. `srHeapBuffer` is a provisional spelling like its siblings. */
template <class T> class srHeapBuffer {
public:
    // TEMPLATE: SURRENDER 0x10024AD0
    // srHeapBuffer<srGERD::Renderer::TexCoordQ>::srHeapBuffer
    inline srHeapBuffer() : data(0), capacity(0)
    {
        ensure(0);
    }

    // TEMPLATE: SURRENDER 0x10027300
    // srHeapBuffer<srGERD::Renderer::TexCoordQ>::~srHeapBuffer
    inline ~srHeapBuffer()
    {
        release();
    }

    /* Unlike srHeapArray::release this tests the pointer first, matching the
       checked-free teardowns ~Renderer and Renderer::reset emit inline. */
    inline void release()
    {
        if (data != 0) {
            srHeap.free(data);
        }
        data = 0;
        capacity = 0;
    }

    // TEMPLATE: SURRENDER 0x100276C0
    // srHeapBuffer<unsigned char>::allocate
    // TEMPLATE: SURRENDER 0x100276E0
    // srHeapBuffer<unsigned long>::allocate
    // TEMPLATE: SURRENDER 0x10027700
    // srHeapBuffer<srGERD::Renderer::TexCoordQ>::allocate
    static inline T* allocate(unsigned long count)
    {
        return static_cast<T*>(srHeap.allocate(count * sizeof(T)));
    }

    /* The same scaled grow srHeapArray::ensure emits, routed through the
       checked release (retail 0x100271D0 for unsigned char, 0x10027280 for
       unsigned long). */
    // TEMPLATE: SURRENDER 0x100271D0
    // srHeapBuffer<unsigned char>::ensure
    // TEMPLATE: SURRENDER 0x10027280
    // srHeapBuffer<unsigned long>::ensure
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

    T* data;
    unsigned long capacity;
};

static_assert(sizeof(srArray<unsigned long>) == 0x08, "srArray_must_be_0x08");
static_assert(sizeof(srHeapArray<unsigned long>) == 0x08, "srHeapArray_must_be_0x08");
