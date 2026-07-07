/*

    Scheduler

    All functions are thread safe.

*/

#pragma once

#include <stdint.h>
#include <setjmp.h>
#include "platform/platform.h"


typedef enum Scheduler_Error {
    SCHEDULER_ERROR_SUCCESS,
    SCHEDULER_ERROR_TASKS_FULL,
} Scheduler_Error;

typedef enum Scheduler_Task_Status {
    TASK_ACTIVE    = 0x1, // Active means Task slot is active, task may sleeping or sleeping in a queue
    TASK_FINISHED  = 0x2,
    TASK_WAITING   = 0x4, // Task wants to execute but is waiting to be scheduled
    TASK_SLEEPING  = 0x8, // Task wants to sleep and is sleeping
} Scheduler_Task_Status;


#define IS_STATUS_ACTIVE(X) ((X) & TASK_ACTIVE)
#define IS_STATUS_FINISHED(X) ((X) & TASK_FINISHED)
#define IS_STATUS_WAITING(X) ((X) & TASK_WAITING)
#define IS_STATUS_SLEEPING(X) ((X) & TASK_SLEEPING)

#define MAX_SCHEDULER_TASKS 1000

#define MS (1000000LLU)
#define US (1000LLU)
#define NS (1LLU)

typedef void(*FnTaskEntry)(void*);

typedef struct Scheduler_Task {
    const char* name; // path/name
    FnTaskEntry entry_point;
    void*       entry_argument;

    Scheduler_Task_Status status;
    uint64_t              executed_ns;  // how long task has scheduled for execution (this is not now() - started_ns)
    uint64_t              started_ns;   // when task started
    uint64_t              sleep_at_ns;  // when task was put to sleep
    uint64_t              sleep_duration_ns;  // when task was put to sleep
    // uint64_t              requested_ns_per_second;        // How much time Task wants every second (high number gives higher priority over others)
    uint64_t              priority; // How much often Task wants to be scheduled every second (higher priority than others, A game running at 144hz wants 1/144)
    uint32_t              requested_frequency_per_second; // How often Task wants to be scheduled every second (A game running at 144hz wants 144 schedulings per second OR to be consistently running during that time)
    
    jmp_buf jump_buffer;
} Scheduler_Task;

typedef struct Scheduler_CpuInfo {
    int core_count;
} Scheduler_CpuInfo;

typedef struct Scheduler {
    Scheduler_CpuInfo cpuinfo;

    Scheduler_Task tasks[MAX_SCHEDULER_TASKS];

    Scheduler_Task* running_tasks[128]; // MAX 128 cores

    Scheduler_Task* waiting_tasks[MAX_SCHEDULER_TASKS];
    int             next_waiting_index;
    int             waiting_tasks_len;
    Scheduler_Task* sleeping_tasks[MAX_SCHEDULER_TASKS];
    int             sleeping_tasks_len;

    volatile int finished_tasks;

    Thread* core_threads; // same as cpuinfo->core_count

    uint64_t time_slice;

    uint64_t execution_start;
    uint64_t execution_duration;

} Scheduler;


void scheduler_init(Scheduler* scheduler, const Scheduler_CpuInfo cpuinfo);

void scheduler_set_cpuinfo(Scheduler* scheduler, const Scheduler_CpuInfo cpuinfo);

/*
    Can be called before and during scheduler execution
*/
Scheduler_Error scheduler_submit(Scheduler* scheduler, const char* name, FnTaskEntry entry_point, void* entry_argument);

// Schedules and executes tasks
void scheduler_execute(Scheduler* scheduler, uint64_t nanoseconds);

void scheduler_context_switch(uint64_t sleep_ns);

// This simulates a context switch of a thread/task.
// It may or may not context switch based on the scheduler.
// Called from a task's execution (in the task's entry point)
// The parameters aren't necessary if we use thread local storage.
#define task_sleep(NS) scheduler_context_switch(NS)
#define task_yield()   task_sleep(0LLU)


void scheduler_dump(Scheduler* scheduler);


