#include "surrender/srBinFStream.h"
#include "surrender/srBinIAsyncStream.h"
#include "surrender/srBinOStream.h"
#include "surrender/srCore.h"
#include "surrender/srCriticalSection.h"
#include "surrender/srFileManager.h"
#include "surrender/srHeap.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srMemoryAllocator.h"
#include "surrender/srScheduler.h"
#include "surrender/srSystem.h"
#include "surrender/srVectorProcessor.h"

#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* TU-local srInlineString expansions: this unit inlines the constructor, copy
   assignment, destructor, find and erase while init, reset and operator+ stay
   callable emissions owned by sibling units (init at 0x10004150, reset at
   0x10012C80, operator+ at 0x10012CB0). The const char* assignment is inline
   here too - setPath expands it while insert keeps a call to the sibling
   unit's callable emission at 0x100040D0. */

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

inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source);
    }
}

inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
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

// FUNCTION: SURRENDER 0x1002E010
srFileManager::Path::Path(const char* name)
{
    next_04 = 0;
    previous_08 = 0;
    if (name != 0 && *name != '\0') {
        name_00 = new char[strlen(name) + 1];
        strcpy(name_00, name);
    } else {
        name_00 = 0;
    }
}

// FUNCTION: SURRENDER 0x1002E080
srFileManager::Path::~Path()
{
    if (name_00 != 0) {
        delete[] name_00;
    }
}

// FUNCTION: SURRENDER 0x1002E090
const char* srFileManager::Path::getName() const
{
    return name_00;
}

// FUNCTION: SURRENDER 0x1002E0A0
srFileManager::Path* srFileManager::Path::getNext() const
{
    return next_04;
}

/* Retail 0x100163A0 copies name_00, next_04 and previous_08 memberwise.
   No Path copy constructor is emitted. */
// FUNCTION: SURRENDER 0x100163A0
srFileManager::Path& srFileManager::Path::operator=(const Path& other)
{
    name_00 = other.name_00;
    next_04 = other.next_04;
    previous_08 = other.previous_08;
    return *this;
}

// FUNCTION: SURRENDER 0x1002E0B0
void srFileManager::addPath(const char* path)
{
    if (path != 0 && *path != '\0' && strlen(path) < 0x103) {
        char local_path[0x104];
        strcpy(local_path, path);
        int length = strlen(local_path) - 1;
        if (local_path[length] != '/') {
            strcat(local_path, "/");
        }
        for (int index = 0; index < length; ++index) {
            if (local_path[index] == '\\') {
                local_path[index] = '/';
            }
        }
        for (Path* node = first_path_04; node != 0; node = node->next_04) {
            if (strcmp(local_path, node->getName()) == 0) {
                return;
            }
        }
        Path* new_node = new Path(local_path);
        new_node->next_04 = first_path_04;
        new_node->previous_08 = 0;
        if (first_path_04 != 0) {
            first_path_04->previous_08 = new_node;
        }
        first_path_04 = new_node;
    }
}

// FUNCTION: SURRENDER 0x1002E240
void srFileManager::removePath(const char* path)
{
    if (path != 0 && *path != '\0' && strlen(path) < 0x103) {
        char local_path[0x104];
        strcpy(local_path, path);
        if (local_path[strlen(local_path) - 1] != '/') {
            strcat(local_path, "/");
        }
        Path* node = first_path_04;
        while (node != 0) {
            if (strcmp(local_path, node->getName()) == 0) {
                if (node->next_04 != 0) {
                    node->next_04->previous_08 = node->previous_08;
                }
                if (node->previous_08 != 0) {
                    node->previous_08->next_04 = node->next_04;
                }
                if (node == first_path_04) {
                    first_path_04 = node->next_04;
                }
                delete node;
                return;
            }
            node = node->next_04;
        }
    }
}

// FUNCTION: SURRENDER 0x1002E390
void srFileManager::setPath(const char* path)
{
    while (first_path_04 != 0) {
        Path* node = first_path_04;
        Path* next = node->next_04;
        delete node;
        first_path_04 = next;
    }
    if (path != 0) {
        addPath(path);
    }
}

