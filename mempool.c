#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "queue.h"
#include "mempool.h"

MemoryUnit *NewUnit(MemoryManager *mm, MemorySize unitlength) {
    void *g = malloc(MemoryUnitSize + unitlength);
    MemoryUnit *mu = (MemoryUnit *) g;
    mu->m = g + MemoryUnitSize;
    mu->size = unitlength;
    mu->unit = mm->unitlength;
    mm->unitlength++;
    mu->length = 0;
    mu->fsize = 0;
    mu->minfo = NULL;
    mu->minfow = NULL;
    mu->next = NULL;

    if (mm->mu == NULL) {
        mm->mu = mu;
        mm->muw = mu;
    } else {
        mm->muw->next = mu;
        mm->muw = mu;
    }
    return mu;
}

void MemoryManagerLock(MemoryManager *mm){
while (true){
        if (pthread_mutex_trylock(&(mm->mutex)) == 0) {
            break;
        }
    }
}

void MemoryManagerUnLock(MemoryManager *mm){
pthread_mutex_unlock(&(mm->mutex));
}

MemoryInfo *MemoryManagerMalloc(MemoryManager *mm, MemorySize ms) {
    int us;
    MemoryUnit *mu = mm->mu;
    while (true) {
        if (mu == NULL) {
            break;
        } else {
            if (mu->size - mu->length >= ms + MemoryInfoSize
                || mu->fsize >= ms + MemoryInfoSize) {
                return UnitMalloc(mu, ms);
            } else {
                us = mu->size;
                mu = mu->next;
            }
        }
    }
    mu = NewUnit(mm, us);
    return UnitMalloc(mu, ms);
}

MemoryInfo *MemoryManagerCalloc(MemoryManager *mm, MemorySize ms) {
    int us;
    MemoryUnit *mu = mm->mu;
    while (true) {
        if (mu == NULL) {
            break;
        } else {
            if (mu->size - mu->length >= ms + MemoryInfoSize
                || mu->fsize >= ms + MemoryInfoSize) {
                MemoryInfo *m = UnitMalloc(mu, ms);
                memset(m->m, 0, (size_t) m->size);
                return m;
            } else {
                us = mu->size;
                mu = mu->next;
            }
        }
    }
    mu = NewUnit(mm, us);
    MemoryInfo *m = UnitMalloc(mu, ms);
    memset(m->m, 0, (size_t) m->size);
    return m;
}

void MemoryManagerFree(MemoryManager *mm, MemoryInfo *mi) {
    MemoryUnit *mu = mm->mu;
    while (true) {
        if (mu == NULL) {
            break;
        } else {
            if (mu->unit == mi->unit) {
                UnitFree(mu, mi);
                break;
            } else {
                mu = mu->next;
            }
        }
    }
}

MemoryManager *MemoryManagerInit(MemorySize unit) {
    MemoryManager *mm = (MemoryManager *) malloc(sizeof(MemoryManager));
    pthread_mutex_init(&(mm->mutex), NULL);
    mm->unitlength = unit;
    mm->mu = NULL;
    mm->muw = NULL;
    NewUnit(mm, unit);
    return mm;
}

MemoryInfo *UnitMalloc(MemoryUnit *mu, MemorySize s) {
    if (mu->minfo != NULL && mu->fsize >= s + MemoryInfoSize) {
        MemoryInfo *m = mu->minfo;
        while (true) {
            if (m == NULL) {
                break;
            } else {
                if (m->size < s) {
                    m = m->next;
                    continue;
                }
                if (m->isfree == 1) {
                    if (m->size == s) {
                        m->isfree = 0;
                        mu->fsize -= m->size + MemoryInfoSize;
                        return m;
                    }
                    if (m->size > s + MemoryInfoSize) {
                        m->size -= s + MemoryInfoSize;
                        mu->fsize -= s + MemoryInfoSize;
                        MemoryInfo *mi = (MemoryInfo *) (m->m + m->size);
                        mi->m = (m->m + m->size + MemoryInfoSize);
                        mi->unit = mu->unit;
                        mi->size = s;
                        mi->isfree = 0;
                        mi->next = m->next;
                        mi->back = m;
                        if (m->next != NULL) {
                            m->next->back = mi;
                        }
                        if (mu->minfow == m) {
                            mu->minfow = mi;
                        }
                        m->next = mi;
                        return mi;
                    }
                }
                m = m->next;
            }
        }
    }
    if (mu->size - mu->length >= s + MemoryInfoSize) {
        MemoryInfo *mi = (MemoryInfo *) (mu->m + mu->length);
        mu->length += MemoryInfoSize;
        mi->m = (mu->m + mu->length);
        mu->length += s;
        mi->unit = mu->unit;
        mi->size = s;
        mi->isfree = 0;
        mi->next = NULL;
        mi->back = mu->minfow;
        if (mu->minfo == NULL) {
            mu->minfo = mi;
            mu->minfow = mi;
        } else {
            mu->minfow->next = mi;
            mu->minfow = mi;
        }
        return mi;
    }
}

