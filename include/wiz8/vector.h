#ifndef WIZ8_VECTOR_H
#define WIZ8_VECTOR_H

#include <new>

/* A hand-rolled growable array used by the engine and UI. */
template <class T> class W8GrowableVector {
public:
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

    /* The copy's capacity is the source's live count. */
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

        /* Insertion grows capacity by five entries. */
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

    /* Removes the entry at position and deletes the object it pointed at. */
    void RemoveAtAndDelete(int position);

    /* Removes the first matching entry and reports whether one was present. */
    unsigned char Remove(T entry);

    /* Returns the first matching index, or -1 when the value is absent. */
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

template <class T> void W8GrowableVector<T>::RemoveAtAndDelete(int position)
{
    int index;
    T entry;

    if (position < count && position >= 0) {
        entry = data[position];
        for (index = position; index < count - 1; ++index) {
            data[index] = data[index + 1];
        }
        --count;
        delete entry;
    }
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

/* A distinct growable-vector subtype. */
template <class T> class W8Vector : public W8GrowableVector<T> {
public:
    explicit W8Vector(int initial_capacity = 5) : W8GrowableVector<T>(initial_capacity) {}
};

#endif
