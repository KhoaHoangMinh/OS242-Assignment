/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include <mm.h>

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>

/**
 * @note: find that proc, remove from queue, free allocated memory
 */
int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc[100]; // Use proc array to store the process name
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;

    /* Retrieve the name of the target process */
    int i = 0;
    data = 0;
    while (data != -1) {
        libread(caller, memrg, i, &data);
        proc[i] = data;
        if (data == -1) proc[i] = '\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc);

    /* Traverse and terminate processes in running_list */
    printf("Traversing running_list...\n");
    char dir[100] = "input/proc/";
    int dir_len = strlen(dir);
    int list_size = caller->running_list->size;
    char * proc_name;

    i = 0;
    while (i < list_size) {
        proc_name = caller->running_list->proc[i]->path + dir_len;
        if (strcmp(proc_name, proc) == 0) {
            struct pcb_t * tgt = caller->running_list->proc[i];
            for (int j = i; j < list_size - 1; j++) {
                caller->running_list->proc[j] = caller->running_list->proc[j + 1];
            }
            free(tgt);
            caller->running_list->size--;
            list_size = caller->running_list->size;
            printf("Process %s killed from running_list\n", proc_name);
            continue;
        }
        i++;
    }
#ifdef MLQ_SCHED
    int prio = caller->prio;
    list_size = caller->mlq_ready_queue[prio].size;
    printf("Traversing mlq_ready_queue...\n");
    i = 0;
    while (i < list_size) {
    proc_name = caller->mlq_ready_queue[prio].proc[i]->path + dir_len;
        if (strcmp(proc_name, proc) == 0) {
            struct pcb_t * tgt = caller->mlq_ready_queue[prio].proc[i];
            for (int j = i; j < list_size - 1; j++) {
                caller->mlq_ready_queue[prio].proc[j] = caller->mlq_ready_queue[prio].proc[j + 1];
            }
            free(tgt);
            caller->mlq_ready_queue[prio].size--;
            list_size = caller->mlq_ready_queue[prio].size;
            printf("Process %s killed from mlq_ready_queue\n", proc_name);
            continue;
        }
        i++;
    }
#endif
    return 0;
}