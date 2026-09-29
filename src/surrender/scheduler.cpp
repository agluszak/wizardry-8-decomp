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
    critical_section_40 = new CRITICAL_SECTION;
    if (critical_section_40 != 0) {
        InitializeCriticalSection(critical_section_40);
    }
    first_job_30 = 0;
    last_job_34 = 0;
    job_count_38 = 0;
    for (int index = 0; index != 4; ++index) {
        workers_00[index].thread_handle_00 = -1;
        workers_00[index].scheduler_04 = this;
    }
    worker_count_3c = srCore.getTimer()->m_cpu_count;
    if (worker_count_3c < 1) {
        worker_count_3c = 1;
    }
    if (4 < worker_count_3c) {
        worker_count_3c = 4;
    }
}

// FUNCTION: SURRENDER 0x10013E30
srScheduler::~srScheduler()
{
    cancelAll();
    for (int index = 0; index < worker_count_3c; ++index) {
        while (workers_00[index].thread_handle_00 != -1) {
            yieldOneMillisecond();
        }
    }
    if (critical_section_40 != 0) {
        EnterCriticalSection(critical_section_40);
        LeaveCriticalSection(critical_section_40);
        DeleteCriticalSection(critical_section_40);
        operator delete(critical_section_40);
    }
}

// FUNCTION: SURRENDER 0x10013EE0
void srScheduler::queue(Job& job)
{
    EnterCriticalSection(critical_section_40);
    QueueEntry* entry = new QueueEntry;
    entry->job_00 = &job;
    entry->next_04 = 0;
    entry->previous_08 = last_job_34;
    entry->state_0c = 0;
    if (last_job_34 != 0) {
        last_job_34->next_04 = entry;
    }
    last_job_34 = entry;
    if (first_job_30 == 0) {
        first_job_30 = entry;
    }
    job_count_38 += 1;
    Job* key = &job;
    lookup_20.Insert(&key, &entry);
    wakeWorker();
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x10014060
void srScheduler::cancel(Job& job)
{
    EnterCriticalSection(critical_section_40);
    Job* key = &job;
    QueueEntry* entry = lookup_20.Lookup(&key);
    if (entry != 0 && entry->state_0c != 2) {
        if (entry->state_0c == 0) {
            entry->job_00->cancel();
            removeQueueEntry(entry);
            LeaveCriticalSection(critical_section_40);
            return;
        }
        LeaveCriticalSection(critical_section_40);
        waitForJob(&job);
        return;
    }
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x10014110
void srScheduler::finish(Job& job)
{
    EnterCriticalSection(critical_section_40);
    Job* key = &job;
    QueueEntry* entry = lookup_20.Lookup(&key);
    if (entry != 0 && entry->state_0c != 2) {
        if (entry->state_0c != 0) {
            LeaveCriticalSection(critical_section_40);
            waitForJob(&job);
            return;
        }
        Job* queued = entry->job_00;
        lookup_20.Remove(&queued, &entry);
        if (entry->previous_08 == 0) {
            first_job_30 = entry->next_04;
        } else {
            entry->previous_08->next_04 = entry->next_04;
        }
        if (entry->next_04 == 0) {
            last_job_34 = entry->previous_08;
        } else {
            entry->next_04->previous_08 = entry->previous_08;
        }
        entry->state_0c = 1;
        LeaveCriticalSection(critical_section_40);
        queued->execute();
        EnterCriticalSection(critical_section_40);
        entry->state_0c = 2;
        delete entry;
        job_count_38 -= 1;
        LeaveCriticalSection(critical_section_40);
        return;
    }
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x10014310
void srScheduler::cancelAll()
{
    EnterCriticalSection(critical_section_40);
    QueueEntry* entry = first_job_30;
    while (entry != 0) {
        entry->job_00->cancel();
        removeQueueEntry(first_job_30);
        entry = first_job_30;
    }
    LeaveCriticalSection(critical_section_40);
    finishAll();
}

// FUNCTION: SURRENDER 0x10014360
void srScheduler::finishAll()
{
    while (true) {
        EnterCriticalSection(critical_section_40);
        long count = job_count_38;
        LeaveCriticalSection(critical_section_40);
        if (count == 0) {
            break;
        }
        yieldOneMillisecond();
    }
}

// FUNCTION: SURRENDER 0x100142F0
long srScheduler::getJobCount() const
{
    CRITICAL_SECTION* section = critical_section_40;
    EnterCriticalSection(section);
    long count = job_count_38;
    LeaveCriticalSection(section);
    return count;
}

// FUNCTION: SURRENDER 0x100143A0
void srScheduler::removeQueueEntry(QueueEntry* entry)
{
    EnterCriticalSection(critical_section_40);
    Job* job = entry->job_00;
    lookup_20.Remove(&job, &entry);
    if (entry->previous_08 == 0) {
        first_job_30 = entry->next_04;
    } else {
        entry->previous_08->next_04 = entry->next_04;
    }
    if (entry->next_04 == 0) {
        last_job_34 = entry->previous_08;
    } else {
        entry->next_04->previous_08 = entry->previous_08;
    }
    delete entry;
    job_count_38 -= 1;
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x100144B0
long srScheduler::executeNextJob()
{
    EnterCriticalSection(critical_section_40);
    QueueEntry* entry = first_job_30;
    if (entry == 0) {
        LeaveCriticalSection(critical_section_40);
        return 0;
    }
    if (entry->previous_08 == 0) {
        first_job_30 = entry->next_04;
    } else {
        entry->previous_08->next_04 = entry->next_04;
    }
    if (entry->next_04 == 0) {
        last_job_34 = entry->previous_08;
    } else {
        entry->next_04->previous_08 = entry->previous_08;
    }
    entry->state_0c = 1;
    LeaveCriticalSection(critical_section_40);
    entry->job_00->execute();
    EnterCriticalSection(critical_section_40);
    Job* job = entry->job_00;
    entry->state_0c = 2;
    lookup_20.Remove(&job, &entry);
    delete entry;
    job_count_38 -= 1;
    LeaveCriticalSection(critical_section_40);
    return 1;
}

// FUNCTION: SURRENDER 0x10014620
void srScheduler::wakeWorker()
{
    EnterCriticalSection(critical_section_40);
    if (job_count_38 != 0) {
        int busy = 0;
        for (int index = 0; index < worker_count_3c; ++index) {
            if (workers_00[index].thread_handle_00 != -1) {
                ++busy;
            }
        }
        if ((busy < job_count_38) && (busy < worker_count_3c)) {
            for (int index = 0; index < worker_count_3c; ++index) {
                if (workers_00[index].thread_handle_00 == -1) {
                    workers_00[index].thread_handle_00 =
                        srThread::begin(workerEntry, &workers_00[index]);
                    break;
                }
            }
        }
    }
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x100146E0
void __cdecl srScheduler::workerEntry(void* argument)
{
    // reinterpret-ok: thread-proc ABI; the argument is the WorkerSlot passed
    // to srThread::begin in wakeWorker.
    WorkerSlot* slot = reinterpret_cast<WorkerSlot*>(argument);
    while (slot->scheduler_04->executeNextJob() != 0) {
        srThread::yield(0);
    }
    slot->thread_handle_00 = -1;
    srThread::end();
}

// FUNCTION: SURRENDER 0x10013FE0
void srScheduler::waitForJob(Job* job)
{
    while (true) {
        EnterCriticalSection(critical_section_40);
        bool pending = lookup_20.Lookup(&job) != 0;
        LeaveCriticalSection(critical_section_40);
        if (!pending) {
            return;
        }
        yieldOneMillisecond();
    }
}

// SYNTHETIC: SURRENDER 0x100142BA
// catch-rethrow funclet emission

// SYNTHETIC: SURRENDER 0x100145F7
// catch-rethrow funclet emission

// SYNTHETIC: SURRENDER 0x10014720
// member pointer-pair destructor emission (EH unwind)

// FUNCTION: SURRENDER 0x100146D0
static void yieldOneMillisecond()
{
    srThread::yield(1);
}
