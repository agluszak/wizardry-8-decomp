#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

#include <new>

/* One hand-rolled growable-array template. Each element type emits its own
   constructor, destructor, vtable and element-width-specific methods. */
template <class T> class W8GrowableVector {
public:
    /* Retail default construction goes through this one constructor with
       five: every site emits PUSH 5 before the call, whether the compiler
       calls the out-of-line emission or inlines the capacity constant. */
    explicit W8GrowableVector(int initial_capacity = 5)
    {
        if (initial_capacity < 1) {
            initial_capacity = 1;
        }
        data = new T[initial_capacity];
        count = 0;
        if (data != 0) {
            capacity = initial_capacity;
        } else {
            capacity = 0;
        }
    }

    /* The copy is sized to the source's live count rather than its capacity.
       The concrete int body at 0x004ED900 is an ordinary template emission. */
    W8GrowableVector(const W8GrowableVector& other)
    {
        data = new T[other.count];
        count = other.count;
        capacity = other.count;
        for (int index = 0; index < count; ++index) {
            data[index] = other.data[index];
        }
    }

    virtual ~W8GrowableVector()
    {
        delete[] data;
    }
    W8GrowableVector& operator=(const W8GrowableVector& other);

    int Grow(int minimum_capacity)
    {
        int index;
        T* previous_data;
        T* replacement;

        if (minimum_capacity > capacity) {
            previous_data = data;
            replacement = new T[minimum_capacity];
            data = replacement;
            if (replacement == 0) {
                data = previous_data;
                return 0;
            }
            capacity = minimum_capacity;
            for (index = 0; index < count; ++index) {
                data[index] = previous_data[index];
            }
            delete[] previous_data;
        }
        return 1;
    }

    int GetCount() const
    {
        return count;
    }

    T* GetAt(int position)
    {
        if (position < count) {
            return data + position;
        }
        return data;
    }

    const T* GetAt(int position) const
    {
        if (position < count) {
            return data + position;
        }
        return data;
    }

    T SetAt(int position, T value)
    {
        T previous;

        if (position >= count) {
            return 0;
        }
        previous = data[position];
        data[position] = value;
        return previous;
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

    unsigned char InsertAt(int position, T value)
    {
        int index;

        /* Retail insertion grows by five, unlike Add's minimum-sized growth
           (005D21F0 pointer entries and 004C80E0 script-condition bytes). */
        if (count + 1 > capacity && !Grow(capacity + 5)) {
            return 0;
        }
        for (index = count; index > position; --index) {
            data[index] = data[index - 1];
        }
        data[position] = value;
        ++count;
        return 1;
    }

    /* Returns the element it unlinked. GenerateItemsFromTable discards that
       value, while callers such as the dialog destructor delete it. */
    T RemoveAt(int position);

    /* Removes the first matching entry, if any, and reports whether one was
       there. The startup entry queues reach it through QueueEntry. */
    unsigned char Remove(T entry);

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

    int count;    /* 0x04 */
    int capacity; /* 0x08 */
    T* data;      /* 0x0c */
}; /* 0x10 in the 32-bit target */

template <class T>
W8GrowableVector<T>& W8GrowableVector<T>::operator=(const W8GrowableVector<T>& other)
{
    int index;

    count = 0;
    if (other.count > capacity && !Grow(other.count)) {
        return *this;
    }
    for (index = 0; index < other.count; ++index) {
        data[index] = other.data[index];
    }
    if (count <= other.count) {
        count = other.count;
    }
    return *this;
}

template <class T> T W8GrowableVector<T>::RemoveAt(int position)
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

template <class T> unsigned char W8GrowableVector<T>::Remove(T entry)
{
    int index = 0;

    while (index < count) {
        if (data[index] == entry) {
            RemoveAt(index);
            return 1;
        }
        ++index;
    }
    return 0;
}

/* Thin derived collection: identical layout and inherited behavior, but the
   retail image gives it its own vtable and deleting destructor (0x005EC018
   over the base 0x005EC004 for the stModelInstance* instantiation, with the
   base constructor emitted at 0x004390F0 and the derived deleting destructor
   at 0x00438F70). The octree model-instance queries take the base pointer. */
template <class T> class W8Vector : public W8GrowableVector<T> {
public:
    W8Vector() {}
    explicit W8Vector(int initial_capacity) : W8GrowableVector<T>(initial_capacity) {}
};

#endif
