#pragma once

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
    void addSeparatedStrings(
        const char* strings, const char* separators, int append_slash);
    long getCount() const;
    char* getString(long index) const;
    void reset();

private:
    char** strings_00;
    long capacity_04;
    long count_08;
};

static_assert((sizeof(srStringTable) == 0x0c), "srStringTable_must_be_0x0c");

#undef SR_STRING_TABLE_API
