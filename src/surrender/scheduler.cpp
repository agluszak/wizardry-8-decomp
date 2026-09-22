#include "surrender/srScheduler.h"
#include "surrender/srCore.h"
#include "surrender/srHeap.h"
#include "surrender/srThread.h"
#include "surrender/srVariableTimer.h"

// FUNCTION: SURRENDER 0x10013D70
srScheduler::srScheduler()
{
    lookup_capacity_2c = 0;
    lookup_buckets_20 = 0;
    lookup_entries_24 = 0;
    free_lookup_entry_28 = -1;
    growLookup();
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
            srThread::yield(1);
        }
    }
    if (critical_section_40 != 0) {
        EnterCriticalSection(critical_section_40);
        LeaveCriticalSection(critical_section_40);
        DeleteCriticalSection(critical_section_40);
        operator delete(critical_section_40);
    }
    if (lookup_buckets_20 != 0) {
        operator delete(lookup_buckets_20);
    }
    if (lookup_entries_24 != 0) {
        operator delete(lookup_entries_24);
    }
}

// FUNCTION: SURRENDER 0x10014750
void srScheduler::growLookup()
{
    unsigned long new_capacity = lookup_capacity_2c * 2;
    if (new_capacity < 4) {
        new_capacity = 4;
    }
    LookupEntry* new_entries = new LookupEntry[new_capacity];
    long* new_buckets = new long[new_capacity];
    int count = 0;
    for (unsigned long index = 0; index < new_capacity; ++index) {
        new_entries[index].next_index_00 = -1;
        new_buckets[index] = -1;
    }
    if (lookup_capacity_2c != 0) {
        for (long bucket = 0; bucket < lookup_capacity_2c; ++bucket) {
            long entry = lookup_buckets_20[bucket];
            if (entry != -1) {
                do {
                    LookupEntry* current = &lookup_entries_24[entry];
                    LookupEntry* moved = &new_entries[count];
                    moved->job_04 = current->job_04;
                    unsigned long hash =
                        (((unsigned long)moved->job_04 >> 10 ^ (unsigned long)moved->job_04) >> 10 ^
                         (unsigned long)moved->job_04) &
                        (new_capacity - 1);
                    moved->queue_entry_08 = current->queue_entry_08;
                    moved->next_index_00 = new_buckets[hash];
                    new_buckets[hash] = count;
                    entry = current->next_index_00;
                    ++count;
                } while (entry != -1);
            }
        }
        delete[] lookup_buckets_20;
        delete[] lookup_entries_24;
    }
    if (count < (long)new_capacity) {
        long entry = count;
        do {
            ++entry;
            new_entries[entry - 1].next_index_00 = entry;
        } while (entry < (long)new_capacity);
    }
    new_entries[new_capacity - 1].next_index_00 = -1;
    free_lookup_entry_28 = count;
    lookup_buckets_20 = new_buckets;
    lookup_capacity_2c = new_capacity;
    lookup_entries_24 = new_entries;
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
    if (free_lookup_entry_28 == -1) {
        growLookup();
    }
    long index = free_lookup_entry_28;
    LookupEntry* lookup = &lookup_entries_24[index];
    free_lookup_entry_28 = lookup->next_index_00;
    unsigned long bucket =
        (((unsigned long)&job >> 10 ^ (unsigned long)&job) >> 10 ^ (unsigned long)&job) &
        (lookup_capacity_2c - 1);
    lookup->job_04 = &job;
    lookup->queue_entry_08 = entry;
    lookup->next_index_00 = lookup_buckets_20[bucket];
    lookup_buckets_20[bucket] = index;
    wakeWorker();
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x10014060
void srScheduler::cancel(Job& job)
{
    EnterCriticalSection(critical_section_40);
    long index = lookup_buckets_20[(((unsigned long)&job >> 10 ^ (unsigned long)&job) >> 10 ^
                                    (unsigned long)&job) &
                                   (lookup_capacity_2c - 1)];
    if (index != -1) {
        do {
            if (lookup_entries_24[index].job_04 == &job) {
                QueueEntry* entry = lookup_entries_24[index].queue_entry_08;
                if ((entry != 0) && (entry->state_0c != 2)) {
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
                break;
            }
            index = lookup_entries_24[index].next_index_00;
        } while (index != -1);
    }
    LeaveCriticalSection(critical_section_40);
}

// FUNCTION: SURRENDER 0x10014110
void srScheduler::finish(Job& job)
{
    EnterCriticalSection(critical_section_40);
    long index = lookup_buckets_20[(((unsigned long)&job >> 10 ^ (unsigned long)&job) >> 10 ^
                                    (unsigned long)&job) &
                                   (lookup_capacity_2c - 1)];
    if (index != -1) {
        do {
            if (lookup_entries_24[index].job_04 == &job) {
                QueueEntry* entry = lookup_entries_24[index].queue_entry_08;
                if ((entry != 0) && (entry->state_0c != 2)) {
                    if (entry->state_0c != 0) {
                        LeaveCriticalSection(critical_section_40);
                        waitForJob(&job);
                        return;
                    }
                    Job* queued = entry->job_00;
                    long* bucket =
                        &lookup_buckets_20[(lookup_capacity_2c - 1) &
                                           (((unsigned long)queued >> 10 ^ (unsigned long)queued) >>
                                                10 ^
                                            (unsigned long)queued)];
                    index = *bucket;
                    long previous = -1;
                    while (index != -1) {
                        LookupEntry* lookup = &lookup_entries_24[index];
                        if ((lookup->job_04 == queued) && (lookup->queue_entry_08 == entry)) {
                            if (previous == -1) {
                                *bucket = lookup->next_index_00;
                            } else {
                                lookup_entries_24[previous].next_index_00 = lookup->next_index_00;
                            }
                            lookup->next_index_00 = free_lookup_entry_28;
                            free_lookup_entry_28 = index;
                            break;
                        }
                        previous = index;
                        index = lookup->next_index_00;
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
                    queued->execute();
                    EnterCriticalSection(critical_section_40);
                    entry->state_0c = 2;
                    delete entry;
                    job_count_38 -= 1;
                    LeaveCriticalSection(critical_section_40);
                    return;
                }
                break;
            }
            index = lookup_entries_24[index].next_index_00;
        } while (index != -1);
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
        srThread::yield(1);
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
    long* bucket = &lookup_buckets_20[(((unsigned long)job >> 10 ^ (unsigned long)job) >> 10 ^
                                       (unsigned long)job) &
                                      (lookup_capacity_2c - 1)];
    long index = *bucket;
    long previous = -1;
    if (index != -1) {
        do {
            LookupEntry* lookup = &lookup_entries_24[index];
            if ((lookup->job_04 == job) && (lookup->queue_entry_08 == entry)) {
                if (previous == -1) {
                    *bucket = lookup->next_index_00;
                } else {
                    lookup_entries_24[previous].next_index_00 = lookup->next_index_00;
                }
                lookup->next_index_00 = free_lookup_entry_28;
                free_lookup_entry_28 = index;
                break;
            }
            index = lookup->next_index_00;
            previous = index;
        } while (index != -1);
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
    long* bucket = &lookup_buckets_20[(((unsigned long)job >> 10 ^ (unsigned long)job) >> 10 ^
                                       (unsigned long)job) &
                                      (lookup_capacity_2c - 1)];
    long index = *bucket;
    long previous = -1;
    if (index != -1) {
        do {
            LookupEntry* lookup = &lookup_entries_24[index];
            if ((lookup->job_04 == job) && (lookup->queue_entry_08 == entry)) {
                if (previous == -1) {
                    *bucket = lookup->next_index_00;
                } else {
                    lookup_entries_24[previous].next_index_00 = lookup->next_index_00;
                }
                lookup->next_index_00 = free_lookup_entry_28;
                free_lookup_entry_28 = index;
                break;
            }
            index = lookup->next_index_00;
            previous = index;
        } while (index != -1);
    }
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
    /* The thread-proc ABI arrives as void*; the argument is the WorkerSlot
       passed to srThread::begin in wakeWorker. */
    WorkerSlot* slot = static_cast<WorkerSlot*>(argument);
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
        long index = lookup_buckets_20[(((unsigned long)job >> 10 ^ (unsigned long)job) >> 10 ^
                                        (unsigned long)job) &
                                       (lookup_capacity_2c - 1)];
        bool pending = false;
        if (index != -1) {
            do {
                if (lookup_entries_24[index].job_04 == job) {
                    pending = lookup_entries_24[index].queue_entry_08 != 0;
                    break;
                }
                index = lookup_entries_24[index].next_index_00;
            } while (index != -1);
        }
        LeaveCriticalSection(critical_section_40);
        if (!pending) {
            return;
        }
        srThread::yield(1);
    }
}
