#include <stdlib.h>
#include <string.h>
#include <new>

#include "surrender/srConfig.h"
#include "surrender/srCore.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srPlugin.h"

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
        : srBinIMStream(allocation, size), allocation_(allocation) {}

    virtual ~srOwnedBinIMStream()
    {
        free(allocation_);
    }

private:
    void* allocation_;
};

typedef char srOwnedBinIMStream_must_be_0x2c[
    (sizeof(srOwnedBinIMStream) == 0x2c) ? 1 : -1];

struct srInlineString {
    srInlineString()
        : size_(1), data_(inline_) { inline_[0] = '\0'; }

    ~srInlineString()
    {
        if (data_ != inline_) {
            srHeap.free(data_);
        }
        inline_[0] = '\0';
        size_ = 1;
        data_ = inline_;
    }

    char inline_[4];
    unsigned long size_;
    char* data_;
};

typedef char srInlineString_must_be_0x0c[
    (sizeof(srInlineString) == 0x0c) ? 1 : -1];

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
    void noteCallback(char* destination);

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
    virtual ~srZipOpener() {}

    virtual srBinIStream* open(char* path);
    virtual const char* getDescription() const;

private:
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

    virtual ~srUnzipPlugin()
    {
        srCore.getIStreamOpener();
        delete opener_;
    }

    virtual const char* getDescription() const;

private:
    srZipOpener* opener_;
};

// The complete path parser at 0x100106B0 is intentionally left for the next
// source-recovery step. This virtual definition establishes the proven opener
// vtable while keeping the owned memory-extraction path independently
// comparable.
srBinIStream* srZipOpener::open(char*)
{
    return 0;
}

static int WINAPI discardPrint(char*, unsigned long)
{
    return 0;
}

static int WINAPI discardReplace(char*)
{
    return 0;
}

static int WINAPI discardService(const char*, unsigned long)
{
    return 0;
}

static void WINAPI discardMessage(
    unsigned long, unsigned long, unsigned, unsigned, unsigned, unsigned,
    unsigned, unsigned, char, char*, char*, unsigned long, char)
{
}

// The adapted Info-ZIP password bridge passes the adapter as its third
// argument. The target body copies the current archive path and counts the
// callback so ambiguous multi-member results can be rejected.
static int WINAPI noteArchive(
    char* destination,
    int,
    srZipAdapter* adapter,
    const char*,
    const char*)
{
    adapter->noteCallback(destination);
    return 0;
}

srZipAdapter::srZipAdapter()
{
    Wiz_NoPrinting(1);
    callbacks_ = static_cast<srZipCallbacks*>(
        ::operator new(sizeof(srZipCallbacks)));
    memset(callbacks_, 0, sizeof(*callbacks_));
    callbacks_->print = discardPrint;
    callbacks_->replace = discardReplace;
    callbacks_->password = reinterpret_cast<DLLPASSWORD*>(noteArchive);
    callbacks_->SendApplicationMessage = discardMessage;
    callbacks_->ServCallBk = discardService;
    callbacks_->adapter = this;
}

srZipAdapter::~srZipAdapter()
{
    ::operator delete(callbacks_);
}

void srZipAdapter::noteCallback(char* destination)
{
    strcpy(destination, archive_path_.data_);
    ++callback_count_;
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
