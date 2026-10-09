#pragma once

#include "srArray.h"

#if defined(SURRENDER_BUILD)
#define SR_STRING_TABLE_API __declspec(dllexport)
#elif defined(_MSC_VER) && !defined(WIZ8_CLANG_LINT)
#define SR_STRING_TABLE_API __declspec(dllimport)
#else
#define SR_STRING_TABLE_API
#endif

class SR_STRING_TABLE_API srStringTable {
public:
    srStringTable();

    ~srStringTable();
    srStringTable& operator=(const srStringTable& other);
    char* operator[](int index);

    void addString(const char* string);
    void addSeparatedStrings(const char* strings, const char* separators, int append_slash);
    // FUNCTION: SURRENDER 0x10003A60
    w8_long getCount() const
    {
        return count;
    }
    char* getString(w8_long index) const;
    void reset();

private:
    /* Copy construction uses srArray assignment and destruction includes the
       implicit srArray teardown, identifying this slot storage as srArray<char*>. */
    srArray<char*> strings;
    w8_long count;
};

W8_ABI_ASSERT((sizeof(srStringTable) == 0x0c), "srStringTable_must_be_0x0c");

#undef SR_STRING_TABLE_API
