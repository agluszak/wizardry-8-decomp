#pragma once

#include <string.h>

#include "srHeap.h"

/* SurRender's narrow string class: a 12-byte object whose first four bytes
   double as the empty string's inline area. +0x04 holds the size including
   the terminator (1 while empty) and +0x08 the data pointer, which equals the
   object address exactly while the string is empty. Storage comes from
   srHeap, not global operator new - the same vendor allocator split as the
   SurRender container templates.

   srInlineString is a provisional identifier: no decorated export or retail
   string names the class.

   Product variants have independently evidenced behavior. Wizardry and the
   unzip extension retain their product-owned definitions; SurRender's
   cross-TU expansions share the canonical provider definitions below. */
struct srInlineString {
    /* Empty-state construction shared by the provider translation units. */
    srInlineString();
    srInlineString(const char* source);
    srInlineString(const srInlineString& source);
    srInlineString(const srInlineString& source, long begin, long end);
    ~srInlineString();

    /* Bare empty-state initialization without releasing storage - the helper
       the provider emits at 0x10004150 and calls inside assignment/copy
       expansions. */
    void init();

    /* Empty-object reinitialization. In the Wiz8 unit the inline destructor
       expansion keeps the release inline but emits a call to the reset
       emission - the function retail lists at 0x0047D290. In the provider
       unit reset itself releases non-inline storage (0x10012C80). */
    void reset();

    /* Releases the object's contents and returns it to the empty state - the
       operation the destructor and copy assignment share. In the Wiz8 unit
       its only emitted form is the destructor body (0x0047CDD0). */
    void release();

    srInlineString& operator=(const char* source);
    srInlineString& operator=(const srInlineString& source);
    srInlineString& operator+=(const char* suffix);

    long find(const srInlineString& needle, unsigned long offset) const;
    void erase(unsigned long begin, unsigned long end);
    void insert(const srInlineString& text, unsigned long position);
    /* Single-shot search-and-replace: the stream unit emits it at
       0x10032B00 and loops it for separator normalization. */
    int replace(const srInlineString& needle, const srInlineString& replacement);

    char* data()
    {
        return data_;
    }
    const char* data() const
    {
        return data_;
    }
    unsigned long size() const
    {
        return size_;
    }

    void erasePrefix(unsigned long count)
    {
        if (count == 0 || count >= size_) {
            operator=("");
            return;
        }
        strncpy(data_, data_ + count, size_ - count);
        size_ = strlen(data_) + 1;
    }

    char inline_[4];
    unsigned long size_;
    char* data_;
};

static_assert((sizeof(srInlineString) == 0x0c), "srInlineString_must_be_0x0c");

srInlineString operator+(const srInlineString& left, const srInlineString& right);

#if defined(SURRENDER_BUILD)
/* Retail expands these methods across config, stream, string-table and
   dynamic-library TUs and also retains standalone emissions. */
inline srInlineString::srInlineString()
{
    init();
}

inline srInlineString::srInlineString(const char* source)
{
    init();
    operator=(source);
}

inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source);
    }
}

// FUNCTION: SURRENDER 0x100040A0
inline srInlineString::~srInlineString()
{
    reset();
}

// FUNCTION: SURRENDER 0x100040D0
inline srInlineString& srInlineString::operator=(const char* source)
{
    reset();
    if (source == 0 || *source == '\0') {
        return *this;
    }
    size_ = strlen(source) + 1;
    data_ = static_cast<char*>(srHeap.allocate(size_));
    strcpy(data_, source);
    return *this;
}

inline srInlineString& srInlineString::operator=(const srInlineString& source)
{
    init();
    if (source.data_ != 0 && *source.data_ != '\0') {
        size_ = strlen(source.data_) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, source.data_);
    }
    return *this;
}

inline void srInlineString::erase(unsigned long begin, unsigned long end)
{
    if (begin != end) {
        strncpy(data_ + begin, data_ + end, size_ - end);
        size_ = strlen(data_) + 1;
    }
}

// FUNCTION: SURRENDER 0x10032E30 SYMBOL
// ?operator+=@srInlineString@@QAEAAV1@PBD@Z
inline srInlineString& srInlineString::operator+=(const char* suffix)
{
    if (suffix != 0 && *suffix != '\0') {
        const unsigned long needed = strlen(suffix) + size_;
        char* buffer = static_cast<char*>(srHeap.allocate(needed));
        strcpy(buffer, data_);
        strcpy(buffer + size_ - 1, suffix);
        reset();
        size_ = needed;
        data_ = buffer;
    }
    return *this;
}

// FUNCTION: SURRENDER 0x100467E0 SYMBOL
// ?find@srInlineString@@QBEJABV1@K@Z
inline long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}
#endif
