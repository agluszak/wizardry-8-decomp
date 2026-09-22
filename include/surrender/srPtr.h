#pragma once

/* SurRender exposes template specializations using this spelling in its
   decorated exports. The polygon-texture accessors prove the instantiated
   object is one pointer wide and that ordinary pointer access is inline. */
template <class T> class srPtr {
public:
    srPtr() : pointer_(0) {}
    srPtr(const srPtr& other) : pointer_(0)
    {
        *this = other.pointer_;
    }
    ~srPtr()
    {
        if (pointer_ != 0) {
            pointer_->release();
        }
    }

    T* get() const
    {
        return pointer_;
    }
    T* operator->() const
    {
        return pointer_;
    }
    operator T*() const
    {
        return pointer_;
    }

    // TEMPLATE: WIZ8 0x00429B00
    // srPtr<srPalette>::operator=
    srPtr& operator=(T* pointer)
    {
        if (pointer != pointer_) {
            if (pointer != 0) {
                pointer->addReference();
            }
            if (pointer_ != 0) {
                pointer_->release();
            }
            pointer_ = pointer;
        }
        return *this;
    }

    srPtr& operator=(const srPtr& other)
    {
        return assign(&other);
    }

    /* Refcounted handoff retail inlines at every srPtr field copy: it guards
       on a null source (so assign(0) clears) and on self-assignment, then
       addrefs the incoming pointer before releasing the held one. */
    srPtr& assign(const srPtr* other)
    {
        if (other != this) {
            if (other != 0) {
                if (other->pointer_ != 0) {
                    other->pointer_->addReference();
                }
                if (pointer_ != 0) {
                    pointer_->release();
                }
                pointer_ = other->pointer_;
            } else {
                if (pointer_ != 0) {
                    pointer_->release();
                }
                pointer_ = 0;
            }
        }
        return *this;
    }

private:
    T* pointer_;
};

static_assert(sizeof(srPtr<void>) == 4, "srPtr_must_be_one_pointer");
