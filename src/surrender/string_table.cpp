#include "surrender/srStringTable.h"

#include "surrender/srHeap.h"
#include "surrender/srString.h"

#include <string.h>

// FUNCTION: SURRENDER 0x10003840
srStringTable::srStringTable() : strings_00(), count_08(0) {}

// FUNCTION: SURRENDER 0x10003850
void srStringTable::reset()
{
    for (long index = 0; index < count_08; ++index) {
        char*& string = strings_00[index];
        if (string != 0) {
            srHeap.free(string);
            string = 0;
        }
    }
    if (strings_00.capacity != 0) {
        strings_00.release();
    }
    count_08 = 0;
}

// FUNCTION: SURRENDER 0x10003910
srStringTable::~srStringTable()
{
    /* The trailing array teardown is the implicit srArray member destruction. */
    reset();
}

// FUNCTION: SURRENDER 0x10003970
void srStringTable::addString(const char* string)
{
    if (string == 0 || *string == '\0') {
        return;
    }

    /* This spelling emits one operator[] grow path before allocation, matching retail. */
    char*& slot = strings_00[count_08];
    char* copy = static_cast<char*>(srHeap.allocate(strlen(string) + 1));
    slot = copy;
    strcpy(copy, string);
    ++count_08;
}

// FUNCTION: SURRENDER 0x10003A40
char* srStringTable::getString(long index) const
{
    if (index < 0 || index >= count_08) {
        return 0;
    }
    return strings_00.data[index];
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

// FUNCTION: SURRENDER 0x10003B40
void srStringTable::addSeparatedStrings(const char* strings, const char* separators,
                                        int append_slash)
{
    srInlineString buffer;
    if (strings != 0) {
        buffer = strings;
    }
    srInlineString separator_copy;
    if (separators != 0) {
        separator_copy = separators;
    }
    char separator_string[2] = " ";
    bool has_more = true;
    const unsigned long separator_count = separator_copy.size() - 1;
    if (separator_count == 0) {
        if (buffer.size() - 1 != 0) {
            if (append_slash && buffer.data()[buffer.size() - 2] != '/' &&
                buffer.data()[buffer.size() - 2] != '\\') {
                buffer += "/";
            }
            addString(buffer.data());
        }
        return;
    }
    for (;;) {
        const unsigned long length = buffer.size() - 1;
        unsigned long prefix = 0;
        while (prefix < length) {
            unsigned long separator = 0;
            while (buffer.data()[prefix] != separators[separator]) {
                ++separator;
                if (separator >= separator_count) {
                    break;
                }
            }
            if (separator >= separator_count) {
                break;
            }
            ++prefix;
        }
        if (prefix == length) {
            return;
        }
        if (prefix != 0) {
            buffer.erase(0, prefix);
        }

        long piece_end = -1;
        for (unsigned long separator = 0; separator < separator_count; ++separator) {
            separator_string[0] = separators[separator];
            srInlineString needle(separator_string);
            const long position = buffer.find(needle, 0);
            if (position != -1 && (piece_end == -1 || position < piece_end)) {
                piece_end = position;
            }
        }
        if (piece_end == -1) {
            piece_end = buffer.size() - 1;
            has_more = false;
        }
        if (piece_end > 0) {
            srInlineString piece;
            char* extracted = static_cast<char*>(srHeap.allocate(piece_end + 2));
            strncpy(extracted, buffer.data(), piece_end);
            extracted[piece_end] = '\0';
            piece = extracted;
            srHeap.free(extracted);
            if (piece.size() - 1 > 0) {
                if (append_slash && piece.data()[piece.size() - 2] != '/' &&
                    piece.data()[piece.size() - 2] != '\\') {
                    piece += "/";
                }
                addString(piece.data());
            }
        }
        if (!has_more) {
            return;
        }
        buffer.erase(0, piece_end + 1);
    }
}