void UnitFree(MemoryUnit *mu, MemoryInfo *m) {
    if (m->isfree == 0) {
        m->isfree = 1;
        mu->fsize += (m->size + MemoryInfoSize);
        MemoryInfo *mi = m->back, *mw = NULL, *miw = NULL;
        int next = 0;
        if (mi != NULL) {
            if (mi->isfree == 1) {
                miw = (mi->m + mi->size);
                if (miw == m) {
                    mi->size += m->size + MemoryInfoSize;
                    mi->next = m->next;
                    if (m->next != NULL) {
                        m->next->back = mi;
                    }
                    mu->minfow = mu->minfow == m ? mi : mu->minfow;
                    mu->minfo = mu->minfo == m ? mi : mu->minfo;

                    if (m->next != NULL) {
                        if (m->next->isfree == 1) {
                            mw = (m->m + m->size);
                            if (mw == m->next) {
                                mi->size += m->next->size + MemoryInfoSize;
                                mi->next = m->next->next;
                                if (m->next->next != NULL) {
                                    m->next->next->back = mi;
                                }
                                mu->minfow = mu->minfow == m->next ? mi : mu->minfow;
                                mu->minfo = mu->minfo == m->next ? mi : mu->minfo;
                                next = 1;
                            }
                        }
                    }

                }
            }
        }

        if (next == 0) {
            mi = m->next;
            if (mi != NULL) {
                if (mi->isfree == 1) {
                    mw = (m->m + m->size);
                    if (mw == mi) {
                        m->size += mi->size + MemoryInfoSize;
                        m->next = mi->next;
                        if (mi->next != NULL) {
                            mi->next->back = m;
                        }
                        mu->minfow = mu->minfow == mi ? m : mu->minfow;
                        mu->minfo = mu->minfo == mi ? m : mu->minfo;
                    }
                }
            }
        }
    }
}

MemoryInfo *MemoryManagerRealloc(MemoryManager *mm, MemoryInfo *mi, MemorySize ms) {
    if (mi->size != ms) {
        int asd = mi->size;
        void *aa = mi->m;
        MemoryManagerFree(mm, mi);
        MemoryInfo *h = MemoryManagerMalloc(mm, ms);
        memcpy(h->m, aa, asd);
        return h;
    } else {
        return mi;
    }
}

void *MemoryManagerGetMemory(MemoryInfo *mi) {
    return mi->m;
}

void MemoryManagerSecuritySetMemory(MemoryInfo *mi, void *v, MemorySize ms) {
    if (mi->size >= ms) {
        memcpy(mi->m, v, ms);
    }
}

void MemoryManagerSetMemory(MemoryInfo *mi, void *v, MemorySize ms) {
    memcpy(mi->m, v, ms);
}

bool MemoryManagerCMP(MemoryInfo *mi, MemoryInfo *mk, MemorySize s) {
    if (memcmp(mi->m, mk->m, s) == 0) {
        return true;
    } else {
        return false;
    }
}

void MemoryManagerDestroy(MemoryManager *mm) {
    MemoryUnit *mu = mm->mu;
    while (true) {
        if (mu == NULL) {
            break;
        } else {
            MemoryUnit *m = mu;
            mu = mu->next;
            free(m);
            m = NULL;
        }
    }
    free(mm);
    mm = NULL;
}

void MemoryUnitCheck(MemoryUnit * u) {
  int qa = 0;
  MemoryUnit *mu = u;
  while (true) {
    if (mu == NULL) {
      break;
    } else {
      printf("MemoryManager Unit?????\n");
      MemoryInfo *fg = mu->minfo;
      while (true) {
        if (fg == NULL) {
          break;
        } else {
          printf("m:%p s:%d v:%p vm:%s vs:%d ", fg, MemoryUnitSize, fg->m,(char*)fg->m,fg->size);
          printf("back:%p ",fg->back);
          printf("isfree:%d m:%d s:%d\n", fg->isfree, qa, MemoryInfoSize);
          fg = fg->next;
          qa++;
        }
      }
      mu = mu->next;
    }
  }
}
