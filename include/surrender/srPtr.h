#pragma once

/* SurRender exposes template specializations using this spelling in its
   decorated exports. The polygon-texture accessors prove the instantiated
   object is one pointer wide and that ordinary pointer access is inline. */
template<class T>
class srPtr {
public:
    T* get() const { return pointer_; }
    T* operator->() const { return pointer_; }
    operator T*() const { return pointer_; }

private:
    T* pointer_;
};

static_assert(sizeof(srPtr<void>) == 4, "srPtr_must_be_one_pointer");
