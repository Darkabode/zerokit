#ifndef __SHARED_HAVEGE_H_
#define __SHARED_HAVEGE_H_

#define COLLECT_SIZE 1024

typedef struct _havege_state
{
    int PT1, PT2, offset[2];
    int pool[COLLECT_SIZE];
    int WALK[8192];
} havege_state_t, *phavege_state_t;

#ifndef FN_HAVEGE_FILL
#define FN_HAVEGE_FILL havege_fill
#endif // FN_HAVEGE_FILL

#endif // __SHARED_HAVEGE_H_
