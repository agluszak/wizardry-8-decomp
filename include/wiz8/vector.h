#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

#include <new>

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

    /* Returns the element it unlinked. GenerateItemsFromTable discards that
       value, so the ItemManager inlining alone does not show it; the dialog
       destructor at 0x005D1590 deletes what the same shifting body returns. */
    __forceinline T RemoveAt(int position)
    {
        int index;
        T result;

        if (position >= count || position < 0) {
            return 0;
        }
        result = data[position];
        for (index = position; index < count - 1; ++index) {
            data[index] = data[index + 1];
        }
        --count;
        return result;
    }

    __forceinline void Clear()
    {
        count = 0;
    }

    int count;                           /* 0x04 */
    int capacity;                        /* 0x08 */
    T* data;                             /* 0x0c */
};                                      /* 0x10 in the 32-bit target */

/* An instantiation whose element type is still unproven is named for the vtable
   the image gives it, so two owners share a spelling exactly when they share an
   instantiation. Erasing every such element to one `void*` spelling would do
   the opposite: 0x005EBFE0 and 0x005EC0E0 are different specializations and
   only the second is W8GrowableVector<int>.

   0x005EBFE0 is embedded by W8MonsterManagerEntry at +0xd8, by
   W8MonsterManagerState at +0x9b7, and by nineteen further owner bodies listed
   in evidence/snapshots/polymorphism/vptr-writes.csv. */
class W8VectorElement005EBFE0;

template <class T>
__forceinline W8GrowableVector<T>::W8GrowableVector()
{
    data = static_cast<T*>(::operator new(5 * sizeof(T)));
    count = 0;
    if (data != 0) {
        capacity = 5;
    }
    else {
        capacity = 0;
    }
}

template <class T>
__forceinline W8GrowableVector<T>::~W8GrowableVector()
{
    ::operator delete(data);
}

// TEMPLATE: WIZ8 0x004ADDF0
// W8GrowableVector<int>::Grow
template <class T>
int W8GrowableVector<T>::Grow(int minimum_capacity)
{
    int index;
    T* previous_data;
    T* replacement;

    if (minimum_capacity > capacity) {
        previous_data = data;
        replacement = static_cast<T*>(::operator new(minimum_capacity * sizeof(T)));
        data = replacement;
        if (replacement == 0) {
            data = previous_data;
            return 0;
        }
        capacity = minimum_capacity;
        for (index = 0; index < count; ++index) {
            data[index] = previous_data[index];
        }
        ::operator delete(previous_data);
    }
    return 1;
}

#endif
