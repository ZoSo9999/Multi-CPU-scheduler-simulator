#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  int quantum;
} SchedRRArgs;

typedef struct {
  int quantum;
  int ageing_time;
} prioritySchedArgs;

void prioritySched(FakeOS* os, void* args_, int cpu){
  prioritySchedArgs* args=(prioritySchedArgs*)args_;
  if (! os->ready.first)
    return;

  if (cpu>-1) {
    FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
    os->running[cpu]=pcb;
    
    assert(pcb->events.first);
    ProcessEvent* e = (ProcessEvent*)pcb->events.first;
    assert(e->type==CPU);

    // look at the first event
    // if duration>quantum
    // push front in the list of event a CPU event of duration quantum
    // alter the duration of the old event subtracting quantum
    if (e->duration>args->quantum) {
      ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
      qe->list.prev=qe->list.next=0;
      qe->type=CPU;
      qe->duration=args->quantum;
      e->duration-=args->quantum;
      List_pushFront(&pcb->events, (ListItem*)qe);
    }
  }

  else {
    ListItem* aux=os->ready.first;
    while(aux) {
      FakePCB* pcb=(FakePCB*)aux;
      aux=aux->next;
      if ((os->timer-pcb->arrival_time) && 
        !((os->timer-pcb->arrival_time) % args->ageing_time))
        pcb->age++;
    }
  }
}

void schedRR(FakeOS* os, void* args_, int cpu){
  SchedRRArgs* args=(SchedRRArgs*)args_;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first || cpu < 0)
    return;

  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running[cpu]=pcb;
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
};

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("use: ./nome_eseguibile n_cpu p1.txt p2.txt...\n");
    return 0;
  }

  int i = 1;
  unsigned cpu = atoi(argv[1]);
  if (cpu) {
    os.n_cpu = cpu;
    ++i;
  }

  FakeOS_init(&os,cpu);

  SchedRRArgs srr_args;
  srr_args.quantum=5;
  os.schedule_args=&srr_args;
  os.schedule_fn=schedRR;
  os.schdule_fn_type=RR_SCHED;

  // prioritySchedArgs sp_args;
  // sp_args.quantum=5;
  // sp_args.ageing_time=2;
  // os.schedule_args=&sp_args;
  // os.schedule_fn=prioritySched;
  // os.schdule_fn_type=P_SCHED;
  
  for (; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d, num cpu %d, sched type %d\n", os.processes.size, os.n_cpu, os.schdule_fn_type);
  while(os.running[0]
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
  FakeOS_destroy(&os);
}
