#include "fake_process.h"
#include "linked_list.h"
#pragma once

#define MAX_RUNNING 1


typedef struct {
  ListItem list;
  int pid;
  ListHead events;
} FakePCB;

struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args, int cpu);

typedef struct FakeOS{
  FakePCB** running;
  ListHead ready;
  ListHead waiting;
  int timer;
  int n_cpu;
  ScheduleFn schedule_fn;
  void* schedule_args;

  ListHead processes;
} FakeOS;

void FakeOS_init(FakeOS* os, int n_cpu);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
