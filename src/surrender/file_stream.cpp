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

#include <ostream>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FUNCTION: SURRENDER 0x1002E010
srFileManager::Path::Path(const char* name)
{
    next = 0;
    previous = 0;
    if (name != 0 && *name != '\0') {
        name0 = new char[strlen(name) + 1];
        strcpy(name0, name);
    } else {
        name0 = 0;
    }
}

// FUNCTION: SURRENDER 0x1002E080
srFileManager::Path::~Path()
{
    if (name0 != 0) {
        delete[] name0;
    }
}

// FUNCTION: SURRENDER 0x1002E090
const char* srFileManager::Path::getName() const
{
    return name0;
}

// FUNCTION: SURRENDER 0x1002E0A0
srFileManager::Path* srFileManager::Path::getNext() const
{
    return next;
}

// FUNCTION: SURRENDER 0x1002E0B0
void srFileManager::addPath(const char* path)
{
    if (path != 0 && *path != '\0' && strlen(path) < 0x103) {
        char local_path[0x104];
        strcpy(local_path, path);
        int length = strlen(local_path);
        if (local_path[length - 1] != '/') {
            strcat(local_path, "/");
        }
        for (int index = 0; index < length; ++index) {
            if (local_path[index] == '\\') {
                local_path[index] = '/';
            }
        }
        for (Path* node = first_path; node != 0; node = node->next) {
            if (strcmp(local_path, node->getName()) == 0) {
                return;
            }
        }
        Path* new_node = new Path(local_path);
        new_node->next = first_path;
        new_node->previous = 0;
        if (first_path != 0) {
            first_path->previous = new_node;
        }
        first_path = new_node;
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
        Path* node = first_path;
        while (node != 0) {
            if (strcmp(local_path, node->getName()) == 0) {
                if (node->next != 0) {
                    node->next->previous = node->previous;
                }
                if (node->previous != 0) {
                    node->previous->next = node->next;
                }
                if (node == first_path) {
                    first_path = node->next;
                }
                delete node;
                return;
            }
            node = node->next;
        }
    }
}

// FUNCTION: SURRENDER 0x1002E390
void srFileManager::setPath(const char* path)
{
    while (first_path != 0) {
        Path* node = first_path;
        Path* next = node->next;
        delete node;
        first_path = next;
    }
    if (path != 0) {
        addPath(path);
    }
}

// FUNCTION: SURRENDER 0x1002E3E0
w8_long srFileManager::getSize(const char* path)
{
    w8_long size = -1;
    srBinIFStream stream(path);
    if (stream.good()) {
        size = stream.getSize();
    }
    return size;
}

// FUNCTION: SURRENDER 0x1002E490
void srFileManager::load(const char* path, void* destination, w8_ulong size)
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
        w8_ulong size = stream.getSize();
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
void srFileManager::save(const char* path, void* source, w8_ulong size)
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
    return first_path;
}

// FUNCTION: SURRENDER 0x1002E700
void srFileManager::dump(std::ostream& stream)
{
    for (Path* path = first_path; path != 0; path = path->getNext()) {
        stream << path->getName() << '\n';
    }
}

// FUNCTION: SURRENDER 0x1002E740
srFileManager::srFileManager()
{
    first_path = 0;
}

// FUNCTION: SURRENDER 0x1002E750
srFileManager::~srFileManager()
{
    setPath(0);
}

namespace {

/* The async reader's scheduler payload: the job holds the opened input stream, the destination
   buffer and the completion status the stream polls. */
class ReadJob : public srScheduler::Job {
public:
    // FUNCTION: SURRENDER 0x1002EBD0
    ReadJob(srBinIStream* stream, void* buffer, w8_ulong size)
    {
        this->stream = stream;
        this->buffer = buffer;
        this->size = size;
        critical_section = new srCriticalSection;
        status = 0;
    }

    // FUNCTION: SURRENDER 0x1002EC70
    virtual ~ReadJob() override
    {
        delete critical_section;
    }

    // FUNCTION: SURRENDER 0x1002ECD0
    virtual void execute() override
    {
        critical_section->getAccess();
        if (buffer != 0 && size != 0) {
            if (stream->good()) {
                stream->read(buffer, size);
            }
        }
        status = 2 - stream->good();
        critical_section->releaseAccess();
    }

