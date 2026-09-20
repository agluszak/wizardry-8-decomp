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

   Each product carries this class in the translation unit that uses it, and
   each TU owns its definitions. Wiz8.exe emits its copies inside the
   VirtualFileBinIStream translation unit (0x0047CDD0-0x0047D48F, the class's
   only .text reach): there the ctors and destructor expand inline at call
   sites while find/erase/insert/operator+=/operator+ are plain out-of-line
   member functions defined in that unit. srEXT_Unzip.dll instead calls every
   method out-of-line - even the trivial default constructor - and its
   (const char*) constructor contains a spelled-out copy of the assignment
   rather than delegating to it, its reference assignment delegates where
   Wiz8's performs destroy-then-copy, and its copy constructor copies inline
   where Wiz8's delegates to the reference assignment - which proves the two
   products do not share the same method bodies. Only the members below are
   identical in both products. */
struct srInlineString {
    /* Bare empty-state construction - the retail expansions write the three
       fields directly rather than calling init. The SurRender translation
       units keep the spelling as a TU-local inline copy; there is no shared
       strong emission. */
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
