#pragma once

#include <iosfwd>

#include "srArray.h"
#include "srHeap.h"

class srConfig {
public:
    SR_DLL_IMPORT srConfig();
    SR_DLL_IMPORT ~srConfig();

    SR_DLL_IMPORT void append(const char* name, const char* value);
    SR_DLL_IMPORT void dump(std::ostream& stream);
    SR_DLL_IMPORT int exists(const char* name) const;
    SR_DLL_IMPORT const char* get(const char* name) const;
    SR_DLL_IMPORT int getBool(const char* name) const;
    SR_DLL_IMPORT float getFloat(const char* name) const;
    SR_DLL_IMPORT w8_long getLong(const char* name) const;
    SR_DLL_IMPORT void remove(const char* name);
    SR_DLL_IMPORT void removeAll();
    SR_DLL_IMPORT void set(const char* name, const char* value);
    SR_DLL_IMPORT void setBool(const char* name, int value);
    SR_DLL_IMPORT void setFloat(const char* name, float value);
    SR_DLL_IMPORT void setLong(const char* name, w8_long value);

    struct Entry {
        char* name;
        char* value;
        Entry* previous;
        Entry* next;
    };
    W8_ABI_ASSERT(sizeof(Entry) == 0x10, "srConfig_Entry_must_be_0x10");

    struct Index;

    // Entry storage pool: heap blocks of pooled Entry records threaded through the name pointer
    // while free.
    struct EntryPool {
        EntryPool() : entry_count(0), free_entries(0), entry_blocks(), entry_block_count(0) {}

        // FUNCTION: SURRENDER 0x10012B40
        ~EntryPool()
        {
            release();
        }

        Entry* allocate()
        {
            if (free_entries == 0) {
                w8_long count = entry_count < 2 ? 1 : entry_count;
                if (count > 0xff) {
                    count = 0x100;
                }
                Entry* block = static_cast<Entry*>(srHeap.allocate(count * sizeof(Entry)));
                w8_ulong block_index = entry_block_count;
                free_entries = block;
                entry_block_count = block_index + 1;
                entry_blocks[block_index] = block;
                Entry* free_entry = block;
                for (w8_ulong index_ = count; index_ != 0; --index_) {
                    // reinterpret-ok: free pool entries thread the next-free pointer
                    // through the name field.
                    free_entry->name = reinterpret_cast<char*>(free_entry + 1);
                    ++free_entry;
                }
                block[count - 1].name = 0;
            }

            Entry* entry = free_entries;
            // reinterpret-ok: the free list link lives in the name pointer.
            free_entries = reinterpret_cast<Entry*>(entry->name);
            ++entry_count;
            return entry;
        }

        void free(Entry* entry)
        {
            --entry_count;
            // reinterpret-ok: the free list link lives in the name pointer.
            entry->name = reinterpret_cast<char*>(free_entries);
            free_entries = entry;
            if (entry_count == 0) {
                release();
            }
        }

        void release()
        {
            for (w8_ulong index = 0; index < entry_block_count; ++index) {
                srHeap.free(entry_blocks[index]);
            }
            entry_blocks.release();
            free_entries = 0;
            entry_block_count = 0;
            entry_count = 0;
        }

        w8_ulong entry_count;
        Entry* free_entries;
        srArray<Entry*> entry_blocks;
        w8_ulong entry_block_count;
    };

private:
    Index* getIndex() const;
    void removeEntry(Entry* entry);

    Entry* first_entry;
    EntryPool entry_pool;
    mutable Index* index;
};

W8_ABI_ASSERT(sizeof(srConfig) == 0x1c, "srConfig_must_be_0x1c");

extern SR_DLL_IMPORT class srConfig srConfig;
