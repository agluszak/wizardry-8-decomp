#pragma once

#include <windows.h>

#include "srCriticalSection.h"
#include "srHeap.h"
#include "srHash.h"

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

    static_assert(sizeof(WorkerSlot) == 0x08, "srScheduler_WorkerSlot_must_be_0x08");
    static_assert(sizeof(QueueEntry) == 0x10, "srScheduler_QueueEntry_must_be_0x10");
    static_assert(sizeof(srHashTable<Job*, QueueEntry*>) == 0x10,
                  "srScheduler_lookup_table_must_be_0x10");
    /* starts a worker thread on the first idle slot when queued
       jobs outnumber busy workers. */
    void wakeWorker();
    /* removes the entry from the lookup and job queue, deletes
       it and decrements the job count. */
    void removeQueueEntry(QueueEntry* entry);
    /* spins until the job's queue entry clears. */
    void waitForJob(Job* job);
    /* pops the head job, executes it and retires its entry;
       returns 0 when the queue is empty. */
    long executeNextJob();
    /* worker thread entry; drains the queue then clears the
       slot's handle. Takes the WorkerSlot as the raw void* thread argument. */
    static void __cdecl workerEntry(void* argument);

    WorkerSlot workers_00[4];
    srHashTable<Job*, QueueEntry*> lookup_20;
    QueueEntry* first_job_30;
    QueueEntry* last_job_34;
    long job_count_38;
    long worker_count_3c;
    srCriticalSection* critical_section_40;
};

static_assert(sizeof(srScheduler) == 0x44, "srScheduler_must_be_0x44");
