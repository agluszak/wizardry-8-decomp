#include "surrender/srScheduler.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srThread.h"
#include "surrender/srVariableTimer.h"

/* This unit's scheduling loops wait through one out-of-line helper
   (0x100146D0) rather than calling srThread::yield directly. */
static void yieldOneMillisecond();

// FUNCTION: SURRENDER 0x10013D70
srScheduler::srScheduler()
{
    critical_section = new srCriticalSection;
    first_job = 0;
    last_job = 0;
    job_count = 0;
    for (int index = 0; index != 4; ++index) {
        workers[index].thread_handle = -1;
        workers[index].scheduler = this;
    }
    worker_count = srCore.getTimer()->m_cpu_count;
    if (worker_count < 1) {
        worker_count = 1;
    }
    if (4 < worker_count) {
        worker_count = 4;
    }
}

// FUNCTION: SURRENDER 0x10013E30
srScheduler::~srScheduler()
{
    cancelAll();
    for (int index = 0; index < worker_count; ++index) {
        while (workers[index].thread_handle != -1) {
            yieldOneMillisecond();
        }
    }
    if (critical_section != 0) {
        critical_section->getAccess();
        critical_section->releaseAccess();
        delete critical_section;
    }
}

// FUNCTION: SURRENDER 0x10013EE0
void srScheduler::queue(Job& job)
{
    srCriticalSectionAccess access(critical_section);
    QueueEntry* entry = new QueueEntry;
    entry->job_00 = &job;
    entry->next_04 = 0;
    entry->previous = last_job;
    entry->state = 0;
    if (last_job != 0) {
        last_job->next_04 = entry;
    }
    last_job = entry;
    if (first_job == 0) {
        first_job = entry;
    }
    job_count += 1;
    Job* key = &job;
    lookup_20.Insert(&key, &entry);
    wakeWorker();
}

// FUNCTION: SURRENDER 0x10014060
void srScheduler::cancel(Job& job)
{
    critical_section->getAccess();
    Job* key = &job;
    QueueEntry* entry = lookup_20.Lookup(&key);
    if (entry != 0 && entry->state != 2) {
        if (entry->state == 0) {
            entry->job_00->cancel();
            removeQueueEntry(entry);
            critical_section->releaseAccess();
            return;
        }
        critical_section->releaseAccess();
        waitForJob(&job);
        return;
    }
    critical_section->releaseAccess();
}

// FUNCTION: SURRENDER 0x10014110
void srScheduler::finish(Job& job)
{
    critical_section->getAccess();
    Job* key = &job;
    QueueEntry* entry = lookup_20.Lookup(&key);
    if (entry != 0 && entry->state != 2) {
        if (entry->state != 0) {
            critical_section->releaseAccess();
            waitForJob(&job);
            return;
        }
        Job* queued = entry->job_00;
        lookup_20.Remove(&queued, &entry);
        if (entry->previous == 0) {
            first_job = entry->next_04;
        } else {
            entry->previous->next_04 = entry->next_04;
        }
        if (entry->next_04 == 0) {
            last_job = entry->previous;
        } else {
            entry->next_04->previous = entry->previous;
        }
        entry->state = 1;
        critical_section->releaseAccess();
        try {
            queued->execute();
        } catch (...) {
            throw;
        }
        critical_section->getAccess();
        entry->state = 2;
        delete entry;
        job_count -= 1;
        critical_section->releaseAccess();
        return;
    }
    critical_section->releaseAccess();
}

// FUNCTION: SURRENDER 0x10014310
void srScheduler::cancelAll()
{
    critical_section->getAccess();
    QueueEntry* entry = first_job;
    while (entry != 0) {
        entry->job_00->cancel();
        removeQueueEntry(first_job);
        entry = first_job;
    }
    critical_section->releaseAccess();
    finishAll();
}

// FUNCTION: SURRENDER 0x10014360
void srScheduler::finishAll()
{
    while (true) {
        critical_section->getAccess();
        long count = job_count;
        critical_section->releaseAccess();
        if (count == 0) {
            break;
        }
        yieldOneMillisecond();
    }
}

// FUNCTION: SURRENDER 0x100142F0
long srScheduler::getJobCount() const
{
    srCriticalSection* section = critical_section;
    section->getAccess();
    long count = job_count;
    section->releaseAccess();
    return count;
}

// FUNCTION: SURRENDER 0x100143A0
void srScheduler::removeQueueEntry(QueueEntry* entry)
{
    srCriticalSectionAccess access(critical_section);
    Job* job = entry->job_00;
    lookup_20.Remove(&job, &entry);
    if (entry->previous == 0) {
        first_job = entry->next_04;
    } else {
        entry->previous->next_04 = entry->next_04;
    }
    if (entry->next_04 == 0) {
        last_job = entry->previous;
    } else {
        entry->next_04->previous = entry->previous;
    }
    delete entry;
    job_count -= 1;
}

// FUNCTION: SURRENDER 0x100144B0
long srScheduler::executeNextJob()
{
    critical_section->getAccess();
    QueueEntry* entry = first_job;
    if (entry == 0) {
        critical_section->releaseAccess();
        return 0;
    }
    if (entry->previous == 0) {
        first_job = entry->next_04;
    } else {
        entry->previous->next_04 = entry->next_04;
    }
    if (entry->next_04 == 0) {
        last_job = entry->previous;
    } else {
        entry->next_04->previous = entry->previous;
    }
    entry->state = 1;
    critical_section->releaseAccess();
    try {
        entry->job_00->execute();
    } catch (...) {
        throw;
    }
    critical_section->getAccess();
    Job* job = entry->job_00;
    entry->state = 2;
    lookup_20.Remove(&job, &entry);
    delete entry;
    job_count -= 1;
    critical_section->releaseAccess();
    return 1;
}

// FUNCTION: SURRENDER 0x10014620
void srScheduler::wakeWorker()
{
    srCriticalSectionAccess access(critical_section);
    if (job_count != 0) {
        int busy = 0;
        for (int index = 0; index < worker_count; ++index) {
            if (workers[index].thread_handle != -1) {
                ++busy;
            }
        }
        if ((busy < job_count) && (busy < worker_count)) {
            for (int index = 0; index < worker_count; ++index) {
                if (workers[index].thread_handle == -1) {
                    workers[index].thread_handle =
                        srThread::begin(workerEntry, &workers[index]);
                    break;
                }
            }
        }
    }
}

// FUNCTION: SURRENDER 0x100146E0
void __cdecl srScheduler::workerEntry(void* argument)
{
    // The thread argument is the WorkerSlot passed
    // to srThread::begin in wakeWorker.
    WorkerSlot* slot = static_cast<WorkerSlot*>(argument);
    while (slot->scheduler->executeNextJob() != 0) {
        srThread::yield(0);
    }
    slot->thread_handle = -1;
    srThread::end();
}

// FUNCTION: SURRENDER 0x10013FE0
void srScheduler::waitForJob(Job* job)
{
    while (true) {
        critical_section->getAccess();
        bool pending = lookup_20.Lookup(&job) != 0;
        critical_section->releaseAccess();
        if (!pending) {
            return;
        }
        yieldOneMillisecond();
    }
}

// FUNCTION: SURRENDER 0x100146D0
static void yieldOneMillisecond()
{
    srThread::yield(1);
}
