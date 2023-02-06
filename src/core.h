#ifndef _CORE_H
#define _CORE_H

#include "pico/util/queue.h"

//#include "common.h"

typedef struct {
    queue_t q0; //  
    queue_t q1; //
} core_t;

// public interface

void core_init(core_t *core);
void core_signal_start1(core_t *core);
void core_wait_start1(core_t *core);
bool core_stop1(core_t *core);
void core_wait_stop1(core_t *core);

#endif
