#pragma once

#include <windows.h>

#include "srHeap.h"

class srScheduler {
public:
    class Job;

    SR_DLL_IMPORT srScheduler();
    SR_DLL_IMPORT ~srScheduler();

    SR_DLL_IMPORT void queue(Job& job);
    SR_DLL_IMPORT void cancel(Job& job);
    SR_DLL_IMPORT void cancelAll();
    SR_DLL_IMPORT void finish(Job& job);
    SR_DLL_IMPORT void finishAll();
    SR_DLL_IMPORT long getJobCount() const;

private:
    struct WorkerSlot {
        long state;
        srScheduler* scheduler;
    };

    struct LookupEntry;
    struct QueueEntry;

    WorkerSlot workers_00[4];
    long* lookup_buckets_20;
    LookupEntry* lookup_entries_24;
    long free_lookup_entry_28;
    long lookup_capacity_2c;
    QueueEntry* first_job_30;
    QueueEntry* last_job_34;
    long job_count_38;
    long worker_count_3c;
    CRITICAL_SECTION* critical_section_40;
};

static_assert(sizeof(srScheduler) == 0x44, "srScheduler_must_be_0x44");
