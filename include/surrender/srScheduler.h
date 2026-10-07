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
        long thread_handle;
        srScheduler* scheduler;
    };

    struct QueueEntry {
        enum State { QUEUED = 0, EXECUTING = 1, FINISHED = 2 };

        Job* job;
        QueueEntry* next;
        QueueEntry* previous;
        State state;
    };

    static_assert(sizeof(WorkerSlot) == 0x08, "srScheduler_WorkerSlot_must_be_0x08");
    static_assert(sizeof(QueueEntry) == 0x10, "srScheduler_QueueEntry_must_be_0x10");
    static_assert(sizeof(srHashTable<Job*, QueueEntry*>) == 0x10,
                  "srScheduler_lookup_table_must_be_0x10");
    /* starts a worker thread on the first idle slot when queued
       jobs outnumber busy workers. */
    void wakeWorker();
    /* Expanded in finish, removeQueueEntry and executeNextJob. Hash
       removal, locking, job count and deletion belong to those callers. */
    void unlinkQueueEntry(QueueEntry* entry)
    {
        if (entry->previous == 0) {
            first_job = entry->next;
        } else {
            entry->previous->next = entry->next;
        }
        if (entry->next == 0) {
            last_job = entry->previous;
        } else {
            entry->next->previous = entry->previous;
        }
    }

    void removeQueueEntry(QueueEntry* entry);
    void waitForJob(Job* job);
    /* pops the head job, executes it and retires its entry;
       returns 0 when the queue is empty. */
    long executeNextJob();
    /* worker thread entry; drains the queue then clears the
       slot's handle. Takes the WorkerSlot as the raw void* thread argument. */
    static void __cdecl workerEntry(void* argument);

    WorkerSlot workers[4];
    srHashTable<Job*, QueueEntry*> lookup;
    QueueEntry* first_job;
    QueueEntry* last_job;
    long job_count;
    long worker_count;
    srCriticalSection* critical_section;
};

static_assert(sizeof(srScheduler) == 0x44, "srScheduler_must_be_0x44");
