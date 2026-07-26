#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

/* One hand-rolled growable-array template. Each element type emits its own
   vtable and deleting destructor, while four-byte instantiations share the
   same machine-code shape for Grow. The original template name is not known. */
template <class T>
class W8GrowableVector {
public:
    W8GrowableVector();
    virtual ~W8GrowableVector();

    int Grow(int minimum_capacity);

    __forceinline int GetCount() const
    {
        return count;
    }

    __forceinline T* GetAt(int position)
    {
        T* result = data;

        if (position < count) {
            result = data + position;
        }
        return result;
    }

    int Add(T value)
    {
        if (count + 1 <= capacity || Grow(count + 1)) {
            data[count] = value;
            ++count;
            return 1;
        }
        return 0;
    }

    __forceinline void RemoveAt(int position)
    {
        int index;

        if (position < count && position >= 0) {
            for (index = position; index < count - 1; ++index) {
                data[index] = data[index + 1];
            }
            --count;
        }
    }

    __forceinline void Clear()
    {
        count = 0;
    }

    int count;                           /* 0x04 */
    int capacity;                        /* 0x08 */
    T* data;                             /* 0x0c */
};                                      /* 0x10 in the 32-bit target */

/* Evidence-only erased spelling for owners whose element type is not yet
   represented in their header. It is a template use, not another container. */
typedef W8GrowableVector<void*> W8PtrVector;

#endif
