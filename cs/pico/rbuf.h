#ifndef _RBUF_H
#define _RBUF_H

#include "pico/stdlib.h"
#include "pico/mutex.h"

#include "common.h"
#include "channel.h"

#define RBUF_NUM_ENTRY 128  // lines in refresh buffer

typedef union {
    uint64_t f5_68;
    struct {
        union {
            struct {
                byte f5_8  : 4;
                byte f9_12 : 4;
            };
            byte f5_12 : 8;
        };
        byte f13_20: 8;
        byte f21_28: 8;
        byte f29_36: 8;
        byte f37_44: 8;
        byte f45_52: 8;
        byte f53_60: 8;
        byte f61_68: 8;
    };
} f5_68_t;

typedef struct {
    volatile byte msb;
    volatile byte lsb;
    volatile byte num_refresh_cycle;
    volatile byte refresh_cycle; 
    volatile byte dir_speed;
    volatile byte f0_4;
    volatile f5_68_t f5_68;
    volatile byte prev;
    volatile byte next;
} rbuf_entry_t; // entry in refresh buffer

typedef struct {
    mutex_t mu;
    channel_t *channel;
    volatile int8_t first, next;
    volatile rbuf_entry_t buf[RBUF_NUM_ENTRY];
} rbuf_t; // refresh buffer

// public interface
void rbuf_init(rbuf_t *rbuf, channel_t *channel);

bool rbuf_deregister(rbuf_t *rbuf, uint addr);

bool rbuf_get_dir(rbuf_t *rbuf, uint addr, bool *dir);
bool rbuf_set_dir(rbuf_t *rbuf, uint addr, bool dir);
bool rbuf_toggle_dir(rbuf_t *rbuf, uint addr, bool *dir);

bool rbuf_get_speed128(rbuf_t *rbuf, uint addr, byte *speed);
bool rbuf_set_speed128(rbuf_t *rbuf, uint addr, byte speed);

bool rbuf_get_fct(rbuf_t *rbuf, uint addr, byte no, bool *fct);
bool rbuf_set_fct(rbuf_t *rbuf, uint addr, byte no, bool fct);
bool rbuf_toggle_fct(rbuf_t *rbuf, uint addr, byte no, bool *fct);

bool rbuf_try_get_next(rbuf_t *rbuf, rbuf_entry_t *entry);

#endif