    // FUNCTION: SURRENDER 0x1002ED70
    virtual void cancel() override
    {
        srCriticalSection* lock = critical_section;
        lock->getAccess();
        status = 2;
        lock->releaseAccess();
    }

    w8_ulong getStatus();

private:
    srBinIStream* stream;
    void* buffer;
    w8_ulong size;
    w8_ulong status;
    srCriticalSection* critical_section;
};

// FUNCTION: SURRENDER 0x1002ECB0
w8_ulong ReadJob::getStatus()
{
    srCriticalSection* lock = critical_section;
    lock->getAccess();
    w8_ulong status = this->status;
    lock->releaseAccess();
    return status;
}

} // namespace

// FUNCTION: SURRENDER 0x1002E7E0
srBinIAsyncStream::srBinIAsyncStream(const char* path)
{
    position = 0;
    size = 0;
    job = 0;
    buffer = 0;
    stream = 0;
    finished = 0;
    if (path != 0 && *path != '\0') {
        stream = srCore.getIStreamOpener()->open(path);
    }
    e_state state = SR_STREAM_ERROR;
    if (stream != 0 && stream->good()) {
        /* Neither allocation result is checked. */
        size = stream->getSize();
        buffer = static_cast<unsigned char*>(srHeap.allocate(size));
        job = new ReadJob(stream, buffer, size);
        srCore.getScheduler()->queue(*job);
        state = SR_STREAM_OK;
    }
    setState(state);
}

// FUNCTION: SURRENDER 0x1002E960
srBinIAsyncStream::~srBinIAsyncStream()
{
    if (job != 0) {
        srCore.getScheduler()->cancel(*job);
        delete job;
    }
    if (stream != 0) {
        delete stream;
    }
    srHeap.free(buffer);
}

// FUNCTION: SURRENDER 0x1002EA00
srBinStream& srBinIAsyncStream::seek(w8_ulong position, e_seekDir direction)
{
    if (!good()) {
        return *this;
    }
    w8_ulong new_position = position;
    if (direction != SR_SEEK_BEGIN) {
        if (direction == SR_SEEK_CURRENT) {
            new_position = this->position + position;
        } else {
            new_position = 0;
            if (direction == SR_SEEK_END) {
                new_position = size - position;
            }
        }
    }
    seek(new_position);
    return *this;
}

