#include "surrender/srStringTable.h"

#include <string.h>

// FUNCTION: SURRENDER 0x10003840
srStringTable::srStringTable()
    : strings_00(0), capacity_04(0), count_08(0)
{
}

// FUNCTION: SURRENDER 0x10003850
void srStringTable::reset()
{
    for (long index = 0; index < count_08; ++index) {
        if (strings_00[index] != 0) {
            delete[] strings_00[index];
            strings_00[index] = 0;
        }
    }
    delete[] strings_00;
    strings_00 = 0;
    capacity_04 = 0;
    count_08 = 0;
}

// FUNCTION: SURRENDER 0x10003910
srStringTable::~srStringTable()
{
    reset();
    delete[] strings_00;
    strings_00 = 0;
    capacity_04 = 0;
}

// FUNCTION: SURRENDER 0x10003970
void srStringTable::addString(const char* string)
{
    if (string == 0 || *string == '\0') {
        return;
    }

    if (count_08 >= capacity_04) {
        const long capacity = capacity_04 + count_08 + 8;
        char** strings = new char*[capacity];
        for (long index = 0; index < capacity_04; ++index) {
            strings[index] = strings_00[index];
        }
        delete[] strings_00;
        strings_00 = strings;
        capacity_04 = capacity;
    }

    strings_00[count_08] = new char[strlen(string) + 1];
    strcpy(strings_00[count_08], string);
    ++count_08;
}

// FUNCTION: SURRENDER 0x10003A40
char* srStringTable::getString(long index) const
{
    if (index < 0 || index >= count_08) {
        return 0;
    }
    return strings_00[index];
}

// FUNCTION: SURRENDER 0x10003A60
long srStringTable::getCount() const
{
    return count_08;
}

// FUNCTION: SURRENDER 0x10003A70
char* srStringTable::operator[](int index)
{
    return getString(index);
}

// FUNCTION: SURRENDER 0x10003A80
srStringTable& srStringTable::operator=(const srStringTable& other)
{
    if (this != &other) {
        reset();
        for (long index = 0; index < other.getCount(); ++index) {
            addString(other.getString(index));
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10003AC0
srStringTable::srStringTable(const srStringTable& other)
    : strings_00(0), capacity_04(0), count_08(other.count_08)
{
    if (other.capacity_04 != 0) {
        capacity_04 = other.capacity_04;
        strings_00 = new char*[capacity_04];
        for (long index = 0; index < capacity_04; ++index) {
            strings_00[index] = other.strings_00[index];
        }
    }
}

// FUNCTION: SURRENDER 0x10003B40
void srStringTable::addSeparatedStrings(
    const char* strings, const char* separators, int append_slash)
{
    if (strings == 0 || *strings == '\0') {
        return;
    }

    if (separators == 0 || *separators == '\0') {
        const size_t length = strlen(strings);
        char* string = new char[length + (append_slash ? 2 : 1)];
        strcpy(string, strings);
        if (append_slash && string[length - 1] != '/' &&
            string[length - 1] != '\\') {
            strcat(string, "/");
        }
        addString(string);
        delete[] string;
        return;
    }

    const char* current = strings;
    while (*current != '\0') {
        current += strspn(current, separators);
        const size_t length = strcspn(current, separators);
        if (length == 0) {
            break;
        }

        char* string = new char[length + (append_slash ? 2 : 1)];
        strncpy(string, current, length);
        string[length] = '\0';
        if (append_slash && string[length - 1] != '/' &&
            string[length - 1] != '\\') {
            strcat(string, "/");
        }
        addString(string);
        delete[] string;
        current += length;
    }
}
