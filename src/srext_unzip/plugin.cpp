#include <stdlib.h>
#include <string.h>
#include <new>

#include "surrender/srConfig.h"
#include "surrender/srCore.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srPlugin.h"
#include "surrender/srStringTable.h"

extern "C" {
#include "unzip.h"
#include "windll/structs.h"

void WINAPI Wiz_NoPrinting(int disabled);
}

extern "C" int WINAPI srWizUnzipToMemory(
    char* archive,
    char* member,
    LPUSERFUNCTIONS callbacks,
    UzpBuffer* result,
    int case_insensitive);

class srOwnedBinIMStream : public srBinIMStream {
public:
    srOwnedBinIMStream(void* allocation, unsigned long size)
        : srBinIMStream(allocation, size)
    {
        allocation_ = allocation;
    }

    virtual ~srOwnedBinIMStream();

private:
    void* allocation_;
};

typedef char srOwnedBinIMStream_must_be_0x2c[
    (sizeof(srOwnedBinIMStream) == 0x2c) ? 1 : -1];

struct srInlineString {
    srInlineString()
    {
        inline_[0] = '\0';
        data_ = inline_;
        size_ = 1;
    }
    srInlineString(const char* source);
    srInlineString(const srInlineString& source)
    {
        inline_[0] = '\0';
        data_ = inline_;
        size_ = 1;
        if (source.data_ != 0 && *source.data_ != '\0') {
            size_ = strlen(source.data_) + 1;
            data_ = static_cast<char*>(srHeap.allocate(size_));
            strcpy(data_, source.data_);
        }
    }
    srInlineString(const srInlineString& source, long begin, long end);
    inline ~srInlineString();

    srInlineString& operator=(const char* source);
    srInlineString& operator=(const srInlineString& source)
    {
        return operator=(source.data_);
    }

    char* data() { return data_; }
    const char* data() const { return data_; }
    unsigned long size() const { return size_; }

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

typedef char srInlineString_must_be_0x0c[
    (sizeof(srInlineString) == 0x0c) ? 1 : -1];

srInlineString operator+(
    const srInlineString& left,
    const srInlineString& right);

class srZipAdapter;

struct srZipCallbacks : USERFUNCTIONS {
    srZipAdapter* adapter;
};

typedef char srZipCallbacks_must_be_0x30[
    (sizeof(srZipCallbacks) == 0x30) ? 1 : -1];

class srZipAdapter {
public:
    srZipAdapter();
    ~srZipAdapter();

    srBinIStream* openMember(char* archive, char* member);
    void noteCallback(char* destination)
    {
        strcpy(destination, archive_path_.data_);
        ++callback_count_;
    }
    void setArchivePath(const char* path) { archive_path_ = path; }

private:
    srInlineString archive_path_;
    srZipCallbacks* callbacks_;
    unsigned long callback_state_10_;
    unsigned long callback_state_14_;
    unsigned long callback_count_;
    int case_insensitive_;
};

typedef char srZipAdapter_must_be_0x20[
    (sizeof(srZipAdapter) == 0x20) ? 1 : -1];

class srZipOpener : public srIStreamOpener::Opener {
public:
    virtual ~srZipOpener();

    virtual srBinIStream* open(char* path);
    virtual const char* getDescription() const;

private:
    srBinIStream* openArchivePath(srInlineString path);

    srZipAdapter adapter_;
};

typedef char srZipOpener_must_be_0x24[
    (sizeof(srZipOpener) == 0x24) ? 1 : -1];

class srUnzipPlugin : public srPlugin {
public:
    srUnzipPlugin()
    {
        opener_ = new srZipOpener;
        srCore.getIStreamOpener()->addStreamType(opener_, "zip");
    }

    virtual ~srUnzipPlugin();