// FUNCTION: SURRENDER 0x1002EA90
srBinStream& srBinIAsyncStream::seek(w8_ulong position)
{
    if (position <= size) {
        this->position = position;
    } else {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002EAD0
w8_ulong srBinIAsyncStream::tell()
{
    return position;
}

// FUNCTION: SURRENDER 0x1002EAE0
w8_ulong srBinIAsyncStream::vread(void* destination, w8_ulong size)
{
    if (this->size <= position + size) {
        size = this->size - position;
    }
    if (static_cast<int>(size) < 1) {
        return 0;
    }
    if (finished == 0) {
        finished = 1;
        srCore.getScheduler()->finish(*job);
        int status = static_cast<ReadJob*>(job)->getStatus();
        if (job != 0) {
            delete job;
        }
        if (stream != 0) {
            delete stream;
        }
        job = 0;
        stream = 0;
        if (status != 1) {
            return 0;
        }
    }
    unsigned char* source = buffer + position;
    if (size != 0 && destination != source) {
        srVectorProcessor::memcopy(destination, source, size);
    }
    position += size;
    return size;
}

// FUNCTION: SURRENDER 0x1002EBB0
int srBinIAsyncStream::isFinished()
{
    if (finished != 0) {
        return 1;
    }
    return static_cast<ReadJob*>(job)->getStatus() != 0;
}

// FUNCTION: SURRENDER 0x1002EFB0
int srBinFStream::isOpen()
{
    return file != 0;
}

// FUNCTION: SURRENDER 0x1002EFC0
void srBinFStream::close()
{
    fclose(file);
    file = 0;
    path = 0;
    setState(SR_STREAM_STATE_2);
}

// FUNCTION: SURRENDER 0x1002F010
const char* srBinFStream::getPath() const
{
    return path.data();
}

// FUNCTION: SURRENDER 0x1002F020
void srBinFStream::setPath(const char* path)
{
    this->path = path;
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
        file = _fsopen(path, mode_string, _SH_DENYWR);
        if (file != 0) {
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
            strncpy(full_name, directory, _MAX_DIR);
            strncat(full_name, filename, _MAX_FNAME);
            srFileManager* manager = srCore.getFileManager();
            for (srFileManager::Path* search = manager->getFirstPath(); search != 0;
                 search = search->getNext()) {
                char search_filename[_MAX_FNAME];
                char search_extension[_MAX_EXT];
                srSystem::splitPath(search->getName(), drive, directory, search_filename,
                                    search_extension);
                char candidate[_MAX_PATH];
                srSystem::makePath(candidate, drive, directory, full_name, extension);
                file = _fsopen(candidate, mode_string, _SH_DENYWR);
                if (file != 0) {
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
    file = 0;
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
srBinStream& srBinFStream::pseek(w8_ulong position, e_seekDir direction)
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
    if (fseek(file, position, whence) != 0) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002F3B0
srBinStream& srBinFStream::pseek(w8_ulong position)
{
    if (fseek(file, position, SEEK_SET) != 0) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1002F400
w8_ulong srBinFStream::ptell()
{
    w8_ulong position = ftell(file);
    if (position == 0xffffffff) {
        setState(SR_STREAM_ERROR);
    }
    return position;
}

// FUNCTION: SURRENDER 0x1002F6B0
srBinIFStream::srBinIFStream() {}

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
    int result = fgetc(file);
    if (result == -1) {
        return 0xffff;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1002F870
w8_ulong srBinIFStream::vread(void* destination, w8_ulong size)
{
    return fread(destination, 1, size, file);
}

// FUNCTION: SURRENDER 0x1002F890
srBinStream& srBinIFStream::seek(w8_ulong position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x1002F8B0
srBinStream& srBinIFStream::seek(w8_ulong position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x1002F8C0
w8_ulong srBinIFStream::tell()
{
    return ptell();
}

// FUNCTION: SURRENDER 0x1002FC40
srBinIOFStream::srBinIOFStream() {}

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
w8_ulong srBinIOFStream::vwrite(const void* source, w8_ulong size)
{
    return fwrite(source, 1, size, file);
}

// FUNCTION: SURRENDER 0x1002FE50
unsigned short srBinIOFStream::vput(char character)
{
    return fputc(character, file) != -1 ? 0 : 0xffff;
}

// FUNCTION: SURRENDER 0x1002FE80
unsigned short srBinIOFStream::vget()
{
    int result = fgetc(file);
    if (result == -1) {
        return 0xffff;
    }
    return result;
}

// FUNCTION: SURRENDER 0x1002FEA0
w8_ulong srBinIOFStream::vread(void* destination, w8_ulong size)
{
    return fread(destination, 1, size, file);
}

// FUNCTION: SURRENDER 0x1002FEC0
srBinStream& srBinIOFStream::seek(w8_ulong position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x1002FEE0
srBinStream& srBinIOFStream::seek(w8_ulong position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x1002FEF0
w8_ulong srBinIOFStream::tell()
{
    return ptell();
}

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
w8_ulong srBinOFStream::vwrite(const void* source, w8_ulong size)
{
    return fwrite(source, 1, size, file);
}

// FUNCTION: SURRENDER 0x10030540
unsigned short srBinOFStream::vput(char character)
{
    return fputc(character, file) != -1 ? 0 : 0xffff;
}

// FUNCTION: SURRENDER 0x10030570
srBinStream& srBinOFStream::seek(w8_ulong position, e_seekDir direction)
{
    return pseek(position, direction);
}

// FUNCTION: SURRENDER 0x10030590
srBinStream& srBinOFStream::seek(w8_ulong position)
{
    return pseek(position);
}

// FUNCTION: SURRENDER 0x100305B0
w8_ulong srBinOFStream::tell()
{
    return ptell();
}
