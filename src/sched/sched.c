
#include "sched/sched.h"
#include "platform/platform.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"


#ifdef _MSC_VER
    #define TLS __declspec(thread)
#else
    #define TLS __thread
#endif


void test();


int main(int argc, const char** argv) {

    test();

}


void scheduler_init(Scheduler* scheduler, const Scheduler_CpuInfo cpuinfo) {
    memset(scheduler, 0, sizeof(*scheduler));

    // for (int i=0;i<MAX_SCHEDULER_TASKS;i++) {
    //     Scheduler_Task* task = &scheduler->tasks[i];
    //     task->status = TASK_INACTIVE;
    // }

    scheduler_set_cpuinfo(scheduler, cpuinfo);
}

void scheduler_set_cpuinfo(Scheduler* scheduler, const Scheduler_CpuInfo cpuinfo) {
    scheduler->cpuinfo = cpuinfo;
}

Scheduler_Error scheduler_submit(Scheduler* scheduler, const char* name, FnTaskEntry entry_point, void* entry_argument) {
    // Find empty task
    Scheduler_Task* new_task = NULL;
    for (int i=0;i<MAX_SCHEDULER_TASKS;i++) {
        Scheduler_Task* task = &scheduler->tasks[i];
        if (!IS_STATUS_ACTIVE(task->status)) {
            new_task = task;
            break;
        }
    }
    if (!new_task) {
        return SCHEDULER_ERROR_TASKS_FULL;
    }

    // Fill found task
    memset(new_task, 0, sizeof(*new_task));
    new_task->name = name;
    new_task->entry_point = entry_point;
    new_task->entry_argument = entry_argument;
    new_task->status = TASK_ACTIVE;

    return SCHEDULER_ERROR_SUCCESS;
}

typedef struct {
    Scheduler* scheduler;
    int core_index;
} CoreInfo;


TLS CoreInfo* core_info;

void scheduler_core_entry(void* arg) {

    core_info = (CoreInfo*) arg;

    while (1) {
        uint64_t execution_time = time__now() - core_info->scheduler->execution_start;
        if (execution_time > core_info->scheduler->execution_duration)
            break;
        
        scheduler_context_switch(0);
    }
}

void scheduler_execute(Scheduler* scheduler, uint64_t nanoseconds) {
    // Execute for X number of seconds

    /*
        Scheduling for ELOS

        Execution domains which is an isolation of a group of threads that execute entry points from artifacts.

        We need to decide when the CPU should execute which virtual thread on which cpu core.

        Some threads have higher priority such as the main/root terminal and other kernel tasks.
        These should always get 10ms every second or whatever they need.

        We have different kinds of work that a thread does.
        1. Consistent work where a thread should be scheduled at least every 16ms to achieve 60 FPS in a game. It could also be something lighter like text editor where the work is less intense.
        2. Light work where a thread executes for a small time (100us) before sleeping. This may happen a couple of times per second.
        2. Normal work where we don't have any constraints on when we finish. Rendering a video. Reading files, processing data, outputting files like a compiler.
            We want to do it has fast as possible but it's not as important as a game.

        If a normal task has 8 threads which is scheduled on a

    */

    int core_count = scheduler->cpuinfo.core_count;
    scheduler->core_threads = malloc(core_count * sizeof(Thread));
    memset(scheduler->core_threads, 0, core_count * sizeof(Thread));

    CoreInfo* core_infos = malloc(core_count * sizeof(CoreInfo));
    memset(core_infos, 0, core_count * sizeof(CoreInfo));

    scheduler->execution_start = time__now();

    for (int i=0;i<core_count;i++) {
        core_infos[i].core_index = i;
        core_infos[i].scheduler = scheduler;
        thread__spawn(&scheduler->core_threads[i], scheduler_core_entry, &core_infos[i]);
    }

    // Chilling
    
    for (int i=0;i<core_count;i++) {
        thread__join(&scheduler->core_threads[i]);
    }

}


const char* status_to_string(char* buffer, Scheduler_Task_Status status) {
    int len = 0;
    if (IS_STATUS_ACTIVE(status)) {
        len += sprintf(buffer+len, "Active");
    } else {
        len += sprintf(buffer+len, "Inactive");
    }
    return buffer;
}

void scheduler_dump(Scheduler* scheduler) {
    
    int active_tasks = 0;
    for (int i=0;i<MAX_SCHEDULER_TASKS;i++) {
        Scheduler_Task* task = &scheduler->tasks[i];
        if (IS_STATUS_ACTIVE(task->status)) {
            active_tasks++;
        }
    }

    log__printf("Scheduler dump:\n");
    log__printf(" CPU Core Count: %d\n", scheduler->cpuinfo.core_count);
    log__printf("\n");
    log__printf("Tasks (%d active, %d finished)\n", active_tasks, scheduler->finished_tasks);


    char buffer[256];
    for (int i=0;i<MAX_SCHEDULER_TASKS;i++) {
        Scheduler_Task* task = &scheduler->tasks[i];
        if (!IS_STATUS_ACTIVE(task->status) && !IS_STATUS_FINISHED(task->status))
            continue;

        log__printf("  %s %s %d ns\n", task->name, status_to_string(buffer, task->status), (int)task->executed_ns);
    }
}


void scheduler_context_switch(uint64_t sleep_ns) {

    // Called from threads running tasks.

    if (sleep_ns > 0) {
        // We should definitely context switch. (unless all other threads are sleeping in which case we're chilling)
    } else {
        // We may not context switch
    }

    // To which task should we switch?

    // We have our consistent tasks, light tasks and normal tasks.

    uint64_t now = time__now();

    Scheduler* scheduler = core_info->scheduler;

    // Pick a task quickly no matter amount of tasks. O(1)

    // Pick from idling tasks.

    while (!scheduler->waiting_tasks_len) ;
    // @NOCHECKIN Not thread-safe

    Scheduler_Task* running_task = scheduler->running_tasks[core_info->core_index];

    // @TODO If task complete then remove.

    running_task->status = TASK_ACTIVE|TASK_WAITING;

    Scheduler_Task* task = scheduler->waiting_tasks[scheduler->next_waiting_index];
    scheduler->waiting_tasks[scheduler->next_waiting_index] = running_task;
    scheduler->next_waiting_index++;

    task->status = TASK_ACTIVE;

    scheduler->running_tasks[core_info->core_index] = task;

    


    for (int i=0;i<MAX_SCHEDULER_TASKS;i++) {
        Scheduler_Task* task = &scheduler->tasks[i];
        if (!IS_STATUS_ACTIVE(task->status))
            continue;

        if (IS_STATUS_SLEEPING(task->status)) {
            if (now > task->sleep_at_ns + task->sleep_duration_ns) {
                // stop sleeping
            }

        }

        // 
        task->executed_ns
    }
}

void firefox_entry(void* arg) {

}
void game_entry(void* arg) {

}

void cat_entry(void* arg) {

}


void test() {
    
    Scheduler* scheduler = mem__alloc(sizeof(Scheduler));

    Scheduler_CpuInfo cpuinfo = {
        .core_count = 8,
    };
    scheduler_init(scheduler, cpuinfo);

    scheduler_submit(scheduler, "firefox", firefox_entry, NULL);
    scheduler_submit(scheduler, "game", game_entry, NULL);
    scheduler_submit(scheduler, "cat", cat_entry, NULL);

    scheduler_dump(scheduler);

    scheduler_execute(scheduler, 1*US);

    scheduler_dump(scheduler);

}