    virtual const char* getDescription() const;

private:
    srZipOpener* opener_;
};

// FUNCTION: SREXT_UNZIP 0x100106B0
srBinIStream* srZipOpener::open(char* path)
{
    srInlineString requested;
    if (path != 0) {
        requested = path;
    }
    srInlineString extension;
    extension = ".zip";

    if (strstr(requested.data(), extension.data()) != 0) {
        return openArchivePath(requested);
    }

    srStringTable search_paths;
    search_paths.addSeparatedStrings(srConfig.get("ZIP_PATH"), ";", 1);
    for (long index = 0; index < search_paths.count(); ++index) {
        srInlineString prefix;
        char* search_path = search_paths.getString(index);
        if (search_path != 0) {
            prefix = search_path;
        }

        long begin = 0;
        long end = static_cast<long>(prefix.size()) - 1;
        while (begin < end && prefix.data()[begin] == ' ') {
            ++begin;
        }
        if (begin != 0) {
            prefix = begin == end ? srInlineString("") : srInlineString(prefix, begin, end);
        }

        for (end = static_cast<long>(prefix.size()) - 2;
             end >= 0 && prefix.data()[end] == ' ';
             --end) {
        }
        if (end != static_cast<long>(prefix.size()) - 2) {
            prefix = end < 0 ? srInlineString("") : srInlineString(prefix, 0, end + 1);
        }

        srInlineString candidate = prefix + requested;
        srBinIStream* stream = openArchivePath(candidate);
        if (stream != 0) {
            return stream;
        }
    }
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x10010A60
srBinIStream* srZipOpener::openArchivePath(srInlineString path)
{
    srInlineString separator_text;
    separator_text = "@";
    const char* separator = strstr(path.data(), separator_text.data());
    if (separator != 0) {
        const unsigned long prefix_length = static_cast<unsigned long>(separator - path.data());
        srInlineString prefix_text;
        char* prefix = static_cast<char*>(srHeap.allocate(prefix_length + 2));
        strncpy(prefix, path.data(), prefix_length);
        prefix[prefix_length] = '\0';
        prefix_text = prefix;
        srHeap.free(prefix);
        adapter_.setArchivePath(prefix_text.data());
        path.erasePrefix(prefix_length + 1);
    }

    srInlineString extension_text;
    extension_text = ".zip";
    const char* extension = strstr(path.data(), extension_text.data());
    if (extension == 0) {
        return 0;
    }

    const long extension_offset = static_cast<long>(extension - path.data());
    const long member_begin = extension_offset + 5;
    const unsigned long member_length = path.size() - 1 - member_begin;
    srInlineString member;
    char* temporary = static_cast<char*>(srHeap.allocate(member_length + 2));
    strncpy(temporary, path.data() + member_begin, member_length);
    temporary[member_length] = '\0';
    member = temporary;
    srHeap.free(temporary);

    srInlineString archive;
    temporary = static_cast<char*>(srHeap.allocate(extension_offset + 6));
    strncpy(temporary, path.data(), extension_offset + 4);
    temporary[extension_offset + 4] = '\0';
    archive = temporary;
    srHeap.free(temporary);
    return adapter_.openMember(archive.data(), member.data());
}

// FUNCTION: SREXT_UNZIP 0x10010D50
srInlineString::srInlineString(
    const srInlineString& source,
    long begin,
    long end)
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    char* temporary = static_cast<char*>(srHeap.allocate(end - begin + 2));
    strncpy(temporary, source.data_ + begin, end - begin);
    temporary[end - begin] = '\0';

    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (temporary != 0 && *temporary != '\0') {
        size_ = strlen(temporary) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, temporary);
    }
    srHeap.free(temporary);
}

// FUNCTION: SREXT_UNZIP 0x10010E30
srInlineString::srInlineString(const char* source)
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source != 0) {
        inline_[0] = '\0';
        data_ = inline_;
        size_ = 1;
        if (*source != '\0') {
            size_ = strlen(source) + 1;
            data_ = static_cast<char*>(srHeap.allocate(size_));
            strcpy(data_, source);
        }
    }
}

