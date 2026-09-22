#pragma once

#include <iostream>

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
    SR_DLL_IMPORT long getLong(const char* name) const;
    SR_DLL_IMPORT void remove(const char* name);
    SR_DLL_IMPORT void removeAll();
    SR_DLL_IMPORT void set(const char* name, const char* value);
    SR_DLL_IMPORT void setBool(const char* name, int value);
    SR_DLL_IMPORT void setFloat(const char* name, float value);
    SR_DLL_IMPORT void setLong(const char* name, long value);

    /* Nested implementation types stay public so out-of-line member
       definitions can name them, matching the ClassNode::NameIndex/IDIndex
       convention in srTypeRegistry.h. */
    struct Entry {
        char* name;
        char* value;
        Entry* previous;
        Entry* next;
    };

    struct Index;

    // Entry storage pool: heap blocks of pooled Entry records threaded through
    // the name pointer while free. The provider keeps it as a member sub-object
    // at +0x04 (its own unwind emission destroys the embedded srArray). The
    // destructor's release path is what removeAll/removeEntry invoke directly.
    struct EntryPool {
        EntryPool() : entry_count(0), free_entries(0), entry_blocks(), entry_block_count(0) {}

        // FUNCTION: SURRENDER 0x10012B40
        ~EntryPool()
        {
            release();
        }

        void release()
        {
            for (unsigned long index = 0; index < entry_block_count; ++index) {
                srHeap.free(entry_blocks[index]);
            }
            /* Retail ~EntryPool (0x10012B40) releases the array storage and
               zeroes the counters; the member's own ~srArray teardown then
               emits a second delete on the nulled fields. The pool stays
               live after release(), so this is release, not member
               teardown. */
            entry_blocks.release();
            free_entries = 0;
            entry_block_count = 0;
            entry_count = 0;
        }

        unsigned long entry_count;
        Entry* free_entries;
        srArray<Entry*> entry_blocks;
        unsigned long entry_block_count;
    };

private:
    Index* getIndex() const;
    void removeEntry(Entry* entry);

    Entry* first_entry_00;
    EntryPool entry_pool_04;
    mutable Index* index_18;
};

static_assert(sizeof(srConfig) == 0x1c, "srConfig_must_be_0x1c");

extern SR_DLL_IMPORT class srConfig srConfig;
