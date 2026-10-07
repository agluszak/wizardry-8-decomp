#pragma once

#include <string.h>

#include "srHeap.h"

struct srInlineString {

    srInlineString();
    srInlineString(const char* source);
    srInlineString(const srInlineString& source);
    srInlineString(const srInlineString& source, long begin, long end);
    ~srInlineString();

    void init();

    void reset();

    void release();

    srInlineString& operator=(const char* source);
    srInlineString& operator=(const srInlineString& source);
    srInlineString& operator+=(const char* suffix);

    long find(const srInlineString& needle, unsigned long offset) const;
    void erase(unsigned long begin, unsigned long end);
    void insert(const srInlineString& text, unsigned long position);

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

inline srInlineString::srInlineString(const srInlineString& source, long begin, long end)
{
    init();
    char* temporary = static_cast<char*>(srHeap.allocate(end - begin + 2));
    strncpy(temporary, source.data_ + begin, end - begin);
    temporary[end - begin] = '\0';
    operator=(temporary);
    srHeap.free(temporary);
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
// RECOMP: ??YsrInlineString@@QAEAAU0@PBD@Z
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
// RECOMP: ?find@srInlineString@@QBEJABU1@K@Z
inline long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}
#endif