// FUNCTION: SURRENDER 0x1002E3E0
long srFileManager::getSize(const char* path)
{
    long size = -1;
    srBinIFStream stream(path);
    if (stream.good()) {
        size = stream.getSize();
    }
    return size;
}

// FUNCTION: SURRENDER 0x1002E490
void srFileManager::load(const char* path, void* destination, unsigned long size)
{
    if (destination != 0 && size != 0) {
        srBinIFStream stream;
        stream.exceptions(true);
        stream.open(path);
        stream.read(destination, size);
    }
}

// FUNCTION: SURRENDER 0x1002E520
void* srFileManager::allocate(const char* path)
{
    if (path != 0 && *path != '\0') {
        srBinIFStream stream;
        stream.exceptions(true);
        stream.open(path);
        unsigned long size = stream.getSize();
        void* allocation = srCore.getMemoryAllocator()->allocate(size, path);
        if (allocation != 0) {
            stream.read(allocation, size);
        }
        return allocation;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1002E630
void srFileManager::free(void* allocation)
{
    if (allocation != 0) {
        srCore.getMemoryAllocator()->free(allocation);
    }
}

// FUNCTION: SURRENDER 0x1002E650
void srFileManager::save(const char* path, void* source, unsigned long size)
{
    if (source != 0 && size != 0 && path != 0 && *path != '\0') {
        srBinOFStream stream;
        stream.exceptions(true);
        stream.open(path);
        stream.write(source, size);
    }
}

// FUNCTION: SURRENDER 0x1002E6F0
srFileManager::Path* srFileManager::getFirstPath() const
{
    return first_path_04;
}

// FUNCTION: SURRENDER 0x1002E700
void srFileManager::dump(std::ostream& stream)
{
    for (Path* path = first_path_04; path != 0; path = path->getNext()) {
        stream << path->getName() << '\n';
    }
}

// FUNCTION: SURRENDER 0x1002E740
srFileManager::srFileManager()
{
    first_path_04 = 0;
}

// FUNCTION: SURRENDER 0x100163C0
srFileManager::srFileManager(const srFileManager& other) : first_path_04(other.first_path_04)
{
}

// FUNCTION: SURRENDER 0x1002E750
srFileManager::~srFileManager()
{
    setPath(0);
}

// FUNCTION: SURRENDER 0x100163E0
srFileManager& srFileManager::operator=(const srFileManager& other)
{
    first_path_04 = other.first_path_04;
    return *this;
}

namespace {

/* The async reader's scheduler payload: the job holds the opened input
   stream, the destination buffer and the completion status the stream polls.
   Retail emits the whole family file-local at 0x1002EBD0-0x1002ED70. */
class ReadJob : public srScheduler::Job {
public:
    ReadJob(srBinIStream* stream, void* buffer, unsigned long size)
    {
        stream_04 = stream;
        buffer_08 = buffer;
        size_0c = size;
        critical_section_14 = new srCriticalSection;
        status_10 = 0;
    }

    // FUNCTION: SURRENDER 0x1002EC70
    virtual ~ReadJob() override
    {
        if (critical_section_14 != 0) {
            critical_section_14->getAccess();
            critical_section_14->releaseAccess();
            delete critical_section_14;
        }
    }

    // FUNCTION: SURRENDER 0x1002ECD0
    virtual void execute() override
    {
        critical_section_14->getAccess();
        if (buffer_08 != 0 && size_0c != 0) {
            if (stream_04->good()) {
                stream_04->read(buffer_08, size_0c);
            }
        }
        status_10 = 2 - stream_04->good();
        critical_section_14->releaseAccess();
    }

    // FUNCTION: SURRENDER 0x1002ED70
    virtual void cancel() override
    {
        srCriticalSection* lock = critical_section_14;
        lock->getAccess();
        status_10 = 2;
        lock->releaseAccess();
    }

    unsigned long getStatus();

private:
    srBinIStream* stream_04;
    void* buffer_08;
    unsigned long size_0c;
    unsigned long status_10;
    srCriticalSection* critical_section_14;
};

// FUNCTION: SURRENDER 0x1002ECB0
unsigned long ReadJob::getStatus()
{
    srCriticalSection* lock = critical_section_14;
    lock->getAccess();
    unsigned long status = status_10;
    lock->releaseAccess();
    return status;
}

} // namespace

// FUNCTION: SURRENDER 0x1002E7E0
srBinIAsyncStream::srBinIAsyncStream(const char* path)
{
    position_14 = 0;
    size_18 = 0;
    job_0c = 0;
    buffer_08 = 0;
    stream_10 = 0;
    finished_1c = 0;
    if (path != 0 && *path != '\0') {
        stream_10 = srCore.getIStreamOpener()->open(path);
    }
    e_state state = SR_STREAM_ERROR;
    if (stream_10 != 0 && stream_10->good()) {
        /* Retail does not branch on either allocation result before queueing
           job_0c and setting SR_STREAM_OK. */
        size_18 = stream_10->getSize();
        buffer_08 = static_cast<unsigned char*>(srHeap.allocate(size_18));
        job_0c = new ReadJob(stream_10, buffer_08, size_18);
        srCore.getScheduler()->queue(*job_0c);
        state = SR_STREAM_OK;
    }
    setState(state);
}

// FUNCTION: SURRENDER 0x1002E960
srBinIAsyncStream::~srBinIAsyncStream()
{
    if (job_0c != 0) {
        srCore.getScheduler()->cancel(*job_0c);
        delete job_0c;
    }
    if (stream_10 != 0) {
        delete stream_10;
    }
    srHeap.free(buffer_08);
}

// FUNCTION: SURRENDER 0x1002EA00
srBinStream& srBinIAsyncStream::seek(unsigned long position, e_seekDir direction)
{
    if (!good()) {
        return *this;
    }
    unsigned long new_position = position;
    if (direction != SR_SEEK_BEGIN) {
        if (direction == SR_SEEK_CURRENT) {
            new_position = position_14 + position;
        } else {
            new_position = 0;
            if (direction == SR_SEEK_END) {
                new_position = size_18 - position;
            }
        }
    }
    seek(new_position);
    return *this;
}

// FUNCTION: SURRENDER 0x1002EA90
srBinStream& srBinIAsyncStream::seek(unsigned long position)
{
    if (position <= size_18) {
        position_14 = position;
    } else {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002EAD0
unsigned long srBinIAsyncStream::tell()
{
    return position_14;
}

// FUNCTION: SURRENDER 0x1002EAE0
unsigned long srBinIAsyncStream::vread(void* destination, unsigned long size)
{
    if (size_18 <= position_14 + size) {
        size = size_18 - position_14;
    }
    if (static_cast<int>(size) < 1) {
        return 0;
    }
    if (finished_1c == 0) {
        finished_1c = 1;
        srCore.getScheduler()->finish(*job_0c);
        int status = static_cast<ReadJob*>(job_0c)->getStatus();
        if (job_0c != 0) {
            delete job_0c;
        }
        if (stream_10 != 0) {
            delete stream_10;
        }
        job_0c = 0;
        stream_10 = 0;
        if (status != 1) {
            return 0;
        }
    }
    unsigned char* source = buffer_08 + position_14;
    if (size != 0 && destination != source) {
        srVectorProcessor::memcopy(destination, source, size);
    }
    position_14 += size;
    return size;
}

// FUNCTION: SURRENDER 0x1002EBB0
int srBinIAsyncStream::isFinished()
{
    if (finished_1c != 0) {
        return 1;
    }
    return static_cast<ReadJob*>(job_0c)->getStatus() != 0;
}

// FUNCTION: SURRENDER 0x1002EFB0
int srBinFStream::isOpen()
{
    return file_08 != 0;
}

// FUNCTION: SURRENDER 0x1002EFC0
void srBinFStream::close()
{
    fclose(file_08);
    file_08 = 0;
    /* Retail inlines the empty-state sequence: release non-inline storage,
       zero inline_, repoint data_, size 1. reset() itself stays out of line
       in this unit (0x10012C80), so the authored spelling here is the
       null-string assignment, which folds to that sequence; the stream
       stays usable afterwards, so it is not member teardown. */
    path_0c = 0;
    setState(SR_STREAM_STATE_2);
}

// FUNCTION: SURRENDER 0x1002F010
const char* srBinFStream::getPath() const
{
    return path_0c.data();
}

// FUNCTION: SURRENDER 0x1002F020
void srBinFStream::setPath(const char* path)
{
    path_0c = path;
}

// FUNCTION: SURRENDER 0x1002F0A0
void srBinFStream::mopen(const char* path, e_mode mode, int search_paths)
{
    if (!isOpen()) {
        char mode_string[4];
        int length;
        switch (mode) {
        case SR_MODE_READ:
            mode_string[0] = 'r';
            length = 1;
            break;
        case SR_MODE_WRITE:
            mode_string[0] = 'w';
            length = 1;
            break;
        case SR_MODE_READ_WRITE:
            mode_string[0] = 'r';
            mode_string[1] = '+';
            length = 2;
            break;
        default:
            length = 0;
            break;
        }
        mode_string[length] = 'b';
        mode_string[length + 1] = '\0';
        file_08 = _fsopen(path, mode_string, _SH_DENYWR);
        if (file_08 != 0) {
            setState(SR_STREAM_OK);
            return;
        }
        if (search_paths != 0) {
            char drive[_MAX_DRIVE];
            char directory[_MAX_DIR];
            char filename[_MAX_FNAME];
            char extension[_MAX_EXT];
            srSystem::splitPath(path, drive, directory, filename, extension);
            char full_name[_MAX_PATH];
            strncpy(full_name, directory, _MAX_PATH);
            strncat(full_name, filename, _MAX_PATH);
            srFileManager* manager = srCore.getFileManager();
            for (srFileManager::Path* search = manager->getFirstPath(); search != 0;
                 search = search->getNext()) {
                char search_filename[_MAX_FNAME];
                char search_extension[_MAX_EXT];
                srSystem::splitPath(search->getName(), drive, directory, search_filename,
                                    search_extension);
                char candidate[_MAX_PATH];
                srSystem::makePath(candidate, drive, directory, full_name, extension);
                file_08 = _fsopen(candidate, mode_string, _SH_DENYWR);
                if (file_08 != 0) {
                    setPath(candidate);
                    setState(SR_STREAM_OK);
                    return;
                }
            }
        }
    }
    setState(SR_STREAM_ERROR);
}

// FUNCTION: SURRENDER 0x1002F250
srBinFStream::srBinFStream()
{
    file_08 = 0;
    setState(SR_STREAM_STATE_2);
}

// FUNCTION: SURRENDER 0x1002F2B0
srBinFStream::~srBinFStream()
{
    if (isOpen()) {
        close();
    }
}

// FUNCTION: SURRENDER 0x1002F340
srBinStream& srBinFStream::pseek(unsigned long position, e_seekDir direction)
{
    int whence;
    switch (direction) {
    case SR_SEEK_BEGIN:
        whence = SEEK_SET;
        break;
    case SR_SEEK_CURRENT:
        whence = SEEK_CUR;
        break;
    case SR_SEEK_END:
        whence = SEEK_END;
        break;
    default:
        return *this;
    }
    if (fseek(file_08, position, whence) != 0) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002F3B0
srBinStream& srBinFStream::pseek(unsigned long position)
{
    if (fseek(file_08, position, SEEK_SET) != 0) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002F400
unsigned long srBinFStream::ptell()
{
    unsigned long position = ftell(file_08);
    if (position == 0xffffffff) {
        setState(SR_STREAM_ERROR);
    }
    return position;
}

// FUNCTION: SURRENDER 0x1002F6B0
srBinIFStream::srBinIFStream() {}

srBinIFStream::~srBinIFStream() {}

// FUNCTION: SURRENDER 0x1002F760
srBinIFStream::srBinIFStream(const char* path)
{
    open(path);
}

// FUNCTION: SURRENDER 0x1002F830
void srBinIFStream::open(const char* path)
{
    mopen(path, SR_MODE_READ, 1);
}

// FUNCTION: SURRENDER 0x1002F850
unsigned short srBinIFStream::vget()
{
    int result = fgetc(file_08);
    if (result == -1) {
        return 0xffff;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1002F870
unsigned long srBinIFStream::vread(void* destination, unsigned long size)
{
    return fread(destination, 1, size, file_08);
}

// FUNCTION: SURRENDER 0x1002F890
srBinStream& srBinIFStream::seek(unsigned long position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x1002F8B0
srBinStream& srBinIFStream::seek(unsigned long position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x1002F8C0
unsigned long srBinIFStream::tell()
{
    return ptell();
}

// FUNCTION: SURRENDER 0x1002FC40
srBinIOFStream::srBinIOFStream() {}

srBinIOFStream::~srBinIOFStream() {}

// FUNCTION: SURRENDER 0x1002FD20
srBinIOFStream::srBinIOFStream(const char* path)
{
    open(path);
}

// FUNCTION: SURRENDER 0x1002FE10
void srBinIOFStream::open(const char* path)
{
    mopen(path, SR_MODE_READ_WRITE, 0);
}

// FUNCTION: SURRENDER 0x1002FE30
unsigned long srBinIOFStream::vwrite(const void* source, unsigned long size)
{
    return fwrite(source, 1, size, file_08);
}

// FUNCTION: SURRENDER 0x1002FE50
unsigned short srBinIOFStream::vput(char character)
{
    return fputc(character, file_08) != -1 ? 0 : 0xffff;
}

// FUNCTION: SURRENDER 0x1002FE80
unsigned short srBinIOFStream::vget()
{
    int result = fgetc(file_08);
    if (result == -1) {
        return 0xffff;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1002FEA0
unsigned long srBinIOFStream::vread(void* destination, unsigned long size)
{
    return fread(destination, 1, size, file_08);
}

// FUNCTION: SURRENDER 0x1002FEC0
srBinStream& srBinIOFStream::seek(unsigned long position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x1002FEE0
srBinStream& srBinIOFStream::seek(unsigned long position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x1002FEF0
unsigned long srBinIOFStream::tell()
{
    return ptell();
}

// FUNCTION: SURRENDER 0x100302B0
srBinOStream::~srBinOStream() {}

// FUNCTION: SURRENDER 0x10030330
srBinOFStream::srBinOFStream(const char* path)
{
    open(path);
}

// FUNCTION: SURRENDER 0x10030420
srBinOFStream::srBinOFStream() {}

// FUNCTION: SURRENDER 0x100304F0
void srBinOFStream::open(const char* path)
{
    mopen(path, SR_MODE_WRITE, 0);
}

// FUNCTION: SURRENDER 0x10030510
unsigned long srBinOFStream::vwrite(const void* source, unsigned long size)
{
    return fwrite(source, 1, size, file_08);
}

// FUNCTION: SURRENDER 0x10030540
unsigned short srBinOFStream::vput(char character)
{
    return fputc(character, file_08) != -1 ? 0 : 0xffff;
}

// FUNCTION: SURRENDER 0x10030570
srBinStream& srBinOFStream::seek(unsigned long position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x10030590
srBinStream& srBinOFStream::seek(unsigned long position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x100305B0
unsigned long srBinOFStream::tell()
{
    return ptell();
}

// FUNCTION: SURRENDER 0x100308C0
srBinOFStream::~srBinOFStream() {}

/* srFileManager's implicit deleting destructor is emitted in this unit with
   the Path members. */
// SYNTHETIC: SURRENDER 0x100163F0
// srFileManager scalar deleting destructor

// SYNTHETIC: SURRENDER 0X1002E770
// std::ios_base::Init global static-init block

// SYNTHETIC: SURRENDER 0X1002E780
// std::ios_base::Init global atexit registrar

// SYNTHETIC: SURRENDER 0X1002E7B0
// std::_Winit global static-init block

// SYNTHETIC: SURRENDER 0X1002E7C0
// std::_Winit global atexit registrar

// SYNTHETIC: SURRENDER 0X1002EC50
// ReadJob scalar deleting destructor

// SYNTHETIC: SURRENDER 0X1002ED90
// base subobject destructor emission (vtable restore)
