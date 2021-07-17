#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "fake_os.h"

// #define _OS_DEBUG_

void FakeOS_init(FakeOS* os, unsigned n_cpu) {
  if (n_cpu)  
     os->n_cpu=n_cpu;
  else 
    os->n_cpu=MAX_RUNNING;
  os->running=(FakePCB**)malloc(n_cpu*sizeof(FakePCB*));
  for (int i=0; i<os->n_cpu; ++i) os->running[i]=0;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schdule_fn_type=-1;
  os->schedule_fn=0;
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid

  int i;
  ListItem* aux;

  for (i=0; i<os->n_cpu; ++i){
    assert((!os->running[i] || os->running[i]->pid!=p->pid) && "pid taken");
  }

  aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // all fine, no such pcb exists
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->events=p->events;
  new_pcb->priority=p->priority;
  new_pcb->arrival_time=-1;
  new_pcb->age=0;

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){
  case CPU:
    if (os->schdule_fn_type == RR_SCHED)
      List_pushBack(&os->ready, (ListItem*) new_pcb);
    else {
      new_pcb->arrival_time=os->timer;
      ListItem* aux=os->ready.first;
      while (aux){
        FakePCB* pcb=(FakePCB*)aux;
        if (pcb->priority-pcb->age > new_pcb->priority){
          List_insert_next(&os->ready, (ListItem*)pcb, (ListItem*)new_pcb);
          break;
        }
        aux=aux->next;
      }  
      if (!aux)
        List_pushBack(&os->ready, (ListItem*) new_pcb);
    }
    break;
  case IO:
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
    break;
  default:
    assert(0 && "illegal resource");
    ;
  }
}




void FakeOS_simStep(FakeOS* os){
  
  printf("************** TIME: %08d **************\n", os->timer);

  //aging processes
  if (os->schdule_fn_type==P_SCHED)
    (*os->schedule_fn)(os, os->schedule_args, -1);


  //scan process waiting to be started
  //and create all processes starting now
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    if (new_process) {
      printf("\tcreate pid:%d\n", new_process->pid);
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }

  // scan waiting list, and put in ready all items whose event terminates
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    printf("\twaiting pid: %d\n", pcb->pid);
    assert(e->type==IO);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend burst\n");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);
      if (! pcb->events.first) {
        // kill process
        printf("\t\tend process\n");
        free(pcb);
      } else {
        //handle next event
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          if (os->schdule_fn_type == RR_SCHED)
            List_pushBack(&os->ready, (ListItem*) pcb);
          else {
            pcb->arrival_time=os->timer;
            ListItem* aux=os->ready.first;
            while (aux){
              FakePCB* next_pcb=(FakePCB*)aux;
              if (next_pcb->priority-next_pcb->age > pcb->priority){
                List_insert_next(&os->ready, (ListItem*)next_pcb, (ListItem*)pcb);
                break;
              }
              aux=aux->next;
            }  
            if (!aux)
              List_pushBack(&os->ready, (ListItem*) pcb);
          }
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
      }
    }
  }

  // decrement the duration of running processes
  // if event over, destroy event
  // and reschedule process
  // if last event, destroy running
  int i;
  for (i=0; i<os->n_cpu; ++i){
    FakePCB* running=os->running[i];
    printf("\tCPU: %d, pid: %d\n", i+1, running?running->pid:-1);
    if (running) {
      ProcessEvent* e=(ProcessEvent*) running->events.first;
      assert(e->type==CPU);
      e->duration--;
      printf("\t\tremaining time:%d\n",e->duration);
      if (e->duration==0){
        printf("\t\tend burst\n");
        List_popFront(&running->events);
        free(e);
        if (! running->events.first) {
          printf("\t\tend process\n");
          free(running); // kill process
        } else {
          running->age=0;
          e=(ProcessEvent*) running->events.first;
          switch (e->type){
          case CPU:
            printf("\t\tmove to ready\n");
            if (os->schdule_fn_type == RR_SCHED)
              List_pushBack(&os->ready, (ListItem*) running);
            else {
              running->arrival_time=os->timer;
              ListItem* aux=os->ready.first;
              while (aux){
                FakePCB* next_pcb=(FakePCB*)aux;
                if (next_pcb->priority-next_pcb->age > running->priority){
                  List_insert_next(&os->ready, (ListItem*)next_pcb, (ListItem*)running);
                  break;
                }
                aux=aux->next;
              }  
              if (!aux)
                List_pushBack(&os->ready, (ListItem*) running);
            }
            break;
          case IO:
            printf("\t\tmove to waiting\n");
            running->arrival_time=-1;
            List_pushBack(&os->waiting, (ListItem*) running);
            break;
          }
        }
        os->running[i] = 0;
      }
    }
  }
  
  // call schedule and list the ready processes, if defined
  if (os->schedule_fn){
    #ifdef _OS_DEBUG_
      aux=os->ready.first;
      while(aux){
      FakePCB* pcb=(FakePCB*)aux;
      aux=aux->next;
      if (os->schdule_fn_type==P_SCHED)
          printf("\tready pid:%d\n\t\tpriority:%d\n",pcb->pid,pcb->priority-pcb->age<0 ? 0:pcb->priority-pcb->age);
      else 
        printf("\tready pid:%d\n",pcb->pid);
      }
    #endif

    for (i=0; i<os->n_cpu; ++i)
      if (!os->running[i])
        (*os->schedule_fn)(os, os->schedule_args, i);
  }

  // if running not defined and ready queue not empty
  // put the first in ready to run
  if (os->ready.first) {
    for (i=0; i<os->n_cpu; ++i)
      if (!os->running[i])
        os->running[i]=(FakePCB*) List_popFront(&os->ready);
  }
  
  ++os->timer;
}

void FakeOS_destroy(FakeOS* os) {
  free(os->running);
}
