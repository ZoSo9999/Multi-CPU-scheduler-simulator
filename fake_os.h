#include "fake_process.h"
#include "linked_list.h"
#pragma once

#define MAX_RUNNING 1
#define RR_SCHED 0
#define P_SCHED 1


typedef struct {
  ListItem list;
  int pid;
  ListHead events;
  int age;
  int priority;
  int arrival_time;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args, int cpu);

typedef struct FakeOS{
  FakePCB** running;
  ListHead ready;
  ListHead waiting;
  int timer;
  unsigned n_cpu;
  char schdule_fn_type;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os, unsigned n_cpu);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);