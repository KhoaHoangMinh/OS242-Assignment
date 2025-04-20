/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "queue.h"

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1; //memory region id
    
    /* TODO: Get name of the target proc */
    //proc_name = libread..
    int i = 0;
    data = 0;
    while(data != -1){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    //caller->running_list
    //caller->mlq_ready_queu

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */
    int target_pid = atoi(&proc_name[1]) + 1;
    int kill_count = 0;
    if(caller->running_list != NULL){
        struct queue_t* running_q = caller->running_list;
        for(int i = 0; i < running_q->size; i++){
            struct pcb_t* running_proc = running_q->proc[i];
            if(running_proc->pid == target_pid){
                printf("Killing process \"%s\" with PID %d\n", proc_name, target_pid);

                struct pcb_t* kill_proc = running_proc;
                for(int j = i; j < running_q->size - 1; j++){
                    running_q->proc[j] = running_q->proc[j + 1];
                }
                running_q->proc[running_q->size - 1] = NULL;
                i--;
                running_q->size--;
                kill_count++;

                /* if(kill_proc->code) free(kill_proc->code);
                if(kill_proc->ready_queue) free(kill_proc->ready_queue);
                if(kill_proc->running_list) free(kill_proc->running_list);
                if(kill_proc->mlq_ready_queue) free(kill_proc->mlq_ready_queue); */

                printf("Process \"%s\" with PID %d killed\n", proc_name, target_pid);
            }
        }
    }

    #ifdef MLQ_SCHED
    if(caller->mlq_ready_queue != NULL){
        for(int prio = 0; prio < MAX_PRIO; prio++){
            struct queue_t* mlq_q = &caller->mlq_ready_queue[prio];
            for(int i = 0; i < mlq_q->size; i++){
                struct pcb_t* mlq_proc = mlq_q->proc[i];
                if(mlq_proc->pid == target_pid){
                    printf("Killing MLQ process \"%s\" with PID %d at priority %d\n", proc_name, target_pid, prio);

                    struct pcb_t* kill_proc = mlq_proc;
                    for(int j = i; j < mlq_q->size - 1; j++){
                        mlq_q->proc[j] = mlq_q->proc[j + 1];
                    }
                    mlq_q->proc[mlq_q->size - 1] = NULL;
                    i--;
                    mlq_q->size--;
                    kill_count++;

                    /* if(kill_proc->code) free(kill_proc->code);
                    if(kill_proc->ready_queue) free(kill_proc->ready_queue);
                    if(kill_proc->running_list) free(kill_proc->running_list);
                    if(kill_proc->mlq_ready_queue) free(kill_proc->mlq_ready_queue);
                    free(kill_proc); */

                    printf("MLQ Process \"%s\" with PID %d at priority %d killed\n", proc_name, target_pid, prio);
                }
            }
        }
    }
    #endif  

    printf("Total of %d processes with name \"%s\" were killed\n", kill_count, proc_name);
    return kill_count; 
    //return 0; 
}
