#ifndef MMST_H
#define MMST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "queue.h"
#include <pthread.h>

#define MemorySize int

typedef struct mi {
  struct mi *back, *next;
  int isfree,unit;
  void *m;
  MemorySize size;
} MemoryInfo;

typedef struct mu {
  struct mu *next;
  void *m;
  MemoryInfo *minfo, *minfow;
  MemorySize size, length,fsize;
  int unit;

  int state;
  pthread_t id;
  QueueManager *qmm;
} MemoryUnit;

typedef struct mm {
  MemoryUnit *mu;
  MemoryUnit *muw;
  int unitlength;
  pthread_mutex_t mutex;
} MemoryManager;

typedef MemoryInfo *(*MMalloc)(MemoryUnit *mu,MemorySize ms);
typedef void (*MFree)(MemoryUnit *mu,MemoryInfo *mi);
typedef void (*MRealloc)(void *, void *);

#define MemoryUnitSize sizeof(MemoryUnit)
#define MemoryInfoSize sizeof(MemoryInfo)
#define MemoryThreadPackSize sizeof(MemoryThreadPack)

void ThreadUnitMalloc(void *c);

MemoryInfo *MemoryManagerThreadCalloc(MemoryManager *mm, MemorySize ms);

MemoryManager *MemoryManagerInit(MemorySize unit);

MemoryUnit *NewUnit(MemoryManager *mm,MemorySize unitlength);

MemoryInfo *UnitMalloc(MemoryUnit * mu, MemorySize s);

void UnitFree(MemoryUnit * mu, MemoryInfo * mi);

MemoryInfo *MemoryManagerMalloc(MemoryManager *mm,MemorySize ms);

MemoryInfo *MemoryManagerCalloc(MemoryManager *mm,MemorySize ms);

MemoryInfo *MemoryManagerRealloc(MemoryManager *mm,MemoryInfo *mi,MemorySize ms);

void MemoryManagerFree(MemoryManager *mm,MemoryInfo *mi);

void MemoryManagerDestroy(MemoryManager *mm);

void *MemoryManagerGetMemory(MemoryInfo *mi);

bool MemoryManagerCMP(MemoryInfo *mi,MemoryInfo *mk,MemorySize s);

void MemoryManagerSecuritySetMemory(MemoryInfo *mi,void *v,MemorySize ms);

void MemoryManagerSetMemory(MemoryInfo *mi,void *v,MemorySize ms);

void MemoryUnitCheck(MemoryUnit * u);

void MemoryManagerLock(MemoryManager *mm);

void MemoryManagerUnLock(MemoryManager *mm);

#endif
