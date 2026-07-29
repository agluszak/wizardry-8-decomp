#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

#include <new>

/* One hand-rolled growable-array template. Each element type emits its own
   constructor, destructor, vtable and element-width-specific methods. */
template <class T>
class W8GrowableVector {
public:
    explicit W8GrowableVector(int initial_capacity = 5);
    virtual ~W8GrowableVector();

    int Grow(int minimum_capacity);

    int GetCount() const
    {
        return count;
    }

    T* GetAt(int position)
    {
        T* result = data;

        if (position < count) {
            result = data + position;
        }
        return result;
    }

    int Add(T value)
    {
        int position = count + 1;

        if (position > capacity && !Grow(position)) {
            return -1;
        }
        data[count] = value;
        return count++;
    }

    /* Returns the element it unlinked. GenerateItemsFromTable discards that
       value, while callers such as the dialog destructor delete it. */
    T RemoveAt(int position);

    /* The image walks the array from a pointer loaded once rather than
       indexing through GetAt, which bounds-checks. Controls.cpp:2718 asserts on
       the -1 this returns, so the not-found value is the source's own. */
    int IndexOf(T value)
    {
        T* scan = data;
        int index;

        for (index = 0; index < count; ++index) {
            if (*scan == value) {
                return index;
            }
            ++scan;
        }
        return -1;
    }

    void Clear()
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

/* The initial capacity is a parameter, clamped to at least one. Every site that
   constructs with the default folds both the clamp and the multiply away, which
   is why the inlined copies show only `operator new(20)`; the out-of-line
   constructor emissions keep the parameter and are what proves it. Whether the
   original spelled a default argument or a separate default constructor the
   image cannot say, because every capacity-5 site is inlined. */
template <class T>
W8GrowableVector<T>::W8GrowableVector(int initial_capacity)
{
    if (initial_capacity < 1) {
        initial_capacity = 1;
    }
    data = static_cast<T*>(::operator new(initial_capacity * sizeof(T)));
    count = 0;
    if (data != 0) {
        capacity = initial_capacity;
    }
    else {
        capacity = 0;
    }
}

template <class T>
W8GrowableVector<T>::~W8GrowableVector()
{
    ::operator delete(data);
}

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

template <class T>
T W8GrowableVector<T>::RemoveAt(int position)
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

#endif
