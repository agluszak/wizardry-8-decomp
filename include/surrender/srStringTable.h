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
    srStringTable(const srStringTable& other);
    ~srStringTable();
    srStringTable& operator=(const srStringTable& other);
    char* operator[](int index);

    void addString(const char* string);
    void addSeparatedStrings(const char* strings, const char* separators, int append_slash);
    // FUNCTION: SURRENDER 0x10003A60
    long getCount() const
    {
        return count_08;
    }
    char* getString(long index) const;
    void reset();

private:
    /* The copy constructor emits srArray's assignment shape verbatim
       (member self-check on the table pointers, release, unsigned
       capacity>0 grow, elementwise copy bounded by capacity), and
       ~srStringTable's only teardown is reset() plus the implicit
       ~srArray member release - the slot array is an ordinary srArray
       member, not a hand-rolled pair. */
    srArray<char*> strings_00;
    long count_08;
};

static_assert((sizeof(srStringTable) == 0x0c), "srStringTable_must_be_0x0c");

#undef SR_STRING_TABLE_API
