#pragma once

#include <windows.h>

#include "srHeap.h"

class srScheduler {
public:
    class Job {
    public:
        virtual ~Job() {}
        virtual void execute() = 0;
        virtual void cancel() = 0;
    };

    static_assert(sizeof(Job) == 0x04, "srScheduler_Job_must_be_0x04");

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
        long thread_handle_00;
        srScheduler* scheduler_04;
    };

    struct QueueEntry {
        Job* job_00;
        QueueEntry* next_04;
        QueueEntry* previous_08;
        long state_0c;
    };

    struct LookupEntry {
        long next_index_00;
        Job* job_04;
        QueueEntry* queue_entry_08;
    };

    static_assert(sizeof(WorkerSlot) == 0x08,
                  "srScheduler_WorkerSlot_must_be_0x08");
    static_assert(sizeof(QueueEntry) == 0x10,
                  "srScheduler_QueueEntry_must_be_0x10");
    static_assert(sizeof(LookupEntry) == 0x0c,
                  "srScheduler_LookupEntry_must_be_0x0c");

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
