#pragma once

/* VC6 needs the throw() operator delete declaration from <new> for these
   delete[] expressions; without that declaration, owner destructors gain
   unwind states retail does not have. The exact original include path is not
   recoverable, but the declaration environment is. */
#include <new>

#include "srHeap.h"

/* Concrete TEMPLATE names below are recomp pairing selectors; trivial
   construction/release and same-width copies do not prove exact T. Recover
   argument bindings from typed element use and lifetime evidence.

   SurRender's ordinary two-word growable-array boundary. Repeated
   instantiations prove the {data, capacity} layout, indexed growth rule,
   element construction, assignment, and teardown. `srArray` is a provisional
   spelling because the closed SDK's identifier did not survive; the primary
   template and its operations are compiler- and retail-proved.

   Storage goes through new T[] and delete[], so source-level element lifetime
   stays in this template while a class-specific allocation operator may choose
   the underlying heap. The reviewed scalar and pointer instantiations use the
   CRT; the reviewed vector value instantiations use class operator
   new[]/delete[] on srHeap. The renderer's vertex and triangle streams and
   Wiz8's transformed-vertex arrays are therefore this same template. Their
   emissions differ from the scalar ones
   only by the heap calls and the new[] result select a non-POD element adds
   (srArray<srVector4T<float> >::setCapacity 0x10027390 against
   srArray<float>::setCapacity 0x100274E0; the vec2 constructor 0x10027CD0 is
   byte-identical to srArray<float>'s at 0x10027CE0). */
template <class T> class srArray {
public:
    inline srArray() : data(0), capacity(0) {}

    /* The reserve form retail emits out of line for the srGERD::Renderer
       index-batch members (srArray<unsigned long> at 0x10027090, the
       unsigned-char form at 0x10026FD0): conditional exact-size storage
       through reserve(), never the preserving grow. */
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
    void reserve(unsigned long count);

    /* Retail's canonical emissions (srArray<srNode*>::setCapacity at
       0x0049E290, the folded scalar-delete destructor at 0x004701B0) use the
       scalar global operators on raw bytes. delete[] lowers to the same
       scalar operator call for these trivially destructible element types;
       sr.dll imports no vector delete emission at all. */
    inline ~srArray()
    {
        release();
    }

    /* The shared teardown sr.dll emits out of line at 0x100027D0 for the
       srHuffman::Sampler pair array. Wiz8 folds this operation at 0x004701B0
       across arrays of different element types. Both delete with the scalar
       operator and zero the pointer and capacity. */
    inline void release()
    {
        delete[] data;
        data = 0;
        capacity = 0;
    }

    /* Deep copy proved by srHuffman::Sampler's exported copy operations and
       srModeler's assignment (0x10037F90): self-check, reserve the
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
    void setCapacity(unsigned long new_capacity);

    /* Indexed preserving growth, also expanded when renderer batches reserve
       their last required index before taking pointers to the first one.
       The required index is inclusive; this is not an exact-size reserve.
       Method spelling is descriptive. */
    void ensureIndex(unsigned long index);

    T& operator[](unsigned long index);

    T* data;
    unsigned long capacity;
};

/* Ordinary header-visible primary-template definitions. Retail emits
   standalone growth bodies and also expands them in callers: for example,
   srArray<unsigned long>::operator[] (0x10027120) contains the allocation,
   preserving copy and release implemented by setCapacity (0x10027650). */
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

/* Out of line: the renderer calls it even for element 0 (drawImmediate,
   expandTriangles), while srArray<unsigned long>::operator[] (0x10027120)
   carries setCapacity expanded inside it. */
template <class T> T& srArray<T>::operator[](unsigned long index)
{
    ensureIndex(index);
    return data[index];
}

/* Raw SurRender-heap storage: allocate and free are srHeap calls on bytes,
   no element is constructed, and teardown tests the pointer before freeing.
   This is the scratch and cache family - the Renderer's byte, dword and stq
   streams, the pipeline's vertex-processor and culler lists, stMeshModel's
   per-vertex light tables and Wiz8's damage-stage and TGA buffers - as
   against srArray, whose element type picks the allocator. Every release
   emission is the checked form (0x100271A0, 0x10027250, 0x1002C860 and
   0x1001EF80 in sr.dll; 0x004741B0 in Wiz8, which is also the element
   destructor stMeshModel's constructor hands the eh-vector constructor for
   its two light tables). `srHeapBuffer` is a provisional spelling. */
template <class T> class srHeapBuffer {
public:
    /* The constructor runs ensure(0). srGERD::Renderer::Renderer keeps the
       call for its byte and dword streams (0x10024929, 0x1002493B); the
       TexCoordQ emission expands it to nothing. */
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

    /* Scratch grow: discard the old contents and allocate with 1.1 slack
       (retail 0x100271D0 for unsigned char, 0x10027280 for unsigned long). */
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

    /* Exact-size grow. `preserve` copies the overlapping prefix of the old
       contents; callers that refill the whole array pass 0. Retail emits it
       out of line for stMeshModel's light tables (0x004744A0 for srVector3T,
       0x00474650 for float), GetVertexLights inlines it (0x0047211A), and
       stModelInstance::AddDamageStage (0x00480560) inlines it for the int
       damage-stage table with preserve folded to 1, there calling the checked
       release 0x004741B0 where the other emissions expand it. */
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

    /* Deep assignment: release, allocate the source capacity, copy each
       element. Two independent retail owners assign these buffers this way:
       stModelInstance::operator= (WIZ8 0x0047EDF0) for its damage-stage
       table and stMeshModel's vClone (WIZ8 0x00474200) for both light
       tables. */
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