// FUNCTION: SREXT_UNZIP 0x100115C0
static int WINAPI discardPrintOrService(char*, unsigned long)
{
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x100115D0
static int WINAPI discardReplace(char*)
{
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x100115E0
static void WINAPI discardMessage(
    unsigned long, unsigned long, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, char, char*, char*, unsigned long, char)
{
}

// The adapted Info-ZIP password bridge appends the adapter after the four
// upstream callback arguments. The target body copies the current archive
// path and counts the callback so ambiguous multi-member results can be
// rejected.
// FUNCTION: SREXT_UNZIP 0x100115F0
static int WINAPI noteArchive(
    char* destination,
    int,
    const char*,
    const char*,
    srZipAdapter* adapter)
{
    adapter->noteCallback(destination);
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x10010EB0
srZipAdapter::srZipAdapter()
{
    Wiz_NoPrinting(1);
    callbacks_ = static_cast<srZipCallbacks*>(
        ::operator new(sizeof(srZipCallbacks)));
    memset(callbacks_, 0, sizeof(*callbacks_));
    callbacks_->password = reinterpret_cast<DLLPASSWORD*>(noteArchive);
    callbacks_->print = discardPrintOrService;
    callbacks_->sound = 0;
    callbacks_->replace = discardReplace;
    callbacks_->SendApplicationMessage = discardMessage;
    callbacks_->ServCallBk = reinterpret_cast<DLLSERVICE*>(discardPrintOrService);
    callbacks_->adapter = this;
}

// FUNCTION: SREXT_UNZIP 0x10010F60
inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

// FUNCTION: SREXT_UNZIP 0x10010F90
srInlineString& srInlineString::operator=(const char* source)
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

// FUNCTION: SREXT_UNZIP 0x10011020
srZipAdapter::~srZipAdapter()
{
    ::operator delete(callbacks_);
}

// FUNCTION: SREXT_UNZIP 0x10011060
srBinIStream* srZipAdapter::openMember(char* archive, char* member)
{
    UzpBuffer result;

    case_insensitive_ = 0;
    if (srConfig.exists("ZIP_CASE_INSENSITIVE") &&
        srConfig.getBool("ZIP_CASE_INSENSITIVE")) {
        case_insensitive_ = 1;
    }

    callback_count_ = 0;
    srWizUnzipToMemory(
        archive,
        member,
        callbacks_,
        &result,
        case_insensitive_);

    if (callback_count_ < 2) {
        srBinIStream* stream = 0;
        if (result.strlength != 0 && result.strptr != 0) {
            stream = new srOwnedBinIMStream(result.strptr, result.strlength);
        }
        return stream;
    }
    return 0;
}

// FUNCTION: SREXT_UNZIP 0x10011200
const char* srZipOpener::getDescription() const
{
    return "SurRender .zip opener";
}

// FUNCTION: SREXT_UNZIP 0x10011210
const char* srUnzipPlugin::getDescription() const
{
    return "SurRender unzip plug-in";
}

// FUNCTION: SREXT_UNZIP 0x10011240
srZipOpener::~srZipOpener()
{
}

// FUNCTION: SREXT_UNZIP 0x100112A0
srUnzipPlugin::~srUnzipPlugin()
{
    srCore.getIStreamOpener();
    delete opener_;
}

// FUNCTION: SREXT_UNZIP 0x10011350
srOwnedBinIMStream::~srOwnedBinIMStream()
{
    free(allocation_);
}

// FUNCTION: SREXT_UNZIP 0x100113D0
srInlineString operator+(
    const srInlineString& left,
    const srInlineString& right)
{
    srInlineString result(left);
    if (right.data() == 0 || *right.data() == '\0') {
        return result;
    }

    const unsigned long combined_size =
        result.size() + strlen(right.data());
    char* combined = static_cast<char*>(srHeap.allocate(combined_size));
    strcpy(combined, result.data());
    strcpy(combined + result.size() - 1, right.data());
    if (result.data_ != result.inline_) {
        srHeap.free(result.data_);
    }
    result.inline_[0] = '\0';
    result.size_ = combined_size;
    result.data_ = combined;
    return result;
}

// FUNCTION: SREXT_UNZIP 0x10011190
extern "C" srPlugin* __stdcall srInitPlugin()
{
    return new srUnzipPlugin;
}

// FUNCTION: SREXT_UNZIP 0x10011630
extern "C" unsigned long __stdcall srGetLibraryVersion()
{
    return 0x012A0209UL;
}
