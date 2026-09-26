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

/* Retail copy construction assigns strings_00 through srArray::operator=;
   the element pointers are copied shallowly. */
// FUNCTION: SURRENDER 0x10003AC0
srStringTable::srStringTable(const srStringTable& other)
{
    strings_00 = other.strings_00;
    count_08 = other.count_08;
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

/* TU-local srInlineString expansions, the same convention as file_stream:
   this unit inlines construction, the destructor, assignment, find, erase
   and operator+= while init, reset and operator+ stay callable emissions
   owned by sibling units (init at 0x10004150, reset at 0x10012C80,
   operator+ at 0x10012CB0). addSeparatedStrings' operator+= tail is the
   caller this unit keeps for the init emission, the destructor copy its
   EH unwind funclets call sits at 0x100040A0, and the const char*
   assignment it emits for the buffer/needle/piece copies sits at
   0x100040D0. */

// FUNCTION: SURRENDER 0x10004150 SYMBOL
// ?init@srInlineString@@QAEXXZ

inline srInlineString::srInlineString()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

inline srInlineString::srInlineString(const char* source)
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source != 0) {
        operator=(source);
    }
}

// FUNCTION: SURRENDER 0x100040A0 SYMBOL
// ??1srInlineString@@QAEXXZ
inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

// FUNCTION: SURRENDER 0x100040D0 SYMBOL
// ??4srInlineString@@QAEAAU0@PBD@Z
inline srInlineString& srInlineString::operator=(const char* source)
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source == 0 || *source == '\0') {
        return *this;
    }
    size_ = strlen(source) + 1;
    data_ = static_cast<char*>(srHeap.allocate(size_));
    strcpy(data_, source);
    return *this;
}

inline long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}

inline void srInlineString::erase(unsigned long begin, unsigned long end)
{
    if (begin != end) {
        strncpy(data_ + begin, data_ + end, size_ - end);
        size_ = strlen(data_) + 1;
    }
}

inline srInlineString& srInlineString::operator+=(const char* suffix)
{
    if (suffix != 0 && *suffix != '\0') {
        const unsigned long needed = strlen(suffix) + size_;
        char* buffer = static_cast<char*>(srHeap.allocate(needed));
        strcpy(buffer, data_);
        strcpy(buffer + size_ - 1, suffix);
        if (data_ != inline_) {
            srHeap.free(data_);
        }
        init();
        size_ = needed;
        data_ = buffer;
    }
    return *this;
}

// TEMPLATE: SURRENDER 0x10004080
// srArray<char*>::release
